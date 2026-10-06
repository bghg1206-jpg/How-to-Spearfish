#include "Rules/FishRules.h"

namespace SpearfishFish
{
	float WeightForLength(float LengthCm, float WeightCoef)
	{
		const float Meters = FMath::Max(LengthCm, 0.f) / 100.f;
		return FMath::Max(WeightCoef, 0.f) * Meters * Meters * Meters;
	}

	float RollLength(FRandomStream& Rng, float MinCm, float MaxCm, float SizeBias)
	{
		const float Low = FMath::Min(MinCm, MaxCm);
		const float High = FMath::Max(MinCm, MaxCm);
		const float Roll = FMath::Pow(Rng.FRand(), FMath::Max(SizeBias, 0.1f));
		return Low + (High - Low) * Roll;
	}

	float SizeFraction(float LengthCm, float MinCm, float MaxCm)
	{
		return SpearfishMath::MapClamped(LengthCm, MinCm, MaxCm, 0.f, 1.f);
	}

	float RarityValueMultiplier(ESpearfishRarity Rarity)
	{
		switch (Rarity)
		{
		case ESpearfishRarity::Common: return 1.f;
		case ESpearfishRarity::Uncommon: return 1.3f;
		case ESpearfishRarity::Rare: return 1.8f;
		case ESpearfishRarity::Epic: return 2.5f;
		case ESpearfishRarity::Legendary: return 4.f;
		default: return 1.f;
		}
	}

	int32 SaleValue(float WeightKg, float ValuePerKg, ESpearfishRarity Rarity, float Quality, float DepthValueMultiplier)
	{
		const float QualityMultiplier = FMath::Lerp(0.5f, 1.15f, FMath::Clamp(Quality, 0.f, 1.f));
		const float Value = FMath::Max(WeightKg, 0.f) * FMath::Max(ValuePerKg, 0.f) * RarityValueMultiplier(Rarity)
			* QualityMultiplier * FMath::Max(DepthValueMultiplier, 0.f);
		// Even a tiny fish is worth a coin or two.
		return FMath::Max(1, FMath::RoundToInt(Value));
	}

	float CatchQuality(float ShotQuality, float FightSeconds, float ExpectedFightSeconds)
	{
		const float Shot = FMath::Clamp(ShotQuality, 0.f, 1.f);
		const float Expected = FMath::Max(ExpectedFightSeconds, 1.f);
		const float FightPenalty = FMath::Clamp((FightSeconds - Expected) / (Expected * 3.f), 0.f, 1.f);
		return FMath::Clamp(0.55f + 0.3f * Shot + 0.15f * (1.f - FightPenalty) - 0.2f * FightPenalty, 0.f, 1.f);
	}

	float MaxStamina(float BaseStamina, float InSizeFraction)
	{
		return FMath::Max(BaseStamina, 0.1f) * FMath::Lerp(0.7f, 1.6f, FMath::Clamp(InSizeFraction, 0.f, 1.f));
	}

	float PullForce(float BaseStrength, float InSizeFraction, float StaminaFraction)
	{
		const float Size = FMath::Lerp(0.6f, 1.5f, FMath::Clamp(InSizeFraction, 0.f, 1.f));
		const float Effort = StaminaFraction <= ExhaustedThreshold ? 0.08f : FMath::Lerp(0.45f, 1.f, FMath::Clamp(StaminaFraction, 0.f, 1.f));
		return FMath::Max(BaseStrength, 0.f) * Size * Effort;
	}

	float HitStaminaDamage(float GunPower, float HitZoneMultiplier, float InSizeFraction, float MaxStaminaValue)
	{
		const float Resistance = FMath::Lerp(1.f, 2.2f, FMath::Clamp(InSizeFraction, 0.f, 1.f));
		const float Fraction = FMath::Clamp(0.22f * FMath::Max(GunPower, 0.f) * FMath::Max(HitZoneMultiplier, 0.f) / Resistance, 0.f, 0.85f);
		return Fraction * FMath::Max(MaxStaminaValue, 0.f);
	}

