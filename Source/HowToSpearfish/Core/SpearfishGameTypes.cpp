#include "Core/SpearfishGameTypes.h"

#include "Rules/DayRules.h"

#define LOCTEXT_NAMESPACE "SpearfishText"

namespace SpearfishText
{
	FText RoleName(ESpearfishRole Role)
	{
		switch (Role)
		{
		case ESpearfishRole::Diver: return LOCTEXT("RoleDiver", "Diver");
		case ESpearfishRole::Chef: return LOCTEXT("RoleChef", "Chef");
		default: return LOCTEXT("RoleNone", "Guest");
		}
	}

	FText RoleDuty(ESpearfishRole Role, bool bSolo)
	{
		switch (Role)
		{
		case ESpearfishRole::Diver:
			return bSolo
				? LOCTEXT("DutyDiverSolo", "Catch what the kitchen radios for, mind your air, haul it back to the boat.")
				: LOCTEXT("DutyDiver", "Hunt the fish your chef asks for. Watch your air. Talk to your chef!");
		case ESpearfishRole::Chef:
			return LOCTEXT("DutyChef", "Run the floating restaurant: read orders on the tablet, guide the diver, cook and serve.");
		default:
			return LOCTEXT("DutyNone", "Waiting for a role.");
		}
	}

	FLinearColor RoleColor(ESpearfishRole Role)
	{
		switch (Role)
		{
		case ESpearfishRole::Diver: return FLinearColor(0.15f, 0.75f, 1.f);
		case ESpearfishRole::Chef: return FLinearColor(1.f, 0.6f, 0.2f);
		default: return FLinearColor(0.7f, 0.7f, 0.7f);
		}
	}

	FText PhaseName(ESpearfishDayPhase Phase)
	{
		switch (Phase)
		{
		case ESpearfishDayPhase::Morning: return LOCTEXT("PhaseMorning", "Morning prep");
		case ESpearfishDayPhase::Service: return LOCTEXT("PhaseService", "Service");
		case ESpearfishDayPhase::Closing: return LOCTEXT("PhaseClosing", "Last orders");
		case ESpearfishDayPhase::Night: return LOCTEXT("PhaseNight", "Night - go to bed");
		case ESpearfishDayPhase::Sleeping: return LOCTEXT("PhaseSleeping", "Sleeping");
		default: return FText::GetEmpty();
		}
	}

	FText StepName(ESpearfishCookStep Step)
	{
		switch (Step)
		{
		case ESpearfishCookStep::Fillet: return LOCTEXT("StepFillet", "Fillet");
		case ESpearfishCookStep::Chop: return LOCTEXT("StepChop", "Chop");
		case ESpearfishCookStep::Season: return LOCTEXT("StepSeason", "Season");
		case ESpearfishCookStep::Grill: return LOCTEXT("StepGrill", "Grill");
		case ESpearfishCookStep::Fry: return LOCTEXT("StepFry", "Fry");
		case ESpearfishCookStep::Simmer: return LOCTEXT("StepSimmer", "Simmer");
		case ESpearfishCookStep::Plate: return LOCTEXT("StepPlate", "Plate");
		default: return FText::GetEmpty();
		}
	}

	FText RarityName(ESpearfishRarity Rarity)
	{
		switch (Rarity)
		{
		case ESpearfishRarity::Common: return LOCTEXT("Common", "Common");
		case ESpearfishRarity::Uncommon: return LOCTEXT("Uncommon", "Uncommon");
		case ESpearfishRarity::Rare: return LOCTEXT("Rare", "Rare");
		case ESpearfishRarity::Epic: return LOCTEXT("Epic", "Epic");
		case ESpearfishRarity::Legendary: return LOCTEXT("Legendary", "Legendary");
		default: return FText::GetEmpty();
		}
	}

	FLinearColor RarityColor(ESpearfishRarity Rarity)
	{
		switch (Rarity)
		{
		case ESpearfishRarity::Common: return FLinearColor(0.85f, 0.85f, 0.85f);
		case ESpearfishRarity::Uncommon: return FLinearColor(0.35f, 0.9f, 0.4f);
		case ESpearfishRarity::Rare: return FLinearColor(0.3f, 0.6f, 1.f);
		case ESpearfishRarity::Epic: return FLinearColor(0.75f, 0.4f, 1.f);
		case ESpearfishRarity::Legendary: return FLinearColor(1.f, 0.75f, 0.15f);
		default: return FLinearColor::White;
		}
	}

	FText SlotName(ESpearfishEquipmentSlot Slot)
	{
		switch (Slot)
		{
		case ESpearfishEquipmentSlot::Speargun: return LOCTEXT("SlotGun", "Speargun");
		case ESpearfishEquipmentSlot::Reel: return LOCTEXT("SlotReel", "Line & Reel");
		case ESpearfishEquipmentSlot::Tank: return LOCTEXT("SlotTank", "Air Tank");
		case ESpearfishEquipmentSlot::Wetsuit: return LOCTEXT("SlotSuit", "Wetsuit");
		case ESpearfishEquipmentSlot::Mask: return LOCTEXT("SlotMask", "Mask");
		case ESpearfishEquipmentSlot::Fins: return LOCTEXT("SlotFins", "Fins");
		case ESpearfishEquipmentSlot::Bag: return LOCTEXT("SlotBag", "Catch Bag");
		case ESpearfishEquipmentSlot::Light: return LOCTEXT("SlotLight", "Dive Light");
		default: return FText::GetEmpty();
		}
	}

