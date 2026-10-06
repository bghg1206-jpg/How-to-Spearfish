#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Rules/SpearfishRulesTypes.h"
#include "SpearfishCatchStorageComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FSpearfishStorageChanged);

/** The boat's cooler: shared fish storage the kitchen cooks from. Server-authoritative, replicated. */
UCLASS(ClassGroup = (Spearfish), meta = (BlueprintSpawnableComponent))
class HOWTOSPEARFISH_API USpearfishCatchStorageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpearfishCatchStorageComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Server
	/** Adds fish (duplicates by instance id are ignored). Returns the number actually added. */
	int32 Deposit(const TArray<FSpearfishItem>& Items);
	/** Removes and returns the items at the given indices (into GetItems()). */
	TArray<FSpearfishItem> Withdraw(const TArray<int32>& Indices);
	void SetItems(const TArray<FSpearfishItem>& InItems);
	/** Overnight: quality drops; fish below the threshold are discarded. Returns how many spoiled. */
	int32 ApplySpoilage(float QualityLoss, float DiscardBelow);

	const TArray<FSpearfishItem>& GetItems() const { return Items; }
	int32 CountSpecies(FName SpeciesId) const;

	FSpearfishStorageChanged OnStorageChanged;

private:
	UFUNCTION()
	void OnRep_Items();

	UPROPERTY(ReplicatedUsing = OnRep_Items)
	TArray<FSpearfishItem> Items;
};
