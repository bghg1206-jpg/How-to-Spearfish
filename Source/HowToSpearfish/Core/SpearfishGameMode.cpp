#include "Core/SpearfishGameMode.h"

#include "Boat/SpearfishBoat.h"
#include "Boat/SpearfishStation.h"
#include "Core/SpearfishGameInstance.h"
#include "Core/SpearfishGameState.h"
#include "Core/SpearfishPlayerController.h"
#include "Core/SpearfishPlayerState.h"
#include "Core/SpearfishSettings.h"
#include "Data/SpearfishDataRegistry.h"
#include "DayNight/SpearfishDayCycleComponent.h"
#include "Diving/SpearfishCharacter.h"
#include "Diving/SpearfishOxygenComponent.h"
#include "Engine/World.h"
#include "FishAI/SpearfishFishSubsystem.h"
#include "GameFramework/GameSession.h"
#include "HowToSpearfish.h"
#include "Inventory/SpearfishCatchStorageComponent.h"
#include "Inventory/SpearfishDiveBagComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Progression/SpearfishEventDirector.h"
#include "Progression/SpearfishJournalComponent.h"
#include "Progression/SpearfishProgressionComponent.h"
#include "Restaurant/SpearfishRestaurantComponent.h"
#include "Roles/SpearfishRoleSubsystem.h"
#include "Rules/DayRules.h"
#include "Rules/EconomyRules.h"
#include "SaveSystem/SpearfishSaveGame.h"
#include "Speargun/SpeargunComponent.h"
#include "TimerManager.h"
#include "UI/SpearfishHUD.h"
#include "World/SpearfishOceanSubsystem.h"
#include "World/SpearfishRegionBuilder.h"

#define LOCTEXT_NAMESPACE "SpearfishGameMode"

namespace SpearfishGameModePrivate
{
	constexpr float FadeSeconds = 1.6f;
	constexpr float NightProcessDelay = 2.f;
	constexpr float NightSummarySeconds = 4.5f;
	/** Fish below this quality are thrown away overnight. */
	constexpr float SpoiledBelowQuality = 0.2f;
}

ASpearfishGameMode::ASpearfishGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	GameStateClass = ASpearfishGameState::StaticClass();
	PlayerStateClass = ASpearfishPlayerState::StaticClass();
	PlayerControllerClass = ASpearfishPlayerController::StaticClass();
	DefaultPawnClass = ASpearfishCharacter::StaticClass();
	HUDClass = ASpearfishHUD::StaticClass();
	bUseSeamlessTravel = false;
}

// ------------------------------------------------------------------------------------ Session

void ASpearfishGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const USpearfishSettings* Settings = USpearfishSettings::Get();
	const FString Mode = UGameplayStatics::ParseOption(Options, TEXT("mode"));
	const bool bPIE = GetWorld()->IsPlayInEditor();
	// Sessions are always started with ?mode= by the menu. Without it this is a default boot (or the engine's
	// fallback after a disconnect), which belongs in the main menu - except for PIE auto-start.
	if (Mode.IsEmpty() && (!bPIE || !Settings->bAutoStartSessionInPIE))
	{
		bRedirectToMenu = true;
		UE_LOG(LogSpearfish, Log, TEXT("No session mode in the URL; returning to the main menu."));
		return;
	}
	if (Mode.IsEmpty())
	{
		// PIE: the editor's net mode decides (UWorld reports the PIE net mode before the net driver exists).
		bSolo = GetNetMode() == NM_Standalone && !Settings->bPIEStartsCoop;
	}
	else
	{
		bSolo = Mode.Equals(TEXT("solo"), ESearchCase::IgnoreCase);
	}
	bForceNewCampaign = UGameplayStatics::ParseOption(Options, TEXT("new")) == TEXT("1") || (bPIE && Settings->bPIEFreshCampaign);

	if (GameSession)
	{
		GameSession->MaxPlayers = bSolo ? 1 : 2;
	}

	if (USpearfishSaveSubsystem* Saves = GetGameInstance() ? GetGameInstance()->GetSubsystem<USpearfishSaveSubsystem>() : nullptr)
	{
		Campaign = Saves->LoadOrCreate(bSolo, bForceNewCampaign);
	}
	else
	{
		UE_LOG(LogSpearfish, Error, TEXT("Save subsystem missing; starting an unsaved campaign."));
	}

	// Fall back to a known region if the save references one that no longer exists.
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	if (Registry && !Registry->FindRegion(Campaign.CurrentRegion))
	{
		Campaign.CurrentRegion = USpearfishSettings::Get()->StartingRegion;
		if (!Registry->FindRegion(Campaign.CurrentRegion))
		{
			for (const TPair<FName, FSpearfishRegionDef>& Pair : Registry->GetAllRegions())
			{
				if (Pair.Value.bStarter)
				{
					Campaign.CurrentRegion = Pair.Key;
					break;
				}
			}
		}
	}

	if (USpearfishRoleSubsystem* Roles = GetWorld()->GetSubsystem<USpearfishRoleSubsystem>())
	{
		Roles->Configure(bSolo, Campaign.Seats);
	}
	UE_LOG(LogSpearfish, Log, TEXT("Spearfish session: %s, day %d, region %s%s"), bSolo ? TEXT("solo") : TEXT("co-op"), Campaign.Day,
		*Campaign.CurrentRegion.ToString(), bForceNewCampaign ? TEXT(" (new campaign)") : TEXT(""));
}

