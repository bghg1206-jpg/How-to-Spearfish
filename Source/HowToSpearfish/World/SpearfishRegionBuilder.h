#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/SpearfishTerrain.h"
#include "SpearfishRegionBuilder.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UProceduralMeshComponent;
struct FSpearfishRegionDef;

/**
 * Builds a sea region from data. Static scenery (seabed mesh, rocks, coral, seagrass, kelp, wreck, caves,
 * island, dock) is generated locally on every machine from the replicated region id + seed, so it costs no
 * bandwidth and is identical everywhere. The server additionally spawns the dynamic daily content:
 * the hub stations, loot, giant clams and physics props. Fish are spawned by USpearfishFishSubsystem using
 * the habitat samples registered here.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishRegionBuilder : public AActor
{
	GENERATED_BODY()

public:
	ASpearfishRegionBuilder();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server: before FinishSpawning. */
	void Configure(FName InRegionId, int32 InSeed);

	FName GetRegionId() const { return RegionId; }
	bool IsBuilt() const { return bBuilt; }
	const FSpearfishRegionDef* GetRegionDef() const;

	/** Server: hub stations (once) plus loot, clams and props for the day. */
	void SpawnDailyContent(int32 Day, int32 DaySeed);
	void ClearDailyContent();
	/** Server: a chest of treasure in a cave (rare event). */
	void SpawnTreasureChest(FName LootId, int32 Count, int32 DaySeed);

	FVector GetHabitatLocation(FName Biome, int32 Seed) const;

	/** Builds the static scenery if it has not been built yet. Safe to call repeatedly. */
	void BuildStatic();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnRep_Region();

	void BuildTerrainMesh();
	void ScatterScenery();
	void BuildWreck(const FSpearfishPOI& POI);
	void BuildCave(const FSpearfishPOI& POI);
	void BuildIsland();
	void SpawnLocalActors();
	void SpawnHubStations();
	UHierarchicalInstancedStaticMeshComponent* GetInstancer(int32 Shape, const FLinearColor& Color, int32 CollisionMode);

	UPROPERTY(ReplicatedUsing = OnRep_Region)
	FName RegionId;

	UPROPERTY(ReplicatedUsing = OnRep_Region)
	int32 Seed = 0;

	UPROPERTY(VisibleAnywhere, Category = "Region")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> TerrainMesh;

	UPROPERTY(Transient)
	TMap<uint32, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Instancers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> LocalActors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> DailyActors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> HubActors;

	TArray<FSpearfishHabitatPoint> Habitat;
	bool bBuilt = false;
};
