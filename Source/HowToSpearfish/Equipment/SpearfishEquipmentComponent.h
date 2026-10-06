#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Rules/LineRules.h"
#include "Rules/SpearfishRulesTypes.h"
#include "SpearfishEquipmentComponent.generated.h"

class USpearfishDataRegistry;

/** Effective stats of whatever the character currently wears. Derived locally from replicated ids. */
USTRUCT(BlueprintType)
struct FSpearfishDiverStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	bool bHasSpeargun = false;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	bool bHasTank = false;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float GunPower = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float GunRangeCm = 650.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float ReloadSeconds = 2.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float ShaftSpeedCm = 1800.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float AimSpreadDeg = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FSpearfishLineSpec Line;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float MaxAir = 300.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float SafeDepthM = 15.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float StingResist = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float VisibilityBonus = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float SwimSpeedCm = 330.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float SprintMultiplier = 1.6f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float AccelerationCm = 520.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FSpearfishBagCapacity Bag;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float LightIntensity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float LightRangeCm = 0.f;

	// Appearance
	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	FLinearColor SuitColor = FLinearColor(0.15f, 0.15f, 0.2f);

	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	FLinearColor TankColor = FLinearColor(0.75f, 0.75f, 0.78f);

	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	FLinearColor FinsColor = FLinearColor(0.1f, 0.1f, 0.1f);

	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	FLinearColor MaskColor = FLinearColor(0.2f, 0.2f, 0.2f);

	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	FLinearColor GunColor = FLinearColor(0.45f, 0.3f, 0.18f);

	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	FLinearColor BagColor = FLinearColor(0.3f, 0.5f, 0.3f);

	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	int32 GunTier = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	int32 FinsTier = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	int32 TankTier = 0;
};

DECLARE_MULTICAST_DELEGATE(FSpearfishEquipmentChanged);

/**
 * What the character wears today. Divers wear the crew's equipped kit; chefs get a snorkel and an apron
 * (short breath-hold, no speargun) so roles feel different without hard locks.
 */
UCLASS(ClassGroup = (Spearfish), meta = (BlueprintSpawnableComponent))
class HOWTOSPEARFISH_API USpearfishEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpearfishEquipmentComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server. */
	void ApplyLoadout(const TArray<FName>& Loadout, ESpearfishRole Role);

	const FSpearfishDiverStats& GetStats() const { return Stats; }
	ESpearfishRole GetLoadoutRole() const { return LoadoutRole; }
	FName GetEquipped(ESpearfishEquipmentSlot Slot) const;

	static FSpearfishDiverStats ComputeStats(const USpearfishDataRegistry* Registry, const TArray<FName>& Loadout, ESpearfishRole Role);

	/** Fired on every machine after stats are recomputed. */
	FSpearfishEquipmentChanged OnEquipmentChanged;

private:
	UFUNCTION()
	void OnRep_Loadout();

	void Recompute();

	UPROPERTY(ReplicatedUsing = OnRep_Loadout)
	TArray<FName> Equipped;

	UPROPERTY(ReplicatedUsing = OnRep_Loadout)
	ESpearfishRole LoadoutRole = ESpearfishRole::None;

	FSpearfishDiverStats Stats;
};
