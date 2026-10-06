#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Rules/OxygenRules.h"
#include "SpearfishOxygenComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FSpearfishBlackoutEvent);

/**
 * Server-authoritative breathing. Wraps SpearfishOxygen rules with the diver's depth, effort, wetsuit
 * rating, depth zone and status effects. Replicated to everyone: the diver's HUD shows it and the chef's
 * tablet can show it once the Dive Link upgrade is owned.
 */
UCLASS(ClassGroup = (Spearfish), meta = (BlueprintSpawnableComponent))
class HOWTOSPEARFISH_API USpearfishOxygenComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpearfishOxygenComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Server
	void ConfigureTank(float MaxAir, float SafeDepthM, float StingResist, bool bRefill);
	void Refill();
	/** Bites, stings, clam snaps. Stings also leave the diver panicky for a few seconds. */
	void ApplyShock(float Amount, bool bSting);
	void SetSuspended(bool bInSuspended) { bSuspended = bInSuspended; }

	const FSpearfishBreathState& GetBreath() const { return Breath; }
	float GetAirFraction() const { return SpearfishOxygen::AirFraction(Breath); }
	float GetConsumptionRate() const { return ConsumptionRate; }
	float GetSecondsRemaining() const { return SpearfishOxygen::SecondsRemaining(Breath, ConsumptionRate); }
	bool IsStung() const { return StungRemaining > 0.f; }
	bool IsOverDepth() const { return bOverDepth; }
	bool IsLowAir() const { return GetAirFraction() < 0.3f; }
	bool IsCritical() const { return Breath.bOutOfAir || GetAirFraction() < 0.12f; }
	float GetSafeDepthM() const { return SafeDepthM; }

	/** Server: fires once when the diver blacks out. */
	FSpearfishBlackoutEvent OnBlackout;

private:
	UPROPERTY(Replicated)
	FSpearfishBreathState Breath;

	UPROPERTY(Replicated)
	float ConsumptionRate = 0.f;

	UPROPERTY(Replicated)
	float StungRemaining = 0.f;

	UPROPERTY(Replicated)
	bool bOverDepth = false;

	UPROPERTY(Replicated)
	float SafeDepthM = 15.f;

	float StingResist = 0.f;
	bool bSuspended = false;
};
