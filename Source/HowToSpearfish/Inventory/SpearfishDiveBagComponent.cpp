#include "Inventory/SpearfishDiveBagComponent.h"

#include "Net/UnrealNetwork.h"

USpearfishDiveBagComponent::USpearfishDiveBagComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void USpearfishDiveBagComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USpearfishDiveBagComponent, Bag);
}

ESpearfishBagResult USpearfishDiveBagComponent::TryAdd(const FSpearfishItem& Item)
{
	const ESpearfishBagResult Result = SpearfishInventory::TryAdd(Bag, Item);
	if (Result == ESpearfishBagResult::Ok)
	{
		OnRep_Bag();
	}
	return Result;
}

TArray<FSpearfishItem> USpearfishDiveBagComponent::TakeAll()
{
	TArray<FSpearfishItem> Items = SpearfishInventory::TakeAll(Bag);
	OnRep_Bag();
	return Items;
}

void USpearfishDiveBagComponent::SetCapacity(const FSpearfishBagCapacity& Capacity)
{
	SpearfishInventory::SetCapacity(Bag, Capacity);
	OnRep_Bag();
}

void USpearfishDiveBagComponent::OnRep_Bag()
{
	OnBagChanged.Broadcast();
}
