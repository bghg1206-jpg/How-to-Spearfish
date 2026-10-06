#include "Rules/DayRules.h"

namespace SpearfishDay
{
	ESpearfishDayPhase PhaseForHour(float Hour, const FSpearfishDaySchedule& Schedule)
	{
		if (Hour < Schedule.ServiceStartHour)
		{
			return ESpearfishDayPhase::Morning;
		}
		if (Hour < Schedule.LastSeatingHour)
		{
			return ESpearfishDayPhase::Service;
		}
		if (Hour < Schedule.ClosingHour)
		{
			return ESpearfishDayPhase::Closing;
		}
		return ESpearfishDayPhase::Night;
	}

	bool AcceptsNewGuests(float Hour, const FSpearfishDaySchedule& Schedule)
	{
		return PhaseForHour(Hour, Schedule) == ESpearfishDayPhase::Service;
	}

	bool CanSleep(float Hour, const FSpearfishDaySchedule& Schedule)
	{
		return Hour >= Schedule.SleepAllowedHour;
	}

	bool ShouldPassOut(float Hour, const FSpearfishDaySchedule& Schedule)
	{
		return Hour >= Schedule.PassOutHour;
	}

	float Advance(float Hour, float RealDeltaSeconds, const FSpearfishDaySchedule& Schedule)
	{
		if (RealDeltaSeconds <= 0.f || Schedule.RealSecondsPerGameHour <= 0.f)
		{
			return Hour;
		}
		const float Scale = Hour >= Schedule.ClosingHour ? FMath::Max(Schedule.NightTimeScale, 0.f) : 1.f;
		const float Next = Hour + RealDeltaSeconds * Scale / Schedule.RealSecondsPerGameHour;
		return FMath::Min(Next, Schedule.PassOutHour);
	}

	bool EveryoneReady(const TArray<bool>& ReadyFlags)
	{
		if (ReadyFlags.Num() == 0)
		{
			return false;
		}
		for (const bool bReady : ReadyFlags)
		{
			if (!bReady)
			{
				return false;
			}
		}
		return true;
	}

	void ToClock(float Hour, int32& OutHour24, int32& OutMinute)
	{
		const int32 TotalMinutes = FMath::FloorToInt(FMath::Max(Hour, 0.f) * 60.f);
		OutHour24 = (TotalMinutes / 60) % 24;
		OutMinute = TotalMinutes % 60;
	}
}
