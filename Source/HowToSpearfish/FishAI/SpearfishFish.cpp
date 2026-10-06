#include "FishAI/SpearfishFish.h"

#include "CollisionShape.h"
#include "Components/SphereComponent.h"
#include "Core/SpearfishGameTypes.h"
#include "Data/SpearfishDataRegistry.h"
#include "Diving/SpearfishCharacter.h"
#include "Diving/SpearfishOxygenComponent.h"
#include "Engine/World.h"
#include "FishAI/SpearfishFishBodyComponent.h"
#include "FishAI/SpearfishFishSubsystem.h"
#include "HowToSpearfish.h"
#include "Net/UnrealNetwork.h"
#include "Speargun/SpeargunComponent.h"
#include "World/SpearfishOceanSubsystem.h"

#define LOCTEXT_NAMESPACE "SpearfishFish"

namespace SpearfishFishPrivate
{
	constexpr float SurfaceMargin = 40.f;
	constexpr float SeabedMargin = 25.f;
	constexpr float BoundsMargin = 400.f;

	FVector RotateTowards(const FVector& Current, const FVector& Target, float MaxRadians)
	{
		const FVector From = Current.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		const FVector To = Target.GetSafeNormal(UE_SMALL_NUMBER, From);
		const float Dot = FMath::Clamp(static_cast<float>(FVector::DotProduct(From, To)), -1.f, 1.f);
		const float Angle = FMath::Acos(Dot);
		if (Angle <= MaxRadians || Angle < UE_KINDA_SMALL_NUMBER)
		{
			return To;
		}
		const float Alpha = MaxRadians / Angle;
		return FMath::Lerp(From, To, static_cast<double>(Alpha)).GetSafeNormal(UE_SMALL_NUMBER, To);
	}
}

ASpearfishFish::ASpearfishFish()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(10.f);
	SetMinNetUpdateFrequency(2.f);
	SetNetCullDistanceSquared(FMath::Square(9000.0));

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(15.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_Pawn);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Harpoon, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Interact, ECR_Block);
	Collision->SetCanEverAffectNavigation(false);
	RootComponent = Collision;

	Body = CreateDefaultSubobject<USpearfishFishBodyComponent>(TEXT("Body"));
	Body->SetupAttachment(Collision);
}

void ASpearfishFish::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ASpearfishFish, SpeciesId, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ASpearfishFish, LengthCm, COND_InitialOnly);
	DOREPLIFETIME(ASpearfishFish, NetState);
	DOREPLIFETIME(ASpearfishFish, HookedBy);
	DOREPLIFETIME(ASpearfishFish, bHooked);
}

void ASpearfishFish::InitializeFish(FName InSpeciesId, float InLengthCm, const FVector& InHome, int32 InSchoolId, int32 InSeed)
{
	SpeciesId = InSpeciesId;
	LengthCm = InLengthCm;
	Home = InHome;
	WanderTarget = InHome;
	SchoolId = InSchoolId;
	Stream.Initialize(InSeed);
	SpeciesCache = nullptr;
	SetupFromSpecies();
	UpdateNetState();
}

void ASpearfishFish::BeginPlay()
{
	Super::BeginPlay();
	if (USpearfishFishSubsystem* Subsystem = USpearfishFishSubsystem::Get(this))
	{
		Subsystem->RegisterFish(this);
	}
	SetupFromSpecies();
	NextThink = GetWorld()->GetTimeSeconds() + Stream.FRandRange(0.f, 0.3f);
}

void ASpearfishFish::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USpearfishFishSubsystem* Subsystem = USpearfishFishSubsystem::Get(this))
	{
		Subsystem->LeaveSchool(SchoolId, this);
		Subsystem->UnregisterFish(this);
	}
	Super::EndPlay(EndPlayReason);
}

void ASpearfishFish::OnRep_Identity()
{
	SetupFromSpecies();
}

const FSpearfishFishSpeciesDef* ASpearfishFish::GetSpecies() const
{
	if (!SpeciesCache && !SpeciesId.IsNone())
	{
		if (const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this))
		{
			const_cast<ASpearfishFish*>(this)->SpeciesCache = Registry->FindFish(SpeciesId);
		}
	}
	return SpeciesCache;
}

void ASpearfishFish::SetupFromSpecies()
{
	const FSpearfishFishSpeciesDef* Species = GetSpecies();
	if (!Species || !HasActorBegunPlay())
	{
		return;
	}
	Collision->SetSphereRadius(FMath::Max(LengthCm * 0.22f, 8.f));
	MaxStamina = SpearfishFish::MaxStamina(Species->Behavior.Stamina, GetSizeFraction());
	Stamina = MaxStamina;
	Body->Build(*Species, LengthCm);
}

