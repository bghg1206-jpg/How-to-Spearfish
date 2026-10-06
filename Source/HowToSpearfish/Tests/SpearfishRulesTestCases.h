#pragma once

// Rules-layer test cases shared by two runners:
//  - UE automation: Tests/SpearfishRulesAutomationTests.cpp ("Spearfish.Rules" in Session Frontend)
//  - native:        Tools/NativeCheck/run_rules_tests.sh (no engine needed)
// Each case covers gameplay edge cases called out in the design: role swaps, inventory loss,
// duplicate rewards, oxygen depletion, order completion, cooking results and day transitions.

#include "CoreMinimal.h"
#include "Rules/CookingRules.h"
#include "Rules/DayRules.h"
#include "Rules/EconomyRules.h"
#include "Rules/EventRules.h"
#include "Rules/FishRules.h"
#include "Rules/InventoryRules.h"
#include "Rules/LineRules.h"
#include "Rules/OrderRules.h"
#include "Rules/OxygenRules.h"
#include "Rules/RoleRules.h"
#include "World/SpearfishTerrain.h"

class FSpearfishRulesTestContext
{
public:
	virtual ~FSpearfishRulesTestContext() = default;
	virtual void Check(bool bCondition, const TCHAR* Expression, const char* File, int32 Line) = 0;
};

#define SF_CHECK(Ctx, Expr) (Ctx).Check(!!(Expr), TEXT(#Expr), __FILE__, __LINE__)
#define SF_CHECK_NEAR(Ctx, A, B, Tol) (Ctx).Check(FMath::Abs((A) - (B)) <= (Tol), TEXT(#A " ~= " #B), __FILE__, __LINE__)

namespace SpearfishRulesTests
{
	inline FSpearfishItem MakeFish(int32 Id, const TCHAR* Species, const TCHAR* Category, float Length, float Weight, int32 Value, float Quality = 0.8f)
	{
		FSpearfishItem Item;
		Item.InstanceId = Id;
		Item.Kind = ESpearfishItemKind::Fish;
		Item.DefId = FName(Species);
		Item.Category = FName(Category);
		Item.LengthCm = Length;
		Item.WeightKg = Weight;
		Item.Value = Value;
		Item.Quality = Quality;
		Item.SlotCost = SpearfishInventory::SlotCostForLength(Length);
		return Item;
	}

	inline FSpearfishItem MakeLoot(int32 Id, const TCHAR* LootId, float Weight, int32 Value)
	{
		FSpearfishItem Item;
		Item.InstanceId = Id;
		Item.Kind = ESpearfishItemKind::Loot;
		Item.DefId = FName(LootId);
		Item.WeightKg = Weight;
		Item.Value = Value;
		Item.SlotCost = 1;
		return Item;
	}

	inline FSpearfishIngredientReq MakeReq(const TCHAR* Species, const TCHAR* Category, int32 Count, float MinLength = 0.f)
	{
		FSpearfishIngredientReq Req;
		Req.SpeciesId = Species ? FName(Species) : FName();
		Req.Category = Category ? FName(Category) : FName();
		Req.Count = Count;
		Req.MinLengthCm = MinLength;
		return Req;
	}

	// ------------------------------------------------------------------------------------- Oxygen
	inline void OxygenConsumptionScalesWithDepthAndEffort(FSpearfishRulesTestContext& T)
	{
		const FSpearfishOxygenTuning Tuning;
		FSpearfishBreathInput Surface;
		Surface.DepthMeters = 0.2f;
		SF_CHECK(T, SpearfishOxygen::ConsumptionPerSecond(Surface, Tuning) == 0.f);

		FSpearfishBreathInput Shallow;
		Shallow.DepthMeters = 5.f;
		FSpearfishBreathInput Deep = Shallow;
		Deep.DepthMeters = 25.f;
		const float ShallowRate = SpearfishOxygen::ConsumptionPerSecond(Shallow, Tuning);
		const float DeepRate = SpearfishOxygen::ConsumptionPerSecond(Deep, Tuning);
		SF_CHECK(T, ShallowRate > 0.f);
		SF_CHECK(T, DeepRate > ShallowRate);

		FSpearfishBreathInput Sprint = Shallow;
		Sprint.bSprinting = true;
		SF_CHECK(T, SpearfishOxygen::ConsumptionPerSecond(Sprint, Tuning) > ShallowRate);

		FSpearfishBreathInput OverDepth = Deep;
		OverDepth.SafeDepthMeters = 15.f;
		FSpearfishBreathInput Rated = Deep;
		Rated.SafeDepthMeters = 40.f;
		SF_CHECK(T, SpearfishOxygen::ConsumptionPerSecond(OverDepth, Tuning) > SpearfishOxygen::ConsumptionPerSecond(Rated, Tuning));

		FSpearfishBreathInput Stung = Shallow;
		Stung.bStung = true;
		SF_CHECK_NEAR(T, SpearfishOxygen::ConsumptionPerSecond(Stung, Tuning), ShallowRate * Tuning.StungMultiplier, 0.001f);
	}

	inline void OxygenStarterTankIsForgivingButLimited(FSpearfishRulesTestContext& T)
	{
		// A starter tank (300) at 10 m cruising should last roughly 2.5-4 minutes.
		const FSpearfishOxygenTuning Tuning;
		FSpearfishBreathInput Cruise;
		Cruise.DepthMeters = 10.f;
		Cruise.SpeedFraction = 0.6f;
		const float Rate = SpearfishOxygen::ConsumptionPerSecond(Cruise, Tuning);
		const float Seconds = 300.f / Rate;
		SF_CHECK(T, Seconds > 150.f);
		SF_CHECK(T, Seconds < 260.f);
	}

	inline void OxygenBreathHoldThenBlackout(FSpearfishRulesTestContext& T)
	{
		const FSpearfishOxygenTuning Tuning;
		FSpearfishBreathState State;
		SpearfishOxygen::Refill(State, 10.f, Tuning);

		FSpearfishBreathInput Deep;
		Deep.DepthMeters = 12.f;

		bool bBlackout = false;
		float Time = 0.f;
		while (!bBlackout && Time < 120.f)
		{
			bBlackout = SpearfishOxygen::Step(State, Deep, Tuning, 0.1f);
			Time += 0.1f;
		}
		SF_CHECK(T, bBlackout);
		SF_CHECK(T, State.bBlackedOut);
		SF_CHECK(T, State.bOutOfAir);
		// Tank empties in < 10 s at this rate; breath hold adds BreathHoldSeconds on top.
		SF_CHECK(T, Time >= Tuning.BreathHoldSeconds);
		// Blackout is reported exactly once.
		SF_CHECK(T, !SpearfishOxygen::Step(State, Deep, Tuning, 0.1f));
	}

