#include "Restaurant/SpearfishRestaurantComponent.h"

#include "Boat/SpearfishBoat.h"
#include "Boat/SpearfishStation.h"
#include "Core/SpearfishGameState.h"
#include "Core/SpearfishPlayerController.h"
#include "Core/SpearfishPlayerState.h"
#include "Core/SpearfishSettings.h"
#include "Customers/SpearfishCustomer.h"
#include "Data/SpearfishDataRegistry.h"
#include "DayNight/SpearfishDayCycleComponent.h"
#include "Diving/SpearfishCharacter.h"
#include "Engine/World.h"
#include "HowToSpearfish.h"
#include "Inventory/SpearfishCatchStorageComponent.h"
#include "Net/UnrealNetwork.h"
#include "Progression/SpearfishProgressionComponent.h"
#include "Rules/CookingRules.h"
#include "Rules/OrderRules.h"
#include "World/SpearfishOceanSubsystem.h"

#define LOCTEXT_NAMESPACE "SpearfishRestaurant"

namespace SpearfishRestaurantPrivate
{
	constexpr float LockSeconds = 25.f;
}

USpearfishRestaurantComponent::USpearfishRestaurantComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
}

void USpearfishRestaurantComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USpearfishRestaurantComponent, bOpen);
	DOREPLIFETIME(USpearfishRestaurantComponent, Orders);
	DOREPLIFETIME(USpearfishRestaurantComponent, Dishes);
	DOREPLIFETIME(USpearfishRestaurantComponent, KitchenNeeds);
}

ASpearfishBoat* USpearfishRestaurantComponent::GetBoat() const
{
	return Cast<ASpearfishBoat>(GetOwner());
}

float USpearfishRestaurantComponent::ServerNow() const
{
	const ASpearfishGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASpearfishGameState>() : nullptr;
	return GameState ? GameState->GetServerTime() : 0.f;
}

// ------------------------------------------------------------------------------------ Queries

const FSpearfishOrder* USpearfishRestaurantComponent::FindOrder(int32 OrderId) const
{
	return Orders.FindByPredicate([OrderId](const FSpearfishOrder& Order) { return Order.OrderId == OrderId; });
}

const FSpearfishDish* USpearfishRestaurantComponent::FindDish(int32 DishId) const
{
	return Dishes.FindByPredicate([DishId](const FSpearfishDish& Dish) { return Dish.DishId == DishId; });
}

FSpearfishOrder* USpearfishRestaurantComponent::FindOrderMutable(int32 OrderId)
{
	return Orders.FindByPredicate([OrderId](const FSpearfishOrder& Order) { return Order.OrderId == OrderId; });
}

FSpearfishDish* USpearfishRestaurantComponent::FindDishMutable(int32 DishId)
{
	return Dishes.FindByPredicate([DishId](const FSpearfishDish& Dish) { return Dish.DishId == DishId; });
}

int32 USpearfishRestaurantComponent::GetSeatCount() const
{
	const ASpearfishGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASpearfishGameState>() : nullptr;
	const int32 Extra = GameState && GameState->GetProgression() ? GameState->GetProgression()->GetExtraSeats() : 0;
	const int32 MaxSeats = GetBoat() ? GetBoat()->GetMaxSeats() : 8;
	return FMath::Clamp(USpearfishSettings::Get()->BaseSeats + Extra, 1, MaxSeats);
}

int32 USpearfishRestaurantComponent::CountActiveGuests() const
{
	int32 Count = 0;
	for (const ASpearfishCustomer* Guest : Guests)
	{
		Count += Guest ? 1 : 0;
	}
	return Count;
}

float USpearfishRestaurantComponent::GetPatienceRemaining(const FSpearfishOrder& Order) const
{
	return FMath::Max(0.f, Order.PatienceDeadline - ServerNow());
}

float USpearfishRestaurantComponent::GetPatienceFraction(const FSpearfishOrder& Order) const
{
	return SpearfishOrders::PatienceFraction(GetPatienceRemaining(Order), Order.PatienceTotal);
}

TArray<int32> USpearfishRestaurantComponent::GetMissingCounts(const FSpearfishOrder& Order) const
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FSpearfishRecipeDef* Recipe = Registry ? Registry->FindRecipe(Order.RecipeId) : nullptr;
	const ASpearfishBoat* Boat = GetBoat();
	if (!Recipe || !Boat || !Boat->GetCooler())
	{
		return TArray<int32>();
	}
	return SpearfishOrders::MissingCounts(Boat->GetCooler()->GetItems(), Recipe->Ingredients);
}

