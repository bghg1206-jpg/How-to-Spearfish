#pragma once

#include "CoreMinimal.h"
#include "Rules/SpearfishRulesTypes.h"

class ASpearfishBoat;
class ASpearfishCharacter;
class ASpearfishFish;
class UWorld;

/**
 * Server-side catch bookkeeping shared by every path a catch can take (bagging underwater, landing a
 * towed fish at the ladder, picking up loot). Keeps item creation, journal records, stats and team
 * notifications in one place so rewards can never be granted twice by different code paths.
 */
namespace SpearfishCatch
{
	/** Builds the inventory item for a fish (allocates a unique instance id). */
	HOWTOSPEARFISH_API FSpearfishItem MakeFishItem(UWorld* World, const ASpearfishFish* Fish, float ShotQuality, float FightSeconds);

	HOWTOSPEARFISH_API FSpearfishItem MakeLootItem(UWorld* World, FName LootId, float Quality);

	/** Journal, stats and notices after a fish reached the bag or the cooler. */
	HOWTOSPEARFISH_API void RecordCatch(ASpearfishCharacter* Diver, const FSpearfishItem& Item);

	/** Loot pickup into the diver's loot slots. Returns false (with a message) when there is no room. */
	HOWTOSPEARFISH_API bool CollectLoot(ASpearfishCharacter* Diver, FName LootId, float Quality);

	/** Sells one loot item for the team and records it in the journal. Returns coins earned. */
	HOWTOSPEARFISH_API int32 SellLoot(UWorld* World, const FSpearfishItem& Item);

	/**
	 * Diver climbs aboard: bag fish go into the cooler, loot is sold, and an exhausted fish still on the
	 * line is landed straight into the cooler. Returns the number of fish delivered.
	 */
	HOWTOSPEARFISH_API int32 DepositAtBoat(ASpearfishCharacter* Diver, ASpearfishBoat* Boat);
}