	inline void OxygenSurfacingDuringBreathHoldSaves(FSpearfishRulesTestContext& T)
	{
		const FSpearfishOxygenTuning Tuning;
		FSpearfishBreathState State;
		SpearfishOxygen::Refill(State, 2.f, Tuning);
		FSpearfishBreathInput Deep;
		Deep.DepthMeters = 8.f;
		for (int32 Step = 0; Step < 40; ++Step)
		{
			SpearfishOxygen::Step(State, Deep, Tuning, 0.1f);
		}
		SF_CHECK(T, State.bOutOfAir);
		SF_CHECK(T, !State.bBlackedOut);
		SF_CHECK(T, State.BreathHoldRemaining < Tuning.BreathHoldSeconds);

		FSpearfishBreathInput Surface;
		Surface.DepthMeters = 0.f;
		SpearfishOxygen::Step(State, Surface, Tuning, 0.1f);
		SF_CHECK(T, !State.bBlackedOut);
		SF_CHECK_NEAR(T, State.BreathHoldRemaining, Tuning.BreathHoldSeconds, 0.001f);
	}

	inline void OxygenShockAndRefill(FSpearfishRulesTestContext& T)
	{
		const FSpearfishOxygenTuning Tuning;
		FSpearfishBreathState State;
		SpearfishOxygen::Refill(State, 300.f, Tuning);
		SpearfishOxygen::ApplyShock(State, 45.f);
		SF_CHECK_NEAR(T, State.Air, 255.f, 0.001f);
		SpearfishOxygen::ApplyShock(State, 1000.f);
		SF_CHECK(T, State.Air == 0.f);
		SpearfishOxygen::Refill(State, 420.f, Tuning);
		SF_CHECK(T, State.Air == 420.f && State.MaxAir == 420.f && !State.bOutOfAir && !State.bBlackedOut);
	}

	// ---------------------------------------------------------------------------------- Inventory
	inline void BagRespectsSlotsWeightAndDuplicates(FSpearfishRulesTestContext& T)
	{
		FSpearfishBag Bag;
		Bag.Capacity.FishSlots = 4;
		Bag.Capacity.LootSlots = 1;
		Bag.Capacity.MaxWeightKg = 10.f;

		SF_CHECK(T, SpearfishInventory::TryAdd(Bag, MakeFish(1, TEXT("Snapper"), TEXT("ReefFish"), 40.f, 1.f, 10)) == ESpearfishBagResult::Ok);
		// Duplicate instance ids are rejected (protects against double-bagging the same fish).
		SF_CHECK(T, SpearfishInventory::TryAdd(Bag, MakeFish(1, TEXT("Snapper"), TEXT("ReefFish"), 40.f, 1.f, 10)) == ESpearfishBagResult::Invalid);
		// A 70 cm fish costs 2 slots.
		SF_CHECK(T, SpearfishInventory::TryAdd(Bag, MakeFish(2, TEXT("Jack"), TEXT("Pelagic"), 70.f, 3.f, 30)) == ESpearfishBagResult::Ok);
		SF_CHECK(T, SpearfishInventory::UsedFishSlots(Bag) == 3);
		SF_CHECK(T, SpearfishInventory::TryAdd(Bag, MakeFish(3, TEXT("Grouper"), TEXT("ReefFish"), 80.f, 4.f, 40)) == ESpearfishBagResult::NoFishSlots);
		SF_CHECK(T, SpearfishInventory::TryAdd(Bag, MakeFish(4, TEXT("Snapper"), TEXT("ReefFish"), 30.f, 7.f, 5)) == ESpearfishBagResult::TooHeavy);
		SF_CHECK(T, SpearfishInventory::TryAdd(Bag, MakeLoot(5, TEXT("Pearl"), 0.05f, 80)) == ESpearfishBagResult::Ok);
		SF_CHECK(T, SpearfishInventory::TryAdd(Bag, MakeLoot(6, TEXT("Shell"), 0.1f, 5)) == ESpearfishBagResult::NoLootSlots);
		// Invalid item (id 0)
		SF_CHECK(T, SpearfishInventory::TryAdd(Bag, MakeLoot(0, TEXT("Shell"), 0.1f, 5)) == ESpearfishBagResult::Invalid);
		SF_CHECK(T, Bag.Items.Num() == 3);
	}

	inline void BagTakeAllEmptiesExactlyOnce(FSpearfishRulesTestContext& T)
	{
		FSpearfishBag Bag;
		SpearfishInventory::TryAdd(Bag, MakeFish(1, TEXT("Snapper"), TEXT("ReefFish"), 40.f, 1.f, 10));
		SpearfishInventory::TryAdd(Bag, MakeLoot(2, TEXT("Pearl"), 0.05f, 80));
		const TArray<FSpearfishItem> First = SpearfishInventory::TakeAll(Bag);
		const TArray<FSpearfishItem> Second = SpearfishInventory::TakeAll(Bag);
		SF_CHECK(T, First.Num() == 2);
		SF_CHECK(T, Second.Num() == 0);
		SF_CHECK(T, Bag.Items.Num() == 0);
		SF_CHECK(T, SpearfishInventory::TotalValue(First) == 90);
	}

	inline void BagLoadSlowsDiver(FSpearfishRulesTestContext& T)
	{
		FSpearfishBag Bag;
		Bag.Capacity.MaxWeightKg = 10.f;
		SF_CHECK_NEAR(T, SpearfishInventory::LoadSpeedMultiplier(Bag), 1.f, 0.0001f);
		SpearfishInventory::TryAdd(Bag, MakeFish(1, TEXT("Grouper"), TEXT("ReefFish"), 50.f, 10.f, 40));
		SF_CHECK_NEAR(T, SpearfishInventory::LoadSpeedMultiplier(Bag, 0.3f), 0.7f, 0.0001f);
		FSpearfishItem Removed;
		SF_CHECK(T, SpearfishInventory::RemoveByInstance(Bag, 1, &Removed));
		SF_CHECK(T, Removed.InstanceId == 1);
		SF_CHECK(T, !SpearfishInventory::RemoveByInstance(Bag, 1));
	}

	// ------------------------------------------------------------------------------------- Orders
	inline void OrdersPreferSpecificRequirementsAndCheapFish(FSpearfishRulesTestContext& T)
	{
		TArray<FSpearfishItem> Pool;
		Pool.Add(MakeFish(10, TEXT("Snapper"), TEXT("ReefFish"), 40.f, 1.f, 5));
		Pool.Add(MakeFish(11, TEXT("Grouper"), TEXT("ReefFish"), 50.f, 2.f, 20));

		// Category first in the list, species second: naive greedy would take the snapper for the
		// category line and then fail the species line.
		TArray<FSpearfishIngredientReq> Reqs;
		Reqs.Add(MakeReq(nullptr, TEXT("ReefFish"), 1));
		Reqs.Add(MakeReq(TEXT("Snapper"), nullptr, 1, 35.f));

		TArray<int32> Picked;
		SF_CHECK(T, SpearfishOrders::FindIngredients(Pool, Reqs, Picked));
		SF_CHECK(T, Picked.Num() == 2);
		SF_CHECK(T, Picked.Contains(0) && Picked.Contains(1));
	}

