#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Rules/InventoryRules.h"
#include "SpearfishDiveBagComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FSpearfishBagChanged);

/**
 * The diver's catch bag (fish slots + loot slots + weight limit). Server-authoritative and replicated to
 * everyone so the chef's tablet can show exactly what the diver is carrying ("that one's too small!").
 */
UCLASS(ClassGroup = (Spearfish), meta = (BlueprintSpawnableComponent))
class HOWTOSPEARFISH_API USpearfishDiveBagComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpearfishDiveBagComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Server
	ESpearfishBagResult TryAdd(const FSpearfishItem& Item);
	TArray<FSpearfishItem> TakeAll();
	void SetCapacity(const FSpearfishBagCapacity& Capacity);

	ESpearfishBagResult CanAdd(const FSpearfishItem& Item) const { return SpearfishInventory::CanAdd(Bag, Item); }
	const FSpearfishBag& GetBag() const { return Bag; }
	const TArray<FSpearfishItem>& GetItems() const { return Bag.Items; }
	int32 GetUsedFishSlots() const { return SpearfishInventory::UsedFishSlots(Bag); }
	int32 GetUsedLootSlots() const { return SpearfishInventory::UsedLootSlots(Bag); }
	float GetWeightKg() const { return SpearfishInventory::TotalWeightKg(Bag); }
	float GetSpeedMultiplier() const { return SpearfishInventory::LoadSpeedMultiplier(Bag); }
	bool IsEmpty() const { return Bag.Items.Num() == 0; }

	FSpearfishBagChanged OnBagChanged;

private:
	UFUNCTION()
	void OnRep_Bag();

	UPROPERTY(ReplicatedUsing = OnRep_Bag)
	FSpearfishBag Bag;
};
