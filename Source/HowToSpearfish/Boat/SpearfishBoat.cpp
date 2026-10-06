#include "Boat/SpearfishBoat.h"

#include "Boat/SpearfishStation.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/SpearfishGameState.h"
#include "Diving/SpearfishCharacter.h"
#include "Engine/World.h"
#include "HowToSpearfish.h"
#include "Inventory/SpearfishCatchStorageComponent.h"
#include "Net/UnrealNetwork.h"
#include "Restaurant/SpearfishAutoChefComponent.h"
#include "Restaurant/SpearfishRestaurantComponent.h"
#include "World/SpearfishOceanSubsystem.h"
#include "World/SpearfishVisualSubsystem.h"

#define LOCTEXT_NAMESPACE "SpearfishBoat"

namespace SpearfishBoatPrivate
{
	constexpr float DeckTop = 120.f;
	constexpr float StandZ = DeckTop + 90.f;
	constexpr float MaxForwardSpeed = 900.f;
	constexpr float MaxReverseSpeed = 300.f;
	constexpr float TurnRate = 30.f;
	constexpr float MinDepthCm = 180.f;

	const FVector SeatPoints[8] = {
		FVector(420, -210, StandZ), FVector(540, -210, StandZ),
		FVector(420, 210, StandZ), FVector(540, 210, StandZ),
		FVector(700, -170, StandZ), FVector(800, -130, StandZ),
		FVector(700, 170, StandZ), FVector(800, 130, StandZ)
	};
}

ASpearfishBoat::ASpearfishBoat()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(30.f);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	HelmSeat = CreateDefaultSubobject<USceneComponent>(TEXT("HelmSeat"));
	HelmSeat->SetupAttachment(Root);
	HelmSeat->SetRelativeLocation(FVector(-230.f, 160.f, SpearfishBoatPrivate::StandZ));

	MastLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MastLight"));
	MastLight->SetupAttachment(Root);
	MastLight->SetRelativeLocation(FVector(-120.f, 0.f, 620.f));
	MastLight->SetIntensity(4000.f);
	MastLight->SetAttenuationRadius(2500.f);
	MastLight->SetLightColor(FLinearColor(1.f, 0.85f, 0.6f));
	MastLight->SetCastShadows(false);

	Cooler = CreateDefaultSubobject<USpearfishCatchStorageComponent>(TEXT("Cooler"));
	Restaurant = CreateDefaultSubobject<USpearfishRestaurantComponent>(TEXT("Restaurant"));
	AutoChef = CreateDefaultSubobject<USpearfishAutoChefComponent>(TEXT("AutoChef"));
}

void ASpearfishBoat::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpearfishBoat, NetState);
	DOREPLIFETIME(ASpearfishBoat, bAnchored);
	DOREPLIFETIME(ASpearfishBoat, Driver);
	DOREPLIFETIME(ASpearfishBoat, Stations);
}

void ASpearfishBoat::BeginPlay()
{
	Super::BeginPlay();
	Yaw = static_cast<float>(GetActorRotation().Yaw);
	NetState.Location = GetActorLocation();
	NetState.Yaw = Yaw;
	BuildHull();
}

