#pragma once

#include "Core/SpearfishGameTypes.h"
#include "CoreMinimal.h"

class APlayerController;
struct FSpearfishEquipmentDef;

/** What confirming a panel row does (executed by the player controller). */
enum class ESpearfishUIAction : uint8
{
	None,
	StartDish,
	ScrapDish,
	SendQuickMessage,
	ToggleOpen,
	SwitchToDiver,
	BuyEquipment,
	Equip,
	BuyUpgrade,
	UnlockRegion,
	TravelRegion,
	Resume,
	OpenHelp,
	SaveQuit,
	QuitGame
};

/** One line of a panel. Headers are drawn but never selected. */
struct FSpearfishUIRow
{
	FText Label;
	FText Detail;
	/** Right-aligned value: price, timer, state. */
	FText Value;
	FLinearColor Color = FLinearColor::White;
	FLinearColor ValueColor = FLinearColor(0.85f, 0.85f, 0.85f);
	ESpearfishUIAction Action = ESpearfishUIAction::None;
	FName Id;
	int32 IntId = INDEX_NONE;
	float Number = 0.f;
	int32 Count = 0;
	bool bEnabled = true;
	bool bHeader = false;
	/** Destructive actions need a second confirm. */
	bool bNeedsConfirm = false;

	bool IsSelectable() const { return !bHeader; }
};

/**
 * The view-model for every menu panel. Rows are rebuilt from replicated state whenever they are drawn or
 * confirmed, so the HUD and the controller always agree on what a selection means and nothing goes stale.
 */
namespace SpearfishUI
{
	HOWTOSPEARFISH_API TArray<FText> GetTabs(ESpearfishUIPanel Panel);
	/** @param PendingTravel region the boat will sail to tonight (chart). */
	HOWTOSPEARFISH_API TArray<FSpearfishUIRow> BuildRows(const APlayerController* Controller, ESpearfishUIPanel Panel, int32 Tab, FName PendingTravel = NAME_None);
	HOWTOSPEARFISH_API FText PanelTitle(ESpearfishUIPanel Panel);
	HOWTOSPEARFISH_API FText DescribeEquipment(const FSpearfishEquipmentDef& Def);
	/** First selectable row at or after Index (wrapping), INDEX_NONE when the list has none. */
	HOWTOSPEARFISH_API int32 ClampSelection(const TArray<FSpearfishUIRow>& Rows, int32 Index, int32 Direction);
	/** Tablet tab indices. */
	enum ETabletTab : int32 { TabletOrders = 0, TabletKitchen, TabletCooler, TabletDiver, TabletComms, TabletRestaurant };
}
