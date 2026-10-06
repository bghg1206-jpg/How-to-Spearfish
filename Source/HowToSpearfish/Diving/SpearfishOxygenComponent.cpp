#include "Diving/SpearfishOxygenComponent.h"

#include "Core/SpearfishSettings.h"
#include "Data/SpearfishDefinitions.h"
#include "Diving/SpearfishMovementComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "World/SpearfishOceanSubsystem.h"

USpearfishOxygenComponent::USpearfishOxygenComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void USpearfishOxygenComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USpearfishOxygenComponent, Breath);
	DOREPLIFETIME(USpearfishOxygenComponent, ConsumptionRate);
	DOREPLIFETIME(USpearfishOxygenComponent, StungRemaining);
	DOREPLIFETIME(USpearfishOxygenComponent, bOverDepth);
	DOREPLIFETIME(USpearfishOxygenComponent, SafeDepthM);
}

void USpearfishOxygenComponent::ConfigureTank(float MaxAir, float InSafeDepthM, float InStingResist, bool bRefill)
{
	SafeDepthM = InSafeDepthM;
	StingResist = FMath::Clamp(InStingResist, 0.f, 0.9f);
	if (bRefill)
	{
		SpearfishOxygen::Refill(Breath, MaxAir, USpearfishSettings::Get()->Oxygen);
	}
	else
	{
		// Changing tanks mid-dive keeps the current fill fraction.
		const float Fraction = SpearfishOxygen::AirFraction(Breath);
		Breath.MaxAir = FMath::Max(MaxAir, 1.f);
		Breath.Air = Breath.MaxAir * Fraction;
	}
}

void USpearfishOxygenComponent::Refill()
{
	SpearfishOxygen::Refill(Breath, Breath.MaxAir, USpearfishSettings::Get()->Oxygen);
	StungRemaining = 0.f;
}

void USpearfishOxygenComponent::ApplyShock(float Amount, bool bSting)
{
	if (GetOwnerRole() != ROLE_Authority || Breath.bBlackedOut)
	{
		return;
	}
	const float Resisted = Amount * (1.f - StingResist);
	SpearfishOxygen::ApplyShock(Breath, Resisted);
	if (bSting)
	{
		StungRemaining = FMath::Max(StungRemaining, 8.f * (1.f - StingResist));
	}
}

void USpearfishOxygenComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwnerRole() != ROLE_Authority || bSuspended || Breath.bBlackedOut)
	{
		return;
	}

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const USpearfishMovementComponent* Movement = Character ? Cast<USpearfishMovementComponent>(Character->GetCharacterMovement()) : nullptr;
	if (!Movement)
	{
		return;
	}

	StungRemaining = FMath::Max(0.f, StungRemaining - DeltaTime);

	FSpearfishBreathInput Input;
	// Breathing happens at the head, not the capsule centre.
	Input.DepthMeters = Movement->IsInSea() ? Movement->GetDepthMeters() - 0.5f : 0.f;
	Input.SpeedFraction = Movement->GetSwimSpeedFraction();
	Input.bSprinting = Movement->IsSprintSwimming();
	Input.SafeDepthMeters = SafeDepthM;
	Input.bStung = StungRemaining > 0.f;
	if (const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this))
	{
		if (const FSpearfishDepthZoneDef* Zone = Ocean->GetDepthZone(Input.DepthMeters))
		{
			Input.ZoneMultiplier = Zone->AirMultiplier;
		}
	}

	const FSpearfishOxygenTuning& Tuning = USpearfishSettings::Get()->Oxygen;
	ConsumptionRate = SpearfishOxygen::ConsumptionPerSecond(Input, Tuning);
	bOverDepth = Input.DepthMeters > SafeDepthM;

	if (SpearfishOxygen::Step(Breath, Input, Tuning, DeltaTime))
	{
		OnBlackout.Broadcast();
	}
}
