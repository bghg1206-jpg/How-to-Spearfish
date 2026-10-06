#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpearfishHarpoon.generated.h"

class USpeargunComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ESpearfishHarpoonPhase : uint8
{
	Flying,
	Stuck,
	Returning
};

/** Launch parameters replicated once so clients simulate the flight locally (smooth, no jitter). */
USTRUCT()
struct FSpearfishHarpoonLaunch
{
	GENERATED_BODY()

	UPROPERTY()
	FVector_NetQuantize10 Origin;

	UPROPERTY()
	FVector_NetQuantize10 Velocity;
};

/**
 * The spear shaft. Server sweeps its ballistic flight (heavy drag underwater, slight sink) on the Harpoon
 * trace channel and reports hits to its speargun. Once stuck it is attached to whatever it hit, and the line
 * runs from the gun to it. Returning shafts fly back to the muzzle before reloading.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishHarpoon : public AActor
{
	GENERATED_BODY()

public:
	ASpearfishHarpoon();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostNetReceiveLocationAndRotation() override;

	/** Server. */
	void Launch(USpeargunComponent* InOwnerGun, const FVector& Velocity, float MaxRangeCm);
	void StickTo(const FHitResult& Hit);
	void StartReturn();

	ESpearfishHarpoonPhase GetPhase() const { return Phase; }
	FVector GetTipLocation() const;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Launch();

	void SimulateFlight(float DeltaSeconds, bool bAuthority);

	UPROPERTY(VisibleAnywhere, Category = "Harpoon")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(ReplicatedUsing = OnRep_Launch)
	FSpearfishHarpoonLaunch LaunchData;

	UPROPERTY(Replicated)
	ESpearfishHarpoonPhase Phase = ESpearfishHarpoonPhase::Flying;

	UPROPERTY(Transient)
	TObjectPtr<USpeargunComponent> OwnerGun;

	FVector SimVelocity = FVector::ZeroVector;
	float TravelledCm = 0.f;
	float MaxRange = 800.f;
	float FlightTime = 0.f;
	bool bLocalFlight = false;
};
