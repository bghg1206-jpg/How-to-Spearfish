#include "Boat/SpearfishStation.h"

#include "Boat/SpearfishBoat.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/SpearfishGameMode.h"
#include "Core/SpearfishGameState.h"
#include "Core/SpearfishPlayerController.h"
#include "DayNight/SpearfishDayCycleComponent.h"
#include "Diving/SpearfishCharacter.h"
#include "Engine/World.h"
#include "HowToSpearfish.h"
#include "Net/UnrealNetwork.h"
#include "Restaurant/SpearfishRestaurantComponent.h"
#include "World/SpearfishVisualSubsystem.h"

#define LOCTEXT_NAMESPACE "SpearfishStation"

ASpearfishStation::ASpearfishStation()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(true);
	SetNetUpdateFrequency(5.f);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	InteractBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractBox"));
	InteractBox->SetupAttachment(Root);
	InteractBox->SetBoxExtent(FVector(45.f, 45.f, 50.f));
	InteractBox->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	InteractBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractBox->SetCollisionResponseToChannel(ECC_Interact, ECR_Block);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 135.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(14.f);
	Label->SetTextRenderColor(FColor(255, 240, 210));
}

void ASpearfishStation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpearfishStation, StationType);
	DOREPLIFETIME(ASpearfishStation, Index);
	DOREPLIFETIME(ASpearfishStation, Boat);
	DOREPLIFETIME(ASpearfishStation, Occupant);
}

void ASpearfishStation::Configure(ESpearfishStationType InType, int32 InIndex, ASpearfishBoat* InBoat)
{
	StationType = InType;
	Index = InIndex;
	Boat = InBoat;
}

void ASpearfishStation::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASpearfishStation::OnRep_Type()
{
	BuildVisuals();
}

