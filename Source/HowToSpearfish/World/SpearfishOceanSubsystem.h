#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/SpearfishTerrain.h"
#include "SpearfishOceanSubsystem.generated.h"

struct FSpearfishDepthZoneDef;
struct FSpearfishRegionDef;

/**
 * Single source of truth for "where is the water": sea level, depth, depth zones, the region's seabed
 * function and the soft boundary current that keeps divers and the boat inside the playable area.
 * Exists on every machine; populated by ASpearfishRegionBuilder from the replicated region/seed.
 */
UCLASS()
class HOWTOSPEARFISH_API USpearfishOceanSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static USpearfishOceanSubsystem* Get(const UObject* WorldContextObject);

	void SetRegion(FName InRegionId, const FSpearfishTerrainParams& Params, int32 Seed);

	float GetSeaLevel() const { return SeaLevelZ; }
	float GetDepthMeters(const FVector& Location) const { return static_cast<float>(SeaLevelZ - Location.Z) / 100.f; }
	bool IsUnderwater(const FVector& Location) const { return Location.Z < SeaLevelZ; }

	bool HasTerrain() const { return Terrain.IsInitialized(); }
	const FSpearfishTerrain& GetTerrain() const { return Terrain; }
	FName GetRegionId() const { return RegionId; }
	const FSpearfishRegionDef* GetRegionDef() const;

	float GetSeabedZ(const FVector& Location) const;

	/** Depth zone for a depth in metres (nullptr without region data). */
	const FSpearfishDepthZoneDef* GetDepthZone(float DepthM) const;

	/** Acceleration (cm/s^2) of the current that pushes things back inside the region. */
	FVector GetBoundaryCurrent(const FVector& Location) const;

	/** Distance (cm) beyond the playable edge; 0 when inside. */
	float GetOutOfBoundsDistance(const FVector& Location) const;

private:
	FSpearfishTerrain Terrain;
	FName RegionId;
	float SeaLevelZ = 0.f;
};
