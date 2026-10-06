#include "Diving/SpearfishCharacter.h"

#include "Boat/SpearfishBoat.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Core/SpearfishGameMode.h"
#include "Core/SpearfishGameState.h"
#include "Core/SpearfishInputConfig.h"
#include "Core/SpearfishPlayerController.h"
#include "Core/SpearfishPlayerState.h"
#include "Core/SpearfishSettings.h"
#include "Diving/SpearfishDiverBodyComponent.h"
#include "Diving/SpearfishMovementComponent.h"
#include "Diving/SpearfishOxygenComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/World.h"
#include "Equipment/SpearfishEquipmentComponent.h"
#include "Interaction/SpearfishInteractionComponent.h"
#include "Inventory/SpearfishCatchService.h"
#include "Inventory/SpearfishDiveBagComponent.h"
#include "Net/UnrealNetwork.h"
#include "Progression/SpearfishProgressionComponent.h"
#include "Speargun/SpeargunComponent.h"
#include "TimerManager.h"
#include "World/SpearfishOceanSubsystem.h"
#include "World/SpearfishUnderwaterViewComponent.h"

#define LOCTEXT_NAMESPACE "SpearfishCharacter"

ASpearfishCharacter::ASpearfishCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USpearfishMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	BaseEyeHeight = 64.f;
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	GetMesh()->SetVisibility(false);

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetFieldOfView(92.f);

	DiveLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("DiveLight"));
	DiveLight->SetupAttachment(FirstPersonCamera);
	DiveLight->SetRelativeLocation(FVector(20.f, -12.f, -10.f));
	DiveLight->SetIntensityUnits(ELightUnits::Lumens);
	DiveLight->SetIntensity(0.f);
	DiveLight->SetOuterConeAngle(26.f);
	DiveLight->SetInnerConeAngle(10.f);
	DiveLight->SetAttenuationRadius(1200.f);
	DiveLight->SetVisibility(false);

	Oxygen = CreateDefaultSubobject<USpearfishOxygenComponent>(TEXT("Oxygen"));
	Bag = CreateDefaultSubobject<USpearfishDiveBagComponent>(TEXT("Bag"));
	Equipment = CreateDefaultSubobject<USpearfishEquipmentComponent>(TEXT("Equipment"));
	Speargun = CreateDefaultSubobject<USpeargunComponent>(TEXT("Speargun"));
	Interaction = CreateDefaultSubobject<USpearfishInteractionComponent>(TEXT("Interaction"));
	UnderwaterView = CreateDefaultSubobject<USpearfishUnderwaterViewComponent>(TEXT("UnderwaterView"));

	Body = CreateDefaultSubobject<USpearfishDiverBodyComponent>(TEXT("Body"));
	Body->SetupAttachment(GetCapsuleComponent());
}

void ASpearfishCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpearfishCharacter, bBlackedOut);
	DOREPLIFETIME(ASpearfishCharacter, DrivingBoat);
	DOREPLIFETIME(ASpearfishCharacter, bInBed);
	DOREPLIFETIME(ASpearfishCharacter, bLightOn);
	DOREPLIFETIME(ASpearfishCharacter, TetheredBy);
}

USpearfishMovementComponent* ASpearfishCharacter::GetSpearfishMovement() const
{
	return Cast<USpearfishMovementComponent>(GetCharacterMovement());
}

ASpearfishPlayerState* ASpearfishCharacter::GetSpearfishPlayerState() const
{
	return GetPlayerState<ASpearfishPlayerState>();
}

FString ASpearfishCharacter::GetPlayerName() const
{
	const ASpearfishPlayerState* State = GetSpearfishPlayerState();
	return State ? State->GetPlayerName() : FString(TEXT("Diver"));
}

ESpearfishRole ASpearfishCharacter::GetRole() const
{
	const ASpearfishPlayerState* State = GetSpearfishPlayerState();
	return State ? State->GetRole() : ESpearfishRole::None;
}

bool ASpearfishCharacter::IsSwimming() const
{
	const USpearfishMovementComponent* Movement = GetSpearfishMovement();
	return Movement && Movement->IsSwimming();
}

bool ASpearfishCharacter::IsHeadUnderwater() const
{
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	return Ocean && Ocean->IsUnderwater(GetEyeLocation());
}

float ASpearfishCharacter::GetDepthMeters() const
{
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	return Ocean ? FMath::Max(0.f, Ocean->GetDepthMeters(GetEyeLocation())) : 0.f;
}

