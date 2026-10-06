#include "FishAI/SpearfishAmbientSchool.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Rules/SpearfishRulesTypes.h"
#include "World/SpearfishOceanSubsystem.h"
#include "World/SpearfishVisualSubsystem.h"

namespace SpearfishAmbientSchoolPrivate
{
	constexpr float FleeRadius = 450.f;
	constexpr float HomeRadius = 500.f;
	constexpr float NearDistance = 4000.f;
	constexpr float FarDistance = 9000.f;
	constexpr float CullDistance = 15000.f;
	constexpr int32 SeparationNeighbours = 4;
}

ASpearfishAmbientSchool::ASpearfishAmbientSchool()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void ASpearfishAmbientSchool::Configure(const FLinearColor& Color, int32 Count, float LengthCm, int32 Seed)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
		return;
	}
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	if (!Visuals || Count <= 0)
	{
		return;
	}
	Stream.Initialize(Seed);
	Length = FMath::Max(LengthCm, 3.f);
	Speed = Length * 12.f;
	Home = GetActorLocation();

	// Keep the school above the tallest seabed around home and below the surface.
	if (Ocean)
	{
		SeaLevelZ = Ocean->GetSeaLevel();
		float Highest = -1.0e6f;
		for (int32 X = -2; X <= 2; ++X)
		{
			for (int32 Y = -2; Y <= 2; ++Y)
			{
				const FVector Sample = Home + FVector(X * 250.0, Y * 250.0, 0.0);
				Highest = FMath::Max(Highest, Ocean->GetSeabedZ(Sample));
			}
		}
		SeabedZ = Highest + 40.f;
		Home.Z = FMath::Clamp(Home.Z, static_cast<double>(SeabedZ + 150.f), static_cast<double>(SeaLevelZ - 150.f));
	}
	else
	{
		SeaLevelZ = static_cast<float>(Home.Z) + 1000.f;
	}

	auto MakeInstancer = [this, Visuals](ESpearfishShape Shape, const FLinearColor& InColor)
	{
		UInstancedStaticMeshComponent* Instancer = NewObject<UInstancedStaticMeshComponent>(this);
		Instancer->SetStaticMesh(Visuals->GetShape(Shape));
		Instancer->SetMaterial(0, Visuals->GetColorMaterial(InColor));
		Instancer->SetMobility(EComponentMobility::Movable);
		Instancer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Instancer->SetCastShadow(false);
		Instancer->SetCanEverAffectNavigation(false);
		Instancer->SetupAttachment(Root);
		Instancer->RegisterComponent();
		AddInstanceComponent(Instancer);
		return Instancer;
	};
	Bodies = MakeInstancer(ESpearfishShape::Sphere, Color);
	Tails = MakeInstancer(ESpearfishShape::Cone, Color * 0.75f);

	Boids.SetNum(Count);
	BodyTransforms.SetNum(Count);
	TailTransforms.SetNum(Count);
	for (FBoid& Boid : Boids)
	{
		const FVector Direction = Stream.VRand();
		Boid.Position = Home + Direction * static_cast<double>(Stream.FRandRange(0.f, 150.f));
		Boid.Velocity = SpearfishRandom::Vector(Stream, -1.f, 1.f, 0.f, 0.f).GetSafeNormal() * static_cast<double>(Speed);
		Boid.Phase = Stream.FRandRange(0.f, 6.28f);
	}
	PushTransforms();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Bodies->AddInstance(BodyTransforms[Index], true);
		Tails->AddInstance(TailTransforms[Index], true);
	}
}

FVector ASpearfishAmbientSchool::GetViewLocation() const
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return PC && PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraLocation() : FVector(1.0e7);
}

void ASpearfishAmbientSchool::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Boids.Num() == 0 || !Bodies)
	{
		return;
	}

	// Distance LOD: full rate near the camera, slow far away, hidden beyond the fog.
	const FVector View = GetViewLocation();
	const double Distance = FVector::Dist(View, Home);
	const bool bVisible = Distance < SpearfishAmbientSchoolPrivate::CullDistance;
	Bodies->SetVisibility(bVisible);
	Tails->SetVisibility(bVisible);
	SetActorTickInterval(Distance < SpearfishAmbientSchoolPrivate::NearDistance ? 0.f : (Distance < SpearfishAmbientSchoolPrivate::FarDistance ? 0.1f : 0.5f));
	if (!bVisible)
	{
		return;
	}

	const float Step = FMath::Min(DeltaSeconds, 0.12f);
	Simulate(Step, View, Distance < SpearfishAmbientSchoolPrivate::HomeRadius + SpearfishAmbientSchoolPrivate::FleeRadius * 2.f);
	PushTransforms();
	Bodies->BatchUpdateInstancesTransforms(0, BodyTransforms, true, true, true);
	Tails->BatchUpdateInstancesTransforms(0, TailTransforms, true, true, true);
}

