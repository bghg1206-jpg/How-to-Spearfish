#include "UI/SpearfishUIModel.h"

#include "Algo/Sort.h"
#include "Boat/SpearfishBoat.h"
#include "Core/SpearfishGameState.h"
#include "Core/SpearfishPlayerState.h"
#include "Data/SpearfishDataRegistry.h"
#include "Diving/SpearfishCharacter.h"
#include "Diving/SpearfishOxygenComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/SpearfishCatchStorageComponent.h"
#include "Inventory/SpearfishDiveBagComponent.h"
#include "Progression/SpearfishJournalComponent.h"
#include "Progression/SpearfishProgressionComponent.h"
#include "Restaurant/SpearfishRestaurantComponent.h"

#define LOCTEXT_NAMESPACE "SpearfishUI"

namespace SpearfishUI
{
	namespace
	{
		const FLinearColor Header(1.f, 0.85f, 0.45f);
		const FLinearColor Muted(0.6f, 0.62f, 0.66f);
		const FLinearColor Good(0.45f, 1.f, 0.55f);
		const FLinearColor Warn(1.f, 0.75f, 0.3f);
		const FLinearColor Bad(1.f, 0.4f, 0.35f);

		struct FContext
		{
			const APlayerController* Controller = nullptr;
			const ASpearfishGameState* GameState = nullptr;
			const ASpearfishPlayerState* PlayerState = nullptr;
			const USpearfishDataRegistry* Registry = nullptr;
			const ASpearfishBoat* Boat = nullptr;
			const USpearfishRestaurantComponent* Restaurant = nullptr;
			const USpearfishProgressionComponent* Progression = nullptr;
			const USpearfishJournalComponent* Journal = nullptr;

			bool IsValid() const { return GameState && Registry && Progression; }
			ESpearfishRole Role() const { return PlayerState ? PlayerState->GetRole() : ESpearfishRole::None; }
		};

		FContext MakeContext(const APlayerController* Controller)
		{
			FContext Context;
			Context.Controller = Controller;
			UWorld* World = Controller ? Controller->GetWorld() : nullptr;
			Context.GameState = World ? World->GetGameState<ASpearfishGameState>() : nullptr;
			Context.PlayerState = Controller ? Controller->GetPlayerState<ASpearfishPlayerState>() : nullptr;
			Context.Registry = USpearfishDataRegistry::Get(Controller);
			if (Context.GameState)
			{
				Context.Boat = Context.GameState->GetBoat();
				Context.Restaurant = Context.Boat ? Context.Boat->GetRestaurant() : nullptr;
				Context.Progression = Context.GameState->GetProgression();
				Context.Journal = Context.GameState->GetJournal();
			}
			return Context;
		}

		FSpearfishUIRow MakeHeader(const FText& Text)
		{
			FSpearfishUIRow Row;
			Row.Label = Text;
			Row.Color = Header;
			Row.bHeader = true;
			return Row;
		}

		FSpearfishUIRow MakeInfo(const FText& Label, const FText& Value = FText::GetEmpty(), const FLinearColor& Color = FLinearColor::White)
		{
			FSpearfishUIRow Row;
			Row.Label = Label;
			Row.Value = Value;
			Row.Color = Color;
			return Row;
		}

		FText Name(const FContext& Context, FName Id)
		{
			return Id.IsNone() ? FText::GetEmpty() : Context.Registry->GetDisplayName(Id);
		}

		FText DescribeIngredient(const FContext& Context, const FSpearfishIngredientReq& Req, int32 Missing)
		{
			const FText What = Req.SpeciesId.IsNone() ? FText::Format(LOCTEXT("AnyCategory", "any {0}"), FText::FromName(Req.Category)) : Name(Context, Req.SpeciesId);
			FText Text = FText::Format(LOCTEXT("IngredientFmt", "{0}x {1}"), FText::AsNumber(Req.Count), What);
			if (Req.MinLengthCm > 0.f)
			{
				Text = FText::Format(LOCTEXT("IngredientSized", "{0} ({1}+)"), Text, FText::FromString(SpearfishText::Length(Req.MinLengthCm)));
			}
			if (Missing > 0)
			{
				Text = FText::Format(LOCTEXT("IngredientMissing", "{0} - missing {1}"), Text, FText::AsNumber(Missing));
			}
			return Text;
		}

		FText FormatSeconds(float Seconds)
		{
			const int32 Total = FMath::Max(0, FMath::CeilToInt(Seconds));
			return FText::FromString(FString::Printf(TEXT("%d:%02d"), Total / 60, Total % 60));
		}

		FText DescribeFish(const FContext& Context, const FSpearfishItem& Item)
		{
			if (Item.Kind != ESpearfishItemKind::Fish)
			{
				return Name(Context, Item.DefId);
			}
			return FText::Format(LOCTEXT("FishItemFmt", "{0}  {1}  ({2})"), Name(Context, Item.DefId), FText::FromString(SpearfishText::Length(Item.LengthCm)),
				SpearfishText::QualityName(Item.Quality));
		}

