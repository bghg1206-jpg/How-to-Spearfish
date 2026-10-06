#include "Core/SpearfishPlayerController.h"

#include "Boat/SpearfishBoat.h"
#include "Core/SpearfishGameInstance.h"
#include "Core/SpearfishGameMode.h"
#include "Core/SpearfishGameState.h"
#include "Core/SpearfishInputConfig.h"
#include "Core/SpearfishPlayerState.h"
#include "Data/SpearfishDataRegistry.h"
#include "Diving/SpearfishCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "FishAI/SpearfishFish.h"
#include "HowToSpearfish.h"
#include "InputActionValue.h"
#include "Interaction/SpearfishInteractionComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Progression/SpearfishProgressionComponent.h"
#include "Restaurant/SpearfishRestaurantComponent.h"
#include "Roles/SpearfishRoleSubsystem.h"
#include "Speargun/SpeargunComponent.h"

#define LOCTEXT_NAMESPACE "SpearfishPlayerController"

namespace SpearfishPlayerControllerPrivate
{
	constexpr int32 MaxFeedEntries = 40;
	constexpr float QuickMessageCooldown = 0.6f;
	constexpr float MinigameResultSeconds = 1.4f;
	constexpr float RoleBannerSeconds = 7.f;

	FLinearColor NoticeColor(ESpearfishNoticeType Type)
	{
		switch (Type)
		{
		case ESpearfishNoticeType::Good: return FLinearColor(0.5f, 1.f, 0.55f);
		case ESpearfishNoticeType::Warning: return FLinearColor(1.f, 0.78f, 0.3f);
		case ESpearfishNoticeType::Danger: return FLinearColor(1.f, 0.38f, 0.32f);
		case ESpearfishNoticeType::Discovery: return FLinearColor(0.8f, 0.6f, 1.f);
		case ESpearfishNoticeType::Money: return FLinearColor(1.f, 0.9f, 0.35f);
		default: return FLinearColor(0.92f, 0.94f, 0.97f);
		}
	}
}

ASpearfishPlayerController::ASpearfishPlayerController()
{
	bShowMouseCursor = false;
}

// ------------------------------------------------------------------------------------- Setup

void ASpearfishPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		if (!InputConfig)
		{
			InputConfig = NewObject<USpearfishInputConfig>(this);
			InputConfig->Build();
		}
		SetInputMode(FInputModeGameOnly());
		UpdateInputContexts();
		BindGameStateEvents();
	}
}

void ASpearfishPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputConfig)
	{
		InputConfig = NewObject<USpearfishInputConfig>(this);
		InputConfig->Build();
	}
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);
	if (!Input)
	{
		UE_LOG(LogSpearfish, Error, TEXT("Enhanced Input is not the default input component class; check Config/DefaultInput.ini."));
		return;
	}
	Input->BindAction(InputConfig->Tablet, ETriggerEvent::Started, this, &ASpearfishPlayerController::Input_Tablet);
	Input->BindAction(InputConfig->Journal, ETriggerEvent::Started, this, &ASpearfishPlayerController::Input_Journal);
	Input->BindAction(InputConfig->Help, ETriggerEvent::Started, this, &ASpearfishPlayerController::Input_Help);
	Input->BindAction(InputConfig->Pause, ETriggerEvent::Started, this, &ASpearfishPlayerController::Input_Pause);
	Input->BindAction(InputConfig->PushToTalk, ETriggerEvent::Started, this, &ASpearfishPlayerController::Input_TalkStarted);
	Input->BindAction(InputConfig->PushToTalk, ETriggerEvent::Completed, this, &ASpearfishPlayerController::Input_TalkStopped);
	for (int32 Index = 0; Index < InputConfig->QuickComms.Num(); ++Index)
	{
		Input->BindAction(InputConfig->QuickComms[Index], ETriggerEvent::Started, this, &ASpearfishPlayerController::Input_QuickComm, Index);
	}
	Input->BindAction(InputConfig->UIUp, ETriggerEvent::Started, this, &ASpearfishPlayerController::Input_UIUp);
	Input->BindAction(InputConfig->UIDown, ETriggerEvent::Started, this, &ASpearfishPlayerController::Input_UIDown);
	Input->BindAction(InputConfig->UILeft, ETriggerEvent::Started, this, &ASpearfishPlayerController::Input_UILeft);
	Input->BindAction(InputConfig->UIRight, ETriggerEvent::Started, this, &ASpearfishPlayerController::Input_UIRight);
	Input->BindAction(InputConfig->UIConfirm, ETriggerEvent::Started, this, &ASpearfishPlayerController::Input_UIConfirm);
	Input->BindAction(InputConfig->UIConfirm, ETriggerEvent::Completed, this, &ASpearfishPlayerController::Input_UIConfirmReleased);
	Input->BindAction(InputConfig->UIBack, ETriggerEvent::Started, this, &ASpearfishPlayerController::Input_UIBack);
	UpdateInputContexts();
}

void ASpearfishPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (ASpearfishCharacter* Character = Cast<ASpearfishCharacter>(InPawn))
	{
		Character->ApplyRoleLoadout(true);
	}
}

void ASpearfishPlayerController::UpdateInputContexts()
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Subsystem || !InputConfig)
	{
		return;
	}
	if (!Subsystem->HasMappingContext(InputConfig->GameplayContext))
	{
		Subsystem->AddMappingContext(InputConfig->GameplayContext, USpearfishInputConfig::GameplayPriority);
	}
	const bool bWantUI = CurrentPanel != ESpearfishUIPanel::None;
	if (bWantUI && !bUIContextActive)
	{
		Subsystem->AddMappingContext(InputConfig->UIContext, USpearfishInputConfig::UIPriority);
		bUIContextActive = true;
	}
	else if (!bWantUI && bUIContextActive)
	{
		Subsystem->RemoveMappingContext(InputConfig->UIContext);
		bUIContextActive = false;
	}
}

void ASpearfishPlayerController::BindGameStateEvents()
{
	ASpearfishGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASpearfishGameState>() : nullptr;
	if (bBoundGameState || !GameState || !IsLocalController())
	{
		return;
	}
	bBoundGameState = true;
	GameState->OnNotice.AddUObject(this, &ASpearfishPlayerController::HandleGameStateNotice);
	GameState->OnQuickMessage.AddUObject(this, &ASpearfishPlayerController::HandleQuickMessage);
}

// ------------------------------------------------------------------------------------ Queries

bool ASpearfishPlayerController::IsGameplayInputBlocked() const
{
	return CurrentPanel != ESpearfishUIPanel::None || FadeAmount > 0.6f;
}

bool ASpearfishPlayerController::IsLookBlocked() const
{
	return CurrentPanel != ESpearfishUIPanel::None;
}

TArray<FSpearfishUIRow> ASpearfishPlayerController::BuildCurrentRows() const
{
	return SpearfishUI::BuildRows(this, CurrentPanel, UITab, PendingTravel);
}

TArray<FText> ASpearfishPlayerController::GetCurrentTabs() const
{
	return SpearfishUI::GetTabs(CurrentPanel);
}

bool ASpearfishPlayerController::CanUseTabletHere() const
{
	const ASpearfishPlayerState* State = GetPlayerState<ASpearfishPlayerState>();
	if (State && State->IsChef())
	{
		return true;
	}
	// The diver reads the tablet on deck, never underwater.
	const ASpearfishCharacter* Character = Cast<ASpearfishCharacter>(GetPawn());
	return Character && !Character->IsSwimming();
}

// ------------------------------------------------------------------------------------- Panels

void ASpearfishPlayerController::OpenPanel(ESpearfishUIPanel Panel)
{
	if (Panel == ESpearfishUIPanel::None)
	{
		ClosePanel();
		return;
	}
	if (CurrentPanel == ESpearfishUIPanel::Minigame && Panel != ESpearfishUIPanel::Minigame && IsMinigameRunning())
	{
		// Never silently replace a running minigame.
		return;
	}
	if (CurrentPanel != Panel)
	{
		UITab = 0;
	}
	CurrentPanel = Panel;
	PendingConfirmSelection = INDEX_NONE;
	UISelection = SpearfishUI::ClampSelection(BuildCurrentRows(), 0, 1);
	UpdateInputContexts();
}

void ASpearfishPlayerController::ClosePanel()
{
	if (CurrentPanel == ESpearfishUIPanel::Minigame && !bMinigameSubmitted && MinigameDishId != INDEX_NONE)
	{
		ServerAbortCookStep(MinigameDishId);
		MinigameDishId = INDEX_NONE;
	}
	CurrentPanel = ESpearfishUIPanel::None;
	PendingConfirmSelection = INDEX_NONE;
	UpdateInputContexts();
}

