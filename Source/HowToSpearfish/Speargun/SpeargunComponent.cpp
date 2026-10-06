#include "Speargun/SpeargunComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/SpearfishGameState.h"
#include "Core/SpearfishPlayerState.h"
#include "Core/SpearfishSettings.h"
#include "Diving/SpearfishCharacter.h"
#include "Diving/SpearfishDiverBodyComponent.h"
#include "Engine/World.h"
#include "FishAI/SpearfishFish.h"
#include "FishAI/SpearfishFishSubsystem.h"
#include "HowToSpearfish.h"
#include "Inventory/SpearfishCatchService.h"
#include "Inventory/SpearfishDiveBagComponent.h"
#include "Net/UnrealNetwork.h"
#include "Speargun/SpearfishHarpoon.h"
#include "World/SpearfishVisualSubsystem.h"

#define LOCTEXT_NAMESPACE "Speargun"

namespace SpeargunPrivate
{
	constexpr int32 MaxWraps = 8;
	constexpr int32 MaxSegments = 24;
	const FVector MuzzleOffset(55.f, 14.f, -16.f);
}

USpeargunComponent::USpeargunComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void USpeargunComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USpeargunComponent, Line);
	DOREPLIFETIME(USpeargunComponent, Harpoon);
}

ASpearfishCharacter* USpeargunComponent::GetCharacter() const
{
	return Cast<ASpearfishCharacter>(GetOwner());
}

void USpeargunComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USpeargunComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		ClearTarget(true);
		if (Harpoon)
		{
			Harpoon->Destroy();
			Harpoon = nullptr;
		}
	}
	Super::EndPlay(EndPlayReason);
}

void USpeargunComponent::ApplyStats(const FSpearfishDiverStats& InStats)
{
	Stats = InStats;
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (!Stats.bHasSpeargun)
		{
			ForceRelease();
			if (Harpoon)
			{
				Harpoon->Destroy();
				Harpoon = nullptr;
			}
			Line = FSpearfishLineNetState();
			Line.State = ESpeargunState::Unarmed;
		}
		else if (Line.State == ESpeargunState::Unarmed)
		{
			Line.State = ESpeargunState::Loaded;
		}
	}

	// Rebuild the first-person model to match the equipped tier.
	if (FirstPersonBarrel)
	{
		if (USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this))
		{
			const float Length = 45.f + Stats.GunTier * 12.f;
			FirstPersonBarrel->SetRelativeLocation(FVector(34.f + Length * 0.5f, 0.f, 3.f));
			FirstPersonBarrel->SetRelativeScale3D(FVector(3.4f, 3.4f, Length) / 100.0);
			FirstPersonStock->SetMaterial(0, Visuals->GetColorMaterial(Stats.GunColor));
			FirstPersonBand->SetRelativeLocation(FVector(34.f + Length, 0.f, 6.f));
		}
	}
}

// ------------------------------------------------------------------------------------- Input

void USpeargunComponent::RequestFire()
{
	const ASpearfishCharacter* Character = GetCharacter();
	if (!Character || !Stats.bHasSpeargun || Line.State != ESpeargunState::Loaded || !Character->CanUseGear())
	{
		return;
	}
	Recoil = 1.f;
	ServerFire(GetMuzzleLocation(), Character->GetAimDirection());
}

void USpeargunComponent::SetReeling(bool bInReeling)
{
	if (bLocalReeling != bInReeling)
	{
		bLocalReeling = bInReeling;
		ServerSetReeling(bInReeling);
	}
}

void USpeargunComponent::RequestRelease()
{
	ServerRelease();
}

void USpeargunComponent::ServerSetReeling_Implementation(bool bInReeling)
{
	bServerReeling = bInReeling;
}

void USpeargunComponent::ServerRelease_Implementation()
{
	if (Line.State == ESpeargunState::Attached || Line.State == ESpeargunState::Flying)
	{
		BeginRetrieve(true);
	}
}

