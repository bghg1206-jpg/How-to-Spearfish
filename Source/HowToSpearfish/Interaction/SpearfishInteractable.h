#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SpearfishInteractable.generated.h"

class ASpearfishCharacter;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class USpearfishInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything the player can use with the Interact key: boat stations, loot, beds, hooked fish, clams.
 * CanInteract/GetInteractPrompt run on every machine (prompts); Interact runs on the server only.
 */
class HOWTOSPEARFISH_API ISpearfishInteractable
{
	GENERATED_BODY()

public:
	virtual bool CanInteract(const ASpearfishCharacter* Character) const { return true; }

	virtual FText GetInteractPrompt(const ASpearfishCharacter* Character) const { return FText::GetEmpty(); }

	/** Server only. */
	virtual void Interact(ASpearfishCharacter* Character) {}

	/** Max distance (cm) from the character's eyes to the interaction point. */
	virtual float GetInteractRange() const { return 260.f; }
};