		// --------------------------------------------------------------------------------- Tablet

		void BuildOrders(const FContext& Context, TArray<FSpearfishUIRow>& Rows)
		{
			if (!Context.Restaurant)
			{
				Rows.Add(MakeInfo(LOCTEXT("NoRestaurant", "No restaurant connection.")));
				return;
			}
			Rows.Add(MakeHeader(Context.Restaurant->IsOpen() ? LOCTEXT("OrdersOpen", "Orders - restaurant OPEN") : LOCTEXT("OrdersClosed", "Orders - restaurant closed")));
			int32 Shown = 0;
			for (const FSpearfishOrder& Order : Context.Restaurant->GetOrders())
			{
				if (Order.State == ESpearfishOrderState::Served || Order.State == ESpearfishOrderState::Expired)
				{
					continue;
				}
				const FSpearfishRecipeDef* Recipe = Context.Registry->FindRecipe(Order.RecipeId);
				if (!Recipe)
				{
					continue;
				}
				++Shown;
				FSpearfishUIRow Row;
				Row.Label = FText::Format(LOCTEXT("OrderLabel", "Table {0}: {1}"), FText::AsNumber(Order.SeatIndex + 1), Recipe->DisplayName);
				Row.IntId = Order.OrderId;
				const float Patience = Context.Restaurant->GetPatienceFraction(Order);
				Row.Value = FormatSeconds(Context.Restaurant->GetPatienceRemaining(Order));
				Row.ValueColor = Patience > 0.5f ? Good : (Patience > 0.25f ? Warn : Bad);

				if (Order.State == ESpearfishOrderState::Waiting)
				{
					const TArray<int32> Missing = Context.Restaurant->GetMissingCounts(Order);
					TArray<FString> Parts;
					for (int32 Index = 0; Index < Recipe->Ingredients.Num(); ++Index)
					{
						Parts.Add(DescribeIngredient(Context, Recipe->Ingredients[Index], Missing.IsValidIndex(Index) ? Missing[Index] : 0).ToString());
					}
					Row.Detail = FText::FromString(FString::Join(Parts, TEXT(", ")));
					Row.bEnabled = Context.Restaurant->CanStartOrder(Order);
					Row.Action = ESpearfishUIAction::StartDish;
					Row.Color = Row.bEnabled ? FLinearColor::White : Muted;
					if (Row.bEnabled)
					{
						Row.Detail = FText::Format(LOCTEXT("OrderReady", "{0}  -  press to start cooking"), Row.Detail);
					}
				}
				else if (Order.State == ESpearfishOrderState::Cooking)
				{
					const FSpearfishDish* Dish = Context.Restaurant->FindDish(Order.DishId);
					if (Dish && Recipe->Steps.IsValidIndex(Dish->CurrentStep))
					{
						const ESpearfishCookStep Step = Recipe->Steps[Dish->CurrentStep];
						Row.Detail = FText::Format(LOCTEXT("OrderCooking", "Cooking - next: {0} at the {1}{2}"), SpearfishText::StepName(Step),
							SpearfishText::StationName(SpearfishStations::StationForStep(Step)), Dish->bAutoChef ? LOCTEXT("ByAuto", " (auto-chef)") : FText::GetEmpty());
					}
					Row.Color = FLinearColor(0.7f, 0.85f, 1.f);
				}
				else
				{
					Row.Detail = LOCTEXT("OrderServe", "Ready - take it to the pass and serve!");
					Row.Color = Good;
				}
				Rows.Add(Row);
			}
			if (Shown == 0)
			{
				Rows.Add(MakeInfo(Context.Restaurant->IsOpen() ? LOCTEXT("NoOrders", "No orders yet. Guests arrive while anchored.")
					: LOCTEXT("NoOrdersClosed", "Open the restaurant (sign on deck or Restaurant tab) while anchored."), FText::GetEmpty(), Muted));
			}
		}

