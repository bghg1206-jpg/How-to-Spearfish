#include "Customers/SpearfishCustomer.h"

#include "Boat/SpearfishBoat.h"
#include "Components/TextRenderComponent.h"
#include "Data/SpearfishDataRegistry.h"
#include "Net/UnrealNetwork.h"
#include "Restaurant/SpearfishRestaurantComponent.h"
#include "World/SpearfishVisualSubsystem.h"

namespace SpearfishCustomerPrivate
{
	constexpr float WalkSpeed = 130.f;
	const FVector AislePoint(330.f, 0.f, 210.f);
}

ASpearfishCustomer::ASpearfishCustomer()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(4.f);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	MoodText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MoodText"));
	MoodText->SetupAttachment(Root);
	MoodText->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	MoodText->SetHorizontalAlignment(EHTA_Center);
	MoodText->SetWorldSize(22.f);
}

void ASpearfishCustomer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpearfishCustomer, CustomerId);
	DOREPLIFETIME(ASpearfishCustomer, SeatIndex);
	DOREPLIFETIME(ASpearfishCustomer, GuestId);
	DOREPLIFETIME(ASpearfishCustomer, Seed);
	DOREPLIFETIME(ASpearfishCustomer, GuestState);
	DOREPLIFETIME(ASpearfishCustomer, bHappy);
	DOREPLIFETIME(ASpearfishCustomer, Boat);
}

void ASpearfishCustomer::InitGuest(FName InCustomerId, int32 InSeatIndex, int32 InGuestId, ASpearfishBoat* InBoat, int32 InSeed)
{
	CustomerId = InCustomerId;
	SeatIndex = InSeatIndex;
	GuestId = InGuestId;
	Boat = InBoat;
	Seed = InSeed;
}

void ASpearfishCustomer::BeginPlay()
{
	Super::BeginPlay();
	AttachToBoat();
	BuildBody();
}

void ASpearfishCustomer::OnRep_Boat()
{
	AttachToBoat();
	BuildBody();
}

void ASpearfishCustomer::AttachToBoat()
{
	if (!Boat || GetAttachParentActor() == Boat)
	{
		return;
	}
	AttachToActor(Boat, FAttachmentTransformRules::KeepWorldTransform);
	RelativeLocation = Boat->GetRelativeBoardingPoint();
	SetActorRelativeLocation(RelativeLocation);
}

void ASpearfishCustomer::BuildBody()
{
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	if (bBuilt || !Visuals || !HasActorBegunPlay() || CustomerId.IsNone())
	{
		return;
	}
	bBuilt = true;

	FRandomStream Rng(Seed);
	const FSpearfishCustomerDef* Def = Registry ? Registry->FindCustomer(CustomerId) : nullptr;
	const FLinearColor Outfit = Def ? Def->OutfitColor : FLinearColor(0.8f, 0.3f, 0.3f);
	const FLinearColor Skins[] = { FLinearColor(0.95f, 0.78f, 0.62f), FLinearColor(0.75f, 0.55f, 0.4f), FLinearColor(0.5f, 0.35f, 0.25f), FLinearColor(0.35f, 0.24f, 0.18f) };
	const FLinearColor Skin = Skins[Rng.RandRange(0, 3)];
	const FLinearColor Pants(Rng.FRandRange(0.1f, 0.4f), Rng.FRandRange(0.1f, 0.4f), Rng.FRandRange(0.2f, 0.5f));

	// Origin at the feet.
	Visuals->AddPart(this, Root, ESpearfishShape::Cylinder, FVector(0, 0, 40), FRotator::ZeroRotator, FVector(30, 26, 80), Pants, false);
	Visuals->AddPart(this, Root, ESpearfishShape::Cylinder, FVector(0, 0, 112), FRotator::ZeroRotator, FVector(40, 30, 64), Outfit, false);
	Visuals->AddPart(this, Root, ESpearfishShape::Sphere, FVector(0, 0, 160), FRotator::ZeroRotator, FVector(24, 24, 27), Skin, false);
	const int32 Hat = Rng.RandRange(0, 3);
	if (Hat == 1)
	{
		Visuals->AddPart(this, Root, ESpearfishShape::Cylinder, FVector(0, 0, 176), FRotator::ZeroRotator, FVector(46, 46, 3), FLinearColor(0.9f, 0.82f, 0.55f), false);
		Visuals->AddPart(this, Root, ESpearfishShape::Cylinder, FVector(0, 0, 182), FRotator::ZeroRotator, FVector(22, 22, 12), FLinearColor(0.9f, 0.82f, 0.55f), false);
	}
	else if (Hat == 2)
	{
		Visuals->AddPart(this, Root, ESpearfishShape::Sphere, FVector(0, 0, 172), FRotator::ZeroRotator, FVector(26, 26, 14), Outfit * 0.6f, false);
	}
	MoodText->SetRelativeLocation(FVector(0.f, 0.f, 205.f));
}

