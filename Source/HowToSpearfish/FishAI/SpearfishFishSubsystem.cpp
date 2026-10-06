#include "FishAI/SpearfishFishSubsystem.h"

#include "Core/SpearfishGameState.h"
#include "Core/SpearfishPlayerState.h"
#include "Core/SpearfishSettings.h"
#include "Data/SpearfishDataRegistry.h"
#include "Diving/SpearfishCharacter.h"
#include "Engine/World.h"
#include "FishAI/SpearfishFish.h"
#include "HowToSpearfish.h"
#include "Progression/SpearfishJournalComponent.h"
#include "Rules/SpearfishRulesTypes.h"
#include "World/SpearfishOceanSubsystem.h"

#define LOCTEXT_NAMESPACE "SpearfishFishSubsystem"

void USpearfishFishSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Stream.Initialize(12345);
}

void USpearfishFishSubsystem::Deinitialize()
{
	AllFish.Reset();
	Schools.Reset();
	Super::Deinitialize();
}

TStatId USpearfishFishSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USpearfishFishSubsystem, STATGROUP_Tickables);
}

USpearfishFishSubsystem* USpearfishFishSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<USpearfishFishSubsystem>() : nullptr;
}

float USpearfishFishSubsystem::Now() const
{
	const UWorld* World = GetWorld();
	return World ? static_cast<float>(World->GetTimeSeconds()) : 0.f;
}

void USpearfishFishSubsystem::Tick(float DeltaTime)
{
	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	const float Time = Now();
	Noises.RemoveAll([Time](const FNoiseEvent& Noise) { return Time - Noise.Time > 1.5f; });
	Struggles.RemoveAll([Time](const FStruggleEvent& Struggle) { return Time - Struggle.Time > 1.5f || !Struggle.Fish.IsValid(); });
	AllFish.RemoveAll([](const TWeakObjectPtr<ASpearfishFish>& Fish) { return !Fish.IsValid(); });

	if (World->GetNetMode() != NM_Client)
	{
		TickSchools(DeltaTime);
		TickRespawns();
		TickSightings(DeltaTime);
	}
}

// ----------------------------------------------------------------------------------- Registry

void USpearfishFishSubsystem::RegisterFish(ASpearfishFish* Fish)
{
	AllFish.AddUnique(Fish);
}

void USpearfishFishSubsystem::UnregisterFish(ASpearfishFish* Fish)
{
	AllFish.Remove(Fish);
}

// ------------------------------------------------------------------------------------ Schools

int32 USpearfishFishSubsystem::CreateSchool(const FVector& Home, float Radius)
{
	FSpearfishSchool School;
	School.Id = NextSchoolId++;
	School.Home = Home;
	School.Radius = Radius;
	School.Target = Home;
	School.Center = Home;
	Schools.Add(School.Id, School);
	return School.Id;
}

void USpearfishFishSubsystem::JoinSchool(int32 SchoolId, ASpearfishFish* Fish)
{
	if (FSpearfishSchool* School = Schools.Find(SchoolId))
	{
		School->Members.AddUnique(Fish);
	}
}

void USpearfishFishSubsystem::LeaveSchool(int32 SchoolId, ASpearfishFish* Fish)
{
	if (FSpearfishSchool* School = Schools.Find(SchoolId))
	{
		School->Members.Remove(Fish);
	}
}

void USpearfishFishSubsystem::PanicSchool(int32 SchoolId, const FVector& From)
{
	if (FSpearfishSchool* School = Schools.Find(SchoolId))
	{
		School->PanicUntil = Now() + 2.5f;
		School->PanicFrom = From;
		// Scatter the whole school away from the threat and pick a new destination far from it.
		const FVector Away = (School->Center - From).GetSafeNormal2D();
		School->Target = School->Center + Away * static_cast<double>(School->Radius);
		School->RetargetTime = Now() + 6.f;
	}
}

