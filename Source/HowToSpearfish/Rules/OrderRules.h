#pragma once

#include "CoreMinimal.h"
#include "Rules/SpearfishRulesTypes.h"
#include "OrderRules.generated.h"

USTRUCT(BlueprintType)
struct FSpearfishServiceTuning
{
	GENERATED_BODY()

	/** Price multiplier for a 0-quality dish. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Service")
	float MinQualityMultiplier = 0.6f;

	/** Price multiplier for a perfect dish. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Service")
	float MaxQualityMultiplier = 1.4f;

	/** How much remaining patience contributes to satisfaction (the rest is food quality). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Service")
	float PatienceWeight = 0.5f;

	/** Tip as a fraction of the base price for a delighted guest (before archetype multiplier). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Service")
	float MaxTipFraction = 0.35f;

	/** Max reputation swing (stars) for one served guest of weight 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Service")
	float ReputationPerServe = 0.12f;

	/** Reputation lost (stars) when a guest of weight 1 walks out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Service")
	float ReputationLossOnWalkout = 0.15f;
};

USTRUCT(BlueprintType)
struct FSpearfishPayout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Payout")
	int32 BasePrice = 0;

	/** Positive for great food, negative for poor food. */
	UPROPERTY(BlueprintReadOnly, Category = "Payout")
	int32 QualityAdjustment = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Payout")
	int32 Tip = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Payout")
	int32 Total = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Payout")
	float Satisfaction = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Payout")
	float ReputationDelta = 0.f;
};

namespace SpearfishOrders
{
	bool ItemMatches(const FSpearfishItem& Item, const FSpearfishIngredientReq& Req);

	int32 CountMatching(const TArray<FSpearfishItem>& Pool, const FSpearfishIngredientReq& Req);

	/**
	 * Picks pool indices that satisfy every requirement without reusing an item.
	 * Species-specific lines are resolved before category lines, and the cheapest qualifying fish is used,
	 * so valuable catches are not burned on generic ingredients. Returns false (and empties OutIndices)
	 * when the recipe cannot be completed.
	 */
	bool FindIngredients(const TArray<FSpearfishItem>& Pool, const TArray<FSpearfishIngredientReq>& Reqs, TArray<int32>& OutIndices);

	/** Missing count per requirement line (0 = satisfied), used for "we still need..." messages. */
	TArray<int32> MissingCounts(const TArray<FSpearfishItem>& Pool, const TArray<FSpearfishIngredientReq>& Reqs);

	float AverageQuality(const TArray<FSpearfishItem>& Items);

	float PatienceFraction(float Remaining, float Total);

	float Satisfaction(float DishQuality, float PatienceFraction, float QualityExpectation, const FSpearfishServiceTuning& Tuning);

	FSpearfishPayout ComputePayout(int32 BasePrice, float DishQuality, float PatienceFraction, float QualityExpectation,
		float TipMultiplier, float ReputationWeight, const FSpearfishServiceTuning& Tuning);

	float WalkoutReputationDelta(float ReputationWeight, const FSpearfishServiceTuning& Tuning);
}
