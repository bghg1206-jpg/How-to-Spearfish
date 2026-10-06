#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/SpearfishInteractable.h"
#include "SpearfishGiantClam.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * A giant clam that slowly opens and closes. Some hold a pearl. Reaching in while it is open takes the
 * pearl; reaching in while it is closing (the shell trembles first) gets your hand caught - the clam
 * snaps, the diver gasps and loses air, and the pearl stays. A small timing challenge with no UI.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishGiantClam : public AActor, public ISpearfishInteractable
{
	GENERATED_BODY()

public:
	ASpearfishGiantClam();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	bool IsOpen() const { return bOpen; }
	bool HasPearl() const { return !PearlId.IsNone(); }
	/** 0 = shut, 1 = fully open (local visual state). */
	float GetOpenAmount() const { return OpenAmount; }

	// ISpearfishInteractable
	virtual bool CanInteract(const ASpearfishCharacter* Character) const override;
	virtual FText GetInteractPrompt(const ASpearfishCharacter* Character) const override;
	virtual void Interact(ASpearfishCharacter* Character) override;
	virtual float GetInteractRange() const override { return 240.f; }

protected:
	virtual void BeginPlay() override;

private:
	void BuildVisuals();
	void SetOpen(bool bInOpen);
	float GetPhaseRemaining() const;
	bool IsClosingSoon() const;

	UPROPERTY(VisibleAnywhere, Category = "Clam")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Clam")
	TObjectPtr<UBoxComponent> InteractBox;

	UPROPERTY(VisibleAnywhere, Category = "Clam")
	TObjectPtr<USceneComponent> Hinge;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> PearlMesh;

	UPROPERTY(Replicated)
	bool bOpen = false;

	/** Server time at which the current open/closed phase ends (drives the closing tremble on clients). */
	UPROPERTY(Replicated)
	float PhaseEndTime = 0.f;

	UPROPERTY(Replicated)
	FName PearlId;

	float OpenAmount = 0.f;
	float TremblePhase = 0.f;
	bool bVisualsBuilt = false;
};
