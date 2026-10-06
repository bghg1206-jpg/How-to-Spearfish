#include "World/SpearfishOceanSubsystem.h"

#include "Core/SpearfishSettings.h"
#include "Data/SpearfishDataRegistry.h"
#include "Engine/World.h"

void USpearfishOceanSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SeaLevelZ = USpearfishSettings::Get()->SeaLevelZ;
}

USpearfishOceanSubsystem* USpearfishOceanSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<USpearfishOceanSubsystem>() : nullptr;
}

void USpearfishOceanSubsystem::SetRegion(FName InRegionId, const FSpearfishTerrainParams& Params, int32 Seed)
{
	RegionId = InRegionId;
	Terrain.Initialize(Params, Seed);
}

const FSpearfishRegionDef* USpearfishOceanSubsystem::GetRegionDef() const
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	return Registry ? Registry->FindRegion(RegionId) : nullptr;
}

float USpearfishOceanSubsystem::GetSeabedZ(const FVector& Location) const
{
	return Terrain.IsInitialized() ? Terrain.GetSeabedZ(static_cast<float>(Location.X), static_cast<float>(Location.Y)) : SeaLevelZ - 5000.f;
}

const FSpearfishDepthZoneDef* USpearfishOceanSubsystem::GetDepthZone(float DepthM) const
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	return Registry ? Registry->FindDepthZone(RegionId, DepthM) : nullptr;
}

float USpearfishOceanSubsystem::GetOutOfBoundsDistance(const FVector& Location) const
{
	if (!Terrain.IsInitialized())
	{
		return 0.f;
	}
	const float Edge = FMath::Max(FMath::Abs(static_cast<float>(Location.X)), FMath::Abs(static_cast<float>(Location.Y))) - Terrain.GetHalfExtent();
	return FMath::Max(Edge, 0.f);
}

FVector USpearfishOceanSubsystem::GetBoundaryCurrent(const FVector& Location) const
{
	const float Outside = GetOutOfBoundsDistance(Location);
	if (Outside <= 0.f)
	{
		return FVector::ZeroVector;
	}
	// Grows quickly: gentle nudge at the edge, impossible to fight 20 m out.
	const float Strength = FMath::Min(80.f + Outside * 0.6f, 1600.f);
	const FVector ToCenter = FVector(-Location.X, -Location.Y, 0.0).GetSafeNormal();
	return ToCenter * static_cast<double>(Strength);
}
