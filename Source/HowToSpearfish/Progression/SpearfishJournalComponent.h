#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "SaveSystem/SpearfishCampaignData.h"
#include "SpearfishJournalComponent.generated.h"

struct FSpearfishItem;

DECLARE_MULTICAST_DELEGATE(FSpearfishJournalChanged);

struct FSpearfishCatchRecordResult
{
	bool bNewSpecies = false;
	bool bNewRecord = false;
};

/**
 * The crew's shared field journal: species sighted (seen up close) and caught, size records and loot finds.
 * Lives on the GameState so both players see discoveries the moment they happen.
 */
UCLASS(ClassGroup = (Spearfish), meta = (BlueprintSpawnableComponent))
class HOWTOSPEARFISH_API USpearfishJournalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpearfishJournalComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Server
	void LoadFromCampaign(const FSpearfishCampaignData& Campaign);
	void SaveToCampaign(FSpearfishCampaignData& Campaign) const;

	/** Returns true when this is the first sighting of the species. */
	bool MarkSighted(FName SpeciesId);
	FSpearfishCatchRecordResult RecordCatch(const FSpearfishItem& Fish, int32 Day);
	/** Returns true when this loot type was never found before. */
	bool RecordLoot(FName LootId);

	// Queries
	const FSpearfishJournalEntry* FindEntry(FName SpeciesId) const;
	bool IsSighted(FName SpeciesId) const;
	int32 CountCaughtSpecies() const;
	int32 CountSightedSpecies() const;
	const TArray<FSpearfishJournalEntry>& GetEntries() const { return Entries; }
	const TArray<FSpearfishLootRecord>& GetLootRecords() const { return LootRecords; }

	FSpearfishJournalChanged OnJournalChanged;

private:
	UFUNCTION()
	void OnRep_Journal();

	FSpearfishJournalEntry& FindOrAddEntry(FName SpeciesId);

	UPROPERTY(ReplicatedUsing = OnRep_Journal)
	TArray<FSpearfishJournalEntry> Entries;

	UPROPERTY(ReplicatedUsing = OnRep_Journal)
	TArray<FSpearfishLootRecord> LootRecords;
};
