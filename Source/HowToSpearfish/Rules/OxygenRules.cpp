#include "Rules/OxygenRules.h"

namespace SpearfishOxygen
{
	bool IsAtSurface(float DepthMeters, const FSpearfishOxygenTuning& Tuning)
	{
		return DepthMeters <= Tuning.SurfaceDepthMeters;
	}

	float ConsumptionPerSecond(const FSpearfishBreathInput& In, const FSpearfishOxygenTuning& Tuning)
	{
		if (IsAtSurface(In.DepthMeters, Tuning))
		{
			return 0.f;
		}

		const float Depth = FMath::Max(In.DepthMeters, 0.f);
		float Rate = Tuning.BaseConsumption * (1.f + Depth * Tuning.DepthFactorPerMeter);

		const float Exertion = In.bSprinting
			? Tuning.SprintExertion
			: Tuning.SwimExertion * FMath::Clamp(In.SpeedFraction, 0.f, 1.f);
		Rate *= 1.f + Exertion;

		const float OverDepth = Depth - In.SafeDepthMeters;
		if (OverDepth > 0.f)
		{
			Rate *= 1.f + OverDepth * Tuning.OverDepthPenaltyPerMeter;
		}

		Rate *= FMath::Max(In.ZoneMultiplier, 0.f);

		if (In.bStung)
		{
			Rate *= Tuning.StungMultiplier;
		}
		return Rate;
	}

	bool Step(FSpearfishBreathState& State, const FSpearfishBreathInput& In, const FSpearfishOxygenTuning& Tuning, float DeltaSeconds)
	{
		if (State.bBlackedOut || DeltaSeconds <= 0.f)
		{
			return false;
		}

		if (IsAtSurface(In.DepthMeters, Tuning))
		{
			// Breathing real air: no tank use and the breath-hold reserve recovers immediately.
			State.BreathHoldRemaining = Tuning.BreathHoldSeconds;
			State.bOutOfAir = State.Air <= 0.f;
			return false;
		}

		const float Rate = ConsumptionPerSecond(In, Tuning);
		State.Air = FMath::Max(0.f, State.Air - Rate * DeltaSeconds);

		if (State.Air > 0.f)
		{
			State.bOutOfAir = false;
			return false;
		}

		State.bOutOfAir = true;
		State.BreathHoldRemaining = FMath::Max(0.f, State.BreathHoldRemaining - DeltaSeconds);
		if (State.BreathHoldRemaining <= 0.f)
		{
			State.bBlackedOut = true;
			return true;
		}
		return false;
	}

	void ApplyShock(FSpearfishBreathState& State, float Amount)
	{
		if (Amount <= 0.f || State.bBlackedOut)
		{
			return;
		}
		State.Air = FMath::Max(0.f, State.Air - Amount);
	}

	void Refill(FSpearfishBreathState& State, float NewMaxAir, const FSpearfishOxygenTuning& Tuning)
	{
		State.MaxAir = FMath::Max(NewMaxAir, 1.f);
		State.Air = State.MaxAir;
		State.BreathHoldRemaining = Tuning.BreathHoldSeconds;
		State.bOutOfAir = false;
		State.bBlackedOut = false;
	}

	float AirFraction(const FSpearfishBreathState& State)
	{
		return State.MaxAir > 0.f ? FMath::Clamp(State.Air / State.MaxAir, 0.f, 1.f) : 0.f;
	}

	float SecondsRemaining(const FSpearfishBreathState& State, float ConsumptionRate)
	{
		if (ConsumptionRate <= UE_KINDA_SMALL_NUMBER)
		{
			return UE_BIG_NUMBER;
		}
		return State.Air / ConsumptionRate;
	}
}
