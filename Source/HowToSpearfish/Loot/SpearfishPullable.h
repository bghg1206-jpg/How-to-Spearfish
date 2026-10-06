#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/SpearfishInteractable.h"
#include "SpearfishPullable.generated.h"

class UBoxComponent;

UENUM(BlueprintType)
enum class ESpearfishPullableKind : uint8
{
	Crate,		// wreck salvage, can be pried open
	Chest,		// treasure event, heavy, holds valuables
	Boulder		// no contents; blocks gaps, can be dragged with the line
};

/**
 * A physics prop the speargun line can grab and drag (server-simulated, movement replicated). Water gives
 * it heavy drag and near-neutral buoyancy so a diver can tow it, but it settles back to the bottom.
 * Crates and chests can be opened in place for their loot; what doesn't fit in the bag stays inside.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishPullable : public AActor, public ISpearfishInteractable
{
	GENERATED_BODY()

public:
	ASpearfishPullable();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	/** Server: call before FinishSpawning. */
	void ConfigureContents(ESpearfishPullableKind InKind, FName InLootId, int32 InCount);

	ESpearfishPullableKind GetKind() const { return Kind; }
	int32 GetRemaining() const { return Remaining; }

	// ISpearfishInteractable
	virtual bool CanInteract(const ASpearfishCharacter* Character) const override;
	virtual FText GetInteractPrompt(const ASpearfishCharacter* Character) const override;
	virtual void Interact(ASpearfishCharacter* Character) override;
	virtual float GetInteractRange() const override { return 260.f; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Kind();

	void BuildVisuals();
	void UpdateLid();
	void ApplyWaterForces();

	/** Collision + physics body (the speargun line pulls the root primitive). */
	UPROPERTY(VisibleAnywhere, Category = "Pullable")
	TObjectPtr<UBoxComponent> Body;

	/** Hinge for the lid parts (crates and chests). */
	UPROPERTY(VisibleAnywhere, Category = "Pullable")
	TObjectPtr<USceneComponent> Lid;

	UPROPERTY(ReplicatedUsing = OnRep_Kind)
	ESpearfishPullableKind Kind = ESpearfishPullableKind::Crate;

	UPROPERTY(Replicated)
	FName LootId;

	UPROPERTY(ReplicatedUsing = OnRep_Kind)
	int32 Remaining = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Kind)
	bool bOpened = false;

	bool bVisualsBuilt = false;
};
