#include "Progression/SpearfishProgressionComponent.h"

#include "Core/SpearfishGameState.h"
#include "Data/SpearfishDataRegistry.h"
#include "DayNight/SpearfishDayCycleComponent.h"
#include "Net/UnrealNetwork.h"
#include "Progression/SpearfishJournalComponent.h"
#include "Rules/EconomyRules.h"
#include "SaveSystem/SpearfishCampaignData.h"

USpearfishProgressionComponent::USpearfishProgressionComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
	Loadout.SetNum(static_cast<int32>(ESpearfishEquipmentSlot::Count));
}

void USpearfishProgressionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USpearfishProgressionComponent, Money);
	DOREPLIFETIME(USpearfishProgressionComponent, Reputation);
	DOREPLIFETIME(USpearfishProgressionComponent, OwnedEquipment);
	DOREPLIFETIME(USpearfishProgressionComponent, Loadout);
	DOREPLIFETIME(USpearfishProgressionComponent, OwnedUpgrades);
	DOREPLIFETIME(USpearfishProgressionComponent, UnlockedRegions);
}

void USpearfishProgressionComponent::LoadFromCampaign(const FSpearfishCampaignData& Campaign)
{
	Money = Campaign.Money;
	Reputation = Campaign.Reputation;
	OwnedEquipment = Campaign.OwnedEquipment;
	OwnedUpgrades = Campaign.OwnedUpgrades;
	UnlockedRegions = Campaign.UnlockedRegions;
	Loadout = Campaign.Loadout;
	Loadout.SetNum(static_cast<int32>(ESpearfishEquipmentSlot::Count));

	// Repair: every slot always has something equipped (old saves, new slots added by data).
	if (const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this))
	{
		for (int32 SlotIndex = 0; SlotIndex < Loadout.Num(); ++SlotIndex)
		{
			const ESpearfishEquipmentSlot Slot = static_cast<ESpearfishEquipmentSlot>(SlotIndex);
			const FSpearfishEquipmentDef* Equipped = Registry->FindEquipment(Loadout[SlotIndex]);
			if (!Equipped || Equipped->Slot != Slot || !OwnedEquipment.Contains(Loadout[SlotIndex]))
			{
				if (const FSpearfishEquipmentDef* Starter = Registry->GetStarterEquipment(Slot))
				{
					OwnedEquipment.AddUnique(Starter->Id);
					Loadout[SlotIndex] = Starter->Id;
				}
			}
		}
	}
	NotifyChanged();
	OnLoadoutChanged.Broadcast();
}

void USpearfishProgressionComponent::SaveToCampaign(FSpearfishCampaignData& Campaign) const
{
	Campaign.Money = Money;
	Campaign.Reputation = Reputation;
	Campaign.OwnedEquipment = OwnedEquipment;
	Campaign.Loadout = Loadout;
	Campaign.OwnedUpgrades = OwnedUpgrades;
	Campaign.UnlockedRegions = UnlockedRegions;
}

void USpearfishProgressionComponent::AddMoney(int32 Delta)
{
	if (Delta == 0)
	{
		return;
	}
	Money = FMath::Max(0, Money + Delta);
	NotifyChanged();
}

void USpearfishProgressionComponent::AddReputation(float Delta)
{
	Reputation = SpearfishEconomy::ApplyReputation(Reputation, Delta);
	NotifyChanged();
}

FName USpearfishProgressionComponent::GetEquipped(ESpearfishEquipmentSlot Slot) const
{
	const int32 Index = static_cast<int32>(Slot);
	return Loadout.IsValidIndex(Index) ? Loadout[Index] : NAME_None;
}

FSpearfishProgressSnapshot USpearfishProgressionComponent::MakeSnapshot() const
{
	FSpearfishProgressSnapshot Snapshot;
	Snapshot.Money = Money;
	Snapshot.Reputation = Reputation;
	Snapshot.Owned = OwnedEquipment;
	Snapshot.Owned.Append(OwnedUpgrades);
	Snapshot.Owned.Append(UnlockedRegions);
	if (const ASpearfishGameState* GameState = Cast<ASpearfishGameState>(GetOwner()))
	{
		if (const USpearfishDayCycleComponent* DayCycle = GameState->GetDayCycle())
		{
			Snapshot.Day = DayCycle->GetDay();
		}
		if (const USpearfishJournalComponent* Journal = GameState->GetJournal())
		{
			Snapshot.SpeciesCaught = Journal->CountCaughtSpecies();
		}
	}
	return Snapshot;
}

ESpearfishRequirementResult USpearfishProgressionComponent::CheckEquipment(FName EquipmentId) const
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FSpearfishEquipmentDef* Def = Registry ? Registry->FindEquipment(EquipmentId) : nullptr;
	if (!Def)
	{
		return ESpearfishRequirementResult::NeedsPrerequisite;
	}
	return SpearfishEconomy::Check(Def->Unlock, MakeSnapshot(), EquipmentId);
}

ESpearfishRequirementResult USpearfishProgressionComponent::CheckUpgrade(FName UpgradeId) const
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FSpearfishRestaurantUpgradeDef* Def = Registry ? Registry->FindUpgrade(UpgradeId) : nullptr;
	if (!Def)
	{
		return ESpearfishRequirementResult::NeedsPrerequisite;
	}
	return SpearfishEconomy::Check(Def->Unlock, MakeSnapshot(), UpgradeId);
}

