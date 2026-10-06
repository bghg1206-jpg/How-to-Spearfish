#include "Inventory/SpearfishCatchService.h"

#include "Boat/SpearfishBoat.h"
#include "Core/SpearfishGameState.h"
#include "Core/SpearfishPlayerState.h"
#include "Data/SpearfishDataRegistry.h"
#include "DayNight/SpearfishDayCycleComponent.h"
#include "Diving/SpearfishCharacter.h"
#include "Engine/World.h"
#include "FishAI/SpearfishFish.h"
#include "Inventory/SpearfishCatchStorageComponent.h"
#include "Inventory/SpearfishDiveBagComponent.h"
#include "Progression/SpearfishJournalComponent.h"
#include "Progression/SpearfishProgressionComponent.h"
#include "Rules/EconomyRules.h"
#include "Rules/FishRules.h"
#include "Rules/InventoryRules.h"
#include "Speargun/SpeargunComponent.h"
#include "World/SpearfishOceanSubsystem.h"

#define LOCTEXT_NAMESPACE "SpearfishCatch"

namespace SpearfishCatch
{
	FSpearfishItem MakeFishItem(UWorld* World, const ASpearfishFish* Fish, float ShotQuality, float FightSeconds)
	{
		FSpearfishItem Item;
		ASpearfishGameState* GameState = World ? World->GetGameState<ASpearfishGameState>() : nullptr;
		const FSpearfishFishSpeciesDef* Species = Fish ? Fish->GetSpecies() : nullptr;
		if (!GameState || !Species)
		{
			return Item;
		}

		float DepthMultiplier = 1.f;
		if (const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(World))
		{
			if (const FSpearfishDepthZoneDef* Zone = Ocean->GetDepthZone(Ocean->GetDepthMeters(Fish->GetActorLocation())))
			{
				DepthMultiplier = Zone->ValueMultiplier;
			}
		}

		Item.InstanceId = GameState->AllocateItemId();
		Item.Kind = ESpearfishItemKind::Fish;
		Item.DefId = Species->Id;
		Item.Category = Species->Category;
		Item.Rarity = Species->Rarity;
		Item.LengthCm = Fish->GetLengthCm();
		Item.WeightKg = Fish->GetWeightKg();
		Item.Quality = SpearfishFish::CatchQuality(ShotQuality, FightSeconds, Species->Behavior.FightSeconds);
		Item.Value = SpearfishFish::SaleValue(Item.WeightKg, Species->ValuePerKg, Species->Rarity, Item.Quality, DepthMultiplier);
		Item.SlotCost = SpearfishInventory::SlotCostForLength(Item.LengthCm);
		Item.DayCaught = GameState->GetDayCycle() ? GameState->GetDayCycle()->GetDay() : 1;
		return Item;
	}

	FSpearfishItem MakeLootItem(UWorld* World, FName LootId, float Quality)
	{
		FSpearfishItem Item;
		ASpearfishGameState* GameState = World ? World->GetGameState<ASpearfishGameState>() : nullptr;
		const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(World);
		const FSpearfishLootDef* Def = Registry ? Registry->FindLoot(LootId) : nullptr;
		if (!GameState || !Def)
		{
			return Item;
		}
		Item.InstanceId = GameState->AllocateItemId();
		Item.Kind = ESpearfishItemKind::Loot;
		Item.DefId = LootId;
		Item.Rarity = Def->Rarity;
		Item.WeightKg = Def->WeightKg;
		Item.Quality = Quality;
		Item.Value = SpearfishEconomy::LootValue(Def->BaseValue, Quality);
		Item.SlotCost = 1;
		Item.DayCaught = GameState->GetDayCycle() ? GameState->GetDayCycle()->GetDay() : 1;
		return Item;
	}

	void RecordCatch(ASpearfishCharacter* Diver, const FSpearfishItem& Item)
	{
		UWorld* World = Diver ? Diver->GetWorld() : nullptr;
		ASpearfishGameState* GameState = World ? World->GetGameState<ASpearfishGameState>() : nullptr;
		const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(World);
		if (!GameState || !Registry)
		{
			return;
		}

		FSpearfishCatchRecordResult Record;
		if (USpearfishJournalComponent* Journal = GameState->GetJournal())
		{
			Record = Journal->RecordCatch(Item, Item.DayCaught);
		}

		FSpearfishDayStats& Stats = GameState->EditTodayStats();
		++Stats.FishCaught;
		Stats.NewSpecies += Record.bNewSpecies ? 1 : 0;
		if (ASpearfishPlayerState* PlayerState = Diver->GetSpearfishPlayerState())
		{
			PlayerState->AddFishCaught();
		}

		const FText Name = Registry->GetDisplayName(Item.DefId);
		const FText Size = FText::FromString(SpearfishText::Length(Item.LengthCm));
		if (Item.Rarity == ESpearfishRarity::Legendary)
		{
			GameState->BroadcastNotice(FText::Format(LOCTEXT("Legendary", "LEGENDARY CATCH! {0} - {1}!"), Name, Size), ESpearfishNoticeType::Discovery);
		}
		else if (Record.bNewSpecies)
		{
			GameState->BroadcastNotice(FText::Format(LOCTEXT("NewSpecies", "New species caught: {0} ({1}, {2})"), Name, SpearfishText::RarityName(Item.Rarity), Size), ESpearfishNoticeType::Discovery);
		}
		else if (Record.bNewRecord)
		{
			GameState->BroadcastNotice(FText::Format(LOCTEXT("NewRecord", "New size record: {0} at {1}!"), Name, Size), ESpearfishNoticeType::Good);
		}
		else
		{
			Diver->NotifyOwner(FText::Format(LOCTEXT("Bagged", "Bagged: {0}, {1}, {2}"), Name, Size, SpearfishText::QualityName(Item.Quality)), ESpearfishNoticeType::Good);
		}
	}

