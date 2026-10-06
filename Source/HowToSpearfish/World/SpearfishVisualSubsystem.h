#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpearfishVisualSubsystem.generated.h"

class AActor;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ESpearfishShape : uint8
{
	Cube,
	Sphere,
	Cylinder,
	Cone,
	Plane
};

/**
 * Placeholder-art factory. Builds readable geometry from the engine's basic shapes with cached colour
 * materials, so the whole vertical slice runs without authored assets. Art can replace any of it via the
 * mesh/material references in the content definitions.
 */
UCLASS()
class HOWTOSPEARFISH_API USpearfishVisualSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static USpearfishVisualSubsystem* Get(const UObject* WorldContextObject);

	UStaticMesh* GetShape(ESpearfishShape Shape);

	/** Cached colour material (opaque). Emissive > 0 makes it glow when the base material supports it. */
	UMaterialInterface* GetColorMaterial(const FLinearColor& Color, float Emissive = 0.f);

	/** Ocean surface material (top or underside), with fallbacks when no authored material exists. */
	UMaterialInterface* GetWaterMaterial(bool bUnderside, const FLinearColor& Tint);

	/**
	 * Adds a registered mesh part to an actor.
	 * @param SizeCm  world size of the part (basic shapes are 100 cm units).
	 */
	UStaticMeshComponent* AddPart(AActor* Owner, USceneComponent* Parent, ESpearfishShape Shape, const FVector& Location,
		const FRotator& Rotation, const FVector& SizeCm, const FLinearColor& Color, bool bCollision, float Emissive = 0.f);

	/** Adds an instanced mesh component for scattering many copies of one shape/colour. */
	UHierarchicalInstancedStaticMeshComponent* AddInstancer(AActor* Owner, USceneComponent* Parent, ESpearfishShape Shape,
		const FLinearColor& Color, bool bCollision, float Emissive = 0.f);

	/** Applies the standard collision setup for visual parts. */
	static void ConfigureCollision(class UPrimitiveComponent* Component, bool bCollision);

private:
	UMaterialInterface* GetBaseMaterial();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMesh>> Shapes;

	UPROPERTY(Transient)
	TMap<uint32, TObjectPtr<UMaterialInstanceDynamic>> ColorMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> WaterTop;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> WaterUnderside;

	bool bBaseMaterialResolved = false;
	bool bWaterResolved = false;
};