	inline void OrdersRespectMinimumLengthAndCount(FSpearfishRulesTestContext& T)
	{
		TArray<FSpearfishItem> Pool;
		Pool.Add(MakeFish(1, TEXT("Snapper"), TEXT("ReefFish"), 30.f, 1.f, 5));
		Pool.Add(MakeFish(2, TEXT("Snapper"), TEXT("ReefFish"), 38.f, 1.f, 6));

		TArray<int32> Picked;
		TArray<FSpearfishIngredientReq> NeedTwoBig;
		NeedTwoBig.Add(MakeReq(TEXT("Snapper"), nullptr, 2, 35.f));
		SF_CHECK(T, !SpearfishOrders::FindIngredients(Pool, NeedTwoBig, Picked));
		SF_CHECK(T, Picked.Num() == 0);

		const TArray<int32> Missing = SpearfishOrders::MissingCounts(Pool, NeedTwoBig);
		SF_CHECK(T, Missing.Num() == 1 && Missing[0] == 1);

		TArray<FSpearfishIngredientReq> NeedOneBig;
		NeedOneBig.Add(MakeReq(TEXT("snapper"), nullptr, 1, 35.f)); // FName comparison is case-insensitive
		SF_CHECK(T, SpearfishOrders::FindIngredients(Pool, NeedOneBig, Picked));
		SF_CHECK(T, Picked.Num() == 1 && Picked[0] == 1);

		// Loot never satisfies a fish ingredient.
		TArray<FSpearfishItem> LootOnly;
		LootOnly.Add(MakeLoot(3, TEXT("Snapper"), 0.1f, 1));
		TArray<FSpearfishIngredientReq> AnyFish;
		AnyFish.Add(MakeReq(nullptr, nullptr, 1));
		SF_CHECK(T, !SpearfishOrders::FindIngredients(LootOnly, AnyFish, Picked));
	}

	inline void PayoutRewardsQualityAndSpeed(FSpearfishRulesTestContext& T)
	{
		const FSpearfishServiceTuning Tuning;
		const FSpearfishPayout Great = SpearfishOrders::ComputePayout(40, 0.95f, 0.9f, 0.6f, 1.f, 1.f, Tuning);
		const FSpearfishPayout Slow = SpearfishOrders::ComputePayout(40, 0.95f, 0.05f, 0.6f, 1.f, 1.f, Tuning);
		const FSpearfishPayout Poor = SpearfishOrders::ComputePayout(40, 0.1f, 0.9f, 0.6f, 1.f, 1.f, Tuning);

		SF_CHECK(T, Great.Total > Slow.Total);
		SF_CHECK(T, Great.Total > Poor.Total);
		SF_CHECK(T, Great.Tip > 0);
		SF_CHECK(T, Great.QualityAdjustment > 0);
		SF_CHECK(T, Poor.QualityAdjustment < 0);
		SF_CHECK(T, Poor.Total > 0);
		SF_CHECK(T, Great.ReputationDelta > 0.f);
		SF_CHECK(T, Poor.ReputationDelta < 0.f);
		SF_CHECK(T, Great.Total == Great.BasePrice + Great.QualityAdjustment + Great.Tip);
		SF_CHECK(T, SpearfishOrders::WalkoutReputationDelta(1.f, Tuning) < 0.f);
		// A critic (weight 3) swings reputation harder than a tourist.
		const FSpearfishPayout Critic = SpearfishOrders::ComputePayout(40, 0.95f, 0.9f, 0.6f, 1.f, 3.f, Tuning);
		SF_CHECK(T, Critic.ReputationDelta > Great.ReputationDelta);
	}

	// ------------------------------------------------------------------------------------ Cooking
	inline void PerfectMinigamePlayScoresHigh(FSpearfishRulesTestContext& T)
	{
		// Fillet: press exactly on each mark.
		{
			FSpearfishMinigameState S;
			SpearfishMinigame::Start(S, ESpearfishCookStep::Fillet, 0.3f, 7);
			int32 Guard = 0;
			while (!S.bFinished && Guard++ < 10000)
			{
				SpearfishMinigame::Tick(S, 0.005f);
				if (!S.bFinished && S.Progress < S.Targets.Num() && FMath::Abs(S.Cursor - S.Targets[S.Progress]) < 0.004f)
				{
					SpearfishMinigame::Input(S, ESpearfishMinigameInput::Press);
				}
			}
			SF_CHECK(T, S.bFinished);
			SF_CHECK(T, S.Score > 0.9f);
		}
		// Chop: press on the beat.
		{
			FSpearfishMinigameState S;
			SpearfishMinigame::Start(S, ESpearfishCookStep::Chop, 0.5f, 3);
			int32 Guard = 0;
			while (!S.bFinished && Guard++ < 10000)
			{
				SpearfishMinigame::Tick(S, 0.005f);
				if (!S.bFinished && S.Progress < S.Targets.Num() && FMath::Abs(S.Elapsed - S.Targets[S.Progress]) < 0.004f)
				{
					SpearfishMinigame::Input(S, ESpearfishMinigameInput::Press);
				}
			}
			SF_CHECK(T, S.bFinished);
			SF_CHECK(T, S.Score > 0.9f);
		}
		// Grill: flip when doneness is centred.
		{
			FSpearfishMinigameState S;
			SpearfishMinigame::Start(S, ESpearfishCookStep::Grill, 0.5f, 11);
			const float Center = 0.5f * (S.ZoneMin + S.ZoneMax);
			int32 Guard = 0;
			while (!S.bFinished && Guard++ < 10000)
			{
				SpearfishMinigame::Tick(S, 0.005f);
				if (!S.bFinished && FMath::Abs(S.Cursor - Center) < 0.004f)
				{
					SpearfishMinigame::Input(S, ESpearfishMinigameInput::Press);
				}
			}
			SF_CHECK(T, S.Score > 0.9f);
		}
		// Plate: enter the sequence correctly and fast.
		{
			FSpearfishMinigameState S;
			SpearfishMinigame::Start(S, ESpearfishCookStep::Plate, 1.f, 5);
			SF_CHECK(T, S.Sequence.Num() == 7);
			const TArray<ESpearfishMinigameInput> Sequence = S.Sequence;
			for (const ESpearfishMinigameInput Arrow : Sequence)
			{
				SpearfishMinigame::Tick(S, 0.2f);
				SpearfishMinigame::Input(S, Arrow);
			}
			SF_CHECK(T, S.bFinished);
			SF_CHECK(T, S.Score > 0.95f);
		}
	}

	inline void LazyOrSloppyMinigamePlayScoresLow(FSpearfishRulesTestContext& T)
	{
		// Doing nothing fails every minigame and always terminates.
		const ESpearfishCookStep Steps[] = { ESpearfishCookStep::Fillet, ESpearfishCookStep::Chop, ESpearfishCookStep::Season,
			ESpearfishCookStep::Grill, ESpearfishCookStep::Fry, ESpearfishCookStep::Simmer, ESpearfishCookStep::Plate };
		for (const ESpearfishCookStep Step : Steps)
		{
			FSpearfishMinigameState S;
			SpearfishMinigame::Start(S, Step, 0.5f, 99);
			int32 Guard = 0;
			while (!S.bFinished && Guard++ < 100000)
			{
				SpearfishMinigame::Tick(S, 0.01f);
			}
			SF_CHECK(T, S.bFinished);
			SF_CHECK(T, S.Score < 0.25f);
		}

		// Mashing the wrong arrows while plating costs points.
		FSpearfishMinigameState Plate;
		SpearfishMinigame::Start(Plate, ESpearfishCookStep::Plate, 0.f, 5);
		const TArray<ESpearfishMinigameInput> Sequence = Plate.Sequence;
		for (const ESpearfishMinigameInput Arrow : Sequence)
		{
			const ESpearfishMinigameInput Wrong = Arrow == ESpearfishMinigameInput::Up ? ESpearfishMinigameInput::Down : ESpearfishMinigameInput::Up;
			SpearfishMinigame::Input(Plate, Wrong);
			SpearfishMinigame::Input(Plate, Arrow);
		}
		SF_CHECK(T, Plate.bFinished);
		SF_CHECK(T, Plate.Score < 0.7f);
	}

