#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/SpearfishTerrain.h"
#include "SpearfishFishSubsystem.generated.h"

class ASpearfishCharacter;
class ASpearfishFish;
struct FSpearfishFishSpeciesDef;
struct FSpearfishRegionDef;

/** A group of fish moving together. Schools share a roaming target and panic together. */
struct FSpearfishSchool
{
	int32 Id = INDEX_NONE;
	TArray<TWeakObjectPtr<ASpearfishFish>> Members;
	FVector Home = FVector::ZeroVector;
	float Radius = 1200.f;
	FVector Target = FVector::ZeroVector;
	float RetargetTime = 0.f;
	FVector Center = FVector::ZeroVector;
	FVector Heading = FVector::ForwardVector;
	float PanicUntil = 0.f;
	FVector PanicFrom = FVector::ZeroVector;
};

/**
 * The ecosystem manager. Holds the fish registry, schools, perception events (harpoon noise, struggling
 * catches), cover points and habitat samples; spawns each morning's population from region data; respawns
 * species away from divers; and records journal sightings. Spawning and simulation are server-only, the
 * spatial queries are available everywhere.
 */
UCLASS()
class HOWTOSPEARFISH_API USpearfishFishSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	static USpearfishFishSubsystem* Get(const UObject* WorldContextObject);

	// --- Registry ---------------------------------------------------------------------------
	void RegisterFish(ASpearfishFish* Fish);
	void UnregisterFish(ASpearfishFish* Fish);
	const TArray<TWeakObjectPtr<ASpearfishFish>>& GetAllFish() const { return AllFish; }

	// --- Schools ----------------------------------------------------------------------------
	int32 CreateSchool(const FVector& Home, float Radius);
	void JoinSchool(int32 SchoolId, ASpearfishFish* Fish);
	void LeaveSchool(int32 SchoolId, ASpearfishFish* Fish);
	const FSpearfishSchool* GetSchool(int32 SchoolId) const { return Schools.Find(SchoolId); }
	void PanicSchool(int32 SchoolId, const FVector& From);

	// --- Perception -------------------------------------------------------------------------
	void ReportNoise(const FVector& Location, float Radius);
	bool WasStartled(const FVector& Location, float WithinSeconds) const;
	void ReportStruggle(const FVector& Location, ASpearfishFish* Fish);
	ASpearfishFish* FindStrugglingFish(const FVector& Near, float Radius) const;
	ASpearfishCharacter* FindNearestDiver(const FVector& Location, float MaxDistance, float& OutDistance) const;
	ASpearfishFish* FindPrey(const ASpearfishFish* Predator, float Radius) const;
	ASpearfishFish* FindPredatorThreat(const ASpearfishFish* Prey, float Radius) const;

	void RegisterCoverPoint(const FVector& Location) { CoverPoints.Add(Location); }
	bool FindNearestCover(const FVector& Location, float MaxDistance, FVector& OutPoint) const;

	// --- Population (server) ----------------------------------------------------------------
	void SetHabitat(const TArray<FSpearfishHabitatPoint>& Points) { Habitat = Points; }
	void ClearWorldData();
	void SpawnRegionPopulation(const FSpearfishRegionDef& Region, int32 DaySeed);
	/** Spawns a school (or a single fish) of a species near a location. Returns the number spawned. */
	int32 SpawnGroup(FName SpeciesId, const FVector& Location, int32 Count, bool bForceSchool = false);
	ASpearfishFish* SpawnFish(FName SpeciesId, const FVector& Location, int32 SchoolId);
	void DespawnAll();
	void NotifyFishRemoved(ASpearfishFish* Fish);
	bool FindHabitatFor(const FSpearfishFishSpeciesDef& Species, FRandomStream& Stream, bool bAwayFromDivers, FVector& OutLocation) const;

private:
	struct FNoiseEvent
	{
		FVector Location = FVector::ZeroVector;
		float Radius = 0.f;
		float Time = 0.f;
	};

	struct FStruggleEvent
	{
		TWeakObjectPtr<ASpearfishFish> Fish;
		FVector Location = FVector::ZeroVector;
		float Time = 0.f;
	};

	struct FRespawn
	{
		FName SpeciesId;
		float Time = 0.f;
	};

	float Now() const;
	void TickSchools(float DeltaTime);
	void TickRespawns();
	void TickSightings(float DeltaTime);
	bool IsRegionSpecies(FName SpeciesId) const;

	TArray<TWeakObjectPtr<ASpearfishFish>> AllFish;
	TMap<int32, FSpearfishSchool> Schools;
	int32 NextSchoolId = 1;
	TArray<FNoiseEvent> Noises;
	TArray<FStruggleEvent> Struggles;
	TArray<FVector> CoverPoints;
	TArray<FSpearfishHabitatPoint> Habitat;
	TArray<FRespawn> Respawns;
	TArray<FName> RegionSpecies;
	float SightingTimer = 0.f;
	FRandomStream Stream;
};