bool ASpearfishCharacter::CanUseGear() const
{
	return Equipment && Equipment->GetStats().bHasSpeargun && !bBlackedOut && !IsDriving() && !bInBed;
}

bool ASpearfishCharacter::CanInteractNow() const
{
	return !bBlackedOut && !IsDriving() && !bInBed;
}

bool ASpearfishCharacter::IsUIBlockingInput() const
{
	const ASpearfishPlayerController* PlayerController = Cast<ASpearfishPlayerController>(GetController());
	return PlayerController && PlayerController->IsGameplayInputBlocked();
}

// ----------------------------------------------------------------------------- Lifecycle

void ASpearfishCharacter::BeginPlay()
{
	Super::BeginPlay();
	Equipment->OnEquipmentChanged.AddUObject(this, &ASpearfishCharacter::OnEquipmentChanged);
	UnderwaterView->SetCamera(FirstPersonCamera);
	if (HasAuthority())
	{
		Oxygen->OnBlackout.AddUObject(this, &ASpearfishCharacter::HandleBlackout);
	}
	OnEquipmentChanged();
}

void ASpearfishCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		// A diver who disconnects mid-dive hands their catch to the cooler rather than losing it.
		if (ASpearfishGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASpearfishGameState>() : nullptr)
		{
			if (Bag && !Bag->IsEmpty() && GameState->GetBoat() && EndPlayReason == EEndPlayReason::Destroyed)
			{
				SpearfishCatch::DepositAtBoat(this, GameState->GetBoat());
			}
			if (GameState->GetProgression())
			{
				GameState->GetProgression()->OnLoadoutChanged.Remove(LoadoutChangedHandle);
			}
		}
		if (IsDriving())
		{
			StopDriving();
		}
		GetWorldTimerManager().ClearTimer(RescueTimer);
	}
	if (ASpearfishPlayerState* State = GetSpearfishPlayerState())
	{
		State->OnRoleChanged.Remove(RoleChangedHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ASpearfishCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (ASpearfishPlayerState* State = GetSpearfishPlayerState())
	{
		State->OnRoleChanged.Remove(RoleChangedHandle);
		RoleChangedHandle = State->OnRoleChanged.AddUObject(this, &ASpearfishCharacter::OnRoleChanged);
	}
	if (ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>())
	{
		if (USpearfishProgressionComponent* Progression = GameState->GetProgression())
		{
			Progression->OnLoadoutChanged.Remove(LoadoutChangedHandle);
			LoadoutChangedHandle = Progression->OnLoadoutChanged.AddUObject(this, &ASpearfishCharacter::OnCrewLoadoutChanged);
		}
	}
	ApplyRoleLoadout(true);
}

void ASpearfishCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	OnEquipmentChanged();
}

void ASpearfishCharacter::OnRoleChanged(ESpearfishRole NewRole)
{
	ApplyRoleLoadout(!IsHeadUnderwater());
}

void ASpearfishCharacter::OnCrewLoadoutChanged()
{
	ApplyRoleLoadout(false);
}

void ASpearfishCharacter::ApplyRoleLoadout(bool bRefillAir)
{
	if (!HasAuthority())
	{
		return;
	}
	const ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	const USpearfishProgressionComponent* Progression = GameState ? GameState->GetProgression() : nullptr;
	const ESpearfishRole Role = GetRole();

	Equipment->ApplyLoadout(Progression ? Progression->GetLoadout() : TArray<FName>(), Role);

	const FSpearfishDiverStats& Stats = Equipment->GetStats();
	Oxygen->ConfigureTank(Stats.MaxAir, Stats.SafeDepthM, Stats.StingResist, bRefillAir);
	Bag->SetCapacity(Stats.Bag);

	// Leaving the diver role on the boat: whatever is in the bag goes to the kitchen.
	if (Role != ESpearfishRole::Diver && !Bag->IsEmpty() && GameState && GameState->GetBoat())
	{
		SpearfishCatch::DepositAtBoat(this, GameState->GetBoat());
	}
	if (!Stats.bHasSpeargun || Stats.LightIntensity <= 0.f)
	{
		bLightOn = false;
		OnRep_Light();
	}
}