	inline void FryRewardsHoldingTheBand(FSpearfishRulesTestContext& T)
	{
		FSpearfishMinigameState S;
		SpearfishMinigame::Start(S, ESpearfishCookStep::Fry, 0.3f, 21);
		const float Center = 0.5f * (S.ZoneMin + S.ZoneMax);
		int32 Guard = 0;
		while (!S.bFinished && Guard++ < 100000)
		{
			// Simple bang-bang controller around the band centre.
			const bool bWantHeat = S.Cursor < Center;
			if (bWantHeat != S.bHolding)
			{
				SpearfishMinigame::Input(S, bWantHeat ? ESpearfishMinigameInput::Press : ESpearfishMinigameInput::Release);
			}
			SpearfishMinigame::Tick(S, 0.01f);
		}
		SF_CHECK(T, S.Score > 0.75f);
	}

	inline void DishQualityCombinesStepsAndIngredients(FSpearfishRulesTestContext& T)
	{
		TArray<float> Perfect;
		Perfect.Init(1.f, 4);
		SF_CHECK_NEAR(T, SpearfishCooking::DishQuality(Perfect, 1.f), 1.f, 0.0001f);
		TArray<float> Mixed;
		Mixed.Add(1.f);
		Mixed.Add(0.f);
		SF_CHECK_NEAR(T, SpearfishCooking::DishQuality(Mixed, 0.5f), 0.5f, 0.0001f);
		SF_CHECK(T, SpearfishCooking::QualityTier(0.95f) == 3);
		SF_CHECK(T, SpearfishCooking::QualityTier(0.2f) == 0);
		SF_CHECK(T, SpearfishCooking::DishQuality(Perfect, 1.f, 0.5f) <= 1.f);

		FRandomStream Rng(42);
		for (int32 Index = 0; Index < 100; ++Index)
		{
			const float Score = SpearfishCooking::AutoChefStepScore(0.7f, Rng);
			SF_CHECK(T, Score >= 0.2f && Score <= 1.f);
		}
	}

	// -------------------------------------------------------------------------------------- Roles
	inline TArray<FSpearfishRoleSeat> TwoSeats(ESpearfishRole Host, ESpearfishRole Guest)
	{
		TArray<FSpearfishRoleSeat> Seats;
		FSpearfishRoleSeat A;
		A.PlayerKey = FName(TEXT("Host"));
		A.SeatIndex = 0;
		A.Role = Host;
		FSpearfishRoleSeat B;
		B.PlayerKey = FName(TEXT("Guest"));
		B.SeatIndex = 1;
		B.Role = Guest;
		Seats.Add(A);
		Seats.Add(B);
		return Seats;
	}

	inline void RolesSwapEveryNightInCoop(FSpearfishRulesTestContext& T)
	{
		TArray<FSpearfishRoleSeat> Seats = TwoSeats(ESpearfishRole::Diver, ESpearfishRole::Chef);
		for (int32 Night = 0; Night < 6; ++Night)
		{
			const ESpearfishRole HostBefore = Seats[0].Role;
			SpearfishRoles::ResolveNextDay(Seats, false);
			SF_CHECK(T, Seats[0].Role == SpearfishRoles::Opposite(HostBefore));
			SF_CHECK(T, SpearfishRoles::IsValidAssignment(Seats, false));
			SF_CHECK(T, !SpearfishRoles::NeedsAutoChef(Seats, false));
		}
	}

	inline void RolesOrderIndependentOfArrayOrder(FSpearfishRulesTestContext& T)
	{
		// Seats listed guest-first must still resolve by SeatIndex.
		TArray<FSpearfishRoleSeat> Seats = TwoSeats(ESpearfishRole::None, ESpearfishRole::None);
		Seats.Swap(0, 1);
		SpearfishRoles::ResolveNextDay(Seats, false);
		const FSpearfishRoleSeat* Host = Seats.FindByPredicate([](const FSpearfishRoleSeat& S) { return S.SeatIndex == 0; });
		SF_CHECK(T, Host && Host->Role == ESpearfishRole::Diver);
		SF_CHECK(T, SpearfishRoles::IsValidAssignment(Seats, false));
	}

	inline void RolesSoloAlwaysDiver(FSpearfishRulesTestContext& T)
	{
		TArray<FSpearfishRoleSeat> Seats;
		FSpearfishRoleSeat Solo;
		Solo.Role = ESpearfishRole::Chef;
		Seats.Add(Solo);
		SpearfishRoles::ResolveNextDay(Seats, true);
		SF_CHECK(T, Seats[0].Role == ESpearfishRole::Diver);
		SpearfishRoles::ResolveNextDay(Seats, true);
		SF_CHECK(T, Seats[0].Role == ESpearfishRole::Diver);
		SF_CHECK(T, SpearfishRoles::NeedsAutoChef(Seats, true));
		SF_CHECK(T, SpearfishRoles::RoleForJoiningPlayer(Seats, true) == ESpearfishRole::Diver);
	}

	inline void RolesDisconnectAndRejoin(FSpearfishRulesTestContext& T)
	{
		// Diver leaves mid-day: the chef takes over diving, auto-chef runs the kitchen.
		TArray<FSpearfishRoleSeat> Seats = TwoSeats(ESpearfishRole::Chef, ESpearfishRole::Diver);
		Seats[1].bConnected = false;
		SpearfishRoles::HandlePartnerLeft(Seats);
		SF_CHECK(T, Seats[0].Role == ESpearfishRole::Diver);
		SF_CHECK(T, Seats[1].Role == ESpearfishRole::None);
		SF_CHECK(T, SpearfishRoles::NeedsAutoChef(Seats, false));
		SF_CHECK(T, SpearfishRoles::IsValidAssignment(Seats, false));

		// Partner rejoins mid-day and takes the free role.
		Seats[1].bConnected = true;
		Seats[1].Role = SpearfishRoles::RoleForJoiningPlayer(Seats, false);
		SF_CHECK(T, Seats[1].Role == ESpearfishRole::Chef);
		SF_CHECK(T, !SpearfishRoles::NeedsAutoChef(Seats, false));

		// Night: today's diver (host) cooks tomorrow.
		SpearfishRoles::ResolveNextDay(Seats, false);
		SF_CHECK(T, Seats[0].Role == ESpearfishRole::Chef);
		SF_CHECK(T, Seats[1].Role == ESpearfishRole::Diver);
	}