void ASpearfishStation::BuildVisuals()
{
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	if (bVisualsBuilt || !Visuals || !HasActorBegunPlay())
	{
		return;
	}
	bVisualsBuilt = true;
	Label->SetText(SpearfishText::StationName(StationType));

	const FLinearColor Steel(0.55f, 0.57f, 0.6f);
	const FLinearColor Wood(0.55f, 0.38f, 0.22f);
	const FLinearColor Counter(0.85f, 0.83f, 0.78f);
	auto Box = [this, Visuals](const FVector& Location, const FVector& Size, const FLinearColor& Color, bool bCollision = true, float Emissive = 0.f)
	{
		return Visuals->AddPart(this, Root, ESpearfishShape::Cube, Location, FRotator::ZeroRotator, Size, Color, bCollision, Emissive);
	};
	auto Shape = [this, Visuals](ESpearfishShape InShape, const FVector& Location, const FRotator& Rotation, const FVector& Size, const FLinearColor& Color, float Emissive = 0.f)
	{
		return Visuals->AddPart(this, Root, InShape, Location, Rotation, Size, Color, false, Emissive);
	};

	switch (StationType)
	{
	case ESpearfishStationType::Helm:
		Box(FVector(0, 0, 50), FVector(40, 60, 100), Wood);
		Shape(ESpearfishShape::Cylinder, FVector(-14, 0, 110), FRotator(0, 0, 90), FVector(46, 46, 4), FLinearColor(0.35f, 0.22f, 0.1f));
		break;
	case ESpearfishStationType::Anchor:
		Box(FVector(0, 0, 30), FVector(50, 70, 60), Steel);
		Shape(ESpearfishShape::Cylinder, FVector(0, 0, 70), FRotator(0, 0, 90), FVector(30, 30, 60), FLinearColor(0.2f, 0.2f, 0.22f));
		break;
	case ESpearfishStationType::Ladder:
		InteractBox->SetBoxExtent(FVector(60.f, 70.f, 190.f));
		InteractBox->SetRelativeLocation(FVector(0.f, 0.f, -60.f));
		Box(FVector(0, -30, -60), FVector(6, 6, 300), Steel, false);
		Box(FVector(0, 30, -60), FVector(6, 6, 300), Steel, false);
		for (int32 Rung = 0; Rung < 6; ++Rung)
		{
			Box(FVector(0, 0, -190 + Rung * 50), FVector(5, 60, 4), Steel, false);
		}
		// Swim platform.
		Box(FVector(-30, 0, -2), FVector(70, 160, 8), Wood);
		break;
	case ESpearfishStationType::GearLocker:
		Box(FVector(0, 0, 90), FVector(60, 110, 180), FLinearColor(0.15f, 0.35f, 0.55f));
		break;
	case ESpearfishStationType::Cooler:
		Box(FVector(0, 0, 40), FVector(70, 110, 80), FLinearColor(0.9f, 0.9f, 0.95f));
		Box(FVector(0, 0, 82), FVector(72, 112, 6), FLinearColor(0.2f, 0.55f, 0.85f), false);
		break;
	case ESpearfishStationType::CuttingBoard:
		Box(FVector(0, 0, 45), FVector(70, 90, 90), Counter);
		Box(FVector(0, 0, 92), FVector(50, 70, 4), Wood, false);
		Shape(ESpearfishShape::Cube, FVector(10, 20, 96), FRotator(0, 30, 0), FVector(30, 4, 2), Steel);
		break;
	case ESpearfishStationType::SpiceStation:
		Box(FVector(0, 0, 45), FVector(70, 90, 90), Counter);
		for (int32 Jar = 0; Jar < 4; ++Jar)
		{
			const FLinearColor Colors[] = { FLinearColor(0.9f, 0.2f, 0.1f), FLinearColor(0.95f, 0.75f, 0.1f), FLinearColor(0.3f, 0.6f, 0.2f), FLinearColor(0.5f, 0.3f, 0.15f) };
			Shape(ESpearfishShape::Cylinder, FVector(-10, -30 + Jar * 20, 100), FRotator::ZeroRotator, FVector(10, 10, 16), Colors[Jar]);
		}
		break;
	case ESpearfishStationType::Grill:
		Box(FVector(0, 0, 45), FVector(70, 90, 90), FLinearColor(0.15f, 0.15f, 0.16f));
		Box(FVector(0, 0, 92), FVector(60, 80, 4), FLinearColor(1.f, 0.35f, 0.05f), false, 3.f);
		break;
	case ESpearfishStationType::Fryer:
		Box(FVector(0, 0, 45), FVector(70, 90, 90), Steel);
		Box(FVector(0, 0, 90), FVector(50, 60, 6), FLinearColor(0.85f, 0.65f, 0.2f), false, 0.6f);
		break;
	case ESpearfishStationType::Stove:
		Box(FVector(0, 0, 45), FVector(70, 90, 90), FLinearColor(0.25f, 0.25f, 0.28f));
		Shape(ESpearfishShape::Cylinder, FVector(0, 0, 110), FRotator::ZeroRotator, FVector(44, 44, 30), Steel);
		break;
	case ESpearfishStationType::PlatingCounter:
		Box(FVector(0, 0, 45), FVector(70, 120, 90), Counter);
		Shape(ESpearfishShape::Cylinder, FVector(0, -25, 93), FRotator::ZeroRotator, FVector(28, 28, 2), FLinearColor::White);
		Shape(ESpearfishShape::Cylinder, FVector(0, 25, 93), FRotator::ZeroRotator, FVector(28, 28, 2), FLinearColor::White);
		break;
	case ESpearfishStationType::Pass:
		Box(FVector(0, 0, 50), FVector(60, 140, 100), Wood);
		Shape(ESpearfishShape::Sphere, FVector(0, 50, 108), FRotator::ZeroRotator, FVector(10, 10, 10), FLinearColor(1.f, 0.85f, 0.2f), 1.f);
		break;
	case ESpearfishStationType::TabletDock:
		Box(FVector(0, 0, 50), FVector(30, 40, 100), Steel);
		Shape(ESpearfishShape::Cube, FVector(0, 0, 108), FRotator(-30, 0, 0), FVector(24, 34, 2), FLinearColor(0.1f, 0.6f, 0.9f), 2.f);
		break;
	case ESpearfishStationType::Bed:
		InteractBox->SetBoxExtent(FVector(100.f, 50.f, 40.f));
		InteractBox->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
		Box(FVector(0, 0, 20), FVector(200, 90, 40), Wood);
		Box(FVector(0, 0, 45), FVector(190, 84, 12), FLinearColor(0.85f, 0.85f, 0.95f), false);
		Box(FVector(80, 0, 55), FVector(30, 60, 12), FLinearColor::White, false);
		break;
	case ESpearfishStationType::Chart:
		Box(FVector(0, 0, 45), FVector(80, 110, 90), Wood);
		Box(FVector(0, 0, 92), FVector(70, 100, 2), FLinearColor(0.9f, 0.85f, 0.65f), false);
		break;
	case ESpearfishStationType::OpenSign:
		Box(FVector(0, 0, 70), FVector(8, 8, 140), Wood);
		Box(FVector(0, 0, 150), FVector(6, 90, 40), FLinearColor(0.95f, 0.85f, 0.4f), false, 0.8f);
		break;
	case ESpearfishStationType::ShopKiosk:
		InteractBox->SetBoxExtent(FVector(120.f, 160.f, 120.f));
		InteractBox->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
		Box(FVector(0, 0, 55), FVector(160, 260, 110), Wood);
		Box(FVector(0, 0, 250), FVector(200, 300, 12), FLinearColor(0.15f, 0.55f, 0.65f));
		Box(FVector(0, -140, 180), FVector(8, 8, 140), Wood);
		Box(FVector(0, 140, 180), FVector(8, 8, 140), Wood);
		break;
	case ESpearfishStationType::JournalBoard:
		Box(FVector(0, 0, 100), FVector(10, 160, 120), Wood);
		Box(FVector(6, 0, 105), FVector(2, 140, 100), FLinearColor(0.95f, 0.92f, 0.82f), false);
		break;
	default:
		Box(FVector(0, 0, 40), FVector(60, 60, 80), Steel);
		break;
	}
}

