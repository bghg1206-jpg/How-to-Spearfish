#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "SpearfishMenuGameMode.generated.h"

class SSpearfishMainMenu;
class SWidget;

/** Boot/menu mode on the empty Entry map: no pawns, just the Slate main menu. */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASpearfishMenuGameMode();

	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override { return false; }
};

/** Menu-only controller: owns the main menu widget and UI-only input. */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASpearfishMenuPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TSharedPtr<SSpearfishMainMenu> Menu;
	TSharedPtr<SWidget> MenuContainer;
};