ESpearfishRequirementResult USpearfishProgressionComponent::CheckRegion(FName RegionId) const
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FSpearfishRegionDef* Def = Registry ? Registry->FindRegion(RegionId) : nullptr;
	if (!Def)
	{
		return ESpearfishRequirementResult::NeedsPrerequisite;
	}
	return SpearfishEconomy::Check(Def->Unlock, MakeSnapshot(), RegionId);
}

ESpearfishRequirementResult USpearfishProgressionComponent::TryPurchaseEquipment(FName EquipmentId)
{
	const ESpearfishRequirementResult Result = CheckEquipment(EquipmentId);
	if (Result != ESpearfishRequirementResult::Ok)
	{
		return Result;
	}
	const FSpearfishEquipmentDef* Def = USpearfishDataRegistry::Get(this)->FindEquipment(EquipmentId);
	Money -= Def->Unlock.Price;
	OwnedEquipment.AddUnique(EquipmentId);
	// New gear is equipped straight away; the locker can swap back.
	Loadout[static_cast<int32>(Def->Slot)] = EquipmentId;
	NotifyChanged();
	OnLoadoutChanged.Broadcast();
	return ESpearfishRequirementResult::Ok;
}

bool USpearfishProgressionComponent::TryEquip(FName EquipmentId)
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FSpearfishEquipmentDef* Def = Registry ? Registry->FindEquipment(EquipmentId) : nullptr;
	if (!Def || !OwnedEquipment.Contains(EquipmentId))
	{
		return false;
	}
	Loadout[static_cast<int32>(Def->Slot)] = EquipmentId;
	NotifyChanged();
	OnLoadoutChanged.Broadcast();
	return true;
}

ESpearfishRequirementResult USpearfishProgressionComponent::TryPurchaseUpgrade(FName UpgradeId)
{
	const ESpearfishRequirementResult Result = CheckUpgrade(UpgradeId);
	if (Result != ESpearfishRequirementResult::Ok)
	{
		return Result;
	}
	Money -= USpearfishDataRegistry::Get(this)->FindUpgrade(UpgradeId)->Unlock.Price;
	OwnedUpgrades.AddUnique(UpgradeId);
	NotifyChanged();
	return ESpearfishRequirementResult::Ok;
}

ESpearfishRequirementResult USpearfishProgressionComponent::TryUnlockRegion(FName RegionId)
{
	const ESpearfishRequirementResult Result = CheckRegion(RegionId);
	if (Result != ESpearfishRequirementResult::Ok)
	{
		return Result;
	}
	Money -= USpearfishDataRegistry::Get(this)->FindRegion(RegionId)->Unlock.Price;
	UnlockedRegions.AddUnique(RegionId);
	NotifyChanged();
	return ESpearfishRequirementResult::Ok;
}

TArray<const FSpearfishRestaurantUpgradeDef*> USpearfishProgressionComponent::GetOwnedUpgradeDefs() const
{
	TArray<const FSpearfishRestaurantUpgradeDef*> Defs;
	if (const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this))
	{
		for (const FName Id : OwnedUpgrades)
		{
			if (const FSpearfishRestaurantUpgradeDef* Def = Registry->FindUpgrade(Id))
			{
				Defs.Add(Def);
			}
		}
	}
	return Defs;
}

int32 USpearfishProgressionComponent::GetExtraSeats() const
{
	int32 Seats = 0;
	for (const FSpearfishRestaurantUpgradeDef* Def : GetOwnedUpgradeDefs())
	{
		Seats += Def->ExtraSeats;
	}
	return Seats;
}

float USpearfishProgressionComponent::GetCookZoneBonus() const
{
	float Bonus = 0.f;
	for (const FSpearfishRestaurantUpgradeDef* Def : GetOwnedUpgradeDefs())
	{
		Bonus += Def->CookZoneBonus;
	}
	return Bonus;
}

float USpearfishProgressionComponent::GetDishQualityBonus() const
{
	float Bonus = 0.f;
	for (const FSpearfishRestaurantUpgradeDef* Def : GetOwnedUpgradeDefs())
	{
		Bonus += Def->DishQualityBonus;
	}
	return Bonus;
}

float USpearfishProgressionComponent::GetPatienceBonus() const
{
	float Bonus = 0.f;
	for (const FSpearfishRestaurantUpgradeDef* Def : GetOwnedUpgradeDefs())
	{
		Bonus += Def->PatienceBonus;
	}
	return Bonus;
}

float USpearfishProgressionComponent::GetAutoChefSkillBonus() const
{
	float Bonus = 0.f;
	for (const FSpearfishRestaurantUpgradeDef* Def : GetOwnedUpgradeDefs())
	{
		Bonus += Def->AutoChefSkillBonus;
	}
	return Bonus;
}

bool USpearfishProgressionComponent::HasTelemetry() const
{
	for (const FSpearfishRestaurantUpgradeDef* Def : GetOwnedUpgradeDefs())
	{
		if (Def->bUnlocksTelemetry)
		{
			return true;
		}
	}
	return false;
}

bool USpearfishProgressionComponent::HasDiverLocator() const
{
	for (const FSpearfishRestaurantUpgradeDef* Def : GetOwnedUpgradeDefs())
	{
		if (Def->bUnlocksDiverLocator)
		{
			return true;
		}
	}
	return false;
}

void USpearfishProgressionComponent::OnRep_Progress()
{
	OnProgressChanged.Broadcast();
}

void USpearfishProgressionComponent::NotifyChanged()
{
	OnProgressChanged.Broadcast();
}