void ASpearfishGameMode::InitGameState()
{
	Super::InitGameState();
	ASpearfishGameState* State = GetGameState<ASpearfishGameState>();
	if (bRedirectToMenu)
	{
		return;
	}
	if (!State)
	{
		UE_LOG(LogSpearfish, Error, TEXT("GameState is not an ASpearfishGameState."));
		return;
	}
	State->SetSession(bSolo);
	State->SetCurrentRegion(Campaign.CurrentRegion);
	State->SetNextItemId(Campaign.NextItemId);
	State->GetProgression()->LoadFromCampaign(Campaign);
	State->GetJournal()->LoadFromCampaign(Campaign);
}

void ASpearfishGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (ErrorMessage.IsEmpty() && (bSolo || bRedirectToMenu))
	{
		ErrorMessage = TEXT("This is a solo game.");
	}
}

void ASpearfishGameMode::PostLogin(APlayerController* NewPlayer)
{
	// Roles first, so the pawn spawned by Super gets the right kit.
	ASpearfishPlayerState* State = NewPlayer ? NewPlayer->GetPlayerState<ASpearfishPlayerState>() : nullptr;
	USpearfishRoleSubsystem* Roles = GetWorld()->GetSubsystem<USpearfishRoleSubsystem>();
	ESpearfishRole AssignedRole = ESpearfishRole::Diver;
	if (State && Roles)
	{
		AssignedRole = Roles->RegisterPlayer(State);
	}

	Super::PostLogin(NewPlayer);

	if (bSessionReady && State)
	{
		if (ASpearfishGameState* SessionState = GetGameState<ASpearfishGameState>())
		{
			SessionState->BroadcastNotice(FText::Format(LOCTEXT("Joined", "{0} came aboard as the {1}."), FText::FromString(State->GetPlayerName()),
				SpearfishText::RoleName(AssignedRole)), ESpearfishNoticeType::Good);
		}
		if (ASpearfishPlayerController* Controller = Cast<ASpearfishPlayerController>(NewPlayer))
		{
			const USpearfishDayCycleComponent* DayCycle = GetGameState<ASpearfishGameState>()->GetDayCycle();
			Controller->ClientWakeUp(DayCycle ? DayCycle->GetDay() : 1, AssignedRole, false);
			Controller->ClientSetPendingTravel(PendingRegion);
		}
	}
}