		void BuildKitchen(const FContext& Context, TArray<FSpearfishUIRow>& Rows)
		{
			if (!Context.Restaurant)
			{
				return;
			}
			Rows.Add(MakeHeader(LOCTEXT("KitchenHeader", "Dishes in progress")));
			for (const FSpearfishDish& Dish : Context.Restaurant->GetDishes())
			{
				const FSpearfishRecipeDef* Recipe = Context.Registry->FindRecipe(Dish.RecipeId);
				if (!Recipe)
				{
					continue;
				}
				FSpearfishUIRow Row;
				Row.Label = FText::Format(LOCTEXT("DishLabel", "#{0} {1}"), FText::AsNumber(Dish.DishId), Recipe->DisplayName);
				TArray<FString> Steps;
				for (int32 Index = 0; Index < Recipe->Steps.Num(); ++Index)
				{
					FString Step = SpearfishText::StepName(Recipe->Steps[Index]).ToString();
					if (Index < Dish.CurrentStep)
					{
						Step += Dish.StepScores.IsValidIndex(Index) ? FString::Printf(TEXT(" %d%%"), FMath::RoundToInt(Dish.StepScores[Index] * 100.f)) : TEXT(" ok");
					}
					else if (Index == Dish.CurrentStep && Dish.State == ESpearfishDishState::Prep)
					{
						Step = TEXT("[") + Step + TEXT("]");
					}
					Steps.Add(Step);
				}
				Row.Detail = FText::FromString(FString::Join(Steps, TEXT(" > ")));
				if (Dish.State == ESpearfishDishState::Ready)
				{
					Row.Value = LOCTEXT("DishReady", "READY");
					Row.ValueColor = Good;
				}
				else if (Dish.bAutoChef)
				{
					Row.Value = LOCTEXT("DishAuto", "auto-chef");
				}
				else if (Dish.LockedBy)
				{
					Row.Value = FText::FromString(Dish.LockedBy->GetPlayerName());
				}
				Row.Action = ESpearfishUIAction::ScrapDish;
				Row.IntId = Dish.DishId;
				Row.bNeedsConfirm = true;
				Rows.Add(Row);
			}
			if (Rows.Num() == 1)
			{
				Rows.Add(MakeInfo(LOCTEXT("NoDishes", "Nothing cooking. Start a dish from the Orders tab."), FText::GetEmpty(), Muted));
			}
			else
			{
				Rows.Add(MakeInfo(LOCTEXT("ScrapHint", "Confirm twice on a dish to scrap it (fish are lost)."), FText::GetEmpty(), Muted));
			}
		}

		void BuildCooler(const FContext& Context, TArray<FSpearfishUIRow>& Rows)
		{
			const USpearfishCatchStorageComponent* Cooler = Context.Boat ? Context.Boat->GetCooler() : nullptr;
			if (!Cooler)
			{
				return;
			}
			Rows.Add(MakeHeader(FText::Format(LOCTEXT("CoolerHeader", "Cooler ({0} fish)"), FText::AsNumber(Cooler->GetItems().Num()))));
			TArray<FSpearfishItem> Items = Cooler->GetItems();
			Items.Sort([](const FSpearfishItem& A, const FSpearfishItem& B)
			{
				return A.DefId == B.DefId ? A.LengthCm > B.LengthCm : A.DefId.LexicalLess(B.DefId);
			});
			for (const FSpearfishItem& Item : Items)
			{
				FSpearfishUIRow Row = MakeInfo(DescribeFish(Context, Item), FText::FromString(SpearfishText::Money(Item.Value)));
				Row.Color = Item.Rarity >= ESpearfishRarity::Rare ? SpearfishText::RarityColor(Item.Rarity) : FLinearColor::White;
				Rows.Add(Row);
			}
			if (Items.Num() == 0)
			{
				Rows.Add(MakeInfo(LOCTEXT("CoolerEmpty", "Empty. The diver unloads the bag when climbing the ladder."), FText::GetEmpty(), Muted));
			}
		}

		void BuildDiver(const FContext& Context, TArray<FSpearfishUIRow>& Rows)
		{
			const ASpearfishCharacter* Diver = Context.GameState->GetDiverCharacter();
			Rows.Add(MakeHeader(LOCTEXT("DiverHeader", "Diver")));
			if (!Diver)
			{
				Rows.Add(MakeInfo(LOCTEXT("NoDiver", "No diver in the water.")));
				return;
			}
			Rows.Add(MakeInfo(FText::FromString(Diver->GetPlayerName()), Diver->IsSwimming() ? LOCTEXT("InWater", "in the water") : LOCTEXT("OnDeck", "on deck")));
			if (!Context.Progression->HasTelemetry())
			{
				Rows.Add(MakeInfo(LOCTEXT("NoTelemetry", "No telemetry link - you only know what the diver radios."), FText::GetEmpty(), Muted));
				Rows.Add(MakeInfo(LOCTEXT("TelemetryHint", "The dive shop sells a telemetry buoy upgrade."), FText::GetEmpty(), Muted));
				return;
			}
			const USpearfishOxygenComponent* Oxygen = Diver->GetOxygen();
			const float Air = Oxygen ? Oxygen->GetAirFraction() : 1.f;
			Rows.Add(MakeInfo(LOCTEXT("TelDepth", "Depth"), FText::FromString(FString::Printf(TEXT("%.0f m"), FMath::Max(0.f, Diver->GetDepthMeters())))));
			Rows.Add(MakeInfo(LOCTEXT("TelAir", "Air"), FText::AsPercent(Air), Air > 0.3f ? FLinearColor::White : Bad));
			const USpearfishDiveBagComponent* Bag = Diver->GetBag();
			if (Bag)
			{
				Rows.Add(MakeHeader(FText::Format(LOCTEXT("BagHeader", "Dive bag ({0}/{1} fish slots)"), FText::AsNumber(Bag->GetUsedFishSlots()),
					FText::AsNumber(Bag->GetBag().Capacity.FishSlots))));
				for (const FSpearfishItem& Item : Bag->GetItems())
				{
					Rows.Add(MakeInfo(DescribeFish(Context, Item), FText::FromString(SpearfishText::Money(Item.Value))));
				}
			}
		}

