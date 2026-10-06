#include "UI/SpearfishHUD.h"

#include "Boat/SpearfishBoat.h"
#include "Core/SpearfishGameState.h"
#include "Core/SpearfishPlayerController.h"
#include "Core/SpearfishPlayerState.h"
#include "Data/SpearfishDataRegistry.h"
#include "DayNight/SpearfishDayCycleComponent.h"
#include "Diving/SpearfishCharacter.h"
#include "Diving/SpearfishOxygenComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "Equipment/SpearfishEquipmentComponent.h"
#include "FishAI/SpearfishFish.h"
#include "Interaction/SpearfishInteractionComponent.h"
#include "Inventory/SpearfishDiveBagComponent.h"
#include "Progression/SpearfishProgressionComponent.h"
#include "Restaurant/SpearfishRestaurantComponent.h"
#include "Rules/CookingRules.h"
#include "Speargun/SpeargunComponent.h"
#include "UI/SpearfishUIModel.h"
#include "World/SpearfishOceanSubsystem.h"

#define LOCTEXT_NAMESPACE "SpearfishHUD"

namespace SpearfishHUDPrivate
{
	const FLinearColor PanelColor(0.015f, 0.05f, 0.08f, 0.9f);
	const FLinearColor PanelEdge(0.35f, 0.7f, 0.85f, 0.8f);
	const FLinearColor Ink(0.92f, 0.95f, 0.98f);
	const FLinearColor Dim(0.55f, 0.6f, 0.66f);
	const FLinearColor Gold(1.f, 0.85f, 0.45f);
	const FLinearColor Good(0.45f, 1.f, 0.55f);
	const FLinearColor Warn(1.f, 0.75f, 0.3f);
	const FLinearColor Bad(1.f, 0.35f, 0.3f);
	const FLinearColor Water(0.3f, 0.75f, 1.f);

	FLinearColor WithAlpha(FLinearColor Color, float Alpha)
	{
		Color.A *= FMath::Clamp(Alpha, 0.f, 1.f);
		return Color;
	}

	FString StepHint(ESpearfishCookStep Step)
	{
		switch (Step)
		{
		case ESpearfishCookStep::Fillet: return TEXT("Press [E] / [Space] / [LMB] as the knife crosses each cut mark.");
		case ESpearfishCookStep::Chop: return TEXT("Press on the beat as each mark reaches the line.");
		case ESpearfishCookStep::Season: return TEXT("Stop the swinging needle inside the green zone - twice.");
		case ESpearfishCookStep::Grill: return TEXT("Flip when the side is golden. Both sides!");
		case ESpearfishCookStep::Fry: return TEXT("Hold to heat, release to cool. Keep the oil inside the band.");
		case ESpearfishCookStep::Simmer: return TEXT("Hold to heat, release to cool. The pot reacts slowly - plan ahead.");
		case ESpearfishCookStep::Plate: return TEXT("Enter the arrows in order with W A S D or the arrow keys.");
		default: return FString();
		}
	}

	FString ArrowGlyph(ESpearfishMinigameInput Input)
	{
		switch (Input)
		{
		case ESpearfishMinigameInput::Up: return TEXT("^");
		case ESpearfishMinigameInput::Down: return TEXT("v");
		case ESpearfishMinigameInput::Left: return TEXT("<");
		case ESpearfishMinigameInput::Right: return TEXT(">");
		default: return TEXT("o");
		}
	}

	FString HitWord(float Score)
	{
		if (Score >= 0.95f) { return TEXT("PERFECT!"); }
		if (Score >= 0.7f) { return TEXT("Great"); }
		if (Score >= 0.35f) { return TEXT("Good"); }
		return TEXT("Miss");
	}
}

// ---------------------------------------------------------------------------------- Accessors

ASpearfishPlayerController* ASpearfishHUD::GetSpearfishController() const
{
	return Cast<ASpearfishPlayerController>(PlayerOwner);
}

ASpearfishCharacter* ASpearfishHUD::GetSpearfishCharacter() const
{
	return PlayerOwner ? Cast<ASpearfishCharacter>(PlayerOwner->GetPawn()) : nullptr;
}

ASpearfishGameState* ASpearfishHUD::GetSpearfishGameState() const
{
	return GetWorld() ? GetWorld()->GetGameState<ASpearfishGameState>() : nullptr;
}

// --------------------------------------------------------------------------------- Primitives

void ASpearfishHUD::Text(const FString& String, float X, float Y, const FLinearColor& Color, UFont* Font, float Scale, bool bShadow)
{
	if (String.IsEmpty())
	{
		return;
	}
	const float FinalScale = Scale * FontScale;
	if (bShadow)
	{
		DrawText(String, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.8f), X + 1.5f * UIScale, Y + 1.5f * UIScale, Font, FinalScale);
	}
	DrawText(String, Color, X, Y, Font, FinalScale);
}

FVector2D ASpearfishHUD::Measure(const FString& String, UFont* Font, float Scale)
{
	float Width = 0.f;
	float Height = 0.f;
	GetTextSize(String, Width, Height, Font, Scale * FontScale);
	return FVector2D(Width, Height);
}

void ASpearfishHUD::TextCentered(const FString& String, float CenterX, float Y, const FLinearColor& Color, UFont* Font, float Scale)
{
	Text(String, CenterX - static_cast<float>(Measure(String, Font, Scale).X) * 0.5f, Y, Color, Font, Scale);
}

void ASpearfishHUD::TextRight(const FString& String, float RightX, float Y, const FLinearColor& Color, UFont* Font, float Scale)
{
	Text(String, RightX - static_cast<float>(Measure(String, Font, Scale).X), Y, Color, Font, Scale);
}

void ASpearfishHUD::Box(float X, float Y, float W, float H, const FLinearColor& Color)
{
	DrawRect(Color, X, Y, W, H);
}

void ASpearfishHUD::Frame(float X, float Y, float W, float H, const FLinearColor& Color, float Thickness)
{
	const float T = FMath::Max(1.f, Thickness * UIScale);
	DrawRect(Color, X, Y, W, T);
	DrawRect(Color, X, Y + H - T, W, T);
	DrawRect(Color, X, Y, T, H);
	DrawRect(Color, X + W - T, Y, T, H);
}

void ASpearfishHUD::Bar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Fill, const FLinearColor& Back)
{
	DrawRect(Back, X, Y, W, H);
	DrawRect(Fill, X, Y, W * FMath::Clamp(Fraction, 0.f, 1.f), H);
}

