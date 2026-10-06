#pragma once

#include "CoreMinimal.h"
#include "Rules/SpearfishRulesTypes.h"
#include "CookingRules.generated.h"

/**
 * Cooking is a chain of short (4-8 s) skill minigames. Each recipe lists which steps it needs;
 * every step produces a 0..1 score and the scores combine with ingredient quality into dish quality.
 *
 *   Fillet  - press as the knife passes each cut mark (precision timing)
 *   Chop    - press on the beat (rhythm)
 *   Season  - stop the swinging needle inside the zone, twice
 *   Grill   - flip each side while doneness is in the zone (anticipation)
 *   Fry     - hold to heat, release to cool; keep the oil in the band (fast dynamics)
 *   Simmer  - same as Fry with slower, heavier dynamics
 *   Plate   - enter the arrow sequence quickly without mistakes (memory/dexterity)
 */
UENUM(BlueprintType)
enum class ESpearfishMinigameInput : uint8
{
	Press,
	Release,
	Up,
	Down,
	Left,
	Right
};

USTRUCT(BlueprintType)
struct FSpearfishMinigameState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	ESpearfishCookStep Step = ESpearfishCookStep::Fillet;

	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	float Difficulty = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	float Elapsed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	float TimeLimit = 6.f;

	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	bool bFinished = false;

	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	float Score = 0.f;

	/** Moving marker: knife position, needle, doneness or temperature (0..1). For Chop: elapsed seconds. */
	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	float Cursor = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	float ZoneMin = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	float ZoneMax = 0.f;

	/** Fillet cut marks (0..1) or chop beat times (seconds). */
	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	TArray<float> Targets;

	/** Per-attempt scores. */
	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	TArray<float> Results;

	/** Plating arrow sequence. */
	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	TArray<ESpearfishMinigameInput> Sequence;

	/** Next target / sequence index / flips done / sprinkles done. */
	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	int32 Progress = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	int32 Mistakes = 0;

	/** Seconds spent inside the band (Fry / Simmer). */
	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	float Accumulated = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	bool bHolding = false;

	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	float Speed = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	float Direction = 1.f;

	/** Score of the most recent attempt for UI feedback, -1 when none. */
	UPROPERTY(BlueprintReadOnly, Category = "Minigame")
	float LastHitScore = -1.f;

	float HeatRate = 0.f;
	float CoolRate = 0.f;
	float Disturbance = 0.f;
	float DisturbancePhase = 0.f;
	FRandomStream Rng;
};

namespace SpearfishMinigame
{
	/** Timing windows (Fillet in cursor units, Chop in seconds). */
	constexpr float FilletPerfectWindow = 0.02f;
	constexpr float FilletGoodWindow = 0.07f;
	constexpr float ChopPerfectWindow = 0.045f;
	constexpr float ChopGoodWindow = 0.14f;
	constexpr int32 SeasonSprinkles = 2;
	constexpr int32 GrillSides = 2;
	constexpr float BandGraceSeconds = 0.8f;

	/** 1 inside the perfect window, falling to 0.35 at the edge of the good window, 0 beyond. */
	float ScoreTiming(float Error, float PerfectWindow, float GoodWindow);

	/** 1 at the zone centre, 0.65 at its edge, falling quickly outside. */
	float ScoreZone(float Value, float ZoneMin, float ZoneMax);

	/** @param ZoneBonus widens target zones (restaurant upgrades), typically 0..0.1. */
	void Start(FSpearfishMinigameState& State, ESpearfishCookStep Step, float Difficulty, int32 Seed, float ZoneBonus = 0.f);

	void Tick(FSpearfishMinigameState& State, float DeltaSeconds);

	void Input(FSpearfishMinigameState& State, ESpearfishMinigameInput Input);

	/** Seconds a competent automated cook spends on a step (solo auto-chef). */
	float AutoStepDuration(ESpearfishCookStep Step);
}

namespace SpearfishCooking
{
	/** Combines step scores (70%) and ingredient quality (30%) plus flat bonuses into 0..1. */
	float DishQuality(const TArray<float>& StepScores, float IngredientQuality, float FlatBonus = 0.f);

	/** 0 = Poor, 1 = Good, 2 = Great, 3 = Perfect. */
	int32 QualityTier(float Quality);

	/** Score an automated cook achieves on one step at the given skill (0..1). */
	float AutoChefStepScore(float Skill, FRandomStream& Rng);
}
