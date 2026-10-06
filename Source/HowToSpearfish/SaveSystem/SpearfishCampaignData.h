#pragma once

#include "CoreMinimal.h"
#include "Rules/RoleRules.h"
#include "Rules/SpearfishRulesTypes.h"
#include "SpearfishCampaignData.generated.h"

USTRUCT(BlueprintType)
struct FSpearfishJournalEntry
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Journal")
	FName SpeciesId;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Journal")
	bool bSighted = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Journal")
	int32 Caught = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Journal")
	float BestLengthCm = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Journal")
	float BestWeightKg = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Journal")
	int32 FirstCaughtDay = 0;
};

USTRUCT(BlueprintType)
struct FSpearfishLootRecord
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Journal")
	FName LootId;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Journal")
	int32 Found = 0;
};

/** Everything persisted between sessions. The host owns the co-op campaign. */
USTRUCT(BlueprintType)
struct FSpearfishCampaignData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	int32 Day = 1;

	UPROPERTY(SaveGame)
	int32 Money = 0;

	UPROPERTY(SaveGame)
	float Reputation = 0.f;

	UPROPERTY(SaveGame)
	TArray<FName> OwnedEquipment;

	/** Equipped item per ESpearfishEquipmentSlot index. */
	UPROPERTY(SaveGame)
	TArray<FName> Loadout;

	UPROPERTY(SaveGame)
	TArray<FName> OwnedUpgrades;

	UPROPERTY(SaveGame)
	TArray<FName> UnlockedRegions;

	UPROPERTY(SaveGame)
	FName CurrentRegion;

	UPROPERTY(SaveGame)
	TArray<FSpearfishJournalEntry> Journal;

	UPROPERTY(SaveGame)
	TArray<FSpearfishLootRecord> LootRecords;

	/** Fish waiting in the boat's cooler. */
	UPROPERTY(SaveGame)
	TArray<FSpearfishItem> Cooler;

	/** Role history so the swap continues across sessions. */
	UPROPERTY(SaveGame)
	TArray<FSpearfishRoleSeat> Seats;

	UPROPERTY(SaveGame)
	int32 NextItemId = 1;

	UPROPERTY(SaveGame)
	bool bInitialized = false;
};
