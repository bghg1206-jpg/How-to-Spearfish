#include "Rules/CookingRules.h"

namespace SpearfishMinigame
{
	namespace
	{
		float AverageWithMissing(const TArray<float>& Results, int32 Required)
		{
			const int32 Count = FMath::Max(Required, Results.Num());
			if (Count <= 0)
			{
				return 0.f;
			}
			float Sum = 0.f;
			for (const float Result : Results)
			{
				Sum += Result;
			}
			return Sum / static_cast<float>(Count);
		}

		void Finish(FSpearfishMinigameState& State)
		{
			if (State.bFinished)
			{
				return;
			}
			State.bFinished = true;
			State.bHolding = false;

			float Score = 0.f;
			switch (State.Step)
			{
			case ESpearfishCookStep::Fillet:
				Score = AverageWithMissing(State.Results, State.Targets.Num());
				break;
			case ESpearfishCookStep::Chop:
				Score = AverageWithMissing(State.Results, State.Targets.Num()) - 0.05f * static_cast<float>(State.Mistakes);
				break;
			case ESpearfishCookStep::Season:
				Score = AverageWithMissing(State.Results, SeasonSprinkles);
				break;
			case ESpearfishCookStep::Grill:
				Score = AverageWithMissing(State.Results, GrillSides);
				break;
			case ESpearfishCookStep::Fry:
			case ESpearfishCookStep::Simmer:
			{
				const float Scored = FMath::Max(State.TimeLimit - BandGraceSeconds, 0.1f);
				Score = State.Accumulated / Scored * 1.1f;
				break;
			}
			case ESpearfishCookStep::Plate:
			{
				const float Completed = State.Sequence.Num() > 0
					? static_cast<float>(State.Progress) / static_cast<float>(State.Sequence.Num())
					: 0.f;
				Score = Completed * (1.f - 0.12f * static_cast<float>(State.Mistakes));
				if (State.Progress >= State.Sequence.Num() && State.TimeLimit > 0.f)
				{
					Score += 0.15f * FMath::Clamp(1.f - State.Elapsed / State.TimeLimit, 0.f, 1.f);
				}
				break;
			}
			default:
				break;
			}
			State.Score = FMath::Clamp(Score, 0.f, 1.f);
		}

		void RecordAttempt(FSpearfishMinigameState& State, float AttemptScore)
		{
			State.Results.Add(AttemptScore);
			State.LastHitScore = AttemptScore;
			++State.Progress;
		}
	}

	float ScoreTiming(float Error, float PerfectWindow, float GoodWindow)
	{
		const float AbsError = FMath::Abs(Error);
		if (AbsError <= PerfectWindow)
		{
			return 1.f;
		}
		if (AbsError <= GoodWindow && GoodWindow > PerfectWindow)
		{
			return FMath::Lerp(1.f, 0.35f, (AbsError - PerfectWindow) / (GoodWindow - PerfectWindow));
		}
		return 0.f;
	}

	float ScoreZone(float Value, float ZoneMin, float ZoneMax)
	{
		const float Center = 0.5f * (ZoneMin + ZoneMax);
		const float HalfWidth = FMath::Max(0.5f * (ZoneMax - ZoneMin), 0.001f);
		const float Distance = FMath::Abs(Value - Center);
		if (Distance <= HalfWidth)
		{
			return 1.f - 0.35f * (Distance / HalfWidth);
		}
		return FMath::Max(0.f, 0.65f - (Distance - HalfWidth) * 4.f);
	}