void ASpearfishGameMode::Logout(AController* Exiting)
{
	ASpearfishPlayerState* State = Exiting ? Exiting->GetPlayerState<ASpearfishPlayerState>() : nullptr;
	if (ASpearfishCharacter* Character = Exiting ? Cast<ASpearfishCharacter>(Exiting->GetPawn()) : nullptr)
	{
		if (Character->IsDriving())
		{
			Character->StopDriving();
		}
		if (ASpearfishStation* Bed = Boat ? Boat->FindBedOccupiedBy(Character) : nullptr)
		{
			Bed->SetOccupant(nullptr);
		}
		// Anyone this player had on a line is set free.
		if (Character->GetSpeargun())
		{
			Character->GetSpeargun()->ForceRelease();
		}
	}
	if (State)
	{
		if (USpearfishRoleSubsystem* Roles = GetWorld()->GetSubsystem<USpearfishRoleSubsystem>())
		{
			Roles->UnregisterPlayer(State);
		}
		if (ASpearfishGameState* SessionState = GetGameState<ASpearfishGameState>(); SessionState && bSessionReady)
		{
			SessionState->BroadcastNotice(FText::Format(LOCTEXT("Left", "{0} left the boat. The auto-chef takes over the kitchen."), FText::FromString(State->GetPlayerName())),
				ESpearfishNoticeType::Warning);
		}
	}
	Super::Logout(Exiting);
}

void ASpearfishGameMode::StartPlay()
{
	Super::StartPlay();
	if (bRedirectToMenu)
	{
		if (USpearfishGameInstance* GameInstance = GetGameInstance<USpearfishGameInstance>())
		{
			GameInstance->LeaveToMenu();
		}
		return;
	}
	SetupSession();
}

void ASpearfishGameMode::SetupSession()
{
	ASpearfishGameState* State = GetGameState<ASpearfishGameState>();
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	if (!State || !Registry || !Registry->FindRegion(Campaign.CurrentRegion))
	{
		UE_LOG(LogSpearfish, Error, TEXT("Cannot start the session: missing game state, data or region '%s'. Run Tools/validate_data.py."),
			*Campaign.CurrentRegion.ToString());
		return;
	}

	SpawnRegion(Campaign.CurrentRegion);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Boat = GetWorld()->SpawnActor<ASpearfishBoat>(ASpearfishBoat::StaticClass(), FTransform::Identity, Params);
	if (!Boat)
	{
		UE_LOG(LogSpearfish, Error, TEXT("Failed to spawn the boat."));
		return;
	}
	PlaceBoatAtRegionStart();
	Boat->SpawnStations();
	State->SetBoat(Boat);
	Boat->GetCooler()->SetItems(Campaign.Cooler);

	const int32 Day = FMath::Max(1, Campaign.Day);
	State->GetDayCycle()->StartDay(Day);
	State->ResetTodayStats(Day, State->GetProgression()->GetReputation());
	StartDayContent(Day);

	bSessionReady = true;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Controller = It->Get();
		if (Controller && !Controller->GetPawn())
		{
			RestartPlayer(Controller);
		}
		if (ASpearfishPlayerController* SpearfishController = Cast<ASpearfishPlayerController>(Controller))
		{
			const ASpearfishPlayerState* PlayerState = SpearfishController->GetPlayerState<ASpearfishPlayerState>();
			SpearfishController->ClientWakeUp(Day, PlayerState ? PlayerState->GetRole() : ESpearfishRole::Diver, false);
		}
	}
	// A brand-new campaign is written immediately so "Continue" works even after a crash.
	if (!Campaign.bInitialized)
	{
		SaveCampaign();
	}
}

void ASpearfishGameMode::SpawnRegion(FName RegionId)
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FSpearfishRegionDef* Region = Registry ? Registry->FindRegion(RegionId) : nullptr;
	if (!Region)
	{
		return;
	}
	const FTransform Transform = FTransform::Identity;
	RegionBuilder = GetWorld()->SpawnActorDeferred<ASpearfishRegionBuilder>(ASpearfishRegionBuilder::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (RegionBuilder)
	{
		RegionBuilder->Configure(RegionId, Region->Seed);
		RegionBuilder->FinishSpawning(Transform);
		RegionBuilder->BuildStatic();
	}
}

