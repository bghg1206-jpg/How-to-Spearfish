#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpearfishOceanSurface.generated.h"

class UStaticMeshComponent;

/**
 * Local visual sea surface: a top sheet seen from above and an underside sheet seen from below, both
 * following the local camera so the ocean is endless. Gameplay never reads it (the sea level lives in
 * USpearfishOceanSubsystem). Uses the authored ocean materials when present.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishOceanSurface : public AActor
{
	GENERATED_BODY()

public:
	ASpearfishOceanSurface();

	virtual void Tick(float DeltaSeconds) override;

	void ApplyPalette(const FLinearColor& Shallow, const FLinearColor& Deep);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Ocean")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> TopSheet;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Underside;

	float Size = 160000.f;
	float WaveTime = 0.f;
};