	void Start(FSpearfishMinigameState& State, ESpearfishCookStep Step, float Difficulty, int32 Seed, float ZoneBonus)
	{
		State = FSpearfishMinigameState();
		State.Step = Step;
		State.Difficulty = FMath::Clamp(Difficulty, 0.f, 1.f);
		State.Rng.Initialize(Seed);
		const float D = State.Difficulty;
		const float Bonus = FMath::Clamp(ZoneBonus, 0.f, 0.2f);

		switch (Step)
		{
		case ESpearfishCookStep::Fillet:
		{
			State.TimeLimit = 4.6f - 1.0f * D;
			State.Speed = 1.f / State.TimeLimit;
			const int32 Cuts = D > 0.5f ? 4 : 3;
			for (int32 Index = 0; Index < Cuts; ++Index)
			{
				const float Base = FMath::Lerp(0.2f, 0.86f, Cuts > 1 ? static_cast<float>(Index) / static_cast<float>(Cuts - 1) : 0.5f);
				State.Targets.Add(FMath::Clamp(Base + State.Rng.FRandRange(-0.035f, 0.035f), 0.1f, 0.92f));
			}
			break;
		}
		case ESpearfishCookStep::Chop:
		{
			const float Interval = 0.55f - 0.15f * D;
			for (int32 Beat = 0; Beat < 8; ++Beat)
			{
				State.Targets.Add(0.9f + Interval * static_cast<float>(Beat));
			}
			State.TimeLimit = State.Targets.Last() + 0.6f;
			break;
		}
		case ESpearfishCookStep::Season:
		{
			State.Speed = 1.1f + 0.9f * D;
			const float HalfWidth = 0.09f - 0.04f * D + Bonus * 0.5f;
			const float Center = State.Rng.FRandRange(0.25f, 0.75f);
			State.ZoneMin = Center - HalfWidth;
			State.ZoneMax = Center + HalfWidth;
			State.TimeLimit = 7.f;
			break;
		}
		case ESpearfishCookStep::Grill:
		{
			State.Speed = 1.f / (3.4f - 0.8f * D);
			const float HalfWidth = 0.08f - 0.03f * D + Bonus * 0.5f;
			State.ZoneMin = 0.72f - HalfWidth;
			State.ZoneMax = 0.72f + HalfWidth;
			State.TimeLimit = static_cast<float>(GrillSides) / State.Speed + 2.f;
			break;
		}
		case ESpearfishCookStep::Fry:
		{
			const float HalfWidth = 0.1f - 0.03f * D + Bonus * 0.5f;
			State.ZoneMin = 0.55f - HalfWidth;
			State.ZoneMax = 0.55f + HalfWidth;
			State.TimeLimit = 6.f;
			State.HeatRate = 0.5f;
			State.CoolRate = 0.38f;
			State.Disturbance = 0.12f + 0.1f * D;
			State.Cursor = 0.2f;
			State.DisturbancePhase = State.Rng.FRandRange(0.f, 6.28f);
			break;
		}
		case ESpearfishCookStep::Simmer:
		{
			const float HalfWidth = 0.13f - 0.03f * D + Bonus * 0.5f;
			State.ZoneMin = 0.5f - HalfWidth;
			State.ZoneMax = 0.5f + HalfWidth;
			State.TimeLimit = 7.f;
			State.HeatRate = 0.3f;
			State.CoolRate = 0.22f;
			State.Disturbance = 0.08f + 0.06f * D;
			State.Cursor = 0.3f;
			State.DisturbancePhase = State.Rng.FRandRange(0.f, 6.28f);
			break;
		}
		case ESpearfishCookStep::Plate:
		{
			const int32 Length = 5 + FMath::RoundToInt(2.f * D);
			for (int32 Index = 0; Index < Length; ++Index)
			{
				State.Sequence.Add(static_cast<ESpearfishMinigameInput>(static_cast<uint8>(ESpearfishMinigameInput::Up) + State.Rng.RandRange(0, 3)));
			}
			State.TimeLimit = 6.5f - 1.5f * D;
			break;
		}
		default:
			break;
		}
	}

	void Tick(FSpearfishMinigameState& State, float DeltaSeconds)
	{
		if (State.bFinished || DeltaSeconds <= 0.f)
		{
			return;
		}
		State.Elapsed += DeltaSeconds;

		switch (State.Step)
		{
		case ESpearfishCookStep::Fillet:
			State.Cursor += State.Speed * DeltaSeconds;
			while (State.Progress < State.Targets.Num() && State.Cursor > State.Targets[State.Progress] + FilletGoodWindow)
			{
				RecordAttempt(State, 0.f);
			}
			if (State.Cursor >= 1.f || State.Progress >= State.Targets.Num())
			{
				Finish(State);
			}
			break;

		case ESpearfishCookStep::Chop:
			State.Cursor = State.Elapsed;
			while (State.Progress < State.Targets.Num() && State.Elapsed > State.Targets[State.Progress] + ChopGoodWindow)
			{
				RecordAttempt(State, 0.f);
			}
			if (State.Progress >= State.Targets.Num())
			{
				Finish(State);
			}
			break;

		case ESpearfishCookStep::Season:
			// Ping-pong needle.
			State.Cursor += State.Direction * State.Speed * 2.f * DeltaSeconds;
			if (State.Cursor >= 1.f)
			{
				State.Cursor = 2.f - State.Cursor;
				State.Direction = -1.f;
			}
			else if (State.Cursor <= 0.f)
			{
				State.Cursor = -State.Cursor;
				State.Direction = 1.f;
			}
			State.Cursor = FMath::Clamp(State.Cursor, 0.f, 1.f);
			break;

		case ESpearfishCookStep::Grill:
			State.Cursor += State.Speed * DeltaSeconds;
			if (State.Cursor >= 1.f)
			{
				// Burnt side: auto flip with a token score.
				RecordAttempt(State, 0.1f);
				State.Cursor = 0.f;
				if (State.Progress >= GrillSides)
				{
					Finish(State);
				}
			}
			break;

		case ESpearfishCookStep::Fry:
		case ESpearfishCookStep::Simmer:
		{
			const float Drive = State.bHolding ? State.HeatRate : -State.CoolRate;
			const float Wobble = State.Disturbance * FMath::Sin(State.Elapsed * 2.3f + State.DisturbancePhase);
			State.Cursor = FMath::Clamp(State.Cursor + (Drive + Wobble) * DeltaSeconds, 0.f, 1.f);
			if (State.Elapsed > BandGraceSeconds && State.Cursor >= State.ZoneMin && State.Cursor <= State.ZoneMax)
			{
				State.Accumulated += DeltaSeconds;
			}
			break;
		}

		default:
			break;
		}

		if (!State.bFinished && State.Elapsed >= State.TimeLimit)
		{
			Finish(State);
		}
	}