		void BuildComms(const FContext& Context, TArray<FSpearfishUIRow>& Rows)
		{
			Rows.Add(MakeHeader(LOCTEXT("CommsHeader", "Quick messages (also on keys 1-6)")));
			TArray<ESpearfishQuickMessage> Messages = SpearfishComms::HotkeysForRole(Context.Role());
			Messages.Add(ESpearfishQuickMessage::Yes);
			Messages.Add(ESpearfishQuickMessage::No);
			if (Context.Role() == ESpearfishRole::Chef)
			{
				Messages.Add(ESpearfishQuickMessage::MovingBoat);
			}
			for (const ESpearfishQuickMessage Type : Messages)
			{
				FSpearfishUIRow Row;
				Row.Action = ESpearfishUIAction::SendQuickMessage;
				Row.IntId = static_cast<int32>(Type);
				Row.Label = SpearfishText::QuickMessageLabel(Type);
				Row.Detail = SpearfishText::QuickMessage(Type, FText::GetEmpty(), 1, 0.f);
				if (Type == ESpearfishQuickMessage::NeedFish && Context.Restaurant && Context.Restaurant->GetKitchenNeeds().Num() > 0)
				{
					// One row per thing the kitchen is missing, so the chef can be specific.
					for (const FSpearfishKitchenNeed& Need : Context.Restaurant->GetKitchenNeeds())
					{
						FSpearfishUIRow NeedRow = Row;
						NeedRow.Id = Need.SpeciesId.IsNone() ? Need.Category : Need.SpeciesId;
						NeedRow.Count = Need.Count;
						NeedRow.Number = Need.MinLengthCm;
						const FText What = Need.SpeciesId.IsNone() ? FText::FromName(Need.Category) : Name(Context, Need.SpeciesId);
						NeedRow.Detail = SpearfishText::QuickMessage(Type, What, Need.Count, Need.MinLengthCm);
						Rows.Add(NeedRow);
					}
					continue;
				}
				Rows.Add(Row);
			}
			if (Context.GameState->GetRevealedRumors().Num() > 0)
			{
				Rows.Add(MakeHeader(LOCTEXT("RumorsHeader", "Rumors heard today")));
				for (const FName& EventId : Context.GameState->GetRevealedRumors())
				{
					if (const FSpearfishEventDef* Event = Context.Registry->FindEvent(EventId))
					{
						FSpearfishUIRow Row = MakeInfo(Event->DisplayName);
						Row.Detail = Event->Rumor;
						Row.Color = FLinearColor(0.85f, 0.75f, 1.f);
						Rows.Add(Row);
					}
				}
			}
		}

		void BuildRestaurant(const FContext& Context, TArray<FSpearfishUIRow>& Rows)
		{
			Rows.Add(MakeHeader(LOCTEXT("RestHeader", "The floating restaurant")));
			if (Context.Restaurant)
			{
				FSpearfishUIRow Toggle;
				Toggle.Action = ESpearfishUIAction::ToggleOpen;
				Toggle.Label = Context.Restaurant->IsOpen() ? LOCTEXT("CloseRest", "Flip the sign to CLOSED") : LOCTEXT("OpenRest", "Flip the sign to OPEN");
				Toggle.Detail = Context.Boat && !Context.Boat->IsAnchored() ? LOCTEXT("NeedAnchor", "Guests only come aboard while the boat is anchored.") : FText::GetEmpty();
				Toggle.Value = Context.Restaurant->IsOpen() ? LOCTEXT("IsOpen", "OPEN") : LOCTEXT("IsClosed", "CLOSED");
				Toggle.ValueColor = Context.Restaurant->IsOpen() ? Good : Warn;
				Rows.Add(Toggle);
				Rows.Add(MakeInfo(LOCTEXT("Seats", "Seats"), FText::AsNumber(Context.Restaurant->GetSeatCount())));
			}
			Rows.Add(MakeInfo(LOCTEXT("Reputation", "Reputation"), FText::FromString(FString::Printf(TEXT("%.1f stars"), Context.Progression->GetReputation()))));
			if (Context.GameState->IsAutoChefActive())
			{
				Rows.Add(MakeInfo(LOCTEXT("AutoChefOn", "The auto-chef is running the kitchen."), FText::GetEmpty(), Muted));
			}
			if (!Context.GameState->IsSoloSession() && Context.Role() == ESpearfishRole::Chef && !Context.GameState->FindPlayerWithRole(ESpearfishRole::Diver))
			{
				FSpearfishUIRow Switch;
				Switch.Action = ESpearfishUIAction::SwitchToDiver;
				Switch.Label = LOCTEXT("SwitchDiver", "Take over as diver (no diver connected)");
				Rows.Add(Switch);
			}
			if (Context.Progression->GetOwnedUpgrades().Num() > 0)
			{
				Rows.Add(MakeHeader(LOCTEXT("UpgradesHeader", "Upgrades")));
				for (const FName& Upgrade : Context.Progression->GetOwnedUpgrades())
				{
					Rows.Add(MakeInfo(Name(Context, Upgrade), FText::GetEmpty(), Muted));
				}
			}
		}