void ASpearfishCharacter::OnEquipmentChanged()
{
	const FSpearfishDiverStats& Stats = Equipment->GetStats();
	if (USpearfishMovementComponent* Movement = GetSpearfishMovement())
	{
		Movement->SetSwimTuning(Stats.SwimSpeedCm, Stats.SprintMultiplier, Stats.AccelerationCm);
	}
	Speargun->ApplyStats(Stats);
	Body->ApplyAppearance(Stats, Equipment->GetLoadoutRole());
	UnderwaterView->SetVisibilityBonus(Stats.VisibilityBonus);
	DiveLight->SetIntensity(Stats.LightIntensity);
	DiveLight->SetAttenuationRadius(FMath::Max(Stats.LightRangeCm, 100.f));
}

// ----------------------------------------------------------------------------------- Tick

void ASpearfishCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (USpearfishMovementComponent* Movement = GetSpearfishMovement())
	{
		Movement->SetLoadMultiplier(Bag->GetSpeedMultiplier());
	}

	if (HasAuthority() || IsLocallyControlled())
	{
		UpdateExternalForces(DeltaSeconds);
	}

	if (IsLocallyControlled())
	{
		if (IsSwimming() && !bBlackedOut && !IsUIBlockingInput())
		{
			const float Vertical = (bAscendHeld ? 1.f : 0.f) - (bDescendHeld ? 1.f : 0.f);
			if (!FMath::IsNearlyZero(Vertical))
			{
				AddMovementInput(FVector::UpVector, Vertical);
			}
		}
		SendBoatInput(DeltaSeconds);
		UpdateLocalPresentation(DeltaSeconds);
	}
}

void ASpearfishCharacter::UpdateExternalForces(float DeltaSeconds)
{
	FVector Acceleration = Speargun->GetShooterPullAcceleration();
	if (TetheredBy && TetheredBy->GetSpeargun())
	{
		Acceleration += TetheredBy->GetSpeargun()->GetTargetPullAcceleration(this);
	}
	const bool bSwimming = IsSwimming();
	if (bSwimming)
	{
		if (const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this))
		{
			Acceleration += Ocean->GetBoundaryCurrent(GetActorLocation());
		}
	}
	if (USpearfishMovementComponent* Movement = GetSpearfishMovement())
	{
		Movement->SetExternalAcceleration(bSwimming ? Acceleration : FVector::ZeroVector);
	}

	// A big fish (or a mischievous crewmate) can yank you along the deck - or overboard.
	YankCooldown = FMath::Max(0.f, YankCooldown - DeltaSeconds);
	if (HasAuthority() && !bSwimming && !IsDriving() && !bInBed && YankCooldown <= 0.f && Acceleration.Size() > 650.0)
	{
		LaunchCharacter(Acceleration.GetSafeNormal() * 520.0 + FVector(0.0, 0.0, 260.0), true, true);
		YankCooldown = 1.2f;
		NotifyOwner(LOCTEXT("Yanked", "Whoa! The line yanked you off your feet!"), ESpearfishNoticeType::Warning);
	}
}

void ASpearfishCharacter::UpdateLocalPresentation(float DeltaSeconds)
{
	float Danger = 0.f;
	if (IsSwimming())
	{
		const float Air = Oxygen->GetAirFraction();
		Danger = FMath::Clamp((0.3f - Air) / 0.3f, 0.f, 1.f);
		if (Oxygen->GetBreath().bOutOfAir)
		{
			Danger = 1.f;
		}
		if (Oxygen->IsStung())
		{
			Danger = FMath::Max(Danger, 0.35f);
		}
	}
	UnderwaterView->SetDangerAmount(Danger);
	BlackoutFade = FMath::FInterpTo(BlackoutFade, bBlackedOut ? 1.f : 0.f, DeltaSeconds, bBlackedOut ? 1.5f : 3.f);
	UnderwaterView->SetBlackoutAmount(BlackoutFade);
}

void ASpearfishCharacter::SendBoatInput(float DeltaSeconds)
{
	if (!IsDriving())
	{
		return;
	}
	BoatInputTimer -= DeltaSeconds;
	const FVector2D Input = IsUIBlockingInput() ? FVector2D::ZeroVector : MoveInput;
	if (BoatInputTimer <= 0.f || !Input.Equals(LastSentBoatInput, 0.05))
	{
		ServerSetBoatInput(static_cast<float>(Input.Y), static_cast<float>(Input.X));
		LastSentBoatInput = Input;
		BoatInputTimer = 0.1f;
	}
}

// ---------------------------------------------------------------------------------- Input

void ASpearfishCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	ASpearfishPlayerController* PlayerController = Cast<ASpearfishPlayerController>(GetController());
	const USpearfishInputConfig* Config = PlayerController ? PlayerController->GetInputConfig() : nullptr;
	if (!Input || !Config)
	{
		return;
	}

	Input->BindAction(Config->Move, ETriggerEvent::Triggered, this, &ASpearfishCharacter::Input_Move);
	Input->BindAction(Config->Move, ETriggerEvent::Completed, this, &ASpearfishCharacter::Input_MoveStopped);
	Input->BindAction(Config->Look, ETriggerEvent::Triggered, this, &ASpearfishCharacter::Input_Look);
	Input->BindAction(Config->Ascend, ETriggerEvent::Started, this, &ASpearfishCharacter::Input_AscendStarted);
	Input->BindAction(Config->Ascend, ETriggerEvent::Completed, this, &ASpearfishCharacter::Input_AscendStopped);
	Input->BindAction(Config->Descend, ETriggerEvent::Started, this, &ASpearfishCharacter::Input_DescendStarted);
	Input->BindAction(Config->Descend, ETriggerEvent::Completed, this, &ASpearfishCharacter::Input_DescendStopped);
	Input->BindAction(Config->Sprint, ETriggerEvent::Started, this, &ASpearfishCharacter::Input_SprintStarted);
	Input->BindAction(Config->Sprint, ETriggerEvent::Completed, this, &ASpearfishCharacter::Input_SprintStopped);
	Input->BindAction(Config->Fire, ETriggerEvent::Started, this, &ASpearfishCharacter::Input_Fire);
	Input->BindAction(Config->Reel, ETriggerEvent::Started, this, &ASpearfishCharacter::Input_ReelStarted);
	Input->BindAction(Config->Reel, ETriggerEvent::Completed, this, &ASpearfishCharacter::Input_ReelStopped);
	Input->BindAction(Config->Release, ETriggerEvent::Started, this, &ASpearfishCharacter::Input_Release);
	Input->BindAction(Config->Interact, ETriggerEvent::Started, this, &ASpearfishCharacter::Input_Interact);
	Input->BindAction(Config->ToggleLight, ETriggerEvent::Started, this, &ASpearfishCharacter::Input_ToggleLight);
}

void ASpearfishCharacter::Input_Move(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
	if (IsDriving() || bInBed || bBlackedOut || IsUIBlockingInput())
	{
		return;
	}

	const FRotator Control = GetControlRotation();
	const FVector Right = FRotator(0.f, Control.Yaw + 90.f, 0.f).Vector();
	if (IsSwimming())
	{
		// Swim where you look, in full 3D.
		AddMovementInput(Control.Vector(), static_cast<float>(MoveInput.Y));
		AddMovementInput(Right, static_cast<float>(MoveInput.X));
	}
	else
	{
		AddMovementInput(FRotator(0.f, Control.Yaw, 0.f).Vector(), static_cast<float>(MoveInput.Y));
		AddMovementInput(Right, static_cast<float>(MoveInput.X));
	}
}

void ASpearfishCharacter::Input_MoveStopped(const FInputActionValue& Value)
{
	MoveInput = FVector2D::ZeroVector;
}

void ASpearfishCharacter::Input_Look(const FInputActionValue& Value)
{
	const ASpearfishPlayerController* PlayerController = Cast<ASpearfishPlayerController>(GetController());
	if (PlayerController && PlayerController->IsLookBlocked())
	{
		return;
	}
	const FVector2D Look = Value.Get<FVector2D>();
	AddControllerYawInput(static_cast<float>(Look.X));
	AddControllerPitchInput(static_cast<float>(-Look.Y));
}

void ASpearfishCharacter::Input_AscendStarted(const FInputActionValue& Value)
{
	bAscendHeld = true;
	if (!IsSwimming() && !IsDriving() && !bInBed && !bBlackedOut && !IsUIBlockingInput())
	{
		Jump();
	}
}

void ASpearfishCharacter::Input_AscendStopped(const FInputActionValue& Value)
{
	bAscendHeld = false;
	StopJumping();
}

void ASpearfishCharacter::Input_DescendStarted(const FInputActionValue& Value)
{
	bDescendHeld = true;
}

void ASpearfishCharacter::Input_DescendStopped(const FInputActionValue& Value)
{
	bDescendHeld = false;
}