void ASpearfishHUD::Marker(const FVector& World, const FString& Label, const FLinearColor& Color)
{
	FVector2D Screen;
	if (!PlayerOwner || !PlayerOwner->ProjectWorldLocationToScreen(World, Screen, false))
	{
		return;
	}
	const float X = FMath::Clamp(static_cast<float>(Screen.X), S(40.f), Canvas->ClipX - S(40.f));
	const float Y = FMath::Clamp(static_cast<float>(Screen.Y), S(60.f), Canvas->ClipY - S(160.f));
	Box(X - S(5.f), Y - S(5.f), S(10.f), S(10.f), Color);
	TextCentered(Label, X, Y + S(8.f), Color, SmallFont, 1.1f);
}

// ------------------------------------------------------------------------------------- Frame

void ASpearfishHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas || !GEngine)
	{
		return;
	}
	UIScale = Canvas->ClipY / 1080.f;
	FontScale = FMath::Clamp(UIScale, 0.7f, 2.5f);
	Time = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
	LargeFont = GEngine->GetLargeFont();
	MediumFont = GEngine->GetMediumFont();
	SmallFont = GEngine->GetSmallFont();

	const ASpearfishPlayerController* Controller = GetSpearfishController();
	if (!Controller)
	{
		return;
	}
	const ESpearfishUIPanel Panel = Controller->GetOpenPanel();
	const bool bWorldLayer = Panel == ESpearfishUIPanel::None || Panel == ESpearfishUIPanel::Minigame;
	if (bWorldLayer)
	{
		DrawMarkers();
		DrawCrosshairAndPrompt();
		DrawDiverGauges();
		DrawLineAndGun();
		DrawKitchenBoard();
		DrawContextHints();
	}
	DrawStatusBar();
	DrawFeed();
	DrawRoleBanner();
	DrawFadeAndBlackout();
	if (Panel != ESpearfishUIPanel::None)
	{
		DrawPanel();
	}
}

// ------------------------------------------------------------------------------- World layer

void ASpearfishHUD::DrawStatusBar()
{
	using namespace SpearfishHUDPrivate;
	const ASpearfishGameState* GameState = GetSpearfishGameState();
	const USpearfishDayCycleComponent* DayCycle = GameState ? GameState->GetDayCycle() : nullptr;
	const USpearfishProgressionComponent* Progression = GameState ? GameState->GetProgression() : nullptr;
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	if (!DayCycle || !Progression)
	{
		return;
	}
	const float X = S(24.f);
	const float Y = S(20.f);
	Box(X - S(10.f), Y - S(8.f), S(330.f), S(92.f), WithAlpha(PanelColor, 0.6f));
	const FString Line1 = FString::Printf(TEXT("Day %d   %s   %s"), DayCycle->GetDay(), *SpearfishText::Clock(DayCycle->GetHour()),
		*SpearfishText::PhaseName(DayCycle->GetPhase()).ToString());
	Text(Line1, X, Y, Ink, MediumFont, 0.85f);
	Text(SpearfishText::Money(Progression->GetMoney()), X, Y + S(28.f), Gold, MediumFont, 0.85f);
	const FString Rep = FString::Printf(TEXT("Reputation %.1f / 5"), Progression->GetReputation());
	Text(Rep, X + S(120.f), Y + S(30.f), Ink, SmallFont, 1.1f);
	Bar(X + S(120.f), Y + S(50.f), S(180.f), S(4.f), Progression->GetReputation() / 5.f, Gold, WithAlpha(Dim, 0.4f));
	if (Registry)
	{
		Text(Registry->GetDisplayName(GameState->GetCurrentRegion()).ToString(), X, Y + S(58.f), Water, SmallFont, 1.1f);
	}

	// Role badge, top right.
	const ASpearfishPlayerState* State = PlayerOwner ? PlayerOwner->GetPlayerState<ASpearfishPlayerState>() : nullptr;
	const ASpearfishPlayerController* Controller = GetSpearfishController();
	if (State)
	{
		const FString RoleLabel = SpearfishText::RoleName(State->GetRole()).ToString().ToUpper();
		TextRight(RoleLabel, Canvas->ClipX - S(24.f), Y, SpearfishText::RoleColor(State->GetRole()), LargeFont, 0.8f);
	}
	if (Controller && Controller->IsTalking())
	{
		TextRight(TEXT("(( talking ))"), Canvas->ClipX - S(24.f), Y + S(36.f), Good, SmallFont, 1.1f);
	}
	if (Controller && !Controller->GetPendingTravel().IsNone() && Registry)
	{
		TextRight(FString::Printf(TEXT("Course set: %s (sails tonight)"), *Registry->GetDisplayName(Controller->GetPendingTravel()).ToString()),
			Canvas->ClipX - S(24.f), Y + S(56.f), Water, SmallFont, 1.1f);
	}
}

void ASpearfishHUD::DrawRoleBanner()
{
	using namespace SpearfishHUDPrivate;
	const ASpearfishPlayerController* Controller = GetSpearfishController();
	const ASpearfishPlayerState* State = PlayerOwner ? PlayerOwner->GetPlayerState<ASpearfishPlayerState>() : nullptr;
	const ASpearfishGameState* GameState = GetSpearfishGameState();
	if (!Controller || !State || !GameState || Controller->GetRoleBannerTime() <= 0.f)
	{
		return;
	}
	const float Alpha = FMath::Clamp(Controller->GetRoleBannerTime() / 1.5f, 0.f, 1.f);
	const float CenterX = Canvas->ClipX * 0.5f;
	const float Y = Canvas->ClipY * 0.2f;
	const FLinearColor RoleColor = SpearfishText::RoleColor(State->GetRole());
	Box(CenterX - S(420.f), Y - S(16.f), S(840.f), S(130.f), WithAlpha(PanelColor, 0.75f * Alpha));
	TextCentered(FString::Printf(TEXT("DAY %d"), GameState->GetDayCycle() ? GameState->GetDayCycle()->GetDay() : 1), CenterX, Y, WithAlpha(Gold, Alpha), LargeFont, 0.9f);
	TextCentered(FText::Format(LOCTEXT("YouAre", "You are the {0}"), SpearfishText::RoleName(State->GetRole())).ToString(), CenterX, Y + S(40.f),
		WithAlpha(RoleColor, Alpha), LargeFont, 0.8f);
	TextCentered(SpearfishText::RoleDuty(State->GetRole(), GameState->IsSoloSession()).ToString(), CenterX, Y + S(80.f), WithAlpha(Ink, Alpha), SmallFont, 1.2f);
}

