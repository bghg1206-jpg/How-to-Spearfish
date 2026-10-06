#pragma once

#include "CoreMinimal.h"
#include "Rules/RoleRules.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpearfishRoleSubsystem.generated.h"

class ASpearfishPlayerState;

/**
 * Server-side owner of role assignment. Keeps one seat per player key so roles survive disconnects,
 * reconnects and save/load, and delegates every decision to SpearfishRoles (Rules/RoleRules) which is
 * unit tested. After any change it pushes roles to PlayerStates and toggles the auto-chef.
 */
UCLASS()
class HOWTOSPEARFISH_API USpearfishRoleSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void Configure(bool bInSolo, const TArray<FSpearfishRoleSeat>& SavedSeats);

	/** Assigns a seat and a role to a freshly logged-in player. */
	ESpearfishRole RegisterPlayer(ASpearfishPlayerState* PlayerState);

	void UnregisterPlayer(ASpearfishPlayerState* PlayerState);

	/** Nightly swap. */
	void ResolveNewDay();

	/** Escape hatch: a lone co-op player can suit up when nobody else is diving. */
	bool RequestSwitchToDiver(ASpearfishPlayerState* PlayerState);

	bool NeedsAutoChef() const;
	bool IsSolo() const { return bSolo; }
	const TArray<FSpearfishRoleSeat>& GetSeats() const { return Seats; }

	static FName MakePlayerKey(const ASpearfishPlayerState* PlayerState);

private:
	void ApplyRoles();
	FSpearfishRoleSeat* FindSeat(FName PlayerKey);
	int32 NextFreeSeatIndex() const;

	bool bSolo = false;
	TArray<FSpearfishRoleSeat> Seats;
	TMap<FName, TWeakObjectPtr<ASpearfishPlayerState>> PlayersByKey;
};
