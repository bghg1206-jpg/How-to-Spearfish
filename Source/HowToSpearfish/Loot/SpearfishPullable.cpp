#include "Loot/SpearfishPullable.h"

#include "Components/BoxComponent.h"
#include "Core/SpearfishGameState.h"
#include "Data/SpearfishDataRegistry.h"
#include "Diving/SpearfishCharacter.h"
#include "Engine/World.h"
#include "HowToSpearfish.h"
#include "Inventory/SpearfishCatchService.h"
#include "Net/UnrealNetwork.h"
#include "World/SpearfishOceanSubsystem.h"
#include "World/SpearfishVisualSubsystem.h"

#define LOCTEXT_NAMESPACE "SpearfishPullable"

namespace SpearfishPullablePrivate
{
	struct FKindTuning
	{
		FVector ExtentCm;
		float MassKg;
		/** Fraction of gravity cancelled underwater (1 = neutrally buoyant). */
		float Buoyancy;
	};

	FKindTuning GetTuning(ESpearfishPullableKind Kind)
	{
		switch (Kind)
		{
		case ESpearfishPullableKind::Chest:
			return { FVector(45.f, 30.f, 28.f), 70.f, 0.7f };
		case ESpearfishPullableKind::Boulder:
			return { FVector(65.f, 60.f, 50.f), 260.f, 0.45f };
		case ESpearfishPullableKind::Crate:
		default:
			return { FVector(40.f, 30.f, 28.f), 35.f, 0.82f };
		}
	}
}

ASpearfishPullable::ASpearfishPullable()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bReplicates = true;
	SetReplicatingMovement(true);
	SetNetUpdateFrequency(20.f);
	SetNetCullDistanceSquared(FMath::Square(12000.f));

	Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
	RootComponent = Body;
	Body->SetBoxExtent(FVector(40.f, 30.f, 28.f));
	Body->SetCollisionProfileName(TEXT("PhysicsActor"));
	Body->SetCollisionResponseToChannel(ECC_Interact, ECR_Block);
	Body->SetCollisionResponseToChannel(ECC_Harpoon, ECR_Block);
	Body->SetLinearDamping(2.f);
	Body->SetAngularDamping(2.5f);
	Body->SetCanEverAffectNavigation(false);

	Lid = CreateDefaultSubobject<USceneComponent>(TEXT("Lid"));
	Lid->SetupAttachment(Body);
}

void ASpearfishPullable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpearfishPullable, Kind);
	DOREPLIFETIME(ASpearfishPullable, LootId);
	DOREPLIFETIME(ASpearfishPullable, Remaining);
	DOREPLIFETIME(ASpearfishPullable, bOpened);
}

void ASpearfishPullable::ConfigureContents(ESpearfishPullableKind InKind, FName InLootId, int32 InCount)
{
	Kind = InKind;
	LootId = InKind == ESpearfishPullableKind::Boulder ? NAME_None : InLootId;
	Remaining = LootId.IsNone() ? 0 : FMath::Max(InCount, 0);
}

void ASpearfishPullable::BeginPlay()
{
	Super::BeginPlay();
	const SpearfishPullablePrivate::FKindTuning Tuning = SpearfishPullablePrivate::GetTuning(Kind);
	Body->SetBoxExtent(Tuning.ExtentCm);
	Body->SetMassOverrideInKg(NAME_None, Tuning.MassKg, true);
	// Simulated everywhere so clients interpolate smoothly; the server's state wins via movement replication.
	Body->SetSimulatePhysics(true);
	BuildVisuals();
}

void ASpearfishPullable::OnRep_Kind()
{
	if (!HasActorBegunPlay())
	{
		return;
	}
	const SpearfishPullablePrivate::FKindTuning Tuning = SpearfishPullablePrivate::GetTuning(Kind);
	Body->SetBoxExtent(Tuning.ExtentCm);
	BuildVisuals();
	UpdateLid();
}

