#pragma once

#include "CoreMinimal.h"
#include "OxygenRules.generated.h"

/**
 * Oxygen is the diver's single survival resource. Depth, exertion, pressure beyond the wetsuit rating
 * and hazards (bites, stings) all spend it. One unit is roughly one second of calm breathing at the surface.
 */
USTRUCT(BlueprintType)
struct FSpearfishOxygenTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen")
	float BaseConsumption = 1.0f;

	/** Gentle stand-in for Boyle's law: +3.5% consumption per metre. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen")
	float DepthFactorPerMeter = 0.035f;

	/** Extra multiplier at full cruise speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen")
	float SwimExertion = 0.3f;

	/** Extra multiplier while power-kicking. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen")
	float SprintExertion = 0.9f;

	/** Extra multiplier per metre beyond the wetsuit's safe depth. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen")
	float OverDepthPenaltyPerMeter = 0.08f;

	/** Multiplier while stung / panicking. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen")
	float StungMultiplier = 1.6f;

	/** Seconds of breath-hold after the tank runs dry before the diver blacks out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen")
	float BreathHoldSeconds = 8.f;

	/** Depth (m) treated as "head above water": breathing air, no tank consumption. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen")
	float SurfaceDepthMeters = 0.6f;
};

USTRUCT(BlueprintType)
struct FSpearfishBreathState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Oxygen")
	float Air = 300.f;

	UPROPERTY(BlueprintReadOnly, Category = "Oxygen")
	float MaxAir = 300.f;

	UPROPERTY(BlueprintReadOnly, Category = "Oxygen")
	float BreathHoldRemaining = 8.f;

	UPROPERTY(BlueprintReadOnly, Category = "Oxygen")
	bool bOutOfAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Oxygen")
	bool bBlackedOut = false;
};

struct FSpearfishBreathInput
{
	float DepthMeters = 0.f;
	/** 0..1 of the diver's max swim speed. */
	float SpeedFraction = 0.f;
	bool bSprinting = false;
	float SafeDepthMeters = 20.f;
	/** Depth-zone multiplier from region data. */
	float ZoneMultiplier = 1.f;
	bool bStung = false;
};

namespace SpearfishOxygen
{
	bool IsAtSurface(float DepthMeters, const FSpearfishOxygenTuning& Tuning);

	float ConsumptionPerSecond(const FSpearfishBreathInput& In, const FSpearfishOxygenTuning& Tuning);

	/** Advances the breath state. Returns true exactly on the tick the diver blacks out. */
	bool Step(FSpearfishBreathState& State, const FSpearfishBreathInput& In, const FSpearfishOxygenTuning& Tuning, float DeltaSeconds);

	/** Bites, stings and similar shocks knock air out of the diver. */
	void ApplyShock(FSpearfishBreathState& State, float Amount);

	/** Full refill (back on the boat / respawn), optionally with a new tank size. */
	void Refill(FSpearfishBreathState& State, float NewMaxAir, const FSpearfishOxygenTuning& Tuning);

	float AirFraction(const FSpearfishBreathState& State);

	/** Seconds of air left at the given consumption rate (for UI hints and the auto-chef radio). */
	float SecondsRemaining(const FSpearfishBreathState& State, float ConsumptionRate);
}
