#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class USpearfishGameInstance;

/**
 * The main menu, built in Slate so it needs no widget assets: continue/new solo, host or join co-op by
 * address, quit. Destructive choices (overwriting a save) ask for a second click.
 */
class HOWTOSPEARFISH_API SSpearfishMainMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSpearfishMainMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<USpearfishGameInstance>, GameInstance)
		SLATE_ARGUMENT(FText, Message)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TSharedRef<SWidget> MakeButton(const FText& Label, const FText& Tooltip, FOnClicked OnClicked, const TAttribute<bool>& Enabled = true);

	FReply OnContinueSolo();
	FReply OnNewSolo();
	FReply OnContinueCoop();
	FReply OnNewCoop();
	FReply OnJoin();
	FReply OnQuit();

	FText GetStatusText() const { return Status; }
	FText GetNewSoloLabel() const;
	FText GetNewCoopLabel() const;

	TWeakObjectPtr<USpearfishGameInstance> GameInstance;
	TSharedPtr<SEditableTextBox> AddressBox;
	FText Status;
	bool bConfirmNewSolo = false;
	bool bConfirmNewCoop = false;
	bool bHasSoloSave = false;
	bool bHasCoopSave = false;
};