float ASpearfishFish::GetWeightKg() const
{
	const FSpearfishFishSpeciesDef* Species = GetSpecies();
	return Species ? SpearfishFish::WeightForLength(LengthCm, Species->WeightCoef) : 0.f;
}

float ASpearfishFish::GetSizeFraction() const
{
	const FSpearfishFishSpeciesDef* Species = GetSpecies();
	return Species ? SpearfishFish::SizeFraction(LengthCm, Species->MinLengthCm, Species->MaxLengthCm) : 0.5f;
}

bool ASpearfishFish::IsCatchable() const
{
	const FSpearfishFishSpeciesDef* Species = GetSpecies();
	return Species && Species->bCatchable;
}

// ------------------------------------------------------------------------------------- Tick

void ASpearfishFish::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GetSpecies())
	{
		return;
	}

	if (HasAuthority())
	{
		SimulateServer(DeltaSeconds);
	}
	else
	{
		SmoothClient(DeltaSeconds);
	}

	const FSpearfishFishSpeciesDef* Species = GetSpecies();
	const float Speed = HasAuthority() ? CurrentSpeed : NetState.Speed;
	Body->SetMotion(Species->Behavior.BurstSpeedCm > 0.f ? Speed / Species->Behavior.BurstSpeedCm : 0.f,
		SpearfishFish::IsPanicked(NetState.Mind), NetState.Mind == ESpearfishFishMind::Exhausted);
}

void ASpearfishFish::SmoothClient(float DeltaSeconds)
{
	// Extrapolate along the last known heading and ease toward the replicated position.
	const FVector Target = FVector(NetState.Location) + NetState.Rotation.Vector() * (NetState.Speed * 0.1f);
	const FVector Current = GetActorLocation();
	const FVector NewLocation = (FVector::DistSquared(Current, Target) > FMath::Square(600.0))
		? Target
		: FMath::VInterpTo(Current, Target, DeltaSeconds, 6.f);
	const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), NetState.Rotation, DeltaSeconds, 8.f);
	SetActorLocationAndRotation(NewLocation, NewRotation);
}

void ASpearfishFish::SimulateServer(float DeltaSeconds)
{
	if (bCaught)
	{
		return;
	}
	TimeInState += DeltaSeconds;

	const float Time = GetWorld()->GetTimeSeconds();
	if (Time >= NextThink)
	{
		Think();
		NextThink = Time + Stream.FRandRange(0.15f, 0.3f);
	}

	float DesiredSpeed = 0.f;
	const FVector DesiredDirection = ComputeDesiredDirection(DesiredSpeed);
	Integrate(DeltaSeconds, DesiredDirection, DesiredSpeed);

	if (bHooked)
	{
		// Fighting the line burns stamina; bursts against tension burn it fastest.
		Stamina = FMath::Max(0.f, Stamina - (0.3f + LastTension * 0.0045f) * DeltaSeconds);
	}
	else if (Mind != ESpearfishFishMind::Exhausted)
	{
		Stamina = FMath::Min(MaxStamina, Stamina + 0.15f * DeltaSeconds);
	}

	UpdateNetState();
}

void ASpearfishFish::UpdateNetState()
{
	NetState.Location = GetActorLocation();
	NetState.Rotation = GetActorRotation();
	NetState.Speed = CurrentSpeed;
	NetState.Mind = Mind;
}

void ASpearfishFish::SetMind(ESpearfishFishMind NewMind)
{
	if (NewMind == Mind)
	{
		return;
	}
	const ESpearfishFishMind Previous = Mind;
	Mind = NewMind;
	TimeInState = 0.f;

	USpearfishFishSubsystem* Subsystem = USpearfishFishSubsystem::Get(this);
	if (Mind == ESpearfishFishMind::Flee)
	{
		const FSpearfishFishSpeciesDef* Species = GetSpecies();
		FVector Away = (GetActorLocation() - ThreatLocation).GetSafeNormal(UE_SMALL_NUMBER, Stream.VRand());
		if (Species && Species->Behavior.bSeeksCover && bHasCover)
		{
			Away = (Away + (CoverPoint - GetActorLocation()).GetSafeNormal() * 1.5).GetSafeNormal();
		}
		FleeDirection = Away;
		if (Subsystem && SchoolId != INDEX_NONE)
		{
			Subsystem->PanicSchool(SchoolId, ThreatLocation);
		}
	}

	// Panicking and fighting fish update the network faster.
	const bool bFast = SpearfishFish::IsPanicked(Mind) || Mind == ESpearfishFishMind::Attack || Mind == ESpearfishFishMind::Hunt
		|| Mind == ESpearfishFishMind::Lunge || Mind == ESpearfishFishMind::Exhausted;
	SetNetUpdateFrequency(bFast ? 30.f : 10.f);
	if (bFast != (SpearfishFish::IsPanicked(Previous) || Previous == ESpearfishFishMind::Attack))
	{
		ForceNetUpdate();
	}
}

