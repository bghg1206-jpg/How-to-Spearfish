#include "Rules/RoleRules.h"

namespace SpearfishRoles
{
	namespace
	{
		/** Connected seat indices (array positions) ordered by SeatIndex. */
		TArray<int32> ConnectedBySeat(const TArray<FSpearfishRoleSeat>& Seats)
		{
			TArray<int32> Result;
			for (int32 Index = 0; Index < Seats.Num(); ++Index)
			{
				if (Seats[Index].bConnected)
				{
					Result.Add(Index);
				}
			}
			Result.Sort([&Seats](const int32 A, const int32 B) { return Seats[A].SeatIndex < Seats[B].SeatIndex; });
			return Result;
		}
	}

	ESpearfishRole Opposite(ESpearfishRole Role)
	{
		switch (Role)
		{
		case ESpearfishRole::Diver: return ESpearfishRole::Chef;
		case ESpearfishRole::Chef: return ESpearfishRole::Diver;
		default: return ESpearfishRole::None;
		}
	}

	int32 CountConnected(const TArray<FSpearfishRoleSeat>& Seats)
	{
		int32 Count = 0;
		for (const FSpearfishRoleSeat& Seat : Seats)
		{
			Count += Seat.bConnected ? 1 : 0;
		}
		return Count;
	}

	ESpearfishRole RoleForJoiningPlayer(const TArray<FSpearfishRoleSeat>& Seats, bool bSoloSession)
	{
		if (bSoloSession)
		{
			return ESpearfishRole::Diver;
		}
		const bool bDiverTaken = Seats.ContainsByPredicate([](const FSpearfishRoleSeat& Seat)
		{
			return Seat.bConnected && Seat.Role == ESpearfishRole::Diver;
		});
		return bDiverTaken ? ESpearfishRole::Chef : ESpearfishRole::Diver;
	}

	void ResolveNextDay(TArray<FSpearfishRoleSeat>& Seats, bool bSoloSession)
	{
		const TArray<int32> Connected = ConnectedBySeat(Seats);

		for (FSpearfishRoleSeat& Seat : Seats)
		{
			if (!Seat.bConnected)
			{
				Seat.Role = ESpearfishRole::None;
			}
		}

		if (Connected.Num() == 0)
		{
			return;
		}

		if (bSoloSession || Connected.Num() == 1)
		{
			Seats[Connected[0]].Role = ESpearfishRole::Diver;
			for (int32 Extra = 1; Extra < Connected.Num(); ++Extra)
			{
				Seats[Connected[Extra]].Role = ESpearfishRole::None;
			}
			return;
		}

		FSpearfishRoleSeat& A = Seats[Connected[0]];
		FSpearfishRoleSeat& B = Seats[Connected[1]];
		const ESpearfishRole RoleA = A.Role;
		const ESpearfishRole RoleB = B.Role;

		if (RoleA == ESpearfishRole::Diver && RoleB != ESpearfishRole::Diver)
		{
			// Today's diver cooks tomorrow.
			A.Role = ESpearfishRole::Chef;
			B.Role = ESpearfishRole::Diver;
		}
		else if (RoleB == ESpearfishRole::Diver && RoleA != ESpearfishRole::Diver)
		{
			A.Role = ESpearfishRole::Diver;
			B.Role = ESpearfishRole::Chef;
		}
		else if (RoleA == ESpearfishRole::Chef && RoleB != ESpearfishRole::Chef)
		{
			// Today's chef dives tomorrow (partner joined during the night with no role yet).
			A.Role = ESpearfishRole::Diver;
			B.Role = ESpearfishRole::Chef;
		}
		else if (RoleB == ESpearfishRole::Chef && RoleA != ESpearfishRole::Chef)
		{
			A.Role = ESpearfishRole::Chef;
			B.Role = ESpearfishRole::Diver;
		}
		else
		{
			// No usable history (both None or both identical): host dives first.
			A.Role = ESpearfishRole::Diver;
			B.Role = ESpearfishRole::Chef;
		}

		// Co-op is capped at two players; anyone extra spectates.
		for (int32 Extra = 2; Extra < Connected.Num(); ++Extra)
		{
			Seats[Connected[Extra]].Role = ESpearfishRole::None;
		}
	}

	void HandlePartnerLeft(TArray<FSpearfishRoleSeat>& Seats)
	{
		for (FSpearfishRoleSeat& Seat : Seats)
		{
			if (!Seat.bConnected)
			{
				Seat.Role = ESpearfishRole::None;
			}
		}
		const TArray<int32> Connected = ConnectedBySeat(Seats);
		if (Connected.Num() == 1)
		{
			Seats[Connected[0]].Role = ESpearfishRole::Diver;
		}
	}

	bool NeedsAutoChef(const TArray<FSpearfishRoleSeat>& Seats, bool bSoloSession)
	{
		if (bSoloSession)
		{
			return true;
		}
		return !Seats.ContainsByPredicate([](const FSpearfishRoleSeat& Seat)
		{
			return Seat.bConnected && Seat.Role == ESpearfishRole::Chef;
		});
	}

	bool IsValidAssignment(const TArray<FSpearfishRoleSeat>& Seats, bool bSoloSession)
	{
		int32 Divers = 0;
		int32 Chefs = 0;
		int32 Connected = 0;
		for (const FSpearfishRoleSeat& Seat : Seats)
		{
			if (!Seat.bConnected)
			{
				continue;
			}
			++Connected;
			Divers += Seat.Role == ESpearfishRole::Diver ? 1 : 0;
			Chefs += Seat.Role == ESpearfishRole::Chef ? 1 : 0;
		}
		if (Connected == 0)
		{
			return true;
		}
		if (bSoloSession || Connected == 1)
		{
			return Divers == 1 && Chefs == 0;
		}
		return Divers == 1 && Chefs == 1;
	}
}