bool USpearfishRestaurantComponent::CanStartOrder(const FSpearfishOrder& Order) const
{
	if (Order.State != ESpearfishOrderState::Waiting || Order.DishId != INDEX_NONE)
	{
		return false;
	}
	for (const int32 Missing : GetMissingCounts(Order))
	{
		if (Missing > 0)
		{
			return false;
		}
	}
	return true;
}

FText USpearfishRestaurantComponent::DescribeStationWork(const ASpearfishCharacter* Character, ESpearfishStationType Station) const
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	if (!Registry)
	{
		return FText::GetEmpty();
	}
	for (const FSpearfishDish& Dish : Dishes)
	{
		const FSpearfishRecipeDef* Recipe = Registry->FindRecipe(Dish.RecipeId);
		if (!Recipe || Dish.State != ESpearfishDishState::Prep || Dish.bAutoChef || !Recipe->Steps.IsValidIndex(Dish.CurrentStep))
		{
			continue;
		}
		const ESpearfishCookStep Step = Recipe->Steps[Dish.CurrentStep];
		if (SpearfishStations::SupportsStep(Station, Step))
		{
			return FText::Format(LOCTEXT("StationWork", "{0}: {1} (step {2}/{3})"), SpearfishText::StepName(Step), Recipe->DisplayName,
				FText::AsNumber(Dish.CurrentStep + 1), FText::AsNumber(Recipe->Steps.Num()));
		}
	}
	return FText::Format(LOCTEXT("StationIdle", "{0}: nothing to do (start a dish on the tablet)"), SpearfishText::StationName(Station));
}

FText USpearfishRestaurantComponent::DescribeServe() const
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	for (const FSpearfishDish& Dish : Dishes)
	{
		if (Dish.State == ESpearfishDishState::Ready)
		{
			const FSpearfishRecipeDef* Recipe = Registry ? Registry->FindRecipe(Dish.RecipeId) : nullptr;
			return FText::Format(LOCTEXT("ServePrompt", "Serve {0}"), Recipe ? Recipe->DisplayName : FText::FromName(Dish.RecipeId));
		}
	}
	return LOCTEXT("NothingToServe", "Service pass (no dish ready)");
}

// ------------------------------------------------------------------------------------- Server

void USpearfishRestaurantComponent::SetOpen(bool bInOpen)
{
	if (bOpen == bInOpen)
	{
		return;
	}
	bOpen = bInOpen;
	NextArrivalTime = FMath::Min(NextArrivalTime, ServerNow() + 8.f);
	if (ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>())
	{
		GameState->BroadcastNotice(bOpen ? LOCTEXT("Opened", "The restaurant is OPEN.") : LOCTEXT("Closed", "The restaurant is CLOSED."), ESpearfishNoticeType::Info);
	}
}

void USpearfishRestaurantComponent::BeginDay(int32 Day, int32 Seed)
{
	Stream.Initialize(Seed);
	for (ASpearfishCustomer* Guest : Guests)
	{
		if (Guest)
		{
			Guest->Destroy();
		}
	}
	Guests.Reset();
	Orders.Reset();
	// Unfinished dishes go back to the cooler overnight.
	if (ASpearfishBoat* Boat = GetBoat())
	{
		for (const FSpearfishDish& Dish : Dishes)
		{
			Boat->GetCooler()->Deposit(Dish.Ingredients);
		}
	}
	Dishes.Reset();
	KitchenNeeds.Reset();
	bOpen = false;
	bCriticPending = false;
	NextArrivalTime = ServerNow() + 10.f;

	if (const ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>())
	{
		const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
		for (const FName& EventId : GameState->GetActiveEvents())
		{
			const FSpearfishEventDef* Event = Registry ? Registry->FindEvent(EventId) : nullptr;
			bCriticPending |= Event && Event->Type == ESpearfishEventType::CriticVisit;
		}
	}
}

void USpearfishRestaurantComponent::CloseForNight()
{
	bOpen = false;
	for (FSpearfishOrder& Order : Orders)
	{
		if (Order.State == ESpearfishOrderState::Waiting || Order.State == ESpearfishOrderState::Cooking || Order.State == ESpearfishOrderState::Ready)
		{
			ExpireOrder(Order, 0.5f);
		}
	}
	for (ASpearfishCustomer* Guest : Guests)
	{
		if (Guest)
		{
			Guest->Leave(false);
		}
	}
}

void USpearfishRestaurantComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	Guests.RemoveAll([](const TObjectPtr<ASpearfishCustomer>& Guest) { return !IsValid(Guest); });
	TickArrivals(DeltaTime);
	TickPatience();

	// Locks expire so a disconnected or distracted cook cannot block a dish forever.
	const float Now = ServerNow();
	for (FSpearfishDish& Dish : Dishes)
	{
		if (Dish.LockedBy && Now > Dish.LockExpires)
		{
			Dish.LockedBy = nullptr;
		}
	}

	NeedsTimer += DeltaTime;
	if (NeedsTimer >= 1.f)
	{
		NeedsTimer = 0.f;
		UpdateKitchenNeeds();
	}
}

void USpearfishRestaurantComponent::TickArrivals(float DeltaTime)
{
	const ASpearfishBoat* Boat = GetBoat();
	const ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	const USpearfishDayCycleComponent* DayCycle = GameState ? GameState->GetDayCycle() : nullptr;
	if (!bOpen || !Boat || !DayCycle || !Boat->IsAnchored() || !DayCycle->AcceptsNewGuests())
	{
		return;
	}
	const float Now = ServerNow();
	if (Now < NextArrivalTime || FindFreeSeat() == INDEX_NONE)
	{
		return;
	}
	SpawnGuest();
	const USpearfishSettings* Settings = USpearfishSettings::Get();
	NextArrivalTime = Now + Stream.FRandRange(Settings->MinGuestArrivalSeconds, Settings->MaxGuestArrivalSeconds);
}

int32 USpearfishRestaurantComponent::FindFreeSeat() const
{
	const int32 SeatCount = GetSeatCount();
	for (int32 Seat = 0; Seat < SeatCount; ++Seat)
	{
		const bool bTaken = Guests.ContainsByPredicate([Seat](const TObjectPtr<ASpearfishCustomer>& Guest) { return Guest && Guest->GetSeatIndex() == Seat; });
		if (!bTaken)
		{
			return Seat;
		}
	}
	return INDEX_NONE;
}

const FSpearfishCustomerDef* USpearfishRestaurantComponent::PickCustomer()
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	if (!Registry || !GameState)
	{
		return nullptr;
	}
	const float Reputation = GameState->GetProgression()->GetReputation();

	TArray<const FSpearfishCustomerDef*> Candidates;
	float TotalWeight = 0.f;
	for (const TPair<FName, FSpearfishCustomerDef>& Pair : Registry->GetAllCustomers())
	{
		if (bCriticPending && Pair.Value.ReputationWeight >= 3.f)
		{
			// The critic event guarantees the critic's visit today.
			bCriticPending = false;
			return &Pair.Value;
		}
		if (Pair.Value.MinReputation <= Reputation && Pair.Value.SpawnWeight > 0.f)
		{
			Candidates.Add(&Pair.Value);
			TotalWeight += Pair.Value.SpawnWeight;
		}
	}
	float Roll = Stream.FRandRange(0.f, TotalWeight);
	for (const FSpearfishCustomerDef* Candidate : Candidates)
	{
		Roll -= Candidate->SpawnWeight;
		if (Roll <= 0.f)
		{
			return Candidate;
		}
	}
	return Candidates.Num() > 0 ? Candidates.Last() : nullptr;
}

bool USpearfishRestaurantComponent::IsRecipeAvailable(const FSpearfishRecipeDef& Recipe) const
{
	const ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	const USpearfishProgressionComponent* Progression = GameState ? GameState->GetProgression() : nullptr;
	if (!Progression || Progression->GetReputation() + UE_KINDA_SMALL_NUMBER < Recipe.MinReputation)
	{
		return false;
	}
	if (!Recipe.RequiredUpgrade.IsNone() && !Progression->HasUpgrade(Recipe.RequiredUpgrade))
	{
		return false;
	}

	// Only order what this region can actually provide (species listed in its spawns or active events).
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	const FSpearfishRegionDef* Region = Ocean ? Ocean->GetRegionDef() : nullptr;
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	if (!Region || !Registry)
	{
		return true;
	}
	for (const FSpearfishIngredientReq& Req : Recipe.Ingredients)
	{
		bool bAvailable = false;
		for (const FSpearfishRegionSpawn& Spawn : Region->Spawns)
		{
			const FSpearfishFishSpeciesDef* Species = Registry->FindFish(Spawn.SpeciesId);
			if (!Species || Spawn.Groups <= 0 || !Species->bCatchable)
			{
				continue;
			}
			if ((!Req.SpeciesId.IsNone() && Species->Id == Req.SpeciesId)
				|| (Req.SpeciesId.IsNone() && (Req.Category.IsNone() || Species->Category == Req.Category)))
			{
				bAvailable = Species->MaxLengthCm >= Req.MinLengthCm;
				if (bAvailable)
				{
					break;
				}
			}
		}
		if (!bAvailable && !Req.SpeciesId.IsNone())
		{
			// Event visitors (e.g. a legendary fish) put their dish on the menu for the day.
			for (const FName& EventId : GameState->GetActiveEvents())
			{
				const FSpearfishEventDef* Event = Registry->FindEvent(EventId);
				bAvailable |= Event && Event->SpeciesId == Req.SpeciesId;
			}
		}
		if (!bAvailable)
		{
			return false;
		}
	}
	return true;
}

