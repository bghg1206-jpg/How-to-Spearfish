#pragma once

#include "CoreMinimal.h"
#include "Rules/SpearfishRulesTypes.h"
#include "DayRules.generated.h"

/**
 * The working day in game hours. Hours past midnight continue above 24 (26 = 02:00) so the clock is
 * monotonic within one day. A day ends only when every connected player sleeps (or at PassOutHour).
 */
USTRUCT(BlueprintType)
struct FSpearfishDaySchedule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Day")
	float DayStartHour = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Day")
	float ServiceStartHour = 8.f;

	/** Last time new guests arrive. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Day")
	float LastSeatingHour = 18.f;

	/** After this hour it is night: remaining guests leave, beds await. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Day")
	float ClosingHour = 20.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Day")
	float SleepAllowedHour = 17.5f;

	/** Players who are still awake pass out (and wake in bed with a small penalty). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Day")
	float PassOutHour = 26.f;

	/** Real seconds per game hour: 75 s gives a ~18 minute working day. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Day")
	float RealSecondsPerGameHour = 75.f;

	/** Night runs faster so stragglers are not stuck waiting for long. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Day")
	float NightTimeScale = 2.f;
};

namespace SpearfishDay
{
	ESpearfishDayPhase PhaseForHour(float Hour, const FSpearfishDaySchedule& Schedule);

	bool AcceptsNewGuests(float Hour, const FSpearfishDaySchedule& Schedule);

	bool CanSleep(float Hour, const FSpearfishDaySchedule& Schedule);

	bool ShouldPassOut(float Hour, const FSpearfishDaySchedule& Schedule);

	/** Advances the clock by real seconds. Never runs past PassOutHour. */
	float Advance(float Hour, float RealDeltaSeconds, const FSpearfishDaySchedule& Schedule);

	/** True only if at least one flag exists and all are set. */
	bool EveryoneReady(const TArray<bool>& ReadyFlags);

	void ToClock(float Hour, int32& OutHour24, int32& OutMinute);
}