	inline void RolesNewcomerAtNightGetsSwappedFairly(FSpearfishRulesTestContext& T)
	{
		// Host dove solo all day; guest joined at night with no role. Tomorrow host cooks, guest dives.
		TArray<FSpearfishRoleSeat> Seats = TwoSeats(ESpearfishRole::Diver, ESpearfishRole::None);
		SpearfishRoles::ResolveNextDay(Seats, false);
		SF_CHECK(T, Seats[0].Role == ESpearfishRole::Chef && Seats[1].Role == ESpearfishRole::Diver);

		// Disconnected seats never keep roles.
		TArray<FSpearfishRoleSeat> Gone = TwoSeats(ESpearfishRole::Diver, ESpearfishRole::Chef);
		Gone[0].bConnected = false;
		SpearfishRoles::ResolveNextDay(Gone, false);
		SF_CHECK(T, Gone[0].Role == ESpearfishRole::None);
		SF_CHECK(T, Gone[1].Role == ESpearfishRole::Diver);
	}

	// ---------------------------------------------------------------------------------------- Day
	inline void DayPhasesAndSleepGates(FSpearfishRulesTestContext& T)
	{
		const FSpearfishDaySchedule Schedule;
		SF_CHECK(T, SpearfishDay::PhaseForHour(6.f, Schedule) == ESpearfishDayPhase::Morning);
		SF_CHECK(T, SpearfishDay::PhaseForHour(12.f, Schedule) == ESpearfishDayPhase::Service);
		SF_CHECK(T, SpearfishDay::PhaseForHour(19.f, Schedule) == ESpearfishDayPhase::Closing);
		SF_CHECK(T, SpearfishDay::PhaseForHour(22.f, Schedule) == ESpearfishDayPhase::Night);
		SF_CHECK(T, SpearfishDay::AcceptsNewGuests(12.f, Schedule));
		SF_CHECK(T, !SpearfishDay::AcceptsNewGuests(19.f, Schedule));
		SF_CHECK(T, !SpearfishDay::CanSleep(12.f, Schedule));
		SF_CHECK(T, SpearfishDay::CanSleep(21.f, Schedule));

		TArray<bool> Nobody;
		SF_CHECK(T, !SpearfishDay::EveryoneReady(Nobody));
		TArray<bool> OneOfTwo;
		OneOfTwo.Add(true);
		OneOfTwo.Add(false);
		SF_CHECK(T, !SpearfishDay::EveryoneReady(OneOfTwo));
		OneOfTwo[1] = true;
		SF_CHECK(T, SpearfishDay::EveryoneReady(OneOfTwo));
	}

	inline void DayClockAdvancesAndClampsAtPassOut(FSpearfishRulesTestContext& T)
	{
		const FSpearfishDaySchedule Schedule;
		float Hour = Schedule.DayStartHour;
		Hour = SpearfishDay::Advance(Hour, Schedule.RealSecondsPerGameHour, Schedule);
		SF_CHECK_NEAR(T, Hour, Schedule.DayStartHour + 1.f, 0.001f);
		// Night runs faster.
		const float NightStart = Schedule.ClosingHour + 0.1f;
		const float NightNext = SpearfishDay::Advance(NightStart, Schedule.RealSecondsPerGameHour, Schedule);
		SF_CHECK_NEAR(T, NightNext - NightStart, Schedule.NightTimeScale, 0.001f);
		// Clamp.
		const float Late = SpearfishDay::Advance(25.9f, 10000.f, Schedule);
		SF_CHECK(T, Late == Schedule.PassOutHour);
		SF_CHECK(T, SpearfishDay::ShouldPassOut(Late, Schedule));

		int32 H = 0;
		int32 M = 0;
		SpearfishDay::ToClock(25.5f, H, M);
		SF_CHECK(T, H == 1 && M == 30);
	}

	// ------------------------------------------------------------------------------------ Economy
	inline void UnlockChecksReportFundamentalBlockerFirst(FSpearfishRulesTestContext& T)
	{
		FSpearfishUnlockReq Req;
		Req.Price = 500;
		Req.MinReputation = 2.f;
		Req.MinDay = 3;
		Req.MinSpeciesCaught = 6;
		Req.Prerequisite = FName(TEXT("Gun_T1"));

		FSpearfishProgressSnapshot Progress;
		Progress.Money = 100;
		SF_CHECK(T, SpearfishEconomy::Check(Req, Progress, FName(TEXT("Gun_T2"))) == ESpearfishRequirementResult::NeedsPrerequisite);
		Progress.Owned.Add(FName(TEXT("Gun_T1")));
		SF_CHECK(T, SpearfishEconomy::Check(Req, Progress, FName(TEXT("Gun_T2"))) == ESpearfishRequirementResult::NeedsDay);
		Progress.Day = 3;
		SF_CHECK(T, SpearfishEconomy::Check(Req, Progress, FName(TEXT("Gun_T2"))) == ESpearfishRequirementResult::NeedsReputation);
		Progress.Reputation = 2.f;
		SF_CHECK(T, SpearfishEconomy::Check(Req, Progress, FName(TEXT("Gun_T2"))) == ESpearfishRequirementResult::NeedsSpecies);
		Progress.SpeciesCaught = 6;
		SF_CHECK(T, SpearfishEconomy::Check(Req, Progress, FName(TEXT("Gun_T2"))) == ESpearfishRequirementResult::NotEnoughMoney);
		Progress.Money = 500;
		SF_CHECK(T, SpearfishEconomy::Check(Req, Progress, FName(TEXT("Gun_T2"))) == ESpearfishRequirementResult::Ok);
		Progress.Owned.Add(FName(TEXT("Gun_T2")));
		SF_CHECK(T, SpearfishEconomy::Check(Req, Progress, FName(TEXT("Gun_T2"))) == ESpearfishRequirementResult::AlreadyOwned);
	}

	inline void EconomyClampsNeverGoNegative(FSpearfishRulesTestContext& T)
	{
		SF_CHECK(T, SpearfishEconomy::ApplyReputation(4.9f, 1.f) == SpearfishEconomy::MaxReputation);
		SF_CHECK(T, SpearfishEconomy::ApplyReputation(0.1f, -1.f) == 0.f);
		SF_CHECK(T, SpearfishEconomy::RescueFee(10, 25) == 10);
		SF_CHECK(T, SpearfishEconomy::RescueFee(-5, 25) == 0);
		SF_CHECK(T, SpearfishEconomy::RescueFee(100, 25) == 25);
		SF_CHECK(T, SpearfishEconomy::LootValue(100, 1.f) == 100);
		SF_CHECK(T, SpearfishEconomy::LootValue(100, 0.f) == 50);
	}