void ASpearfishFish::Startle(const FVector& From)
{
	if (bHooked || Mind == ESpearfishFishMind::Exhausted)
	{
		return;
	}
	const FSpearfishFishSpeciesDef* Species = GetSpecies();
	if (!Species || Species->Archetype == ESpearfishFishArchetype::Predator || Species->Archetype == ESpearfishFishArchetype::Drifter)
	{
		return;
	}
	ThreatLocation = From;
	SetMind(ESpearfishFishMind::Flee);
}

// ------------------------------------------------------------------------------------ Think

void ASpearfishFish::Think()
{
	const FSpearfishFishSpeciesDef* Species = GetSpecies();
	USpearfishFishSubsystem* Subsystem = USpearfishFishSubsystem::Get(this);
	UWorld* World = GetWorld();
	if (!Species || !Subsystem || !World)
	{
		return;
	}
	const FSpearfishFishBehavior& B = Species->Behavior;
	const FVector Location = GetActorLocation();
	const float Time = World->GetTimeSeconds();

	FSpearfishFishSenses Senses;
	Senses.Archetype = Species->Archetype;
	Senses.bHooked = bHooked;
	Senses.StaminaFraction = GetStaminaFraction();
	Senses.CalmDownSeconds = B.CalmDownSeconds;
	Senses.TimeInState = TimeInState;
	Senses.bHasSchool = SchoolId != INDEX_NONE;
	Senses.AttackRadius = B.AttackRadiusCm;
	Senses.HuntRadius = B.HuntRadiusCm;

	// Divers: fast, lit divers are noticed from further away.
	float DiverDistance = UE_BIG_NUMBER;
	ASpearfishCharacter* Diver = Subsystem->FindNearestDiver(Location, 4000.f, DiverDistance);
	DiverTarget = Diver;
	Senses.DiverDistance = DiverDistance;
	ThreatDistance = UE_BIG_NUMBER;
	bThreatIsDiver = false;
	if (Diver)
	{
		const float DiverSpeed = static_cast<float>(Diver->GetVelocity().Size());
		Senses.DetectionRadius = SpearfishFish::DetectionRadius(B.DetectionRadiusCm, DiverSpeed / 450.f, Diver->IsLightOn(), B.LightSensitivity);
		ThreatDistance = DiverDistance;
		ThreatLocation = Diver->GetActorLocation();
		bThreatIsDiver = true;
	}
	else
	{
		Senses.DetectionRadius = B.DetectionRadiusCm;
	}

	// Predators are a threat to their prey, regardless of divers.
	if (Species->bCatchable)
	{
		if (const ASpearfishFish* Predator = Subsystem->FindPredatorThreat(this, B.DetectionRadiusCm * 1.2f))
		{
			const float PredatorDistance = static_cast<float>(FVector::Dist(Location, Predator->GetActorLocation()));
			if (PredatorDistance < ThreatDistance)
			{
				ThreatDistance = PredatorDistance;
				ThreatLocation = Predator->GetActorLocation();
				bThreatIsDiver = false;
				Senses.DetectionRadius = FMath::Max(Senses.DetectionRadius, B.DetectionRadiusCm * 1.2f);
			}
		}
	}
	Senses.ThreatDistance = ThreatDistance;

	// Curious species (barracuda) only spook when crowded.
	if (B.bCurious && bThreatIsDiver)
	{
		Senses.DetectionRadius *= 0.45f;
	}

	Senses.bStartled = Subsystem->WasStartled(Location, 0.5f);
	if (const FSpearfishSchool* School = Subsystem->GetSchool(SchoolId))
	{
		if (School->PanicUntil > Time)
		{
			Senses.bStartled = true;
			ThreatLocation = School->PanicFrom;
		}
	}

	FVector Cover;
	bHasCover = Subsystem->FindNearestCover(Location, 1600.f, Cover);
	if (bHasCover)
	{
		CoverPoint = Cover;
		Senses.bHasCover = true;
		Senses.bInCover = FVector::DistSquared(Location, Cover) < FMath::Square(180.0);
	}

	if (Species->Archetype == ESpearfishFishArchetype::Predator)
	{
		Senses.bProvoked = ProvokedUntil > Time;
		ASpearfishFish* Prey = nullptr;
		if (B.bAttractedByStruggle)
		{
			Prey = Subsystem->FindStrugglingFish(Location, B.HuntRadiusCm * 1.5f);
		}
		if (!Prey && Time > AttackCooldownUntil)
		{
			Prey = Subsystem->FindPrey(this, B.HuntRadiusCm);
		}
		PreyTarget = Prey;
		Senses.PreyDistance = Prey ? static_cast<float>(FVector::Dist(Location, Prey->GetActorLocation())) : UE_BIG_NUMBER;
		if (Senses.bProvoked && ProvokedBy.IsValid())
		{
			DiverTarget = ProvokedBy;
			Senses.DiverDistance = static_cast<float>(FVector::Dist(Location, ProvokedBy->GetActorLocation()));
		}
		if (Time < AttackCooldownUntil)
		{
			Senses.bProvoked = false;
		}
	}
	if (Species->Archetype == ESpearfishFishArchetype::Ambusher && Time < AttackCooldownUntil)
	{
		Senses.AttackRadius = 0.f;
	}

	ESpearfishFishMind NewMind = SpearfishFish::DecideMind(Mind, Senses);

	// Reef browsers spend some calm time feeding.
	if (NewMind == ESpearfishFishMind::Wander && Species->Archetype == ESpearfishFishArchetype::Reef && Stream.FRand() < 0.06f)
	{
		NewMind = ESpearfishFishMind::Feed;
	}
	SetMind(NewMind);

	// Rays and drifters sting when a diver blunders into them.
	if (Diver && (Species->Archetype == ESpearfishFishArchetype::Drifter || Species->Archetype == ESpearfishFishArchetype::Ray))
	{
		const bool bTooClose = DiverDistance < B.AttackRadiusCm + Collision->GetScaledSphereRadius();
		const bool bCornered = Species->Archetype == ESpearfishFishArchetype::Ray ? DiverDistance < B.AttackRadiusCm && Diver->GetVelocity().Size() > 250.0 : bTooClose;
		if (bCornered)
		{
			TryAttackDiver(Diver, DiverDistance);
		}
	}

	// Obstacle feeler along the current heading.
	AvoidNormal = FVector::ZeroVector;
	const FVector Heading = Velocity.GetSafeNormal(UE_SMALL_NUMBER, GetActorForwardVector());
	const float Probe = CurrentSpeed * 0.9f + 120.f;
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpearfishFishFeeler), false, this);
	FHitResult Hit;
	if (World->SweepSingleByObjectType(Hit, Location, Location + Heading * Probe, FQuat::Identity, Objects,
		FCollisionShape::MakeSphere(Collision->GetScaledSphereRadius()), Params))
	{
		AvoidNormal = Hit.ImpactNormal;
	}

	// Wander target refresh.
	if (FVector::DistSquared(Location, WanderTarget) < FMath::Square(150.0) || Stream.FRand() < 0.02f)
	{
		const float Radius = Species->Archetype == ESpearfishFishArchetype::Pelagic ? 2500.f : 900.f;
		WanderTarget = Home + FVector(Stream.FRandRange(-Radius, Radius), Stream.FRandRange(-Radius, Radius), Stream.FRandRange(-200.f, 200.f));
	}
}

