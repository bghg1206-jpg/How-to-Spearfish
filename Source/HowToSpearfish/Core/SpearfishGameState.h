#pragma once

#include "Core/SpearfishGameTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SpearfishGameState.generated.h"

class ASpearfishBoat;
class ASpearfishCharacter;
class ASpearfishPlayerState;
class USpearfishDayCycleComponent;
class USpearfishJournalComponent;
class USpearfishProgressionComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FSpearfishQuickMessageEvent, const FSpearfishQuickMessageData& /*Message*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FSpearfishNoticeEvent, const FSpearfishNotice& /*Notice*/);

/**
 * Shared team state, replicated to everyone: clock (DayCycle), economy (Progression), discoveries
 * (Journal), the boat, active rare events and team-wide radio/notifications.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ASpearfishGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	USpearfishDayCycleComponent* GetDayCycle() const { return DayCycle; }
	USpearfishProgressionComponent* GetProgression() const { return Progression; }
	USpearfishJournalComponent* GetJournal() const { return Journal; }

	ASpearfishBoat* GetBoat() const { return Boat; }
	bool IsSoloSession() const { return bSoloSession; }
	bool IsAutoChefActive() const { return bAutoChefActive; }
	FName GetCurrentRegion() const { return CurrentRegion; }
	const TArray<FName>& GetActiveEvents() const { return ActiveEvents; }
	const TArray<FName>& GetRevealedRumors() const { return RevealedRumors; }
	const FSpearfishDayStats& GetTodayStats() const { return TodayStats; }

	ASpearfishPlayerState* FindPlayerWithRole(ESpearfishRole Role) const;
	ASpearfishCharacter* GetDiverCharacter() const;

	/** Server time in seconds as float (UI/patience timers). */
	float GetServerTime() const;

	// --- Server API -------------------------------------------------------------------------
	void SetSession(bool bInSolo) { bSoloSession = bInSolo; }
	void SetAutoChefActive(bool bActive) { bAutoChefActive = bActive; }
	void SetBoat(ASpearfishBoat* InBoat) { Boat = InBoat; }
	void SetCurrentRegion(FName RegionId) { CurrentRegion = RegionId; }
	void SetActiveEvents(const TArray<FName>& Events);
	void RevealRumor(FName EventId);
	FSpearfishDayStats& EditTodayStats() { return TodayStats; }
	void ResetTodayStats(int32 Day, float Reputation);

	int32 AllocateItemId() { return NextItemId++; }
	int32 GetNextItemId() const { return NextItemId; }
	void SetNextItemId(int32 Value) { NextItemId = FMath::Max(Value, 1); }

	void BroadcastNotice(const FText& Text, ESpearfishNoticeType Type);
	void BroadcastQuickMessage(const FSpearfishQuickMessageData& Message);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastNotice(const FSpearfishNotice& Notice);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastQuickMessage(const FSpearfishQuickMessageData& Message);

	FSpearfishQuickMessageEvent OnQuickMessage;
	FSpearfishNoticeEvent OnNotice;

private:
	UPROPERTY(VisibleAnywhere, Category = "Spearfish")
	TObjectPtr<USpearfishDayCycleComponent> DayCycle;

	UPROPERTY(VisibleAnywhere, Category = "Spearfish")
	TObjectPtr<USpearfishProgressionComponent> Progression;

	UPROPERTY(VisibleAnywhere, Category = "Spearfish")
	TObjectPtr<USpearfishJournalComponent> Journal;

	UPROPERTY(Replicated)
	TObjectPtr<ASpearfishBoat> Boat;

	UPROPERTY(Replicated)
	bool bSoloSession = false;

	UPROPERTY(Replicated)
	bool bAutoChefActive = false;

	UPROPERTY(Replicated)
	FName CurrentRegion;

	UPROPERTY(Replicated)
	TArray<FName> ActiveEvents;

	/** Rumors a gossiping guest has shared today (shown on the chef's tablet). */
	UPROPERTY(Replicated)
	TArray<FName> RevealedRumors;

	UPROPERTY(Replicated)
	FSpearfishDayStats TodayStats;

	int32 NextItemId = 1;
};
