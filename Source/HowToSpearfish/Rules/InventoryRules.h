#pragma once

#include "CoreMinimal.h"
#include "Rules/SpearfishRulesTypes.h"
#include "InventoryRules.generated.h"

/**
 * The diver's catch bag: separate fish and loot slots plus a weight limit. Big fish cost more slots,
 * and a heavy bag slows the diver, so every dive involves real carry decisions.
 */
USTRUCT(BlueprintType)
struct FSpearfishBag
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Bag")
	TArray<FSpearfishItem> Items;

	UPROPERTY(BlueprintReadOnly, Category = "Bag")
	FSpearfishBagCapacity Capacity;
};

namespace SpearfishInventory
{
	/** Fish slot cost by length: small and medium fish take 1 slot, large 2, giants 3. */
	uint8 SlotCostForLength(float LengthCm);

	int32 UsedFishSlots(const FSpearfishBag& Bag);
	int32 UsedLootSlots(const FSpearfishBag& Bag);
	float TotalWeightKg(const FSpearfishBag& Bag);
	int32 TotalValue(const TArray<FSpearfishItem>& Items);
	bool ContainsInstance(const TArray<FSpearfishItem>& Items, int32 InstanceId);

	ESpearfishBagResult CanAdd(const FSpearfishBag& Bag, const FSpearfishItem& Item);

	/** Adds the item when it fits. Rejects invalid items and duplicate instance ids. */
	ESpearfishBagResult TryAdd(FSpearfishBag& Bag, const FSpearfishItem& Item);

	bool RemoveByInstance(FSpearfishBag& Bag, int32 InstanceId, FSpearfishItem* OutItem = nullptr);

	/** Empties the bag and returns everything that was in it (deposit or loss on death). */
	TArray<FSpearfishItem> TakeAll(FSpearfishBag& Bag);

	/** Swim speed multiplier from carried weight: 1 for an empty bag, (1 - MaxPenalty) when full. */
	float LoadSpeedMultiplier(const FSpearfishBag& Bag, float MaxPenalty = 0.3f);

	/** Shrinking capacity never deletes items; it only blocks new additions. */
	void SetCapacity(FSpearfishBag& Bag, const FSpearfishBagCapacity& NewCapacity);
}
