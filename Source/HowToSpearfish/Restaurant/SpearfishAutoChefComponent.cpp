#include "Restaurant/SpearfishAutoChefComponent.h"

#include "Boat/SpearfishBoat.h"
#include "Core/SpearfishGameState.h"
#include "Core/SpearfishSettings.h"
#include "DayNight/SpearfishDayCycleComponent.h"
#include "Engine/World.h"
#include "Progression/SpearfishProgressionComponent.h"
#include "Restaurant/SpearfishRestaurantComponent.h"

USpearfishAutoChefComponent::USpearfishAutoChefComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USpearfishAutoChefComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (const ASpearfishBoat* Boat = Cast<ASpearfishBoat>(GetOwner()))
		{
			Boat->GetRestaurant()->OnOrderPlaced.AddUObject(this, &USpearfishAutoChefComponent::OnOrderPlaced);
		}
	}
}

bool USpearfishAutoChefComponent::IsActive() const
{
	const ASpearfishGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASpearfishGameState>() : nullptr;
	return GameState && GameState->IsAutoChefActive();
}

float USpearfishAutoChefComponent::GetSkill() const
{
	const ASpearfishGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASpearfishGameState>() : nullptr;
	const float Bonus = GameState && GameState->GetProgression() ? GameState->GetProgression()->GetAutoChefSkillBonus() : 0.f;
	return FMath::Clamp(USpearfishSettings::Get()->AutoChefBaseSkill + Bonus, 0.f, 0.95f);
}

void USpearfishAutoChefComponent::OnOrderPlaced(const FSpearfishOrder& Order)
{
	// Give the restaurant a moment to refresh its "kitchen needs" list before radioing.
	bPendingRadio = true;
	RadioCooldown = FMath::Max(RadioCooldown, 1.25f);
}

void USpearfishAutoChefComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwnerRole() != ROLE_Authority || !IsActive())
	{
		return;
	}
	ASpearfishBoat* Boat = Cast<ASpearfishBoat>(GetOwner());
	USpearfishRestaurantComponent* Restaurant = Boat ? Boat->GetRestaurant() : nullptr;
	const ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	const USpearfishDayCycleComponent* DayCycle = GameState ? GameState->GetDayCycle() : nullptr;
	if (!Restaurant || !DayCycle)
	{
		return;
	}

	// Cook continuously (per-frame step timers), think twice a second.
	const float Skill = GetSkill();
	TArray<int32> AutoDishes;
	for (const FSpearfishDish& Dish : Restaurant->GetDishes())
	{
		if (Dish.bAutoChef)
		{
			AutoDishes.Add(Dish.DishId);
		}
	}
	for (const int32 DishId : AutoDishes)
	{
		Restaurant->TickAutoStep(DishId, DeltaTime, Skill);
	}

	RadioCooldown = FMath::Max(0.f, RadioCooldown - DeltaTime);
	ThinkTimer -= DeltaTime;
	if (ThinkTimer > 0.f)
	{
		return;
	}
	ThinkTimer = 0.5f;

	// Open for service while anchored; close at night.
	const bool bServiceTime = DayCycle->AcceptsNewGuests();
	if (bServiceTime && Boat->IsAnchored() && !Restaurant->IsOpen())
	{
		Restaurant->SetOpen(true);
	}

	// Start whatever the cooler allows, oldest guests first.
	TArray<int32> Startable;
	for (const FSpearfishOrder& Order : Restaurant->GetOrders())
	{
		if (Restaurant->CanStartOrder(Order))
		{
			Startable.Add(Order.OrderId);
		}
	}
	for (const int32 OrderId : Startable)
	{
		FText Reason;
		Restaurant->TryStartDish(OrderId, nullptr, true, Reason);
	}

	// Serve ready dishes (including ones a human finished).
	TArray<int32> Ready;
	for (const FSpearfishDish& Dish : Restaurant->GetDishes())
	{
		if (Dish.State == ESpearfishDishState::Ready)
		{
			Ready.Add(Dish.DishId);
		}
	}
	for (const int32 DishId : Ready)
	{
		Restaurant->ServeDish(DishId, true);
	}

	if (bPendingRadio && RadioCooldown <= 0.f)
	{
		RadioNeeds(false);
	}
}

void USpearfishAutoChefComponent::RadioNeeds(bool bForce)
{
	ASpearfishBoat* Boat = Cast<ASpearfishBoat>(GetOwner());
	ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	if (!Boat || !GameState)
	{
		return;
	}
	bPendingRadio = false;
	RadioCooldown = 8.f;

	for (const FSpearfishKitchenNeed& Need : Boat->GetRestaurant()->GetKitchenNeeds())
	{
		FSpearfishQuickMessageData Message;
		Message.Type = ESpearfishQuickMessage::KitchenNeeds;
		Message.SenderName = TEXT("Kitchen");
		Message.SenderRole = ESpearfishRole::Chef;
		Message.Param = Need.SpeciesId.IsNone() ? Need.Category : Need.SpeciesId;
		Message.Count = Need.Count;
		Message.MinLengthCm = Need.MinLengthCm;
		Message.ServerTime = GameState->GetServerTime();
		GameState->BroadcastQuickMessage(Message);
	}
}