void ASpearfishCharacter::Input_SprintStarted(const FInputActionValue& Value)
{
	if (USpearfishMovementComponent* Movement = GetSpearfishMovement())
	{
		Movement->SetWantsToSprint(true);
	}
}

void ASpearfishCharacter::Input_SprintStopped(const FInputActionValue& Value)
{
	if (USpearfishMovementComponent* Movement = GetSpearfishMovement())
	{
		Movement->SetWantsToSprint(false);
	}
}

void ASpearfishCharacter::Input_Fire(const FInputActionValue& Value)
{
	if (!IsUIBlockingInput())
	{
		Speargun->RequestFire();
	}
}

void ASpearfishCharacter::Input_ReelStarted(const FInputActionValue& Value)
{
	if (!IsUIBlockingInput())
	{
		Speargun->SetReeling(true);
	}
}

void ASpearfishCharacter::Input_ReelStopped(const FInputActionValue& Value)
{
	Speargun->SetReeling(false);
}

void ASpearfishCharacter::Input_Release(const FInputActionValue& Value)
{
	if (IsUIBlockingInput())
	{
		return;
	}
	if (TetheredBy)
	{
		ServerShakeOffLine();
	}
	else
	{
		Speargun->RequestRelease();
	}
}

void ASpearfishCharacter::Input_Interact(const FInputActionValue& Value)
{
	if (IsUIBlockingInput())
	{
		return;
	}
	if (IsDriving())
	{
		ServerLeaveHelm();
	}
	else if (bInBed)
	{
		ServerLeaveBed();
	}
	else
	{
		Interaction->TryInteract();
	}
}

void ASpearfishCharacter::Input_ToggleLight(const FInputActionValue& Value)
{
	ServerToggleLight();
}

// ----------------------------------------------------------------------------- Server RPCs

void ASpearfishCharacter::ServerSetBoatInput_Implementation(float Throttle, float Steer)
{
	if (DrivingBoat)
	{
		DrivingBoat->SetDriverInput(FMath::Clamp(Throttle, -1.f, 1.f), FMath::Clamp(Steer, -1.f, 1.f));
	}
}

void ASpearfishCharacter::ServerLeaveHelm_Implementation()
{
	StopDriving();
}

void ASpearfishCharacter::ServerToggleLight_Implementation()
{
	if (Equipment->GetStats().LightIntensity <= 0.f)
	{
		NotifyOwner(LOCTEXT("NoLight", "You have no dive light today."), ESpearfishNoticeType::Info);
		return;
	}
	bLightOn = !bLightOn;
	OnRep_Light();
}

void ASpearfishCharacter::ServerShakeOffLine_Implementation()
{
	if (TetheredBy && TetheredBy->GetSpeargun())
	{
		TetheredBy->GetSpeargun()->ForceRelease();
	}
	SetTetheredBy(nullptr);
}

void ASpearfishCharacter::ServerLeaveBed_Implementation()
{
	if (!bInBed)
	{
		return;
	}
	if (ASpearfishGameMode* GameMode = GetWorld()->GetAuthGameMode<ASpearfishGameMode>())
	{
		GameMode->HandleLeaveBed(this);
	}
}

// ----------------------------------------------------------------------------- Server API

void ASpearfishCharacter::SetTetheredBy(ASpearfishCharacter* Shooter)
{
	TetheredBy = Shooter;
}

void ASpearfishCharacter::StartDriving(ASpearfishBoat* Boat)
{
	if (!Boat || IsDriving() || bInBed || bBlackedOut)
	{
		return;
	}
	Speargun->ForceRelease();
	DrivingBoat = Boat;
	Boat->SetDriver(this);
	OnRep_DrivingBoat();
}

void ASpearfishCharacter::StopDriving()
{
	ASpearfishBoat* Boat = DrivingBoat;
	if (!Boat)
	{
		return;
	}
	Boat->SetDriver(nullptr);
	DrivingBoat = nullptr;
	OnRep_DrivingBoat();
	TeleportForGameplay(Boat->GetHelmExitTransform(), false);
}

