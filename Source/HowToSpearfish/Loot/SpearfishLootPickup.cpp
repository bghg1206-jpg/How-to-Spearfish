#include "Loot/SpearfishLootPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/SpearfishGameTypes.h"
#include "Data/SpearfishDataRegistry.h"
#include "Diving/SpearfishCharacter.h"
#include "Engine/StaticMesh.h"
#include "HowToSpearfish.h"
#include "Inventory/SpearfishCatchService.h"
#include "Net/UnrealNetwork.h"
#include "World/SpearfishVisualSubsystem.h"

#define LOCTEXT_NAMESPACE "SpearfishLoot"

ASpearfishLootPickup::ASpearfishLootPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(2.f);
	SetNetCullDistanceSquared(FMath::Square(9000.f));

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	RootComponent = InteractSphere;
	InteractSphere->SetSphereRadius(35.f);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Interact, ECR_Block);

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(InteractSphere);
}

void ASpearfishLootPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpearfishLootPickup, LootId);
	DOREPLIFETIME(ASpearfishLootPickup, Quality);
}

void ASpearfishLootPickup::SetLoot(FName InLootId, float InQuality)
{
	LootId = InLootId;
	Quality = FMath::Clamp(InQuality, 0.f, 1.f);
}

void ASpearfishLootPickup::BeginPlay()
{
	Super::BeginPlay();
	GlintPhase = FMath::FRandRange(0.f, 6.f);
	BuildVisuals();
}

void ASpearfishLootPickup::OnRep_Loot()
{
	BuildVisuals();
}

void ASpearfishLootPickup::BuildVisuals()
{
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FSpearfishLootDef* Def = Registry ? Registry->FindLoot(LootId) : nullptr;
	if (bVisualsBuilt || !Visuals || !Def || !HasActorBegunPlay())
	{
		return;
	}
	bVisualsBuilt = true;
	bGlints = Def->Rarity >= ESpearfishRarity::Rare;
	const FLinearColor Color = Def->Color;
	const float Glow = bGlints ? 1.5f : 0.f;

	// Authored art wins over the placeholder parts.
	if (UStaticMesh* Mesh = Def->Mesh.IsNull() ? nullptr : Def->Mesh.LoadSynchronous())
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
		Part->SetStaticMesh(Mesh);
		USpearfishVisualSubsystem::ConfigureCollision(Part, false);
		Part->SetupAttachment(Pivot);
		Part->RegisterComponent();
		AddInstanceComponent(Part);
		SetActorTickEnabled(bGlints);
		return;
	}

	switch (Def->Category)
	{
	case ESpearfishLootCategory::Shell:
		Visuals->AddPart(this, Pivot, ESpearfishShape::Cone, FVector(0, 0, 6), FRotator(0.f, 0.f, 80.f), FVector(16, 16, 26), Color, false);
		Visuals->AddPart(this, Pivot, ESpearfishShape::Sphere, FVector(0, 0, 6), FRotator::ZeroRotator, FVector(14, 12, 10), Color * 0.85f, false);
		break;
	case ESpearfishLootCategory::Pearl:
		Visuals->AddPart(this, Pivot, ESpearfishShape::Sphere, FVector(0, 0, 4), FRotator::ZeroRotator, FVector(7, 7, 7), Color, false, 0.6f);
		break;
	case ESpearfishLootCategory::Treasure:
		for (int32 Coin = 0; Coin < 3; ++Coin)
		{
			Visuals->AddPart(this, Pivot, ESpearfishShape::Cylinder, FVector(Coin * 5.f - 5.f, Coin * 3.f, 1.f + Coin * 1.5f),
				FRotator(Coin * 9.f, 0.f, Coin * 6.f), FVector(9, 9, 1.2f), Color, false, Glow);
		}
		break;
	case ESpearfishLootCategory::Artifact:
		Visuals->AddPart(this, Pivot, ESpearfishShape::Cylinder, FVector(0, 0, 14), FRotator(0.f, 0.f, 70.f), FVector(22, 22, 34), Color, false);
		Visuals->AddPart(this, Pivot, ESpearfishShape::Cylinder, FVector(0, 20, 20), FRotator(0.f, 0.f, 70.f), FVector(10, 10, 12), Color * 0.8f, false);
		break;
	case ESpearfishLootCategory::Salvage:
		Visuals->AddPart(this, Pivot, ESpearfishShape::Cylinder, FVector(0, 0, 6), FRotator(0.f, 0.f, 20.f), FVector(32, 32, 8), Color, false, Glow);
		Visuals->AddPart(this, Pivot, ESpearfishShape::Cylinder, FVector(0, 0, 8), FRotator(0.f, 0.f, 20.f), FVector(22, 22, 9), FLinearColor(0.3f, 0.45f, 0.5f), false);
		break;
	case ESpearfishLootCategory::Curiosity:
	default:
		Visuals->AddPart(this, Pivot, ESpearfishShape::Cylinder, FVector(0, 0, 6), FRotator(85.f, 0.f, 0.f), FVector(9, 9, 26), Color, false);
		Visuals->AddPart(this, Pivot, ESpearfishShape::Cylinder, FVector(16, 0, 6), FRotator(85.f, 0.f, 0.f), FVector(4, 4, 8), Color * 0.7f, false);
		break;
	}
	SetActorTickEnabled(bGlints);
}

void ASpearfishLootPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bGlints || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// A slow wobble catches the light; cheaper than a particle system and readable at distance.
	GlintPhase += DeltaSeconds;
	Pivot->SetRelativeRotation(FRotator(FMath::Sin(GlintPhase * 1.7f) * 8.f, GlintPhase * 25.f, FMath::Cos(GlintPhase * 1.3f) * 6.f));
}

bool ASpearfishLootPickup::CanInteract(const ASpearfishCharacter* Character) const
{
	return !bCollected && Character && !Character->IsBlackedOut() && !LootId.IsNone();
}

FText ASpearfishLootPickup::GetInteractPrompt(const ASpearfishCharacter* Character) const
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	return FText::Format(LOCTEXT("PickUp", "Pick up {0}"), Registry ? Registry->GetDisplayName(LootId) : FText::FromName(LootId));
}

void ASpearfishLootPickup::Interact(ASpearfishCharacter* Character)
{
	if (!HasAuthority() || bCollected || !CanInteract(Character))
	{
		return;
	}
	if (SpearfishCatch::CollectLoot(Character, LootId, Quality))
	{
		bCollected = true;
		Destroy();
	}
}

#undef LOCTEXT_NAMESPACE