void ASpearfishHUD::DrawDiverGauges()
{
	using namespace SpearfishHUDPrivate;
	const ASpearfishCharacter* Character = GetSpearfishCharacter();
	if (!Character || Character->IsInBed())
	{
		return;
	}
	const USpearfishOxygenComponent* Oxygen = Character->GetOxygen();
	const USpearfishEquipmentComponent* Equipment = Character->GetEquipment();
	const bool bSwimming = Character->IsSwimming();
	const bool bHasTank = Equipment && Equipment->GetStats().bHasTank;
	if (!Oxygen || (!bSwimming && !bHasTank))
	{
		return;
	}

	const float X = S(30.f);
	float Y = Canvas->ClipY - S(190.f);
	Box(X - S(12.f), Y - S(12.f), S(380.f), S(176.f), WithAlpha(PanelColor, 0.55f));

	// Air gauge: analog-style bar with ticks, no numbers - read it like a pressure gauge.
	const float Air = Oxygen->GetAirFraction();
	const bool bCritical = Oxygen->IsCritical();
	const FLinearColor AirColor = Air > 0.5f ? Good : (Air > 0.25f ? Warn : Bad);
	const bool bBlink = bCritical && FMath::Fmod(Time, 0.6f) < 0.3f;
	Text(TEXT("AIR"), X, Y, bBlink ? Bad : Ink, SmallFont, 1.2f);
	Bar(X + S(50.f), Y + S(4.f), S(290.f), S(14.f), Air, AirColor, WithAlpha(Dim, 0.35f));
	for (int32 Tick = 1; Tick < 4; ++Tick)
	{
		Box(X + S(50.f) + S(290.f) * Tick / 4.f, Y + S(2.f), S(2.f), S(18.f), WithAlpha(Ink, 0.6f));
	}
	Y += S(30.f);
	if (Oxygen->GetBreath().bOutOfAir)
	{
		Text(FString::Printf(TEXT("OUT OF AIR - SURFACE!  (%.0f s of breath)"), Oxygen->GetBreath().BreathHoldRemaining), X, Y, bBlink ? Bad : Warn, SmallFont, 1.2f);
		Y += S(24.f);
	}

	// Depth gauge and the zone the diver is in.
	if (bSwimming)
	{
		const float Depth = FMath::Max(0.f, Character->GetDepthMeters());
		FString Zone;
		if (const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this))
		{
			if (const FSpearfishDepthZoneDef* DepthZone = Ocean->GetDepthZone(Depth))
			{
				Zone = DepthZone->DisplayName.ToString();
			}
		}
		Text(FString::Printf(TEXT("DEPTH %.0f m   %s"), Depth, *Zone), X, Y, Water, SmallFont, 1.2f);
		Y += S(24.f);
		if (Oxygen->IsOverDepth())
		{
			Text(FString::Printf(TEXT("Past your suit's comfort depth (%.0f m) - air drains faster"), Oxygen->GetSafeDepthM()), X, Y, Warn, SmallFont, 1.05f);
			Y += S(22.f);
		}
	}
	if (Oxygen->IsStung())
	{
		Text(TEXT("STUNG - breathing hard"), X, Y, FLinearColor(0.85f, 0.5f, 1.f), SmallFont, 1.15f);
		Y += S(22.f);
	}

	// Bag.
	if (const USpearfishDiveBagComponent* Bag = Character->GetBag())
	{
		const FSpearfishBagCapacity& Capacity = Bag->GetBag().Capacity;
		const FString BagText = FString::Printf(TEXT("BAG  fish %d/%d   loot %d/%d   %.1f/%.0f kg"), Bag->GetUsedFishSlots(), Capacity.FishSlots,
			Bag->GetUsedLootSlots(), Capacity.LootSlots, Bag->GetWeightKg(), Capacity.MaxWeightKg);
		const bool bFull = Bag->GetUsedFishSlots() >= Capacity.FishSlots;
		Text(BagText, X, Y, bFull ? Warn : Ink, SmallFont, 1.15f);
	}
}

void ASpearfishHUD::DrawLineAndGun()
{
	using namespace SpearfishHUDPrivate;
	const ASpearfishCharacter* Character = GetSpearfishCharacter();
	const USpeargunComponent* Gun = Character ? Character->GetSpeargun() : nullptr;
	if (!Gun || !Character->GetEquipment() || !Character->GetEquipment()->GetStats().bHasSpeargun || Character->IsDriving() || Character->IsInBed())
	{
		return;
	}
	const FSpearfishLineNetState& Line = Gun->GetLine();
	const FSpearfishDiverStats& Stats = Gun->GetStats();
	const float CenterX = Canvas->ClipX * 0.5f;
	const float Y = Canvas->ClipY - S(120.f);
	const float Width = S(420.f);

	switch (Line.State)
	{
	case ESpeargunState::Loaded:
		TextCentered(TEXT("Speargun loaded"), CenterX, Y + S(20.f), WithAlpha(Ink, 0.7f), SmallFont, 1.1f);
		break;
	case ESpeargunState::Reloading:
		TextCentered(TEXT("Loading the band..."), CenterX, Y, Ink, SmallFont, 1.1f);
		Bar(CenterX - Width * 0.25f, Y + S(22.f), Width * 0.5f, S(6.f), Gun->GetReloadFraction(), Ink, WithAlpha(Dim, 0.35f));
		break;
	case ESpeargunState::Flying:
		TextCentered(TEXT("..."), CenterX, Y + S(20.f), Ink, SmallFont, 1.1f);
		break;
	case ESpeargunState::Retrieving:
		TextCentered(TEXT("Reeling the shaft back in"), CenterX, Y + S(20.f), Ink, SmallFont, 1.1f);
		break;
	case ESpeargunState::Attached:
	{
		const float Break = FMath::Max(Stats.Line.BreakForce, 1.f);
		const float Tension = Line.Tension / Break;
		const FLinearColor TensionColor = Tension < 0.6f ? Good : (Tension < 0.9f ? Warn : Bad);
		FString Title;
		FString Hint;
		const ASpearfishFish* Fish = Gun->GetHookedFish();
		switch (Line.Target)
		{
		case ESpearfishLineTarget::Fish:
			if (Fish && Gun->HasExhaustedFishOnLine())
			{
				Title = TEXT("The fish is spent");
				Hint = TEXT("Reel it close and press [E] to bag it - or tow it to the boat ladder.");
			}
			else
			{
				Title = TEXT("FISH ON!");
				Hint = TEXT("[RMB] reel   [R] cut loose   let it run when the line screams");
			}
			break;
		case ESpearfishLineTarget::Character:
			Title = TEXT("Your partner is on the line");
			Hint = TEXT("[RMB] reel them in   [R] release");
			break;
		case ESpearfishLineTarget::Physics:
			Title = TEXT("Line on an object");
			Hint = TEXT("[RMB] drag it   [R] release");
			break;
		case ESpearfishLineTarget::World:
			Title = TEXT("Shaft stuck fast");
			Hint = TEXT("[RMB] pull yourself along   [R] retrieve");
			break;
		default:
			break;
		}
		TextCentered(Title, CenterX, Y - S(30.f), Line.Target == ESpearfishLineTarget::Fish ? Gold : Ink, MediumFont, 0.85f);
		Bar(CenterX - Width * 0.5f, Y, Width, S(12.f), Tension, TensionColor, WithAlpha(Dim, 0.35f));
		if (Line.Stress > 0.01f)
		{
			Bar(CenterX - Width * 0.5f, Y + S(14.f), Width, S(4.f), Line.Stress, Bad, WithAlpha(Dim, 0.2f));
		}
		const float MaxLength = FMath::Max(Stats.Line.MaxLengthCm, 1.f);
		TextCentered(FString::Printf(TEXT("line %.0f / %.0f m%s"), Line.LengthCm / 100.f, MaxLength / 100.f, Line.bReeling ? TEXT("   reeling") : TEXT("")),
			CenterX, Y + S(22.f), WithAlpha(Ink, 0.85f), SmallFont, 1.05f);
		TextCentered(Hint, CenterX, Y + S(44.f), WithAlpha(Ink, 0.7f), SmallFont, 1.f);
		break;
	}
	default:
		break;
	}
}

