#include "Core/SpearfishMenuGameMode.h"

#include "Core/SpearfishGameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "UI/SpearfishMainMenu.h"
#include "Widgets/SWeakWidget.h"

ASpearfishMenuGameMode::ASpearfishMenuGameMode()
{
	PlayerControllerClass = ASpearfishMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

ASpearfishMenuPlayerController::ASpearfishMenuPlayerController()
{
	bShowMouseCursor = true;
}

void ASpearfishMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	USpearfishGameInstance* GameInstance = GetGameInstance<USpearfishGameInstance>();
	if (!IsLocalController() || !Viewport || !GameInstance)
	{
		return;
	}
	GameInstance->NotifyMenuReached();

	SAssignNew(Menu, SSpearfishMainMenu)
		.GameInstance(GameInstance)
		.Message(GameInstance->ConsumeLastError());
	MenuContainer = SNew(SWeakWidget).PossiblyNullContent(Menu.ToSharedRef());
	Viewport->AddViewportWidgetContent(MenuContainer.ToSharedRef(), 10);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Menu);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ASpearfishMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (Viewport && MenuContainer.IsValid())
	{
		Viewport->RemoveViewportWidgetContent(MenuContainer.ToSharedRef());
	}
	MenuContainer.Reset();
	Menu.Reset();
	Super::EndPlay(EndPlayReason);
}