void ASpearfishPlayerController::MoveSelection(int32 Delta)
{
	const TArray<FSpearfishUIRow> Rows = BuildCurrentRows();
	if (Rows.Num() == 0)
	{
		UISelection = INDEX_NONE;
		return;
	}
	const int32 Start = UISelection == INDEX_NONE ? 0 : UISelection + Delta;
	UISelection = SpearfishUI::ClampSelection(Rows, Start, Delta);
	PendingConfirmSelection = INDEX_NONE;
}

void ASpearfishPlayerController::ChangeTab(int32 Delta)
{
	const int32 TabCount = GetCurrentTabs().Num();
	if (TabCount <= 1)
	{
		return;
	}
	UITab = (UITab + Delta + TabCount) % TabCount;
	PendingConfirmSelection = INDEX_NONE;
	UISelection = SpearfishUI::ClampSelection(BuildCurrentRows(), 0, 1);
}

void ASpearfishPlayerController::ExecuteRow(const FSpearfishUIRow& Row)
{
	if (!Row.bEnabled || Row.Action == ESpearfishUIAction::None)
	{
		return;
	}
	if (Row.bNeedsConfirm && PendingConfirmSelection != UISelection)
	{
		PendingConfirmSelection = UISelection;
		return;
	}
	PendingConfirmSelection = INDEX_NONE;

	switch (Row.Action)
	{
	case ESpearfishUIAction::StartDish:
		ServerStartDish(Row.IntId);
		break;
	case ESpearfishUIAction::ScrapDish:
		ServerCancelDish(Row.IntId);
		break;
	case ESpearfishUIAction::SendQuickMessage:
		ServerQuickMessage(static_cast<ESpearfishQuickMessage>(Row.IntId), Row.Id, Row.Count, Row.Number);
		break;
	case ESpearfishUIAction::ToggleOpen:
	{
		const ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
		const USpearfishRestaurantComponent* Restaurant = GameState && GameState->GetBoat() ? GameState->GetBoat()->GetRestaurant() : nullptr;
		ServerSetRestaurantOpen(!(Restaurant && Restaurant->IsOpen()));
		break;
	}
	case ESpearfishUIAction::SwitchToDiver:
		ServerSwitchToDiver();
		ClosePanel();
		break;
	case ESpearfishUIAction::BuyEquipment:
		ServerBuyEquipment(Row.Id);
		break;
	case ESpearfishUIAction::Equip:
		ServerEquip(Row.Id);
		break;
	case ESpearfishUIAction::BuyUpgrade:
		ServerBuyUpgrade(Row.Id);
		break;
	case ESpearfishUIAction::UnlockRegion:
		ServerUnlockRegion(Row.Id);
		break;
	case ESpearfishUIAction::TravelRegion:
		ServerSetTravelDestination(Row.Id);
		break;
	case ESpearfishUIAction::Resume:
		ClosePanel();
		break;
	case ESpearfishUIAction::OpenHelp:
		OpenPanel(ESpearfishUIPanel::Help);
		break;
	case ESpearfishUIAction::SaveQuit:
		ReturnToMenu();
		break;
	case ESpearfishUIAction::QuitGame:
		if (HasAuthority())
		{
			if (ASpearfishGameMode* GameMode = GetWorld()->GetAuthGameMode<ASpearfishGameMode>())
			{
				GameMode->SaveCampaign();
			}
		}
		UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
		break;
	default:
		break;
	}
}

void ASpearfishPlayerController::ReturnToMenu()
{
	if (HasAuthority())
	{
		if (ASpearfishGameMode* GameMode = GetWorld()->GetAuthGameMode<ASpearfishGameMode>())
		{
			GameMode->SaveCampaign();
		}
	}
	if (USpearfishGameInstance* GameInstance = GetGameInstance<USpearfishGameInstance>())
	{
		GameInstance->LeaveToMenu();
	}
}

// -------------------------------------------------------------------------------------- Input

void ASpearfishPlayerController::Input_Tablet(const FInputActionValue& Value)
{
	if (CurrentPanel == ESpearfishUIPanel::Tablet)
	{
		ClosePanel();
		return;
	}
	if (CurrentPanel != ESpearfishUIPanel::None)
	{
		return;
	}
	if (CanUseTabletHere())
	{
		OpenPanel(ESpearfishUIPanel::Tablet);
	}
	else
	{
		AddFeed(LOCTEXT("TabletOnBoat", "The tablet stays dry on the boat. Radio the kitchen with 1-6."), ESpearfishNoticeType::Info, false);
	}
}

