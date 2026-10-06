#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SpearfishMovementComponent.generated.h"

/**
 * Free 3D swimming built on the predicted MOVE_Flying mode (no water volumes needed: the sea is a plane).
 *  - Walking/falling below wading depth switches to swimming; touching a shallow beach stands you up.
 *  - Swimming is momentum-heavy with gentle depth-dependent buoyancy and a hard ceiling at the surface.
 *  - Sprint ("power kick") is a predicted compressed flag. Fins, carried weight and the harpoon line
 *    (external acceleration) shape the feel.
 */
UCLASS()
class HOWTOSPEARFISH_API USpearfishMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	USpearfishMovementComponent();

	// --- Tuning (applied identically on every machine from the replicated loadout) -------------
	void SetSwimTuning(float InSwimSpeed, float InSprintMultiplier, float InAcceleration);
	void SetLoadMultiplier(float InMultiplier) { LoadMultiplier = FMath::Clamp(InMultiplier, 0.3f, 1.f); }

	/** Line pulls and currents for this frame (cm/s^2). Not predicted; both sides apply the same value. */
	void SetExternalAcceleration(const FVector& InAcceleration) { ExternalAcceleration = InAcceleration; }
	const FVector& GetExternalAcceleration() const { return ExternalAcceleration; }

	/** Local input. */
	void SetWantsToSprint(bool bInWantsToSprint) { bWantsToSprint = bInWantsToSprint; }
	bool WantsToSprint() const { return bWantsToSprint; }

	bool IsSwimming() const { return MovementMode == MOVE_Flying; }
	bool IsSprintSwimming() const { return IsSwimming() && bWantsToSprint && Velocity.SizeSquared() > 100.0; }
	bool IsAtSurface() const;
	float GetDepthMeters() const;
	float GetSwimSpeedFraction() const;
	float GetSeaLevel() const;

	// --- UCharacterMovementComponent ------------------------------------------------------------
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual bool CanAttemptJump() const override;

	/** Water deeper than this at the feet makes you swim instead of wade. */
	UPROPERTY(EditAnywhere, Category = "Swimming")
	float WadeDepth = 115.f;

	/** Capsule centre below sea level while floating at the surface (eyes just above water). */
	UPROPERTY(EditAnywhere, Category = "Swimming")
	float SurfaceFloatDepth = 45.f;

	/** Upward drift near the surface (cm/s^2), turning slightly negative at depth. */
	UPROPERTY(EditAnywhere, Category = "Swimming")
	float SurfaceBuoyancy = 14.f;

	UPROPERTY(EditAnywhere, Category = "Swimming")
	float DeepBuoyancy = -10.f;

	uint8 bWantsToSprint : 1;

protected:
	virtual void PhysFlying(float DeltaTime, int32 Iterations) override;

private:
	float GetSurfaceZ() const;

	float SwimSpeed = 330.f;
	float SprintMultiplier = 1.6f;
	float SwimAcceleration = 520.f;
	float LoadMultiplier = 1.f;
	FVector ExternalAcceleration = FVector::ZeroVector;
};

class HOWTOSPEARFISH_API FSavedMove_Spearfish : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;

	uint8 bSavedWantsToSprint : 1;
};

class HOWTOSPEARFISH_API FNetworkPredictionData_Client_Spearfish : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	explicit FNetworkPredictionData_Client_Spearfish(const UCharacterMovementComponent& ClientMovement);

	virtual FSavedMovePtr AllocateNewMove() override;
};