void USpearfishFishSubsystem::TickSchools(float DeltaTime)
{
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	TArray<int32> Empty;
	for (TPair<int32, FSpearfishSchool>& Pair : Schools)
	{
		FSpearfishSchool& School = Pair.Value;
		School.Members.RemoveAll([](const TWeakObjectPtr<ASpearfishFish>& Fish) { return !Fish.IsValid(); });
		if (School.Members.Num() == 0)
		{
			Empty.Add(Pair.Key);
			continue;
		}

		FVector Center = FVector::ZeroVector;
		FVector Heading = FVector::ZeroVector;
		for (const TWeakObjectPtr<ASpearfishFish>& Member : School.Members)
		{
			Center += Member->GetActorLocation();
			Heading += Member->GetFishVelocity();
		}
		School.Center = Center / static_cast<double>(School.Members.Num());
		School.Heading = Heading.GetSafeNormal(UE_SMALL_NUMBER, School.Heading);

		if (Now() > School.RetargetTime || FVector::DistSquared(School.Center, School.Target) < FMath::Square(250.0))
		{
			FVector Target = School.Home + SpearfishRandom::Vector(Stream, -School.Radius, School.Radius, -250.f, 250.f);
			if (Ocean)
			{
				const double Top = Ocean->GetSeaLevel() - 150.0;
				const double Bottom = Ocean->GetSeabedZ(Target) + 120.0;
				Target.Z = FMath::Clamp(Target.Z, FMath::Min(Bottom, Top), Top);
			}
			School.Target = Target;
			School.RetargetTime = Now() + Stream.FRandRange(8.f, 16.f);
		}
	}
	for (const int32 Id : Empty)
	{
		Schools.Remove(Id);
	}
}

// --------------------------------------------------------------------------------- Perception

void USpearfishFishSubsystem::ReportNoise(const FVector& Location, float Radius)
{
	FNoiseEvent Noise;
	Noise.Location = Location;
	Noise.Radius = Radius;
	Noise.Time = Now();
	Noises.Add(Noise);
}

bool USpearfishFishSubsystem::WasStartled(const FVector& Location, float WithinSeconds) const
{
	const float Time = Now();
	for (const FNoiseEvent& Noise : Noises)
	{
		if (Time - Noise.Time <= WithinSeconds && FVector::DistSquared(Location, Noise.Location) < FMath::Square(static_cast<double>(Noise.Radius)))
		{
			return true;
		}
	}
	return false;
}

void USpearfishFishSubsystem::ReportStruggle(const FVector& Location, ASpearfishFish* Fish)
{
	const float Time = Now();
	for (FStruggleEvent& Existing : Struggles)
	{
		if (Existing.Fish == Fish)
		{
			Existing.Location = Location;
			Existing.Time = Time;
			return;
		}
	}
	FStruggleEvent Struggle;
	Struggle.Fish = Fish;
	Struggle.Location = Location;
	Struggle.Time = Time;
	Struggles.Add(Struggle);
}

ASpearfishFish* USpearfishFishSubsystem::FindStrugglingFish(const FVector& Near, float Radius) const
{
	ASpearfishFish* Best = nullptr;
	double BestDistance = FMath::Square(static_cast<double>(Radius));
	for (const FStruggleEvent& Struggle : Struggles)
	{
		ASpearfishFish* Fish = Struggle.Fish.Get();
		if (!Fish || !Fish->IsHooked())
		{
			continue;
		}
		const double Distance = FVector::DistSquared(Near, Fish->GetActorLocation());
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Fish;
		}
	}
	return Best;
}

ASpearfishCharacter* USpearfishFishSubsystem::FindNearestDiver(const FVector& Location, float MaxDistance, float& OutDistance) const
{
	OutDistance = UE_BIG_NUMBER;
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState)
	{
		return nullptr;
	}
	ASpearfishCharacter* Best = nullptr;
	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		ASpearfishCharacter* Character = PlayerState ? PlayerState->GetPawn<ASpearfishCharacter>() : nullptr;
		if (!Character || !Character->IsSwimming() || Character->IsBlackedOut())
		{
			continue;
		}
		const float Distance = static_cast<float>(FVector::Dist(Location, Character->GetActorLocation()));
		if (Distance < MaxDistance && Distance < OutDistance)
		{
			OutDistance = Distance;
			Best = Character;
		}
	}
	return Best;
}

