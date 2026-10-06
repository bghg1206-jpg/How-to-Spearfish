#include "Diving/SpearfishMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "World/SpearfishOceanSubsystem.h"

USpearfishMovementComponent::USpearfishMovementComponent()
{
	bWantsToSprint = false;

	// Deck walking.
	MaxWalkSpeed = 420.f;
	MaxWalkSpeedCrouched = 220.f;
	JumpZVelocity = 420.f;
	AirControl = 0.35f;
	GravityScale = 1.f;
	bCanWalkOffLedges = true;

	// Swimming (flying mode) feel: momentum and water drag.
	MaxFlySpeed = SwimSpeed;
	BrakingDecelerationFlying = 380.f;
	bOrientRotationToMovement = false;

	NavAgentProps.bCanFly = true;
	NavAgentProps.bCanSwim = true;
}

void USpearfishMovementComponent::SetSwimTuning(float InSwimSpeed, float InSprintMultiplier, float InAcceleration)
{
	SwimSpeed = FMath::Max(InSwimSpeed, 50.f);
	SprintMultiplier = FMath::Max(InSprintMultiplier, 1.f);
	SwimAcceleration = FMath::Max(InAcceleration, 100.f);
	MaxFlySpeed = SwimSpeed;
}

float USpearfishMovementComponent::GetSeaLevel() const
{
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	return Ocean ? Ocean->GetSeaLevel() : 0.f;
}

float USpearfishMovementComponent::GetSurfaceZ() const
{
	return GetSeaLevel() - SurfaceFloatDepth;
}

bool USpearfishMovementComponent::IsAtSurface() const
{
	return UpdatedComponent && UpdatedComponent->GetComponentLocation().Z >= GetSurfaceZ() - 25.f;
}

float USpearfishMovementComponent::GetDepthMeters() const
{
	if (!UpdatedComponent)
	{
		return 0.f;
	}
	return FMath::Max(0.f, (GetSeaLevel() - static_cast<float>(UpdatedComponent->GetComponentLocation().Z)) / 100.f);
}

float USpearfishMovementComponent::GetSwimSpeedFraction() const
{
	return SwimSpeed > 0.f ? FMath::Clamp(static_cast<float>(Velocity.Size()) / SwimSpeed, 0.f, 1.f) : 0.f;
}

float USpearfishMovementComponent::GetMaxSpeed() const
{
	if (MovementMode == MOVE_Flying)
	{
		const float Sprint = bWantsToSprint ? SprintMultiplier : 1.f;
		return SwimSpeed * LoadMultiplier * Sprint;
	}
	return Super::GetMaxSpeed();
}

float USpearfishMovementComponent::GetMaxAcceleration() const
{
	if (MovementMode == MOVE_Flying)
	{
		return SwimAcceleration * (bWantsToSprint ? 1.4f : 1.f);
	}
	return Super::GetMaxAcceleration();
}

bool USpearfishMovementComponent::CanAttemptJump() const
{
	// In water the jump key is "ascend".
	return MovementMode != MOVE_Flying && Super::CanAttemptJump();
}

void USpearfishMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

void USpearfishMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	if (!UpdatedComponent || !CharacterOwner || MovementMode == MOVE_None)
	{
		return;
	}

	const FVector Location = UpdatedComponent->GetComponentLocation();
	const float HalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float SeaZ = GetSeaLevel();
	const float FeetZ = static_cast<float>(Location.Z) - HalfHeight;
	const float WaterAtFeet = SeaZ - FeetZ;

	if (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking)
	{
		if (WaterAtFeet > WadeDepth)
		{
			SetMovementMode(MOVE_Flying);
		}
	}
	else if (MovementMode == MOVE_Falling)
	{
		// Splashdown once the body is properly in the water.
		if (static_cast<float>(Location.Z) < SeaZ - SurfaceFloatDepth * 0.5f)
		{
			Velocity *= 0.35;
			SetMovementMode(MOVE_Flying);
		}
	}
	else if (MovementMode == MOVE_Flying)
	{
		if (FeetZ > SeaZ + 10.f)
		{
			SetMovementMode(MOVE_Falling);
		}
		else if (WaterAtFeet < WadeDepth * 0.8f)
		{
			// Swimming into the shallows: stand up when there is walkable ground under the feet.
			FFindFloorResult Floor;
			FindFloor(Location, Floor, false);
			if (Floor.IsWalkableFloor() && Floor.FloorDist < 25.f)
			{
				SetMovementMode(MOVE_Walking);
			}
		}
	}
}

void USpearfishMovementComponent::PhysFlying(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME || !UpdatedComponent)
	{
		return;
	}

	const float SurfaceZ = GetSurfaceZ();
	const bool bAtSurface = static_cast<float>(UpdatedComponent->GetComponentLocation().Z) >= SurfaceZ - 5.f;

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		FVector Extra = ExternalAcceleration;
		const bool bVerticalInput = FMath::Abs(Acceleration.Z) > 1.0;
		if (!bVerticalInput && !bAtSurface)
		{
			const float Alpha = FMath::Clamp(GetDepthMeters() / 20.f, 0.f, 1.f);
			Extra.Z += FMath::Lerp(SurfaceBuoyancy, DeepBuoyancy, Alpha);
		}
		Velocity += Extra * DeltaTime;
	}

	// The surface is a ceiling: you can float at it but not climb out of the sea.
	if (bAtSurface)
	{
		Acceleration.Z = FMath::Min(Acceleration.Z, 0.0);
		Velocity.Z = FMath::Min(Velocity.Z, 0.0);
	}

	Super::PhysFlying(DeltaTime, Iterations);

	const FVector NewLocation = UpdatedComponent->GetComponentLocation();
	if (static_cast<float>(NewLocation.Z) > SurfaceZ)
	{
		FHitResult Hit;
		SafeMoveUpdatedComponent(FVector(0.0, 0.0, SurfaceZ - NewLocation.Z), UpdatedComponent->GetComponentQuat(), true, Hit);
		Velocity.Z = FMath::Min(Velocity.Z, 0.0);
	}
}

FNetworkPredictionData_Client* USpearfishMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		USpearfishMovementComponent* MutableThis = const_cast<USpearfishMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Spearfish(*this);
	}
	return ClientPredictionData;
}

// --------------------------------------------------------------------------------- Saved moves

void FSavedMove_Spearfish::Clear()
{
	Super::Clear();
	bSavedWantsToSprint = false;
}

uint8 FSavedMove_Spearfish::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bSavedWantsToSprint)
	{
		Result |= FLAG_Custom_0;
	}
	return Result;
}

bool FSavedMove_Spearfish::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_Spearfish* Other = static_cast<const FSavedMove_Spearfish*>(NewMove.Get());
	if (Other && bSavedWantsToSprint != Other->bSavedWantsToSprint)
	{
		return false;
	}
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_Spearfish::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	if (const USpearfishMovementComponent* Movement = C ? Cast<USpearfishMovementComponent>(C->GetCharacterMovement()) : nullptr)
	{
		bSavedWantsToSprint = Movement->bWantsToSprint;
	}
}

void FSavedMove_Spearfish::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);
	if (USpearfishMovementComponent* Movement = C ? Cast<USpearfishMovementComponent>(C->GetCharacterMovement()) : nullptr)
	{
		Movement->bWantsToSprint = bSavedWantsToSprint;
	}
}

FNetworkPredictionData_Client_Spearfish::FNetworkPredictionData_Client_Spearfish(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_Spearfish::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Spearfish());
}