void USpeargunComponent::ServerFire_Implementation(FVector_NetQuantize Origin, FVector_NetQuantizeNormal Direction)
{
	ASpearfishCharacter* Character = GetCharacter();
	if (!Character || !Stats.bHasSpeargun || Line.State != ESpeargunState::Loaded || !Character->CanUseGear())
	{
		return;
	}

	FVector Start = GetMuzzleLocation();
	if (FVector::DistSquared(Start, Origin) < FMath::Square(150.0))
	{
		Start = Origin;
	}
	FVector Direction3 = FVector(Direction).GetSafeNormal();
	if (Direction3.IsNearlyZero())
	{
		Direction3 = Character->GetAimDirection();
	}
	Direction3 = FMath::VRandCone(Direction3, FMath::DegreesToRadians(Stats.AimSpreadDeg));

	FActorSpawnParameters Params;
	Params.Owner = Character;
	Params.Instigator = Character;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASpearfishHarpoon* NewHarpoon = GetWorld()->SpawnActor<ASpearfishHarpoon>(ASpearfishHarpoon::StaticClass(), Start, Direction3.Rotation(), Params);
	if (!NewHarpoon)
	{
		return;
	}
	NewHarpoon->Launch(this, Direction3 * Stats.ShaftSpeedCm, Stats.GunRangeCm);
	Harpoon = NewHarpoon;

	Line = FSpearfishLineNetState();
	Line.State = ESpeargunState::Flying;

	if (USpearfishFishSubsystem* Fish = USpearfishFishSubsystem::Get(this))
	{
		Fish->ReportNoise(Start, 500.f);
	}
	MulticastFireEffects(Start);
}

void USpeargunComponent::MulticastFireEffects_Implementation(FVector_NetQuantize Origin)
{
	if (ASpearfishCharacter* Character = GetCharacter())
	{
		if (USpearfishDiverBodyComponent* Body = Character->GetBody())
		{
			Body->EmitBubbles(4);
		}
	}
}

// ------------------------------------------------------------------------------ Server: hits

void USpeargunComponent::OnHarpoonHit(const FHitResult& Hit)
{
	ASpearfishCharacter* Character = GetCharacter();
	if (!Character || !Harpoon || Line.State != ESpeargunState::Flying)
	{
		return;
	}

	AActor* HitActor = Hit.GetActor();
	USpearfishFishSubsystem* FishSystem = USpearfishFishSubsystem::Get(this);

	auto AttachLine = [this](ESpearfishLineTarget Target, AActor* TargetActor)
	{
		Line.State = ESpeargunState::Attached;
		Line.Target = Target;
		Line.TargetActor = TargetActor;
		Line.WrapPoints.Reset();
		AttachedTime = 0.f;
		const float Distance = static_cast<float>(FVector::Dist(GetMuzzleLocation(), Harpoon->GetTipLocation()));
		SpearfishLine::Attach(LineSim, Distance, Stats.Line);
		Line.LengthCm = LineSim.LengthCm;
	};

	if (ASpearfishFish* Fish = Cast<ASpearfishFish>(HitActor))
	{
		float Quality = 0.5f;
		if (Fish->TryHook(Character, Hit, Stats.GunPower, Quality))
		{
			ShotQuality = Quality;
			Harpoon->StickTo(Hit);
			AttachLine(ESpearfishLineTarget::Fish, Fish);
			return;
		}
		// Uncatchable (sharks, rays...) shrug it off - and remember who did it.
		Fish->Provoke(Character);
		if (FishSystem)
		{
			FishSystem->ReportNoise(Hit.ImpactPoint, 900.f);
		}
		BeginRetrieve(false);
		return;
	}

	if (ASpearfishCharacter* Victim = Cast<ASpearfishCharacter>(HitActor))
	{
		if (Victim != Character)
		{
			Harpoon->StickTo(Hit);
			AttachLine(ESpearfishLineTarget::Character, Victim);
			Victim->SetTetheredBy(Character);
			if (ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>())
			{
				GameState->BroadcastNotice(FText::Format(LOCTEXT("Tethered", "{0} clipped a harpoon line onto {1}!"),
					FText::FromString(Character->GetPlayerName()), FText::FromString(Victim->GetPlayerName())), ESpearfishNoticeType::Warning);
			}
			return;
		}
	}

	if (const UPrimitiveComponent* Component = Hit.GetComponent())
	{
		if (Component->IsSimulatingPhysics())
		{
			Harpoon->StickTo(Hit);
			AttachLine(ESpearfishLineTarget::Physics, HitActor);
			return;
		}
	}

	// Rock, coral, wreck, hull: the shaft sticks and becomes a grapple anchor.
	Harpoon->StickTo(Hit);
	AttachLine(ESpearfishLineTarget::World, nullptr);
	if (FishSystem)
	{
		FishSystem->ReportNoise(Hit.ImpactPoint, 700.f);
	}
}

