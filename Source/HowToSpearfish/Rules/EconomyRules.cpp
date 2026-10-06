#include "Rules/EconomyRules.h"

namespace SpearfishEconomy
{
	ESpearfishRequirementResult Check(const FSpearfishUnlockReq& Req, const FSpearfishProgressSnapshot& Progress, FName ItemId)
	{
		if (!ItemId.IsNone() && Progress.Owned.Contains(ItemId))
		{
			return ESpearfishRequirementResult::AlreadyOwned;
		}
		if (!Req.Prerequisite.IsNone() && !Progress.Owned.Contains(Req.Prerequisite))
		{
			return ESpearfishRequirementResult::NeedsPrerequisite;
		}
		if (Progress.Day < Req.MinDay)
		{
			return ESpearfishRequirementResult::NeedsDay;
		}
		if (Progress.Reputation + UE_KINDA_SMALL_NUMBER < Req.MinReputation)
		{
			return ESpearfishRequirementResult::NeedsReputation;
		}
		if (Progress.SpeciesCaught < Req.MinSpeciesCaught)
		{
			return ESpearfishRequirementResult::NeedsSpecies;
		}
		if (Progress.Money < Req.Price)
		{
			return ESpearfishRequirementResult::NotEnoughMoney;
		}
		return ESpearfishRequirementResult::Ok;
	}

	float ApplyReputation(float Current, float Delta)
	{
		return FMath::Clamp(Current + Delta, 0.f, MaxReputation);
	}

	int32 RescueFee(int32 Money, int32 Fee)
	{
		return FMath::Clamp(Fee, 0, FMath::Max(Money, 0));
	}

	int32 LootValue(int32 BaseValue, float Quality)
	{
		return FMath::Max(0, FMath::RoundToInt(static_cast<float>(BaseValue) * FMath::Lerp(0.5f, 1.f, FMath::Clamp(Quality, 0.f, 1.f))));
	}

	int32 PassOutFee(int32 Money, int32 Fee)
	{
		return FMath::Clamp(Fee, 0, FMath::Max(Money, 0));
	}
}