void ASpearfishFish::TryAttackDiver(ASpearfishCharacter* Diver, float Distance)
{
	const FSpearfishFishSpeciesDef* Species = GetSpecies();
	const float Time = GetWorld()->GetTimeSeconds();
	if (!Species || !Diver || Time < AttackCooldownUntil || Diver->IsBlackedOut())
	{
		return;
	}
	const FSpearfishFishBehavior& B = Species->Behavior;
	AttackCooldownUntil = Time + (Species->Archetype == ESpearfishFishArchetype::Drifter ? 3.f : 6.f);

	const bool bSting = Species->Archetype == ESpearfishFishArchetype::Drifter || Species->Archetype == ESpearfishFishArchetype::Ray;
	if (USpearfishOxygenComponent* Oxygen = Diver->GetOxygen())
	{
		Oxygen->ApplyShock(B.AttackShock, bSting);
	}
	const FVector Push = (Diver->GetActorLocation() - GetActorLocation()).GetSafeNormal() * 420.0 + FVector(0, 0, 80.0);
	Diver->LaunchCharacter(Push, true, true);

	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FText Name = Registry ? Registry->GetDisplayName(SpeciesId) : FText::FromName(SpeciesId);
	Diver->NotifyOwner(FText::Format(bSting ? LOCTEXT("Stung", "Stung by a {0}! Breathing hard...") : LOCTEXT("Bitten", "A {0} bit you! You lost air."), Name),
		ESpearfishNoticeType::Danger);

	// Predators back off after a bite instead of chain-attacking.
	ProvokedUntil = 0.f;
	if (Species->Archetype == ESpearfishFishArchetype::Predator || Species->Archetype == ESpearfishFishArchetype::Ambusher)
	{
		ThreatLocation = Diver->GetActorLocation();
		SetMind(ESpearfishFishMind::Wander);
	}
}

