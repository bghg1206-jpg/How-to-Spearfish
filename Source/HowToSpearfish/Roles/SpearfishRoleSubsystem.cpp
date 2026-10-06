#include "Roles/SpearfishRoleSubsystem.h"

#include "Core/SpearfishGameState.h"
#include "Core/SpearfishPlayerState.h"
#include "Engine/World.h"
#include "HowToSpearfish.h"

void USpearfishRoleSubsystem::Configure(bool bInSolo, const TArray<FSpearfishRoleSeat>& SavedSeats)
{
	bSolo = bInSolo;
	Seats = SavedSeats;
	for (FSpearfishRoleSeat& Seat : Seats)
	{
		Seat.bConnected = false;
	}
	PlayersByKey.Reset();
}

FName USpearfishRoleSubsystem::MakePlayerKey(const ASpearfishPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return NAME_None;
	}
	const FUniqueNetIdRepl& UniqueId = PlayerState->GetUniqueId();
	if (UniqueId.IsValid())
	{
		return FName(*UniqueId->ToString());
	}
	// No online identity (offline PIE): fall back to the player id, stable for this session.
	return FName(*FString::Printf(TEXT("Player_%d"), PlayerState->GetPlayerId()));
}

FSpearfishRoleSeat* USpearfishRoleSubsystem::FindSeat(FName PlayerKey)
{
	return Seats.FindByPredicate([PlayerKey](const FSpearfishRoleSeat& Seat) { return Seat.PlayerKey == PlayerKey; });
}

int32 USpearfishRoleSubsystem::NextFreeSeatIndex() const
{
	int32 Index = 0;
	while (Seats.ContainsByPredicate([Index](const FSpearfishRoleSeat& Seat) { return Seat.SeatIndex == Index && Seat.bConnected; }))
	{
		++Index;
	}
	return Index;
}

ESpearfishRole USpearfishRoleSubsystem::RegisterPlayer(ASpearfishPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return ESpearfishRole::None;
	}

	const FName Key = MakePlayerKey(PlayerState);
	PlayersByKey.Add(Key, PlayerState);

	FSpearfishRoleSeat* Seat = FindSeat(Key);
	if (!Seat)
	{
		// Reuse a disconnected seat slot index only for the same key; new players get the lowest free index.
		FSpearfishRoleSeat NewSeat;
		NewSeat.PlayerKey = Key;
		NewSeat.SeatIndex = NextFreeSeatIndex();
		NewSeat.Role = ESpearfishRole::None;
		Seats.Add(NewSeat);
		Seat = &Seats.Last();
	}
	Seat->bConnected = true;

	// Keep the remembered role if nobody else connected holds it; otherwise take the free role.
	const ESpearfishRole Remembered = Seat->Role;
	const bool bRoleTaken = Seats.ContainsByPredicate([&Key, Remembered](const FSpearfishRoleSeat& Other)
	{
		return Other.bConnected && Other.PlayerKey != Key && Other.Role == Remembered;
	});
	if (bSolo)
	{
		Seat->Role = ESpearfishRole::Diver;
	}
	else if (Remembered == ESpearfishRole::None || bRoleTaken)
	{
		Seat->Role = ESpearfishRole::None;
		Seat->Role = SpearfishRoles::RoleForJoiningPlayer(Seats, false);
	}

	PlayerState->SetSeat(Seat->SeatIndex, Key);
	const ESpearfishRole Assigned = Seat->Role;
	ApplyRoles();
	UE_LOG(LogSpearfish, Log, TEXT("Role: %s joined seat %d as %s"), *Key.ToString(), PlayerState->GetSeatIndex(), *SpearfishText::RoleName(Assigned).ToString());
	return Assigned;
}

void USpearfishRoleSubsystem::UnregisterPlayer(ASpearfishPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}
	const FName Key = PlayerState->GetPlayerKey().IsNone() ? MakePlayerKey(PlayerState) : PlayerState->GetPlayerKey();
	if (FSpearfishRoleSeat* Seat = FindSeat(Key))
	{
		Seat->bConnected = false;
	}
	PlayersByKey.Remove(Key);

	if (!bSolo)
	{
		SpearfishRoles::HandlePartnerLeft(Seats);
	}
	ApplyRoles();
}

void USpearfishRoleSubsystem::ResolveNewDay()
{
	SpearfishRoles::ResolveNextDay(Seats, bSolo);
	ApplyRoles();
}

bool USpearfishRoleSubsystem::RequestSwitchToDiver(ASpearfishPlayerState* PlayerState)
{
	if (!PlayerState || bSolo)
	{
		return false;
	}
	const bool bDiverPresent = Seats.ContainsByPredicate([](const FSpearfishRoleSeat& Seat)
	{
		return Seat.bConnected && Seat.Role == ESpearfishRole::Diver;
	});
	FSpearfishRoleSeat* Seat = FindSeat(PlayerState->GetPlayerKey());
	if (bDiverPresent || !Seat)
	{
		return false;
	}
	Seat->Role = ESpearfishRole::Diver;
	ApplyRoles();
	return true;
}

bool USpearfishRoleSubsystem::NeedsAutoChef() const
{
	return SpearfishRoles::NeedsAutoChef(Seats, bSolo);
}

void USpearfishRoleSubsystem::ApplyRoles()
{
	for (const FSpearfishRoleSeat& Seat : Seats)
	{
		if (!Seat.bConnected)
		{
			continue;
		}
		if (const TWeakObjectPtr<ASpearfishPlayerState>* Found = PlayersByKey.Find(Seat.PlayerKey))
		{
			if (ASpearfishPlayerState* PlayerState = Found->Get())
			{
				PlayerState->SetRole(Seat.Role);
			}
		}
	}

	if (ASpearfishGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASpearfishGameState>() : nullptr)
	{
		GameState->SetAutoChefActive(NeedsAutoChef());
	}

	if (!SpearfishRoles::IsValidAssignment(Seats, bSolo))
	{
		UE_LOG(LogSpearfish, Warning, TEXT("Role assignment is currently incomplete (waiting for players)."));
	}
}