void ASpearfishHUD::DrawCrosshairAndPrompt()
{
	using namespace SpearfishHUDPrivate;
	const ASpearfishCharacter* Character = GetSpearfishCharacter();
	if (!Character || Character->IsInBed() || Character->IsBlackedOut())
	{
		return;
	}
	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;
	if (!Character->IsDriving())
	{
		Box(CenterX - S(2.f), CenterY - S(2.f), S(4.f), S(4.f), WithAlpha(Ink, 0.8f));
	}
	const USpearfishInteractionComponent* Interaction = Character->GetInteraction();
	const FText Prompt = Interaction ? Interaction->GetFocusedPrompt() : FText::GetEmpty();
	if (!Prompt.IsEmpty())
	{
		TextCentered(FString::Printf(TEXT("[E]  %s"), *Prompt.ToString()), CenterX, CenterY + S(28.f), Gold, MediumFont, 0.8f);
	}
}

void ASpearfishHUD::DrawFeed()
{
	using namespace SpearfishHUDPrivate;
	const ASpearfishPlayerController* Controller = GetSpearfishController();
	if (!Controller)
	{
		return;
	}
	const TArray<FSpearfishFeedEntry>& Feed = Controller->GetFeed();
	const float X = S(30.f);
	float Y = Canvas->ClipY * 0.36f;
	int32 Shown = 0;
	for (int32 Index = Feed.Num() - 1; Index >= 0 && Shown < 7; --Index)
	{
		const FSpearfishFeedEntry& Entry = Feed[Index];
		const float Age = Time - Entry.Time;
		const float Lifetime = Entry.bRadio ? 18.f : 9.f;
		if (Age > Lifetime)
		{
			continue;
		}
		const float Alpha = FMath::Clamp((Lifetime - Age) / 1.5f, 0.f, 1.f);
		Text(Entry.Text.ToString(), X, Y, WithAlpha(Entry.Color, Alpha), SmallFont, Entry.bRadio ? 1.25f : 1.15f);
		Y += S(Entry.bRadio ? 26.f : 23.f);
		++Shown;
	}
}

void ASpearfishHUD::DrawMarkers()
{
	using namespace SpearfishHUDPrivate;
	const ASpearfishCharacter* Character = GetSpearfishCharacter();
	const ASpearfishGameState* GameState = GetSpearfishGameState();
	if (!Character || !GameState)
	{
		return;
	}
	// The boat is visible from the surface, so it is marked there; underwater you navigate by sight.
	if (const ASpearfishBoat* Boat = GameState->GetBoat())
	{
		const float Distance = static_cast<float>(FVector::Dist(Boat->GetActorLocation(), Character->GetActorLocation())) / 100.f;
		if (!Character->IsHeadUnderwater() && Distance > 15.f && !Character->IsDriving())
		{
			Marker(Boat->GetActorLocation() + FVector(0.f, 0.f, 600.f), FString::Printf(TEXT("Boat %.0f m"), Distance), Gold);
		}
	}
	// Chef with the locator upgrade sees where the diver is.
	const ASpearfishPlayerState* State = PlayerOwner ? PlayerOwner->GetPlayerState<ASpearfishPlayerState>() : nullptr;
	const ASpearfishCharacter* Diver = GameState->GetDiverCharacter();
	if (State && State->IsChef() && Diver && Diver != Character && GameState->GetProgression() && GameState->GetProgression()->HasDiverLocator())
	{
		const float Distance = static_cast<float>(FVector::Dist(Diver->GetActorLocation(), Character->GetActorLocation())) / 100.f;
		Marker(Diver->GetActorLocation(), FString::Printf(TEXT("Diver %.0f m  (%.0f m deep)"), Distance, FMath::Max(0.f, Diver->GetDepthMeters())), Water);
	}
}