FName USpearfishRestaurantComponent::PickRecipe(const FSpearfishCustomerDef& Customer)
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	if (!Registry)
	{
		return NAME_None;
	}
	TArray<const FSpearfishRecipeDef*> Candidates;
	TArray<float> Weights;
	float Total = 0.f;
	for (const TPair<FName, FSpearfishRecipeDef>& Pair : Registry->GetAllRecipes())
	{
		if (!IsRecipeAvailable(Pair.Value))
		{
			continue;
		}
		float Weight = 1.f;
		for (const FName& Tag : Pair.Value.Tags)
		{
			Weight += Customer.FavoriteTags.Contains(Tag) ? 1.5f : 0.f;
		}
		// Legendary feasts are rare requests.
		if (Pair.Value.BasePrice > 300)
		{
			Weight *= 0.3f;
		}
		Candidates.Add(&Pair.Value);
		Weights.Add(Weight);
		Total += Weight;
	}
	float Roll = Stream.FRandRange(0.f, Total);
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		Roll -= Weights[Index];
		if (Roll <= 0.f)
		{
			return Candidates[Index]->Id;
		}
	}
	return Candidates.Num() > 0 ? Candidates.Last()->Id : NAME_None;
}

void USpearfishRestaurantComponent::SpawnGuest()
{
	ASpearfishBoat* Boat = GetBoat();
	const FSpearfishCustomerDef* Customer = PickCustomer();
	const int32 Seat = FindFreeSeat();
	if (!Boat || !Customer || Seat == INDEX_NONE)
	{
		return;
	}
	const FTransform Spawn(Boat->GetActorRotation(), Boat->GetBoardingPoint());
	ASpearfishCustomer* Guest = GetWorld()->SpawnActorDeferred<ASpearfishCustomer>(ASpearfishCustomer::StaticClass(), Spawn, Boat, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Guest)
	{
		return;
	}
	Guest->InitGuest(Customer->Id, Seat, NextGuestId++, Boat, Stream.RandRange(1, 1 << 20));
	Guest->FinishSpawning(Spawn);
	Guests.Add(Guest);

	if (ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>())
	{
		GameState->BroadcastNotice(FText::Format(LOCTEXT("GuestArrived", "A water taxi drops off a guest: {0}."), Customer->DisplayName), ESpearfishNoticeType::Info);
	}
}

