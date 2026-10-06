#include "DayNight/SpearfishDayCycleComponent.h"

#include "Core/SpearfishSettings.h"
#include "Net/UnrealNetwork.h"

USpearfishDayCycleComponent::USpearfishDayCycleComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void USpearfishDayCycleComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USpearfishDayCycleComponent, Day);
	DOREPLIFETIME(USpearfishDayCycleComponent, ReplicatedHour);
	DOREPLIFETIME(USpearfishDayCycleComponent, Phase);
	DOREPLIFETIME(USpearfishDayCycleComponent, bClockRunning);
}

const FSpearfishDaySchedule& USpearfishDayCycleComponent::GetSchedule() const
{
	return USpearfishSettings::Get()->DaySchedule;
}

void USpearfishDayCycleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bClockRunning || Phase == ESpearfishDayPhase::Sleeping)
	{
		return;
	}

	LocalHour = SpearfishDay::Advance(LocalHour, DeltaTime, GetSchedule());

	if (GetOwnerRole() == ROLE_Authority)
	{
		ReplicationAccumulator += DeltaTime;
		if (ReplicationAccumulator >= 2.f)
		{
			ReplicationAccumulator = 0.f;
			ReplicatedHour = LocalHour;
		}
		UpdatePhase();
	}
}

void USpearfishDayCycleComponent::StartDay(int32 NewDay)
{
	Day = FMath::Max(1, NewDay);
	LocalHour = GetSchedule().DayStartHour;
	ReplicatedHour = LocalHour;
	Phase = ESpearfishDayPhase::Morning;
	bClockRunning = true;
	OnPhaseChanged.Broadcast(Phase);
	OnDayStarted.Broadcast(Day);
}

void USpearfishDayCycleComponent::SetClockRunning(bool bRunning)
{
	bClockRunning = bRunning;
}

void USpearfishDayCycleComponent::SetHour(float NewHour)
{
	LocalHour = FMath::Clamp(NewHour, 0.f, GetSchedule().PassOutHour);
	ReplicatedHour = LocalHour;
	UpdatePhase();
}

void USpearfishDayCycleComponent::SetSleepingPhase(bool bSleeping)
{
	if (bSleeping)
	{
		if (Phase != ESpearfishDayPhase::Sleeping)
		{
			Phase = ESpearfishDayPhase::Sleeping;
			OnPhaseChanged.Broadcast(Phase);
		}
	}
	else if (Phase == ESpearfishDayPhase::Sleeping)
	{
		Phase = SpearfishDay::PhaseForHour(LocalHour, GetSchedule());
		OnPhaseChanged.Broadcast(Phase);
	}
}

bool USpearfishDayCycleComponent::CanSleepNow() const
{
	return SpearfishDay::CanSleep(LocalHour, GetSchedule());
}

bool USpearfishDayCycleComponent::AcceptsNewGuests() const
{
	return Phase == ESpearfishDayPhase::Service;
}

float USpearfishDayCycleComponent::GetDayFraction() const
{
	return FMath::Fmod(LocalHour, 24.f) / 24.f;
}

void USpearfishDayCycleComponent::UpdatePhase()
{
	// While the night transition runs the phase is pinned to Sleeping.
	if (Phase == ESpearfishDayPhase::Sleeping)
	{
		return;
	}
	const ESpearfishDayPhase NewPhase = SpearfishDay::PhaseForHour(LocalHour, GetSchedule());
	if (NewPhase != Phase)
	{
		Phase = NewPhase;
		OnPhaseChanged.Broadcast(Phase);
	}
}

void USpearfishDayCycleComponent::OnRep_Phase()
{
	OnPhaseChanged.Broadcast(Phase);
}

void USpearfishDayCycleComponent::OnRep_ReplicatedHour()
{
	// Snap if we drifted noticeably (or the server jumped the clock); otherwise keep the smooth local clock.
	if (FMath::Abs(ReplicatedHour - LocalHour) > 0.05f)
	{
		LocalHour = ReplicatedHour;
	}
}

void USpearfishDayCycleComponent::OnRep_Day()
{
	OnDayStarted.Broadcast(Day);
}