	void Input(FSpearfishMinigameState& State, ESpearfishMinigameInput InInput)
	{
		if (State.bFinished)
		{
			return;
		}

		switch (State.Step)
		{
		case ESpearfishCookStep::Fillet:
			if (InInput == ESpearfishMinigameInput::Press && State.Progress < State.Targets.Num())
			{
				const float Error = FMath::Abs(State.Cursor - State.Targets[State.Progress]);
				RecordAttempt(State, ScoreTiming(Error, FilletPerfectWindow, FilletGoodWindow));
				if (State.Progress >= State.Targets.Num())
				{
					Finish(State);
				}
			}
			break;

		case ESpearfishCookStep::Chop:
			if (InInput == ESpearfishMinigameInput::Press && State.Progress < State.Targets.Num())
			{
				const float Error = FMath::Abs(State.Elapsed - State.Targets[State.Progress]);
				if (Error <= ChopGoodWindow)
				{
					RecordAttempt(State, ScoreTiming(Error, ChopPerfectWindow, ChopGoodWindow));
					if (State.Progress >= State.Targets.Num())
					{
						Finish(State);
					}
				}
				else
				{
					++State.Mistakes;
					State.LastHitScore = 0.f;
				}
			}
			break;

		case ESpearfishCookStep::Season:
			if (InInput == ESpearfishMinigameInput::Press)
			{
				RecordAttempt(State, ScoreZone(State.Cursor, State.ZoneMin, State.ZoneMax));
				if (State.Progress >= SeasonSprinkles)
				{
					Finish(State);
				}
			}
			break;

		case ESpearfishCookStep::Grill:
			if (InInput == ESpearfishMinigameInput::Press)
			{
				RecordAttempt(State, ScoreZone(State.Cursor, State.ZoneMin, State.ZoneMax));
				State.Cursor = 0.f;
				if (State.Progress >= GrillSides)
				{
					Finish(State);
				}
			}
			break;

		case ESpearfishCookStep::Fry:
		case ESpearfishCookStep::Simmer:
			if (InInput == ESpearfishMinigameInput::Press)
			{
				State.bHolding = true;
			}
			else if (InInput == ESpearfishMinigameInput::Release)
			{
				State.bHolding = false;
			}
			break;

		case ESpearfishCookStep::Plate:
			if (InInput != ESpearfishMinigameInput::Press && InInput != ESpearfishMinigameInput::Release
				&& State.Progress < State.Sequence.Num())
			{
				if (State.Sequence[State.Progress] == InInput)
				{
					++State.Progress;
					State.LastHitScore = 1.f;
					if (State.Progress >= State.Sequence.Num())
					{
						Finish(State);
					}
				}
				else
				{
					++State.Mistakes;
					State.LastHitScore = 0.f;
				}
			}
			break;

		default:
			break;
		}
	}

	float AutoStepDuration(ESpearfishCookStep Step)
	{
		switch (Step)
		{
		case ESpearfishCookStep::Fillet: return 5.f;
		case ESpearfishCookStep::Chop: return 4.5f;
		case ESpearfishCookStep::Season: return 3.f;
		case ESpearfishCookStep::Grill: return 7.f;
		case ESpearfishCookStep::Fry: return 6.f;
		case ESpearfishCookStep::Simmer: return 8.f;
		case ESpearfishCookStep::Plate: return 4.f;
		default: return 5.f;
		}
	}
}

namespace SpearfishCooking
{
	float DishQuality(const TArray<float>& StepScores, float IngredientQuality, float FlatBonus)
	{
		float StepAverage = 0.f;
		if (StepScores.Num() > 0)
		{
			for (const float Score : StepScores)
			{
				StepAverage += FMath::Clamp(Score, 0.f, 1.f);
			}
			StepAverage /= static_cast<float>(StepScores.Num());
		}
		return FMath::Clamp(0.7f * StepAverage + 0.3f * FMath::Clamp(IngredientQuality, 0.f, 1.f) + FlatBonus, 0.f, 1.f);
	}

	int32 QualityTier(float Quality)
	{
		if (Quality >= 0.9f)
		{
			return 3;
		}
		if (Quality >= 0.72f)
		{
			return 2;
		}
		if (Quality >= 0.45f)
		{
			return 1;
		}
		return 0;
	}

	float AutoChefStepScore(float Skill, FRandomStream& Rng)
	{
		return FMath::Clamp(Skill + Rng.FRandRange(-0.15f, 0.1f), 0.2f, 1.f);
	}
}