void USpearfishRestaurantComponent::OnGuestSeated(ASpearfishCustomer* Guest)
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	const FSpearfishCustomerDef* Customer = (Registry && Guest) ? Registry->FindCustomer(Guest->GetCustomerId()) : nullptr;
	if (!Customer || !GameState)
	{
		return;
	}

	const float PatienceBonus = GameState->GetProgression()->GetPatienceBonus();
	const float Patience = Customer->PatienceSeconds * (1.f + PatienceBonus);
	for (int32 Dish = 0; Dish < FMath::Max(Customer->Dishes, 1); ++Dish)
	{
		const FName RecipeId = PickRecipe(*Customer);
		if (RecipeId.IsNone())
		{
			continue;
		}
		FSpearfishOrder Order;
		Order.OrderId = NextOrderId++;
		Order.GuestId = Guest->GetGuestId();
		Order.SeatIndex = Guest->GetSeatIndex();
		Order.RecipeId = RecipeId;
		Order.CustomerId = Customer->Id;
		Order.PatienceTotal = Patience;
		Order.PatienceDeadline = ServerNow() + Patience;
		Orders.Add(Order);
		OnOrderPlaced.Broadcast(Order);

		const FSpearfishRecipeDef* Recipe = Registry->FindRecipe(RecipeId);
		GameState->BroadcastNotice(FText::Format(LOCTEXT("NewOrder", "Table {0} orders {1}."), FText::AsNumber(Order.SeatIndex / 2 + 1),
			Recipe ? Recipe->DisplayName : FText::FromName(RecipeId)), ESpearfishNoticeType::Info);
	}

	if (Customer->bShareRumors)
	{
		for (const FName& EventId : GameState->GetActiveEvents())
		{
			if (!GameState->GetRevealedRumors().Contains(EventId))
			{
				GameState->RevealRumor(EventId);
				GameState->BroadcastNotice(LOCTEXT("Rumor", "A local is gossiping at the bar... the chef's tablet has a new rumor."), ESpearfishNoticeType::Discovery);
				break;
			}
		}
	}
	if (Orders.ContainsByPredicate([Guest](const FSpearfishOrder& Order) { return Order.GuestId == Guest->GetGuestId(); }) == false)
	{
		Guest->Leave(false);
	}
}

void USpearfishRestaurantComponent::OnGuestLeft(ASpearfishCustomer* Guest)
{
	Guests.Remove(Guest);
}

void USpearfishRestaurantComponent::TickPatience()
{
	const float Now = ServerNow();
	for (FSpearfishOrder& Order : Orders)
	{
		const bool bActive = Order.State == ESpearfishOrderState::Waiting || Order.State == ESpearfishOrderState::Cooking || Order.State == ESpearfishOrderState::Ready;
		if (bActive && Now > Order.PatienceDeadline)
		{
			ExpireOrder(Order, 1.f);
		}
	}
	// Forget resolved orders after a while to keep replication small.
	Orders.RemoveAll([Now](const FSpearfishOrder& Order)
	{
		return (Order.State == ESpearfishOrderState::Served || Order.State == ESpearfishOrderState::Expired) && Now - Order.PatienceDeadline > 30.f;
	});
}

void USpearfishRestaurantComponent::ExpireOrder(FSpearfishOrder& Order, float PenaltyScale)
{
	Order.State = ESpearfishOrderState::Expired;
	if (FSpearfishDish* Dish = FindDishMutable(Order.DishId))
	{
		Dish->OrderId = INDEX_NONE;
	}
	Order.DishId = INDEX_NONE;
	Order.PatienceDeadline = FMath::Min(Order.PatienceDeadline, ServerNow());

	ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FSpearfishCustomerDef* Customer = Registry ? Registry->FindCustomer(Order.CustomerId) : nullptr;
	if (GameState)
	{
		const float Delta = SpearfishOrders::WalkoutReputationDelta(Customer ? Customer->ReputationWeight : 1.f, USpearfishSettings::Get()->Service) * PenaltyScale;
		GameState->GetProgression()->AddReputation(Delta);
		++GameState->EditTodayStats().GuestsLost;
		GameState->BroadcastNotice(FText::Format(LOCTEXT("Walkout", "Table {0} gave up waiting and left!"), FText::AsNumber(Order.SeatIndex / 2 + 1)), ESpearfishNoticeType::Danger);
	}
	ReleaseGuestIfDone(Order.GuestId, false);
}

void USpearfishRestaurantComponent::ReleaseGuestIfDone(int32 GuestId, bool bHappy)
{
	const bool bPending = Orders.ContainsByPredicate([GuestId](const FSpearfishOrder& Order)
	{
		return Order.GuestId == GuestId && (Order.State == ESpearfishOrderState::Waiting || Order.State == ESpearfishOrderState::Cooking || Order.State == ESpearfishOrderState::Ready);
	});
	if (bPending)
	{
		return;
	}
	for (ASpearfishCustomer* Guest : Guests)
	{
		if (Guest && Guest->GetGuestId() == GuestId)
		{
			Guest->Leave(bHappy);
		}
	}
}