void ASpearfishBoat::BuildHull()
{
	using namespace SpearfishBoatPrivate;
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	if (bHullBuilt || !Visuals)
	{
		return;
	}
	bHullBuilt = true;

	const FLinearColor HullWhite(0.92f, 0.93f, 0.95f);
	const FLinearColor HullStripe(0.08f, 0.35f, 0.55f);
	const FLinearColor Teak(0.58f, 0.4f, 0.24f);
	const FLinearColor Cabin(0.95f, 0.9f, 0.82f);
	const FLinearColor Roof(0.75f, 0.25f, 0.2f);
	auto Part = [this, Visuals](ESpearfishShape Shape, const FVector& Location, const FRotator& Rotation, const FVector& Size, const FLinearColor& Color, bool bCollision, float Emissive = 0.f)
	{
		return Visuals->AddPart(this, Root, Shape, Location, Rotation, Size, Color, bCollision, Emissive);
	};

	// Hull and deck.
	Part(ESpearfishShape::Cube, FVector(0, 0, 10), FRotator::ZeroRotator, FVector(1700, 640, 200), HullWhite, true);
	Part(ESpearfishShape::Cube, FVector(850, 0, 10), FRotator(0, 45, 0), FVector(452, 452, 200), HullWhite, true);
	Part(ESpearfishShape::Cube, FVector(0, 0, -40), FRotator::ZeroRotator, FVector(1710, 650, 30), HullStripe, false);
	Part(ESpearfishShape::Cube, FVector(0, 0, DeckTop - 10), FRotator::ZeroRotator, FVector(1690, 620, 20), Teak, true);
	Part(ESpearfishShape::Cube, FVector(850, 0, DeckTop - 10), FRotator(0, 45, 0), FVector(440, 440, 20), Teak, true);

	// Rails (with a gap at the stern for the ladder and on the starboard side for the guest gangway).
	Part(ESpearfishShape::Cube, FVector(0, -318, DeckTop + 45), FRotator::ZeroRotator, FVector(1700, 8, 90), HullWhite, true);
	Part(ESpearfishShape::Cube, FVector(-300, 318, DeckTop + 45), FRotator::ZeroRotator, FVector(1100, 8, 90), HullWhite, true);
	Part(ESpearfishShape::Cube, FVector(700, 318, DeckTop + 45), FRotator::ZeroRotator, FVector(400, 8, 90), HullWhite, true);
	Part(ESpearfishShape::Cube, FVector(-848, -200, DeckTop + 45), FRotator::ZeroRotator, FVector(8, 240, 90), HullWhite, true);
	Part(ESpearfishShape::Cube, FVector(-848, 200, DeckTop + 45), FRotator::ZeroRotator, FVector(8, 240, 90), HullWhite, true);

	// Cabin with the bunks (stern half), door facing the bow.
	Part(ESpearfishShape::Cube, FVector(-650, 0, DeckTop + 115), FRotator::ZeroRotator, FVector(10, 620, 230), Cabin, true);
	Part(ESpearfishShape::Cube, FVector(-250, -190, DeckTop + 115), FRotator::ZeroRotator, FVector(10, 240, 230), Cabin, true);
	Part(ESpearfishShape::Cube, FVector(-250, 190, DeckTop + 115), FRotator::ZeroRotator, FVector(10, 240, 230), Cabin, true);
	Part(ESpearfishShape::Cube, FVector(-450, -310, DeckTop + 115), FRotator::ZeroRotator, FVector(400, 10, 230), Cabin, true);
	Part(ESpearfishShape::Cube, FVector(-450, 310, DeckTop + 115), FRotator::ZeroRotator, FVector(400, 10, 230), Cabin, true);
	Part(ESpearfishShape::Cube, FVector(-450, 0, DeckTop + 236), FRotator::ZeroRotator, FVector(440, 660, 14), Roof, true);
	Part(ESpearfishShape::Sphere, FVector(-450, 0, DeckTop + 200), FRotator::ZeroRotator, FVector(14, 14, 14), FLinearColor(1.f, 0.85f, 0.6f), false, 6.f);

	// Kitchen awning and dining canopy.
	Part(ESpearfishShape::Cube, FVector(80, 0, DeckTop + 260), FRotator::ZeroRotator, FVector(620, 640, 8), FLinearColor(0.95f, 0.55f, 0.2f), false);
	for (const FVector& Post : { FVector(-210, -300, 0), FVector(-210, 300, 0), FVector(380, -300, 0), FVector(380, 300, 0) })
	{
		Part(ESpearfishShape::Cylinder, FVector(Post.X, Post.Y, DeckTop + 130), FRotator::ZeroRotator, FVector(10, 10, 260), Teak, true);
	}

	// Dining tables (4 tables x 2 seats; seats beyond the owned count stay empty).
	for (const FVector& Table : { FVector(480, -210, 0), FVector(480, 210, 0), FVector(750, -150, 0), FVector(750, 150, 0) })
	{
		Part(ESpearfishShape::Cylinder, FVector(Table.X, Table.Y, DeckTop + 38), FRotator::ZeroRotator, FVector(10, 10, 76), Teak, true);
		Part(ESpearfishShape::Cylinder, FVector(Table.X, Table.Y, DeckTop + 78), FRotator::ZeroRotator, FVector(90, 90, 5), FLinearColor(0.95f, 0.95f, 0.9f), true);
	}

	// Mast with the dive flag and a bright lamp, visible from far away and from below.
	Part(ESpearfishShape::Cylinder, FVector(-120, 0, DeckTop + 260), FRotator::ZeroRotator, FVector(12, 12, 520), Teak, true);
	Part(ESpearfishShape::Cube, FVector(-120, 45, DeckTop + 460), FRotator::ZeroRotator, FVector(4, 80, 55), FLinearColor(0.85f, 0.1f, 0.1f), false);
	Part(ESpearfishShape::Cube, FVector(-120, 45, DeckTop + 460), FRotator(0, 0, 35), FVector(5, 90, 8), FLinearColor::White, false);
	Part(ESpearfishShape::Sphere, FVector(-120, 0, 620), FRotator::ZeroRotator, FVector(22, 22, 22), FLinearColor(1.f, 0.9f, 0.6f), false, 10.f);

	// Weighted descent line off the stern: an underwater beacon back to the ladder.
	Part(ESpearfishShape::Cylinder, FVector(-930, 0, -600), FRotator::ZeroRotator, FVector(2, 2, 1200), FLinearColor(1.f, 0.6f, 0.1f), false, 1.5f);
	Part(ESpearfishShape::Sphere, FVector(-930, 0, -1200), FRotator::ZeroRotator, FVector(28, 28, 28), FLinearColor(1.f, 0.55f, 0.1f), false, 6.f);
	Part(ESpearfishShape::Sphere, FVector(-930, 0, -400), FRotator::ZeroRotator, FVector(16, 16, 16), FLinearColor(1.f, 0.9f, 0.3f), false, 6.f);
}

