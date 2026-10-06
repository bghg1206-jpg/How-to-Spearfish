#pragma once

#include "CoreMinimal.h"

class ASpearfishRegionBuilder;
class UWorld;
struct FSpearfishRegionDef;

/**
 * Rare and legendary day events. Each morning the server rolls the current region's event list
 * (SpearfishEventRules), applies the world side of each event (a legendary fish at a wreck or cave, a
 * treasure chest, a bait ball with predators, a species bloom), and publishes the list on the game state.
 * The restaurant reacts to CriticVisit itself, and gossiping guests reveal rumors for active events.
 * Call after the regular fish population has spawned.
 */
namespace SpearfishEventDirector
{
	HOWTOSPEARFISH_API TArray<FName> StartDay(UWorld* World, const FSpearfishRegionDef& Region, int32 Day, int32 DaySeed, ASpearfishRegionBuilder* Builder);

	/** Development: applies one event now (ignoring chance and day) and adds it to today's list. */
	HOWTOSPEARFISH_API bool ForceEvent(UWorld* World, const FSpearfishRegionDef& Region, FName EventId, int32 Seed, ASpearfishRegionBuilder* Builder);
}