void USpeargunComponent::OnHarpoonExpired()
{
	BeginRetrieve(false);
}

void USpeargunComponent::OnHarpoonReturned()
{
	if (Harpoon)
	{
		Harpoon->Destroy();
		Harpoon = nullptr;
	}
	BeginReload();
}

void USpeargunComponent::ClearTarget(bool bFishEscapes)
{
	if (Line.Target == ESpearfishLineTarget::Fish)
	{
		if (ASpearfishFish* Fish = Cast<ASpearfishFish>(Line.TargetActor))
		{
			Fish->ReleaseFromLine(bFishEscapes);
		}
	}
	else if (Line.Target == ESpearfishLineTarget::Character)
	{
		if (ASpearfishCharacter* Victim = Cast<ASpearfishCharacter>(Line.TargetActor))
		{
			Victim->SetTetheredBy(nullptr);
		}
	}
	Line.Target = ESpearfishLineTarget::None;
	Line.TargetActor = nullptr;
	Line.ShooterPull = FVector::ZeroVector;
	Line.TargetPull = FVector::ZeroVector;
	Line.Tension = 0.f;
	Line.Stress = 0.f;
	Line.bTaut = false;
	Line.WrapPoints.Reset();
}

void USpeargunComponent::BeginRetrieve(bool bFishEscapes)
{
	ClearTarget(bFishEscapes);
	if (Harpoon)
	{
		Harpoon->StartReturn();
		Line.State = ESpeargunState::Retrieving;
	}
	else
	{
		BeginReload();
	}
}

void USpeargunComponent::BeginReload()
{
	Line.State = Stats.bHasSpeargun ? ESpeargunState::Reloading : ESpeargunState::Unarmed;
	Line.ReloadRemaining = Stats.ReloadSeconds;
}

void USpeargunComponent::ForceRelease()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (Line.State == ESpeargunState::Attached || Line.State == ESpeargunState::Flying)
	{
		BeginRetrieve(true);
	}
}

void USpeargunComponent::Snap(bool bTangled)
{
	if (ASpearfishCharacter* Character = GetCharacter())
	{
		Character->NotifyOwner(bTangled
			? LOCTEXT("Tangled", "The line tangled on the reef and snapped!")
			: LOCTEXT("Snapped", "SNAP! The line couldn't take the strain."), ESpearfishNoticeType::Danger);
	}
	BeginRetrieve(true);
}

ASpearfishFish* USpeargunComponent::GetHookedFish() const
{
	return Line.Target == ESpearfishLineTarget::Fish ? Cast<ASpearfishFish>(Line.TargetActor) : nullptr;
}

bool USpeargunComponent::HasExhaustedFishOnLine() const
{
	const ASpearfishFish* Fish = GetHookedFish();
	return Fish && Fish->IsExhausted();
}

