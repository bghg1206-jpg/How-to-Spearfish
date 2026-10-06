#pragma once

#include "CoreMinimal.h"

/** One event a region can roll (built from FSpearfishEventDef by the event director). */
struct FSpearfishEventCandidate
{
	FName Id;
	float Chance = 0.f;
	int32 MinDay = 1;
	/** Events sharing a non-zero group are mutually exclusive on the same day (e.g. one legendary visitor). */
	int32 Group = 0;
};

/**
 * Rare and legendary day events (design: Docs/GAME_DESIGN.md, "Events"). Rolled once per morning, before
 * any fish spawn, so the whole day is consistent and rumors can hint at what is out there.
 */
namespace SpearfishEventRules
{
	/**
	 * Today's events: every candidate whose MinDay has passed rolls its chance independently, in a seeded
	 * shuffled order so no entry is favoured by list position, capped at MaxEvents with at most one event per
	 * group. Deterministic for a given seed.
	 */
	HOWTOSPEARFISH_API TArray<FName> RollDailyEvents(const TArray<FSpearfishEventCandidate>& Candidates, int32 Day, int32 Seed, int32 MaxEvents = 2);
}