	// --------------------------------------------------------------------------------------- Line
	inline void LinePaysOutUnderStrongFishAndReelsWeakOnes(FSpearfishRulesTestContext& T)
	{
		FSpearfishLineSpec Spec;
		FSpearfishLineState Line;
		SpearfishLine::Attach(Line, 500.f, Spec);

		// Weak fish: reeling brings it in.
		FSpearfishLineInput Weak;
		Weak.DistanceCm = 500.f;
		Weak.PullAwayForce = 60.f;
		Weak.bReeling = true;
		Weak.DeltaSeconds = 0.1f;
		for (int32 Step = 0; Step < 10; ++Step)
		{
			SpearfishLine::Step(Line, Weak, Spec);
			Weak.DistanceCm = Line.LengthCm;
		}
		SF_CHECK(T, Line.LengthCm < 500.f);
		SF_CHECK(T, !Line.bSnapped);

		// Strong fish: drag slips and line pays out even while reeling.
		FSpearfishLineState Strong;
		SpearfishLine::Attach(Strong, 500.f, Spec);
		FSpearfishLineInput Pull;
		Pull.DistanceCm = 500.f;
		Pull.PullAwayForce = Spec.DragForce * 1.5f;
		Pull.bReeling = true;
		Pull.DeltaSeconds = 0.1f;
		SpearfishLine::Step(Strong, Pull, Spec);
		SF_CHECK(T, Strong.LengthCm > 500.f);
		SF_CHECK(T, Strong.bTaut);
	}

	inline void LineSnapsOnlyUnderSustainedOverloadAtFullLength(FSpearfishRulesTestContext& T)
	{
		FSpearfishLineSpec Spec;
		FSpearfishLineState Line;
		SpearfishLine::Attach(Line, Spec.MaxLengthCm, Spec);
		FSpearfishLineInput Heavy;
		Heavy.DistanceCm = Spec.MaxLengthCm;
		Heavy.PullAwayForce = Spec.BreakForce * 1.2f;
		Heavy.DeltaSeconds = 0.1f;
		SpearfishLine::Step(Line, Heavy, Spec);
		SF_CHECK(T, !Line.bSnapped);
		for (int32 Step = 0; Step < 20 && !Line.bSnapped; ++Step)
		{
			SpearfishLine::Step(Line, Heavy, Spec);
		}
		SF_CHECK(T, Line.bSnapped);

		// Slack line never snaps.
		FSpearfishLineState Slack;
		SpearfishLine::Attach(Slack, Spec.MaxLengthCm, Spec);
		FSpearfishLineInput Loose = Heavy;
		Loose.DistanceCm = Spec.MaxLengthCm * 0.5f;
		for (int32 Step = 0; Step < 50; ++Step)
		{
			SpearfishLine::Step(Slack, Loose, Spec);
		}
		SF_CHECK(T, !Slack.bSnapped && !Slack.bTaut && Slack.Tension == 0.f);

		// Instant snap under huge shock (e.g. hooking a shark).
		FSpearfishLineState Shock;
		SpearfishLine::Attach(Shock, 300.f, Spec);
		FSpearfishLineInput Yank = Heavy;
		Yank.DistanceCm = 300.f;
		Yank.PullAwayForce = Spec.BreakForce * 2.f;
		SpearfishLine::Step(Shock, Yank, Spec);
		SF_CHECK(T, Shock.bSnapped);
	}

	// --------------------------------------------------------------------------------------- Fish
	inline void FishSizesWeightsAndValues(FSpearfishRulesTestContext& T)
	{
		FRandomStream Rng(1234);
		int32 SmallHalf = 0;
		for (int32 Index = 0; Index < 1000; ++Index)
		{
			const float Length = SpearfishFish::RollLength(Rng, 20.f, 60.f);
			SF_CHECK(T, Length >= 20.f && Length <= 60.f);
			SmallHalf += Length < 40.f ? 1 : 0;
		}
		// Size bias: most fish are on the small side.
		SF_CHECK(T, SmallHalf > 550);

		SF_CHECK(T, SpearfishFish::WeightForLength(100.f, 12.f) > SpearfishFish::WeightForLength(50.f, 12.f) * 7.f);
		const int32 Common = SpearfishFish::SaleValue(2.f, 10.f, ESpearfishRarity::Common, 0.8f, 1.f);
		const int32 Legend = SpearfishFish::SaleValue(2.f, 10.f, ESpearfishRarity::Legendary, 0.8f, 1.f);
		SF_CHECK(T, Legend > Common * 3);
		SF_CHECK(T, SpearfishFish::SaleValue(0.f, 10.f, ESpearfishRarity::Common, 0.f, 1.f) == 1);

		SF_CHECK(T, SpearfishFish::CatchQuality(1.f, 2.f, 5.f) > SpearfishFish::CatchQuality(0.f, 40.f, 5.f));
		SF_CHECK(T, SpearfishFish::PullForce(300.f, 1.f, 1.f) > SpearfishFish::PullForce(300.f, 0.f, 1.f));
		SF_CHECK(T, SpearfishFish::PullForce(300.f, 1.f, 0.f) < SpearfishFish::PullForce(300.f, 1.f, 0.5f) * 0.3f);
		const float MaxStamina = SpearfishFish::MaxStamina(10.f, 0.5f);
		SF_CHECK(T, SpearfishFish::HitStaminaDamage(3.f, 2.f, 0.f, MaxStamina) <= MaxStamina * 0.85f + 0.001f);
		SF_CHECK(T, SpearfishFish::HitStaminaDamage(1.f, 2.f, 0.f, MaxStamina) > SpearfishFish::HitStaminaDamage(1.f, 0.6f, 0.f, MaxStamina));
	}

	inline void FishPerceptionRewardsSlowApproach(FSpearfishRulesTestContext& T)
	{
		const float Slow = SpearfishFish::DetectionRadius(800.f, 0.1f, false, 0.f);
		const float Fast = SpearfishFish::DetectionRadius(800.f, 1.f, false, 0.f);
		const float Lit = SpearfishFish::DetectionRadius(800.f, 0.1f, true, 0.5f);
		SF_CHECK(T, Fast > Slow * 2.f);
		SF_CHECK(T, Lit > Slow);
		// Light-loving species (negative sensitivity) notice lights less.
		SF_CHECK(T, SpearfishFish::DetectionRadius(800.f, 0.1f, true, -0.5f) < Slow);
	}