ASpearfishFish* USpearfishFishSubsystem::FindPrey(const ASpearfishFish* Predator, float Radius) const
{
	const FSpearfishFishSpeciesDef* PredatorSpecies = Predator ? Predator->GetSpecies() : nullptr;
	if (!PredatorSpecies || PredatorSpecies->Behavior.PreyCategories.Num() == 0)
	{
		return nullptr;
	}
	ASpearfishFish* Best = nullptr;
	double BestDistance = FMath::Square(static_cast<double>(Radius));
	for (const TWeakObjectPtr<ASpearfishFish>& Weak : AllFish)
	{
		ASpearfishFish* Fish = Weak.Get();
		const FSpearfishFishSpeciesDef* Species = Fish ? Fish->GetSpecies() : nullptr;
		if (!Species || Fish == Predator || !Species->bCatchable || Fish->GetLengthCm() > Predator->GetLengthCm() * 0.6f)
		{
			continue;
		}
		if (!PredatorSpecies->Behavior.PreyCategories.Contains(Species->Category))
		{
			continue;
		}
		const double Distance = FVector::DistSquared(Predator->GetActorLocation(), Fish->GetActorLocation());
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Fish;
		}
	}
	return Best;
}

ASpearfishFish* USpearfishFishSubsystem::FindPredatorThreat(const ASpearfishFish* Prey, float Radius) const
{
	const FSpearfishFishSpeciesDef* PreySpecies = Prey ? Prey->GetSpecies() : nullptr;
	if (!PreySpecies)
	{
		return nullptr;
	}
	for (const TWeakObjectPtr<ASpearfishFish>& Weak : AllFish)
	{
		ASpearfishFish* Fish = Weak.Get();
		const FSpearfishFishSpeciesDef* Species = Fish ? Fish->GetSpecies() : nullptr;
		if (!Species || Species->Archetype != ESpearfishFishArchetype::Predator || !Species->Behavior.PreyCategories.Contains(PreySpecies->Category))
		{
			continue;
		}
		if (FVector::DistSquared(Prey->GetActorLocation(), Fish->GetActorLocation()) < FMath::Square(static_cast<double>(Radius)))
		{
			return Fish;
		}
	}
	return nullptr;
}

bool USpearfishFishSubsystem::FindNearestCover(const FVector& Location, float MaxDistance, FVector& OutPoint) const
{
	double BestDistance = FMath::Square(static_cast<double>(MaxDistance));
	bool bFound = false;
	for (const FVector& Point : CoverPoints)
	{
		const double Distance = FVector::DistSquared(Location, Point);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			OutPoint = Point;
			bFound = true;
		}
	}
	return bFound;
}

// --------------------------------------------------------------------------------- Population

void USpearfishFishSubsystem::ClearWorldData()
{
	CoverPoints.Reset();
	Habitat.Reset();
}

bool USpearfishFishSubsystem::IsRegionSpecies(FName SpeciesId) const
{
	return RegionSpecies.Contains(SpeciesId);
}

