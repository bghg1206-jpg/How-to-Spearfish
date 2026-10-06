#include "World/SpearfishVisualSubsystem.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/SpearfishSettings.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HowToSpearfish.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace SpearfishVisualPrivate
{
	const TCHAR* ShapePath(ESpearfishShape Shape)
	{
		switch (Shape)
		{
		case ESpearfishShape::Cube: return TEXT("/Engine/BasicShapes/Cube.Cube");
		case ESpearfishShape::Sphere: return TEXT("/Engine/BasicShapes/Sphere.Sphere");
		case ESpearfishShape::Cylinder: return TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
		case ESpearfishShape::Cone: return TEXT("/Engine/BasicShapes/Cone.Cone");
		case ESpearfishShape::Plane: return TEXT("/Engine/BasicShapes/Plane.Plane");
		default: return TEXT("/Engine/BasicShapes/Cube.Cube");
		}
	}

	uint32 ColorKey(const FLinearColor& Color, float Emissive)
	{
		const FColor Quantized = Color.ToFColor(false);
		const uint32 EmissiveBits = static_cast<uint32>(FMath::Clamp(FMath::RoundToInt(Emissive * 4.f), 0, 255));
		return HashCombine(HashCombine(HashCombine(Quantized.R, Quantized.G), HashCombine(Quantized.B, Quantized.A)), EmissiveBits);
	}
}

USpearfishVisualSubsystem* USpearfishVisualSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<USpearfishVisualSubsystem>() : nullptr;
}

UStaticMesh* USpearfishVisualSubsystem::GetShape(ESpearfishShape Shape)
{
	const int32 Index = static_cast<int32>(Shape);
	if (Shapes.Num() <= Index)
	{
		Shapes.SetNum(static_cast<int32>(ESpearfishShape::Plane) + 1);
	}
	if (!Shapes[Index])
	{
		Shapes[Index] = LoadObject<UStaticMesh>(nullptr, SpearfishVisualPrivate::ShapePath(Shape));
	}
	return Shapes[Index];
}

UMaterialInterface* USpearfishVisualSubsystem::GetBaseMaterial()
{
	if (!bBaseMaterialResolved)
	{
		bBaseMaterialResolved = true;
		const USpearfishSettings* Settings = USpearfishSettings::Get();
		if (!Settings->BaseMaterial.IsNull())
		{
			BaseMaterial = Settings->BaseMaterial.LoadSynchronous();
		}
		if (!BaseMaterial && !Settings->FallbackBaseMaterial.IsNull())
		{
			BaseMaterial = Settings->FallbackBaseMaterial.LoadSynchronous();
		}
		UE_LOG(LogSpearfish, Log, TEXT("Visuals: base material %s"), BaseMaterial ? *BaseMaterial->GetPathName() : TEXT("<none>"));
	}
	return BaseMaterial;
}

UMaterialInterface* USpearfishVisualSubsystem::GetColorMaterial(const FLinearColor& Color, float Emissive)
{
	const uint32 Key = SpearfishVisualPrivate::ColorKey(Color, Emissive);
	if (const TObjectPtr<UMaterialInstanceDynamic>* Existing = ColorMaterials.Find(Key))
	{
		return *Existing;
	}

	UMaterialInterface* Base = GetBaseMaterial();
	if (!Base)
	{
		return nullptr;
	}
	const USpearfishSettings* Settings = USpearfishSettings::Get();
	UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Base, this);
	Instance->SetVectorParameterValue(Settings->BaseColorParameter, Color);
	if (Emissive > 0.f)
	{
		Instance->SetScalarParameterValue(Settings->EmissiveParameter, Emissive);
	}
	ColorMaterials.Add(Key, Instance);
	return Instance;
}

UMaterialInterface* USpearfishVisualSubsystem::GetWaterMaterial(bool bUnderside, const FLinearColor& Tint)
{
	if (!bWaterResolved)
	{
		bWaterResolved = true;
		const USpearfishSettings* Settings = USpearfishSettings::Get();
		WaterTop = Settings->WaterSurfaceMaterial.IsNull() ? nullptr : Settings->WaterSurfaceMaterial.LoadSynchronous();
		WaterUnderside = Settings->WaterUndersideMaterial.IsNull() ? nullptr : Settings->WaterUndersideMaterial.LoadSynchronous();
		if (!WaterTop)
		{
			UE_LOG(LogSpearfish, Log, TEXT("Visuals: no authored ocean material; using opaque placeholder (run Tools/Editor/bootstrap_content.py)."));
		}
	}

	UMaterialInterface* Authored = bUnderside ? WaterUnderside.Get() : WaterTop.Get();
	if (Authored)
	{
		UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Authored, this);
		Instance->SetVectorParameterValue(TEXT("Tint"), Tint);
		return Instance;
	}
	// Placeholder: the underside glows softly so the surface reads as bright from below.
	return GetColorMaterial(Tint, bUnderside ? 1.5f : 0.f);
}

void USpearfishVisualSubsystem::ConfigureCollision(UPrimitiveComponent* Component, bool bCollision)
{
	if (!Component)
	{
		return;
	}
	if (bCollision)
	{
		Component->SetCollisionProfileName(TEXT("BlockAll"));
	}
	else
	{
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

UStaticMeshComponent* USpearfishVisualSubsystem::AddPart(AActor* Owner, USceneComponent* Parent, ESpearfishShape Shape, const FVector& Location,
	const FRotator& Rotation, const FVector& SizeCm, const FLinearColor& Color, bool bCollision, float Emissive)
{
	if (!Owner)
	{
		return nullptr;
	}
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner);
	Part->SetStaticMesh(GetShape(Shape));
	Part->SetMaterial(0, GetColorMaterial(Color, Emissive));
	Part->SetupAttachment(Parent ? Parent : Owner->GetRootComponent());
	Part->SetRelativeLocation(Location);
	Part->SetRelativeRotation(Rotation);
	Part->SetRelativeScale3D(SizeCm / 100.0);
	ConfigureCollision(Part, bCollision);
	Part->SetCanEverAffectNavigation(false);
	Part->RegisterComponent();
	Owner->AddInstanceComponent(Part);
	return Part;
}

UHierarchicalInstancedStaticMeshComponent* USpearfishVisualSubsystem::AddInstancer(AActor* Owner, USceneComponent* Parent, ESpearfishShape Shape,
	const FLinearColor& Color, bool bCollision, float Emissive)
{
	if (!Owner)
	{
		return nullptr;
	}
	UHierarchicalInstancedStaticMeshComponent* Instancer = NewObject<UHierarchicalInstancedStaticMeshComponent>(Owner);
	Instancer->SetStaticMesh(GetShape(Shape));
	Instancer->SetMaterial(0, GetColorMaterial(Color, Emissive));
	Instancer->SetupAttachment(Parent ? Parent : Owner->GetRootComponent());
	ConfigureCollision(Instancer, bCollision);
	Instancer->SetCanEverAffectNavigation(false);
	Instancer->RegisterComponent();
	Owner->AddInstanceComponent(Instancer);
	return Instancer;
}
