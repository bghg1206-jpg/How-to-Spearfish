#pragma once

#include "CoreMinimal.h"
#include "Data/SpearfishDefinitions.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SpearfishDataRegistry.generated.h"

/**
 * Loads every content definition once per game instance (server and clients alike).
 * Source priority per category: DataTable assigned in USpearfishSettings, otherwise the JSON file in
 * Content/<JsonDataDirectory>. Cross references are validated on load and reported to the log.
 */
UCLASS()
class HOWTOSPEARFISH_API USpearfishDataRegistry : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static USpearfishDataRegistry* Get(const UObject* WorldContextObject);

	/** Reloads all definitions (console: Spearfish.ReloadData). */
	void Reload();

	const FSpearfishFishSpeciesDef* FindFish(FName Id) const { return Fish.Find(Id); }
	const FSpearfishEquipmentDef* FindEquipment(FName Id) const { return Equipment.Find(Id); }
	const FSpearfishRecipeDef* FindRecipe(FName Id) const { return Recipes.Find(Id); }
	const FSpearfishCustomerDef* FindCustomer(FName Id) const { return Customers.Find(Id); }
	const FSpearfishRegionDef* FindRegion(FName Id) const { return Regions.Find(Id); }
	const FSpearfishLootDef* FindLoot(FName Id) const { return Loot.Find(Id); }
	const FSpearfishRestaurantUpgradeDef* FindUpgrade(FName Id) const { return Upgrades.Find(Id); }
	const FSpearfishEventDef* FindEvent(FName Id) const { return Events.Find(Id); }

	const TMap<FName, FSpearfishFishSpeciesDef>& GetAllFish() const { return Fish; }
	const TMap<FName, FSpearfishEquipmentDef>& GetAllEquipment() const { return Equipment; }
	const TMap<FName, FSpearfishRecipeDef>& GetAllRecipes() const { return Recipes; }
	const TMap<FName, FSpearfishCustomerDef>& GetAllCustomers() const { return Customers; }
	const TMap<FName, FSpearfishRegionDef>& GetAllRegions() const { return Regions; }
	const TMap<FName, FSpearfishLootDef>& GetAllLoot() const { return Loot; }
	const TMap<FName, FSpearfishRestaurantUpgradeDef>& GetAllUpgrades() const { return Upgrades; }
	const TMap<FName, FSpearfishEventDef>& GetAllEvents() const { return Events; }

	/** Equipment for a slot, ordered by tier. */
	TArray<const FSpearfishEquipmentDef*> GetEquipmentForSlot(ESpearfishEquipmentSlot Slot) const;

	const FSpearfishEquipmentDef* GetStarterEquipment(ESpearfishEquipmentSlot Slot) const;

	/** Display name for any id (species, loot, recipe, equipment, upgrade, region). */
	FText GetDisplayName(FName Id) const;

	/** Sorted ids for deterministic iteration (procedural spawning, UI ordering). */
	TArray<FName> GetSortedFishIds() const;

	/** Region the given depth (m) falls into, or nullptr. */
	const FSpearfishDepthZoneDef* FindDepthZone(FName RegionId, float DepthM) const;

	int32 GetProblemCount() const { return ProblemCount; }

private:
	void Validate();

	TMap<FName, FSpearfishFishSpeciesDef> Fish;
	TMap<FName, FSpearfishEquipmentDef> Equipment;
	TMap<FName, FSpearfishRecipeDef> Recipes;
	TMap<FName, FSpearfishCustomerDef> Customers;
	TMap<FName, FSpearfishRegionDef> Regions;
	TMap<FName, FSpearfishLootDef> Loot;
	TMap<FName, FSpearfishRestaurantUpgradeDef> Upgrades;
	TMap<FName, FSpearfishEventDef> Events;

	int32 ProblemCount = 0;
};