	inline void FishMindDecisions(FSpearfishRulesTestContext& T)
	{
		FSpearfishFishSenses S;
		S.Archetype = ESpearfishFishArchetype::Schooler;
		S.bHasSchool = true;
		SF_CHECK(T, SpearfishFish::DecideMind(ESpearfishFishMind::Wander, S) == ESpearfishFishMind::School);

		S.ThreatDistance = 300.f;
		S.DetectionRadius = 800.f;
		SF_CHECK(T, SpearfishFish::DecideMind(ESpearfishFishMind::School, S) == ESpearfishFishMind::Flee);
		S.ThreatDistance = 700.f;
		SF_CHECK(T, SpearfishFish::DecideMind(ESpearfishFishMind::School, S) == ESpearfishFishMind::Alert);

		// Fleeing persists for a while, then calms into cover or alert.
		S.ThreatDistance = 2000.f;
		S.TimeInState = 0.5f;
		SF_CHECK(T, SpearfishFish::DecideMind(ESpearfishFishMind::Flee, S) == ESpearfishFishMind::Flee);
		S.TimeInState = 10.f;
		S.bInCover = true;
		SF_CHECK(T, SpearfishFish::DecideMind(ESpearfishFishMind::Flee, S) == ESpearfishFishMind::Hide);

		// Hooked overrides everything; exhausted when stamina is gone.
		S.bHooked = true;
		S.StaminaFraction = 0.6f;
		SF_CHECK(T, SpearfishFish::DecideMind(ESpearfishFishMind::Wander, S) == ESpearfishFishMind::Hooked);
		S.StaminaFraction = 0.f;
		SF_CHECK(T, SpearfishFish::DecideMind(ESpearfishFishMind::Hooked, S) == ESpearfishFishMind::Exhausted);

		// Predators hunt prey in range and only attack divers when provoked.
		FSpearfishFishSenses Shark;
		Shark.Archetype = ESpearfishFishArchetype::Predator;
		Shark.PreyDistance = 500.f;
		Shark.HuntRadius = 1500.f;
		Shark.DiverDistance = 200.f;
		Shark.AttackRadius = 400.f;
		SF_CHECK(T, SpearfishFish::DecideMind(ESpearfishFishMind::Wander, Shark) == ESpearfishFishMind::Hunt);
		Shark.bProvoked = true;
		SF_CHECK(T, SpearfishFish::DecideMind(ESpearfishFishMind::Wander, Shark) == ESpearfishFishMind::Attack);

		// Morays lunge when you put your hand in their den.
		FSpearfishFishSenses Moray;
		Moray.Archetype = ESpearfishFishArchetype::Ambusher;
		Moray.AttackRadius = 150.f;
		Moray.DiverDistance = 100.f;
		SF_CHECK(T, SpearfishFish::DecideMind(ESpearfishFishMind::Hide, Moray) == ESpearfishFishMind::Lunge);

		FSpearfishFishSenses Jelly;
		Jelly.Archetype = ESpearfishFishArchetype::Drifter;
		Jelly.ThreatDistance = 10.f;
		SF_CHECK(T, SpearfishFish::DecideMind(ESpearfishFishMind::Wander, Jelly) == ESpearfishFishMind::Drift);
	}


	// ------------------------------------------------------------------------------------ Terrain
	inline FSpearfishTerrainParams CoralCoveParams()
	{
		FSpearfishTerrainParams Params;
		Params.HalfExtentCm = 9000.f;
		Params.ShallowDepthM = 3.f;
		Params.MaxDepthM = 46.f;
		Params.ReefDensity = 0.65f;
		Params.Wrecks = 1;
		Params.Caves = 2;
		Params.GiantClams = 5;
		return Params;
	}

	inline void TerrainIsDeterministic(FSpearfishRulesTestContext& T)
	{
		FSpearfishTerrain A;
		FSpearfishTerrain B;
		FSpearfishTerrain C;
		A.Initialize(CoralCoveParams(), 1337);
		B.Initialize(CoralCoveParams(), 1337);
		C.Initialize(CoralCoveParams(), 4242);
		FRandomStream Rng(5);
		int32 Different = 0;
		for (int32 Index = 0; Index < 200; ++Index)
		{
			const float X = Rng.FRandRange(-9000.f, 9000.f);
			const float Y = Rng.FRandRange(-9000.f, 9000.f);
			SF_CHECK(T, A.GetSeabedZ(X, Y) == B.GetSeabedZ(X, Y));
			SF_CHECK(T, A.GetBiome(X, Y) == B.GetBiome(X, Y));
			Different += FMath::Abs(A.GetSeabedZ(X, Y) - C.GetSeabedZ(X, Y)) > 1.f ? 1 : 0;
		}
		SF_CHECK(T, Different > 100);
		SF_CHECK(T, A.GetLayout().POIs.Num() == B.GetLayout().POIs.Num());
	}

	inline void TerrainLayoutIsPlayable(FSpearfishRulesTestContext& T)
	{
		FSpearfishTerrain Terrain;
		Terrain.Initialize(CoralCoveParams(), 1337);
		const FSpearfishTerrainLayout& Layout = Terrain.GetLayout();

		// Island above water, dock on land, boat moored in water deep enough for the hull.
		SF_CHECK(T, Terrain.GetSeabedZ(static_cast<float>(Layout.IslandCenter.X), static_cast<float>(Layout.IslandCenter.Y)) > 300.f);
		SF_CHECK(T, Layout.DockLocation.Z > 0.0);
		SF_CHECK(T, Terrain.GetSeabedZ(static_cast<float>(Layout.BoatStart.X), static_cast<float>(Layout.BoatStart.Y)) < -200.f);
		SF_CHECK(T, Terrain.GetBiome(static_cast<float>(Layout.IslandCenter.X), static_cast<float>(Layout.IslandCenter.Y)) == FName(TEXT("Land")));

		// Deep water in the far corner, within the configured maximum (+ trench).
		const float FarDepth = Terrain.GetWaterDepthM(8000.f, 8000.f, 0.f);
		SF_CHECK(T, FarDepth > 30.f);
		SF_CHECK(T, FarDepth < 46.f + 10.f);

		int32 Wrecks = 0;
		int32 Caves = 0;
		for (const FSpearfishPOI& POI : Layout.POIs)
		{
			if (POI.Type == ESpearfishPOIType::Wreck)
			{
				++Wrecks;
				const float Depth = -static_cast<float>(POI.Location.Z) / 100.f;
				SF_CHECK(T, Depth >= 15.f && Depth <= 40.f);
				SF_CHECK(T, Terrain.GetBiome(static_cast<float>(POI.Location.X), static_cast<float>(POI.Location.Y)) == FName(TEXT("Wreck")));
			}
			if (POI.Type == ESpearfishPOIType::Cave)
			{
				++Caves;
			}
			SF_CHECK(T, Terrain.IsInsideBounds(static_cast<float>(POI.Location.X), static_cast<float>(POI.Location.Y)));
		}
		SF_CHECK(T, Wrecks == 1);
		SF_CHECK(T, Caves == 2);

		// Biome variety and no extreme cliffs in the playable area.
		int32 Reef = 0;
		int32 Sand = 0;
		float MaxStep = 0.f;
		for (float X = -8800.f; X < 8800.f; X += 400.f)
		{
			for (float Y = -8800.f; Y < 8800.f; Y += 400.f)
			{
				const FName Biome = Terrain.GetBiome(X, Y);
				Reef += Biome == FName(TEXT("Reef")) ? 1 : 0;
				Sand += Biome == FName(TEXT("Sand")) ? 1 : 0;
				MaxStep = FMath::Max(MaxStep, FMath::Abs(Terrain.GetSeabedZ(X + 100.f, Y) - Terrain.GetSeabedZ(X, Y)));
			}
		}
		SF_CHECK(T, Reef > 50);
		SF_CHECK(T, Sand > 50);
		// Drop-off walls are intended (< ~63 degrees); POI terracing artefacts produced > 2.4 m steps.
		SF_CHECK(T, MaxStep < 200.f);
	}

	// ------------------------------------------------------------------------------------- Events