FTransform ASpearfishStation::GetLieTransform() const
{
	return FTransform(GetActorRotation(), GetActorTransform().TransformPosition(FVector(0.f, 0.f, 50.f + 90.f)));
}

FTransform ASpearfishStation::GetExitTransform() const
{
	return FTransform(GetActorRotation(), GetActorTransform().TransformPosition(FVector(0.f, 110.f, 100.f)));
}

float ASpearfishStation::GetInteractRange() const
{
	switch (StationType)
	{
	case ESpearfishStationType::Ladder: return 380.f;
	case ESpearfishStationType::ShopKiosk: return 420.f;
	case ESpearfishStationType::Bed: return 320.f;
	default: return 280.f;
	}
}

bool ASpearfishStation::CanInteract(const ASpearfishCharacter* Character) const
{
	if (!Character)
	{
		return false;
	}
	switch (StationType)
	{
	case ESpearfishStationType::Helm:
		return Boat && !Boat->GetDriver();
	case ESpearfishStationType::Bed:
		return !Occupant || Occupant == Character;
	default:
		return true;
	}
}

FText ASpearfishStation::GetInteractPrompt(const ASpearfishCharacter* Character) const
{
	const ASpearfishGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASpearfishGameState>() : nullptr;
	const USpearfishRestaurantComponent* Restaurant = Boat ? Boat->GetRestaurant() : nullptr;
	switch (StationType)
	{
	case ESpearfishStationType::Helm:
		return LOCTEXT("Helm", "Take the helm (W/S throttle, A/D steer, E to leave)");
	case ESpearfishStationType::Anchor:
		return Boat && Boat->IsAnchored() ? LOCTEXT("RaiseAnchor", "Raise anchor") : LOCTEXT("DropAnchor", "Drop anchor (guests only come aboard at anchor)");
	case ESpearfishStationType::Ladder:
		return Character && Character->IsSwimming()
			? LOCTEXT("ClimbAboard", "Climb aboard (catch to cooler, refill air)")
			: LOCTEXT("DiveIn", "Dive in");
	case ESpearfishStationType::GearLocker:
		return LOCTEXT("Locker", "Open gear locker");
	case ESpearfishStationType::Cooler:
		return LOCTEXT("Cooler", "Check the cooler");
	case ESpearfishStationType::Pass:
		return Restaurant ? Restaurant->DescribeServe() : FText::GetEmpty();
	case ESpearfishStationType::TabletDock:
		return LOCTEXT("Tablet", "Pick up the tablet");
	case ESpearfishStationType::Bed:
	{
		if (Occupant == Character)
		{
			return LOCTEXT("GetUp", "Get up");
		}
		const USpearfishDayCycleComponent* DayCycle = GameState ? GameState->GetDayCycle() : nullptr;
		if (DayCycle && !DayCycle->CanSleepNow())
		{
			return FText::Format(LOCTEXT("TooEarly", "Too early to sleep (after {0})"), FText::FromString(SpearfishText::Clock(DayCycle->GetSchedule().SleepAllowedHour)));
		}
		return LOCTEXT("Sleep", "Sleep (the day ends when everyone is in bed)");
	}
	case ESpearfishStationType::Chart:
		return LOCTEXT("Chart", "Open the sea chart");
	case ESpearfishStationType::OpenSign:
		return Restaurant && Restaurant->IsOpen() ? LOCTEXT("Close", "Flip sign to CLOSED") : LOCTEXT("Open", "Flip sign to OPEN");
	case ESpearfishStationType::ShopKiosk:
		return LOCTEXT("Shop", "Browse the dive shop");
	case ESpearfishStationType::JournalBoard:
		return LOCTEXT("Journal", "Read the fish journal");
	default:
		if (SpearfishStations::IsCookStation(StationType) && Restaurant)
		{
			return Restaurant->DescribeStationWork(Character, StationType);
		}
		return SpearfishText::StationName(StationType);
	}
}