// --------------------------------------------------------------------------------- Steering

FVector ASpearfishFish::ComputeDesiredDirection(float& OutSpeed)
{
	using namespace SpearfishFishPrivate;
	const FSpearfishFishSpeciesDef* Species = GetSpecies();
	const FSpearfishFishBehavior& B = Species->Behavior;
	const FVector Location = GetActorLocation();
	const float Time = GetWorld()->GetTimeSeconds();
	USpearfishFishSubsystem* Subsystem = USpearfishFishSubsystem::Get(this);

	FVector Desired = Velocity.GetSafeNormal(UE_SMALL_NUMBER, GetActorForwardVector());
	OutSpeed = B.CruiseSpeedCm * 0.5f;

	switch (Mind)
	{
	case ESpearfishFishMind::Wander:
		Desired = (WanderTarget - Location).GetSafeNormal();
		OutSpeed = B.CruiseSpeedCm * 0.6f;
		break;

	case ESpearfishFishMind::School:
	{
		const FSpearfishSchool* School = Subsystem ? Subsystem->GetSchool(SchoolId) : nullptr;
		if (School)
		{
			FVector Separation = FVector::ZeroVector;
			for (const TWeakObjectPtr<ASpearfishFish>& Mate : School->Members)
			{
				const ASpearfishFish* Other = Mate.Get();
				if (!Other || Other == this)
				{
					continue;
				}
				const FVector Offset = Location - Other->GetActorLocation();
				const double DistanceSq = Offset.SizeSquared();
				const double Spacing = FMath::Max(LengthCm * 1.6, 45.0);
				if (DistanceSq < Spacing * Spacing && DistanceSq > 1.0)
				{
					Separation += Offset / FMath::Sqrt(DistanceSq);
				}
			}
			Desired = (School->Target - Location).GetSafeNormal() * 0.6
				+ School->Heading * 0.7
				+ (School->Center - Location).GetSafeNormal() * 0.35
				+ Separation * 1.3;
			Desired = Desired.GetSafeNormal();
		}
		OutSpeed = B.CruiseSpeedCm;
		break;
	}

	case ESpearfishFishMind::Feed:
	{
		const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
		const float Seabed = Ocean ? Ocean->GetSeabedZ(Location) : static_cast<float>(Location.Z) - 100.f;
		const FVector Target(WanderTarget.X, WanderTarget.Y, Seabed + 35.f);
		Desired = (Target - Location).GetSafeNormal();
		OutSpeed = B.CruiseSpeedCm * 0.25f;
		break;
	}

	case ESpearfishFishMind::Alert:
	{
		const FVector Away = (Location - ThreatLocation).GetSafeNormal();
		if (B.bCurious)
		{
			// Hover at a respectful distance, facing the diver.
			const float Distance = static_cast<float>(FVector::Dist(Location, ThreatLocation));
			Desired = Distance < 420.f ? Away : (Distance > 560.f ? -Away : FVector::CrossProduct(Away, FVector::UpVector));
			OutSpeed = B.CruiseSpeedCm * 0.5f;
		}
		else
		{
			Desired = (Away + FVector::CrossProduct(Away, FVector::UpVector) * 0.4).GetSafeNormal();
			OutSpeed = B.CruiseSpeedCm * 0.9f;
		}
		break;
	}

	case ESpearfishFishMind::Flee:
	{
		const float Zig = FMath::Sin(Time * 6.f + static_cast<float>(GetUniqueID() % 17)) * 0.35f;
		Desired = (FleeDirection + FVector::CrossProduct(FleeDirection, FVector::UpVector) * Zig).GetSafeNormal();
		if (B.bSeeksCover && bHasCover)
		{
			Desired = (Desired + (CoverPoint - Location).GetSafeNormal() * 1.2).GetSafeNormal();
		}
		OutSpeed = B.BurstSpeedCm;
		break;
	}

	case ESpearfishFishMind::Hide:
	{
		const FVector ToCover = CoverPoint - Location;
		Desired = ToCover.SizeSquared() > 100.0 * 100.0 ? ToCover.GetSafeNormal() : (ThreatLocation - Location).GetSafeNormal() * 0.1;
		OutSpeed = ToCover.SizeSquared() > 100.0 * 100.0 ? B.CruiseSpeedCm * 0.6f : 15.f;
		break;
	}

	case ESpearfishFishMind::Hunt:
	{
		if (ASpearfishFish* Prey = PreyTarget.Get())
		{
			const FVector Lead = Prey->GetActorLocation() + Prey->GetFishVelocity() * 0.4;
			Desired = (Lead - Location).GetSafeNormal();
			const float Distance = static_cast<float>(FVector::Dist(Location, Prey->GetActorLocation()));
			OutSpeed = Distance > 600.f ? B.CruiseSpeedCm * 1.3f : B.BurstSpeedCm;
			if (Distance < 90.f + LengthCm * 0.3f)
			{
				Prey->GetEaten(this);
				PreyTarget.Reset();
				AttackCooldownUntil = Time + 25.f;
				SetMind(ESpearfishFishMind::Wander);
			}
		}
		break;
	}

	case ESpearfishFishMind::Attack:
	{
		if (ASpearfishCharacter* Diver = DiverTarget.Get())
		{
			Desired = (Diver->GetActorLocation() - Location).GetSafeNormal();
			OutSpeed = B.BurstSpeedCm;
			const float Distance = static_cast<float>(FVector::Dist(Location, Diver->GetActorLocation()));
			if (Distance < 130.f + LengthCm * 0.3f)
			{
				TryAttackDiver(Diver, Distance);
			}
		}
		break;
	}

	case ESpearfishFishMind::Lunge:
	{
		ASpearfishCharacter* Diver = DiverTarget.Get();
		if (Diver && FVector::Dist(Home, Location) < 220.0)
		{
			Desired = (Diver->GetActorLocation() - Location).GetSafeNormal();
			OutSpeed = B.BurstSpeedCm;
			const float Distance = static_cast<float>(FVector::Dist(Location, Diver->GetActorLocation()));
			if (Distance < 110.f + LengthCm * 0.2f)
			{
				TryAttackDiver(Diver, Distance);
			}
		}
		else
		{
			Desired = (Home - Location).GetSafeNormal();
			OutSpeed = B.CruiseSpeedCm;
		}
		break;
	}

	case ESpearfishFishMind::Drift:
		Desired = FVector(FMath::Sin(Time * 0.21f + Home.X), FMath::Cos(Time * 0.17f + Home.Y), FMath::Sin(Time * 0.5f) * 0.6).GetSafeNormal();
		OutSpeed = B.CruiseSpeedCm;
		break;

	case ESpearfishFishMind::Hooked:
	{
		if (Time >= NextZigzag)
		{
			// Escape: away from the diver, down and toward cover, with sudden direction changes.
			const FVector AwayFromLine = (Location - LineAnchor).GetSafeNormal(UE_SMALL_NUMBER, Stream.VRand());
			FVector Escape = AwayFromLine + FVector(0, 0, -0.35) + Stream.VRand() * 0.8;
			if (B.bSeeksCover && bHasCover)
			{
				Escape += (CoverPoint - Location).GetSafeNormal() * 1.4;
			}
			EscapeDirection = Escape.GetSafeNormal();
			NextZigzag = Time + Stream.FRandRange(0.45f, 1.2f);
		}
		Desired = EscapeDirection;
		OutSpeed = B.BurstSpeedCm * (0.45f + 0.55f * GetStaminaFraction());
		break;
	}

	case ESpearfishFishMind::Exhausted:
		Desired = FVector(Velocity.X, Velocity.Y, -40.0).GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector);
		OutSpeed = 18.f;
		break;

	default:
		break;
	}

	// Obstacles, depth band and bounds.
	if (!AvoidNormal.IsNearlyZero() && Mind != ESpearfishFishMind::Exhausted)
	{
		Desired = (Desired + AvoidNormal * 1.6).GetSafeNormal();
	}
	if (const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this))
	{
		const float DepthM = Ocean->GetDepthMeters(Location);
		const float SeabedClearance = static_cast<float>(Location.Z) - Ocean->GetSeabedZ(Location);
		if (!bHooked)
		{
			if (DepthM < Species->MinDepthM * 0.8f)
			{
				Desired.Z -= 0.6;
			}
			else if (DepthM > Species->MaxDepthM + 4.f)
			{
				Desired.Z += 0.6;
			}
		}
		if (SeabedClearance < 60.f && Mind != ESpearfishFishMind::Feed)
		{
			Desired.Z += 0.5;
		}
		if (Ocean->HasTerrain() && !Ocean->GetTerrain().IsInsideBounds(static_cast<float>(Location.X), static_cast<float>(Location.Y), BoundsMargin))
		{
			Desired += FVector(-Location.X, -Location.Y, 0.0).GetSafeNormal() * 1.5;
		}
		Desired = Desired.GetSafeNormal(UE_SMALL_NUMBER, GetActorForwardVector());
	}
	return Desired;
}

