#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Rules/DayRules.h"
#include "Rules/OrderRules.h"
#include "Rules/OxygenRules.h"
#include "SpearfishSettings.generated.h"

class UDataTable;
class UMaterialInterface;

/**
 * Project-wide tuning and content references (Project Settings > Game > How to Spearfish).
 * Values persist in Config/DefaultGame.ini under [/Script/HowToSpearfish.SpearfishSettings].
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "How to Spearfish"))
class HOWTOSPEARFISH_API USpearfishSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USpearfishSettings();

	static const USpearfishSettings* Get() { return GetDefault<USpearfishSettings>(); }

	// --- Campaign -----------------------------------------------------------------------------
	UPROPERTY(config, EditAnywhere, Category = "Campaign")
	FName StartingRegion = TEXT("CoralCove");

	UPROPERTY(config, EditAnywhere, Category = "Campaign")
	int32 StartingMoney = 60;

	/** Charged when a blacked-out diver is hauled back aboard. */
	UPROPERTY(config, EditAnywhere, Category = "Campaign")
	int32 RescueFee = 20;

	/** Charged when the crew passes out instead of going to bed. */
	UPROPERTY(config, EditAnywhere, Category = "Campaign")
	int32 PassOutFee = 30;

	/** Quality lost by fish left in the cooler overnight. */
	UPROPERTY(config, EditAnywhere, Category = "Campaign")
	float OvernightSpoilage = 0.12f;

	// --- Rules tuning -----------------------------------------------------------------------
	UPROPERTY(config, EditAnywhere, Category = "Tuning")
	FSpearfishDaySchedule DaySchedule;

	UPROPERTY(config, EditAnywhere, Category = "Tuning")
	FSpearfishOxygenTuning Oxygen;

	UPROPERTY(config, EditAnywhere, Category = "Tuning")
	FSpearfishServiceTuning Service;

	/** Diver acceleration (cm/s^2) per unit of line tension. Big fish tow divers. */
	UPROPERTY(config, EditAnywhere, Category = "Tuning")
	float LinePullOnDiverScale = 1.1f;

	/** Seconds after blackout before the diver is hauled back aboard. */
	UPROPERTY(config, EditAnywhere, Category = "Tuning")
	float RescueDelaySeconds = 3.5f;

	/** Solo automated kitchen skill (0..1) before upgrades. */
	UPROPERTY(config, EditAnywhere, Category = "Tuning")
	float AutoChefBaseSkill = 0.68f;

	UPROPERTY(config, EditAnywhere, Category = "Tuning")
	int32 MaxActiveDishes = 3;

	UPROPERTY(config, EditAnywhere, Category = "Tuning")
	int32 BaseSeats = 2;

	UPROPERTY(config, EditAnywhere, Category = "Tuning")
	float MinGuestArrivalSeconds = 35.f;

	UPROPERTY(config, EditAnywhere, Category = "Tuning")
	float MaxGuestArrivalSeconds = 70.f;

	/** Seconds before a caught/eaten species respawns elsewhere in the region. */
	UPROPERTY(config, EditAnywhere, Category = "Tuning")
	float FishRespawnSeconds = 90.f;

	/** Loose loot pickups placed per region each morning. */
	UPROPERTY(config, EditAnywhere, Category = "Tuning")
	int32 DailyLootPickups = 16;

	// --- World ------------------------------------------------------------------------------
	UPROPERTY(config, EditAnywhere, Category = "World")
	float SeaLevelZ = 0.f;

	// --- Data -------------------------------------------------------------------------------
	/** Folder (relative to Content/) holding the JSON data used when no DataTable is assigned. */
	UPROPERTY(config, EditAnywhere, Category = "Data")
	FString JsonDataDirectory = TEXT("Data/Source");

	UPROPERTY(config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> FishTable;

	UPROPERTY(config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> EquipmentTable;

	UPROPERTY(config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> RecipeTable;

	UPROPERTY(config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> CustomerTable;

	UPROPERTY(config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> RegionTable;

	UPROPERTY(config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> LootTable;

	UPROPERTY(config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> UpgradeTable;

	UPROPERTY(config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> EventTable;

	// --- Presentation -----------------------------------------------------------------------
	/** Opaque material with a vector parameter (BaseColorParameter) used for all placeholder geometry. */
	UPROPERTY(config, EditAnywhere, Category = "Presentation")
	TSoftObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(config, EditAnywhere, Category = "Presentation")
	TSoftObjectPtr<UMaterialInterface> FallbackBaseMaterial;

	UPROPERTY(config, EditAnywhere, Category = "Presentation")
	FName BaseColorParameter = TEXT("Color");

	/** Optional emissive strength parameter of BaseMaterial. */
	UPROPERTY(config, EditAnywhere, Category = "Presentation")
	FName EmissiveParameter = TEXT("Emissive");

	/** Translucent ocean surface seen from above. Created by Tools/Editor/bootstrap_content.py. */
	UPROPERTY(config, EditAnywhere, Category = "Presentation")
	TSoftObjectPtr<UMaterialInterface> WaterSurfaceMaterial;

	/** Surface seen from below (bright, rippling). */
	UPROPERTY(config, EditAnywhere, Category = "Presentation")
	TSoftObjectPtr<UMaterialInterface> WaterUndersideMaterial;

	/** Optional post-process material blended in underwater (caustics, distortion). */
	UPROPERTY(config, EditAnywhere, Category = "Presentation")
	TSoftObjectPtr<UMaterialInterface> UnderwaterPostProcessMaterial;

	// --- Development ------------------------------------------------------------------------
	/** Play-In-Editor skips the main menu and starts a session directly. */
	UPROPERTY(config, EditAnywhere, Category = "Development")
	bool bAutoStartSessionInPIE = true;

	/** When auto-starting in PIE: co-op rules (listen server) instead of solo. */
	UPROPERTY(config, EditAnywhere, Category = "Development")
	bool bPIEStartsCoop = false;

	/** PIE sessions start from a fresh campaign instead of loading the save. */
	UPROPERTY(config, EditAnywhere, Category = "Development")
	bool bPIEFreshCampaign = true;
};