		// ---------------------------------------------------------------------------- Shop / gear

		FText RequirementValue(const FContext& Context, ESpearfishRequirementResult Result, const FSpearfishUnlockReq& Unlock)
		{
			switch (Result)
			{
			case ESpearfishRequirementResult::Ok:
			case ESpearfishRequirementResult::NotEnoughMoney:
				return FText::FromString(SpearfishText::Money(Unlock.Price));
			case ESpearfishRequirementResult::AlreadyOwned:
				return LOCTEXT("Owned", "owned");
			default:
				return SpearfishText::RequirementText(Result);
			}
		}

		void BuildShopGear(const FContext& Context, TArray<FSpearfishUIRow>& Rows)
		{
			for (int32 SlotIndex = 0; SlotIndex < static_cast<int32>(ESpearfishEquipmentSlot::Count); ++SlotIndex)
			{
				const ESpearfishEquipmentSlot Slot = static_cast<ESpearfishEquipmentSlot>(SlotIndex);
				Rows.Add(MakeHeader(SpearfishText::SlotName(Slot)));
				for (const FSpearfishEquipmentDef* Def : Context.Registry->GetEquipmentForSlot(Slot))
				{
					if (!Def || Def->bStarter)
					{
						continue;
					}
					FSpearfishUIRow Row;
					Row.Label = Def->DisplayName;
					Row.Detail = DescribeEquipment(*Def);
					Row.Id = Def->Id;
					const ESpearfishRequirementResult Result = Context.Progression->CheckEquipment(Def->Id);
					Row.Value = RequirementValue(Context, Result, Def->Unlock);
					if (Result == ESpearfishRequirementResult::AlreadyOwned)
					{
						const bool bEquipped = Context.Progression->GetEquipped(Slot) == Def->Id;
						Row.Value = bEquipped ? LOCTEXT("Equipped", "equipped") : LOCTEXT("OwnedEquip", "owned - equip");
						Row.Action = bEquipped ? ESpearfishUIAction::None : ESpearfishUIAction::Equip;
						Row.Color = Muted;
						Row.ValueColor = bEquipped ? Good : FLinearColor::White;
					}
					else
					{
						Row.Action = ESpearfishUIAction::BuyEquipment;
						Row.bEnabled = Result == ESpearfishRequirementResult::Ok;
						Row.Color = Row.bEnabled ? FLinearColor::White : Muted;
						Row.ValueColor = Row.bEnabled ? Good : Bad;
					}
					Rows.Add(Row);
				}
			}
		}

		void BuildShopUpgrades(const FContext& Context, TArray<FSpearfishUIRow>& Rows)
		{
			Rows.Add(MakeHeader(LOCTEXT("UpgradeShop", "Boat & restaurant upgrades")));
			TArray<const FSpearfishRestaurantUpgradeDef*> Upgrades;
			for (const TPair<FName, FSpearfishRestaurantUpgradeDef>& Pair : Context.Registry->GetAllUpgrades())
			{
				Upgrades.Add(&Pair.Value);
			}
			Algo::Sort(Upgrades, [](const FSpearfishRestaurantUpgradeDef* A, const FSpearfishRestaurantUpgradeDef* B) { return A->Unlock.Price < B->Unlock.Price; });
			for (const FSpearfishRestaurantUpgradeDef* Def : Upgrades)
			{
				FSpearfishUIRow Row;
				Row.Label = Def->DisplayName;
				Row.Id = Def->Id;
				TArray<FString> Effects;
				if (Def->ExtraSeats > 0) { Effects.Add(FString::Printf(TEXT("+%d seats"), Def->ExtraSeats)); }
				if (Def->CookZoneBonus > 0.f) { Effects.Add(TEXT("easier cooking")); }
				if (Def->DishQualityBonus > 0.f) { Effects.Add(FString::Printf(TEXT("+%d%% dish quality"), FMath::RoundToInt(Def->DishQualityBonus * 100.f))); }
				if (Def->PatienceBonus > 0.f) { Effects.Add(FString::Printf(TEXT("+%d%% guest patience"), FMath::RoundToInt(Def->PatienceBonus * 100.f))); }
				if (Def->AutoChefSkillBonus > 0.f) { Effects.Add(TEXT("better auto-chef")); }
				if (Def->bUnlocksTelemetry) { Effects.Add(TEXT("diver telemetry on the tablet")); }
				if (Def->bUnlocksDiverLocator) { Effects.Add(TEXT("diver locator on the chef's screen")); }
				Row.Detail = FText::FromString(FString::Join(Effects, TEXT(", ")));
				const ESpearfishRequirementResult Result = Context.Progression->CheckUpgrade(Def->Id);
				Row.Value = RequirementValue(Context, Result, Def->Unlock);
				Row.Action = Result == ESpearfishRequirementResult::AlreadyOwned ? ESpearfishUIAction::None : ESpearfishUIAction::BuyUpgrade;
				Row.bEnabled = Result == ESpearfishRequirementResult::Ok;
				Row.Color = Result == ESpearfishRequirementResult::AlreadyOwned || !Row.bEnabled ? Muted : FLinearColor::White;
				Row.ValueColor = Result == ESpearfishRequirementResult::AlreadyOwned ? Good : (Row.bEnabled ? Good : Bad);
				Rows.Add(Row);
			}
		}

