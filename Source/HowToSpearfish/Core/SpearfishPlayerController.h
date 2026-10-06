#pragma once

#include "Core/SpearfishGameTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Rules/CookingRules.h"
#include "UI/SpearfishUIModel.h"
#include "SpearfishPlayerController.generated.h"

class USpearfishInputConfig;
struct FInputActionValue;

/** One line in the local message feed (notices and radio). */
struct FSpearfishFeedEntry
{
	FText Text;
	FLinearColor Color = FLinearColor::White;
	float Time = 0.f;
	bool bRadio = false;
	ESpearfishNoticeType Type = ESpearfishNoticeType::Info;
};

/**
 * Owns everything local to one player's screen: input contexts, the open UI panel and its selection, the
 * running cooking minigame, fades and the message feed. Gameplay requests go to the server through the
 * RPCs below; the server answers with client RPCs (notices, panels, minigame starts, day transitions).
 * Panels are data-driven by SpearfishUI::BuildRows so the HUD only draws what the controller acts on.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASpearfishPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;

	// --- Queries used by the pawn and HUD ---------------------------------------------------------
	const USpearfishInputConfig* GetInputConfig() const { return InputConfig; }
	/** True while a panel or fade owns the screen: the pawn ignores movement and gear input. */
	bool IsGameplayInputBlocked() const;
	bool IsLookBlocked() const;

	ESpearfishUIPanel GetOpenPanel() const { return CurrentPanel; }
	int32 GetUITab() const { return UITab; }
	int32 GetUISelection() const { return UISelection; }
	int32 GetPendingConfirm() const { return PendingConfirmSelection; }
	const FSpearfishMinigameState& GetMinigame() const { return Minigame; }
	bool IsMinigameRunning() const { return CurrentPanel == ESpearfishUIPanel::Minigame && !Minigame.bFinished; }
	float GetMinigameResultTimer() const { return MinigameResultTimer; }
	float GetFadeAmount() const { return FadeAmount; }
	const FSpearfishDaySummary& GetLastSummary() const { return LastSummary; }
	const TArray<FSpearfishFeedEntry>& GetFeed() const { return Feed; }
	float GetRoleBannerTime() const { return RoleBannerTime; }
	bool IsTalking() const { return bTalking; }
	FName GetPendingTravel() const { return PendingTravel; }
	/** Rows of the open panel (rebuilt on demand; cheap). */
	TArray<FSpearfishUIRow> BuildCurrentRows() const;
	TArray<FText> GetCurrentTabs() const;

	void OpenPanel(ESpearfishUIPanel Panel);
	void ClosePanel();
	void AddFeed(const FText& Text, ESpearfishNoticeType Type, bool bRadio);

	// --- Server -> client ---------------------------------------------------------------------------
	UFUNCTION(Client, Reliable)
	void ClientNotice(const FSpearfishNotice& Notice);

	UFUNCTION(Client, Reliable)
	void ClientOpenPanel(ESpearfishUIPanel Panel);

	UFUNCTION(Client, Reliable)
	void ClientBeginMinigame(int32 DishId, int32 StepIndex, ESpearfishCookStep Step, float Difficulty, int32 Seed, float ZoneBonus);

	/** Night: fade to black over the given time (the summary follows). */
	UFUNCTION(Client, Reliable)
	void ClientBeginNight(float FadeSeconds);

	UFUNCTION(Client, Reliable)
	void ClientShowDaySummary(const FSpearfishDaySummary& Summary);

	/** Morning: fade in and show the role banner. */
	UFUNCTION(Client, Reliable)
	void ClientWakeUp(int32 Day, ESpearfishRole NewRole, bool bRoleChanged);

	UFUNCTION(Client, Reliable)
	void ClientSetPendingTravel(FName RegionId);

	// --- Client -> server ---------------------------------------------------------------------------
	UFUNCTION(Server, Reliable)
	void ServerStartDish(int32 OrderId);

	UFUNCTION(Server, Reliable)
	void ServerCancelDish(int32 DishId);

	UFUNCTION(Server, Reliable)
	void ServerSubmitCookStep(int32 DishId, int32 StepIndex, float Score);

	UFUNCTION(Server, Reliable)
	void ServerAbortCookStep(int32 DishId);

	UFUNCTION(Server, Reliable)
	void ServerQuickMessage(ESpearfishQuickMessage Type, FName Param, int32 Count, float MinLengthCm);

	UFUNCTION(Server, Reliable)
	void ServerBuyEquipment(FName EquipmentId);

	UFUNCTION(Server, Reliable)
	void ServerEquip(FName EquipmentId);

	UFUNCTION(Server, Reliable)
	void ServerBuyUpgrade(FName UpgradeId);

	UFUNCTION(Server, Reliable)
	void ServerUnlockRegion(FName RegionId);

	UFUNCTION(Server, Reliable)
	void ServerSetTravelDestination(FName RegionId);

	UFUNCTION(Server, Reliable)
	void ServerSwitchToDiver();

	UFUNCTION(Server, Reliable)
	void ServerSetRestaurantOpen(bool bOpen);

	UFUNCTION(Server, Reliable)
	void ServerSaveCampaign();

	UFUNCTION(Server, Reliable)
	void ServerDebugCommand(const FString& Command, const FString& Argument);

	// --- Console commands (development builds) -------------------------------------------------------
	UFUNCTION(Exec)
	void SpearfishMoney(int32 Amount);

	UFUNCTION(Exec)
	void SpearfishHour(float Hour);

	UFUNCTION(Exec)
	void SpearfishEndDay();

	UFUNCTION(Exec)
	void SpearfishSpawnFish(FName SpeciesId);

	UFUNCTION(Exec)
	void SpearfishRole(FName RoleName);

	UFUNCTION(Exec)
	void SpearfishEvent(FName EventId);

