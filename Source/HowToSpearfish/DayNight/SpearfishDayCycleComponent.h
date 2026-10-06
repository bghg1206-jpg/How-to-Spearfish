#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Rules/DayRules.h"
#include "SpearfishDayCycleComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FSpearfishPhaseChanged, ESpearfishDayPhase /*NewPhase*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FSpearfishDayStarted, int32 /*Day*/);

/**
 * In-game clock on the GameState. The server owns the authoritative hour and replicates it every couple of
 * seconds; clients advance a local copy at the same rate so the sun moves smoothly. Phase changes are
 * replicated separately and drive gameplay (guest arrivals, sleeping, lighting).
 */
UCLASS(ClassGroup = (Spearfish), meta = (BlueprintSpawnableComponent))
class HOWTOSPEARFISH_API USpearfishDayCycleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpearfishDayCycleComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Server
	void StartDay(int32 NewDay);
	void SetClockRunning(bool bRunning);
	void SetHour(float NewHour);
	void SetSleepingPhase(bool bSleeping);

	int32 GetDay() const { return Day; }
	float GetHour() const { return LocalHour; }
	ESpearfishDayPhase GetPhase() const { return Phase; }
	bool IsClockRunning() const { return bClockRunning; }
	const FSpearfishDaySchedule& GetSchedule() const;
	bool CanSleepNow() const;
	bool AcceptsNewGuests() const;

	/** 0 at midnight, 0.5 at noon. Used by the sky. */
	float GetDayFraction() const;

	FSpearfishPhaseChanged OnPhaseChanged;
	FSpearfishDayStarted OnDayStarted;

private:
	UFUNCTION()
	void OnRep_Phase();

	UFUNCTION()
	void OnRep_ReplicatedHour();

	UFUNCTION()
	void OnRep_Day();

	void UpdatePhase();

	UPROPERTY(ReplicatedUsing = OnRep_Day)
	int32 Day = 1;

	/** Authoritative hour sent to clients every few seconds (and on jumps). */
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedHour)
	float ReplicatedHour = 6.f;

	UPROPERTY(ReplicatedUsing = OnRep_Phase)
	ESpearfishDayPhase Phase = ESpearfishDayPhase::Morning;

	UPROPERTY(Replicated)
	bool bClockRunning = false;

	/** Server: authoritative. Clients: locally simulated, corrected by ReplicatedHour. */
	float LocalHour = 6.f;
	float ReplicationAccumulator = 0.f;
};
