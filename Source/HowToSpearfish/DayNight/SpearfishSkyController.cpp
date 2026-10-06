#include "DayNight/SpearfishSkyController.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Core/SpearfishGameState.h"
#include "DayNight/SpearfishDayCycleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ASpearfishSkyController::ASpearfishSkyController()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetAtmosphereSunLight(true);
	Sun->SetAtmosphereSunLightIndex(0);
	Sun->SetIntensity(10.f);
	Sun->SetCastShadows(true);
	Sun->SetEnableLightShaftBloom(true);
	Sun->SetVolumetricScatteringIntensity(1.f);

	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetupAttachment(Root);
	Moon->SetMobility(EComponentMobility::Movable);
	Moon->SetAtmosphereSunLight(true);
	Moon->SetAtmosphereSunLightIndex(1);
	Moon->SetIntensity(0.25f);
	Moon->SetLightColor(FLinearColor(0.55f, 0.65f, 1.f));
	Moon->SetCastShadows(false);

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(Root);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;
	SkyLight->SourceType = SLS_CapturedScene;
	SkyLight->SetIntensity(1.f);

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(Root);
	Fog->SetVolumetricFog(true);
	Fog->SetFogDensity(0.002f);
	Fog->SetFogHeightFalloff(0.2f);
	Fog->SetVolumetricFogScatteringDistribution(0.6f);
}

ASpearfishSkyController* ASpearfishSkyController::Get(const UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ASpearfishSkyController> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void ASpearfishSkyController::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(FVector::ZeroVector);
	UpdateCelestial(9.f);
	UpdateFog();
}

void ASpearfishSkyController::SetUnderwaterState(float InBlend, float InDepthM, const FLinearColor& InShallowColor, const FLinearColor& InDeepColor,
	float InBaseFog, float InVisibilityBonus)
{
	UnderwaterBlend = FMath::Clamp(InBlend, 0.f, 1.f);
	UnderwaterDepthM = FMath::Max(InDepthM, 0.f);
	ShallowColor = InShallowColor;
	DeepColor = InDeepColor;
	BaseFog = InBaseFog;
	VisibilityBonus = InVisibilityBonus;
}

void ASpearfishSkyController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	float Hour = 9.f;
	if (const ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>())
	{
		if (const USpearfishDayCycleComponent* DayCycle = GameState->GetDayCycle())
		{
			Hour = DayCycle->GetHour();
		}
	}
	UpdateCelestial(Hour);
	UpdateFog();
}

void ASpearfishSkyController::UpdateCelestial(float Hour)
{
	// Sunrise 06:00 in the east (+X), sunset 18:00 in the west.
	const float DayAngle = (FMath::Fmod(Hour, 24.f) - 6.f) / 12.f * 180.f;
	SunElevation = FMath::Sin(FMath::DegreesToRadians(DayAngle)) * 70.f;
	Daylight = FMath::Clamp(SunElevation / 25.f, 0.f, 1.f);

	// Directional lights shine along their forward vector, so pitch is negative elevation.
	Sun->SetWorldRotation(FRotator(-SunElevation, 180.f - DayAngle * 0.6f, 0.f));
	Moon->SetWorldRotation(FRotator(-FMath::Max(25.f, -SunElevation), -DayAngle * 0.6f, 0.f));

	const float Warmth = 1.f - FMath::Clamp((SunElevation - 4.f) / 30.f, 0.f, 1.f);
	Sun->SetLightColor(FMath::Lerp(FLinearColor(1.f, 0.97f, 0.92f), FLinearColor(1.f, 0.55f, 0.3f), Warmth));
	Sun->SetIntensity(SunElevation > -2.f ? FMath::Lerp(0.5f, 10.f, Daylight) : 0.f);
	Moon->SetIntensity(SunElevation < 5.f ? 0.35f : 0.f);
	SkyLight->SetIntensity(FMath::Lerp(0.25f, 1.f, Daylight));
}

void ASpearfishSkyController::UpdateFog()
{
	// Above water: light atmospheric haze. Underwater: dense, coloured, darker with depth.
	const float DepthAlpha = FMath::Clamp(UnderwaterDepthM / 40.f, 0.f, 1.f);
	const FLinearColor WaterColor = FMath::Lerp(ShallowColor, DeepColor, DepthAlpha) * FMath::Lerp(0.35f, 1.f, Daylight);
	const float WaterDensity = FMath::Max(0.004f, (BaseFog * 2.2f + UnderwaterDepthM * 0.0009f) * (1.f - 0.45f * VisibilityBonus));

	const float AirDensity = 0.0025f;
	Fog->SetFogDensity(FMath::Lerp(AirDensity, WaterDensity, UnderwaterBlend));
	Fog->SetFogHeightFalloff(FMath::Lerp(0.2f, 0.0005f, UnderwaterBlend));
	Fog->SetFogInscatteringColor(FMath::Lerp(FLinearColor(0.45f, 0.6f, 0.8f) * FMath::Max(Daylight, 0.1f), WaterColor, UnderwaterBlend));
	Fog->SetStartDistance(FMath::Lerp(1500.f, 0.f, UnderwaterBlend));
	Fog->SetVolumetricFogExtinctionScale(FMath::Lerp(1.f, 3.f, UnderwaterBlend));
	Fog->SetFogMaxOpacity(FMath::Lerp(0.85f, 1.f, UnderwaterBlend));
	Sun->SetVolumetricScatteringIntensity(FMath::Lerp(1.f, 4.f, UnderwaterBlend));
}