void ASpearfishGameMode::PlaceBoatAtRegionStart()
{
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	if (!Boat || !Ocean || !Ocean->HasTerrain())
	{
		return;
	}
	const FSpearfishTerrainLayout& Layout = Ocean->GetTerrain().GetLayout();
	Boat->PlaceAt(FVector(Layout.BoatStart.X, Layout.BoatStart.Y, Ocean->GetSeaLevel()), Layout.BoatStartYaw);
}

int32 ASpearfishGameMode::MakeDaySeed(int32 Day) const
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FSpearfishRegionDef* Region = Registry ? Registry->FindRegion(Campaign.CurrentRegion) : nullptr;
	return static_cast<int32>(HashCombine(GetTypeHash(Region ? Region->Seed : 1), GetTypeHash(Day * 7919 + 31)) & 0x7fffffff);
}

void ASpearfishGameMode::StartDayContent(int32 Day)
{
	ASpearfishGameState* State = GetGameState<ASpearfishGameState>();
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const FSpearfishRegionDef* Region = Registry ? Registry->FindRegion(Campaign.CurrentRegion) : nullptr;
	if (!State || !Region)
	{
		return;
	}
	const int32 Seed = MakeDaySeed(Day);
	if (Boat)
	{
		Boat->GetRestaurant()->BeginDay(Day, Seed);
	}
	if (USpearfishFishSubsystem* Fish = USpearfishFishSubsystem::Get(this))
	{
		Fish->SpawnRegionPopulation(*Region, Seed);
	}
	if (RegionBuilder)
	{
		RegionBuilder->SpawnDailyContent(Day, Seed);
	}
	const TArray<FName> Events = SpearfishEventDirector::StartDay(GetWorld(), *Region, Day, Seed, RegionBuilder);
	UE_LOG(LogSpearfish, Log, TEXT("Day %d in %s: seed %d, %d event(s)"), Day, *Region->Id.ToString(), Seed, Events.Num());
}

bool ASpearfishGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return bSessionReady && Super::PlayerCanRestart_Implementation(Player);
}

void ASpearfishGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!bSessionReady || !Boat || !NewPlayer)
	{
		return;
	}
	RestartPlayerAtTransform(NewPlayer, GetSpawnTransformFor(NewPlayer));
}

FTransform ASpearfishGameMode::GetSpawnTransformFor(AController* Controller) const
{
	const ASpearfishPlayerState* State = Controller ? Controller->GetPlayerState<ASpearfishPlayerState>() : nullptr;
	return Boat ? Boat->GetDeckSpawnTransform(State ? State->GetSeatIndex() : 0) : FTransform(FVector(0.f, 0.f, 300.f));
}

TArray<ASpearfishCharacter*> ASpearfishGameMode::GetPlayerCharacters() const
{
	TArray<ASpearfishCharacter*> Characters;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASpearfishCharacter* Character = It->Get() ? Cast<ASpearfishCharacter>(It->Get()->GetPawn()) : nullptr)
		{
			Characters.Add(Character);
		}
	}
	return Characters;
}

// --------------------------------------------------------------------------------- Day cycle

void ASpearfishGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bSessionReady || bNightInProgress)
	{
		return;
	}
	SleepCheckTimer -= DeltaSeconds;
	if (SleepCheckTimer <= 0.f)
	{
		SleepCheckTimer = 0.5f;
		TickSleepCheck();
	}
}

void ASpearfishGameMode::TickSleepCheck()
{
	const ASpearfishGameState* State = GetGameState<ASpearfishGameState>();
	const USpearfishDayCycleComponent* DayCycle = State ? State->GetDayCycle() : nullptr;
	if (!DayCycle || DayCycle->GetPhase() == ESpearfishDayPhase::Sleeping)
	{
		return;
	}
	if (SpearfishDay::ShouldPassOut(DayCycle->GetHour(), DayCycle->GetSchedule()))
	{
		BeginNight(true);
		return;
	}
	if (!DayCycle->CanSleepNow())
	{
		return;
	}
	TArray<bool> Ready;
	for (const TObjectPtr<APlayerState>& PlayerState : State->PlayerArray)
	{
		if (const ASpearfishPlayerState* SpearfishState = Cast<ASpearfishPlayerState>(PlayerState))
		{
			if (!SpearfishState->IsInactive())
			{
				Ready.Add(SpearfishState->IsInBed());
			}
		}
	}
	if (SpearfishDay::EveryoneReady(Ready))
	{
		BeginNight(false);
	}
}