	FText RequirementText(ESpearfishRequirementResult Result)
	{
		switch (Result)
		{
		case ESpearfishRequirementResult::Ok: return LOCTEXT("ReqOk", "Available");
		case ESpearfishRequirementResult::AlreadyOwned: return LOCTEXT("ReqOwned", "Owned");
		case ESpearfishRequirementResult::NotEnoughMoney: return LOCTEXT("ReqMoney", "Not enough coins");
		case ESpearfishRequirementResult::NeedsReputation: return LOCTEXT("ReqRep", "Needs more reputation");
		case ESpearfishRequirementResult::NeedsDay: return LOCTEXT("ReqDay", "Available later");
		case ESpearfishRequirementResult::NeedsSpecies: return LOCTEXT("ReqSpecies", "Catch more species first");
		case ESpearfishRequirementResult::NeedsPrerequisite: return LOCTEXT("ReqPrereq", "Needs the previous tier");
		default: return FText::GetEmpty();
		}
	}

	FText StationName(ESpearfishStationType Type)
	{
		switch (Type)
		{
		case ESpearfishStationType::Helm: return LOCTEXT("StHelm", "Helm");
		case ESpearfishStationType::Anchor: return LOCTEXT("StAnchor", "Anchor winch");
		case ESpearfishStationType::Ladder: return LOCTEXT("StLadder", "Dive ladder");
		case ESpearfishStationType::GearLocker: return LOCTEXT("StLocker", "Gear locker");
		case ESpearfishStationType::Cooler: return LOCTEXT("StCooler", "Cooler");
		case ESpearfishStationType::CuttingBoard: return LOCTEXT("StBoard", "Cutting board");
		case ESpearfishStationType::SpiceStation: return LOCTEXT("StSpice", "Spice station");
		case ESpearfishStationType::Grill: return LOCTEXT("StGrill", "Grill");
		case ESpearfishStationType::Fryer: return LOCTEXT("StFryer", "Fryer");
		case ESpearfishStationType::Stove: return LOCTEXT("StStove", "Stove");
		case ESpearfishStationType::PlatingCounter: return LOCTEXT("StPlate", "Plating counter");
		case ESpearfishStationType::Pass: return LOCTEXT("StPass", "Service pass");
		case ESpearfishStationType::TabletDock: return LOCTEXT("StTablet", "Tablet");
		case ESpearfishStationType::Bed: return LOCTEXT("StBed", "Bunk");
		case ESpearfishStationType::Chart: return LOCTEXT("StChart", "Chart table");
		case ESpearfishStationType::OpenSign: return LOCTEXT("StSign", "Open sign");
		case ESpearfishStationType::ShopKiosk: return LOCTEXT("StShop", "Dive shop");
		case ESpearfishStationType::JournalBoard: return LOCTEXT("StJournal", "Fish journal");
		default: return FText::GetEmpty();
		}
	}

	FText QualityName(float Quality)
	{
		if (Quality >= 0.9f)
		{
			return LOCTEXT("QPerfect", "Perfect");
		}
		if (Quality >= 0.72f)
		{
			return LOCTEXT("QGreat", "Great");
		}
		if (Quality >= 0.45f)
		{
			return LOCTEXT("QGood", "Good");
		}
		return LOCTEXT("QPoor", "Poor");
	}

	FString Money(int32 Coins)
	{
		return FString::Printf(TEXT("%d c"), Coins);
	}

	FString Clock(float Hour)
	{
		int32 Hour24 = 0;
		int32 Minute = 0;
		SpearfishDay::ToClock(Hour, Hour24, Minute);
		return FString::Printf(TEXT("%02d:%02d"), Hour24, Minute);
	}

	FString Length(float LengthCm)
	{
		return FString::Printf(TEXT("%d cm"), FMath::RoundToInt(LengthCm));
	}
}

namespace SpearfishStations
{
	bool SupportsStep(ESpearfishStationType Station, ESpearfishCookStep Step)
	{
		switch (Step)
		{
		case ESpearfishCookStep::Fillet:
		case ESpearfishCookStep::Chop:
			return Station == ESpearfishStationType::CuttingBoard;
		case ESpearfishCookStep::Season:
			return Station == ESpearfishStationType::SpiceStation;
		case ESpearfishCookStep::Grill:
			return Station == ESpearfishStationType::Grill;
		case ESpearfishCookStep::Fry:
			return Station == ESpearfishStationType::Fryer;
		case ESpearfishCookStep::Simmer:
			return Station == ESpearfishStationType::Stove;
		case ESpearfishCookStep::Plate:
			return Station == ESpearfishStationType::PlatingCounter;
		default:
			return false;
		}
	}

	ESpearfishStationType StationForStep(ESpearfishCookStep Step)
	{
		switch (Step)
		{
		case ESpearfishCookStep::Fillet:
		case ESpearfishCookStep::Chop: return ESpearfishStationType::CuttingBoard;
		case ESpearfishCookStep::Season: return ESpearfishStationType::SpiceStation;
		case ESpearfishCookStep::Grill: return ESpearfishStationType::Grill;
		case ESpearfishCookStep::Fry: return ESpearfishStationType::Fryer;
		case ESpearfishCookStep::Simmer: return ESpearfishStationType::Stove;
		case ESpearfishCookStep::Plate: return ESpearfishStationType::PlatingCounter;
		default: return ESpearfishStationType::CuttingBoard;
		}
	}

	bool IsCookStation(ESpearfishStationType Station)
	{
		switch (Station)
		{
		case ESpearfishStationType::CuttingBoard:
		case ESpearfishStationType::SpiceStation:
		case ESpearfishStationType::Grill:
		case ESpearfishStationType::Fryer:
		case ESpearfishStationType::Stove:
		case ESpearfishStationType::PlatingCounter:
			return true;
		default:
			return false;
		}
	}
}

#undef LOCTEXT_NAMESPACE
