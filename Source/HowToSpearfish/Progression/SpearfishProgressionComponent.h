#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Rules/SpearfishRulesTypes.h"
#include "SpearfishProgressionComponent.generated.h"

struct FSpearfishCampaignData;
struct FSpearfishRestaurantUpgradeDef;

DECLARE_MULTICAST_DELEGATE(FSpearfishProgressChanged);

/**
 * Team-wide economy and unlocks, lives on the GameState. Equipment is a shared "crew kit": because roles
 * swap every day, whoever dives today wears the best kit the crew owns. Server-authoritative; every
 * purchase goes through SpearfishEconomy::Check so money, reputation, day and discovery gates apply.
 */
UCLASS(ClassGroup = (Spearfish), meta = (BlueprintSpawnableComponent))
class HOWTOSPEARFISH_API USpearfishProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpearfishProgressionComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Server API ---------------------------------------------------------------------------
	void LoadFromCampaign(const FSpearfishCampaignData& Campaign);
	void SaveToCampaign(FSpearfishCampaignData& Campaign) const;

	void AddMoney(int32 Delta);
	void AddReputation(float Delta);

	ESpearfishRequirementResult TryPurchaseEquipment(FName EquipmentId);
	bool TryEquip(FName EquipmentId);
	ESpearfishRequirementResult TryPurchaseUpgrade(FName UpgradeId);
	ESpearfishRequirementResult TryUnlockRegion(FName RegionId);

	// --- Queries (all machines) ---------------------------------------------------------------
	int32 GetMoney() const { return Money; }
	float GetReputation() const { return Reputation; }
	bool OwnsEquipment(FName Id) const { return OwnedEquipment.Contains(Id); }
	bool HasUpgrade(FName Id) const { return OwnedUpgrades.Contains(Id); }
	bool IsRegionUnlocked(FName Id) const { return UnlockedRegions.Contains(Id); }
	FName GetEquipped(ESpearfishEquipmentSlot Slot) const;
	const TArray<FName>& GetLoadout() const { return Loadout; }
	const TArray<FName>& GetOwnedUpgrades() const { return OwnedUpgrades; }

	ESpearfishRequirementResult CheckEquipment(FName EquipmentId) const;
	ESpearfishRequirementResult CheckUpgrade(FName UpgradeId) const;
	ESpearfishRequirementResult CheckRegion(FName RegionId) const;

	FSpearfishProgressSnapshot MakeSnapshot() const;

	// Aggregated restaurant upgrade effects.
	int32 GetExtraSeats() const;
	float GetCookZoneBonus() const;
	float GetDishQualityBonus() const;
	float GetPatienceBonus() const;
	float GetAutoChefSkillBonus() const;
	bool HasTelemetry() const;
	bool HasDiverLocator() const;

	/** Broadcast on every machine whenever money, reputation or ownership changes. */
	FSpearfishProgressChanged OnProgressChanged;

	/** Server: the equipped kit changed and divers must refresh. */
	FSpearfishProgressChanged OnLoadoutChanged;

private:
	UFUNCTION()
	void OnRep_Progress();

	void NotifyChanged();
	TArray<const FSpearfishRestaurantUpgradeDef*> GetOwnedUpgradeDefs() const;

	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	int32 Money = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	float Reputation = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	TArray<FName> OwnedEquipment;

	/** Equipped item per ESpearfishEquipmentSlot index. */
	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	TArray<FName> Loadout;

	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	TArray<FName> OwnedUpgrades;

	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	TArray<FName> UnlockedRegions;
};
