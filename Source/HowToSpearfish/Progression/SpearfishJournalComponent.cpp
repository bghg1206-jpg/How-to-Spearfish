#include "Progression/SpearfishJournalComponent.h"

#include "Net/UnrealNetwork.h"
#include "Rules/SpearfishRulesTypes.h"

USpearfishJournalComponent::USpearfishJournalComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void USpearfishJournalComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USpearfishJournalComponent, Entries);
	DOREPLIFETIME(USpearfishJournalComponent, LootRecords);
}

void USpearfishJournalComponent::LoadFromCampaign(const FSpearfishCampaignData& Campaign)
{
	Entries = Campaign.Journal;
	LootRecords = Campaign.LootRecords;
	OnJournalChanged.Broadcast();
}

void USpearfishJournalComponent::SaveToCampaign(FSpearfishCampaignData& Campaign) const
{
	Campaign.Journal = Entries;
	Campaign.LootRecords = LootRecords;
}

FSpearfishJournalEntry& USpearfishJournalComponent::FindOrAddEntry(FName SpeciesId)
{
	if (FSpearfishJournalEntry* Existing = Entries.FindByPredicate([SpeciesId](const FSpearfishJournalEntry& Entry) { return Entry.SpeciesId == SpeciesId; }))
	{
		return *Existing;
	}
	FSpearfishJournalEntry& Entry = Entries.AddDefaulted_GetRef();
	Entry.SpeciesId = SpeciesId;
	return Entry;
}

bool USpearfishJournalComponent::MarkSighted(FName SpeciesId)
{
	if (SpeciesId.IsNone() || IsSighted(SpeciesId))
	{
		return false;
	}
	FindOrAddEntry(SpeciesId).bSighted = true;
	OnJournalChanged.Broadcast();
	return true;
}

FSpearfishCatchRecordResult USpearfishJournalComponent::RecordCatch(const FSpearfishItem& Fish, int32 Day)
{
	FSpearfishCatchRecordResult Result;
	if (Fish.Kind != ESpearfishItemKind::Fish || Fish.DefId.IsNone())
	{
		return Result;
	}

	FSpearfishJournalEntry& Entry = FindOrAddEntry(Fish.DefId);
	Entry.bSighted = true;
	Result.bNewSpecies = Entry.Caught == 0;
	Result.bNewRecord = !Result.bNewSpecies && Fish.LengthCm > Entry.BestLengthCm;
	if (Entry.Caught == 0)
	{
		Entry.FirstCaughtDay = Day;
	}
	++Entry.Caught;
	Entry.BestLengthCm = FMath::Max(Entry.BestLengthCm, Fish.LengthCm);
	Entry.BestWeightKg = FMath::Max(Entry.BestWeightKg, Fish.WeightKg);
	OnJournalChanged.Broadcast();
	return Result;
}

bool USpearfishJournalComponent::RecordLoot(FName LootId)
{
	if (LootId.IsNone())
	{
		return false;
	}
	if (FSpearfishLootRecord* Existing = LootRecords.FindByPredicate([LootId](const FSpearfishLootRecord& Record) { return Record.LootId == LootId; }))
	{
		++Existing->Found;
		OnJournalChanged.Broadcast();
		return false;
	}
	FSpearfishLootRecord& Record = LootRecords.AddDefaulted_GetRef();
	Record.LootId = LootId;
	Record.Found = 1;
	OnJournalChanged.Broadcast();
	return true;
}

const FSpearfishJournalEntry* USpearfishJournalComponent::FindEntry(FName SpeciesId) const
{
	return Entries.FindByPredicate([SpeciesId](const FSpearfishJournalEntry& Entry) { return Entry.SpeciesId == SpeciesId; });
}

bool USpearfishJournalComponent::IsSighted(FName SpeciesId) const
{
	const FSpearfishJournalEntry* Entry = FindEntry(SpeciesId);
	return Entry && Entry->bSighted;
}

int32 USpearfishJournalComponent::CountCaughtSpecies() const
{
	int32 Count = 0;
	for (const FSpearfishJournalEntry& Entry : Entries)
	{
		Count += Entry.Caught > 0 ? 1 : 0;
	}
	return Count;
}

int32 USpearfishJournalComponent::CountSightedSpecies() const
{
	int32 Count = 0;
	for (const FSpearfishJournalEntry& Entry : Entries)
	{
		Count += Entry.bSighted ? 1 : 0;
	}
	return Count;
}

void USpearfishJournalComponent::OnRep_Journal()
{
	OnJournalChanged.Broadcast();
}