ASpearfishStation* ASpearfishBoat::SpawnStation(ESpearfishStationType Type, int32 Index, const FVector& RelativeLocation, float RelativeYaw)
{
	UWorld* World = GetWorld();
	const FTransform Transform(FRotator(0.f, RelativeYaw, 0.f), RelativeLocation);
	const FTransform WorldTransform = Transform * GetActorTransform();
	ASpearfishStation* Station = World->SpawnActorDeferred<ASpearfishStation>(ASpearfishStation::StaticClass(), WorldTransform, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Station)
	{
		return nullptr;
	}
	Station->Configure(Type, Index, this);
	Station->FinishSpawning(WorldTransform);
	Station->AttachToComponent(Root, FAttachmentTransformRules::KeepWorldTransform);
	Stations.Add(Station);
	return Station;
}

void ASpearfishBoat::SpawnStations()
{
	using namespace SpearfishBoatPrivate;
	if (!HasAuthority() || Stations.Num() > 0)
	{
		return;
	}
	const float Deck = DeckTop;
	SpawnStation(ESpearfishStationType::Helm, 0, FVector(-170.f, 160.f, Deck), 0.f);
	SpawnStation(ESpearfishStationType::Anchor, 0, FVector(860.f, 0.f, Deck), 0.f);
	SpawnStation(ESpearfishStationType::Ladder, 0, FVector(-880.f, 0.f, Deck), 180.f);
	SpawnStation(ESpearfishStationType::GearLocker, 0, FVector(-760.f, -240.f, Deck), 90.f);
	SpawnStation(ESpearfishStationType::Chart, 0, FVector(-760.f, 240.f, Deck), -90.f);

	// Kitchen line along the port side, facing the deck.
	SpawnStation(ESpearfishStationType::CuttingBoard, 0, FVector(-130.f, -255.f, Deck), 90.f);
	SpawnStation(ESpearfishStationType::SpiceStation, 0, FVector(-30.f, -255.f, Deck), 90.f);
	SpawnStation(ESpearfishStationType::Grill, 0, FVector(70.f, -255.f, Deck), 90.f);
	SpawnStation(ESpearfishStationType::Fryer, 0, FVector(170.f, -255.f, Deck), 90.f);
	SpawnStation(ESpearfishStationType::Stove, 0, FVector(270.f, -255.f, Deck), 90.f);
	// Service side.
	SpawnStation(ESpearfishStationType::Cooler, 0, FVector(-120.f, 250.f, Deck), -90.f);
	SpawnStation(ESpearfishStationType::TabletDock, 0, FVector(-20.f, 270.f, Deck), -90.f);
	SpawnStation(ESpearfishStationType::PlatingCounter, 0, FVector(80.f, 250.f, Deck), -90.f);
	SpawnStation(ESpearfishStationType::Pass, 0, FVector(250.f, 220.f, Deck), -90.f);
	SpawnStation(ESpearfishStationType::OpenSign, 0, FVector(340.f, 300.f, Deck), -90.f);

	// Bunks in the cabin.
	SpawnStation(ESpearfishStationType::Bed, 0, FVector(-480.f, -230.f, Deck), 0.f);
	SpawnStation(ESpearfishStationType::Bed, 1, FVector(-480.f, 230.f, Deck), 0.f);
}

