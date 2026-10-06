#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "SpearfishUnderwaterViewComponent.generated.h"

class UCameraComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;

/**
 * Local-only underwater presentation for the locally controlled character: depth-driven colour grading,
 * vignette and bloom on the camera, fog/light-shaft blending through the sky controller, drifting marine
 * snow around the lens, and an optional post-process material (caustics / distortion) when authored.
 * Mask upgrades push visibility further.
 */
UCLASS(ClassGroup = (Spearfish), meta = (BlueprintSpawnableComponent))
class HOWTOSPEARFISH_API USpearfishUnderwaterViewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpearfishUnderwaterViewComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetCamera(UCameraComponent* InCamera) { Camera = InCamera; }
	void SetVisibilityBonus(float InBonus) { VisibilityBonus = FMath::Clamp(InBonus, 0.f, 1.f); }

	/** Extra red vignette for low air / blackout (0..1). */
	void SetDangerAmount(float InDanger) { Danger = FMath::Clamp(InDanger, 0.f, 1.f); }
	void SetBlackoutAmount(float InBlackout) { Blackout = FMath::Clamp(InBlackout, 0.f, 1.f); }

	float GetUnderwaterBlend() const { return UnderwaterBlend; }

private:
	void EnsureSnow();
	void UpdateSnow(const FVector& CameraLocation, float DeltaTime);
	void ApplyPostProcess(float DepthM);

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Snow;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PostProcessMaterial;

	TArray<FVector> SnowOffsets;
	float UnderwaterBlend = 0.f;
	float VisibilityBonus = 0.f;
	float Danger = 0.f;
	float Blackout = 0.f;
	float SnowTime = 0.f;
	bool bActive = false;
};
