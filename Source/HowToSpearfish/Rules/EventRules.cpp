#include "Rules/EventRules.h"

namespace SpearfishEventRules
{
	TArray<FName> RollDailyEvents(const TArray<FSpearfishEventCandidate>& Candidates, int32 Day, int32 Seed, int32 MaxEvents)
	{
		TArray<FName> Result;
		if (MaxEvents <= 0)
		{
			return Result;
		}
		FRandomStream Stream(Seed);

		TArray<int32> Order;
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			Order.Add(Index);
		}
		for (int32 Index = Order.Num() - 1; Index > 0; --Index)
		{
			Order.Swap(Index, Stream.RandRange(0, Index));
		}

		TArray<int32> UsedGroups;
		for (const int32 Index : Order)
		{
			const FSpearfishEventCandidate& Candidate = Candidates[Index];
			// Always consume a roll so one entry's outcome never shifts another's.
			const float Roll = Stream.FRand();
			if (Candidate.Id.IsNone() || Day < Candidate.MinDay || Roll >= Candidate.Chance)
			{
				continue;
			}
			if (Candidate.Group != 0 && UsedGroups.Contains(Candidate.Group))
			{
				continue;
			}
			if (Result.Contains(Candidate.Id))
			{
				continue;
			}
			Result.Add(Candidate.Id);
			if (Candidate.Group != 0)
			{
				UsedGroups.Add(Candidate.Group);
			}
			if (Result.Num() >= MaxEvents)
			{
				break;
			}
		}
		return Result;
	}
}
