#include "Speargun/SpearfishHarpoon.h"

#include "CollisionShape.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "HowToSpearfish.h"
#include "Net/UnrealNetwork.h"
#include "Speargun/SpeargunComponent.h"
#include "World/SpearfishOceanSubsystem.h"
#include "World/SpearfishVisualSubsystem.h"

namespace SpearfishHarpoonPrivate
{
	constexpr float WaterDrag = 0.45f;
	constexpr float WaterSink = 140.f;
	constexpr float AirGravity = 980.f;
	constexpr float ReturnSpeed = 2600.f;
	constexpr float ShaftLength = 95.f;
}

ASpearfishHarpoon::ASpearfishHarpoon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);
	SetNetUpdateFrequency(60.f);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void ASpearfishHarpoon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ASpearfishHarpoon, LaunchData, COND_InitialOnly);
	DOREPLIFETIME(ASpearfishHarpoon, Phase);
}

void ASpearfishHarpoon::BeginPlay()
{
	Super::BeginPlay();
	if (USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this))
	{
		using namespace SpearfishHarpoonPrivate;
		// Shaft along +X with the tip at the actor origin, so "stuck" looks embedded.
		Visuals->AddPart(this, Root, ESpearfishShape::Cylinder, FVector(-ShaftLength * 0.5f, 0, 0), FRotator(90, 0, 0), FVector(1.4f, 1.4f, ShaftLength), FLinearColor(0.75f, 0.76f, 0.8f), false);
		Visuals->AddPart(this, Root, ESpearfishShape::Cone, FVector(-2.f, 0, 0), FRotator(-90, 0, 0), FVector(4.f, 4.f, 8.f), FLinearColor(0.85f, 0.85f, 0.9f), false);
	}
}

FVector ASpearfishHarpoon::GetTipLocation() const
{
	return GetActorLocation();
}

void ASpearfishHarpoon::Launch(USpeargunComponent* InOwnerGun, const FVector& Velocity, float MaxRangeCm)
{
	OwnerGun = InOwnerGun;
	MaxRange = MaxRangeCm;
	SimVelocity = Velocity;
	TravelledCm = 0.f;
	FlightTime = 0.f;
	Phase = ESpearfishHarpoonPhase::Flying;
	LaunchData.Origin = GetActorLocation();
	LaunchData.Velocity = Velocity;
	bLocalFlight = true;
}

void ASpearfishHarpoon::OnRep_Launch()
{
	// Clients simulate the flight from the replicated launch; the server corrects on impact.
	SimVelocity = LaunchData.Velocity;
	SetActorLocationAndRotation(LaunchData.Origin, SimVelocity.Rotation());
	bLocalFlight = Phase == ESpearfishHarpoonPhase::Flying;
}

void ASpearfishHarpoon::PostNetReceiveLocationAndRotation()
{
	if (bLocalFlight && Phase == ESpearfishHarpoonPhase::Flying)
	{
		return;
	}
	Super::PostNetReceiveLocationAndRotation();
}

void ASpearfishHarpoon::StickTo(const FHitResult& Hit)
{
	Phase = ESpearfishHarpoonPhase::Stuck;
	bLocalFlight = false;
	SetActorLocation(Hit.ImpactPoint);
	if (UPrimitiveComponent* HitComponent = Hit.GetComponent())
	{
		AttachToComponent(HitComponent, FAttachmentTransformRules::KeepWorldTransform);
	}
	ForceNetUpdate();
}

void ASpearfishHarpoon::StartReturn()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Phase = ESpearfishHarpoonPhase::Returning;
	bLocalFlight = false;
	ForceNetUpdate();
}

void ASpearfishHarpoon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const bool bAuthority = HasAuthority();

	switch (Phase)
	{
	case ESpearfishHarpoonPhase::Flying:
		if (bAuthority || bLocalFlight)
		{
			SimulateFlight(DeltaSeconds, bAuthority);
		}
		break;

	case ESpearfishHarpoonPhase::Returning:
		if (bAuthority && OwnerGun)
		{
			const FVector Target = OwnerGun->GetMuzzleLocation();
			const FVector Delta = Target - GetActorLocation();
			const float Distance = static_cast<float>(Delta.Size());
			const float Step = SpearfishHarpoonPrivate::ReturnSpeed * DeltaSeconds;
			if (Distance <= FMath::Max(Step, 60.f))
			{
				OwnerGun->OnHarpoonReturned();
				return;
			}
			SetActorLocationAndRotation(GetActorLocation() + Delta / Distance * Step, (-Delta).Rotation());
		}
		else if (bAuthority && !OwnerGun)
		{
			Destroy();
		}
		break;

	default:
		break;
	}
}

void ASpearfishHarpoon::SimulateFlight(float DeltaSeconds, bool bAuthority)
{
	using namespace SpearfishHarpoonPrivate;
	FlightTime += DeltaSeconds;

	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	const bool bUnderwater = !Ocean || Ocean->IsUnderwater(GetActorLocation());
	if (bUnderwater)
	{
		SimVelocity *= FMath::Exp(-WaterDrag * DeltaSeconds);
		SimVelocity.Z -= WaterSink * DeltaSeconds;
	}
	else
	{
		SimVelocity.Z -= AirGravity * DeltaSeconds;
	}

	const FVector Start = GetActorLocation();
	const FVector End = Start + SimVelocity * DeltaSeconds;

	if (bAuthority)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SpearfishHarpoon), true, this);
		if (AActor* ShooterActor = GetOwner())
		{
			Params.AddIgnoredActor(ShooterActor);
		}
		Params.bReturnPhysicalMaterial = false;

		FHitResult Hit;
		if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Harpoon, FCollisionShape::MakeSphere(3.f), Params))
		{
			SetActorLocationAndRotation(Hit.ImpactPoint, SimVelocity.Rotation());
			if (OwnerGun)
			{
				OwnerGun->OnHarpoonHit(Hit);
			}
			return;
		}
	}

	TravelledCm += static_cast<float>((End - Start).Size());
	SetActorLocationAndRotation(End, SimVelocity.Rotation());

	if (bAuthority && (TravelledCm >= MaxRange || SimVelocity.Size() < 250.0 || FlightTime > 3.f))
	{
		if (OwnerGun)
		{
			OwnerGun->OnHarpoonExpired();
		}
	}
}