void ASpearfishGameMode::BeginNight(bool bPassedOut)
{
	if (bNightInProgress)
	{
		return;
	}
	ASpearfishGameState* State = GetGameState<ASpearfishGameState>();
	if (!State)
	{
		return;
	}
	bNightInProgress = true;
	bNightPassedOut = bPassedOut;
	State->GetDayCycle()->SetSleepingPhase(true);
	if (Boat)
	{
		Boat->GetRestaurant()->CloseForNight();
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASpearfishPlayerController* Controller = Cast<ASpearfishPlayerController>(It->Get()))
		{
			Controller->ClientBeginNight(SpearfishGameModePrivate::FadeSeconds);
		}
	}
	GetWorldTimerManager().SetTimer(NightTimer, this, &ASpearfishGameMode::FinishNight, SpearfishGameModePrivate::NightProcessDelay, false);
}

void ASpearfishGameMode::FinishNight()
{
	ASpearfishGameState* State = GetGameState<ASpearfishGameState>();
	USpearfishRoleSubsystem* Roles = GetWorld()->GetSubsystem<USpearfishRoleSubsystem>();
	if (!State || !Boat)
	{
		bNightInProgress = false;
		return;
	}
	USpearfishProgressionComponent* Progression = State->GetProgression();
	const USpearfishSettings* Settings = USpearfishSettings::Get();

	// Stragglers who never made it to bed pay for it; a diver still in the water also loses the bag.
	for (ASpearfishCharacter* Character : GetPlayerCharacters())
	{
		if (Character->IsDriving())
		{
			Character->StopDriving();
		}
		if (Character->GetSpeargun())
		{
			Character->GetSpeargun()->ForceRelease();
		}
		if (!Character->IsInBed())
		{
			const int32 Fee = SpearfishEconomy::PassOutFee(Progression->GetMoney(), Settings->PassOutFee);
			Progression->AddMoney(-Fee);
			State->EditTodayStats().Expenses += Fee;
			if (Character->IsSwimming() && Character->GetBag())
			{
				Character->GetBag()->TakeAll();
				Character->NotifyOwner(LOCTEXT("PassOutWater", "You passed out in the water. A fisherman hauled you aboard - your catch is gone."),
					ESpearfishNoticeType::Danger);
			}
			else
			{
				Character->NotifyOwner(FText::Format(LOCTEXT("PassOut", "You passed out on deck (-{0})."), FText::FromString(SpearfishText::Money(Fee))),
					ESpearfishNoticeType::Warning);
			}
		}
	}

	const int32 Spoiled = Boat->GetCooler()->ApplySpoilage(Settings->OvernightSpoilage, SpearfishGameModePrivate::SpoiledBelowQuality);
	State->EditTodayStats().ReputationEnd = Progression->GetReputation();

	FSpearfishDaySummary Summary;
	BuildSummary(Summary, bNightPassedOut, Spoiled);

	// Remember who was what, then swap.
	TMap<TWeakObjectPtr<APlayerState>, ESpearfishRole> RolesBefore;
	for (const TObjectPtr<APlayerState>& PlayerState : State->PlayerArray)
	{
		if (const ASpearfishPlayerState* SpearfishState = Cast<ASpearfishPlayerState>(PlayerState))
		{
			RolesBefore.Add(PlayerState.Get(), SpearfishState->GetRole());
		}
	}
	if (Roles)
	{
		Roles->ResolveNewDay();
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASpearfishPlayerController* Controller = Cast<ASpearfishPlayerController>(It->Get()))
		{
			Controller->ClientShowDaySummary(Summary);
		}
	}

	if (!PendingRegion.IsNone() && PendingRegion != Campaign.CurrentRegion)
	{
		ChangeRegion(PendingRegion);
	}
	PendingRegion = NAME_None;

	const int32 NextDay = State->GetDayCycle()->GetDay() + 1;
	Campaign.Day = NextDay;
	StartDayContent(NextDay);

	// Morning comes after a short beat on the summary screen.
	FTimerDelegate Wake;
	Wake.BindWeakLambda(this, [this, NextDay, RolesBefore]()
	{
		ASpearfishGameState* WakeState = GetGameState<ASpearfishGameState>();
		if (!WakeState)
		{
			return;
		}
		WakeState->GetDayCycle()->StartDay(NextDay);
		WakeState->ResetTodayStats(NextDay, WakeState->GetProgression()->GetReputation());
		for (const TObjectPtr<APlayerState>& PlayerState : WakeState->PlayerArray)
		{
			if (ASpearfishPlayerState* SpearfishState = Cast<ASpearfishPlayerState>(PlayerState))
			{
				SpearfishState->ResetDailyStats();
			}
		}
		WakePlayers();
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			ASpearfishPlayerController* Controller = Cast<ASpearfishPlayerController>(It->Get());
			const ASpearfishPlayerState* SpearfishState = Controller ? Controller->GetPlayerState<ASpearfishPlayerState>() : nullptr;
			if (!Controller || !SpearfishState)
			{
				continue;
			}
			const ESpearfishRole* Before = RolesBefore.Find(Controller->PlayerState.Get());
			const bool bChanged = Before && *Before != SpearfishState->GetRole();
			Controller->ClientWakeUp(NextDay, SpearfishState->GetRole(), bChanged);
		}
		bNightInProgress = false;
		SaveCampaign();
	});
	GetWorldTimerManager().SetTimer(NightTimer, Wake, SpearfishGameModePrivate::NightSummarySeconds, false);
}

