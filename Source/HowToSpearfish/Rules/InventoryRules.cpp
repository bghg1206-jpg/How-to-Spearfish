#include "Rules/InventoryRules.h"

namespace SpearfishInventory
{
	uint8 SlotCostForLength(float LengthCm)
	{
		if (LengthCm >= 110.f)
		{
			return 3;
		}
		if (LengthCm >= 65.f)
		{
			return 2;
		}
		return 1;
	}

	int32 UsedFishSlots(const FSpearfishBag& Bag)
	{
		int32 Used = 0;
		for (const FSpearfishItem& Item : Bag.Items)
		{
			if (Item.Kind == ESpearfishItemKind::Fish)
			{
				Used += Item.SlotCost;
			}
		}
		return Used;
	}

	int32 UsedLootSlots(const FSpearfishBag& Bag)
	{
		int32 Used = 0;
		for (const FSpearfishItem& Item : Bag.Items)
		{
			if (Item.Kind == ESpearfishItemKind::Loot)
			{
				Used += Item.SlotCost;
			}
		}
		return Used;
	}

	float TotalWeightKg(const FSpearfishBag& Bag)
	{
		float Weight = 0.f;
		for (const FSpearfishItem& Item : Bag.Items)
		{
			Weight += Item.WeightKg;
		}
		return Weight;
	}

	int32 TotalValue(const TArray<FSpearfishItem>& Items)
	{
		int32 Value = 0;
		for (const FSpearfishItem& Item : Items)
		{
			Value += Item.Value;
		}
		return Value;
	}

	bool ContainsInstance(const TArray<FSpearfishItem>& Items, int32 InstanceId)
	{
		return Items.ContainsByPredicate([InstanceId](const FSpearfishItem& Item) { return Item.InstanceId == InstanceId; });
	}

	ESpearfishBagResult CanAdd(const FSpearfishBag& Bag, const FSpearfishItem& Item)
	{
		if (!Item.IsValid() || ContainsInstance(Bag.Items, Item.InstanceId))
		{
			return ESpearfishBagResult::Invalid;
		}

		if (Item.Kind == ESpearfishItemKind::Fish)
		{
			if (UsedFishSlots(Bag) + Item.SlotCost > Bag.Capacity.FishSlots)
			{
				return ESpearfishBagResult::NoFishSlots;
			}
		}
		else if (UsedLootSlots(Bag) + Item.SlotCost > Bag.Capacity.LootSlots)
		{
			return ESpearfishBagResult::NoLootSlots;
		}

		if (TotalWeightKg(Bag) + Item.WeightKg > Bag.Capacity.MaxWeightKg + UE_KINDA_SMALL_NUMBER)
		{
			return ESpearfishBagResult::TooHeavy;
		}
		return ESpearfishBagResult::Ok;
	}

	ESpearfishBagResult TryAdd(FSpearfishBag& Bag, const FSpearfishItem& Item)
	{
		const ESpearfishBagResult Result = CanAdd(Bag, Item);
		if (Result == ESpearfishBagResult::Ok)
		{
			Bag.Items.Add(Item);
		}
		return Result;
	}

	bool RemoveByInstance(FSpearfishBag& Bag, int32 InstanceId, FSpearfishItem* OutItem)
	{
		const int32 Index = Bag.Items.IndexOfByPredicate([InstanceId](const FSpearfishItem& Item) { return Item.InstanceId == InstanceId; });
		if (Index == INDEX_NONE)
		{
			return false;
		}
		if (OutItem)
		{
			*OutItem = Bag.Items[Index];
		}
		Bag.Items.RemoveAt(Index);
		return true;
	}

	TArray<FSpearfishItem> TakeAll(FSpearfishBag& Bag)
	{
		TArray<FSpearfishItem> Taken = MoveTemp(Bag.Items);
		Bag.Items.Reset();
		return Taken;
	}

	float LoadSpeedMultiplier(const FSpearfishBag& Bag, float MaxPenalty)
	{
		if (Bag.Capacity.MaxWeightKg <= 0.f)
		{
			return 1.f;
		}
		const float Load = FMath::Clamp(TotalWeightKg(Bag) / Bag.Capacity.MaxWeightKg, 0.f, 1.f);
		return 1.f - Load * FMath::Clamp(MaxPenalty, 0.f, 0.9f);
	}

	void SetCapacity(FSpearfishBag& Bag, const FSpearfishBagCapacity& NewCapacity)
	{
		Bag.Capacity = NewCapacity;
	}
}