		void BuildLocker(const FContext& Context, TArray<FSpearfishUIRow>& Rows)
		{
			for (int32 SlotIndex = 0; SlotIndex < static_cast<int32>(ESpearfishEquipmentSlot::Count); ++SlotIndex)
			{
				const ESpearfishEquipmentSlot Slot = static_cast<ESpearfishEquipmentSlot>(SlotIndex);
				Rows.Add(MakeHeader(SpearfishText::SlotName(Slot)));
				for (const FSpearfishEquipmentDef* Def : Context.Registry->GetEquipmentForSlot(Slot))
				{
					if (!Def || !Context.Progression->OwnsEquipment(Def->Id))
					{
						continue;
					}
					const bool bEquipped = Context.Progression->GetEquipped(Slot) == Def->Id;
					FSpearfishUIRow Row;
					Row.Label = Def->DisplayName;
					Row.Detail = DescribeEquipment(*Def);
					Row.Id = Def->Id;
					Row.Action = bEquipped ? ESpearfishUIAction::None : ESpearfishUIAction::Equip;
					Row.Value = bEquipped ? LOCTEXT("LockerEquipped", "equipped") : LOCTEXT("LockerEquip", "equip");
					Row.ValueColor = bEquipped ? Good : FLinearColor::White;
					Rows.Add(Row);
				}
			}
			Rows.Add(MakeInfo(LOCTEXT("LockerHint", "The crew shares one kit: whoever dives wears it."), FText::GetEmpty(), Muted));
		}

		void BuildChart(const FContext& Context, TArray<FSpearfishUIRow>& Rows, const FName PendingTravel)
		{
			Rows.Add(MakeHeader(LOCTEXT("ChartHeader", "Sea chart - the boat sails overnight")));
			TArray<const FSpearfishRegionDef*> Regions;
			for (const TPair<FName, FSpearfishRegionDef>& Pair : Context.Registry->GetAllRegions())
			{
				Regions.Add(&Pair.Value);
			}
			Algo::Sort(Regions, [](const FSpearfishRegionDef* A, const FSpearfishRegionDef* B) { return A->Unlock.Price < B->Unlock.Price; });
			for (const FSpearfishRegionDef* Region : Regions)
			{
				FSpearfishUIRow Row;
				Row.Label = Region->DisplayName;
				Row.Detail = Region->Description;
				Row.Id = Region->Id;
				if (Region->Id == Context.GameState->GetCurrentRegion())
				{
					Row.Value = LOCTEXT("ChartHere", "you are here");
					Row.ValueColor = Good;
				}
				else if (Context.Progression->IsRegionUnlocked(Region->Id))
				{
					Row.Action = ESpearfishUIAction::TravelRegion;
					Row.Value = Region->Id == PendingTravel ? LOCTEXT("ChartPlotted", "course set for tonight") : LOCTEXT("ChartSail", "sail here tonight");
					Row.ValueColor = Region->Id == PendingTravel ? Good : FLinearColor::White;
				}
				else
				{
					const ESpearfishRequirementResult Result = Context.Progression->CheckRegion(Region->Id);
					Row.Action = ESpearfishUIAction::UnlockRegion;
					Row.bEnabled = Result == ESpearfishRequirementResult::Ok;
					Row.Value = Result == ESpearfishRequirementResult::Ok || Result == ESpearfishRequirementResult::NotEnoughMoney
						? FText::Format(LOCTEXT("ChartBuy", "chart {0}"), FText::FromString(SpearfishText::Money(Region->Unlock.Price)))
						: SpearfishText::RequirementText(Result);
					Row.ValueColor = Row.bEnabled ? Good : Bad;
					Row.Color = Row.bEnabled ? FLinearColor::White : Muted;
				}
				Rows.Add(Row);
			}
		}

		// --------------------------------------------------------------------------------- Journal