void ASpearfishGameMode::BuildSummary(FSpearfishDaySummary& Summary, bool bPassedOut, int32 Spoiled) const
{
	const ASpearfishGameState* State = GetGameState<ASpearfishGameState>();
	if (!State)
	{
		return;
	}
	Summary.Stats = State->GetTodayStats();
	Summary.Stats.Day = State->GetDayCycle()->GetDay();
	Summary.bPassedOut = bPassedOut;
	Summary.SpoiledFish = Spoiled;
}

void ASpearfishGameMode::WakePlayers()
{
	for (ASpearfishCharacter* Character : GetPlayerCharacters())
	{
		ASpearfishStation* Bed = Boat ? Boat->FindBedOccupiedBy(Character) : nullptr;
		if (Character->IsInBed())
		{
			Character->LeaveBed(Bed ? Bed->GetExitTransform() : GetSpawnTransformFor(Character->GetController()));
		}
		else if (!Character->IsBlackedOut())
		{
			Character->TeleportForGameplay(GetSpawnTransformFor(Character->GetController()), false);
		}
		if (Bed)
		{
			Bed->SetOccupant(nullptr);
		}
		if (Character->GetOxygen())
		{
			Character->GetOxygen()->Refill();
		}
		Character->ApplyRoleLoadout(true);
	}
}

void ASpearfishGameMode::ChangeRegion(FName RegionId)
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	ASpearfishGameState* State = GetGameState<ASpearfishGameState>();
	if (!Registry || !Registry->FindRegion(RegionId) || !State)
	{
		return;
	}
	if (USpearfishFishSubsystem* Fish = USpearfishFishSubsystem::Get(this))
	{
		Fish->DespawnAll();
	}
	if (RegionBuilder)
	{
		RegionBuilder->Destroy();
		RegionBuilder = nullptr;
	}
	Campaign.CurrentRegion = RegionId;
	State->SetCurrentRegion(RegionId);
	SpawnRegion(RegionId);
	PlaceBoatAtRegionStart();
	State->BroadcastNotice(FText::Format(LOCTEXT("Sailed", "The boat sailed through the night. Welcome to {0}!"), Registry->GetDisplayName(RegionId)),
		ESpearfishNoticeType::Discovery);
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASpearfishPlayerController* Controller = Cast<ASpearfishPlayerController>(It->Get()))
		{
			Controller->ClientSetPendingTravel(NAME_None);
		}
	}
}

