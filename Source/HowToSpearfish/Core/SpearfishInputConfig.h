#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SpearfishInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Enhanced Input actions and mapping contexts created in code, so the project boots without input assets.
 * Gameplay context is always active; the UI context (higher priority) is pushed while a panel such as the
 * tablet, a cooking minigame or the shop is open and consumes movement keys.
 * Designers can replace these with authored assets later; the bindings only reference the UInputAction pointers.
 */
UCLASS()
class HOWTOSPEARFISH_API USpearfishInputConfig : public UObject
{
	GENERATED_BODY()

public:
	void Build();

	static constexpr int32 GameplayPriority = 0;
	static constexpr int32 UIPriority = 10;
	static constexpr int32 QuickCommCount = 6;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> GameplayContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> UIContext;

	// Gameplay
	UPROPERTY(Transient) TObjectPtr<UInputAction> Move;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Look;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Ascend;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Descend;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Sprint;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Fire;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Reel;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Release;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Interact;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ToggleLight;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Tablet;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Journal;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Help;
	UPROPERTY(Transient) TObjectPtr<UInputAction> PushToTalk;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Pause;
	UPROPERTY(Transient) TArray<TObjectPtr<UInputAction>> QuickComms;

	// UI
	UPROPERTY(Transient) TObjectPtr<UInputAction> UIUp;
	UPROPERTY(Transient) TObjectPtr<UInputAction> UIDown;
	UPROPERTY(Transient) TObjectPtr<UInputAction> UILeft;
	UPROPERTY(Transient) TObjectPtr<UInputAction> UIRight;
	UPROPERTY(Transient) TObjectPtr<UInputAction> UIConfirm;
	UPROPERTY(Transient) TObjectPtr<UInputAction> UIBack;

private:
	UInputAction* MakeAction(const TCHAR* Name, bool bAxis2D);
};
