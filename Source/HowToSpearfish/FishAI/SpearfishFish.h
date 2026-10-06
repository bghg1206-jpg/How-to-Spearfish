#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GameFramework/Actor.h"
#include "Interaction/SpearfishInteractable.h"
#include "Rules/FishRules.h"
#include "SpearfishFish.generated.h"

class ASpearfishCharacter;
class USphereComponent;
class USpearfishFishBodyComponent;
struct FSpearfishFishSpeciesDef;

/** Compact movement snapshot replicated to clients, which interpolate toward it. */
USTRUCT()
struct FSpearfishFishNetState
{
	GENERATED_BODY()

	UPROPERTY()
	FVector_NetQuantize10 Location;

	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY()
	float Speed = 0.f;

	UPROPERTY()
	ESpearfishFishMind Mind = ESpearfishFishMind::Wander;
};

/**
 * One fish (or hazard animal). Server simulates a mind state (Rules/FishRules DecideMind) and per-state
 * steering with school boids, threat avoidance, cover seeking, obstacle feelers and depth keeping. When
 * speared it fights the line: panics, bursts, zigzags toward cover, drags the diver and burns stamina
 * until exhausted - it never just "dies". Predators hunt prey and steal struggling catches; morays lunge
 * from dens; jellyfish sting on contact.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishFish : public AActor, public ISpearfishInteractable
{
	GENERATED_BODY()

public:
	ASpearfishFish();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	/** Server: configure before FinishSpawning (or right after spawn). */
	void InitializeFish(FName InSpeciesId, float InLengthCm, const FVector& InHome, int32 InSchoolId, int32 InSeed);

	// --- Queries ----------------------------------------------------------------------------
	const FSpearfishFishSpeciesDef* GetSpecies() const;
	FName GetSpeciesId() const { return SpeciesId; }
	float GetLengthCm() const { return LengthCm; }
	float GetWeightKg() const;
	float GetSizeFraction() const;
	float GetStaminaFraction() const { return MaxStamina > 0.f ? FMath::Clamp(Stamina / MaxStamina, 0.f, 1.f) : 0.f; }
	ESpearfishFishMind GetMind() const { return NetState.Mind; }
	bool IsHooked() const { return bHooked; }
	bool IsExhausted() const { return NetState.Mind == ESpearfishFishMind::Exhausted; }
	bool IsCatchable() const;
	ASpearfishCharacter* GetHookedBy() const { return HookedBy; }
	int32 GetSchoolId() const { return SchoolId; }
	FVector GetFishVelocity() const { return Velocity; }

	// --- Line interaction (server) ----------------------------------------------------------
	bool TryHook(ASpearfishCharacter* Shooter, const FHitResult& Hit, float GunPower, float& OutShotQuality);
	void Provoke(ASpearfishCharacter* By);
	float GetLinePullAway(const FVector& LineDirectionFromAnchor) const;
	void ApplyLineConstraint(const FVector& Anchor, float MaxDistance, float Tension, float ReelSpeed, float DeltaSeconds);
	void ReleaseFromLine(bool bEscaped);
	void OnCaught();
	void GetEaten(ASpearfishFish* Predator);
	void Startle(const FVector& From);

	// --- ISpearfishInteractable -------------------------------------------------------------
	virtual bool CanInteract(const ASpearfishCharacter* Character) const override;
	virtual FText GetInteractPrompt(const ASpearfishCharacter* Character) const override;
	virtual void Interact(ASpearfishCharacter* Character) override;
	virtual float GetInteractRange() const override { return 320.f; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnRep_Identity();

	void SetupFromSpecies();
	void SimulateServer(float DeltaSeconds);
	void Think();
	FVector ComputeDesiredDirection(float& OutSpeed);
	void Integrate(float DeltaSeconds, const FVector& DesiredDirection, float DesiredSpeed);
	void KeepInWater(FVector& Location) const;
	void TryAttackDiver(ASpearfishCharacter* Diver, float Distance);
	void SetMind(ESpearfishFishMind NewMind);
	void UpdateNetState();
	void SmoothClient(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, Category = "Fish")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Fish")
	TObjectPtr<USpearfishFishBodyComponent> Body;

	UPROPERTY(ReplicatedUsing = OnRep_Identity)
	FName SpeciesId;

	UPROPERTY(ReplicatedUsing = OnRep_Identity)
	float LengthCm = 40.f;

	UPROPERTY(Replicated)
	FSpearfishFishNetState NetState;

	UPROPERTY(Replicated)
	TObjectPtr<ASpearfishCharacter> HookedBy;

	UPROPERTY(Replicated)
	bool bHooked = false;

	// --- Server simulation ------------------------------------------------------------------
	const FSpearfishFishSpeciesDef* SpeciesCache = nullptr;
	FVector Velocity = FVector::ZeroVector;
	FVector Home = FVector::ZeroVector;
	FVector WanderTarget = FVector::ZeroVector;
	FVector FleeDirection = FVector::ForwardVector;
	FVector ThreatLocation = FVector::ZeroVector;
	FVector CoverPoint = FVector::ZeroVector;
	FVector AvoidNormal = FVector::ZeroVector;
	FVector LineAnchor = FVector::ZeroVector;
	FVector EscapeDirection = FVector::ForwardVector;
	TWeakObjectPtr<ASpearfishFish> PreyTarget;
	TWeakObjectPtr<ASpearfishCharacter> DiverTarget;
	TWeakObjectPtr<ASpearfishCharacter> ProvokedBy;
	ESpearfishFishMind Mind = ESpearfishFishMind::Wander;
	int32 SchoolId = INDEX_NONE;
	float TimeInState = 0.f;
	float NextThink = 0.f;
	float NextZigzag = 0.f;
	float Stamina = 5.f;
	float MaxStamina = 5.f;
	float ProvokedUntil = 0.f;
	float AttackCooldownUntil = 0.f;
	float LastTension = 0.f;
	float CurrentSpeed = 0.f;
	float ThreatDistance = UE_BIG_NUMBER;
	bool bHasCover = false;
	bool bThreatIsDiver = false;
	bool bCaught = false;
	FRandomStream Stream;

	// --- Client smoothing -------------------------------------------------------------------
	FVector SmoothedVelocity = FVector::ZeroVector;
};
