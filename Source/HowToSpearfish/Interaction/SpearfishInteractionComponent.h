#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "SpearfishInteractionComponent.generated.h"

class ASpearfishCharacter;

/**
 * Finds the interactable under the crosshair (locally, every frame) and forwards the Interact key to the
 * server, which re-validates range and CanInteract before calling ISpearfishInteractable::Interact.
 */
UCLASS(ClassGroup = (Spearfish), meta = (BlueprintSpawnableComponent))
class HOWTOSPEARFISH_API USpearfishInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpearfishInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	AActor* GetFocusedActor() const { return FocusedActor.Get(); }
	FText GetFocusedPrompt() const;

	/** Local: interact with whatever is focused. */
	void TryInteract();

	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* Target);

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float TraceDistance = 320.f;

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float TraceRadius = 14.f;

private:
	ASpearfishCharacter* GetCharacter() const;
	AActor* FindFocus() const;

	TWeakObjectPtr<AActor> FocusedActor;
};
