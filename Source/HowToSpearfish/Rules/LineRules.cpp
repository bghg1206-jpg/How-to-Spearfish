#include "Rules/LineRules.h"

namespace SpearfishLine
{
	void Attach(FSpearfishLineState& State, float InitialDistanceCm, const FSpearfishLineSpec& Spec)
	{
		State = FSpearfishLineState();
		State.LengthCm = FMath::Clamp(InitialDistanceCm, Spec.MinLengthCm, Spec.MaxLengthCm);
	}

	void Step(FSpearfishLineState& State, const FSpearfishLineInput& In, const FSpearfishLineSpec& Spec)
	{
		if (State.bSnapped || In.DeltaSeconds <= 0.f)
		{
			return;
		}

		State.bTaut = In.DistanceCm >= State.LengthCm - TautSlackCm;
		const float FishPull = State.bTaut ? FMath::Max(In.PullAwayForce, 0.f) : 0.f;
		const float ReelLoad = (State.bTaut && In.bReeling) ? Spec.ReelForce : 0.f;
		State.Tension = FishPull + ReelLoad;

		if (FishPull > Spec.DragForce && Spec.DragForce > 0.f)
		{
			// Drag slips: the fish takes line.
			const float Payout = FMath::Min(Spec.PayoutSpeedCm, Spec.PayoutSpeedCm * (FishPull - Spec.DragForce) / Spec.DragForce);
			State.LengthCm = FMath::Min(Spec.MaxLengthCm, State.LengthCm + Payout * In.DeltaSeconds);
		}
		else if (In.bReeling)
		{
			// Winding slows down as the fish pulls closer to the drag setting.
			const float LoadFactor = Spec.DragForce > 0.f ? FMath::Clamp(1.f - FishPull / (Spec.DragForce * 1.2f), 0.15f, 1.f) : 1.f;
			// Reeling in slack is quick: it just takes up loose line.
			const float SlackBoost = State.bTaut ? 1.f : 1.8f;
			State.LengthCm = FMath::Max(Spec.MinLengthCm, State.LengthCm - Spec.ReelSpeedCm * LoadFactor * SlackBoost * In.DeltaSeconds);
		}

		const bool bAtMaxLength = State.LengthCm >= Spec.MaxLengthCm - 1.f;
		if (State.Tension > Spec.BreakForce * InstantSnapMultiplier)
		{
			State.bSnapped = true;
		}
		else if (bAtMaxLength && State.Tension > Spec.BreakForce)
		{
			State.OverloadSeconds += In.DeltaSeconds;
			if (State.OverloadSeconds >= Spec.SnapGraceSeconds)
			{
				State.bSnapped = true;
			}
		}
		else
		{
			State.OverloadSeconds = FMath::Max(0.f, State.OverloadSeconds - In.DeltaSeconds * 2.f);
		}
	}

	float StressFraction(const FSpearfishLineState& State, const FSpearfishLineSpec& Spec)
	{
		if (Spec.BreakForce <= 0.f)
		{
			return 0.f;
		}
		const float TensionStress = FMath::Clamp(State.Tension / Spec.BreakForce, 0.f, 1.f);
		const float OverloadStress = Spec.SnapGraceSeconds > 0.f ? FMath::Clamp(State.OverloadSeconds / Spec.SnapGraceSeconds, 0.f, 1.f) : 0.f;
		return FMath::Max(TensionStress * 0.85f, OverloadStress);
	}

	float DiverPullAcceleration(float Tension, float PullScale)
	{
		return FMath::Max(Tension, 0.f) * FMath::Max(PullScale, 0.f);
	}
}