void ASpearfishFish::Integrate(float DeltaSeconds, const FVector& DesiredDirection, float DesiredSpeed)
{
	const FSpearfishFishSpeciesDef* Species = GetSpecies();
	const float TurnRate = FMath::DegreesToRadians(Species->Behavior.TurnRateDeg) * (SpearfishFish::IsPanicked(Mind) ? 1.6f : 1.f);
	const FVector Heading = SpearfishFishPrivate::RotateTowards(Velocity.GetSafeNormal(UE_SMALL_NUMBER, GetActorForwardVector()), DesiredDirection, TurnRate * DeltaSeconds);
	const float Response = (Mind == ESpearfishFishMind::Flee || Mind == ESpearfishFishMind::Hooked || Mind == ESpearfishFishMind::Attack) ? 6.f : 2.f;
	CurrentSpeed = FMath::FInterpTo(CurrentSpeed, DesiredSpeed, DeltaSeconds, Response);
	Velocity = Heading * CurrentSpeed;

	FVector Location = GetActorLocation() + Velocity * DeltaSeconds;
	KeepInWater(Location);

	FRotator Rotation = Heading.Rotation();
	Rotation.Pitch = FMath::Clamp(Rotation.Pitch, -45.0, 45.0);
	Rotation.Roll = 0.0;
	SetActorLocationAndRotation(Location, Rotation);
}