void USpearfishRestaurantComponent::UpdateKitchenNeeds()
{
	KitchenNeeds.Reset();
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	if (!Registry)
	{
		return;
	}
	for (const FSpearfishOrder& Order : Orders)
	{
		if (Order.State != ESpearfishOrderState::Waiting || Order.DishId != INDEX_NONE)
		{
			continue;
		}
		const FSpearfishRecipeDef* Recipe = Registry->FindRecipe(Order.RecipeId);
		const TArray<int32> Missing = GetMissingCounts(Order);
		for (int32 Index = 0; Recipe && Index < Missing.Num() && Index < Recipe->Ingredients.Num(); ++Index)
		{
			if (Missing[Index] <= 0)
			{
				continue;
			}
			const FSpearfishIngredientReq& Req = Recipe->Ingredients[Index];
			FSpearfishKitchenNeed* Existing = KitchenNeeds.FindByPredicate([&Req](const FSpearfishKitchenNeed& Need)
			{
				return Need.SpeciesId == Req.SpeciesId && Need.Category == Req.Category;
			});
			if (!Existing)
			{
				Existing = &KitchenNeeds.AddDefaulted_GetRef();
				Existing->SpeciesId = Req.SpeciesId;
				Existing->Category = Req.Category;
			}
			Existing->Count += Missing[Index];
			Existing->MinLengthCm = FMath::Max(Existing->MinLengthCm, Req.MinLengthCm);
		}
	}
}

bool USpearfishRestaurantComponent::TryStartDish(int32 OrderId, APlayerState* By, bool bAuto, FText& OutReason)
{
	FSpearfishOrder* Order = FindOrderMutable(OrderId);
	ASpearfishBoat* Boat = GetBoat();
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FSpearfishRecipeDef* Recipe = (Order && Registry) ? Registry->FindRecipe(Order->RecipeId) : nullptr;
	if (!Order || !Boat || !Recipe || Order->State != ESpearfishOrderState::Waiting || Order->DishId != INDEX_NONE)
	{
		OutReason = LOCTEXT("CannotStart", "That order can't be started.");
		return false;
	}
	const int32 Active = Dishes.FilterByPredicate([](const FSpearfishDish& Dish) { return Dish.State == ESpearfishDishState::Prep; }).Num();
	if (Active >= USpearfishSettings::Get()->MaxActiveDishes)
	{
		OutReason = LOCTEXT("TooManyDishes", "The kitchen is full - finish a dish first.");
		return false;
	}

	TArray<int32> Picked;
	if (!SpearfishOrders::FindIngredients(Boat->GetCooler()->GetItems(), Recipe->Ingredients, Picked))
	{
		OutReason = LOCTEXT("MissingFish", "Not enough of the right fish in the cooler.");
		return false;
	}

	FSpearfishDish Dish;
	Dish.DishId = NextDishId++;
	Dish.OrderId = OrderId;
	Dish.RecipeId = Order->RecipeId;
	Dish.bAutoChef = bAuto;
	Dish.Ingredients = Boat->GetCooler()->Withdraw(Picked);
	Dish.IngredientQuality = SpearfishOrders::AverageQuality(Dish.Ingredients);
	if (bAuto && Recipe->Steps.Num() > 0)
	{
		Dish.AutoStepRemaining = SpearfishMinigame::AutoStepDuration(Recipe->Steps[0]);
	}
	Dishes.Add(Dish);
	Order->State = ESpearfishOrderState::Cooking;
	Order->DishId = Dish.DishId;
	return true;
}

bool USpearfishRestaurantComponent::CancelDish(int32 DishId)
{
	const int32 Index = Dishes.IndexOfByPredicate([DishId](const FSpearfishDish& Dish) { return Dish.DishId == DishId; });
	if (Index == INDEX_NONE || !GetBoat())
	{
		return false;
	}
	GetBoat()->GetCooler()->Deposit(Dishes[Index].Ingredients);
	if (FSpearfishOrder* Order = FindOrderMutable(Dishes[Index].OrderId))
	{
		if (Order->State == ESpearfishOrderState::Cooking || Order->State == ESpearfishOrderState::Ready)
		{
			Order->State = ESpearfishOrderState::Waiting;
		}
		Order->DishId = INDEX_NONE;
	}
	Dishes.RemoveAt(Index);
	return true;
}

