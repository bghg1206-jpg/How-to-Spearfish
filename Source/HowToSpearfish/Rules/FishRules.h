#pragma once

#include "CoreMinimal.h"
#include "Rules/SpearfishRulesTypes.h"
#include "FishRules.generated.h"

/** High-level state of a fish's mind; steering is computed per state by the fish simulation. */
UENUM(BlueprintType)
enum class ESpearfishFishMind : uint8
{
	Wander,		// cruising around its home area
	School,		// following its school
	Feed,		// picking at coral or sand
	Alert,		// noticed something, keeps distance
	Flee,		// burst away from a threat, toward cover
	Hide,		// sheltering in cover until calm
	Hunt,		// predator chasing prey
	Attack,		// predator / territorial animal going for a diver
	Lunge,		// ambusher striking from its den
	Drift,		// passive drifter (jellyfish)
	Hooked,		// fighting the harpoon line
	Exhausted	// spent; can be reeled in and bagged
};

/** Movement/temperament archetype from species data. */
UENUM(BlueprintType)
enum class ESpearfishFishArchetype : uint8
{
	Schooler,	// jacks, snappers: move as a group, flee as a group
	Reef,		// parrotfish, hogfish: browse near the bottom, dart into cover
	Ambusher,	// groupers, morays: hold near a den, explode when threatened
	Pelagic,	// tuna, barracuda: open water, fast, strong
	Predator,	// sharks: patrol, hunt fish, investigate struggling catches
	Drifter,	// jellyfish: drift, sting on contact
	Ray			// rays: glide over sand, sting when cornered
};

/** What a fish perceives this think step (filled by the fish simulation). */
struct FSpearfishFishSenses
{
	ESpearfishFishArchetype Archetype = ESpearfishFishArchetype::Reef;
	bool bHooked = false;
	float StaminaFraction = 1.f;
	/** Distance (cm) to the most threatening diver or predator. */
	float ThreatDistance = UE_BIG_NUMBER;
	/** Detection radius for this threat already scaled by threat speed/light (cm). */
	float DetectionRadius = 800.f;
	/** A harpoon impact or a fleeing neighbour nearby. */
	bool bStartled = false;
	bool bHasCover = false;
	bool bInCover = false;
	/** Distance to the nearest catchable prey (predators only). */
	float PreyDistance = UE_BIG_NUMBER;
	float HuntRadius = 0.f;
	/** Diver distance for territorial/aggressive behaviour. */
	float DiverDistance = UE_BIG_NUMBER;
	float AttackRadius = 0.f;
	/** Aggressive predators (provoked, or drawn by a struggling catch). */
	bool bProvoked = false;
	bool bHasSchool = false;
	float TimeInState = 0.f;
	float CalmDownSeconds = 4.f;
};

namespace SpearfishFish
{
	/** Fraction of the stamina pool below which a hooked fish gives up. */
	constexpr float ExhaustedThreshold = 0.02f;

	float WeightForLength(float LengthCm, float WeightCoef);

	/** Rolls a body length biased toward the smaller end (big fish are rarer). */
	float RollLength(FRandomStream& Rng, float MinCm, float MaxCm, float SizeBias = 1.6f);

	/** 0 for the smallest, 1 for the largest specimen of the species. */
	float SizeFraction(float LengthCm, float MinCm, float MaxCm);

	float RarityValueMultiplier(ESpearfishRarity Rarity);

	int32 SaleValue(float WeightKg, float ValuePerKg, ESpearfishRarity Rarity, float Quality, float DepthValueMultiplier);

	/** Quality from shot placement (0..1) and how long the fight dragged on. */
	float CatchQuality(float ShotQuality, float FightSeconds, float ExpectedFightSeconds);

	float MaxStamina(float BaseStamina, float SizeFraction);

	/** Pull force for line tension; exhausted fish barely pull. */
	float PullForce(float BaseStrength, float SizeFraction, float StaminaFraction);

	/** Stamina removed by the harpoon hit itself. Head shots and powerful guns shorten fights. */
	float HitStaminaDamage(float GunPower, float HitZoneMultiplier, float SizeFraction, float MaxStaminaValue);

	/** Fast-moving divers and lights are noticed from further away; slow careful approaches pay off. */
	float DetectionRadius(float BaseRadiusCm, float ThreatSpeedFraction, bool bThreatLightOn, float LightSensitivity);

	/** Picks the next mind state. Pure decision logic; steering lives in the fish simulation. */
	ESpearfishFishMind DecideMind(ESpearfishFishMind Current, const FSpearfishFishSenses& Senses);

	/** Whether a mind state counts as "panicking" (used to propagate fear through schools). */
	bool IsPanicked(ESpearfishFishMind Mind);
}
