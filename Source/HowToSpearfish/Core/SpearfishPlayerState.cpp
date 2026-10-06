#include "Core/SpearfishPlayerState.h"

#include "Diving/SpearfishCharacter.h"
#include "Net/UnrealNetwork.h"

ASpearfishPlayerState::ASpearfishPlayerState()
{
	SetNetUpdateFrequency(10.f);
}

void ASpearfishPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpearfishPlayerState, Role);
	DOREPLIFETIME(ASpearfishPlayerState, SeatIndex);
	DOREPLIFETIME(ASpearfishPlayerState, PlayerKey);
	DOREPLIFETIME(ASpearfishPlayerState, bInBed);
	DOREPLIFETIME(ASpearfishPlayerState, FishCaughtToday);
	DOREPLIFETIME(ASpearfishPlayerState, DishesCookedToday);
}

ASpearfishCharacter* ASpearfishPlayerState::GetSpearfishCharacter() const
{
	return GetPawn<ASpearfishCharacter>();
}

void ASpearfishPlayerState::SetRole(ESpearfishRole NewRole)
{
	if (Role == NewRole)
	{
		return;
	}
	Role = NewRole;
	OnRep_Role();
	ForceNetUpdate();
}

void ASpearfishPlayerState::SetSeat(int32 NewSeatIndex, FName NewPlayerKey)
{
	SeatIndex = NewSeatIndex;
	PlayerKey = NewPlayerKey;
}

void ASpearfishPlayerState::SetInBed(bool bNewInBed)
{
	bInBed = bNewInBed;
	ForceNetUpdate();
}

void ASpearfishPlayerState::ResetDailyStats()
{
	FishCaughtToday = 0;
	DishesCookedToday = 0;
}

void ASpearfishPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	if (ASpearfishPlayerState* Target = Cast<ASpearfishPlayerState>(PlayerState))
	{
		Target->Role = Role;
		Target->SeatIndex = SeatIndex;
		Target->PlayerKey = PlayerKey;
	}
}

void ASpearfishPlayerState::OnRep_Role()
{
	OnRoleChanged.Broadcast(Role);
}
