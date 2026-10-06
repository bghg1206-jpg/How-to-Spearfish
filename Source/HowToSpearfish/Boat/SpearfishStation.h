#pragma once

#include "Core/SpearfishGameTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/SpearfishInteractable.h"
#include "SpearfishStation.generated.h"

class ASpearfishBoat;
class ASpearfishCharacter;
class UBoxComponent;
class UTextRenderComponent;

/**
 * Everything usable on the boat and the hub: helm, anchor, dive ladder, gear locker, cooler, the cooking
 * stations, the service pass, bunks, chart table, open sign, dive shop and journal board. One class with a
 * type switch keeps stations data-light; per-type visuals are built from placeholder shapes.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishStation : public AActor, public ISpearfishInteractable
{
	GENERATED_BODY()

public:
	ASpearfishStation();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server: call before FinishSpawning. */
	void Configure(ESpearfishStationType InType, int32 InIndex, ASpearfishBoat* InBoat);

	ESpearfishStationType GetStationType() const { return StationType; }
	int32 GetIndex() const { return Index; }
	ASpearfishBoat* GetBoat() const { return Boat; }
	ASpearfishCharacter* GetOccupant() const { return Occupant; }
	void SetOccupant(ASpearfishCharacter* InOccupant) { Occupant = InOccupant; }

	/** Bunk helpers. */
	FTransform GetLieTransform() const;
	FTransform GetExitTransform() const;

	// ISpearfishInteractable
	virtual bool CanInteract(const ASpearfishCharacter* Character) const override;
	virtual FText GetInteractPrompt(const ASpearfishCharacter* Character) const override;
	virtual void Interact(ASpearfishCharacter* Character) override;
	virtual float GetInteractRange() const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Type();

	void BuildVisuals();

	UPROPERTY(VisibleAnywhere, Category = "Station")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Station")
	TObjectPtr<UBoxComponent> InteractBox;

	UPROPERTY(VisibleAnywhere, Category = "Station")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(ReplicatedUsing = OnRep_Type)
	ESpearfishStationType StationType = ESpearfishStationType::Cooler;

	UPROPERTY(Replicated)
	int32 Index = 0;

	UPROPERTY(Replicated)
	TObjectPtr<ASpearfishBoat> Boat;

	UPROPERTY(Replicated)
	TObjectPtr<ASpearfishCharacter> Occupant;

	bool bVisualsBuilt = false;
};