bool USpearfishRestaurantComponent::TryBeginCookStep(ASpearfishCharacter* Cook, ASpearfishStation* Station)
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	APlayerState* CookState = Cook ? Cook->GetPlayerState() : nullptr;
	ASpearfishPlayerController* Controller = Cook ? Cast<ASpearfishPlayerController>(Cook->GetController()) : nullptr;
	if (!Registry || !CookState || !Controller || !Station)
	{
		return false;
	}

	const float Now = ServerNow();
	FSpearfishDish* Best = nullptr;
	for (FSpearfishDish& Dish : Dishes)
	{
		const FSpearfishRecipeDef* Recipe = Registry->FindRecipe(Dish.RecipeId);
		if (!Recipe || Dish.State != ESpearfishDishState::Prep || Dish.bAutoChef || !Recipe->Steps.IsValidIndex(Dish.CurrentStep))
		{
			continue;
		}
		if (!SpearfishStations::SupportsStep(Station->GetStationType(), Recipe->Steps[Dish.CurrentStep]))
		{
			continue;
		}
		if (Dish.LockedBy && Dish.LockedBy != CookState && Now <= Dish.LockExpires)
		{
			continue;
		}
		if (!Best || Dish.LockedBy == CookState || Dish.DishId < Best->DishId)
		{
			Best = &Dish;
		}
	}
	if (!Best)
	{
		Cook->NotifyOwner(LOCTEXT("NoWork", "Nothing to do here. Start a dish from the tablet (Tab)."), ESpearfishNoticeType::Info);
		return false;
	}

	const FSpearfishRecipeDef* Recipe = Registry->FindRecipe(Best->RecipeId);
	Best->LockedBy = CookState;
	Best->LockExpires = Now + SpearfishRestaurantPrivate::LockSeconds;
	const ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	const float ZoneBonus = GameState ? GameState->GetProgression()->GetCookZoneBonus() : 0.f;
	Controller->ClientBeginMinigame(Best->DishId, Best->CurrentStep, Recipe->Steps[Best->CurrentStep], Recipe->Difficulty, Stream.RandRange(1, 1 << 24), ZoneBonus);
	return true;
}

bool USpearfishRestaurantComponent::SubmitCookStep(int32 DishId, int32 StepIndex, float Score, APlayerState* By, bool bAuto)
{
	FSpearfishDish* Dish = FindDishMutable(DishId);
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FSpearfishRecipeDef* Recipe = (Dish && Registry) ? Registry->FindRecipe(Dish->RecipeId) : nullptr;
	if (!Dish || !Recipe || Dish->State != ESpearfishDishState::Prep || Dish->CurrentStep != StepIndex)
	{
		return false;
	}
	if (!bAuto && Dish->LockedBy && Dish->LockedBy != By && ServerNow() <= Dish->LockExpires)
	{
		return false;
	}

	Dish->StepScores.Add(FMath::Clamp(Score, 0.f, 1.f));
	++Dish->CurrentStep;
	Dish->LockedBy = nullptr;
	if (ASpearfishPlayerState* Cook = Cast<ASpearfishPlayerState>(By))
	{
		Cook->AddDishCooked();
	}

	if (Dish->CurrentStep >= Recipe->Steps.Num())
	{
		Dish->State = ESpearfishDishState::Ready;
		if (FSpearfishOrder* Order = FindOrderMutable(Dish->OrderId))
		{
			Order->State = ESpearfishOrderState::Ready;
		}
		if (!bAuto)
		{
			if (ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>())
			{
				GameState->BroadcastNotice(FText::Format(LOCTEXT("DishReady", "{0} is ready at the pass!"), Recipe->DisplayName), ESpearfishNoticeType::Good);
			}
		}
	}
	else if (bAuto)
	{
		Dish->AutoStepRemaining = SpearfishMinigame::AutoStepDuration(Recipe->Steps[Dish->CurrentStep]);
	}
	return true;
}

void USpearfishRestaurantComponent::AbortCookStep(int32 DishId, APlayerState* By)
{
	if (FSpearfishDish* Dish = FindDishMutable(DishId))
	{
		if (Dish->LockedBy == By)
		{
			Dish->LockedBy = nullptr;
		}
	}
}

void USpearfishRestaurantComponent::SetAutoStepRemaining(int32 DishId, float Seconds)
{
	if (FSpearfishDish* Dish = FindDishMutable(DishId))
	{
		Dish->AutoStepRemaining = Seconds;
	}
}

void USpearfishRestaurantComponent::TickAutoStep(int32 DishId, float DeltaSeconds, float Skill)
{
	FSpearfishDish* Dish = FindDishMutable(DishId);
	if (!Dish || !Dish->bAutoChef || Dish->State != ESpearfishDishState::Prep)
	{
		return;
	}
	Dish->AutoStepRemaining -= DeltaSeconds;
	if (Dish->AutoStepRemaining <= 0.f)
	{
		SubmitCookStep(DishId, Dish->CurrentStep, SpearfishCooking::AutoChefStepScore(Skill, Stream), nullptr, true);
	}
}

