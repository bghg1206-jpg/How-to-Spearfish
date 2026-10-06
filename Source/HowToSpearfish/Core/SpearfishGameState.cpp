#include "Core/SpearfishGameState.h"

#include "Boat/SpearfishBoat.h"
#include "Core/SpearfishPlayerState.h"
#include "DayNight/SpearfishDayCycleComponent.h"
#include "Diving/SpearfishCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Progression/SpearfishJournalComponent.h"
#include "Progression/SpearfishProgressionComponent.h"

ASpearfishGameState::ASpearfishGameState()
{
	DayCycle = CreateDefaultSubobject<USpearfishDayCycleComponent>(TEXT("DayCycle"));
	Progression = CreateDefaultSubobject<USpearfishProgressionComponent>(TEXT("Progression"));
	Journal = CreateDefaultSubobject<USpearfishJournalComponent>(TEXT("Journal"));
}

void ASpearfishGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpearfishGameState, Boat);
	DOREPLIFETIME(ASpearfishGameState, bSoloSession);
	DOREPLIFETIME(ASpearfishGameState, bAutoChefActive);
	DOREPLIFETIME(ASpearfishGameState, CurrentRegion);
	DOREPLIFETIME(ASpearfishGameState, ActiveEvents);
	DOREPLIFETIME(ASpearfishGameState, RevealedRumors);
	DOREPLIFETIME(ASpearfishGameState, TodayStats);
}

ASpearfishPlayerState* ASpearfishGameState::FindPlayerWithRole(ESpearfishRole Role) const
{
	for (APlayerState* PlayerState : PlayerArray)
	{
		ASpearfishPlayerState* SpearfishState = Cast<ASpearfishPlayerState>(PlayerState);
		if (SpearfishState && SpearfishState->GetRole() == Role)
		{
			return SpearfishState;
		}
	}
	return nullptr;
}

ASpearfishCharacter* ASpearfishGameState::GetDiverCharacter() const
{
	const ASpearfishPlayerState* Diver = FindPlayerWithRole(ESpearfishRole::Diver);
	return Diver ? Diver->GetSpearfishCharacter() : nullptr;
}

float ASpearfishGameState::GetServerTime() const
{
	return static_cast<float>(GetServerWorldTimeSeconds());
}

void ASpearfishGameState::SetActiveEvents(const TArray<FName>& Events)
{
	ActiveEvents = Events;
	RevealedRumors.Reset();
}

void ASpearfishGameState::RevealRumor(FName EventId)
{
	if (ActiveEvents.Contains(EventId))
	{
		RevealedRumors.AddUnique(EventId);
	}
}

void ASpearfishGameState::ResetTodayStats(int32 Day, float Reputation)
{
	TodayStats = FSpearfishDayStats();
	TodayStats.Day = Day;
	TodayStats.ReputationStart = Reputation;
	TodayStats.ReputationEnd = Reputation;
}

void ASpearfishGameState::BroadcastNotice(const FText& Text, ESpearfishNoticeType Type)
{
	FSpearfishNotice Notice;
	Notice.Text = Text;
	Notice.Type = Type;
	MulticastNotice(Notice);
}

void ASpearfishGameState::BroadcastQuickMessage(const FSpearfishQuickMessageData& Message)
{
	MulticastQuickMessage(Message);
}

void ASpearfishGameState::MulticastNotice_Implementation(const FSpearfishNotice& Notice)
{
	OnNotice.Broadcast(Notice);
}

void ASpearfishGameState::MulticastQuickMessage_Implementation(const FSpearfishQuickMessageData& Message)
{
	OnQuickMessage.Broadcast(Message);
}
