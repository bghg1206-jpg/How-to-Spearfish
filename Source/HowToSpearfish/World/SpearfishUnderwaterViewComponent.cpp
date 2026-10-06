#include "World/SpearfishUnderwaterViewComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Core/SpearfishSettings.h"
#include "Data/SpearfishDefinitions.h"
#include "DayNight/SpearfishSkyController.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "Rules/SpearfishRulesTypes.h"
#include "World/SpearfishOceanSubsystem.h"
#include "World/SpearfishVisualSubsystem.h"

namespace SpearfishUnderwaterPrivate
{
	constexpr int32 SnowCount = 360;
	constexpr float SnowBox = 1400.f;
}

USpearfishUnderwaterViewComponent::USpearfishUnderwaterViewComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void USpearfishUnderwaterViewComponent::BeginPlay()
{
	Super::BeginPlay();
	const USpearfishSettings* Settings = USpearfishSettings::Get();
	if (!Settings->UnderwaterPostProcessMaterial.IsNull())
	{
		PostProcessMaterial = Settings->UnderwaterPostProcessMaterial.LoadSynchronous();
	}
}

void USpearfishUnderwaterViewComponent::EnsureSnow()
{
	if (Snow)
	{
		return;
	}
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	AActor* Owner = GetOwner();
	if (!Visuals || !Owner)
	{
		return;
	}
	Snow = NewObject<UInstancedStaticMeshComponent>(Owner);
	Snow->SetStaticMesh(Visuals->GetShape(ESpearfishShape::Sphere));
	Snow->SetMaterial(0, Visuals->GetColorMaterial(FLinearColor(0.85f, 0.9f, 0.85f), 0.3f));
	Snow->SetUsingAbsoluteLocation(true);
	Snow->SetUsingAbsoluteRotation(true);
	Snow->SetUsingAbsoluteScale(true);
	Snow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Snow->SetCastShadow(false);
	Snow->SetupAttachment(Owner->GetRootComponent());
	Snow->RegisterComponent();

	FRandomStream Rng(17);
	for (int32 Index = 0; Index < SpearfishUnderwaterPrivate::SnowCount; ++Index)
	{
		const float Half = SpearfishUnderwaterPrivate::SnowBox * 0.5f;
		SnowOffsets.Add(SpearfishRandom::Vector(Rng, -Half, Half, -Half, Half));
		const float Size = Rng.FRandRange(0.25f, 0.9f) / 100.f;
		Snow->AddInstance(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(Size)), true);
	}
}

void USpearfishUnderwaterViewComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled() || !Camera)
	{
		return;
	}

	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	const float SeaLevel = Ocean ? Ocean->GetSeaLevel() : 0.f;
	const FVector CameraLocation = Camera->GetComponentLocation();
	const float DepthM = (SeaLevel - static_cast<float>(CameraLocation.Z)) / 100.f;
	const float TargetBlend = FMath::Clamp((DepthM + 0.05f) / 0.15f, 0.f, 1.f);
	UnderwaterBlend = TargetBlend;

	FLinearColor Shallow(0.08f, 0.62f, 0.68f);
	FLinearColor Deep(0.01f, 0.07f, 0.2f);
	float BaseFog = 0.016f;
	if (const FSpearfishRegionDef* Region = Ocean ? Ocean->GetRegionDef() : nullptr)
	{
		Shallow = Region->Palette.ShallowWater;
		Deep = Region->Palette.DeepWater;
		BaseFog = Region->Palette.UnderwaterFog;
	}
	if (ASpearfishSkyController* Sky = ASpearfishSkyController::Get(this))
	{
		Sky->SetUnderwaterState(UnderwaterBlend, FMath::Max(DepthM, 0.f), Shallow, Deep, BaseFog, VisibilityBonus);
	}

	ApplyPostProcess(FMath::Max(DepthM, 0.f));

	EnsureSnow();
	if (Snow)
	{
		const bool bShow = UnderwaterBlend > 0.5f;
		if (Snow->IsVisible() != bShow)
		{
			Snow->SetVisibility(bShow);
		}
		if (bShow)
		{
			UpdateSnow(CameraLocation, DeltaTime);
		}
	}
}