bool USpeargunComponent::TryBagHookedFish()
{
	ASpearfishCharacter* Character = GetCharacter();
	ASpearfishFish* Fish = GetHookedFish();
	if (!Character || !Fish || !Fish->IsExhausted())
	{
		return false;
	}

	const FSpearfishItem Item = SpearfishCatch::MakeFishItem(GetWorld(), Fish, ShotQuality, AttachedTime);
	if (!Item.IsValid())
	{
		return false;
	}
	const ESpearfishBagResult Result = Character->GetBag()->TryAdd(Item);
	if (Result != ESpearfishBagResult::Ok)
	{
		Character->NotifyOwner(Result == ESpearfishBagResult::TooHeavy
			? LOCTEXT("TooHeavy", "Too heavy for the bag - tow it back to the boat ladder!")
			: LOCTEXT("BagFull", "No room in the bag - tow it back to the boat ladder!"), ESpearfishNoticeType::Warning);
		return false;
	}

	// Detach first so the shaft is not destroyed with the fish.
	Line.Target = ESpearfishLineTarget::None;
	Line.TargetActor = nullptr;
	if (Harpoon)
	{
		Harpoon->StartReturn();
		Line.State = ESpeargunState::Retrieving;
	}
	else
	{
		BeginReload();
	}
	Fish->OnCaught();
	SpearfishCatch::RecordCatch(Character, Item);
	return true;
}

ASpearfishFish* USpeargunComponent::DetachFishForLanding()
{
	ASpearfishFish* Fish = GetHookedFish();
	if (!Fish || !Fish->IsExhausted())
	{
		return nullptr;
	}
	Line.Target = ESpearfishLineTarget::None;
	Line.TargetActor = nullptr;
	if (Harpoon)
	{
		Harpoon->StartReturn();
		Line.State = ESpeargunState::Retrieving;
	}
	else
	{
		BeginReload();
	}
	return Fish;
}

void USpeargunComponent::OnFishStolen()
{
	Line.Target = ESpearfishLineTarget::None;
	Line.TargetActor = nullptr;
	if (ASpearfishCharacter* Character = GetCharacter())
	{
		Character->NotifyOwner(LOCTEXT("Stolen", "A shark tore your catch off the spear!"), ESpearfishNoticeType::Danger);
	}
	BeginRetrieve(false);
}

// ------------------------------------------------------------------------------ Server: line

void USpeargunComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwnerRole() == ROLE_Authority)
	{
		switch (Line.State)
		{
		case ESpeargunState::Reloading:
			Line.ReloadRemaining = FMath::Max(0.f, Line.ReloadRemaining - DeltaTime);
			if (Line.ReloadRemaining <= 0.f)
			{
				Line.State = Stats.bHasSpeargun ? ESpeargunState::Loaded : ESpeargunState::Unarmed;
			}
			break;
		case ESpeargunState::Flying:
		case ESpeargunState::Retrieving:
			if (!Harpoon)
			{
				BeginReload();
			}
			break;
		case ESpeargunState::Attached:
			TickAttachedLine(DeltaTime);
			break;
		default:
			break;
		}
	}

	UpdateFirstPersonGun(DeltaTime);
	UpdateLineVisuals();
}

