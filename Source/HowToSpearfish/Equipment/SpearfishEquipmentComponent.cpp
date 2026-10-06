#include "Equipment/SpearfishEquipmentComponent.h"

#include "Data/SpearfishDataRegistry.h"
#include "Net/UnrealNetwork.h"

USpearfishEquipmentComponent::USpearfishEquipmentComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void USpearfishEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USpearfishEquipmentComponent, Equipped);
	DOREPLIFETIME(USpearfishEquipmentComponent, LoadoutRole);
}

void USpearfishEquipmentComponent::ApplyLoadout(const TArray<FName>& Loadout, ESpearfishRole Role)
{
	Equipped = Loadout;
	LoadoutRole = Role;
	Recompute();
}

FName USpearfishEquipmentComponent::GetEquipped(ESpearfishEquipmentSlot Slot) const
{
	const int32 Index = static_cast<int32>(Slot);
	return Equipped.IsValidIndex(Index) ? Equipped[Index] : NAME_None;
}

void USpearfishEquipmentComponent::OnRep_Loadout()
{
	Recompute();
}

void USpearfishEquipmentComponent::Recompute()
{
	Stats = ComputeStats(USpearfishDataRegistry::Get(this), Equipped, LoadoutRole);
	OnEquipmentChanged.Broadcast();
}

FSpearfishDiverStats USpearfishEquipmentComponent::ComputeStats(const USpearfishDataRegistry* Registry, const TArray<FName>& Loadout, ESpearfishRole Role)
{
	FSpearfishDiverStats Out;

	if (Role != ESpearfishRole::Diver || !Registry)
	{
		// Snorkel kit: a lungful of air, no gun, a tiny pocket for one fish.
		Out.bHasSpeargun = false;
		Out.bHasTank = false;
		Out.MaxAir = 35.f;
		Out.SafeDepthM = 8.f;
		Out.SwimSpeedCm = 240.f;
		Out.SprintMultiplier = 1.4f;
		Out.AccelerationCm = 420.f;
		Out.Bag.FishSlots = 1;
		Out.Bag.LootSlots = 1;
		Out.Bag.MaxWeightKg = 6.f;
		Out.SuitColor = Role == ESpearfishRole::Chef ? FLinearColor(0.95f, 0.95f, 0.92f) : FLinearColor(0.5f, 0.5f, 0.5f);
		return Out;
	}

	for (int32 SlotIndex = 0; SlotIndex < Loadout.Num(); ++SlotIndex)
	{
		const FSpearfishEquipmentDef* Def = Registry->FindEquipment(Loadout[SlotIndex]);
		if (!Def)
		{
			continue;
		}
		const FSpearfishEquipmentStats& S = Def->Stats;
		switch (Def->Slot)
		{
		case ESpearfishEquipmentSlot::Speargun:
			Out.bHasSpeargun = true;
			Out.GunPower = S.GunPower;
			Out.GunRangeCm = S.GunRangeCm;
			Out.ReloadSeconds = S.ReloadSeconds;
			Out.ShaftSpeedCm = S.ShaftSpeedCm;
			Out.AimSpreadDeg = S.AimSpreadDeg;
			Out.GunColor = Def->Color;
			Out.GunTier = Def->Tier;
			break;
		case ESpearfishEquipmentSlot::Reel:
			Out.Line.MaxLengthCm = S.LineLengthCm;
			Out.Line.ReelSpeedCm = S.ReelSpeedCm;
			Out.Line.DragForce = S.DragForce;
			Out.Line.BreakForce = S.BreakForce;
			Out.Line.ReelForce = S.DragForce * 0.6f;
			break;
		case ESpearfishEquipmentSlot::Tank:
			Out.bHasTank = true;
			Out.MaxAir = S.AirCapacity;
			Out.TankColor = Def->Color;
			Out.TankTier = Def->Tier;
			break;
		case ESpearfishEquipmentSlot::Wetsuit:
			Out.SafeDepthM = S.SafeDepthM;
			Out.StingResist = S.StingResist;
			Out.SuitColor = Def->Color;
			break;
		case ESpearfishEquipmentSlot::Mask:
			Out.VisibilityBonus = S.VisibilityBonus;
			Out.MaskColor = Def->Color;
			break;
		case ESpearfishEquipmentSlot::Fins:
			Out.SwimSpeedCm = S.SwimSpeedCm;
			Out.SprintMultiplier = S.SprintMultiplier;
			Out.AccelerationCm = S.AccelerationCm;
			Out.FinsColor = Def->Color;
			Out.FinsTier = Def->Tier;
			break;
		case ESpearfishEquipmentSlot::Bag:
			Out.Bag.FishSlots = S.FishSlots;
			Out.Bag.LootSlots = S.LootSlots;
			Out.Bag.MaxWeightKg = S.MaxWeightKg;
			Out.BagColor = Def->Color;
			break;
		case ESpearfishEquipmentSlot::Light:
			Out.LightIntensity = S.LightIntensity;
			Out.LightRangeCm = S.LightRangeCm;
			break;
		default:
			break;
		}
	}
	return Out;
}
