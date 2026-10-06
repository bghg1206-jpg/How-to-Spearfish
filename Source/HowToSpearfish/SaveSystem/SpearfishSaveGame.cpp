#include "SaveSystem/SpearfishSaveGame.h"

#include "Core/SpearfishSettings.h"
#include "Data/SpearfishDataRegistry.h"
#include "HowToSpearfish.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"

FString USpearfishSaveSubsystem::SlotName(bool bSolo)
{
	return bSolo ? TEXT("Spearfish_Solo_0") : TEXT("Spearfish_Coop_0");
}

bool USpearfishSaveSubsystem::HasSave(bool bSolo) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName(bSolo), 0);
}

FSpearfishCampaignData USpearfishSaveSubsystem::MakeNewCampaign() const
{
	FSpearfishCampaignData Campaign;
	const USpearfishSettings* Settings = USpearfishSettings::Get();
	Campaign.Day = 1;
	Campaign.Money = Settings->StartingMoney;
	Campaign.Reputation = 0.f;
	Campaign.CurrentRegion = Settings->StartingRegion;
	Campaign.Loadout.SetNum(static_cast<int32>(ESpearfishEquipmentSlot::Count));

	if (const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this))
	{
		for (int32 SlotIndex = 0; SlotIndex < Campaign.Loadout.Num(); ++SlotIndex)
		{
			if (const FSpearfishEquipmentDef* Starter = Registry->GetStarterEquipment(static_cast<ESpearfishEquipmentSlot>(SlotIndex)))
			{
				Campaign.OwnedEquipment.AddUnique(Starter->Id);
				Campaign.Loadout[SlotIndex] = Starter->Id;
			}
		}
		for (const TPair<FName, FSpearfishRegionDef>& Pair : Registry->GetAllRegions())
		{
			if (Pair.Value.bStarter)
			{
				Campaign.UnlockedRegions.AddUnique(Pair.Key);
			}
		}
	}
	Campaign.UnlockedRegions.AddUnique(Campaign.CurrentRegion);
	Campaign.bInitialized = true;
	return Campaign;
}

FSpearfishCampaignData USpearfishSaveSubsystem::LoadOrCreate(bool bSolo, bool bForceNew)
{
	if (!bForceNew && HasSave(bSolo))
	{
		if (const USpearfishSaveGame* SaveGame = Cast<USpearfishSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName(bSolo), 0)))
		{
			if (SaveGame->SaveVersion <= USpearfishSaveGame::CurrentVersion && SaveGame->Campaign.bInitialized)
			{
				UE_LOG(LogSpearfish, Log, TEXT("Loaded campaign %s (day %d)"), *SlotName(bSolo), SaveGame->Campaign.Day);
				return SaveGame->Campaign;
			}
			UE_LOG(LogSpearfish, Warning, TEXT("Save %s has unsupported version %d, starting fresh"), *SlotName(bSolo), SaveGame->SaveVersion);
		}
	}
	return MakeNewCampaign();
}

bool USpearfishSaveSubsystem::Save(bool bSolo, const FSpearfishCampaignData& Campaign)
{
	USpearfishSaveGame* SaveGame = Cast<USpearfishSaveGame>(UGameplayStatics::CreateSaveGameObject(USpearfishSaveGame::StaticClass()));
	if (!SaveGame)
	{
		return false;
	}
	SaveGame->Campaign = Campaign;
	SaveGame->SavedAt = FDateTime::Now().ToString();
	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SlotName(bSolo), 0);
	UE_LOG(LogSpearfish, Log, TEXT("Saved campaign %s day %d: %s"), *SlotName(bSolo), Campaign.Day, bSaved ? TEXT("ok") : TEXT("FAILED"));
	return bSaved;
}

void USpearfishSaveSubsystem::DeleteSave(bool bSolo)
{
	UGameplayStatics::DeleteGameInSlot(SlotName(bSolo), 0);
}