void USpearfishUnderwaterViewComponent::UpdateSnow(const FVector& CameraLocation, float DeltaTime)
{
	SnowTime += DeltaTime;
	const float Box = SpearfishUnderwaterPrivate::SnowBox;
	const float Half = Box * 0.5f;
	TArray<FTransform> Transforms;
	Transforms.Reserve(SnowOffsets.Num());
	for (int32 Index = 0; Index < SnowOffsets.Num(); ++Index)
	{
		FVector& Offset = SnowOffsets[Index];
		// Slow sinking drift with a little sway.
		Offset.Z -= DeltaTime * 4.f;
		Offset.X += FMath::Sin(SnowTime * 0.3f + static_cast<float>(Index)) * DeltaTime * 3.f;

		// Wrap into a box that follows the camera so particles never run out.
		FVector World = Offset;
		World.X = FMath::Fmod(Offset.X - CameraLocation.X + Half * 1000.0, static_cast<double>(Box)) - Half + CameraLocation.X;
		World.Y = FMath::Fmod(Offset.Y - CameraLocation.Y + Half * 1000.0, static_cast<double>(Box)) - Half + CameraLocation.Y;
		World.Z = FMath::Fmod(Offset.Z - CameraLocation.Z + Half * 1000.0, static_cast<double>(Box)) - Half + CameraLocation.Z;

		const float Size = (0.3f + 0.6f * static_cast<float>((Index * 37) % 10) / 10.f) / 100.f;
		Transforms.Add(FTransform(FRotator::ZeroRotator, World, FVector(Size)));
	}
	Snow->BatchUpdateInstancesTransforms(0, Transforms, true, true, true);
}

void USpearfishUnderwaterViewComponent::ApplyPostProcess(float DepthM)
{
	FPostProcessSettings& PP = Camera->PostProcessSettings;
	Camera->PostProcessBlendWeight = 1.f;

	const float U = UnderwaterBlend;
	const float DepthAlpha = FMath::Clamp(DepthM / 40.f, 0.f, 1.f);

	// Water absorbs red first: pull red down and push blue/green up as you go deeper.
	PP.bOverride_ColorGain = true;
	PP.ColorGain = FVector4(FMath::Lerp(1.0, 0.55 - 0.25 * DepthAlpha, static_cast<double>(U)),
		FMath::Lerp(1.0, 0.95 - 0.1 * DepthAlpha, static_cast<double>(U)),
		FMath::Lerp(1.0, 1.08, static_cast<double>(U)), 1.0);
	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(1.0, 1.0, 1.0, FMath::Lerp(1.0, 0.85 - 0.25 * DepthAlpha, static_cast<double>(U)));

	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = FMath::Lerp(0.675f, 1.4f, U);

	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = FMath::Lerp(0.4f, 0.75f + 0.3f * DepthAlpha, U) + Danger * 0.6f + Blackout * 2.f;

	PP.bOverride_SceneFringeIntensity = true;
	PP.SceneFringeIntensity = U * 0.8f;

	PP.bOverride_SceneColorTint = true;
	PP.SceneColorTint = FMath::Lerp(FLinearColor::White, FLinearColor(1.f, 0.55f, 0.55f), Danger * 0.6f) * (1.f - Blackout * 0.95f);

	if (PostProcessMaterial)
	{
		FWeightedBlendable* Existing = PP.WeightedBlendables.Array.FindByPredicate([this](const FWeightedBlendable& Blendable)
		{
			return Blendable.Object == PostProcessMaterial;
		});
		if (Existing)
		{
			Existing->Weight = U;
		}
		else
		{
			PP.WeightedBlendables.Array.Add(FWeightedBlendable(U, PostProcessMaterial));
		}
	}
}
