#pragma once

#include "CoreMinimal.h"
#include "Rules/SpearfishRulesTypes.h"

namespace SpearfishEconomy
{
	constexpr float MaxReputation = 5.f;

	/**
	 * Checks an unlock/purchase gate. Order of checks: already owned, prerequisite, day, reputation,
	 * species discovered, money — so the UI always reports the most fundamental blocker first.
	 */
	ESpearfishRequirementResult Check(const FSpearfishUnlockReq& Req, const FSpearfishProgressSnapshot& Progress, FName ItemId);

	float ApplyReputation(float Current, float Delta);

	/** Small fee charged when the diver blacks out and is hauled back aboard. Never goes negative. */
	int32 RescueFee(int32 Money, int32 Fee);

	/** Loot sale price scaled by condition. */
	int32 LootValue(int32 BaseValue, float Quality);

	/** Penalty for passing out instead of going to bed. */
	int32 PassOutFee(int32 Money, int32 Fee);
}
