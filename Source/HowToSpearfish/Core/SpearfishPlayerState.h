#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Rules/SpearfishRulesTypes.h"
#include "SpearfishPlayerState.generated.h"

class ASpearfishCharacter;

DECLARE_MULTICAST_DELEGATE_OneParam(FSpearfishRoleChanged, ESpearfishRole /*NewRole*/);

/** Per-player replicated state: today's role, seat, bed status and daily stats. */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ASpearfishPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	ESpearfishRole GetRole() const { return Role; }
	bool IsDiver() const { return Role == ESpearfishRole::Diver; }
	bool IsChef() const { return Role == ESpearfishRole::Chef; }
	int32 GetSeatIndex() const { return SeatIndex; }
	FName GetPlayerKey() const { return PlayerKey; }
	bool IsInBed() const { return bInBed; }
	int32 GetFishCaughtToday() const { return FishCaughtToday; }
	int32 GetDishesCookedToday() const { return DishesCookedToday; }

	ASpearfishCharacter* GetSpearfishCharacter() const;

	// Server
	void SetRole(ESpearfishRole NewRole);
	void SetSeat(int32 NewSeatIndex, FName NewPlayerKey);
	void SetInBed(bool bNewInBed);
	void AddFishCaught() { ++FishCaughtToday; }
	void AddDishCooked() { ++DishesCookedToday; }
	void ResetDailyStats();

	FSpearfishRoleChanged OnRoleChanged;

protected:
	virtual void CopyProperties(APlayerState* PlayerState) override;

private:
	UFUNCTION()
	void OnRep_Role();

	UPROPERTY(ReplicatedUsing = OnRep_Role)
	ESpearfishRole Role = ESpearfishRole::None;

	UPROPERTY(Replicated)
	int32 SeatIndex = INDEX_NONE;

	UPROPERTY(Replicated)
	FName PlayerKey;

	UPROPERTY(Replicated)
	bool bInBed = false;

	UPROPERTY(Replicated)
	int32 FishCaughtToday = 0;

	UPROPERTY(Replicated)
	int32 DishesCookedToday = 0;
};