FVector ASpearfishCustomer::GetTargetRelative() const
{
	if (!Boat)
	{
		return RelativeLocation;
	}
	switch (GuestState)
	{
	case ESpearfishGuestState::Arriving:
	case ESpearfishGuestState::Seated:
		return Boat->GetRelativeSeatPoint(SeatIndex) - FVector(0.f, 0.f, 90.f);
	case ESpearfishGuestState::Leaving:
	default:
		return Boat->GetRelativeBoardingPoint() - FVector(0.f, 0.f, 90.f);
	}
}

void ASpearfishCustomer::Leave(bool bInHappy)
{
	if (GuestState == ESpearfishGuestState::Leaving)
	{
		return;
	}
	bHappy = bInHappy;
	GuestState = ESpearfishGuestState::Leaving;
	ForceNetUpdate();
}

void ASpearfishCustomer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Boat)
	{
		return;
	}
	AttachToBoat();

	// Walk through the aisle point so guests don't clip through tables.
	const FVector Target = GetTargetRelative();
	const FVector Aisle = SpearfishCustomerPrivate::AislePoint - FVector(0.f, 0.f, 90.f);
	const bool bUseAisle = FMath::Abs(RelativeLocation.Y - Target.Y) > 120.0 && FVector::DistSquared2D(RelativeLocation, Aisle) > FMath::Square(40.0);
	const FVector Waypoint = bUseAisle && GuestState != ESpearfishGuestState::Seated ? Aisle : Target;
	const FVector Delta = Waypoint - RelativeLocation;
	const double Distance = Delta.Size();
	const double Step = SpearfishCustomerPrivate::WalkSpeed * DeltaSeconds;
	if (Distance > 1.0)
	{
		RelativeLocation += Delta / Distance * FMath::Min(Step, Distance);
		SetActorRelativeRotation(FRotator(0.f, static_cast<float>(Delta.Rotation().Yaw), 0.f));
	}
	SetActorRelativeLocation(RelativeLocation);

	const bool bArrived = FVector::DistSquared(RelativeLocation, Target) < FMath::Square(10.0);
	if (HasAuthority() && bArrived)
	{
		if (GuestState == ESpearfishGuestState::Arriving && !bReportedSeated)
		{
			bReportedSeated = true;
			GuestState = ESpearfishGuestState::Seated;
			if (USpearfishRestaurantComponent* Restaurant = Boat->GetRestaurant())
			{
				Restaurant->OnGuestSeated(this);
			}
		}
		else if (GuestState == ESpearfishGuestState::Leaving)
		{
			if (USpearfishRestaurantComponent* Restaurant = Boat->GetRestaurant())
			{
				Restaurant->OnGuestLeft(this);
			}
			Destroy();
			return;
		}
	}

	MoodTimer -= DeltaSeconds;
	if (MoodTimer <= 0.f)
	{
		MoodTimer = 0.5f;
		UpdateMood();
	}
}

void ASpearfishCustomer::UpdateMood()
{
	const USpearfishRestaurantComponent* Restaurant = Boat ? Boat->GetRestaurant() : nullptr;
	if (!Restaurant)
	{
		return;
	}
	if (GuestState == ESpearfishGuestState::Leaving)
	{
		MoodText->SetText(FText::FromString(bHappy ? TEXT(":)") : TEXT(">:(")));
		MoodText->SetTextRenderColor(bHappy ? FColor(120, 255, 120) : FColor(255, 90, 80));
		return;
	}

	// Patience as a little bar; the chef's tablet has the details.
	float Lowest = 1.f;
	bool bHasOrder = false;
	for (const FSpearfishOrder& Order : Restaurant->GetOrders())
	{
		if (Order.GuestId == GuestId && (Order.State == ESpearfishOrderState::Waiting || Order.State == ESpearfishOrderState::Cooking || Order.State == ESpearfishOrderState::Ready))
		{
			bHasOrder = true;
			Lowest = FMath::Min(Lowest, Restaurant->GetPatienceFraction(Order));
		}
	}
	if (!bHasOrder)
	{
		MoodText->SetText(FText::GetEmpty());
		return;
	}
	const int32 Filled = FMath::Clamp(FMath::CeilToInt(Lowest * 5.f), 0, 5);
	FString Bar;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Bar += Index < Filled ? TEXT("|") : TEXT(".");
	}
	MoodText->SetText(FText::FromString(Lowest < 0.25f ? Bar + TEXT(" !") : Bar));
	MoodText->SetTextRenderColor(Lowest > 0.5f ? FColor(160, 255, 160) : (Lowest > 0.25f ? FColor(255, 220, 90) : FColor(255, 90, 80)));
}
