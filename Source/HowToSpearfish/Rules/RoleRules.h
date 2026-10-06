#pragma once

#include "CoreMinimal.h"
#include "Rules/SpearfishRulesTypes.h"
#include "RoleRules.generated.h"

/**
 * Role bookkeeping for one player "seat". Seat 0 is the host. PlayerKey is a stable id (unique net id
 * string) so a reconnecting player and save games can be matched to their seat.
 */
USTRUCT(BlueprintType)
struct FSpearfishRoleSeat
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Roles")
	FName PlayerKey;

	UPROPERTY(BlueprintReadOnly, Category = "Roles")
	int32 SeatIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Roles")
	ESpearfishRole Role = ESpearfishRole::None;

	UPROPERTY(BlueprintReadOnly, Category = "Roles")
	bool bConnected = true;
};

/**
 * Role rules (design: Docs/GAME_DESIGN.md, "Role swap").
 *  - Solo: the player is always the Diver; an automated chef runs the restaurant.
 *  - Co-op: exactly one Diver and one Chef. Every night the roles swap: today's Diver is tomorrow's Chef.
 *  - If a partner disconnects mid-day, the remaining player becomes the Diver and the auto-chef takes the
 *    kitchen, so the game never stalls. A (re)joining player takes whichever role is free.
 */
namespace SpearfishRoles
{
	ESpearfishRole Opposite(ESpearfishRole Role);

	int32 CountConnected(const TArray<FSpearfishRoleSeat>& Seats);

	ESpearfishRole RoleForJoiningPlayer(const TArray<FSpearfishRoleSeat>& Seats, bool bSoloSession);

	/** Applies the nightly swap to all connected seats. Disconnected seats are set to None. */
	void ResolveNextDay(TArray<FSpearfishRoleSeat>& Seats, bool bSoloSession);

	/** The remaining connected player becomes the Diver. */
	void HandlePartnerLeft(TArray<FSpearfishRoleSeat>& Seats);

	bool NeedsAutoChef(const TArray<FSpearfishRoleSeat>& Seats, bool bSoloSession);

	/** Exactly one connected Diver; in co-op with two connected players also exactly one Chef. */
	bool IsValidAssignment(const TArray<FSpearfishRoleSeat>& Seats, bool bSoloSession);
}
