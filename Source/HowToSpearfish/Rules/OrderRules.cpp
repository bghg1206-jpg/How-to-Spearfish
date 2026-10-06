#include "Rules/OrderRules.h"

namespace SpearfishOrders
{
	namespace
	{
		int32 Specificity(const FSpearfishIngredientReq& Req)
		{
			if (!Req.SpeciesId.IsNone())
			{
				return 0;
			}
			return Req.Category.IsNone() ? 2 : 1;
		}

		/** Requirement indices ordered from most to least specific (stable). */
		TArray<int32> OrderedRequirementIndices(const TArray<FSpearfishIngredientReq>& Reqs)
		{
			TArray<int32> Order;
			for (int32 Pass = 0; Pass < 3; ++Pass)
			{
				for (int32 Index = 0; Index < Reqs.Num(); ++Index)
				{
					if (Specificity(Reqs[Index]) == Pass)
					{
						Order.Add(Index);
					}
				}
			}
			return Order;
		}

		/** Cheapest unused matching item; ties broken by lowest instance id for determinism. */
		int32 PickCheapest(const TArray<FSpearfishItem>& Pool, const FSpearfishIngredientReq& Req, const TArray<bool>& Used)
		{
			int32 Best = INDEX_NONE;
			for (int32 Index = 0; Index < Pool.Num(); ++Index)
			{
				if (Used[Index] || !ItemMatches(Pool[Index], Req))
				{
					continue;
				}
				if (Best == INDEX_NONE
					|| Pool[Index].Value < Pool[Best].Value
					|| (Pool[Index].Value == Pool[Best].Value && Pool[Index].InstanceId < Pool[Best].InstanceId))
				{
					Best = Index;
				}
			}
			return Best;
		}
	}

	bool ItemMatches(const FSpearfishItem& Item, const FSpearfishIngredientReq& Req)
	{
		if (Item.Kind != ESpearfishItemKind::Fish || !Item.IsValid())
		{
			return false;
		}
		if (!Req.SpeciesId.IsNone() && Item.DefId != Req.SpeciesId)
		{
			return false;
		}
		if (Req.SpeciesId.IsNone() && !Req.Category.IsNone() && Item.Category != Req.Category)
		{
			return false;
		}
		return Item.LengthCm + UE_KINDA_SMALL_NUMBER >= Req.MinLengthCm && Item.Quality + UE_KINDA_SMALL_NUMBER >= Req.MinQuality;
	}

	int32 CountMatching(const TArray<FSpearfishItem>& Pool, const FSpearfishIngredientReq& Req)
	{
		int32 Count = 0;
		for (const FSpearfishItem& Item : Pool)
		{
			Count += ItemMatches(Item, Req) ? 1 : 0;
		}
		return Count;
	}

	bool FindIngredients(const TArray<FSpearfishItem>& Pool, const TArray<FSpearfishIngredientReq>& Reqs, TArray<int32>& OutIndices)
	{
		OutIndices.Reset();
		TArray<bool> Used;
		Used.Init(false, Pool.Num());

		for (const int32 ReqIndex : OrderedRequirementIndices(Reqs))
		{
			const FSpearfishIngredientReq& Req = Reqs[ReqIndex];
			for (int32 Copy = 0; Copy < FMath::Max(Req.Count, 0); ++Copy)
			{
				const int32 Picked = PickCheapest(Pool, Req, Used);
				if (Picked == INDEX_NONE)
				{
					OutIndices.Reset();
					return false;
				}
				Used[Picked] = true;
				OutIndices.Add(Picked);
			}
		}
		return true;
	}

	TArray<int32> MissingCounts(const TArray<FSpearfishItem>& Pool, const TArray<FSpearfishIngredientReq>& Reqs)
	{
		TArray<int32> Missing;
		Missing.Init(0, Reqs.Num());
		TArray<bool> Used;
		Used.Init(false, Pool.Num());

		for (const int32 ReqIndex : OrderedRequirementIndices(Reqs))
		{
			const FSpearfishIngredientReq& Req = Reqs[ReqIndex];
			for (int32 Copy = 0; Copy < FMath::Max(Req.Count, 0); ++Copy)
			{
				const int32 Picked = PickCheapest(Pool, Req, Used);
				if (Picked == INDEX_NONE)
				{
					++Missing[ReqIndex];
				}
				else
				{
					Used[Picked] = true;
				}
			}
		}
		return Missing;
	}

	float AverageQuality(const TArray<FSpearfishItem>& Items)
	{
		if (Items.Num() == 0)
		{
			return 0.f;
		}
		float Sum = 0.f;
		for (const FSpearfishItem& Item : Items)
		{
			Sum += Item.Quality;
		}
		return Sum / static_cast<float>(Items.Num());
	}

	float PatienceFraction(float Remaining, float Total)
	{
		return Total > 0.f ? FMath::Clamp(Remaining / Total, 0.f, 1.f) : 0.f;
	}

	float Satisfaction(float DishQuality, float InPatienceFraction, float QualityExpectation, const FSpearfishServiceTuning& Tuning)
	{
		const float QualityScore = FMath::Clamp(0.5f + (DishQuality - QualityExpectation) * 1.5f, 0.f, 1.f);
		const float Weight = FMath::Clamp(Tuning.PatienceWeight, 0.f, 1.f);
		return FMath::Clamp(Weight * FMath::Clamp(InPatienceFraction, 0.f, 1.f) + (1.f - Weight) * QualityScore, 0.f, 1.f);
	}

	FSpearfishPayout ComputePayout(int32 BasePrice, float DishQuality, float InPatienceFraction, float QualityExpectation,
		float TipMultiplier, float ReputationWeight, const FSpearfishServiceTuning& Tuning)
	{
		FSpearfishPayout Payout;
		Payout.BasePrice = FMath::Max(BasePrice, 0);

		const float Quality = FMath::Clamp(DishQuality, 0.f, 1.f);
		const float QualityMultiplier = FMath::Lerp(Tuning.MinQualityMultiplier, Tuning.MaxQualityMultiplier, Quality);
		const int32 QualityPrice = FMath::RoundToInt(static_cast<float>(Payout.BasePrice) * QualityMultiplier);
		Payout.QualityAdjustment = QualityPrice - Payout.BasePrice;

		Payout.Satisfaction = Satisfaction(Quality, InPatienceFraction, QualityExpectation, Tuning);
		const float TipAlpha = FMath::Clamp((Payout.Satisfaction - 0.4f) / 0.6f, 0.f, 1.f);
		Payout.Tip = FMath::RoundToInt(static_cast<float>(Payout.BasePrice) * Tuning.MaxTipFraction * FMath::Max(TipMultiplier, 0.f) * TipAlpha);

		Payout.Total = FMath::Max(0, QualityPrice + Payout.Tip);
		Payout.ReputationDelta = (Payout.Satisfaction - 0.5f) * 2.f * Tuning.ReputationPerServe * FMath::Max(ReputationWeight, 0.f);
		return Payout;
	}

	float WalkoutReputationDelta(float ReputationWeight, const FSpearfishServiceTuning& Tuning)
	{
		return -Tuning.ReputationLossOnWalkout * FMath::Max(ReputationWeight, 0.f);
	}
}