ASpearfishStation* ASpearfishBoat::FindStation(ESpearfishStationType Type, int32 Index) const
{
	for (ASpearfishStation* Station : Stations)
	{
		if (Station && Station->GetStationType() == Type && Station->GetIndex() == Index)
		{
			return Station;
		}
	}
	return nullptr;
}

ASpearfishStation* ASpearfishBoat::FindBedOccupiedBy(const ASpearfishCharacter* Character) const
{
	for (ASpearfishStation* Station : Stations)
	{
		if (Station && Station->GetStationType() == ESpearfishStationType::Bed && Station->GetOccupant() == Character)
		{
			return Station;
		}
	}
	return nullptr;
}

// ------------------------------------------------------------------------------------ Layout

FTransform ASpearfishBoat::GetHelmExitTransform() const
{
	return FTransform(GetActorRotation(), GetActorTransform().TransformPosition(FVector(-230.f, 60.f, SpearfishBoatPrivate::StandZ)));
}

FTransform ASpearfishBoat::GetLadderTopTransform() const
{
	return FTransform(GetActorRotation(), GetActorTransform().TransformPosition(FVector(-780.f, 0.f, SpearfishBoatPrivate::StandZ)));
}

FTransform ASpearfishBoat::GetLadderWaterTransform() const
{
	const FRotator Facing = GetActorRotation() + FRotator(0.f, 180.f, 0.f);
	return FTransform(Facing, GetActorTransform().TransformPosition(FVector(-1020.f, 0.f, -130.f)));
}

FTransform ASpearfishBoat::GetDeckSpawnTransform(int32 SeatIndex) const
{
	const FVector Local = SeatIndex % 2 == 0 ? FVector(-60.f, 60.f, SpearfishBoatPrivate::StandZ) : FVector(-60.f, -60.f, SpearfishBoatPrivate::StandZ);
	return FTransform(GetActorRotation(), GetActorTransform().TransformPosition(Local));
}

FVector ASpearfishBoat::GetRelativeBoardingPoint() const
{
	return FVector(330.f, 290.f, SpearfishBoatPrivate::StandZ);
}

FVector ASpearfishBoat::GetBoardingPoint() const
{
	return GetActorTransform().TransformPosition(GetRelativeBoardingPoint());
}

FVector ASpearfishBoat::GetRelativeSeatPoint(int32 SeatIndex) const
{
	return SpearfishBoatPrivate::SeatPoints[FMath::Clamp(SeatIndex, 0, 7)];
}

FTransform ASpearfishBoat::GetCustomerSeatTransform(int32 SeatIndex) const
{
	return FTransform(GetActorRotation(), GetActorTransform().TransformPosition(GetRelativeSeatPoint(SeatIndex)));
}

// ----------------------------------------------------------------------------------- Driving

void ASpearfishBoat::SetDriver(ASpearfishCharacter* InDriver)
{
	Driver = InDriver;
	Throttle = 0.f;
	Steer = 0.f;
	if (InDriver)
	{
		bAutopilot = false;
	}
}

void ASpearfishBoat::SetDriverInput(float InThrottle, float InSteer)
{
	Throttle = InThrottle;
	Steer = InSteer;
}

void ASpearfishBoat::SetAnchored(bool bInAnchored)
{
	if (bAnchored == bInAnchored)
	{
		return;
	}
	bAnchored = bInAnchored;
	if (ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>())
	{
		GameState->BroadcastNotice(bAnchored ? LOCTEXT("Anchored", "Anchor down - guests can come aboard.") : LOCTEXT("Weighed", "Anchor up - the boat is free to move."),
			ESpearfishNoticeType::Info);
	}
}

void ASpearfishBoat::SetAutopilotTarget(const FVector& Target)
{
	if (Driver)
	{
		return;
	}
	bAutopilot = true;
	AutopilotTarget = Target;
	bAnchored = false;
}

void ASpearfishBoat::PlaceAt(const FVector& Location, float InYaw)
{
	Yaw = InYaw;
	Speed = 0.f;
	SetActorLocationAndRotation(Location, FRotator(0.f, Yaw, 0.f));
	NetState.Location = Location;
	NetState.Yaw = Yaw;
	NetState.Speed = 0.f;
}

void ASpearfishBoat::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		SimulateServer(DeltaSeconds);
	}
	else
	{
		SmoothClient(DeltaSeconds);
	}
}

