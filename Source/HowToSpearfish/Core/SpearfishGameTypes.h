#pragma once

#include "CoreMinimal.h"
#include "Rules/SpearfishRulesTypes.h"
#include "SpearfishGameTypes.generated.h"

/** Preset radio callouts. They complement voice chat and keep the information asymmetry intact. */
UENUM(BlueprintType)
enum class ESpearfishQuickMessage : uint8
{
	// Chef -> Diver
	ComeBackNow,
	OrderExpiring,
	NeedFish,
	TooSmall,
	GreatCatch,
	MovingBoat,
	WatchYourAir,
	// Diver -> Chef
	FoundRare,
	BagFull,
	LowAir,
	SharkHelp,
	BringBoat,
	OnMyWay,
	// Anyone
	Yes,
	No,
	// Automated kitchen (solo / partner missing)
	KitchenNeeds
};

USTRUCT(BlueprintType)
struct FSpearfishQuickMessageData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Comms")
	ESpearfishQuickMessage Type = ESpearfishQuickMessage::Yes;

	UPROPERTY(BlueprintReadOnly, Category = "Comms")
	FString SenderName;

	UPROPERTY(BlueprintReadOnly, Category = "Comms")
	ESpearfishRole SenderRole = ESpearfishRole::None;

	/** Species for NeedFish / KitchenNeeds. */
	UPROPERTY(BlueprintReadOnly, Category = "Comms")
	FName Param;

	UPROPERTY(BlueprintReadOnly, Category = "Comms")
	int32 Count = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Comms")
	float MinLengthCm = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Comms")
	float ServerTime = 0.f;
};

UENUM(BlueprintType)
enum class ESpearfishNoticeType : uint8
{
	Info,
	Good,
	Warning,
	Danger,
	Discovery,
	Money
};

USTRUCT(BlueprintType)
struct FSpearfishNotice
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Notice")
	FText Text;

	UPROPERTY(BlueprintReadOnly, Category = "Notice")
	ESpearfishNoticeType Type = ESpearfishNoticeType::Info;
};

UENUM(BlueprintType)
enum class ESpearfishStationType : uint8
{
	Helm,
	Anchor,
	Ladder,
	GearLocker,
	Cooler,
	CuttingBoard,
	SpiceStation,
	Grill,
	Fryer,
	Stove,
	PlatingCounter,
	Pass,
	TabletDock,
	Bed,
	Chart,
	OpenSign,
	ShopKiosk,
	JournalBoard
};

UENUM(BlueprintType)
enum class ESpearfishUIPanel : uint8
{
	None,
	Tablet,
	Minigame,
	Shop,
	Locker,
	Chart,
	Journal,
	Help,
	DaySummary
};

USTRUCT(BlueprintType)
struct FSpearfishDayStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Day = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Revenue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Tips = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 DishesServed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 GuestsLost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 FishCaught = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 LootValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 NewSpecies = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Expenses = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float ReputationStart = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float ReputationEnd = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float BestDishQuality = 0.f;
};

USTRUCT(BlueprintType)
struct FSpearfishDaySummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	FSpearfishDayStats Stats;

	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	bool bPassedOut = false;

	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	int32 SpoiledFish = 0;
};

/** Display helpers shared by HUD, tablet and notifications. */
namespace SpearfishText
{
	HOWTOSPEARFISH_API FText RoleName(ESpearfishRole Role);
	HOWTOSPEARFISH_API FText RoleDuty(ESpearfishRole Role, bool bSolo);
	HOWTOSPEARFISH_API FLinearColor RoleColor(ESpearfishRole Role);
	HOWTOSPEARFISH_API FText PhaseName(ESpearfishDayPhase Phase);
	HOWTOSPEARFISH_API FText StepName(ESpearfishCookStep Step);
	HOWTOSPEARFISH_API FText RarityName(ESpearfishRarity Rarity);
	HOWTOSPEARFISH_API FLinearColor RarityColor(ESpearfishRarity Rarity);
	HOWTOSPEARFISH_API FText SlotName(ESpearfishEquipmentSlot Slot);
	HOWTOSPEARFISH_API FText RequirementText(ESpearfishRequirementResult Result);
	HOWTOSPEARFISH_API FText StationName(ESpearfishStationType Type);
	HOWTOSPEARFISH_API FText QualityName(float Quality);
	HOWTOSPEARFISH_API FString Money(int32 Coins);
	HOWTOSPEARFISH_API FString Clock(float Hour);
	HOWTOSPEARFISH_API FString Length(float LengthCm);
}

namespace SpearfishStations
{
	/** Which cook step a station performs (Fillet and Chop share the cutting board). */
	HOWTOSPEARFISH_API bool SupportsStep(ESpearfishStationType Station, ESpearfishCookStep Step);
	HOWTOSPEARFISH_API ESpearfishStationType StationForStep(ESpearfishCookStep Step);
	HOWTOSPEARFISH_API bool IsCookStation(ESpearfishStationType Station);
}