void ASpearfishPlayerController::Input_Journal(const FInputActionValue& Value)
{
	if (CurrentPanel == ESpearfishUIPanel::Journal)
	{
		ClosePanel();
	}
	else if (CurrentPanel == ESpearfishUIPanel::None)
	{
		OpenPanel(ESpearfishUIPanel::Journal);
	}
}

void ASpearfishPlayerController::Input_Help(const FInputActionValue& Value)
{
	if (CurrentPanel == ESpearfishUIPanel::Help)
	{
		ClosePanel();
	}
	else if (CurrentPanel == ESpearfishUIPanel::None)
	{
		OpenPanel(ESpearfishUIPanel::Help);
	}
}

void ASpearfishPlayerController::Input_Pause(const FInputActionValue& Value)
{
	if (CurrentPanel == ESpearfishUIPanel::None)
	{
		OpenPanel(ESpearfishUIPanel::Pause);
	}
}

void ASpearfishPlayerController::Input_TalkStarted(const FInputActionValue& Value)
{
	bTalking = true;
	ToggleSpeaking(true);
}

void ASpearfishPlayerController::Input_TalkStopped(const FInputActionValue& Value)
{
	bTalking = false;
	ToggleSpeaking(false);
}

void ASpearfishPlayerController::Input_QuickComm(int32 Index)
{
	const ASpearfishPlayerState* State = GetPlayerState<ASpearfishPlayerState>();
	const TArray<ESpearfishQuickMessage> Hotkeys = SpearfishComms::HotkeysForRole(State ? State->GetRole() : ESpearfishRole::Diver);
	if (Hotkeys.IsValidIndex(Index))
	{
		SendQuickMessage(Hotkeys[Index]);
	}
}

void ASpearfishPlayerController::SendQuickMessage(ESpearfishQuickMessage Type)
{
	const float Now = GetWorld()->GetRealTimeSeconds();
	if (Now - LastQuickMessageTime < SpearfishPlayerControllerPrivate::QuickMessageCooldown)
	{
		return;
	}
	LastQuickMessageTime = Now;

	FName Param;
	int32 Count = 0;
	float MinLength = 0.f;
	if (Type == ESpearfishQuickMessage::NeedFish)
	{
		// The most urgent thing the kitchen is missing.
		const ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
		const USpearfishRestaurantComponent* Restaurant = GameState && GameState->GetBoat() ? GameState->GetBoat()->GetRestaurant() : nullptr;
		if (Restaurant && Restaurant->GetKitchenNeeds().Num() > 0)
		{
			const FSpearfishKitchenNeed& Need = Restaurant->GetKitchenNeeds()[0];
			Param = Need.SpeciesId.IsNone() ? Need.Category : Need.SpeciesId;
			Count = Need.Count;
			MinLength = Need.MinLengthCm;
		}
	}
	else if (Type == ESpearfishQuickMessage::FoundRare)
	{
		// Name what the diver is looking at or fighting, if anything.
		if (const ASpearfishCharacter* Character = Cast<ASpearfishCharacter>(GetPawn()))
		{
			const ASpearfishFish* Fish = Character->GetSpeargun() ? Character->GetSpeargun()->GetHookedFish() : nullptr;
			if (!Fish && Character->GetInteraction())
			{
				Fish = Cast<ASpearfishFish>(Character->GetInteraction()->GetFocusedActor());
			}
			Param = Fish ? Fish->GetSpeciesId() : NAME_None;
		}
	}
	ServerQuickMessage(Type, Param, Count, MinLength);
}

void ASpearfishPlayerController::Input_UIUp(const FInputActionValue& Value)
{
	if (CurrentPanel == ESpearfishUIPanel::Minigame)
	{
		SpearfishMinigame::Input(Minigame, ESpearfishMinigameInput::Up);
		return;
	}
	MoveSelection(-1);
}

void ASpearfishPlayerController::Input_UIDown(const FInputActionValue& Value)
{
	if (CurrentPanel == ESpearfishUIPanel::Minigame)
	{
		SpearfishMinigame::Input(Minigame, ESpearfishMinigameInput::Down);
		return;
	}
	MoveSelection(1);
}

void ASpearfishPlayerController::Input_UILeft(const FInputActionValue& Value)
{
	if (CurrentPanel == ESpearfishUIPanel::Minigame)
	{
		SpearfishMinigame::Input(Minigame, ESpearfishMinigameInput::Left);
		return;
	}
	ChangeTab(-1);
}