	inline void EventsRespectDayCapAndGroups(FSpearfishRulesTestContext& T)
	{
		TArray<FSpearfishEventCandidate> Candidates;
		Candidates.Add({ FName(TEXT("Legend1")), 1.f, 3, 1 });
		Candidates.Add({ FName(TEXT("Legend2")), 1.f, 1, 1 });
		Candidates.Add({ FName(TEXT("Treasure")), 1.f, 2, 2 });
		Candidates.Add({ FName(TEXT("Never")), 0.f, 1, 3 });
		Candidates.Add({ FName(TEXT("Bloom")), 1.f, 1, 4 });

		// Deterministic per seed.
		SF_CHECK(T, SpearfishEventRules::RollDailyEvents(Candidates, 5, 77) == SpearfishEventRules::RollDailyEvents(Candidates, 5, 77));

		for (int32 Seed = 1; Seed <= 200; ++Seed)
		{
			const TArray<FName> Day1 = SpearfishEventRules::RollDailyEvents(Candidates, 1, Seed, 4);
			// MinDay gates: on day 1 only Legend2 and Bloom are eligible.
			SF_CHECK(T, !Day1.Contains(FName(TEXT("Legend1"))) && !Day1.Contains(FName(TEXT("Treasure"))));
			SF_CHECK(T, Day1.Num() == 2);

			const TArray<FName> Day9 = SpearfishEventRules::RollDailyEvents(Candidates, 9, Seed, 2);
			SF_CHECK(T, Day9.Num() == 2);
			SF_CHECK(T, !Day9.Contains(FName(TEXT("Never"))));
			// At most one legendary visitor per day.
			SF_CHECK(T, !(Day9.Contains(FName(TEXT("Legend1"))) && Day9.Contains(FName(TEXT("Legend2")))));

			const TArray<FName> Wide = SpearfishEventRules::RollDailyEvents(Candidates, 9, Seed, 10);
			SF_CHECK(T, Wide.Num() == 3);
		}
		SF_CHECK(T, SpearfishEventRules::RollDailyEvents(Candidates, 9, 5, 0).Num() == 0);
	}

	inline void EventsFireAtTheirChance(FSpearfishRulesTestContext& T)
	{
		TArray<FSpearfishEventCandidate> Candidates;
		Candidates.Add({ FName(TEXT("A")), 0.15f, 1, 1 });
		Candidates.Add({ FName(TEXT("B")), 0.25f, 1, 2 });
		Candidates.Add({ FName(TEXT("C")), 0.10f, 1, 3 });
		int32 CountA = 0;
		int32 CountB = 0;
		int32 CountC = 0;
		const int32 Days = 4000;
		for (int32 Seed = 0; Seed < Days; ++Seed)
		{
			const TArray<FName> Events = SpearfishEventRules::RollDailyEvents(Candidates, 10, Seed * 7919 + 13, 3);
			CountA += Events.Contains(FName(TEXT("A"))) ? 1 : 0;
			CountB += Events.Contains(FName(TEXT("B"))) ? 1 : 0;
			CountC += Events.Contains(FName(TEXT("C"))) ? 1 : 0;
		}
		SF_CHECK_NEAR(T, static_cast<float>(CountA) / Days, 0.15f, 0.03f);
		SF_CHECK_NEAR(T, static_cast<float>(CountB) / Days, 0.25f, 0.03f);
		SF_CHECK_NEAR(T, static_cast<float>(CountC) / Days, 0.10f, 0.03f);
	}

	struct FCase
	{
		const TCHAR* Name;
		void (*Run)(FSpearfishRulesTestContext&);
	};

	inline TArray<FCase> AllCases()
	{
		TArray<FCase> Cases;
		Cases.Add({ TEXT("Oxygen.ConsumptionScales"), &OxygenConsumptionScalesWithDepthAndEffort });
		Cases.Add({ TEXT("Oxygen.StarterTankBudget"), &OxygenStarterTankIsForgivingButLimited });
		Cases.Add({ TEXT("Oxygen.BreathHoldThenBlackout"), &OxygenBreathHoldThenBlackout });
		Cases.Add({ TEXT("Oxygen.SurfacingSaves"), &OxygenSurfacingDuringBreathHoldSaves });
		Cases.Add({ TEXT("Oxygen.ShockAndRefill"), &OxygenShockAndRefill });
		Cases.Add({ TEXT("Inventory.SlotsWeightDuplicates"), &BagRespectsSlotsWeightAndDuplicates });
		Cases.Add({ TEXT("Inventory.TakeAllOnce"), &BagTakeAllEmptiesExactlyOnce });
		Cases.Add({ TEXT("Inventory.LoadSlowsDiver"), &BagLoadSlowsDiver });
		Cases.Add({ TEXT("Orders.SpecificFirstCheapest"), &OrdersPreferSpecificRequirementsAndCheapFish });
		Cases.Add({ TEXT("Orders.LengthAndCount"), &OrdersRespectMinimumLengthAndCount });
		Cases.Add({ TEXT("Orders.Payout"), &PayoutRewardsQualityAndSpeed });
		Cases.Add({ TEXT("Cooking.PerfectPlay"), &PerfectMinigamePlayScoresHigh });
		Cases.Add({ TEXT("Cooking.LazyPlay"), &LazyOrSloppyMinigamePlayScoresLow });
		Cases.Add({ TEXT("Cooking.FryBand"), &FryRewardsHoldingTheBand });
		Cases.Add({ TEXT("Cooking.DishQuality"), &DishQualityCombinesStepsAndIngredients });
		Cases.Add({ TEXT("Roles.SwapEveryNight"), &RolesSwapEveryNightInCoop });
		Cases.Add({ TEXT("Roles.SeatOrder"), &RolesOrderIndependentOfArrayOrder });
		Cases.Add({ TEXT("Roles.SoloDiver"), &RolesSoloAlwaysDiver });
		Cases.Add({ TEXT("Roles.DisconnectRejoin"), &RolesDisconnectAndRejoin });
		Cases.Add({ TEXT("Roles.NewcomerAtNight"), &RolesNewcomerAtNightGetsSwappedFairly });
		Cases.Add({ TEXT("Day.PhasesAndSleep"), &DayPhasesAndSleepGates });
		Cases.Add({ TEXT("Day.ClockAdvance"), &DayClockAdvancesAndClampsAtPassOut });
		Cases.Add({ TEXT("Economy.UnlockOrder"), &UnlockChecksReportFundamentalBlockerFirst });
		Cases.Add({ TEXT("Economy.Clamps"), &EconomyClampsNeverGoNegative });
		Cases.Add({ TEXT("Line.PayoutAndReel"), &LinePaysOutUnderStrongFishAndReelsWeakOnes });
		Cases.Add({ TEXT("Line.Snap"), &LineSnapsOnlyUnderSustainedOverloadAtFullLength });
		Cases.Add({ TEXT("Fish.SizeWeightValue"), &FishSizesWeightsAndValues });
		Cases.Add({ TEXT("Fish.Perception"), &FishPerceptionRewardsSlowApproach });
		Cases.Add({ TEXT("Fish.Mind"), &FishMindDecisions });
		Cases.Add({ TEXT("Terrain.Deterministic"), &TerrainIsDeterministic });
		Cases.Add({ TEXT("Terrain.Playable"), &TerrainLayoutIsPlayable });
		Cases.Add({ TEXT("Events.DayCapGroups"), &EventsRespectDayCapAndGroups });
		Cases.Add({ TEXT("Events.Chance"), &EventsFireAtTheirChance });
		return Cases;
	}
}
