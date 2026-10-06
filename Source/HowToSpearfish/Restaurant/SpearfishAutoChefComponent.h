#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Orders/SpearfishOrderTypes.h"
#include "SpearfishAutoChefComponent.generated.h"

/**
 * The automated kitchen used in solo (and whenever no human chef is connected). It runs the restaurant
 * competently but not perfectly: opens at service time while anchored, starts dishes as soon as the cooler
 * allows, cooks each step at an upgradeable skill level, serves, and radios the diver what the kitchen is
 * still missing - so a solo player is never "missing half the game", they just get the radio side of it.
 */
UCLASS(ClassGroup = (Spearfish), meta = (BlueprintSpawnableComponent))
class HOWTOSPEARFISH_API USpearfishAutoChefComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpearfishAutoChefComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float GetSkill() const;

private:
	bool IsActive() const;
	void OnOrderPlaced(const FSpearfishOrder& Order);
	void RadioNeeds(bool bForce);

	float ThinkTimer = 0.f;
	float RadioCooldown = 0.f;
	bool bPendingRadio = false;
};
