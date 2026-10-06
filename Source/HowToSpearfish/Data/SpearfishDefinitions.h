#pragma once

// Data-driven content definitions. Every struct derives from FTableRowBase so designers can import the
// JSON files in Content/Data/Source into DataTables (row name = "Name" field) and point
// USpearfishSettings at them. Without DataTables, USpearfishDataRegistry reads the JSON directly.

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Rules/FishRules.h"
#include "Rules/LineRules.h"
#include "Rules/SpearfishRulesTypes.h"
#include "World/SpearfishTerrain.h"
#include "SpearfishDefinitions.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UWorld;

/** Procedural body plan used to build placeholder fish when no art mesh is assigned. */
UENUM(BlueprintType)
enum class ESpearfishBodyPlan : uint8
{
	Torpedo,	// jacks, tuna, barracuda
	Oval,		// snappers, parrotfish, hogfish
	Bulky,		// groupers, sea bass
	Spiny,		// lionfish
	Eel,		// morays, wolf eels
	Disc,		// rays
	Bell,		// jellyfish
	Shark		// sharks
};

UENUM(BlueprintType)
enum class ESpearfishActivity : uint8
{
	Any,
	Day,
	Night
};

UENUM(BlueprintType)
enum class ESpearfishLootCategory : uint8
{
	Shell,
	Pearl,
	Treasure,
	Artifact,
	Salvage,
	Curiosity
};

UENUM(BlueprintType)
enum class ESpearfishEventType : uint8
{
	LegendaryVisitor,	// a legendary fish spawns somewhere special; customers gossip about it
	TreasureCache,		// extra treasure in a cave or wreck
	CriticVisit,		// a food critic is guaranteed to show up
	BaitBall,			// huge school with predators circling
	SpeciesBloom		// a rare species appears in numbers
};

// ------------------------------------------------------------------------------------------- Fish
USTRUCT(BlueprintType)
struct FSpearfishFishBehavior
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	float CruiseSpeedCm = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	float BurstSpeedCm = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	float TurnRateDeg = 160.f;

	/** How far away a slow, unlit diver is noticed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	float DetectionRadiusCm = 700.f;

	/** >0 scared of lights, <0 attracted/indifferent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	float LightSensitivity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	int32 SchoolSizeMin = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	int32 SchoolSizeMax = 1;

	/** Line pull at full stamina for a mid-size specimen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	float Strength = 180.f;

	/** Stamina pool; drained by line tension and bursts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	float Stamina = 6.f;

	/** Typical fight length in seconds (catch quality reference). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	float FightSeconds = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	bool bSeeksCover = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	float CalmDownSeconds = 5.f;

	/** Air knocked out of a diver by a bite or sting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	float AttackShock = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	float AttackRadiusCm = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	float HuntRadiusCm = 0.f;

	/** Recipe categories this predator hunts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	TArray<FName> PreyCategories;

	/** Predators drawn to fish struggling on a line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	bool bAttractedByStruggle = false;

	/** Curious fish (barracuda) approach slow divers instead of fleeing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	bool bCurious = false;
};

USTRUCT(BlueprintType)
struct FSpearfishFishVisual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	ESpearfishBodyPlan BodyPlan = ESpearfishBodyPlan::Oval;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor PrimaryColor = FLinearColor(0.6f, 0.6f, 0.65f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor SecondaryColor = FLinearColor(0.9f, 0.9f, 0.9f);

	/** Optional art mesh (forward = +X, length 100 cm at scale 1). Falls back to a procedural body. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** Height / length ratio of the procedural body. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	float BodyDepth = 0.32f;

	/** Width / length ratio of the procedural body. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	float BodyWidth = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	float TailBeatHz = 2.2f;
};

USTRUCT(BlueprintType)
struct FSpearfishFishSpeciesDef : public FTableRowBase
{
	GENERATED_BODY()

	/** Filled from the row name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fish")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fish")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fish", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fish")
	ESpearfishRarity Rarity = ESpearfishRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fish")
	ESpearfishFishArchetype Archetype = ESpearfishFishArchetype::Reef;

	/** Recipe family ("ReefFish", "Pelagic", "Bottom") or "Hazard". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fish")
	FName Category;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fish")
	bool bCatchable = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habitat")
	float MinDepthM = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habitat")
	float MaxDepthM = 20.f;

	/** Biome tags where this species lives: Reef, Sand, Seagrass, Cave, Wreck, OpenWater, Kelp, Wall. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habitat")
	TArray<FName> Biomes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habitat")
	ESpearfishActivity Activity = ESpearfishActivity::Any;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size")
	float MinLengthCm = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size")
	float MaxLengthCm = 50.f;

	/** Weight (kg) = WeightCoef * (length in metres)^3. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Size")
	float WeightCoef = 13.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
	float ValuePerKg = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	FSpearfishFishBehavior Behavior;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FSpearfishFishVisual Visual;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Journal", meta = (MultiLine = true))
	FText FunFact;
};

// -------------------------------------------------------------------------------------- Equipment
/** Flat stat block; each slot reads the fields relevant to it. */
USTRUCT(BlueprintType)
struct FSpearfishEquipmentStats
{
	GENERATED_BODY()