void ASpearfishPlayerController::Input_UIRight(const FInputActionValue& Value)
{
	if (CurrentPanel == ESpearfishUIPanel::Minigame)
	{
		SpearfishMinigame::Input(Minigame, ESpearfishMinigameInput::Right);
		return;
	}
	ChangeTab(1);
}

void ASpearfishPlayerController::Input_UIConfirm(const FInputActionValue& Value)
{
	switch (CurrentPanel)
	{
	case ESpearfishUIPanel::Minigame:
		if (!Minigame.bFinished)
		{
			SpearfishMinigame::Input(Minigame, ESpearfishMinigameInput::Press);
		}
		else if (bMinigameSubmitted)
		{
			ClosePanel();
		}
		break;
	case ESpearfishUIPanel::Help:
	case ESpearfishUIPanel::DaySummary:
		ClosePanel();
		break;
	default:
	{
		const TArray<FSpearfishUIRow> Rows = BuildCurrentRows();
		if (Rows.IsValidIndex(UISelection))
		{
			ExecuteRow(Rows[UISelection]);
		}
		break;
	}
	}
}

void ASpearfishPlayerController::Input_UIConfirmReleased(const FInputActionValue& Value)
{
	if (CurrentPanel == ESpearfishUIPanel::Minigame && !Minigame.bFinished)
	{
		SpearfishMinigame::Input(Minigame, ESpearfishMinigameInput::Release);
	}
}

void ASpearfishPlayerController::Input_UIBack(const FInputActionValue& Value)
{
	if (PendingConfirmSelection != INDEX_NONE)
	{
		PendingConfirmSelection = INDEX_NONE;
		return;
	}
	ClosePanel();
}

// ----------------------------------------------------------------------------------- Ticking

void ASpearfishPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!IsLocalController())
	{
		return;
	}
	BindGameStateEvents();

	FadeAmount = FMath::FInterpConstantTo(FadeAmount, FadeTarget, DeltaTime, FadeSpeed);
	RoleBannerTime = FMath::Max(0.f, RoleBannerTime - DeltaTime);
	TickMinigame(DeltaTime);

	const float Now = GetWorld()->GetRealTimeSeconds();
	Feed.RemoveAll([Now](const FSpearfishFeedEntry& Entry) { return Now - Entry.Time > 120.f; });
}

void ASpearfishPlayerController::TickMinigame(float DeltaTime)
{
	if (CurrentPanel != ESpearfishUIPanel::Minigame)
	{
		return;
	}
	if (!Minigame.bFinished)
	{
		SpearfishMinigame::Tick(Minigame, DeltaTime);
		if (Minigame.bFinished)
		{
			FinishMinigame();
		}
		return;
	}
	MinigameResultTimer -= DeltaTime;
	if (MinigameResultTimer <= 0.f)
	{
		ClosePanel();
	}
}

void ASpearfishPlayerController::FinishMinigame()
{
	if (bMinigameSubmitted || MinigameDishId == INDEX_NONE)
	{
		return;
	}
	bMinigameSubmitted = true;
	MinigameResultTimer = SpearfishPlayerControllerPrivate::MinigameResultSeconds;
	ServerSubmitCookStep(MinigameDishId, MinigameStepIndex, FMath::Clamp(Minigame.Score, 0.f, 1.f));
	AddFeed(FText::Format(LOCTEXT("StepResult", "{0}: {1}%"), SpearfishText::StepName(Minigame.Step), FText::AsNumber(FMath::RoundToInt(Minigame.Score * 100.f))),
		Minigame.Score >= 0.7f ? ESpearfishNoticeType::Good : (Minigame.Score >= 0.4f ? ESpearfishNoticeType::Info : ESpearfishNoticeType::Warning), false);
	MinigameDishId = INDEX_NONE;
}

// ------------------------------------------------------------------------------------- Feed

void ASpearfishPlayerController::AddFeed(const FText& Text, ESpearfishNoticeType Type, bool bRadio)
{
	if (Text.IsEmpty())
	{
		return;
	}
	FSpearfishFeedEntry Entry;
	Entry.Text = Text;
	Entry.Type = Type;
	Entry.bRadio = bRadio;
	Entry.Color = bRadio ? FLinearColor(0.55f, 0.95f, 1.f) : SpearfishPlayerControllerPrivate::NoticeColor(Type);
	Entry.Time = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
	Feed.Add(Entry);
	if (Feed.Num() > SpearfishPlayerControllerPrivate::MaxFeedEntries)
	{
		Feed.RemoveAt(0, Feed.Num() - SpearfishPlayerControllerPrivate::MaxFeedEntries);
	}
}