	float DetectionRadius(float BaseRadiusCm, float ThreatSpeedFraction, bool bThreatLightOn, float LightSensitivity)
	{
		const float SpeedFactor = FMath::Lerp(0.45f, 1.25f, FMath::Clamp(ThreatSpeedFraction, 0.f, 1.f));
		const float LightFactor = bThreatLightOn ? FMath::Max(1.f + LightSensitivity, 0.2f) : 1.f;
		return FMath::Max(BaseRadiusCm, 0.f) * SpeedFactor * LightFactor;
	}

	bool IsPanicked(ESpearfishFishMind Mind)
	{
		return Mind == ESpearfishFishMind::Flee || Mind == ESpearfishFishMind::Hooked;
	}

	ESpearfishFishMind DecideMind(ESpearfishFishMind Current, const FSpearfishFishSenses& S)
	{
		// 1. The line overrides everything.
		if (S.bHooked)
		{
			return S.StaminaFraction <= ExhaustedThreshold ? ESpearfishFishMind::Exhausted : ESpearfishFishMind::Hooked;
		}
		if (Current == ESpearfishFishMind::Exhausted)
		{
			// Released while exhausted: recover slowly, then bolt.
			return S.TimeInState > S.CalmDownSeconds ? ESpearfishFishMind::Flee : ESpearfishFishMind::Exhausted;
		}

		// 2. Archetypes with special temperaments.
		if (S.Archetype == ESpearfishFishArchetype::Drifter)
		{
			return ESpearfishFishMind::Drift;
		}

		if (S.Archetype == ESpearfishFishArchetype::Ambusher)
		{
			if (S.DiverDistance < S.AttackRadius)
			{
				return ESpearfishFishMind::Lunge;
			}
			if (S.bStartled || S.ThreatDistance < S.DetectionRadius * 0.5f)
			{
				return S.bHasCover ? ESpearfishFishMind::Hide : ESpearfishFishMind::Flee;
			}
			if (Current == ESpearfishFishMind::Hide && S.TimeInState < S.CalmDownSeconds)
			{
				return ESpearfishFishMind::Hide;
			}
			return ESpearfishFishMind::Wander;
		}

		if (S.Archetype == ESpearfishFishArchetype::Predator)
		{
			if (S.bProvoked && S.DiverDistance < S.AttackRadius)
			{
				return ESpearfishFishMind::Attack;
			}
			if (S.PreyDistance < S.HuntRadius)
			{
				return ESpearfishFishMind::Hunt;
			}
			return ESpearfishFishMind::Wander;
		}

		// 3. Everyone else reacts to threats.
		const bool bTooClose = S.ThreatDistance < S.DetectionRadius * 0.55f;
		if (bTooClose || S.bStartled)
		{
			return ESpearfishFishMind::Flee;
		}

		if (Current == ESpearfishFishMind::Flee)
		{
			if (S.TimeInState < S.CalmDownSeconds * 0.6f)
			{
				return ESpearfishFishMind::Flee;
			}
			return S.bInCover ? ESpearfishFishMind::Hide : ESpearfishFishMind::Alert;
		}

		if (Current == ESpearfishFishMind::Hide && S.TimeInState < S.CalmDownSeconds)
		{
			return ESpearfishFishMind::Hide;
		}

		if (S.ThreatDistance < S.DetectionRadius)
		{
			return ESpearfishFishMind::Alert;
		}

		if (Current == ESpearfishFishMind::Alert && S.TimeInState < S.CalmDownSeconds * 0.5f)
		{
			return ESpearfishFishMind::Alert;
		}

		if (S.bHasSchool && S.Archetype == ESpearfishFishArchetype::Schooler)
		{
			return ESpearfishFishMind::School;
		}

		// Reef fish alternate between feeding and wandering; the simulation decides timing.
		if (Current == ESpearfishFishMind::Feed && S.TimeInState < S.CalmDownSeconds * 1.5f)
		{
			return ESpearfishFishMind::Feed;
		}
		return ESpearfishFishMind::Wander;
	}
}