	// Speargun
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speargun")
	float GunPower = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speargun")
	float GunRangeCm = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speargun")
	float ReloadSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speargun")
	float ShaftSpeedCm = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speargun")
	float AimSpreadDeg = 0.f;

	// Reel / line
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reel")
	float LineLengthCm = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reel")
	float ReelSpeedCm = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reel")
	float DragForce = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reel")
	float BreakForce = 0.f;

	// Tank
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank")
	float AirCapacity = 0.f;

	// Wetsuit
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wetsuit")
	float SafeDepthM = 0.f;

	/** 0..1 reduction of sting/bite shocks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wetsuit")
	float StingResist = 0.f;

	// Mask
	/** 0..1 reduction of underwater fog: better masks see further. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mask")
	float VisibilityBonus = 0.f;

	// Fins
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fins")
	float SwimSpeedCm = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fins")
	float SprintMultiplier = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fins")
	float AccelerationCm = 0.f;

	// Bag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bag")
	int32 FishSlots = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bag")
	int32 LootSlots = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bag")
	float MaxWeightKg = 0.f;

	// Light
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light")
	float LightIntensity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light")
	float LightRangeCm = 0.f;
};

USTRUCT(BlueprintType)
struct FSpearfishEquipmentDef : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	ESpearfishEquipmentSlot Slot = ESpearfishEquipmentSlot::Speargun;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	int32 Tier = 0;

	/** Owned from day one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	bool bStarter = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FSpearfishUnlockReq Unlock;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FSpearfishEquipmentStats Stats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor Color = FLinearColor(0.2f, 0.2f, 0.2f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UStaticMesh> Mesh;
};

// --------------------------------------------------------------------------------------- Recipes
USTRUCT(BlueprintType)
struct FSpearfishRecipeDef : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Recipe")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TArray<FSpearfishIngredientReq> Ingredients;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TArray<ESpearfishCookStep> Steps;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	int32 BasePrice = 30;

	/** 0..1 minigame difficulty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	float Difficulty = 0.3f;

	/** Customers pick dishes matching their favourite tags more often. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	TArray<FName> Tags;

	/** Recipe appears on the menu once the restaurant reaches this reputation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	float MinReputation = 0.f;

	/** Recipe appears on the menu once this restaurant upgrade is owned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FName RequiredUpgrade;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor PlateColor = FLinearColor(0.9f, 0.6f, 0.3f);
};

// ------------------------------------------------------------------------------------- Customers
USTRUCT(BlueprintType)
struct FSpearfishCustomerDef : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Customer")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Customer")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Customer")
	float PatienceSeconds = 160.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Customer")
	float TipMultiplier = 1.f;

	/** Dish quality the guest expects (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Customer")
	float QualityExpectation = 0.5f;

	/** Reputation impact multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Customer")
	float ReputationWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Customer")
	float SpawnWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Customer")
	float MinReputation = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Customer")
	TArray<FName> FavoriteTags;

	/** Dishes ordered at once (families order for everyone). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Customer")
	int32 Dishes = 1;

	/** Locals gossip about rare sightings and treasure. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Customer")
	bool bShareRumors = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor OutfitColor = FLinearColor(0.8f, 0.3f, 0.3f);
};

// --------------------------------------------------------------------------------------- Regions
USTRUCT(BlueprintType)
struct FSpearfishDepthZoneDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Depth")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Depth")
	float MinDepthM = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Depth")
	float MaxDepthM = 10.f;

	/** Extra multiplier on top of the per-metre consumption curve. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Depth")
	float AirMultiplier = 1.f;

	/** Sale value multiplier for fish caught here: deep fish are worth more. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Depth")
	float ValueMultiplier = 1.f;
};

USTRUCT(BlueprintType)
struct FSpearfishRegionSpawn
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	FName SpeciesId;

	/** Number of groups (schools count as one group). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	int32 Groups = 3;
};

USTRUCT(BlueprintType)
struct FSpearfishWeightedId
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weighted")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weighted")
	float Weight = 1.f;
};

USTRUCT(BlueprintType)
struct FSpearfishRegionPalette
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor ShallowWater = FLinearColor(0.1f, 0.65f, 0.7f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor DeepWater = FLinearColor(0.01f, 0.08f, 0.2f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor Sand = FLinearColor(0.85f, 0.78f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor Rock = FLinearColor(0.35f, 0.33f, 0.3f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor Vegetation = FLinearColor(0.2f, 0.5f, 0.25f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	TArray<FLinearColor> Coral;

	/** Underwater fog density at the surface; increases with depth. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	float UnderwaterFog = 0.018f;
};

USTRUCT(BlueprintType)
struct FSpearfishRegionDef : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FText HubName;

	/** Unlocked from the start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	bool bStarter = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FSpearfishUnlockReq Unlock;

	/** Optional authored map. When empty the region is generated procedurally. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TSoftObjectPtr<UWorld> Map;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	int32 Seed = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FSpearfishTerrainParams Terrain;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FSpearfishRegionPalette Palette;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TArray<FSpearfishDepthZoneDef> DepthZones;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TArray<FSpearfishRegionSpawn> Spawns;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TArray<FSpearfishWeightedId> Loot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TArray<FName> Events;
};

// ------------------------------------------------------------------------------------------ Loot
USTRUCT(BlueprintType)
struct FSpearfishLootDef : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	ESpearfishLootCategory Category = ESpearfishLootCategory::Shell;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	ESpearfishRarity Rarity = ESpearfishRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 BaseValue = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	float WeightKg = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	float MinDepthM = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	float MaxDepthM = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TArray<FName> Biomes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor Color = FLinearColor(0.9f, 0.85f, 0.7f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UStaticMesh> Mesh;
};

// -------------------------------------------------------------------------- Restaurant upgrades
USTRUCT(BlueprintType)
struct FSpearfishRestaurantUpgradeDef : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FSpearfishUnlockReq Unlock;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	int32 ExtraSeats = 0;

	/** Widens cooking minigame zones. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	float CookZoneBonus = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	float DishQualityBonus = 0.f;

	/** Fraction added to guest patience. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	float PatienceBonus = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	float AutoChefSkillBonus = 0.f;

	/** Tablet shows the diver's air and depth. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	bool bUnlocksTelemetry = false;

	/** Tablet shows the diver's bearing and distance from the boat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	bool bUnlocksDiverLocator = false;
};

// ---------------------------------------------------------------------------------------- Events
USTRUCT(BlueprintType)
struct FSpearfishEventDef : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Event")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	ESpearfishEventType Type = ESpearfishEventType::SpeciesBloom;

	/** Daily chance (0..1) once MinDay is reached. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	float Chance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	int32 MinDay = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	FName SpeciesId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	FName LootId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	int32 Count = 1;

	/** What gossiping guests tell the chef. Only the chef's tablet shows rumors. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event", meta = (MultiLine = true))
	FText Rumor;
};