void ASpearfishPlayerController::HandleGameStateNotice(const FSpearfishNotice& Notice)
{
	AddFeed(Notice.Text, Notice.Type, false);
}

void ASpearfishPlayerController::HandleQuickMessage(const FSpearfishQuickMessageData& Message)
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FText ParamName = Message.Param.IsNone() ? FText::GetEmpty() : (Registry ? Registry->GetDisplayName(Message.Param) : FText::FromName(Message.Param));
	const FText Line = SpearfishText::QuickMessage(Message.Type, ParamName, Message.Count, Message.MinLengthCm);
	const FText Sender = Message.SenderName.IsEmpty() ? SpearfishText::RoleName(Message.SenderRole) : FText::FromString(Message.SenderName);
	AddFeed(FText::Format(LOCTEXT("RadioLine", "[{0}] {1}"), Sender, Line), ESpearfishNoticeType::Info, true);
}

// ---------------------------------------------------------------------------- Client RPCs

void ASpearfishPlayerController::ClientNotice_Implementation(const FSpearfishNotice& Notice)
{
	AddFeed(Notice.Text, Notice.Type, false);
}

void ASpearfishPlayerController::ClientOpenPanel_Implementation(ESpearfishUIPanel Panel)
{
	if (Panel == ESpearfishUIPanel::Tablet && CurrentPanel == ESpearfishUIPanel::Tablet)
	{
		return;
	}
	OpenPanel(Panel);
}

void ASpearfishPlayerController::ClientBeginMinigame_Implementation(int32 DishId, int32 StepIndex, ESpearfishCookStep Step, float Difficulty, int32 Seed, float ZoneBonus)
{
	if (IsMinigameRunning() && MinigameDishId != INDEX_NONE && MinigameDishId != DishId)
	{
		// Already busy with another dish: give this one back.
		ServerAbortCookStep(DishId);
		return;
	}
	SpearfishMinigame::Start(Minigame, Step, Difficulty, Seed, ZoneBonus);
	MinigameDishId = DishId;
	MinigameStepIndex = StepIndex;
	bMinigameSubmitted = false;
	MinigameResultTimer = 0.f;
	if (CurrentPanel != ESpearfishUIPanel::None && CurrentPanel != ESpearfishUIPanel::Minigame)
	{
		CurrentPanel = ESpearfishUIPanel::None;
	}
	OpenPanel(ESpearfishUIPanel::Minigame);
}

void ASpearfishPlayerController::ClientBeginNight_Implementation(float FadeSeconds)
{
	if (CurrentPanel != ESpearfishUIPanel::None)
	{
		ClosePanel();
	}
	FadeTarget = 1.f;
	FadeSpeed = 1.f / FMath::Max(FadeSeconds, 0.1f);
}

void ASpearfishPlayerController::ClientShowDaySummary_Implementation(const FSpearfishDaySummary& Summary)
{
	LastSummary = Summary;
	OpenPanel(ESpearfishUIPanel::DaySummary);
}

void ASpearfishPlayerController::ClientWakeUp_Implementation(int32 Day, ESpearfishRole Role, bool bRoleChanged)
{
	FadeTarget = 0.f;
	FadeSpeed = 0.5f;
	RoleBannerTime = SpearfishPlayerControllerPrivate::RoleBannerSeconds;
	PendingTravel = NAME_None;
	if (bRoleChanged)
	{
		AddFeed(FText::Format(LOCTEXT("RoleSwap", "Day {0}: roles swapped - you are the {1} today."), FText::AsNumber(Day), SpearfishText::RoleName(Role)),
			ESpearfishNoticeType::Good, false);
	}
}

void ASpearfishPlayerController::ClientSetPendingTravel_Implementation(FName RegionId)
{
	PendingTravel = RegionId;
}

// ---------------------------------------------------------------------------- Server RPCs

namespace SpearfishPlayerControllerPrivate
{
	USpearfishRestaurantComponent* GetRestaurant(const UWorld* World)
	{
		const ASpearfishGameState* GameState = World ? World->GetGameState<ASpearfishGameState>() : nullptr;
		return GameState && GameState->GetBoat() ? GameState->GetBoat()->GetRestaurant() : nullptr;
	}

	USpearfishProgressionComponent* GetProgression(const UWorld* World)
	{
		const ASpearfishGameState* GameState = World ? World->GetGameState<ASpearfishGameState>() : nullptr;
		return GameState ? GameState->GetProgression() : nullptr;
	}
}

