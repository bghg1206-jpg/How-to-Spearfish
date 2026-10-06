#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpearfishAmbientSchool.generated.h"

class UInstancedStaticMeshComponent;

/**
 * Decorative baitfish school: dozens of tiny fish drawn with two instanced meshes and steered by a cheap
 * local boids model (cohesion around a home point, alignment, separation, and a flinch away from the local
 * camera). Not replicated and not catchable - it exists to make the reef feel alive at zero network cost.
 * Gameplay fish are ASpearfishFish.
 */
UCLASS(NotPlaceable)
class HOWTOSPEARFISH_API ASpearfishAmbientSchool : public AActor
{
	GENERATED_BODY()

public:
	ASpearfishAmbientSchool();

	/** Call right after spawning. */
	void Configure(const FLinearColor& Color, int32 Count, float LengthCm, int32 Seed);

	virtual void Tick(float DeltaSeconds) override;

private:
	struct FBoid
	{
		FVector Position = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Phase = 0.f;
	};

	FVector GetViewLocation() const;
	void Simulate(float DeltaSeconds, const FVector& Threat, bool bThreatNear);
	void PushTransforms();

	UPROPERTY(VisibleAnywhere, Category = "School")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Bodies;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Tails;

	TArray<FBoid> Boids;
	TArray<FTransform> BodyTransforms;
	TArray<FTransform> TailTransforms;
	FVector Home = FVector::ZeroVector;
	FVector Wander = FVector::ZeroVector;
	float WanderTimer = 0.f;
	float Length = 8.f;
	float Speed = 120.f;
	float PanicTimer = 0.f;
	float SeaLevelZ = 0.f;
	float SeabedZ = -100000.f;
	FRandomStream Stream;
};
