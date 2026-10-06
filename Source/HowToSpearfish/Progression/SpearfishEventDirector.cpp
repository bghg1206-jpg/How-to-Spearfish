#include "Progression/SpearfishEventDirector.h"

#include "Core/SpearfishGameState.h"
#include "Data/SpearfishDataRegistry.h"
#include "Engine/World.h"
#include "FishAI/SpearfishFish.h"
#include "FishAI/SpearfishFishSubsystem.h"
#include "HowToSpearfish.h"
#include "Rules/EventRules.h"
#include "World/SpearfishRegionBuilder.h"

namespace SpearfishEventDirector
{
	namespace
	{
		bool FindSpot(USpearfishFishSubsystem* Fish, const FSpearfishFishSpeciesDef& Species, FRandomStream& Stream, FVector& OutLocation)
		{
			return Fish->FindHabitatFor(Species, Stream, true, OutLocation);
		}

		/** A region predator to circle the bait ball: a Predator archetype first, then any other pelagic. */
		FName PickPredator(const USpearfishDataRegistry& Registry, const FSpearfishRegionDef& Region, FName BaitSpecies)
		{
			FName Fallback;
			for (const FSpearfishRegionSpawn& Spawn : Region.Spawns)
			{
				const FSpearfishFishSpeciesDef* Species = Registry.FindFish(Spawn.SpeciesId);
				if (!Species || Spawn.SpeciesId == BaitSpecies)
				{
					continue;
				}
				if (Species->Archetype == ESpearfishFishArchetype::Predator)
				{
					return Spawn.SpeciesId;
				}
				if (Fallback.IsNone() && Species->Archetype == ESpearfishFishArchetype::Pelagic)
				{
					Fallback = Spawn.SpeciesId;
				}
			}
			return Fallback;
		}

		void ApplyEvent(UWorld* World, const FSpearfishEventDef& Event, const FSpearfishRegionDef& Region, FRandomStream& Stream, int32 DaySeed,
			ASpearfishRegionBuilder* Builder)
		{
			const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(World);
			USpearfishFishSubsystem* Fish = USpearfishFishSubsystem::Get(World);
			const FSpearfishFishSpeciesDef* Species = Registry && !Event.SpeciesId.IsNone() ? Registry->FindFish(Event.SpeciesId) : nullptr;

			switch (Event.Type)
			{
			case ESpearfishEventType::LegendaryVisitor:
				if (Fish && Species)
				{
					FVector Location;
					if (FindSpot(Fish, *Species, Stream, Location))
					{
						for (int32 Index = 0; Index < FMath::Max(Event.Count, 1); ++Index)
						{
							Fish->SpawnFish(Event.SpeciesId, Location + FVector(Index * 300.0, 0.0, 0.0), INDEX_NONE);
						}
						UE_LOG(LogSpearfish, Log, TEXT("Event %s: legendary %s at %s"), *Event.Id.ToString(), *Event.SpeciesId.ToString(), *Location.ToCompactString());
					}
				}
				break;

			case ESpearfishEventType::TreasureCache:
				if (Builder && !Event.LootId.IsNone())
				{
					Builder->SpawnTreasureChest(Event.LootId, FMath::Max(Event.Count, 1), DaySeed + 17);
				}
				break;

			case ESpearfishEventType::BaitBall:
				if (Fish && Species && Registry)
				{
					FVector Location;
					if (FindSpot(Fish, *Species, Stream, Location))
					{
						Fish->SpawnGroup(Event.SpeciesId, Location, FMath::Max(Event.Count, 1) * 8, true);
						const FName Predator = PickPredator(*Registry, Region, Event.SpeciesId);
						if (!Predator.IsNone())
						{
							Fish->SpawnGroup(Predator, Location + FVector(900.0, 0.0, 0.0), Stream.RandRange(1, 2));
						}
					}
				}
				break;

			case ESpearfishEventType::SpeciesBloom:
				if (Fish && Species)
				{
					for (int32 Group = 0; Group < FMath::Max(Event.Count, 1); ++Group)
					{
						FVector Location;
						if (FindSpot(Fish, *Species, Stream, Location))
						{
							Fish->SpawnGroup(Event.SpeciesId, Location, Stream.RandRange(2, 4));
						}
					}
				}
				break;

			case ESpearfishEventType::CriticVisit:
			default:
				// Handled by the restaurant when it picks guests.
				break;
			}
		}
	}

	TArray<FName> StartDay(UWorld* World, const FSpearfishRegionDef& Region, int32 Day, int32 DaySeed, ASpearfishRegionBuilder* Builder)
	{
		const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(World);
		ASpearfishGameState* GameState = World ? World->GetGameState<ASpearfishGameState>() : nullptr;
		if (!Registry || !GameState)
		{
			return {};
		}

		TArray<FSpearfishEventCandidate> Candidates;
		for (const FName EventId : Region.Events)
		{
			if (const FSpearfishEventDef* Event = Registry->FindEvent(EventId))
			{
				FSpearfishEventCandidate Candidate;
				Candidate.Id = EventId;
				Candidate.Chance = Event->Chance;
				Candidate.MinDay = Event->MinDay;
				Candidate.Group = static_cast<int32>(Event->Type) + 1;
				Candidates.Add(Candidate);
			}
		}

		const TArray<FName> Events = SpearfishEventRules::RollDailyEvents(Candidates, Day, DaySeed ^ 0x5EA5);
		FRandomStream Stream(DaySeed + 4242);
		for (const FName EventId : Events)
		{
			if (const FSpearfishEventDef* Event = Registry->FindEvent(EventId))
			{
				ApplyEvent(World, *Event, Region, Stream, DaySeed, Builder);
			}
		}
		GameState->SetActiveEvents(Events);
		return Events;
	}

	bool ForceEvent(UWorld* World, const FSpearfishRegionDef& Region, FName EventId, int32 Seed, ASpearfishRegionBuilder* Builder)
	{
		const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(World);
		ASpearfishGameState* GameState = World ? World->GetGameState<ASpearfishGameState>() : nullptr;
		const FSpearfishEventDef* Event = Registry ? Registry->FindEvent(EventId) : nullptr;
		if (!Event || !GameState)
		{
			return false;
		}
		FRandomStream Stream(Seed);
		ApplyEvent(World, *Event, Region, Stream, Seed, Builder);
		TArray<FName> Events = GameState->GetActiveEvents();
		Events.AddUnique(EventId);
		GameState->SetActiveEvents(Events);
		return true;
	}
}
