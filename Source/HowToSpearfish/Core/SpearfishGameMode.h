#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SaveSystem/SpearfishCampaignData.h"
#include "SpearfishGameMode.generated.h"

class ASpearfishBoat;
class ASpearfishCharacter;
class ASpearfishPlayerController;
class ASpearfishRegionBuilder;
class ASpearfishStation;
struct FSpearfishDaySummary;

/**
 * Server-side session director for both solo and co-op.
 *  - Loads the campaign, builds the current region, places the boat and spawns players on deck.
 *  - Runs the day: when every connected player is in a bunk (or it gets too late) the night transition
 *    closes the restaurant, sums up the day, spoils the cooler a little, swaps roles, sails to a new region
 *    if one was plotted, restocks the sea, rolls events and saves.
 *  - Handles rescues after blackouts and the occasional debug command.
 * Options: ?mode=solo|coop and ?new=1 for a fresh campaign. Without ?mode= (a plain boot or the engine's
 * fallback after a disconnect) it hands over to the main menu, except when auto-starting in PIE.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASpearfishGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void StartPlay() override;
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool IsSolo() const { return bSolo; }
	bool IsSessionReady() const { return bSessionReady; }
	ASpearfishBoat* GetBoat() const { return Boat; }
	ASpearfishRegionBuilder* GetRegionBuilder() const { return RegionBuilder; }

	// --- Called by gameplay -------------------------------------------------------------------------
	/** After a blackout: fee, then wake up on the boat. */
	void RescueDiver(ASpearfishCharacter* Character);
	void HandleEnterBed(ASpearfishCharacter* Character, ASpearfishStation* Bed);
	void HandleLeaveBed(ASpearfishCharacter* Character);
	void SetTravelDestination(ASpearfishPlayerController* By, FName RegionId);
	void SaveCampaign();
	void HandleDebugCommand(ASpearfishPlayerController* By, const FString& Command, const FString& Argument);

private:
	void SetupSession();
	void SpawnRegion(FName RegionId);
	void PlaceBoatAtRegionStart();
	void StartDayContent(int32 Day);
	int32 MakeDaySeed(int32 Day) const;
	void TickSleepCheck();
	void BeginNight(bool bPassedOut);
	void FinishNight();
	void BuildSummary(FSpearfishDaySummary& Summary, bool bPassedOut, int32 Spoiled) const;
	void WakePlayers();
	void ChangeRegion(FName RegionId);
	FTransform GetSpawnTransformFor(AController* Controller) const;
	TArray<ASpearfishCharacter*> GetPlayerCharacters() const;

	UPROPERTY(Transient)
	TObjectPtr<ASpearfishBoat> Boat;

	UPROPERTY(Transient)
	TObjectPtr<ASpearfishRegionBuilder> RegionBuilder;

	FSpearfishCampaignData Campaign;
	FName PendingRegion;
	FTimerHandle NightTimer;
	bool bSolo = true;
	bool bRedirectToMenu = false;
	bool bForceNewCampaign = false;
	bool bSessionReady = false;
	bool bNightInProgress = false;
	bool bNightPassedOut = false;
	float SleepCheckTimer = 0.f;
};
