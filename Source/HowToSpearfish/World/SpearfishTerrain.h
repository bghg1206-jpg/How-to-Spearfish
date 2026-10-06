#pragma once

// Deterministic procedural seabed for regions without an authored map.
// Pure math on CoreMinimal types: every machine (server and clients) evaluates the same function from the
// replicated seed, so static geometry never needs replication. Unit tested natively (Tools/NativeCheck).

#include "CoreMinimal.h"
#include "SpearfishTerrain.generated.h"

/**
 * Triangle winding for procedural meshes. Unreal renders triangle (A, B, C) front-facing toward
 * (C - A) x (B - A), i.e. opposite to (B - A) x (C - A). Two engine references agree:
 * UKismetProceduralMeshLibrary::GenerateBoxMesh builds its +Z face from (-x,+y), (+x,+y), (+x,-y), (-x,-y)
 * through ConvertQuadToTris (0,1,3 / 1,2,3), and CalculateTangentsForMesh derives the face normal as
 * (P1 - P2) ^ (P0 - P2). Covered by the Terrain.Winding test.
 */
namespace SpearfishMeshWinding
{
	inline FVector FrontNormal(const FVector& A, const FVector& B, const FVector& C)
	{
		return (C - A) ^ (B - A);
	}

	/** True when (A, B, C) must be emitted as (A, C, B) to face Facing. */
	inline bool NeedsSwap(const FVector& A, const FVector& B, const FVector& C, const FVector& Facing)
	{
		return FVector::DotProduct(FrontNormal(A, B, C), Facing) < 0.0;
	}
}

USTRUCT(BlueprintType)
struct FSpearfishTerrainParams
{
	GENERATED_BODY()

	/** Half size of the playable seabed square. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	float HalfExtentCm = 9000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	float ShallowDepthM = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	float MaxDepthM = 45.f;

	/** 0..1 amount of reef cover. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	float ReefDensity = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	float RockDensity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	float KelpDensity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	float SeagrassDensity = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	int32 Wrecks = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	int32 Caves = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	int32 GiantClams = 4;
};

UENUM(BlueprintType)
enum class ESpearfishPOIType : uint8
{
	Wreck,
	Cave,
	ReefHead,
	ClamBed,
	Trench
};

USTRUCT(BlueprintType)
struct FSpearfishPOI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	ESpearfishPOIType Type = ESpearfishPOIType::ReefHead;

	/** On the seabed. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float Yaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float RadiusCm = 800.f;
};

USTRUCT(BlueprintType)
struct FSpearfishTerrainLayout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	FVector2D IslandCenter = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float IslandRadiusCm = 2000.f;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float IslandPeakCm = 700.f;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float BeachWidthCm = 1100.f;

	/** Unit direction from the island toward the reef and the deep water. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	FVector2D ReefDirection = FVector2D(1.0, 0.0);

	/** Shore end of the dock (on land). */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	FVector DockLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float DockYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float DockLengthCm = 1400.f;

	/** Boat mooring at sea level next to the dock head. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	FVector BoatStart = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float BoatStartYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	TArray<FSpearfishPOI> POIs;
};

/** A sampled spot on the seabed or in the water column where fish/loot can be placed. */
USTRUCT(BlueprintType)
struct FSpearfishHabitatPoint
{
	GENERATED_BODY()

	/** Seabed point (for OpenWater: a mid-water point). */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	FName Biome;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float DepthM = 0.f;
};

class HOWTOSPEARFISH_API FSpearfishTerrain
{
public:
	void Initialize(const FSpearfishTerrainParams& InParams, int32 InSeed);
	bool IsInitialized() const { return bInitialized; }

	/** Seabed / ground height in cm (negative underwater, positive on the island). */
	float GetSeabedZ(float X, float Y) const;

	/** Water depth in metres above the seabed at this column (0 on land). */
	float GetWaterDepthM(float X, float Y, float SeaLevelZ) const;

	FVector GetNormal(float X, float Y) const;

	/** Land, Sand, Reef, Seagrass, Kelp, Wall, Wreck, Cave. */
	FName GetBiome(float X, float Y) const;

	const FSpearfishTerrainLayout& GetLayout() const { return Layout; }
	const FSpearfishTerrainParams& GetParams() const { return Params; }
	int32 GetSeed() const { return Seed; }
	float GetHalfExtent() const { return Params.HalfExtentCm; }
	bool IsInsideBounds(float X, float Y, float Margin = 0.f) const;

	/** Deterministic grid of habitat samples (seabed + open-water points) for spawning. */
	TArray<FSpearfishHabitatPoint> SampleHabitat(float SpacingCm, float SeaLevelZ) const;

	// Noise primitives (deterministic across platforms; outputs in [0, 1)).
	static float Hash2D(int32 X, int32 Y, int32 InSeed);
	static float ValueNoise(float X, float Y, int32 InSeed);
	static float FBM(float X, float Y, int32 InSeed, int32 Octaves);
	static float Ridged(float X, float Y, int32 InSeed, int32 Octaves);

private:
	float BaseHeight(float X, float Y) const;
	static float DepthProfile(float T);
	void PlanLayout();
	bool FindPOISpot(FRandomStream& Rng, float MinDepthM, float MaxDepthM, float Spacing, float MaxReliefCm, float ReliefRadius, FVector& OutLocation) const;

	FSpearfishTerrainParams Params;
	FSpearfishTerrainLayout Layout;
	int32 Seed = 0;
	float FarDistanceCm = 1.f;
	bool bInitialized = false;
};