// ------------------------------------------------------------------------- Beds and rescue

void ASpearfishGameMode::HandleEnterBed(ASpearfishCharacter* Character, ASpearfishStation* Bed)
{
	ASpearfishGameState* State = GetGameState<ASpearfishGameState>();
	if (!Character || !Bed || !State || bNightInProgress || Character->IsInBed())
	{
		return;
	}
	if (Bed->GetOccupant() && Bed->GetOccupant() != Character)
	{
		Character->NotifyOwner(LOCTEXT("BedTaken", "That bunk is taken."), ESpearfishNoticeType::Info);
		return;
	}
	const USpearfishDayCycleComponent* DayCycle = State->GetDayCycle();
	if (!DayCycle->CanSleepNow())
	{
		Character->NotifyOwner(FText::Format(LOCTEXT("TooEarly", "Too early to sleep. Bunks open at {0}."),
			FText::FromString(SpearfishText::Clock(DayCycle->GetSchedule().SleepAllowedHour))), ESpearfishNoticeType::Info);
		return;
	}
	Character->EnterBed(Bed->GetLieTransform());
	Bed->SetOccupant(Character);
	if (!bSolo)
	{
		State->BroadcastNotice(FText::Format(LOCTEXT("InBed", "{0} turned in for the night. The day ends when everyone is in bed."),
			FText::FromString(Character->GetPlayerName())), ESpearfishNoticeType::Info);
	}
}

void ASpearfishGameMode::HandleLeaveBed(ASpearfishCharacter* Character)
{
	if (!Character || !Character->IsInBed() || bNightInProgress)
	{
		return;
	}
	ASpearfishStation* Bed = Boat ? Boat->FindBedOccupiedBy(Character) : nullptr;
	Character->LeaveBed(Bed ? Bed->GetExitTransform() : GetSpawnTransformFor(Character->GetController()));
	if (Bed)
	{
		Bed->SetOccupant(nullptr);
	}
}

void ASpearfishGameMode::RescueDiver(ASpearfishCharacter* Character)
{
	ASpearfishGameState* State = GetGameState<ASpearfishGameState>();
	if (!Character || !State)
	{
		return;
	}
	USpearfishProgressionComponent* Progression = State->GetProgression();
	const int32 Fee = SpearfishEconomy::RescueFee(Progression->GetMoney(), USpearfishSettings::Get()->RescueFee);
	Progression->AddMoney(-Fee);
	State->EditTodayStats().Expenses += Fee;
	Character->Rescue(Boat ? Boat->GetLadderTopTransform() : GetSpawnTransformFor(Character->GetController()));
	Character->NotifyOwner(Fee > 0 ? FText::Format(LOCTEXT("Rescued", "You came to on the deck. The rescue cost {0}."), FText::FromString(SpearfishText::Money(Fee)))
		: LOCTEXT("RescuedFree", "You came to on the deck. Breathe."), ESpearfishNoticeType::Warning);
}

// ------------------------------------------------------------------------------ Travel / save

void ASpearfishGameMode::SetTravelDestination(ASpearfishPlayerController* By, FName RegionId)
{
	ASpearfishGameState* State = GetGameState<ASpearfishGameState>();
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	if (!State || !Registry || !Registry->FindRegion(RegionId) || !State->GetProgression()->IsRegionUnlocked(RegionId))
	{
		return;
	}
	PendingRegion = RegionId == Campaign.CurrentRegion ? NAME_None : RegionId;
	State->BroadcastNotice(PendingRegion.IsNone() ? LOCTEXT("Staying", "Course cleared: the boat stays here tonight.")
		: FText::Format(LOCTEXT("CourseSet", "Course set for {0}. The boat sails tonight while you sleep."), Registry->GetDisplayName(RegionId)),
		ESpearfishNoticeType::Info);
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASpearfishPlayerController* Controller = Cast<ASpearfishPlayerController>(It->Get()))
		{
			Controller->ClientSetPendingTravel(PendingRegion);
		}
	}
}