void ASpearfishHUD::DrawKitchenBoard()
{
	using namespace SpearfishHUDPrivate;
	const ASpearfishGameState* GameState = GetSpearfishGameState();
	const ASpearfishPlayerState* State = PlayerOwner ? PlayerOwner->GetPlayerState<ASpearfishPlayerState>() : nullptr;
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	// Only when a machine runs the kitchen: in co-op the diver learns what is needed from the chef.
	if (!GameState || !State || !Registry || !GameState->IsAutoChefActive() || State->IsChef() || !GameState->GetBoat())
	{
		return;
	}
	const USpearfishRestaurantComponent* Restaurant = GameState->GetBoat()->GetRestaurant();
	if (!Restaurant || !Restaurant->IsOpen())
	{
		return;
	}
	const float Right = Canvas->ClipX - S(24.f);
	float Y = S(110.f);
	TextRight(TEXT("KITCHEN BOARD"), Right, Y, Gold, SmallFont, 1.2f);
	Y += S(24.f);
	int32 Waiting = 0;
	for (const FSpearfishOrder& Order : Restaurant->GetOrders())
	{
		Waiting += Order.State == ESpearfishOrderState::Waiting ? 1 : 0;
	}
	TextRight(FString::Printf(TEXT("%d order(s) waiting"), Waiting), Right, Y, Ink, SmallFont, 1.1f);
	Y += S(22.f);
	if (Restaurant->GetKitchenNeeds().Num() == 0)
	{
		TextRight(TEXT("Nothing missing - catch anything good"), Right, Y, Dim, SmallFont, 1.05f);
		return;
	}
	for (const FSpearfishKitchenNeed& Need : Restaurant->GetKitchenNeeds())
	{
		FString What = Need.SpeciesId.IsNone() ? FString::Printf(TEXT("any %s"), *Need.Category.ToString()) : Registry->GetDisplayName(Need.SpeciesId).ToString();
		if (Need.MinLengthCm > 0.f)
		{
			What += FString::Printf(TEXT(" (%s+)"), *SpearfishText::Length(Need.MinLengthCm));
		}
		TextRight(FString::Printf(TEXT("%d x %s"), Need.Count, *What), Right, Y, Ink, SmallFont, 1.1f);
		Y += S(22.f);
	}
}

void ASpearfishHUD::DrawContextHints()
{
	using namespace SpearfishHUDPrivate;
	const ASpearfishCharacter* Character = GetSpearfishCharacter();
	const ASpearfishGameState* GameState = GetSpearfishGameState();
	if (!Character || !GameState)
	{
		return;
	}
	const float Right = Canvas->ClipX - S(30.f);
	const float Y = Canvas->ClipY - S(70.f);
	if (const ASpearfishBoat* Boat = Character->GetDrivingBoat())
	{
		const float Knots = FMath::Abs(Boat->GetSpeed()) / 51.4f;
		TextRight(FString::Printf(TEXT("%.1f kn  %s"), Knots, Boat->IsAnchored() ? TEXT("ANCHORED") : TEXT("under way")), Right, Y - S(26.f), Gold, MediumFont, 0.8f);
		TextRight(TEXT("[W/S] throttle   [A/D] steer   [E] leave the helm   (raise the anchor first)"), Right, Y, Ink, SmallFont, 1.05f);
		return;
	}
	if (Character->IsInBed())
	{
		TextRight(GameState->IsSoloSession() ? TEXT("Sleeping...") : TEXT("In your bunk - the day ends when your partner sleeps too."), Right, Y - S(24.f), Ink, SmallFont, 1.15f);
		TextRight(TEXT("[E] get up"), Right, Y, Dim, SmallFont, 1.05f);
		return;
	}
	const USpearfishDayCycleComponent* DayCycle = GameState->GetDayCycle();
	if (DayCycle && DayCycle->GetPhase() == ESpearfishDayPhase::Night)
	{
		TextRight(TEXT("It's late. Find your bunk in the cabin."), Right, Y, Warn, SmallFont, 1.15f);
	}
}

// ------------------------------------------------------------------------------------- Fades

void ASpearfishHUD::DrawFadeAndBlackout()
{
	using namespace SpearfishHUDPrivate;
	const ASpearfishPlayerController* Controller = GetSpearfishController();
	const ASpearfishCharacter* Character = GetSpearfishCharacter();
	if (Character && Character->IsBlackedOut())
	{
		Box(0.f, 0.f, Canvas->ClipX, Canvas->ClipY, FLinearColor(0.f, 0.f, 0.02f, 0.92f));
		TextCentered(TEXT("You blacked out..."), Canvas->ClipX * 0.5f, Canvas->ClipY * 0.45f, Bad, LargeFont, 0.9f);
		TextCentered(TEXT("Your bag slipped away into the blue."), Canvas->ClipX * 0.5f, Canvas->ClipY * 0.45f + S(44.f), Ink, SmallFont, 1.2f);
	}
	if (Controller && Controller->GetFadeAmount() > 0.001f)
	{
		Box(0.f, 0.f, Canvas->ClipX, Canvas->ClipY, FLinearColor(0.f, 0.f, 0.f, Controller->GetFadeAmount()));
	}
}

// ------------------------------------------------------------------------------------ Panels

void ASpearfishHUD::DrawPanel()
{
	using namespace SpearfishHUDPrivate;
	const ASpearfishPlayerController* Controller = GetSpearfishController();
	const ESpearfishUIPanel Panel = Controller->GetOpenPanel();
	if (Panel == ESpearfishUIPanel::Minigame)
	{
		DrawMinigame();
		return;
	}

	const float W = FMath::Min(S(1180.f), Canvas->ClipX - S(40.f));
	const float H = FMath::Min(S(760.f), Canvas->ClipY - S(40.f));
	const float X = (Canvas->ClipX - W) * 0.5f;
	const float Y = (Canvas->ClipY - H) * 0.5f;
	const float Pad = S(28.f);
	Box(X, Y, W, H, PanelColor);
	Frame(X, Y, W, H, PanelEdge, 2.f);
	Text(SpearfishUI::PanelTitle(Panel).ToString(), X + Pad, Y + S(18.f), Gold, LargeFont, 0.85f);
	if (const ASpearfishGameState* GameState = GetSpearfishGameState())
	{
		if (GameState->GetProgression())
		{
			TextRight(SpearfishText::Money(GameState->GetProgression()->GetMoney()), X + W - Pad, Y + S(22.f), Gold, MediumFont, 0.85f);
		}
	}

	if (Panel == ESpearfishUIPanel::DaySummary)
	{
		DrawSummary(X + Pad, Y + S(80.f), W - Pad * 2.f);
		TextCentered(TEXT("[E] continue"), X + W * 0.5f, Y + H - S(40.f), Dim, SmallFont, 1.15f);
		return;
	}
	if (Panel == ESpearfishUIPanel::Help)
	{
		DrawHelp(X + Pad, Y + S(80.f), W - Pad * 2.f);
		TextCentered(TEXT("[E] / [Esc] close"), X + W * 0.5f, Y + H - S(40.f), Dim, SmallFont, 1.15f);
		return;
	}

	float ContentY = Y + S(74.f);
	const TArray<FText> Tabs = Controller->GetCurrentTabs();
	if (Tabs.Num() > 1)
	{
		float TabX = X + Pad;
		for (int32 Index = 0; Index < Tabs.Num(); ++Index)
		{
			const FString Label = Tabs[Index].ToString();
			const FVector2D Size = Measure(Label, MediumFont, 0.75f);
			const bool bSelected = Index == Controller->GetUITab();
			if (bSelected)
			{
				Box(TabX - S(10.f), ContentY - S(4.f), static_cast<float>(Size.X) + S(20.f), static_cast<float>(Size.Y) + S(8.f), WithAlpha(PanelEdge, 0.35f));
			}
			Text(Label, TabX, ContentY, bSelected ? Ink : Dim, MediumFont, 0.75f);
			TabX += static_cast<float>(Size.X) + S(36.f);
		}
		ContentY += S(44.f);
	}
	DrawRows(Controller->BuildCurrentRows(), X + Pad, ContentY, W - Pad * 2.f, Y + H - S(64.f) - ContentY);

	const FString Footer = Tabs.Num() > 1 ? TEXT("[W/S] select   [A/D] tab   [E] confirm   [Esc] close") : TEXT("[W/S] select   [E] confirm   [Esc] close");
	TextCentered(Footer, X + W * 0.5f, Y + H - S(40.f), Dim, SmallFont, 1.15f);
}