bool USpearfishFishSubsystem::FindHabitatFor(const FSpearfishFishSpeciesDef& Species, FRandomStream& InStream, bool bAwayFromDivers, FVector& OutLocation) const
{
	static const FName OpenWater(TEXT("OpenWater"));
	const bool bOpenWaterSpecies = Species.Biomes.Contains(OpenWater);

	TArray<const FSpearfishHabitatPoint*> Candidates;
	for (const FSpearfishHabitatPoint& Point : Habitat)
	{
		const bool bBiome = Species.Biomes.Contains(Point.Biome) || (bOpenWaterSpecies && Point.Biome == OpenWater);
		if (bBiome && Point.DepthM >= Species.MinDepthM && Point.DepthM <= Species.MaxDepthM + 3.f)
		{
			Candidates.Add(&Point);
		}
	}
	if (Candidates.Num() == 0)
	{
		// Data asked for a biome this region lacks: fall back to the depth band.
		for (const FSpearfishHabitatPoint& Point : Habitat)
		{
			if (Point.DepthM >= Species.MinDepthM && Point.DepthM <= Species.MaxDepthM + 3.f)
			{
				Candidates.Add(&Point);
			}
		}
	}
	if (Candidates.Num() == 0)
	{
		return false;
	}

	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	for (int32 Attempt = 0; Attempt < 16; ++Attempt)
	{
		const FSpearfishHabitatPoint* Point = Candidates[InStream.RandRange(0, Candidates.Num() - 1)];
		FVector Location = Point->Location;
		if (Point->Biome != OpenWater)
		{
			Location.Z += InStream.FRandRange(60.f, 180.f);
		}
		if (Ocean)
		{
			Location.Z = FMath::Min(Location.Z, static_cast<double>(Ocean->GetSeaLevel() - 120.f));
		}
		if (bAwayFromDivers && Attempt < 15)
		{
			float DiverDistance = 0.f;
			if (FindNearestDiver(Location, 3000.f, DiverDistance))
			{
				continue;
			}
		}
		OutLocation = Location;
		return true;
	}
	return false;
}

ASpearfishFish* USpearfishFishSubsystem::SpawnFish(FName SpeciesId, const FVector& Location, int32 SchoolId)
{
	UWorld* World = GetWorld();
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FSpearfishFishSpeciesDef* Species = Registry ? Registry->FindFish(SpeciesId) : nullptr;
	if (!World || !Species)
	{
		return nullptr;
	}
	const FTransform Transform(FRotator(0.f, Stream.FRandRange(0.f, 360.f), 0.f), Location);
	ASpearfishFish* Fish = World->SpawnActorDeferred<ASpearfishFish>(ASpearfishFish::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Fish)
	{
		return nullptr;
	}
	const float Length = SpearfishFish::RollLength(Stream, Species->MinLengthCm, Species->MaxLengthCm);
	Fish->InitializeFish(SpeciesId, Length, Location, SchoolId, Stream.RandRange(1, 1 << 30));
	Fish->FinishSpawning(Transform);
	JoinSchool(SchoolId, Fish);
	return Fish;
}

int32 USpearfishFishSubsystem::SpawnGroup(FName SpeciesId, const FVector& Location, int32 Count, bool bForceSchool)
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FSpearfishFishSpeciesDef* Species = Registry ? Registry->FindFish(SpeciesId) : nullptr;
	if (!Species || Count <= 0)
	{
		return 0;
	}
	const bool bSchool = Count > 1 || bForceSchool;
	const float Radius = Species->Archetype == ESpearfishFishArchetype::Pelagic || Species->Archetype == ESpearfishFishArchetype::Schooler ? 2500.f : 1200.f;
	const int32 SchoolId = bSchool ? CreateSchool(Location, Radius) : INDEX_NONE;
	int32 Spawned = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Offset = Stream.VRand() * static_cast<double>(60.f + Count * 12.f);
		Spawned += SpawnFish(SpeciesId, Location + Offset, SchoolId) ? 1 : 0;
	}
	return Spawned;
}

void USpearfishFishSubsystem::DespawnAll()
{
	TArray<TWeakObjectPtr<ASpearfishFish>> Copy = AllFish;
	for (const TWeakObjectPtr<ASpearfishFish>& Fish : Copy)
	{
		if (ASpearfishFish* Actor = Fish.Get())
		{
			Actor->Destroy();
		}
	}
	AllFish.Reset();
	Schools.Reset();
	Respawns.Reset();
}

