#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveSystem/SpearfishCampaignData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SpearfishSaveGame.generated.h"

UCLASS()
class HOWTOSPEARFISH_API USpearfishSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY(SaveGame)
	int32 SaveVersion = CurrentVersion;

	UPROPERTY(SaveGame)
	FString SavedAt;

	UPROPERTY(SaveGame)
	FSpearfishCampaignData Campaign;
};

/**
 * Campaign persistence. Solo and co-op campaigns use separate slots; in co-op the host's machine owns the
 * save. Autosaves happen at every night transition (after the role swap) and when the host quits.
 */
UCLASS()
class HOWTOSPEARFISH_API USpearfishSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static FString SlotName(bool bSolo);

	bool HasSave(bool bSolo) const;

	/** Loads the campaign for the mode, or creates a fresh one when missing/forced. */
	FSpearfishCampaignData LoadOrCreate(bool bSolo, bool bForceNew);

	bool Save(bool bSolo, const FSpearfishCampaignData& Campaign);

	void DeleteSave(bool bSolo);

	/** Starter kit, starter region and starting money from data + settings. */
	FSpearfishCampaignData MakeNewCampaign() const;
};