void ASpearfishGameMode::SaveCampaign()
{
	ASpearfishGameState* State = GetGameState<ASpearfishGameState>();
	if (!bSessionReady || !State)
	{
		return;
	}
	if (GetWorld()->IsPlayInEditor() && USpearfishSettings::Get()->bPIEFreshCampaign)
	{
		UE_LOG(LogSpearfish, Verbose, TEXT("PIE fresh campaign: not writing the save slot."));
		return;
	}
	Campaign.Day = FMath::Max(Campaign.Day, State->GetDayCycle()->GetDay());
	State->GetProgression()->SaveToCampaign(Campaign);
	State->GetJournal()->SaveToCampaign(Campaign);
	Campaign.Cooler = Boat ? Boat->GetCooler()->GetItems() : TArray<FSpearfishItem>();
	if (const USpearfishRoleSubsystem* Roles = GetWorld()->GetSubsystem<USpearfishRoleSubsystem>())
	{
		Campaign.Seats = Roles->GetSeats();
	}
	Campaign.NextItemId = State->GetNextItemId();
	Campaign.CurrentRegion = State->GetCurrentRegion();
	Campaign.bInitialized = true;
	if (USpearfishSaveSubsystem* Saves = GetGameInstance()->GetSubsystem<USpearfishSaveSubsystem>())
	{
		Saves->Save(bSolo, Campaign);
	}
}

void ASpearfishGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Quit || EndPlayReason == EEndPlayReason::LevelTransition)
	{
		SaveCampaign();
	}
	GetWorldTimerManager().ClearTimer(NightTimer);
	Super::EndPlay(EndPlayReason);
}

// ------------------------------------------------------------------------------------- Debug

void ASpearfishGameMode::HandleDebugCommand(ASpearfishPlayerController* By, const FString& Command, const FString& Argument)
{
#if !UE_BUILD_SHIPPING
	ASpearfishGameState* State = GetGameState<ASpearfishGameState>();
	if (!State || !By)
	{
		return;
	}
	UE_LOG(LogSpearfish, Log, TEXT("Debug command from %s: %s %s"), *By->GetName(), *Command, *Argument);
	if (Command == TEXT("Money"))
	{
		State->GetProgression()->AddMoney(FCString::Atoi(*Argument));
	}
	else if (Command == TEXT("Hour"))
	{
		State->GetDayCycle()->SetHour(FCString::Atof(*Argument));
	}
	else if (Command == TEXT("EndDay"))
	{
		BeginNight(false);
	}
	else if (Command == TEXT("SpawnFish"))
	{
		const APawn* Pawn = By->GetPawn();
		USpearfishFishSubsystem* Fish = USpearfishFishSubsystem::Get(this);
		if (Pawn && Fish)
		{
			Fish->SpawnFish(FName(*Argument), Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 450.0, INDEX_NONE);
		}
	}
	else if (Command == TEXT("Role"))
	{
		ASpearfishPlayerState* PlayerState = By->GetPlayerState<ASpearfishPlayerState>();
		const ESpearfishRole DebugRole = Argument.Equals(TEXT("Chef"), ESearchCase::IgnoreCase) ? ESpearfishRole::Chef : ESpearfishRole::Diver;
		if (PlayerState)
		{
			PlayerState->SetRole(DebugRole);
			State->SetAutoChefActive(State->FindPlayerWithRole(ESpearfishRole::Chef) == nullptr);
		}
	}
	else if (Command == TEXT("Event"))
	{
		const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
		const FSpearfishRegionDef* Region = Registry ? Registry->FindRegion(Campaign.CurrentRegion) : nullptr;
		if (Region)
		{
			SpearfishEventDirector::ForceEvent(GetWorld(), *Region, FName(*Argument), FMath::Rand(), RegionBuilder);
		}
	}
#endif
}

#undef LOCTEXT_NAMESPACE