		void BuildJournalFish(const FContext& Context, TArray<FSpearfishUIRow>& Rows)
		{
			if (!Context.Journal)
			{
				return;
			}
			Rows.Add(MakeHeader(FText::Format(LOCTEXT("JournalHeader", "Species caught: {0} / {1}"), FText::AsNumber(Context.Journal->CountCaughtSpecies()),
				FText::AsNumber(Context.Registry->GetAllFish().Num()))));
			for (const FName& SpeciesId : Context.Registry->GetSortedFishIds())
			{
				const FSpearfishFishSpeciesDef* Species = Context.Registry->FindFish(SpeciesId);
				const FSpearfishJournalEntry* Entry = Context.Journal->FindEntry(SpeciesId);
				if (!Species)
				{
					continue;
				}
				FSpearfishUIRow Row;
				if (!Entry || (!Entry->bSighted && Entry->Caught == 0))
				{
					Row.Label = LOCTEXT("Unknown", "???");
					Row.Detail = FText::Format(LOCTEXT("UnknownHint", "{0} - not yet seen"), SpearfishText::RarityName(Species->Rarity));
					Row.Color = Muted;
				}
				else
				{
					Row.Label = Species->DisplayName;
					Row.Color = SpearfishText::RarityColor(Species->Rarity);
					if (Entry->Caught > 0)
					{
						Row.Detail = FText::Format(LOCTEXT("CaughtDetail", "{0} - {1} to {2} m - best {3}"), SpearfishText::RarityName(Species->Rarity),
							FText::AsNumber(FMath::RoundToInt(Species->MinDepthM)), FText::AsNumber(FMath::RoundToInt(Species->MaxDepthM)),
							FText::FromString(SpearfishText::Length(Entry->BestLengthCm)));
						Row.Value = FText::Format(LOCTEXT("CaughtCount", "caught {0}"), FText::AsNumber(Entry->Caught));
					}
					else
					{
						Row.Detail = FText::Format(LOCTEXT("SeenDetail", "{0} - seen, not caught"), SpearfishText::RarityName(Species->Rarity));
						Row.Value = LOCTEXT("Seen", "seen");
					}
				}
				Rows.Add(Row);
			}
		}

		void BuildJournalFinds(const FContext& Context, TArray<FSpearfishUIRow>& Rows)
		{
			if (!Context.Journal)
			{
				return;
			}
			Rows.Add(MakeHeader(LOCTEXT("FindsHeader", "Finds")));
			TArray<FName> LootIds;
			Context.Registry->GetAllLoot().GetKeys(LootIds);
			LootIds.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
			for (const FName& LootId : LootIds)
			{
				const FSpearfishLootDef* Loot = Context.Registry->FindLoot(LootId);
				const FSpearfishLootRecord* Record = Context.Journal->GetLootRecords().FindByPredicate([LootId](const FSpearfishLootRecord& R) { return R.LootId == LootId; });
				if (!Loot)
				{
					continue;
				}
				FSpearfishUIRow Row;
				if (Record && Record->Found > 0)
				{
					Row.Label = Loot->DisplayName;
					Row.Detail = Loot->Description;
					Row.Value = FText::Format(LOCTEXT("FoundCount", "found {0}"), FText::AsNumber(Record->Found));
					Row.Color = SpearfishText::RarityColor(Loot->Rarity);
				}
				else
				{
					Row.Label = LOCTEXT("UnknownFind", "???");
					Row.Detail = SpearfishText::RarityName(Loot->Rarity);
					Row.Color = Muted;
				}
				Rows.Add(Row);
			}
		}

		void BuildPause(const FContext& Context, TArray<FSpearfishUIRow>& Rows)
		{
			auto Add = [&Rows](ESpearfishUIAction Action, const FText& Label, const FText& Detail = FText::GetEmpty())
			{
				FSpearfishUIRow Row;
				Row.Action = Action;
				Row.Label = Label;
				Row.Detail = Detail;
				Rows.Add(Row);
			};
			const bool bHost = Context.Controller && Context.Controller->HasAuthority();
			Add(ESpearfishUIAction::Resume, LOCTEXT("Resume", "Resume"));
			Add(ESpearfishUIAction::OpenHelp, LOCTEXT("Controls", "Controls & how to play"));
			Add(ESpearfishUIAction::SaveQuit, bHost ? LOCTEXT("SaveQuit", "Save and return to menu") : LOCTEXT("Leave", "Leave the session"),
				bHost ? LOCTEXT("SaveQuitDetail", "Progress, gear, the journal and the cooler are kept. Today restarts from the morning.") : FText::GetEmpty());
			Add(ESpearfishUIAction::QuitGame, LOCTEXT("QuitGame", "Quit to desktop"));
		}
	}

	TArray<FText> GetTabs(ESpearfishUIPanel Panel)
	{
		switch (Panel)
		{
		case ESpearfishUIPanel::Tablet:
			return { LOCTEXT("TabOrders", "Orders"), LOCTEXT("TabKitchen", "Kitchen"), LOCTEXT("TabCooler", "Cooler"), LOCTEXT("TabDiver", "Diver"),
				LOCTEXT("TabComms", "Comms"), LOCTEXT("TabRestaurant", "Restaurant") };
		case ESpearfishUIPanel::Shop:
			return { LOCTEXT("TabGear", "Dive gear"), LOCTEXT("TabUpgrades", "Upgrades") };
		case ESpearfishUIPanel::Journal:
			return { LOCTEXT("TabFish", "Fish"), LOCTEXT("TabFinds", "Finds") };
		default:
			return {};
		}
	}