void USpearfishFishSubsystem::SpawnRegionPopulation(const FSpearfishRegionDef& Region, int32 DaySeed)
{
	DespawnAll();
	Stream.Initialize(DaySeed);
	RegionSpecies.Reset();

	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	if (!Registry)
	{
		return;
	}
	int32 Total = 0;
	for (const FSpearfishRegionSpawn& Spawn : Region.Spawns)
	{
		const FSpearfishFishSpeciesDef* Species = Registry->FindFish(Spawn.SpeciesId);
		if (!Species)
		{
			continue;
		}
		RegionSpecies.AddUnique(Spawn.SpeciesId);
		for (int32 Group = 0; Group < Spawn.Groups; ++Group)
		{
			FVector Location;
			if (!FindHabitatFor(*Species, Stream, false, Location))
			{
				continue;
			}
			const int32 Min = FMath::Max(1, Species->Behavior.SchoolSizeMin);
			const int32 Max = FMath::Max(Min, Species->Behavior.SchoolSizeMax);
			Total += SpawnGroup(Spawn.SpeciesId, Location, Stream.RandRange(Min, Max));
		}
	}
	UE_LOG(LogSpearfish, Log, TEXT("Ecosystem: spawned %d animals for region %s"), Total, *Region.Id.ToString());
}

void USpearfishFishSubsystem::NotifyFishRemoved(ASpearfishFish* Fish)
{
	if (!Fish || !IsRegionSpecies(Fish->GetSpeciesId()))
	{
		return;
	}
	FRespawn Respawn;
	Respawn.SpeciesId = Fish->GetSpeciesId();
	Respawn.Time = Now() + USpearfishSettings::Get()->FishRespawnSeconds * Stream.FRandRange(0.8f, 1.4f);
	Respawns.Add(Respawn);
}

void USpearfishFishSubsystem::TickRespawns()
{
	const float Time = Now();
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	for (int32 Index = Respawns.Num() - 1; Index >= 0; --Index)
	{
		if (Respawns[Index].Time > Time)
		{
			continue;
		}
		const FName SpeciesId = Respawns[Index].SpeciesId;
		Respawns.RemoveAt(Index);
		const FSpearfishFishSpeciesDef* Species = Registry ? Registry->FindFish(SpeciesId) : nullptr;
		FVector Location;
		if (!Species || !FindHabitatFor(*Species, Stream, true, Location))
		{
			continue;
		}

		// Rejoin an existing school of the same species when there is one.
		int32 SchoolId = INDEX_NONE;
		for (const TPair<int32, FSpearfishSchool>& Pair : Schools)
		{
			const ASpearfishFish* Member = Pair.Value.Members.Num() > 0 ? Pair.Value.Members[0].Get() : nullptr;
			if (Member && Member->GetSpeciesId() == SpeciesId)
			{
				SchoolId = Pair.Key;
				Location = Pair.Value.Center + Stream.VRand() * 300.0;
				break;
			}
		}
		SpawnFish(SpeciesId, Location, SchoolId);
	}
}

void USpearfishFishSubsystem::TickSightings(float DeltaTime)
{
	SightingTimer += DeltaTime;
	if (SightingTimer < 0.5f)
	{
		return;
	}
	SightingTimer = 0.f;

	ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	USpearfishJournalComponent* Journal = GameState ? GameState->GetJournal() : nullptr;
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	if (!Journal || !Registry)
	{
		return;
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const ASpearfishCharacter* Character = PlayerState ? PlayerState->GetPawn<ASpearfishCharacter>() : nullptr;
		if (!Character || !Character->IsSwimming())
		{
			continue;
		}
		for (const TWeakObjectPtr<ASpearfishFish>& Weak : AllFish)
		{
			const ASpearfishFish* Fish = Weak.Get();
			if (!Fish || FVector::DistSquared(Fish->GetActorLocation(), Character->GetActorLocation()) > FMath::Square(700.0))
			{
				continue;
			}
			if (Journal->MarkSighted(Fish->GetSpeciesId()))
			{
				const FSpearfishFishSpeciesDef* Species = Fish->GetSpecies();
				const ESpearfishRarity Rarity = Species ? Species->Rarity : ESpearfishRarity::Common;
				GameState->BroadcastNotice(FText::Format(LOCTEXT("Sighted", "{0} spotted a {1} ({2})!"),
					FText::FromString(PlayerState->GetPlayerName()), Registry->GetDisplayName(Fish->GetSpeciesId()), SpearfishText::RarityName(Rarity)),
					Rarity >= ESpearfishRarity::Rare ? ESpearfishNoticeType::Discovery : ESpearfishNoticeType::Info);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