void ASpearfishStation::Interact(ASpearfishCharacter* Character)
{
	if (!Character)
	{
		return;
	}
	ASpearfishPlayerController* PlayerController = Cast<ASpearfishPlayerController>(Character->GetController());
	USpearfishRestaurantComponent* Restaurant = Boat ? Boat->GetRestaurant() : nullptr;
	ASpearfishGameMode* GameMode = GetWorld()->GetAuthGameMode<ASpearfishGameMode>();

	switch (StationType)
	{
	case ESpearfishStationType::Helm:
		Character->StartDriving(Boat);
		break;
	case ESpearfishStationType::Anchor:
		if (Boat)
		{
			Boat->SetAnchored(!Boat->IsAnchored());
		}
		break;
	case ESpearfishStationType::Ladder:
		if (Character->IsSwimming())
		{
			Character->ClimbAboard(Boat);
		}
		else
		{
			Character->EnterWater(Boat);
		}
		break;
	case ESpearfishStationType::GearLocker:
		if (PlayerController)
		{
			PlayerController->ClientOpenPanel(ESpearfishUIPanel::Locker);
		}
		break;
	case ESpearfishStationType::Cooler:
	case ESpearfishStationType::TabletDock:
		if (PlayerController)
		{
			PlayerController->ClientOpenPanel(ESpearfishUIPanel::Tablet);
		}
		break;
	case ESpearfishStationType::Pass:
		if (Restaurant)
		{
			Restaurant->TryServe(Character);
		}
		break;
	case ESpearfishStationType::Bed:
		if (GameMode)
		{
			if (Occupant == Character)
			{
				GameMode->HandleLeaveBed(Character);
			}
			else
			{
				GameMode->HandleEnterBed(Character, this);
			}
		}
		break;
	case ESpearfishStationType::Chart:
		if (PlayerController)
		{
			PlayerController->ClientOpenPanel(ESpearfishUIPanel::Chart);
		}
		break;
	case ESpearfishStationType::OpenSign:
		if (Restaurant)
		{
			Restaurant->SetOpen(!Restaurant->IsOpen());
		}
		break;
	case ESpearfishStationType::ShopKiosk:
		if (PlayerController)
		{
			PlayerController->ClientOpenPanel(ESpearfishUIPanel::Shop);
		}
		break;
	case ESpearfishStationType::JournalBoard:
		if (PlayerController)
		{
			PlayerController->ClientOpenPanel(ESpearfishUIPanel::Journal);
		}
		break;
	default:
		if (SpearfishStations::IsCookStation(StationType) && Restaurant)
		{
			Restaurant->TryBeginCookStep(Character, this);
		}
		break;
	}
}

#undef LOCTEXT_NAMESPACE