void ASpearfishPlayerController::ServerStartDish_Implementation(int32 OrderId)
{
	if (USpearfishRestaurantComponent* Restaurant = SpearfishPlayerControllerPrivate::GetRestaurant(GetWorld()))
	{
		FText Reason;
		if (!Restaurant->TryStartDish(OrderId, PlayerState, false, Reason) && !Reason.IsEmpty())
		{
			FSpearfishNotice Notice;
			Notice.Text = Reason;
			Notice.Type = ESpearfishNoticeType::Warning;
			ClientNotice(Notice);
		}
	}
}

void ASpearfishPlayerController::ServerCancelDish_Implementation(int32 DishId)
{
	if (USpearfishRestaurantComponent* Restaurant = SpearfishPlayerControllerPrivate::GetRestaurant(GetWorld()))
	{
		Restaurant->CancelDish(DishId);
	}
}

void ASpearfishPlayerController::ServerSubmitCookStep_Implementation(int32 DishId, int32 StepIndex, float Score)
{
	if (USpearfishRestaurantComponent* Restaurant = SpearfishPlayerControllerPrivate::GetRestaurant(GetWorld()))
	{
		Restaurant->SubmitCookStep(DishId, StepIndex, FMath::Clamp(Score, 0.f, 1.f), PlayerState, false);
	}
}

void ASpearfishPlayerController::ServerAbortCookStep_Implementation(int32 DishId)
{
	if (USpearfishRestaurantComponent* Restaurant = SpearfishPlayerControllerPrivate::GetRestaurant(GetWorld()))
	{
		Restaurant->AbortCookStep(DishId, PlayerState);
	}
}

void ASpearfishPlayerController::ServerQuickMessage_Implementation(ESpearfishQuickMessage Type, FName Param, int32 Count, float MinLengthCm)
{
	ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	const ASpearfishPlayerState* State = GetPlayerState<ASpearfishPlayerState>();
	if (!GameState || !State || Type == ESpearfishQuickMessage::KitchenNeeds)
	{
		return;
	}
	// Server-side flood guard (the client limits too).
	const float Now = GameState->GetServerTime();
	if (Now - LastServerQuickMessageTime < 0.3f && Now >= LastServerQuickMessageTime)
	{
		return;
	}
	LastServerQuickMessageTime = Now;

	FSpearfishQuickMessageData Message;
	Message.Type = Type;
	Message.SenderName = State->GetPlayerName();
	Message.SenderRole = State->GetRole();
	Message.Param = Param;
	Message.Count = FMath::Clamp(Count, 0, 99);
	Message.MinLengthCm = FMath::Clamp(MinLengthCm, 0.f, 500.f);
	Message.ServerTime = Now;
	GameState->BroadcastQuickMessage(Message);
}

void ASpearfishPlayerController::ServerBuyEquipment_Implementation(FName EquipmentId)
{
	USpearfishProgressionComponent* Progression = SpearfishPlayerControllerPrivate::GetProgression(GetWorld());
	ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	if (!Progression || !GameState || !Registry)
	{
		return;
	}
	const ESpearfishRequirementResult Result = Progression->TryPurchaseEquipment(EquipmentId);
	if (Result == ESpearfishRequirementResult::Ok)
	{
		GameState->BroadcastNotice(FText::Format(LOCTEXT("Bought", "{0} bought: {1} (equipped)"), FText::FromString(PlayerState->GetPlayerName()),
			Registry->GetDisplayName(EquipmentId)), ESpearfishNoticeType::Money);
	}
	else
	{
		FSpearfishNotice Notice;
		Notice.Text = SpearfishText::RequirementText(Result);
		Notice.Type = ESpearfishNoticeType::Warning;
		ClientNotice(Notice);
	}
}

void ASpearfishPlayerController::ServerEquip_Implementation(FName EquipmentId)
{
	if (USpearfishProgressionComponent* Progression = SpearfishPlayerControllerPrivate::GetProgression(GetWorld()))
	{
		Progression->TryEquip(EquipmentId);
	}
}

