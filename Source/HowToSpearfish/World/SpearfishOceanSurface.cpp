#include "World/SpearfishOceanSurface.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "World/SpearfishOceanSubsystem.h"
#include "World/SpearfishVisualSubsystem.h"

ASpearfishOceanSurface::ASpearfishOceanSurface()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void ASpearfishOceanSurface::BeginPlay()
{
	Super::BeginPlay();
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	if (!Visuals)
	{
		return;
	}

	const FVector SheetSize(Size, Size, 1.f);
	TopSheet = Visuals->AddPart(this, Root, ESpearfishShape::Plane, FVector::ZeroVector, FRotator::ZeroRotator, SheetSize, FLinearColor(0.05f, 0.35f, 0.45f), false);
	// Rolled 180 degrees so its face points down at swimmers.
	Underside = Visuals->AddPart(this, Root, ESpearfishShape::Plane, FVector(0, 0, -1), FRotator(0, 0, 180), SheetSize, FLinearColor(0.55f, 0.85f, 0.9f), false);

	for (UStaticMeshComponent* Sheet : { TopSheet.Get(), Underside.Get() })
	{
		Sheet->SetCastShadow(false);
		Sheet->bAffectDistanceFieldLighting = false;
		Sheet->SetAffectDynamicIndirectLighting(false);
	}
	ApplyPalette(FLinearColor(0.08f, 0.62f, 0.68f), FLinearColor(0.01f, 0.07f, 0.2f));
}

void ASpearfishOceanSurface::ApplyPalette(const FLinearColor& Shallow, const FLinearColor& Deep)
{
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	if (!Visuals || !TopSheet || !Underside)
	{
		return;
	}
	TopSheet->SetMaterial(0, Visuals->GetWaterMaterial(false, FMath::Lerp(Deep, Shallow, 0.45f)));
	Underside->SetMaterial(0, Visuals->GetWaterMaterial(true, FMath::Lerp(Shallow, FLinearColor::White, 0.45f)));
}

void ASpearfishOceanSurface::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	WaveTime += DeltaSeconds;

	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	const float SeaLevel = Ocean ? Ocean->GetSeaLevel() : 0.f;

	FVector Focus = FVector::ZeroVector;
	if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		Focus = CameraManager->GetCameraLocation();
	}
	// Snap to a coarse grid so the sheet does not visibly slide with the camera.
	const double Grid = 2000.0;
	const FVector Snapped(FMath::FloorToDouble(Focus.X / Grid) * Grid, FMath::FloorToDouble(Focus.Y / Grid) * Grid, SeaLevel);
	SetActorLocation(Snapped);
}
