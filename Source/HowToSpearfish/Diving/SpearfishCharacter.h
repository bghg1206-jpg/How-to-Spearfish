#pragma once

#include "Core/SpearfishGameTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Rules/SpearfishRulesTypes.h"
#include "SpearfishCharacter.generated.h"

class ASpearfishBoat;
class ASpearfishPlayerState;
class UCameraComponent;
class USpearfishDiveBagComponent;
class USpearfishDiverBodyComponent;
class USpearfishEquipmentComponent;
class USpearfishInteractionComponent;
class USpearfishMovementComponent;
class USpearfishOxygenComponent;
class USpearfishUnderwaterViewComponent;
class USpeargunComponent;
class USpotLightComponent;
struct FInputActionValue;

/**
 * The player body for both roles. First-person, swims in full 3D, walks the boat deck, drives the boat,
 * sleeps in a bunk. Role (from the PlayerState) decides the kit: divers wear the crew's gear and carry the
 * speargun; chefs get a snorkel, apron and the tablet. All gameplay state is server-authoritative.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	explicit ASpearfishCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// --- Components -----------------------------------------------------------------------------
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
	USpearfishMovementComponent* GetSpearfishMovement() const;
	USpearfishOxygenComponent* GetOxygen() const { return Oxygen; }
	USpearfishDiveBagComponent* GetBag() const { return Bag; }
	USpearfishEquipmentComponent* GetEquipment() const { return Equipment; }
	USpeargunComponent* GetSpeargun() const { return Speargun; }
	USpearfishInteractionComponent* GetInteraction() const { return Interaction; }
	USpearfishDiverBodyComponent* GetBody() const { return Body; }
	USpearfishUnderwaterViewComponent* GetUnderwaterView() const { return UnderwaterView; }

	// --- State queries --------------------------------------------------------------------------
	ASpearfishPlayerState* GetSpearfishPlayerState() const;
	FString GetPlayerName() const;
	ESpearfishRole GetRole() const;
	bool IsSwimming() const;
	bool IsHeadUnderwater() const;
	float GetDepthMeters() const;
	bool IsBlackedOut() const { return bBlackedOut; }
	bool IsDriving() const { return DrivingBoat != nullptr; }
	ASpearfishBoat* GetDrivingBoat() const { return DrivingBoat; }
	bool IsInBed() const { return bInBed; }
	bool IsLightOn() const { return bLightOn; }
	ASpearfishCharacter* GetTetheredBy() const { return TetheredBy; }
	/** Diver kit, not blacked out, not driving, not asleep, no blocking UI. */
	bool CanUseGear() const;
	bool CanInteractNow() const;

	FVector GetEyeLocation() const { return GetPawnViewLocation(); }
	FRotator GetAimRotation() const { return GetBaseAimRotation(); }
	FVector GetAimDirection() const { return GetBaseAimRotation().Vector(); }

	// --- Server API -----------------------------------------------------------------------------
	/** Re-applies equipment for the current role and the crew's equipped kit. */
	void ApplyRoleLoadout(bool bRefillAir);
	void SetTetheredBy(ASpearfishCharacter* Shooter);
	void StartDriving(ASpearfishBoat* Boat);
	void StopDriving();
	void EnterBed(const FTransform& LieTransform);
	void LeaveBed(const FTransform& ExitTransform);
	/** Teleport onto the boat deck / ladder top (walking) or into the water (swimming). */
	void TeleportForGameplay(const FTransform& Where, bool bInWater);
	void ClimbAboard(ASpearfishBoat* Boat);
	void EnterWater(ASpearfishBoat* Boat);
	void Rescue(const FTransform& Where);
	/** Sends a private notification to this character's player. */
	void NotifyOwner(const FText& Text, ESpearfishNoticeType Type);

	UFUNCTION(Server, Unreliable)
	void ServerSetBoatInput(float Throttle, float Steer);

	UFUNCTION(Server, Reliable)
	void ServerLeaveHelm();

	UFUNCTION(Server, Reliable)
	void ServerToggleLight();

	UFUNCTION(Server, Reliable)
	void ServerShakeOffLine();

	UFUNCTION(Server, Reliable)
	void ServerLeaveBed();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Input handlers
	void Input_Move(const FInputActionValue& Value);
	void Input_MoveStopped(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_AscendStarted(const FInputActionValue& Value);
	void Input_AscendStopped(const FInputActionValue& Value);
	void Input_DescendStarted(const FInputActionValue& Value);
	void Input_DescendStopped(const FInputActionValue& Value);
	void Input_SprintStarted(const FInputActionValue& Value);
	void Input_SprintStopped(const FInputActionValue& Value);
	void Input_Fire(const FInputActionValue& Value);
	void Input_ReelStarted(const FInputActionValue& Value);
	void Input_ReelStopped(const FInputActionValue& Value);
	void Input_Release(const FInputActionValue& Value);
	void Input_Interact(const FInputActionValue& Value);
	void Input_ToggleLight(const FInputActionValue& Value);

	void OnEquipmentChanged();
	void OnRoleChanged(ESpearfishRole NewRole);
	void OnCrewLoadoutChanged();
	void HandleBlackout();
	void FinishBlackout();
	void UpdateExternalForces(float DeltaSeconds);
	void UpdateLocalPresentation(float DeltaSeconds);
	void SendBoatInput(float DeltaSeconds);
	bool IsUIBlockingInput() const;

	UFUNCTION()
	void OnRep_BlackedOut();

	UFUNCTION()
	void OnRep_DrivingBoat();

	UFUNCTION()
	void OnRep_Light();

	UPROPERTY(VisibleAnywhere, Category = "Spearfish")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, Category = "Spearfish")
	TObjectPtr<USpotLightComponent> DiveLight;

	UPROPERTY(VisibleAnywhere, Category = "Spearfish")
	TObjectPtr<USpearfishOxygenComponent> Oxygen;

	UPROPERTY(VisibleAnywhere, Category = "Spearfish")
	TObjectPtr<USpearfishDiveBagComponent> Bag;

	UPROPERTY(VisibleAnywhere, Category = "Spearfish")
	TObjectPtr<USpearfishEquipmentComponent> Equipment;

	UPROPERTY(VisibleAnywhere, Category = "Spearfish")
	TObjectPtr<USpeargunComponent> Speargun;

	UPROPERTY(VisibleAnywhere, Category = "Spearfish")
	TObjectPtr<USpearfishInteractionComponent> Interaction;

	UPROPERTY(VisibleAnywhere, Category = "Spearfish")
	TObjectPtr<USpearfishDiverBodyComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Spearfish")
	TObjectPtr<USpearfishUnderwaterViewComponent> UnderwaterView;

	UPROPERTY(ReplicatedUsing = OnRep_BlackedOut)
	bool bBlackedOut = false;

	UPROPERTY(ReplicatedUsing = OnRep_DrivingBoat)
	TObjectPtr<ASpearfishBoat> DrivingBoat;

	UPROPERTY(Replicated)
	bool bInBed = false;

	UPROPERTY(ReplicatedUsing = OnRep_Light)
	bool bLightOn = false;

	UPROPERTY(Replicated)
	TObjectPtr<ASpearfishCharacter> TetheredBy;

	// Local input state
	FVector2D MoveInput = FVector2D::ZeroVector;
	bool bAscendHeld = false;
	bool bDescendHeld = false;
	float BoatInputTimer = 0.f;
	FVector2D LastSentBoatInput = FVector2D::ZeroVector;
	float BlackoutFade = 0.f;
	float YankCooldown = 0.f;
	FDelegateHandle RoleChangedHandle;
	FDelegateHandle LoadoutChangedHandle;
	FTimerHandle RescueTimer;
};