void USpeargunComponent::TickAttachedLine(float DeltaTime)
{
	ASpearfishCharacter* Character = GetCharacter();
	if (!Character || !Harpoon)
	{
		BeginRetrieve(true);
		return;
	}
	if ((Line.Target == ESpearfishLineTarget::Fish || Line.Target == ESpearfishLineTarget::Character || Line.Target == ESpearfishLineTarget::Physics)
		&& !IsValid(Line.TargetActor))
	{
		BeginRetrieve(false);
		return;
	}

	AttachedTime += DeltaTime;
	const FVector Gun = GetMuzzleLocation();
	const FVector End = Harpoon->GetTipLocation();

	UpdateWraps(Gun, End);
	if (Line.WrapPoints.Num() > SpeargunPrivate::MaxWraps)
	{
		Snap(true);
		return;
	}

	const float Path = ComputePathLength(Gun, End);
	const FVector LastPoint = Line.WrapPoints.Num() > 0 ? FVector(Line.WrapPoints.Last()) : Gun;
	const FVector FirstPoint = Line.WrapPoints.Num() > 0 ? FVector(Line.WrapPoints[0]) : End;
	const FVector EndDirection = (End - LastPoint).GetSafeNormal();
	const FVector ToFirst = (FirstPoint - Gun).GetSafeNormal();

	float PullAway = 0.f;
	ASpearfishFish* Fish = GetHookedFish();
	ASpearfishCharacter* Victim = Line.Target == ESpearfishLineTarget::Character ? Cast<ASpearfishCharacter>(Line.TargetActor) : nullptr;
	if (Fish)
	{
		PullAway = Fish->GetLinePullAway(EndDirection);
	}
	else if (Victim)
	{
		PullAway = FMath::Max(0.f, static_cast<float>(FVector::DotProduct(Victim->GetVelocity(), EndDirection))) * 1.1f;
	}

	FSpearfishLineInput Input;
	Input.DistanceCm = Path;
	Input.PullAwayForce = PullAway;
	Input.bReeling = bServerReeling;
	Input.DeltaSeconds = DeltaTime;
	SpearfishLine::Step(LineSim, Input, Stats.Line);
	if (LineSim.bSnapped)
	{
		Snap(false);
		return;
	}

	const float EndSegment = static_cast<float>(FVector::Dist(End, LastPoint));
	const float AllowedForEnd = FMath::Max(50.f, LineSim.LengthCm - (Path - EndSegment));
	const float Excess = FMath::Max(0.f, Path - LineSim.LengthCm);
	const float PullScale = USpearfishSettings::Get()->LinePullOnDiverScale;
	const float ReelSpeed = (bServerReeling && LineSim.bTaut) ? Stats.Line.ReelSpeedCm : 0.f;

	FVector ShooterPull = FVector::ZeroVector;
	FVector TargetPull = FVector::ZeroVector;

	switch (Line.Target)
	{
	case ESpearfishLineTarget::Fish:
		Fish->ApplyLineConstraint(LastPoint, AllowedForEnd, LineSim.Tension, ReelSpeed, DeltaTime);
		if (LineSim.bTaut)
		{
			ShooterPull = ToFirst * (SpearfishLine::DiverPullAcceleration(LineSim.Tension, PullScale) + Excess * 4.f);
		}
		if (USpearfishFishSubsystem* FishSystem = USpearfishFishSubsystem::Get(this))
		{
			FishSystem->ReportStruggle(End, Fish);
		}
		break;

	case ESpearfishLineTarget::Character:
		if (LineSim.bTaut)
		{
			TargetPull = -EndDirection * ((bServerReeling ? Stats.Line.ReelForce * 2.2f : 0.f) + Excess * 8.f);
			ShooterPull = ToFirst * PullAway * PullScale * 0.8f;
		}
		break;

	case ESpearfishLineTarget::Physics:
		if (UPrimitiveComponent* Body = Line.TargetActor ? Cast<UPrimitiveComponent>(Line.TargetActor->GetRootComponent()) : nullptr)
		{
			const float Mass = Body->IsSimulatingPhysics() ? Body->GetMass() : 1000.f;
			if (LineSim.bTaut || Excess > 0.f)
			{
				const float Acceleration = (bServerReeling ? 450.f : 0.f) + Excess * 6.f;
				Body->AddForce(-EndDirection * Acceleration, NAME_None, true);
			}
			if (LineSim.bTaut && bServerReeling)
			{
				ShooterPull = ToFirst * (Stats.Line.ReelForce * PullScale * Mass / (Mass + 150.f) + Excess * 3.f);
			}
		}
		break;

	case ESpearfishLineTarget::World:
		if (LineSim.bTaut && bServerReeling)
		{
			// Reeling toward a stuck shaft drags the diver along: a grapple for fighting currents.
			ShooterPull = ToFirst * (Stats.Line.ReelForce * PullScale * 1.6f);
		}
		ShooterPull += ToFirst * Excess * 10.f;
		if (bServerReeling && Path < 140.f)
		{
			BeginRetrieve(false);
			return;
		}
		break;

	default:
		break;
	}

	Line.ShooterPull = ShooterPull;
	Line.TargetPull = TargetPull;
	Line.LengthCm = LineSim.LengthCm;
	Line.Tension = LineSim.Tension;
	Line.bTaut = LineSim.bTaut;
	Line.bReeling = bServerReeling;
	Line.Stress = SpearfishLine::StressFraction(LineSim, Stats.Line);
}

