#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpearfishSkyController.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;

/**
 * Local (non-replicated) lighting rig spawned on every machine: sun + moon, physical sky atmosphere,
 * real-time sky light and volumetric height fog. Follows the replicated day clock and blends into the
 * underwater look (dense tinted fog, light shafts) when the local camera is below the surface.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishSkyController : public AActor
{
	GENERATED_BODY()

public:
	ASpearfishSkyController();

	virtual void Tick(float DeltaSeconds) override;

	static ASpearfishSkyController* Get(const UObject* WorldContextObject);

	/** Called each frame by the local underwater view. Blend 0 = above water, 1 = fully underwater. */
	void SetUnderwaterState(float InBlend, float InDepthM, const FLinearColor& InShallowColor, const FLinearColor& InDeepColor,
		float InBaseFog, float InVisibilityBonus);

	float GetSunElevationDegrees() const { return SunElevation; }

	/** 0 at night, 1 at noon. */
	float GetDaylight() const { return Daylight; }

protected:
	virtual void BeginPlay() override;

private:
	void UpdateCelestial(float Hour);
	void UpdateFog();

	UPROPERTY(VisibleAnywhere, Category = "Sky")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Sky")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, Category = "Sky")
	TObjectPtr<UDirectionalLightComponent> Moon;

	UPROPERTY(VisibleAnywhere, Category = "Sky")
	TObjectPtr<USkyAtmosphereComponent> Atmosphere;

	UPROPERTY(VisibleAnywhere, Category = "Sky")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, Category = "Sky")
	TObjectPtr<UExponentialHeightFogComponent> Fog;

	float UnderwaterBlend = 0.f;
	float UnderwaterDepthM = 0.f;
	FLinearColor ShallowColor = FLinearColor(0.08f, 0.62f, 0.68f);
	FLinearColor DeepColor = FLinearColor(0.01f, 0.07f, 0.2f);
	float BaseFog = 0.016f;
	float VisibilityBonus = 0.f;
	float SunElevation = 45.f;
	float Daylight = 1.f;
};