void ASpearfishBoat::SimulateServer(float DeltaSeconds)
{
	using namespace SpearfishBoatPrivate;
	ShallowWarningCooldown = FMath::Max(0.f, ShallowWarningCooldown - DeltaSeconds);

	float InputThrottle = Driver ? Throttle : 0.f;
	float InputSteer = Driver ? Steer : 0.f;

	if (bAutopilot && !Driver)
	{
		const FVector ToTarget = FVector(AutopilotTarget.X, AutopilotTarget.Y, 0.0) - FVector(GetActorLocation().X, GetActorLocation().Y, 0.0);
		const float Distance = static_cast<float>(ToTarget.Size());
		if (Distance < 500.f)
		{
			bAutopilot = false;
			SetAnchored(true);
		}
		else
		{
			const float TargetYaw = static_cast<float>(ToTarget.Rotation().Yaw);
			const float Delta = FMath::FindDeltaAngleDegrees(Yaw, TargetYaw);
			InputSteer = FMath::Clamp(Delta / 30.f, -1.f, 1.f);
			InputThrottle = FMath::Clamp(Distance / 1500.f, 0.25f, 0.7f) * (FMath::Abs(Delta) > 60.f ? 0.3f : 1.f);
		}
	}

	if (bAnchored)
	{
		InputThrottle = 0.f;
		InputSteer = 0.f;
		Speed = FMath::FInterpTo(Speed, 0.f, DeltaSeconds, 3.f);
	}
	else
	{
		const float TargetSpeed = InputThrottle >= 0.f ? InputThrottle * MaxForwardSpeed : InputThrottle * MaxReverseSpeed;
		Speed = FMath::FInterpTo(Speed, TargetSpeed, DeltaSeconds, 0.6f);
	}

	const float SteerAuthority = FMath::Clamp(FMath::Abs(Speed) / 250.f, 0.25f, 1.f) * (Speed >= 0.f ? 1.f : -1.f);
	Yaw = static_cast<float>(FRotator::NormalizeAxis(Yaw + InputSteer * TurnRate * SteerAuthority * DeltaSeconds));

	const FVector Forward = FRotator(0.f, Yaw, 0.f).Vector();
	FVector Location = GetActorLocation() + Forward * (Speed * DeltaSeconds);

	if (const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this))
	{
		Location.Z = Ocean->GetSeaLevel();
		// Keep off reefs and beaches: test the bow and the stern.
		const FVector Bow = Location + Forward * 950.0;
		const FVector Stern = Location - Forward * 900.0;
		const bool bShallow = Ocean->GetSeaLevel() - Ocean->GetSeabedZ(Speed >= 0.f ? Bow : Stern) < MinDepthCm;
		if (bShallow && !FMath::IsNearlyZero(Speed))
		{
			Location = GetActorLocation();
			Speed = 0.f;
			bAutopilot = false;
			if (ShallowWarningCooldown <= 0.f)
			{
				ShallowWarningCooldown = 4.f;
				if (Driver)
				{
					Driver->NotifyOwner(LOCTEXT("Shallow", "Too shallow! Back off before you hit the reef."), ESpearfishNoticeType::Warning);
				}
			}
		}
		// The region edge has a strong current that turns boats around.
		const float Outside = Ocean->GetOutOfBoundsDistance(Location);
		if (Outside > 0.f)
		{
			Location -= FVector(Location.X, Location.Y, 0.0).GetSafeNormal() * static_cast<double>(FMath::Min(Outside, 300.f * DeltaSeconds + Outside * 0.5f));
		}
	}

	SetActorLocationAndRotation(Location, FRotator(0.f, Yaw, 0.f));
	NetState.Location = Location;
	NetState.Yaw = Yaw;
	NetState.Speed = Speed;
}

void ASpearfishBoat::SmoothClient(float DeltaSeconds)
{
	// Extrapolate a little to hide latency, then ease toward it.
	const FVector Forward = FRotator(0.f, NetState.Yaw, 0.f).Vector();
	const FVector Target = FVector(NetState.Location) + Forward * (NetState.Speed * 0.08f);
	const FVector Current = GetActorLocation();
	const FVector NewLocation = FVector::DistSquared(Current, Target) > FMath::Square(1500.0) ? Target : FMath::VInterpTo(Current, Target, DeltaSeconds, 8.f);
	const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), FRotator(0.f, NetState.Yaw, 0.f), DeltaSeconds, 8.f);
	SetActorLocationAndRotation(NewLocation, NewRotation);
}

#undef LOCTEXT_NAMESPACE