bool USpeargunComponent::IsSegmentBlocked(const FVector& From, const FVector& To, FHitResult& OutHit) const
{
	const FVector Delta = To - From;
	const double Length = Delta.Size();
	if (Length < 40.0)
	{
		return false;
	}
	const FVector Direction = Delta / Length;
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpearfishLineWrap), false, GetOwner());
	if (Harpoon)
	{
		Params.AddIgnoredActor(Harpoon);
	}
	if (Line.TargetActor)
	{
		Params.AddIgnoredActor(Line.TargetActor);
	}
	return GetWorld()->LineTraceSingleByObjectType(OutHit, From + Direction * 12.0, To - Direction * 12.0, Objects, Params);
}

void USpeargunComponent::UpdateWraps(const FVector& Gun, const FVector& End)
{
	TArray<FVector_NetQuantize10>& Wraps = Line.WrapPoints;
	FHitResult Hit;

	const FVector FirstPoint = Wraps.Num() > 0 ? FVector(Wraps[0]) : End;
	if (IsSegmentBlocked(Gun, FirstPoint, Hit))
	{
		Wraps.Insert(FVector_NetQuantize10(Hit.ImpactPoint + Hit.ImpactNormal * 8.0), 0);
	}
	const FVector LastPoint = Wraps.Num() > 0 ? FVector(Wraps.Last()) : Gun;
	if (IsSegmentBlocked(LastPoint, End, Hit))
	{
		Wraps.Add(FVector_NetQuantize10(Hit.ImpactPoint + Hit.ImpactNormal * 8.0));
	}

	// Unwrap from both ends as soon as the line can shortcut past a bend.
	while (Wraps.Num() > 0)
	{
		const FVector Next = Wraps.Num() > 1 ? FVector(Wraps[1]) : End;
		if (IsSegmentBlocked(Gun, Next, Hit))
		{
			break;
		}
		Wraps.RemoveAt(0);
	}
	while (Wraps.Num() > 0)
	{
		const FVector Previous = Wraps.Num() > 1 ? FVector(Wraps[Wraps.Num() - 2]) : Gun;
		if (IsSegmentBlocked(Previous, End, Hit))
		{
			break;
		}
		Wraps.Pop();
	}
}

float USpeargunComponent::ComputePathLength(const FVector& Gun, const FVector& End) const
{
	float Length = 0.f;
	FVector Previous = Gun;
	for (const FVector_NetQuantize10& Wrap : Line.WrapPoints)
	{
		Length += static_cast<float>(FVector::Dist(Previous, Wrap));
		Previous = Wrap;
	}
	return Length + static_cast<float>(FVector::Dist(Previous, End));
}

// ---------------------------------------------------------------------------------- Queries

float USpeargunComponent::GetReloadFraction() const
{
	switch (Line.State)
	{
	case ESpeargunState::Loaded: return 1.f;
	case ESpeargunState::Reloading: return Stats.ReloadSeconds > 0.f ? 1.f - Line.ReloadRemaining / Stats.ReloadSeconds : 1.f;
	default: return 0.f;
	}
}

FVector USpeargunComponent::GetMuzzleLocation() const
{
	const ASpearfishCharacter* Character = GetCharacter();
	if (!Character)
	{
		return FVector::ZeroVector;
	}
	return Character->GetEyeLocation() + Character->GetAimRotation().RotateVector(SpeargunPrivate::MuzzleOffset);
}