	FText PanelTitle(ESpearfishUIPanel Panel)
	{
		switch (Panel)
		{
		case ESpearfishUIPanel::Tablet: return LOCTEXT("TitleTablet", "Waterproof tablet");
		case ESpearfishUIPanel::Shop: return LOCTEXT("TitleShop", "Dive shop");
		case ESpearfishUIPanel::Locker: return LOCTEXT("TitleLocker", "Gear locker");
		case ESpearfishUIPanel::Chart: return LOCTEXT("TitleChart", "Chart table");
		case ESpearfishUIPanel::Journal: return LOCTEXT("TitleJournal", "Field journal");
		case ESpearfishUIPanel::Help: return LOCTEXT("TitleHelp", "How to play");
		case ESpearfishUIPanel::DaySummary: return LOCTEXT("TitleSummary", "End of the day");
		case ESpearfishUIPanel::Pause: return LOCTEXT("TitlePause", "Paused (the sea keeps moving)");
		default: return FText::GetEmpty();
		}
	}

	TArray<FSpearfishUIRow> BuildRows(const APlayerController* Controller, ESpearfishUIPanel Panel, int32 Tab, FName PendingTravel)
	{
		TArray<FSpearfishUIRow> Rows;
		const FContext Context = MakeContext(Controller);
		if (Panel == ESpearfishUIPanel::Pause)
		{
			BuildPause(Context, Rows);
			return Rows;
		}
		if (!Context.IsValid())
		{
			return Rows;
		}
		switch (Panel)
		{
		case ESpearfishUIPanel::Tablet:
			switch (Tab)
			{
			case TabletOrders: BuildOrders(Context, Rows); break;
			case TabletKitchen: BuildKitchen(Context, Rows); break;
			case TabletCooler: BuildCooler(Context, Rows); break;
			case TabletDiver: BuildDiver(Context, Rows); break;
			case TabletComms: BuildComms(Context, Rows); break;
			default: BuildRestaurant(Context, Rows); break;
			}
			break;
		case ESpearfishUIPanel::Shop:
			if (Tab == 0)
			{
				BuildShopGear(Context, Rows);
			}
			else
			{
				BuildShopUpgrades(Context, Rows);
			}
			break;
		case ESpearfishUIPanel::Locker:
			BuildLocker(Context, Rows);
			break;
		case ESpearfishUIPanel::Chart:
			BuildChart(Context, Rows, PendingTravel);
			break;
		case ESpearfishUIPanel::Journal:
			if (Tab == 0)
			{
				BuildJournalFish(Context, Rows);
			}
			else
			{
				BuildJournalFinds(Context, Rows);
			}
			break;
		default:
			break;
		}
		return Rows;
	}

	FText DescribeEquipment(const FSpearfishEquipmentDef& Def)
	{
		const FSpearfishEquipmentStats& S = Def.Stats;
		FString Text;
		switch (Def.Slot)
		{
		case ESpearfishEquipmentSlot::Speargun:
			Text = FString::Printf(TEXT("power %.0f, range %.0f m, reload %.1f s"), S.GunPower, S.GunRangeCm / 100.f, S.ReloadSeconds);
			break;
		case ESpearfishEquipmentSlot::Reel:
			Text = FString::Printf(TEXT("line %.0f m, breaks at %.0f, reel %.1f m/s"), S.LineLengthCm / 100.f, S.BreakForce, S.ReelSpeedCm / 100.f);
			break;
		case ESpearfishEquipmentSlot::Tank:
			Text = FString::Printf(TEXT("%.0f air"), S.AirCapacity);
			break;
		case ESpearfishEquipmentSlot::Wetsuit:
			Text = FString::Printf(TEXT("comfortable to %.0f m, %d%% sting protection"), S.SafeDepthM, FMath::RoundToInt(S.StingResist * 100.f));
			break;
		case ESpearfishEquipmentSlot::Mask:
			Text = FString::Printf(TEXT("+%d%% visibility"), FMath::RoundToInt(S.VisibilityBonus * 100.f));
			break;
		case ESpearfishEquipmentSlot::Fins:
			Text = FString::Printf(TEXT("swim %.1f m/s, sprint x%.2f"), S.SwimSpeedCm / 100.f, S.SprintMultiplier);
			break;
		case ESpearfishEquipmentSlot::Bag:
			Text = FString::Printf(TEXT("%d fish slots, %d loot slots, %.0f kg"), S.FishSlots, S.LootSlots, S.MaxWeightKg);
			break;
		case ESpearfishEquipmentSlot::Light:
			Text = S.LightIntensity > 0.f ? FString::Printf(TEXT("beam %.0f m"), S.LightRangeCm / 100.f) : FString(TEXT("no light"));
			break;
		default:
			break;
		}
		return FText::FromString(Text);
	}

	int32 ClampSelection(const TArray<FSpearfishUIRow>& Rows, int32 Index, int32 Direction)
	{
		if (Rows.Num() == 0)
		{
			return INDEX_NONE;
		}
		const int32 Step = Direction < 0 ? -1 : 1;
		int32 Candidate = ((Index % Rows.Num()) + Rows.Num()) % Rows.Num();
		for (int32 Tries = 0; Tries < Rows.Num(); ++Tries)
		{
			if (Rows[Candidate].IsSelectable())
			{
				return Candidate;
			}
			Candidate = (Candidate + Step + Rows.Num()) % Rows.Num();
		}
		return INDEX_NONE;
	}
}

#undef LOCTEXT_NAMESPACE