	bool CollectLoot(ASpearfishCharacter* Diver, FName LootId, float Quality)
	{
		UWorld* World = Diver ? Diver->GetWorld() : nullptr;
		const FSpearfishItem Item = MakeLootItem(World, LootId, Quality);
		if (!Item.IsValid())
		{
			return false;
		}
		const ESpearfishBagResult Result = Diver->GetBag()->TryAdd(Item);
		const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(World);
		const FText Name = Registry ? Registry->GetDisplayName(LootId) : FText::FromName(LootId);
		if (Result != ESpearfishBagResult::Ok)
		{
			Diver->NotifyOwner(FText::Format(LOCTEXT("LootFull", "No room for the {0} - drop some loot at the boat first."), Name), ESpearfishNoticeType::Warning);
			return false;
		}
		Diver->NotifyOwner(FText::Format(LOCTEXT("LootFound", "Found: {0} (~{1})"), Name, FText::FromString(SpearfishText::Money(Item.Value))), ESpearfishNoticeType::Money);
		return true;
	}

	int32 SellLoot(UWorld* World, const FSpearfishItem& Item)
	{
		ASpearfishGameState* GameState = World ? World->GetGameState<ASpearfishGameState>() : nullptr;
		if (!GameState || Item.Kind != ESpearfishItemKind::Loot)
		{
			return 0;
		}
		GameState->GetProgression()->AddMoney(Item.Value);
		GameState->EditTodayStats().LootValue += Item.Value;
		if (GameState->GetJournal()->RecordLoot(Item.DefId))
		{
			const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(World);
			GameState->BroadcastNotice(FText::Format(LOCTEXT("NewLoot", "New find for the journal: {0}"),
				Registry ? Registry->GetDisplayName(Item.DefId) : FText::FromName(Item.DefId)), ESpearfishNoticeType::Discovery);
		}
		return Item.Value;
	}

	int32 DepositAtBoat(ASpearfishCharacter* Diver, ASpearfishBoat* Boat)
	{
		UWorld* World = Diver ? Diver->GetWorld() : nullptr;
		USpearfishCatchStorageComponent* Cooler = Boat ? Boat->GetCooler() : nullptr;
		if (!World || !Cooler)
		{
			return 0;
		}

		TArray<FSpearfishItem> Fish;
		int32 LootCoins = 0;
		for (const FSpearfishItem& Item : Diver->GetBag()->TakeAll())
		{
			if (Item.Kind == ESpearfishItemKind::Fish)
			{
				Fish.Add(Item);
			}
			else
			{
				LootCoins += SellLoot(World, Item);
			}
		}

		// A towed, exhausted fish on the line is landed straight into the cooler.
		if (USpeargunComponent* Speargun = Diver->GetSpeargun())
		{
			if (ASpearfishFish* Towed = Speargun->DetachFishForLanding())
			{
				const FSpearfishItem Landed = MakeFishItem(World, Towed, 0.5f, 30.f);
				Towed->OnCaught();
				if (Landed.IsValid())
				{
					Fish.Add(Landed);
					RecordCatch(Diver, Landed);
				}
			}
		}

		const int32 Delivered = Cooler->Deposit(Fish);
		if (Delivered > 0 || LootCoins > 0)
		{
			if (ASpearfishGameState* GameState = World->GetGameState<ASpearfishGameState>())
			{
				GameState->BroadcastNotice(LootCoins > 0
					? FText::Format(LOCTEXT("DepositLoot", "{0} fish into the cooler, loot sold for {1}."), FText::AsNumber(Delivered), FText::FromString(SpearfishText::Money(LootCoins)))
					: FText::Format(LOCTEXT("Deposit", "{0} fish into the cooler."), FText::AsNumber(Delivered)), ESpearfishNoticeType::Good);
			}
		}
		return Delivered;
	}
}

#undef LOCTEXT_NAMESPACE
