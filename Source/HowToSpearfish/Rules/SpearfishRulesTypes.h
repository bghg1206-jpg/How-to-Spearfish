#pragma once

// Shared enums and plain data used by the rules layer (Rules/*) and by gameplay code.
//
// Everything under Rules/ is deliberately engine-light: it only depends on CoreMinimal-level types
// (TArray, FName, FMath, FRandomStream) so it can be compiled and unit-tested natively outside the
// editor (see Tools/NativeCheck). Gameplay actors/components wrap these rules with replication.

#include "CoreMinimal.h"
#include "SpearfishRulesTypes.generated.h"

/** The job a player holds for the current in-game day. Swapped every night in co-op. */
UENUM(BlueprintType)
enum class ESpearfishRole : uint8
{
	None,
	Diver,
	Chef
};

UENUM(BlueprintType)
enum class ESpearfishDayPhase : uint8
{
	Morning,	// prep time, no guests yet
	Service,	// guests arrive and order
	Closing,	// no new guests, finish open tickets
	Night,		// time to go to bed
	Sleeping	// night transition in progress
};

UENUM(BlueprintType)
enum class ESpearfishRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary
};

UENUM(BlueprintType)
enum class ESpearfishItemKind : uint8
{
	Fish,
	Loot
};

UENUM(BlueprintType)
enum class ESpearfishEquipmentSlot : uint8
{
	Speargun,
	Reel,
	Tank,
	Wetsuit,
	Mask,
	Fins,
	Bag,
	Light,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ESpearfishCookStep : uint8
{
	Fillet,
	Chop,
	Season,
	Grill,
	Fry,
	Simmer,
	Plate
};

UENUM(BlueprintType)
enum class ESpearfishOrderState : uint8
{
	Waiting,
	Cooking,
	Ready,
	Served,
	Expired
};

UENUM(BlueprintType)
enum class ESpearfishBagResult : uint8
{
	Ok,
	NoFishSlots,
	NoLootSlots,
	TooHeavy,
	Invalid
};

UENUM(BlueprintType)
enum class ESpearfishRequirementResult : uint8
{
	Ok,
	AlreadyOwned,
	NotEnoughMoney,
	NeedsReputation,
	NeedsDay,
	NeedsSpecies,
	NeedsPrerequisite
};

/** One physical thing a diver can carry: a caught fish or a piece of loot. */
USTRUCT(BlueprintType)
struct FSpearfishItem
{
	GENERATED_BODY()

	/** Unique per campaign, allocated by the server. 0 = invalid. */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 InstanceId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	ESpearfishItemKind Kind = ESpearfishItemKind::Fish;

	/** Species id (fish) or loot id. */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FName DefId;

	/** Recipe-matching family, e.g. "ReefFish" or "Pelagic". */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FName Category;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	ESpearfishRarity Rarity = ESpearfishRarity::Common;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	float LengthCm = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	float WeightKg = 0.f;

	/** 0..1. Clean shots and short fights produce better fish. */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	float Quality = 1.f;

	/** Sale value estimate in coins. */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 Value = 0;

	/** Bag slots this item occupies (fish slots for fish, loot slots for loot). */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	uint8 SlotCost = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 DayCaught = 0;

	bool IsValid() const { return InstanceId != 0 && !DefId.IsNone(); }
};

USTRUCT(BlueprintType)
struct FSpearfishBagCapacity
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bag")
	int32 FishSlots = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bag")
	int32 LootSlots = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bag")
	float MaxWeightKg = 15.f;
};

/** A recipe ingredient line: "2x Bluestripe Snapper, at least 35 cm". */
USTRUCT(BlueprintType)
struct FSpearfishIngredientReq
{
	GENERATED_BODY()

	/** Specific species. When None, Category is used; when both are None any fish matches. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ingredient")
	FName SpeciesId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ingredient")
	FName Category;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ingredient")
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ingredient")
	float MinLengthCm = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ingredient")
	float MinQuality = 0.f;
};

/** Gate for purchases and unlocks. Money alone is never the only requirement for regions. */
USTRUCT(BlueprintType)
struct FSpearfishUnlockReq
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock")
	int32 Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock")
	float MinReputation = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock")
	int32 MinDay = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock")
	int32 MinSpeciesCaught = 0;

	/** Id that must already be owned (previous equipment tier, upgrade or region). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock")
	FName Prerequisite;
};

/** Everything the unlock checks need to know about the team. */
USTRUCT(BlueprintType)
struct FSpearfishProgressSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	int32 Money = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	float Reputation = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	int32 Day = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	int32 SpeciesCaught = 0;

	/** Owned equipment ids, restaurant upgrades and unlocked regions. */
	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	TArray<FName> Owned;
};

namespace SpearfishMath
{
	/** Linear remap with clamping. Float-only to avoid UE5 float/double template deduction issues. */
	inline float MapClamped(float Value, float InMin, float InMax, float OutMin, float OutMax)
	{
		if (FMath::IsNearlyEqual(InMin, InMax))
		{
			return OutMin;
		}
		const float Alpha = FMath::Clamp((Value - InMin) / (InMax - InMin), 0.f, 1.f);
		return OutMin + (OutMax - OutMin) * Alpha;
	}
}