private:
	// Input
	void Input_Tablet(const FInputActionValue& Value);
	void Input_Journal(const FInputActionValue& Value);
	void Input_Help(const FInputActionValue& Value);
	void Input_Pause(const FInputActionValue& Value);
	void Input_TalkStarted(const FInputActionValue& Value);
	void Input_TalkStopped(const FInputActionValue& Value);
	void Input_QuickComm(int32 Index);
	void Input_UIUp(const FInputActionValue& Value);
	void Input_UIDown(const FInputActionValue& Value);
	void Input_UILeft(const FInputActionValue& Value);
	void Input_UIRight(const FInputActionValue& Value);
	void Input_UIConfirm(const FInputActionValue& Value);
	void Input_UIConfirmReleased(const FInputActionValue& Value);
	void Input_UIBack(const FInputActionValue& Value);

	void MoveSelection(int32 Delta);
	void ChangeTab(int32 Delta);
	void ExecuteRow(const FSpearfishUIRow& Row);
	void SendQuickMessage(ESpearfishQuickMessage Type);
	void TickMinigame(float DeltaTime);
	void FinishMinigame();
	void UpdateInputContexts();
	void BindGameStateEvents();
	void HandleGameStateNotice(const FSpearfishNotice& Notice);
	void HandleQuickMessage(const FSpearfishQuickMessageData& Message);
	void ReturnToMenu();
	bool CanUseTabletHere() const;

	UPROPERTY(Transient)
	TObjectPtr<USpearfishInputConfig> InputConfig;

	ESpearfishUIPanel CurrentPanel = ESpearfishUIPanel::None;
	int32 UITab = 0;
	int32 UISelection = 0;
	int32 PendingConfirmSelection = INDEX_NONE;
	bool bUIContextActive = false;

	// Minigame (local simulation; the score is submitted to the server)
	FSpearfishMinigameState Minigame;
	int32 MinigameDishId = INDEX_NONE;
	int32 MinigameStepIndex = 0;
	float MinigameResultTimer = 0.f;
	bool bMinigameSubmitted = false;

	// Presentation
	float FadeAmount = 0.f;
	float FadeTarget = 0.f;
	float FadeSpeed = 1.f;
	float RoleBannerTime = 0.f;
	FSpearfishDaySummary LastSummary;
	TArray<FSpearfishFeedEntry> Feed;
	FName PendingTravel;
	float LastQuickMessageTime = -100.f;
	float LastServerQuickMessageTime = -100.f;
	bool bTalking = false;
	bool bBoundGameState = false;
};