void ASpearfishPullable::BuildVisuals()
{
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	if (bVisualsBuilt || !Visuals)
	{
		return;
	}
	bVisualsBuilt = true;
	const FVector Size = SpearfishPullablePrivate::GetTuning(Kind).ExtentCm * 2.0;

	switch (Kind)
	{
	case ESpearfishPullableKind::Boulder:
		Visuals->AddPart(this, Body, ESpearfishShape::Sphere, FVector::ZeroVector, FRotator(10.f, 20.f, 5.f), Size * 1.08, FLinearColor(0.32f, 0.3f, 0.28f), false);
		Visuals->AddPart(this, Body, ESpearfishShape::Sphere, FVector(20.f, -15.f, 18.f), FRotator::ZeroRotator, Size * 0.6, FLinearColor(0.28f, 0.3f, 0.26f), false);
		break;

	case ESpearfishPullableKind::Chest:
	{
		const FLinearColor Wood(0.35f, 0.2f, 0.1f);
		const FLinearColor Iron(0.25f, 0.25f, 0.27f);
		Visuals->AddPart(this, Body, ESpearfishShape::Cube, FVector(0.f, 0.f, -6.f), FRotator::ZeroRotator, FVector(Size.X, Size.Y, Size.Z - 12.f), Wood, false);
		Visuals->AddPart(this, Body, ESpearfishShape::Cube, FVector(-Size.X * 0.3f, 0.f, -6.f), FRotator::ZeroRotator, FVector(6.f, Size.Y + 2.f, Size.Z - 10.f), Iron, false);
		Visuals->AddPart(this, Body, ESpearfishShape::Cube, FVector(Size.X * 0.3f, 0.f, -6.f), FRotator::ZeroRotator, FVector(6.f, Size.Y + 2.f, Size.Z - 10.f), Iron, false);
		// Lid hinged along the back edge; the gold inside glints once it's open.
		Lid->SetRelativeLocation(FVector(0.f, -Size.Y * 0.5f, Size.Z * 0.5f - 12.f));
		Visuals->AddPart(this, Lid, ESpearfishShape::Cylinder, FVector(0.f, Size.Y * 0.5f, 4.f), FRotator(90.f, 0.f, 0.f), FVector(Size.Y, Size.Y, Size.X), Wood * 1.2f, false);
		Visuals->AddPart(this, Body, ESpearfishShape::Sphere, FVector(0.f, 0.f, Size.Z * 0.5f - 14.f), FRotator::ZeroRotator, FVector(Size.X * 0.8f, Size.Y * 0.7f, 10.f),
			FLinearColor(1.f, 0.78f, 0.25f), false, 2.f);
		break;
	}

	case ESpearfishPullableKind::Crate:
	default:
	{
		const FLinearColor Wood(0.45f, 0.36f, 0.24f);
		Visuals->AddPart(this, Body, ESpearfishShape::Cube, FVector(0.f, 0.f, -4.f), FRotator::ZeroRotator, FVector(Size.X, Size.Y, Size.Z - 8.f), Wood, false);
		for (int32 Slat = -1; Slat <= 1; ++Slat)
		{
			Visuals->AddPart(this, Body, ESpearfishShape::Cube, FVector(Slat * Size.X * 0.35f, 0.f, -4.f), FRotator::ZeroRotator, FVector(5.f, Size.Y + 2.f, Size.Z - 6.f), Wood * 0.7f, false);
		}
		Lid->SetRelativeLocation(FVector(0.f, -Size.Y * 0.5f, Size.Z * 0.5f - 8.f));
		Visuals->AddPart(this, Lid, ESpearfishShape::Cube, FVector(0.f, Size.Y * 0.5f, 3.f), FRotator::ZeroRotator, FVector(Size.X + 2.f, Size.Y + 2.f, 6.f), Wood * 0.85f, false);
		break;
	}
	}
	UpdateLid();
}

void ASpearfishPullable::UpdateLid()
{
	Lid->SetRelativeRotation(FRotator(0.f, 0.f, bOpened ? -105.f : 0.f));
}

void ASpearfishPullable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ApplyWaterForces();
}

void ASpearfishPullable::ApplyWaterForces()
{
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	if (!Body->IsSimulatingPhysics() || !Ocean)
	{
		return;
	}
	const bool bUnderwater = Ocean->IsUnderwater(Body->GetComponentLocation());
	const float GravityZ = GetWorld()->GetGravityZ();
	if (bUnderwater)
	{
		const float Buoyancy = SpearfishPullablePrivate::GetTuning(Kind).Buoyancy;
		Body->AddForce(FVector(0.f, 0.f, -GravityZ * Buoyancy), NAME_None, true);
	}
	Body->SetLinearDamping(bUnderwater ? 2.f : 0.15f);
	Body->SetAngularDamping(bUnderwater ? 2.5f : 0.3f);
}

bool ASpearfishPullable::CanInteract(const ASpearfishCharacter* Character) const
{
	return Character && !Character->IsBlackedOut() && Kind != ESpearfishPullableKind::Boulder && (!bOpened || Remaining > 0);
}

FText ASpearfishPullable::GetInteractPrompt(const ASpearfishCharacter* Character) const
{
	if (!bOpened)
	{
		return Kind == ESpearfishPullableKind::Chest ? LOCTEXT("OpenChest", "Open the chest") : LOCTEXT("PryCrate", "Pry the crate open");
	}
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	return FText::Format(LOCTEXT("TakeLoot", "Take {0} ({1} left)"), Registry ? Registry->GetDisplayName(LootId) : FText::FromName(LootId), FText::AsNumber(Remaining));
}

void ASpearfishPullable::Interact(ASpearfishCharacter* Character)
{
	if (!HasAuthority() || !CanInteract(Character))
	{
		return;
	}
	if (!bOpened)
	{
		bOpened = true;
		UpdateLid();
		if (Kind == ESpearfishPullableKind::Chest)
		{
			if (ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>())
			{
				GameState->BroadcastNotice(FText::Format(LOCTEXT("ChestFound", "{0} found the storm chest!"), FText::FromString(Character->GetPlayerName())),
					ESpearfishNoticeType::Discovery);
			}
		}
		if (Remaining <= 0)
		{
			Character->NotifyOwner(LOCTEXT("Empty", "Empty. Someone got here first."), ESpearfishNoticeType::Info);
		}
	}
	// Take as much as the bag allows; the rest stays inside for a second trip.
	const float Quality = Kind == ESpearfishPullableKind::Chest ? 1.f : FMath::FRandRange(0.55f, 0.95f);
	while (Remaining > 0 && SpearfishCatch::CollectLoot(Character, LootId, Quality))
	{
		--Remaining;
	}
	ForceNetUpdate();
}

#undef LOCTEXT_NAMESPACE