void ASpearfishHUD::DrawRows(const TArray<FSpearfishUIRow>& Rows, float X, float Y, float Width, float Height)
{
	using namespace SpearfishHUDPrivate;
	const ASpearfishPlayerController* Controller = GetSpearfishController();
	const int32 Selected = Controller->GetUISelection();

	TArray<float> Heights;
	for (const FSpearfishUIRow& Row : Rows)
	{
		Heights.Add(S(Row.bHeader ? 36.f : 30.f) + (Row.Detail.IsEmpty() ? 0.f : S(22.f)) + S(6.f));
	}
	// Scroll so the selection stays visible.
	int32 First = 0;
	if (Rows.IsValidIndex(Selected))
	{
		float Span = 0.f;
		for (int32 Index = Selected; Index >= 0; --Index)
		{
			Span += Heights[Index];
			if (Span > Height)
			{
				First = Index + 1;
				break;
			}
		}
	}

	float RowY = Y;
	for (int32 Index = First; Index < Rows.Num(); ++Index)
	{
		if (RowY + Heights[Index] > Y + Height + 0.5f)
		{
			TextCentered(TEXT("..."), X + Width * 0.5f, RowY, Dim, SmallFont, 1.f);
			break;
		}
		const FSpearfishUIRow& Row = Rows[Index];
		const bool bIsSelected = Index == Selected;
		if (Row.bHeader)
		{
			Text(Row.Label.ToString(), X, RowY + S(8.f), Row.Color, MediumFont, 0.72f);
			Box(X, RowY + S(32.f), Width, S(1.f), WithAlpha(PanelEdge, 0.5f));
		}
		else
		{
			if (bIsSelected)
			{
				Box(X - S(10.f), RowY, Width + S(20.f), Heights[Index] - S(4.f), WithAlpha(PanelEdge, 0.22f));
				Box(X - S(10.f), RowY, S(4.f), Heights[Index] - S(4.f), Gold);
			}
			const float Alpha = Row.bEnabled ? 1.f : 0.75f;
			Text(Row.Label.ToString(), X, RowY + S(4.f), WithAlpha(Row.Color, Alpha), MediumFont, 0.7f);
			FString Value = Row.Value.ToString();
			FLinearColor ValueColor = Row.ValueColor;
			if (bIsSelected && Controller->GetPendingConfirm() == Index)
			{
				Value = TEXT("press again to confirm");
				ValueColor = Bad;
			}
			TextRight(Value, X + Width, RowY + S(6.f), ValueColor, SmallFont, 1.15f);
			if (!Row.Detail.IsEmpty())
			{
				Text(Row.Detail.ToString(), X + S(16.f), RowY + S(31.f), WithAlpha(Dim, 1.f), SmallFont, 1.05f);
			}
		}
		RowY += Heights[Index];
	}
	if (Rows.Num() == 0)
	{
		Text(TEXT("Nothing here yet."), X, Y, Dim, MediumFont, 0.7f);
	}
}

void ASpearfishHUD::DrawSummary(float X, float Y, float Width)
{
	using namespace SpearfishHUDPrivate;
	const FSpearfishDaySummary& Summary = GetSpearfishController()->GetLastSummary();
	const FSpearfishDayStats& Stats = Summary.Stats;
	Text(FString::Printf(TEXT("Day %d is over"), Stats.Day), X, Y, Ink, LargeFont, 0.8f);
	Y += S(56.f);

	auto Line = [this, X, Width, &Y](const FString& Label, const FString& Value, const FLinearColor& Color)
	{
		Text(Label, X, Y, SpearfishHUDPrivate::Ink, MediumFont, 0.72f);
		TextRight(Value, X + Width * 0.6f, Y, Color, MediumFont, 0.72f);
		Y += S(32.f);
	};
	Line(TEXT("Dishes served"), FString::FromInt(Stats.DishesServed), Ink);
	Line(TEXT("Restaurant takings"), SpearfishText::Money(Stats.Revenue), Gold);
	Line(TEXT("Tips"), SpearfishText::Money(Stats.Tips), Gold);
	Line(TEXT("Guests who walked out"), FString::FromInt(Stats.GuestsLost), Stats.GuestsLost > 0 ? Warn : Ink);
	Line(TEXT("Fish caught"), FString::FromInt(Stats.FishCaught), Ink);
	Line(TEXT("Finds sold"), SpearfishText::Money(Stats.LootValue), Gold);
	Line(TEXT("New species in the journal"), FString::FromInt(Stats.NewSpecies), Stats.NewSpecies > 0 ? Good : Ink);
	Line(TEXT("Expenses (rescues, fees)"), SpearfishText::Money(Stats.Expenses), Stats.Expenses > 0 ? Bad : Ink);
	const float RepDelta = Stats.ReputationEnd - Stats.ReputationStart;
	Line(TEXT("Reputation"), FString::Printf(TEXT("%.1f  (%s%.2f)"), Stats.ReputationEnd, RepDelta >= 0.f ? TEXT("+") : TEXT(""), RepDelta), RepDelta >= 0.f ? Good : Bad);
	if (Stats.BestDishQuality > 0.f)
	{
		Line(TEXT("Best dish"), SpearfishText::QualityName(Stats.BestDishQuality).ToString(), Good);
	}
	if (Summary.SpoiledFish > 0)
	{
		Line(TEXT("Fish spoiled overnight"), FString::FromInt(Summary.SpoiledFish), Warn);
	}
	if (Summary.bPassedOut)
	{
		Y += S(10.f);
		Text(TEXT("Somebody never made it to bed and passed out on deck."), X, Y, Warn, SmallFont, 1.2f);
	}
}