void ASpearfishFish::KeepInWater(FVector& Location) const
{
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	if (!Ocean)
	{
		return;
	}
	const double Top = Ocean->GetSeaLevel() - SpearfishFishPrivate::SurfaceMargin;
	const double Bottom = Ocean->GetSeabedZ(Location) + SpearfishFishPrivate::SeabedMargin;
	Location.Z = FMath::Clamp(Location.Z, FMath::Min(Bottom, Top), Top);
}

// ----------------------------------------------------------------------------- Line contact

bool ASpearfishFish::TryHook(ASpearfishCharacter* Shooter, const FHitResult& Hit, float GunPower, float& OutShotQuality)
{
	if (!IsCatchable() || bHooked || bCaught || !Shooter)
	{
		return false;
	}

	// Where did the shaft land? Head shots shorten the fight and make better fish.
	const FVector Local = GetActorTransform().InverseTransformPosition(Hit.ImpactPoint);
	const float AlongBody = static_cast<float>(Local.X) / FMath::Max(LengthCm * 0.5f, 1.f);
	float ZoneMultiplier = 1.f;
	OutShotQuality = 0.6f;
	if (AlongBody > 0.35f)
	{
		ZoneMultiplier = 2.f;
		OutShotQuality = 1.f;
	}
	else if (AlongBody < -0.4f)
	{
		ZoneMultiplier = 0.6f;
		OutShotQuality = 0.25f;
	}
	Stamina = FMath::Max(0.f, Stamina - SpearfishFish::HitStaminaDamage(GunPower, ZoneMultiplier, GetSizeFraction(), MaxStamina));

	bHooked = true;
	HookedBy = Shooter;
	LineAnchor = Shooter->GetActorLocation();
	NextZigzag = 0.f;
	if (USpearfishFishSubsystem* Subsystem = USpearfishFishSubsystem::Get(this))
	{
		Subsystem->PanicSchool(SchoolId, Shooter->GetActorLocation());
		Subsystem->LeaveSchool(SchoolId, this);
		Subsystem->ReportNoise(GetActorLocation(), 900.f);
	}
	SchoolId = INDEX_NONE;
	SetMind(Stamina <= 0.f ? ESpearfishFishMind::Exhausted : ESpearfishFishMind::Hooked);
	ForceNetUpdate();
	return true;
}