void ASpearfishAmbientSchool::Simulate(float DeltaSeconds, const FVector& Threat, bool bThreatNear)
{
	using namespace SpearfishAmbientSchoolPrivate;

	WanderTimer -= DeltaSeconds;
	if (WanderTimer <= 0.f)
	{
		WanderTimer = Stream.FRandRange(3.f, 6.f);
		Wander = SpearfishRandom::Vector(Stream, -HomeRadius, HomeRadius, -120.f, 120.f);
	}
	PanicTimer = FMath::Max(0.f, PanicTimer - DeltaSeconds);

	FVector Center = FVector::ZeroVector;
	FVector Heading = FVector::ZeroVector;
	for (const FBoid& Boid : Boids)
	{
		Center += Boid.Position;
		Heading += Boid.Velocity;
	}
	Center /= Boids.Num();
	Heading /= Boids.Num();
	const FVector Target = Home + Wander;
	const float SeparationDistance = Length * 2.2f;

	for (int32 Index = 0; Index < Boids.Num(); ++Index)
	{
		FBoid& Boid = Boids[Index];
		FVector Steer = (Center - Boid.Position) * 0.35 + (Heading - Boid.Velocity) * 0.8 + (Target - Boid.Position) * 0.25;

		// Separation against a few ring neighbours: O(N) and good enough for a decorative school.
		for (int32 Offset = 1; Offset <= SeparationNeighbours; ++Offset)
		{
			const FBoid& Other = Boids[(Index + Offset) % Boids.Num()];
			const FVector Away = Boid.Position - Other.Position;
			const double Gap = Away.Size();
			if (Gap > UE_KINDA_SMALL_NUMBER && Gap < SeparationDistance)
			{
				Steer += Away / Gap * (SeparationDistance - Gap) * 25.0;
			}
		}

		if (bThreatNear)
		{
			const FVector FromThreat = Boid.Position - Threat;
			const double ThreatDistance = FromThreat.Size();
			if (ThreatDistance < FleeRadius && ThreatDistance > UE_KINDA_SMALL_NUMBER)
			{
				Steer += FromThreat / ThreatDistance * 1400.0 * (1.0 - ThreatDistance / FleeRadius);
				PanicTimer = 1.5f;
			}
		}

		Boid.Velocity += Steer * DeltaSeconds;
		const float MaxSpeed = Speed * (PanicTimer > 0.f ? 2.8f : 1.f);
		const double CurrentSpeed = Boid.Velocity.Size();
		if (CurrentSpeed > MaxSpeed)
		{
			Boid.Velocity *= MaxSpeed / CurrentSpeed;
		}
		else if (CurrentSpeed < Speed * 0.4f)
		{
			Boid.Velocity = (CurrentSpeed > UE_KINDA_SMALL_NUMBER ? Boid.Velocity / CurrentSpeed : FVector::ForwardVector) * (Speed * 0.4f);
		}
		// Fish swim mostly level.
		Boid.Velocity.Z = FMath::Clamp(Boid.Velocity.Z, -0.45 * CurrentSpeed, 0.45 * CurrentSpeed);
		Boid.Position += Boid.Velocity * DeltaSeconds;
		Boid.Position.Z = FMath::Clamp(Boid.Position.Z, static_cast<double>(SeabedZ), static_cast<double>(SeaLevelZ - 40.f));
		Boid.Phase += DeltaSeconds * (10.f + static_cast<float>(Boid.Velocity.Size()) / FMath::Max(Length, 1.f));
	}
}

void ASpearfishAmbientSchool::PushTransforms()
{
	const FVector BodyScale(Length / 100.f, Length * 0.28f / 100.f, Length * 0.42f / 100.f);
	const FVector TailScale(Length * 0.4f / 100.f, Length * 0.05f / 100.f, Length * 0.32f / 100.f);
	const FQuat TailBase = FRotator(-90.f, 0.f, 0.f).Quaternion();
	for (int32 Index = 0; Index < Boids.Num(); ++Index)
	{
		const FBoid& Boid = Boids[Index];
		const FVector Forward = Boid.Velocity.GetSafeNormal(UE_KINDA_SMALL_NUMBER, FVector::ForwardVector);
		const FQuat Rotation = Forward.Rotation().Quaternion();
		BodyTransforms[Index] = FTransform(Rotation, Boid.Position, BodyScale);
		const FQuat Wiggle = FRotator(0.f, FMath::Sin(Boid.Phase) * 25.f, 0.f).Quaternion();
		TailTransforms[Index] = FTransform(Rotation * Wiggle * TailBase, Boid.Position - Forward * (Length * 0.62), TailScale);
	}
}