void ASpearfishHUD::DrawHelp(float X, float Y, float Width)
{
	using namespace SpearfishHUDPrivate;
	const ASpearfishPlayerState* State = PlayerOwner ? PlayerOwner->GetPlayerState<ASpearfishPlayerState>() : nullptr;
	const ASpearfishGameState* GameState = GetSpearfishGameState();
	const ESpearfishRole PlayerRole = State ? State->GetRole() : ESpearfishRole::Diver;
	Text(FString::Printf(TEXT("You are the %s: %s"), *SpearfishText::RoleName(PlayerRole).ToString(),
		*SpearfishText::RoleDuty(PlayerRole, GameState && GameState->IsSoloSession()).ToString()), X, Y, SpearfishText::RoleColor(PlayerRole), SmallFont, 1.2f);
	Y += S(40.f);

	struct FControl
	{
		const TCHAR* Action;
		const TCHAR* Keys;
	};
	static const FControl Controls[] = {
		{ TEXT("Move / swim"), TEXT("W A S D (swim where you look)") },
		{ TEXT("Up / down"), TEXT("Space / C or Ctrl") },
		{ TEXT("Sprint (fins)"), TEXT("Shift - burns air faster") },
		{ TEXT("Shoot"), TEXT("Left mouse") },
		{ TEXT("Reel / pull"), TEXT("Hold right mouse") },
		{ TEXT("Release line"), TEXT("R") },
		{ TEXT("Use / bag fish / climb"), TEXT("E") },
		{ TEXT("Dive light"), TEXT("L") },
		{ TEXT("Tablet (chef anywhere, diver on deck)"), TEXT("Tab") },
		{ TEXT("Journal"), TEXT("J") },
		{ TEXT("Quick radio messages"), TEXT("1 - 6") },
		{ TEXT("Voice (push to talk)"), TEXT("V") },
		{ TEXT("Menu"), TEXT("Esc") },
	};
	for (const FControl& Control : Controls)
	{
		Text(Control.Action, X, Y, Ink, SmallFont, 1.15f);
		Text(Control.Keys, X + Width * 0.45f, Y, Gold, SmallFont, 1.15f);
		Y += S(25.f);
	}
	Y += S(14.f);
	const TArray<FString> Tips = PlayerRole == ESpearfishRole::Chef
		? TArray<FString>{ TEXT("Orders arrive while the boat is anchored and the sign says OPEN."), TEXT("Start a dish on the tablet, then cook each step at its station."),
			TEXT("The diver can't see your orders - radio what you need (1 = need fish)."), TEXT("Serve ready dishes at the pass before the guests lose patience.") }
		: TArray<FString>{ TEXT("Watch the air gauge: surface before it runs dry or you'll black out and lose the bag."),
			TEXT("Approach slowly - fast swimmers spook fish. Big fish fight: let the line run, then reel."),
			TEXT("Climb the ladder to unload your bag into the cooler and refill your tank."), TEXT("Bunks open in the evening. Roles swap every night in co-op.") };
	for (const FString& Tip : Tips)
	{
		Text(FString::Printf(TEXT("- %s"), *Tip), X, Y, Dim, SmallFont, 1.1f);
		Y += S(24.f);
	}
}

// ---------------------------------------------------------------------------------- Minigame

void ASpearfishHUD::DrawMinigame()
{
	using namespace SpearfishHUDPrivate;
	const ASpearfishPlayerController* Controller = GetSpearfishController();
	const FSpearfishMinigameState& Game = Controller->GetMinigame();
	const float W = FMath::Min(S(960.f), Canvas->ClipX - S(40.f));
	const float H = S(420.f);
	const float X = (Canvas->ClipX - W) * 0.5f;
	const float Y = Canvas->ClipY * 0.5f - H * 0.5f;
	Box(X, Y, W, H, PanelColor);
	Frame(X, Y, W, H, PanelEdge, 2.f);

	Text(SpearfishText::StepName(Game.Step).ToString(), X + S(28.f), Y + S(18.f), Gold, LargeFont, 0.85f);
	const float TimeFraction = Game.TimeLimit > 0.f ? 1.f - Game.Elapsed / Game.TimeLimit : 0.f;
	Bar(X + W - S(280.f), Y + S(30.f), S(250.f), S(8.f), TimeFraction, TimeFraction > 0.3f ? Ink : Bad, WithAlpha(Dim, 0.3f));

	if (Game.bFinished)
	{
		TextCentered(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Game.Score * 100.f)), X + W * 0.5f, Y + S(130.f), Game.Score >= 0.7f ? Good : (Game.Score >= 0.4f ? Warn : Bad), LargeFont, 1.6f);
		TextCentered(SpearfishText::QualityName(Game.Score).ToString(), X + W * 0.5f, Y + S(220.f), Ink, MediumFont, 0.9f);
		TextCentered(TEXT("[E] continue"), X + W * 0.5f, Y + H - S(40.f), Dim, SmallFont, 1.15f);
		return;
	}
	DrawMinigameBody(Game, X + S(40.f), Y + S(90.f), W - S(80.f), H - S(170.f));
	if (Game.LastHitScore >= 0.f)
	{
		TextCentered(HitWord(Game.LastHitScore), X + W * 0.5f, Y + S(70.f), Game.LastHitScore >= 0.7f ? Good : (Game.LastHitScore >= 0.35f ? Warn : Bad), MediumFont, 0.85f);
	}
	TextCentered(StepHint(Game.Step), X + W * 0.5f, Y + H - S(64.f), Ink, SmallFont, 1.15f);
	TextCentered(TEXT("[Esc] give the dish back"), X + W * 0.5f, Y + H - S(36.f), Dim, SmallFont, 1.f);
}