void ASpearfishPlayerController::ServerBuyUpgrade_Implementation(FName UpgradeId)
{
	USpearfishProgressionComponent* Progression = SpearfishPlayerControllerPrivate::GetProgression(GetWorld());
	ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	if (!Progression || !GameState || !Registry)
	{
		return;
	}
	const ESpearfishRequirementResult Result = Progression->TryPurchaseUpgrade(UpgradeId);
	if (Result == ESpearfishRequirementResult::Ok)
	{
		GameState->BroadcastNotice(FText::Format(LOCTEXT("Upgraded", "New upgrade: {0}"), Registry->GetDisplayName(UpgradeId)), ESpearfishNoticeType::Money);
	}
	else
	{
		FSpearfishNotice Notice;
		Notice.Text = SpearfishText::RequirementText(Result);
		Notice.Type = ESpearfishNoticeType::Warning;
		ClientNotice(Notice);
	}
}

void ASpearfishPlayerController::ServerUnlockRegion_Implementation(FName RegionId)
{
	USpearfishProgressionComponent* Progression = SpearfishPlayerControllerPrivate::GetProgression(GetWorld());
	ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	if (!Progression || !GameState || !Registry)
	{
		return;
	}
	const ESpearfishRequirementResult Result = Progression->TryUnlockRegion(RegionId);
	if (Result == ESpearfishRequirementResult::Ok)
	{
		GameState->BroadcastNotice(FText::Format(LOCTEXT("Charted", "Charted a new region: {0}. Plot a course on the chart table."),
			Registry->GetDisplayName(RegionId)), ESpearfishNoticeType::Discovery);
	}
	else
	{
		FSpearfishNotice Notice;
		Notice.Text = SpearfishText::RequirementText(Result);
		Notice.Type = ESpearfishNoticeType::Warning;
		ClientNotice(Notice);
	}
}

void ASpearfishPlayerController::ServerSetTravelDestination_Implementation(FName RegionId)
{
	if (ASpearfishGameMode* GameMode = GetWorld()->GetAuthGameMode<ASpearfishGameMode>())
	{
		GameMode->SetTravelDestination(this, RegionId);
	}
}

void ASpearfishPlayerController::ServerSwitchToDiver_Implementation()
{
	USpearfishRoleSubsystem* Roles = GetWorld()->GetSubsystem<USpearfishRoleSubsystem>();
	ASpearfishPlayerState* State = GetPlayerState<ASpearfishPlayerState>();
	if (Roles && State && Roles->RequestSwitchToDiver(State))
	{
		if (ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>())
		{
			GameState->BroadcastNotice(FText::Format(LOCTEXT("Switched", "{0} grabbed the speargun - the auto-chef runs the kitchen."),
				FText::FromString(State->GetPlayerName())), ESpearfishNoticeType::Info);
		}
	}
}

void ASpearfishPlayerController::ServerSetRestaurantOpen_Implementation(bool bOpen)
{
	if (USpearfishRestaurantComponent* Restaurant = SpearfishPlayerControllerPrivate::GetRestaurant(GetWorld()))
	{
		Restaurant->SetOpen(bOpen);
	}
}

void ASpearfishPlayerController::ServerSaveCampaign_Implementation()
{
	// Only the host decides when the campaign is written.
	if (IsLocalController())
	{
		if (ASpearfishGameMode* GameMode = GetWorld()->GetAuthGameMode<ASpearfishGameMode>())
		{
			GameMode->SaveCampaign();
		}
	}
}

void ASpearfishPlayerController::ServerDebugCommand_Implementation(const FString& Command, const FString& Argument)
{
#if !UE_BUILD_SHIPPING
	if (ASpearfishGameMode* GameMode = GetWorld()->GetAuthGameMode<ASpearfishGameMode>())
	{
		GameMode->HandleDebugCommand(this, Command, Argument);
	}
#endif
}

// ------------------------------------------------------------------------- Console commands

void ASpearfishPlayerController::SpearfishMoney(int32 Amount)
{
	ServerDebugCommand(TEXT("Money"), FString::FromInt(Amount));
}

void ASpearfishPlayerController::SpearfishHour(float Hour)
{
	ServerDebugCommand(TEXT("Hour"), FString::SanitizeFloat(Hour));
}

void ASpearfishPlayerController::SpearfishEndDay()
{
	ServerDebugCommand(TEXT("EndDay"), FString());
}

void ASpearfishPlayerController::SpearfishSpawnFish(FName SpeciesId)
{
	ServerDebugCommand(TEXT("SpawnFish"), SpeciesId.ToString());
}

void ASpearfishPlayerController::SpearfishRole(FName Role)
{
	ServerDebugCommand(TEXT("Role"), Role.ToString());
}

void ASpearfishPlayerController::SpearfishEvent(FName EventId)
{
	ServerDebugCommand(TEXT("Event"), EventId.ToString());
}

#undef LOCTEXT_NAMESPACE