bool USpearfishRestaurantComponent::TryServe(ASpearfishCharacter* Server)
{
	bool bAnyReady = false;
	TArray<int32> ReadyIds;
	for (const FSpearfishDish& Dish : Dishes)
	{
		if (Dish.State == ESpearfishDishState::Ready)
		{
			ReadyIds.Add(Dish.DishId);
		}
	}
	for (const int32 DishId : ReadyIds)
	{
		bAnyReady = true;
		if (ServeDish(DishId, false))
		{
			return true;
		}
	}
	if (Server)
	{
		Server->NotifyOwner(bAnyReady
			? LOCTEXT("NobodyWaiting", "Nobody is waiting for that dish anymore - it keeps until someone orders it.")
			: LOCTEXT("NoDishReady", "No dish is ready to serve."), ESpearfishNoticeType::Info);
	}
	return false;
}

bool USpearfishRestaurantComponent::ServeDish(int32 DishId, bool bAuto)
{
	const int32 DishIndex = Dishes.IndexOfByPredicate([DishId](const FSpearfishDish& Dish) { return Dish.DishId == DishId; });
	if (DishIndex == INDEX_NONE || Dishes[DishIndex].State != ESpearfishDishState::Ready)
	{
		return false;
	}
	const FSpearfishDish Dish = Dishes[DishIndex];

	// The original guest, or anyone else still waiting for this recipe.
	FSpearfishOrder* Order = FindOrderMutable(Dish.OrderId);
	if (!Order || Order->State == ESpearfishOrderState::Served || Order->State == ESpearfishOrderState::Expired)
	{
		Order = nullptr;
		for (FSpearfishOrder& Candidate : Orders)
		{
			if (Candidate.RecipeId == Dish.RecipeId && Candidate.State == ESpearfishOrderState::Waiting && Candidate.DishId == INDEX_NONE)
			{
				Order = &Candidate;
				break;
			}
		}
	}
	if (!Order)
	{
		return false;
	}

	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	const FSpearfishRecipeDef* Recipe = Registry ? Registry->FindRecipe(Dish.RecipeId) : nullptr;
	const FSpearfishCustomerDef* Customer = Registry ? Registry->FindCustomer(Order->CustomerId) : nullptr;
	if (!GameState || !Recipe)
	{
		return false;
	}

	const float QualityBonus = GameState->GetProgression()->GetDishQualityBonus();
	const float Quality = SpearfishCooking::DishQuality(Dish.StepScores, Dish.IngredientQuality, QualityBonus);
	const FSpearfishPayout Payout = SpearfishOrders::ComputePayout(Recipe->BasePrice, Quality, GetPatienceFraction(*Order),
		Customer ? Customer->QualityExpectation : 0.5f, Customer ? Customer->TipMultiplier : 1.f, Customer ? Customer->ReputationWeight : 1.f,
		USpearfishSettings::Get()->Service);

	// Commit atomically: dish consumed, order closed, then rewards.
	Dishes.RemoveAt(DishIndex);
	Order->State = ESpearfishOrderState::Served;
	Order->DishId = INDEX_NONE;
	Order->PatienceDeadline = ServerNow();

	GameState->GetProgression()->AddMoney(Payout.Total);
	GameState->GetProgression()->AddReputation(Payout.ReputationDelta);
	FSpearfishDayStats& Stats = GameState->EditTodayStats();
	Stats.Revenue += Payout.Total;
	Stats.Tips += Payout.Tip;
	++Stats.DishesServed;
	Stats.BestDishQuality = FMath::Max(Stats.BestDishQuality, Quality);
	Stats.ReputationEnd = GameState->GetProgression()->GetReputation();

	GameState->BroadcastNotice(FText::Format(LOCTEXT("Served", "Table {0}: {1} ({2}) +{3}{4}"),
		FText::AsNumber(Order->SeatIndex / 2 + 1), Recipe->DisplayName, SpearfishText::QualityName(Quality),
		FText::FromString(SpearfishText::Money(Payout.Total)),
		Payout.Tip > 0 ? FText::Format(LOCTEXT("Tip", " (tip {0})"), FText::AsNumber(Payout.Tip)) : FText::GetEmpty()), ESpearfishNoticeType::Money);

	ReleaseGuestIfDone(Order->GuestId, Payout.Satisfaction >= 0.5f);
	return true;
}

#undef LOCTEXT_NAMESPACE