FVector USpeargunComponent::GetVisualMuzzleLocation() const
{
	const ASpearfishCharacter* Character = GetCharacter();
	if (Character && Character->IsLocallyControlled() && FirstPersonBand)
	{
		return FirstPersonBand->GetComponentLocation();
	}
	return GetMuzzleLocation();
}

FVector USpeargunComponent::GetShooterPullAcceleration() const
{
	return Line.State == ESpeargunState::Attached ? FVector(Line.ShooterPull) : FVector::ZeroVector;
}

FVector USpeargunComponent::GetTargetPullAcceleration(const AActor* Target) const
{
	return (Line.State == ESpeargunState::Attached && Line.TargetActor == Target) ? FVector(Line.TargetPull) : FVector::ZeroVector;
}

void USpeargunComponent::OnRep_Line()
{
}

// --------------------------------------------------------------------------------- Visuals

void USpeargunComponent::BuildFirstPersonGun()
{
	ASpearfishCharacter* Character = GetCharacter();
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	if (FirstPersonRoot || !Character || !Visuals || !Character->GetFirstPersonCamera())
	{
		return;
	}

	FirstPersonRoot = NewObject<USceneComponent>(Character);
	FirstPersonRoot->SetupAttachment(Character->GetFirstPersonCamera());
	FirstPersonRoot->SetRelativeLocation(FVector(0.f, 14.f, -18.f));
	FirstPersonRoot->RegisterComponent();

	auto Add = [Visuals, Character, this](ESpearfishShape Shape, const FVector& Location, const FRotator& Rotation, const FVector& Size, const FLinearColor& Color)
	{
		UStaticMeshComponent* Part = Visuals->AddPart(Character, FirstPersonRoot, Shape, Location, Rotation, Size, Color, false);
		Part->SetOnlyOwnerSee(true);
		Part->SetCastShadow(false);
		return Part;
	};
	FirstPersonStock = Add(ESpearfishShape::Cube, FVector(22.f, 0.f, 0.f), FRotator::ZeroRotator, FVector(26.f, 5.f, 8.f), Stats.GunColor);
	FirstPersonBarrel = Add(ESpearfishShape::Cylinder, FVector(56.f, 0.f, 3.f), FRotator(90.f, 0.f, 0.f), FVector(3.4f, 3.4f, 45.f), FLinearColor(0.2f, 0.2f, 0.22f));
	FirstPersonBand = Add(ESpearfishShape::Cube, FVector(79.f, 0.f, 6.f), FRotator::ZeroRotator, FVector(2.f, 7.f, 2.f), FLinearColor(0.85f, 0.25f, 0.15f));
	FirstPersonShaft = Add(ESpearfishShape::Cylinder, FVector(60.f, 0.f, 7.f), FRotator(90.f, 0.f, 0.f), FVector(1.f, 1.f, 70.f), FLinearColor(0.8f, 0.8f, 0.85f));
	ApplyStats(Stats);
}

void USpeargunComponent::UpdateFirstPersonGun(float DeltaTime)
{
	const ASpearfishCharacter* Character = GetCharacter();
	if (!Character || !Character->IsLocallyControlled())
	{
		return;
	}
	BuildFirstPersonGun();
	if (!FirstPersonRoot)
	{
		return;
	}

	const bool bVisible = Stats.bHasSpeargun && !Character->IsDriving() && !Character->IsInBed();
	FirstPersonRoot->SetVisibility(bVisible, true);
	if (!bVisible)
	{
		return;
	}

	Recoil = FMath::FInterpTo(Recoil, 0.f, DeltaTime, 8.f);
	const float Reel = Line.bReeling ? FMath::Sin(GetWorld()->GetTimeSeconds() * 30.f) * 0.6f : 0.f;
	FirstPersonRoot->SetRelativeLocation(FVector(-Recoil * 7.f, 14.f, -18.f + Reel));

	const float Loaded = GetReloadFraction();
	const bool bShaftVisible = Line.State == ESpeargunState::Loaded || Line.State == ESpeargunState::Reloading;
	FirstPersonShaft->SetVisibility(bShaftVisible);
	FirstPersonShaft->SetRelativeLocation(FVector(60.f - (1.f - Loaded) * 40.f, 0.f, 7.f));
}