void ASpearfishFish::Provoke(ASpearfishCharacter* By)
{
	const FSpearfishFishSpeciesDef* Species = GetSpecies();
	if (!Species || !By)
	{
		return;
	}
	ProvokedBy = By;
	ProvokedUntil = GetWorld()->GetTimeSeconds() + 12.f;
	ThreatLocation = By->GetActorLocation();
	if (Species->Archetype != ESpearfishFishArchetype::Predator && Species->Archetype != ESpearfishFishArchetype::Drifter)
	{
		SetMind(ESpearfishFishMind::Flee);
	}
}

float ASpearfishFish::GetLinePullAway(const FVector& LineDirectionFromAnchor) const
{
	const FSpearfishFishSpeciesDef* Species = GetSpecies();
	if (!Species || !bHooked)
	{
		return 0.f;
	}
	const float Pull = SpearfishFish::PullForce(Species->Behavior.Strength, GetSizeFraction(), GetStaminaFraction());
	const float Alignment = FMath::Max(0.f, static_cast<float>(FVector::DotProduct(Velocity.GetSafeNormal(), LineDirectionFromAnchor)));
	return Mind == ESpearfishFishMind::Exhausted ? Pull * 0.3f : Pull * (0.35f + 0.65f * Alignment);
}

void ASpearfishFish::ApplyLineConstraint(const FVector& Anchor, float MaxDistance, float Tension, float ReelSpeed, float DeltaSeconds)
{
	LineAnchor = Anchor;
	LastTension = Tension;

	FVector Location = GetActorLocation();
	FVector ToFish = Location - Anchor;
	double Distance = ToFish.Size();
	if (Distance < 1.0)
	{
		return;
	}
	const FVector Direction = ToFish / Distance;

	// Exhausted fish are reeled in directly.
	if (ReelSpeed > 0.f && Mind == ESpearfishFishMind::Exhausted)
	{
		Distance = FMath::Max(80.0, Distance - static_cast<double>(ReelSpeed * DeltaSeconds));
		Location = Anchor + Direction * Distance;
	}
	if (Distance > MaxDistance)
	{
		Location = Anchor + Direction * static_cast<double>(MaxDistance);
		const double Outward = FVector::DotProduct(Velocity, Direction);
		if (Outward > 0.0)
		{
			Velocity -= Direction * Outward;
		}
	}
	KeepInWater(Location);
	SetActorLocation(Location);

	if (Stamina <= MaxStamina * SpearfishFish::ExhaustedThreshold && Mind == ESpearfishFishMind::Hooked)
	{
		SetMind(ESpearfishFishMind::Exhausted);
	}
	UpdateNetState();
}

void ASpearfishFish::ReleaseFromLine(bool bEscaped)
{
	if (!bHooked)
	{
		return;
	}
	bHooked = false;
	HookedBy = nullptr;
	LastTension = 0.f;
	ThreatLocation = LineAnchor;
	if (bEscaped)
	{
		// A wounded fish bolts for cover; a spent one recovers first.
		Stamina = FMath::Max(Stamina, MaxStamina * 0.25f);
		SetMind(ESpearfishFishMind::Flee);
	}
	else
	{
		SetMind(ESpearfishFishMind::Wander);
	}
	ForceNetUpdate();
}

void ASpearfishFish::OnCaught()
{
	if (bCaught)
	{
		return;
	}
	bCaught = true;
	if (USpearfishFishSubsystem* Subsystem = USpearfishFishSubsystem::Get(this))
	{
		Subsystem->NotifyFishRemoved(this);
	}
	Destroy();
}

void ASpearfishFish::GetEaten(ASpearfishFish* Predator)
{
	if (bCaught)
	{
		return;
	}
	if (bHooked && HookedBy)
	{
		if (USpeargunComponent* Speargun = HookedBy->GetSpeargun())
		{
			Speargun->OnFishStolen();
		}
	}
	OnCaught();
}

// ---------------------------------------------------------------------------- Interaction

bool ASpearfishFish::CanInteract(const ASpearfishCharacter* Character) const
{
	return bHooked && Character && HookedBy == Character && IsExhausted();
}

FText ASpearfishFish::GetInteractPrompt(const ASpearfishCharacter* Character) const
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FText Name = Registry ? Registry->GetDisplayName(SpeciesId) : FText::FromName(SpeciesId);
	return FText::Format(LOCTEXT("BagPrompt", "Bag the {0} ({1})"), Name, FText::FromString(SpearfishText::Length(LengthCm)));
}

void ASpearfishFish::Interact(ASpearfishCharacter* Character)
{
	if (Character && Character->GetSpeargun())
	{
		Character->GetSpeargun()->TryBagHookedFish();
	}
}

#undef LOCTEXT_NAMESPACE