void ASpearfishHUD::DrawMinigameBody(const FSpearfishMinigameState& Game, float X, float Y, float Width, float Height)
{
	using namespace SpearfishHUDPrivate;
	const float MidY = Y + Height * 0.5f;
	switch (Game.Step)
	{
	case ESpearfishCookStep::Fillet:
	{
		// The fish lies across the board; the knife sweeps along it.
		Box(X, MidY - S(30.f), Width, S(60.f), FLinearColor(0.75f, 0.55f, 0.45f, 1.f));
		for (int32 Index = 0; Index < Game.Targets.Num(); ++Index)
		{
			const float MarkX = X + Width * Game.Targets[Index];
			const bool bDone = Index < Game.Results.Num();
			const FLinearColor MarkColor = bDone ? (Game.Results[Index] >= 0.7f ? Good : (Game.Results[Index] > 0.f ? Warn : Bad)) : Ink;
			Box(MarkX - S(2.f), MidY - S(40.f), S(4.f), S(80.f), MarkColor);
		}
		const float KnifeX = X + Width * FMath::Clamp(Game.Cursor, 0.f, 1.f);
		Box(KnifeX - S(3.f), MidY - S(70.f), S(6.f), S(140.f), FLinearColor(0.85f, 0.9f, 0.95f, 1.f));
		Box(KnifeX - S(8.f), MidY - S(90.f), S(16.f), S(26.f), FLinearColor(0.25f, 0.15f, 0.1f, 1.f));
		break;
	}
	case ESpearfishCookStep::Chop:
	{
		// Beats scroll toward the hit line on the left.
		const float HitX = X + Width * 0.15f;
		const float PixelsPerSecond = Width * 0.28f;
		Box(X, MidY - S(2.f), Width, S(4.f), WithAlpha(Dim, 0.5f));
		Box(HitX - S(3.f), MidY - S(50.f), S(6.f), S(100.f), Gold);
		for (int32 Index = 0; Index < Game.Targets.Num(); ++Index)
		{
			const float BeatX = HitX + (Game.Targets[Index] - Game.Cursor) * PixelsPerSecond;
			if (BeatX < X - S(20.f) || BeatX > X + Width)
			{
				continue;
			}
			const bool bDone = Index < Game.Results.Num();
			const FLinearColor BeatColor = bDone ? (Game.Results[Index] >= 0.7f ? Good : (Game.Results[Index] > 0.f ? Warn : Bad)) : Ink;
			Box(BeatX - S(12.f), MidY - S(12.f), S(24.f), S(24.f), BeatColor);
		}
		break;
	}
	case ESpearfishCookStep::Season:
	{
		Box(X, MidY - S(16.f), Width, S(32.f), WithAlpha(Dim, 0.35f));
		Box(X + Width * Game.ZoneMin, MidY - S(16.f), Width * (Game.ZoneMax - Game.ZoneMin), S(32.f), WithAlpha(Good, 0.75f));
		const float NeedleX = X + Width * FMath::Clamp(Game.Cursor, 0.f, 1.f);
		Box(NeedleX - S(3.f), MidY - S(40.f), S(6.f), S(80.f), Ink);
		Text(FString::Printf(TEXT("Sprinkles %d / %d"), Game.Progress, SpearfishMinigame::SeasonSprinkles), X, MidY + S(50.f), Ink, SmallFont, 1.15f);
		break;
	}
	case ESpearfishCookStep::Grill:
	{
		// Colour runs raw -> golden -> burnt.
		const int32 Segments = 24;
		for (int32 Index = 0; Index < Segments; ++Index)
		{
			const float T = static_cast<float>(Index) / Segments;
			const FLinearColor Raw(0.9f, 0.55f, 0.55f);
			const FLinearColor Golden(0.95f, 0.7f, 0.25f);
			const FLinearColor Burnt(0.2f, 0.12f, 0.08f);
			const FLinearColor Color = T < 0.5f ? FMath::Lerp(Raw, Golden, T * 2.f) : FMath::Lerp(Golden, Burnt, (T - 0.5f) * 2.f);
			Box(X + Width * T, MidY - S(18.f), Width / Segments + 1.f, S(36.f), Color);
		}
		Frame(X + Width * Game.ZoneMin, MidY - S(24.f), Width * (Game.ZoneMax - Game.ZoneMin), S(48.f), Good, 2.f);
		const float CursorX = X + Width * FMath::Clamp(Game.Cursor, 0.f, 1.f);
		Box(CursorX - S(3.f), MidY - S(40.f), S(6.f), S(80.f), Ink);
		Text(FString::Printf(TEXT("Sides done %d / %d"), Game.Progress, SpearfishMinigame::GrillSides), X, MidY + S(50.f), Ink, SmallFont, 1.15f);
		break;
	}
	case ESpearfishCookStep::Fry:
	case ESpearfishCookStep::Simmer:
	{
		// Vertical thermometer with the target band.
		const float BarX = X + Width * 0.5f - S(30.f);
		const float BarH = Height;
		Box(BarX, Y, S(60.f), BarH, WithAlpha(Dim, 0.35f));
		Box(BarX, Y + BarH * (1.f - Game.ZoneMax), S(60.f), BarH * (Game.ZoneMax - Game.ZoneMin), WithAlpha(Good, 0.6f));
		const float Level = FMath::Clamp(Game.Cursor, 0.f, 1.f);
		const bool bInBand = Level >= Game.ZoneMin && Level <= Game.ZoneMax;
		Box(BarX + S(10.f), Y + BarH * (1.f - Level), S(40.f), BarH * Level, bInBand ? Gold : Bad);
		Text(Game.bHolding ? TEXT("HEATING") : TEXT("cooling"), BarX + S(90.f), MidY - S(12.f), Game.bHolding ? Warn : Water, MediumFont, 0.75f);
		const float Held = Game.TimeLimit > 0.f ? Game.Accumulated / Game.TimeLimit : 0.f;
		Text(FString::Printf(TEXT("In the band: %d%%"), FMath::RoundToInt(Held * 100.f)), BarX + S(90.f), MidY + S(20.f), Ink, SmallFont, 1.15f);
		break;
	}
	case ESpearfishCookStep::Plate:
	{
		const int32 Count = Game.Sequence.Num();
		const float Cell = FMath::Min(S(70.f), Width / FMath::Max(Count, 1));
		const float StartX = X + (Width - Cell * Count) * 0.5f;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const bool bDone = Index < Game.Progress;
			const bool bCurrent = Index == Game.Progress;
			const float CellX = StartX + Cell * Index;
			Box(CellX + S(4.f), MidY - Cell * 0.5f, Cell - S(8.f), Cell, bDone ? WithAlpha(Good, 0.6f) : (bCurrent ? WithAlpha(Gold, 0.45f) : WithAlpha(Dim, 0.3f)));
			TextCentered(ArrowGlyph(Game.Sequence[Index]), CellX + Cell * 0.5f, MidY - S(16.f), Ink, LargeFont, 0.9f);
		}
		if (Game.Mistakes > 0)
		{
			TextCentered(FString::Printf(TEXT("Slips: %d"), Game.Mistakes), X + Width * 0.5f, MidY + Cell * 0.5f + S(16.f), Bad, SmallFont, 1.15f);
		}
		break;
	}
	default:
		break;
	}
}

#undef LOCTEXT_NAMESPACE