void USpeargunComponent::SetSegmentCount(int32 Count)
{
	ASpearfishCharacter* Character = GetCharacter();
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	if (!Character || !Visuals)
	{
		return;
	}
	while (LineSegments.Num() < Count && LineSegments.Num() < SpeargunPrivate::MaxSegments)
	{
		UStaticMeshComponent* Segment = Visuals->AddPart(Character, Character->GetRootComponent(), ESpearfishShape::Cube, FVector::ZeroVector,
			FRotator::ZeroRotator, FVector(1.f), FLinearColor(0.95f, 0.95f, 0.9f), false, 0.4f);
		Segment->SetUsingAbsoluteLocation(true);
		Segment->SetUsingAbsoluteRotation(true);
		Segment->SetUsingAbsoluteScale(true);
		Segment->SetCastShadow(false);
		LineSegments.Add(Segment);
	}
	for (int32 Index = 0; Index < LineSegments.Num(); ++Index)
	{
		LineSegments[Index]->SetVisibility(Index < Count);
	}
}

void USpeargunComponent::UpdateLineVisuals()
{
	const bool bShowLine = Harpoon && (Line.State == ESpeargunState::Attached || Line.State == ESpeargunState::Flying || Line.State == ESpeargunState::Retrieving);
	if (!bShowLine)
	{
		SetSegmentCount(0);
		return;
	}

	TArray<FVector> Points;
	Points.Add(GetVisualMuzzleLocation());
	for (const FVector_NetQuantize10& Wrap : Line.WrapPoints)
	{
		Points.Add(Wrap);
	}
	Points.Add(Harpoon->GetTipLocation() - Harpoon->GetActorForwardVector() * 90.0);

	// Slack line sags between anchor points.
	TArray<FVector> Drawn;
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		Drawn.Add(Points[Index]);
		if (Index + 1 < Points.Num() && !Line.bTaut)
		{
			const float Span = static_cast<float>(FVector::Dist(Points[Index], Points[Index + 1]));
			const float Sag = FMath::Clamp((Line.LengthCm - Span) * 0.2f, 0.f, 120.f);
			Drawn.Add((Points[Index] + Points[Index + 1]) * 0.5 - FVector(0.0, 0.0, Sag));
		}
	}

	SetSegmentCount(Drawn.Num() - 1);

	const int32 ColorStep = FMath::Clamp(FMath::FloorToInt(Line.Stress * 4.f), 0, 3);
	if (ColorStep != LastSegmentColorStep)
	{
		LastSegmentColorStep = ColorStep;
		static const FLinearColor Colors[] = { FLinearColor(0.95f, 0.95f, 0.9f), FLinearColor(1.f, 0.9f, 0.3f), FLinearColor(1.f, 0.55f, 0.15f), FLinearColor(1.f, 0.15f, 0.1f) };
		if (USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this))
		{
			for (UStaticMeshComponent* Segment : LineSegments)
			{
				Segment->SetMaterial(0, Visuals->GetColorMaterial(Colors[ColorStep], 0.4f + ColorStep * 0.6f));
			}
		}
	}

	for (int32 Index = 0; Index + 1 < Drawn.Num() && Index < LineSegments.Num(); ++Index)
	{
		const FVector From = Drawn[Index];
		const FVector To = Drawn[Index + 1];
		const FVector Delta = To - From;
		const double Length = FMath::Max(Delta.Size(), 1.0);
		UStaticMeshComponent* Segment = LineSegments[Index];
		Segment->SetWorldLocationAndRotation((From + To) * 0.5, Delta.Rotation());
		Segment->SetWorldScale3D(FVector(Length / 100.0, 0.008, 0.008));
	}
}

#undef LOCTEXT_NAMESPACE
