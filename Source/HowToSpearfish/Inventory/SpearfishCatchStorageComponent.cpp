#include "Inventory/SpearfishCatchStorageComponent.h"

#include "Net/UnrealNetwork.h"
#include "Rules/InventoryRules.h"

USpearfishCatchStorageComponent::USpearfishCatchStorageComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void USpearfishCatchStorageComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USpearfishCatchStorageComponent, Items);
}

int32 USpearfishCatchStorageComponent::Deposit(const TArray<FSpearfishItem>& NewItems)
{
	int32 Added = 0;
	for (const FSpearfishItem& Item : NewItems)
	{
		if (Item.IsValid() && Item.Kind == ESpearfishItemKind::Fish && !SpearfishInventory::ContainsInstance(Items, Item.InstanceId))
		{
			Items.Add(Item);
			++Added;
		}
	}
	if (Added > 0)
	{
		OnRep_Items();
	}
	return Added;
}

TArray<FSpearfishItem> USpearfishCatchStorageComponent::Withdraw(const TArray<int32>& Indices)
{
	TArray<FSpearfishItem> Taken;
	TArray<int32> Sorted = Indices;
	Sorted.Sort([](const int32 A, const int32 B) { return A > B; });
	for (const int32 Index : Sorted)
	{
		if (Items.IsValidIndex(Index))
		{
			Taken.Add(Items[Index]);
			Items.RemoveAt(Index);
		}
	}
	if (Taken.Num() > 0)
	{
		OnRep_Items();
	}
	return Taken;
}

void USpearfishCatchStorageComponent::SetItems(const TArray<FSpearfishItem>& InItems)
{
	Items = InItems;
	OnRep_Items();
}

int32 USpearfishCatchStorageComponent::ApplySpoilage(float QualityLoss, float DiscardBelow)
{
	for (FSpearfishItem& Item : Items)
	{
		Item.Quality = FMath::Max(0.f, Item.Quality - QualityLoss);
	}
	const int32 Removed = Items.RemoveAll([DiscardBelow](const FSpearfishItem& Item) { return Item.Quality < DiscardBelow; });
	OnRep_Items();
	return Removed;
}

int32 USpearfishCatchStorageComponent::CountSpecies(FName SpeciesId) const
{
	int32 Count = 0;
	for (const FSpearfishItem& Item : Items)
	{
		Count += Item.DefId == SpeciesId ? 1 : 0;
	}
	return Count;
}

void USpearfishCatchStorageComponent::OnRep_Items()
{
	OnStorageChanged.Broadcast();
}
