#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/SpearfishInteractable.h"
#include "SpearfishLootPickup.generated.h"

class USphereComponent;

/**
 * A piece of loose loot on the seabed (shell, coin, bottle, artifact). Spawned by the server each morning;
 * the look is built locally from the loot definition. Rare finds glint so they read from a distance.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishLootPickup : public AActor, public ISpearfishInteractable
{
	GENERATED_BODY()

public:
	ASpearfishLootPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	/** Server: call before FinishSpawning. */
	void SetLoot(FName InLootId, float InQuality);

	FName GetLootId() const { return LootId; }

	// ISpearfishInteractable
	virtual bool CanInteract(const ASpearfishCharacter* Character) const override;
	virtual FText GetInteractPrompt(const ASpearfishCharacter* Character) const override;
	virtual void Interact(ASpearfishCharacter* Character) override;
	virtual float GetInteractRange() const override { return 220.f; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Loot();

	void BuildVisuals();

	UPROPERTY(VisibleAnywhere, Category = "Loot")
	TObjectPtr<USphereComponent> InteractSphere;

	UPROPERTY(VisibleAnywhere, Category = "Loot")
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(ReplicatedUsing = OnRep_Loot)
	FName LootId;

	UPROPERTY(Replicated)
	float Quality = 1.f;

	float GlintPhase = 0.f;
	bool bGlints = false;
	bool bVisualsBuilt = false;
	bool bCollected = false;
};