void ASpearfishCharacter::OnRep_DrivingBoat()
{
	USpearfishMovementComponent* Movement = GetSpearfishMovement();
	if (DrivingBoat)
	{
		if (Movement)
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
		AttachToComponent(DrivingBoat->GetHelmSeat(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		if (Movement && Movement->MovementMode == MOVE_None && !bInBed && !bBlackedOut)
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
	}
}

void ASpearfishCharacter::EnterBed(const FTransform& LieTransform)
{
	if (bInBed || IsDriving() || bBlackedOut)
	{
		return;
	}
	Speargun->ForceRelease();
	bInBed = true;
	if (ASpearfishPlayerState* State = GetSpearfishPlayerState())
	{
		State->SetInBed(true);
	}
	TeleportTo(LieTransform.GetLocation(), LieTransform.Rotator(), false, true);
	if (USpearfishMovementComponent* Movement = GetSpearfishMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		const FRotator LookAtCeiling(60.f, LieTransform.Rotator().Yaw, 0.f);
		PlayerController->SetControlRotation(LookAtCeiling);
		PlayerController->ClientSetRotation(LookAtCeiling);
	}
}

void ASpearfishCharacter::LeaveBed(const FTransform& ExitTransform)
{
	if (!bInBed)
	{
		return;
	}
	bInBed = false;
	if (ASpearfishPlayerState* State = GetSpearfishPlayerState())
	{
		State->SetInBed(false);
	}
	TeleportForGameplay(ExitTransform, false);
}

void ASpearfishCharacter::TeleportForGameplay(const FTransform& Where, bool bInWater)
{
	TeleportTo(Where.GetLocation(), Where.Rotator(), false, true);
	if (USpearfishMovementComponent* Movement = GetSpearfishMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(bInWater ? MOVE_Flying : MOVE_Walking);
	}
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		const FRotator Facing(0.f, Where.Rotator().Yaw, 0.f);
		PlayerController->SetControlRotation(Facing);
		PlayerController->ClientSetRotation(Facing);
	}
}

void ASpearfishCharacter::ClimbAboard(ASpearfishBoat* Boat)
{
	if (!Boat || bBlackedOut)
	{
		return;
	}
	SpearfishCatch::DepositAtBoat(this, Boat);
	Speargun->ForceRelease();
	Oxygen->Refill();
	TeleportForGameplay(Boat->GetLadderTopTransform(), false);
}

void ASpearfishCharacter::EnterWater(ASpearfishBoat* Boat)
{
	if (!Boat || bBlackedOut)
	{
		return;
	}
	TeleportForGameplay(Boat->GetLadderWaterTransform(), true);
}

void ASpearfishCharacter::HandleBlackout()
{
	if (bBlackedOut)
	{
		return;
	}
	bBlackedOut = true;
	OnRep_BlackedOut();
	Speargun->ForceRelease();
	if (IsDriving())
	{
		StopDriving();
	}

	const TArray<FSpearfishItem> Lost = Bag->TakeAll();
	if (ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>())
	{
		GameState->BroadcastNotice(Lost.Num() > 0
			? FText::Format(LOCTEXT("BlackoutLost", "{0} blacked out! {1} item(s) sank into the blue."), FText::FromString(GetPlayerName()), FText::AsNumber(Lost.Num()))
			: FText::Format(LOCTEXT("Blackout", "{0} blacked out!"), FText::FromString(GetPlayerName())), ESpearfishNoticeType::Danger);
	}
	if (USpearfishMovementComponent* Movement = GetSpearfishMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	GetWorldTimerManager().SetTimer(RescueTimer, this, &ASpearfishCharacter::FinishBlackout, USpearfishSettings::Get()->RescueDelaySeconds, false);
}

void ASpearfishCharacter::FinishBlackout()
{
	if (ASpearfishGameMode* GameMode = GetWorld()->GetAuthGameMode<ASpearfishGameMode>())
	{
		GameMode->RescueDiver(this);
	}
}

void ASpearfishCharacter::Rescue(const FTransform& Where)
{
	bBlackedOut = false;
	OnRep_BlackedOut();
	Oxygen->Refill();
	TeleportForGameplay(Where, false);
}

void ASpearfishCharacter::OnRep_BlackedOut()
{
	if (bBlackedOut && Body)
	{
		Body->EmitBubbles(12);
	}
}

void ASpearfishCharacter::OnRep_Light()
{
	DiveLight->SetVisibility(bLightOn);
}

void ASpearfishCharacter::NotifyOwner(const FText& Text, ESpearfishNoticeType Type)
{
	if (ASpearfishPlayerController* PlayerController = Cast<ASpearfishPlayerController>(GetController()))
	{
		FSpearfishNotice Notice;
		Notice.Text = Text;
		Notice.Type = Type;
		PlayerController->ClientNotice(Notice);
	}
}

#undef LOCTEXT_NAMESPACE
