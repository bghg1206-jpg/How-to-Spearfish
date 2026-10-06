#include "Loot/SpearfishGiantClam.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/SpearfishGameState.h"
#include "Diving/SpearfishCharacter.h"
#include "Diving/SpearfishOxygenComponent.h"
#include "Engine/World.h"
#include "FishAI/SpearfishFishSubsystem.h"
#include "HowToSpearfish.h"
#include "Inventory/SpearfishCatchService.h"
#include "Net/UnrealNetwork.h"
#include "World/SpearfishVisualSubsystem.h"

#define LOCTEXT_NAMESPACE "SpearfishClam"

namespace SpearfishClamPrivate
{
	constexpr float OpenSecondsMin = 6.f;
	constexpr float OpenSecondsMax = 9.f;
	constexpr float ClosedSecondsMin = 4.f;
	constexpr float ClosedSecondsMax = 7.f;
	/** The last stretch of the open phase, when the shell trembles and a grab gets caught. */
	constexpr float ClosingWarningSeconds = 1.4f;
	constexpr float SnapAirLoss = 30.f;
	constexpr float BlackPearlChance = 0.15f;
	constexpr float PearlChance = 0.65f;
	constexpr float OpenAngle = 38.f;
}

ASpearfishGiantClam::ASpearfishGiantClam()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(2.f);
	SetNetCullDistanceSquared(FMath::Square(9000.f));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	InteractBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractBox"));
	InteractBox->SetupAttachment(Root);
	InteractBox->SetBoxExtent(FVector(60.f, 45.f, 35.f));
	InteractBox->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	InteractBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractBox->SetCollisionResponseToChannel(ECC_Interact, ECR_Block);

	// The hinge sits at the back of the shell; the upper valve rotates around it.
	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(Root);
	Hinge->SetRelativeLocation(FVector(-45.f, 0.f, 22.f));
}

void ASpearfishGiantClam::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpearfishGiantClam, bOpen);
	DOREPLIFETIME(ASpearfishGiantClam, PhaseEndTime);
	DOREPLIFETIME(ASpearfishGiantClam, PearlId);
}

void ASpearfishGiantClam::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		const float Roll = FMath::FRand();
		if (Roll < SpearfishClamPrivate::PearlChance)
		{
			PearlId = FMath::FRand() < SpearfishClamPrivate::BlackPearlChance ? FName(TEXT("BlackPearl")) : FName(TEXT("WhitePearl"));
		}
		// Desynchronise neighbouring clams.
		bOpen = FMath::RandBool();
		const ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
		const float Now = GameState ? GameState->GetServerTime() : GetWorld()->GetTimeSeconds();
		PhaseEndTime = Now + FMath::FRandRange(1.f, SpearfishClamPrivate::OpenSecondsMax);
	}
	TremblePhase = FMath::FRandRange(0.f, 5.f);
	BuildVisuals();
}

void ASpearfishGiantClam::BuildVisuals()
{
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	if (bVisualsBuilt || !Visuals)
	{
		return;
	}
	bVisualsBuilt = true;
	const FLinearColor Shell(0.62f, 0.6f, 0.55f);
	const FLinearColor Mantle(0.15f, 0.45f, 0.75f);

	// Lower valve with a fluted rim, the blue mantle inside, and the upper valve on the hinge.
	Visuals->AddPart(this, Root, ESpearfishShape::Sphere, FVector(0.f, 0.f, 12.f), FRotator::ZeroRotator, FVector(120.f, 90.f, 36.f), Shell, true);
	Visuals->AddPart(this, Root, ESpearfishShape::Sphere, FVector(5.f, 0.f, 26.f), FRotator::ZeroRotator, FVector(100.f, 72.f, 14.f), Mantle, false, 0.4f);
	for (int32 Rib = -2; Rib <= 2; ++Rib)
	{
		Visuals->AddPart(this, Root, ESpearfishShape::Cylinder, FVector(10.f, Rib * 16.f, 20.f), FRotator(90.f, 0.f, 0.f), FVector(10.f, 10.f, 100.f), Shell * 0.85f, false);
	}
	Visuals->AddPart(this, Hinge, ESpearfishShape::Sphere, FVector(45.f, 0.f, 4.f), FRotator::ZeroRotator, FVector(120.f, 90.f, 30.f), Shell * 0.95f, true);
	for (int32 Rib = -2; Rib <= 2; ++Rib)
	{
		Visuals->AddPart(this, Hinge, ESpearfishShape::Cylinder, FVector(55.f, Rib * 16.f, 10.f), FRotator(90.f, 0.f, 0.f), FVector(9.f, 9.f, 96.f), Shell * 0.8f, false);
	}
	PearlMesh = Visuals->AddPart(this, Root, ESpearfishShape::Sphere, FVector(10.f, 0.f, 34.f), FRotator::ZeroRotator, FVector(10.f, 10.f, 10.f),
		FLinearColor(0.95f, 0.95f, 0.92f), false, 0.8f);
}

float ASpearfishGiantClam::GetPhaseRemaining() const
{
	const ASpearfishGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASpearfishGameState>() : nullptr;
	const float Now = GameState ? GameState->GetServerTime() : 0.f;
	return PhaseEndTime - Now;
}

bool ASpearfishGiantClam::IsClosingSoon() const
{
	return bOpen && GetPhaseRemaining() < SpearfishClamPrivate::ClosingWarningSeconds;
}

void ASpearfishGiantClam::SetOpen(bool bInOpen)
{
	const ASpearfishGameState* GameState = GetWorld()->GetGameState<ASpearfishGameState>();
	const float Now = GameState ? GameState->GetServerTime() : GetWorld()->GetTimeSeconds();
	bOpen = bInOpen;
	PhaseEndTime = Now + (bOpen ? FMath::FRandRange(SpearfishClamPrivate::OpenSecondsMin, SpearfishClamPrivate::OpenSecondsMax)
		: FMath::FRandRange(SpearfishClamPrivate::ClosedSecondsMin, SpearfishClamPrivate::ClosedSecondsMax));
	ForceNetUpdate();
}

void ASpearfishGiantClam::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority() && GetPhaseRemaining() <= 0.f)
	{
		SetOpen(!bOpen);
		if (!bOpen)
		{
			if (USpearfishFishSubsystem* Fish = USpearfishFishSubsystem::Get(this))
			{
				Fish->ReportNoise(GetActorLocation(), 250.f);
			}
		}
	}

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// Opens slowly, snaps shut fast; trembles as a warning before closing.
	const float Target = bOpen ? 1.f : 0.f;
	const float Speed = bOpen ? 0.6f : 6.f;
	OpenAmount = FMath::FInterpConstantTo(OpenAmount, Target, DeltaSeconds, Speed);
	TremblePhase += DeltaSeconds * 38.f;
	const float Tremble = IsClosingSoon() ? FMath::Sin(TremblePhase) * 2.5f : 0.f;
	Hinge->SetRelativeRotation(FRotator(OpenAmount * SpearfishClamPrivate::OpenAngle + Tremble, 0.f, 0.f));
	if (PearlMesh)
	{
		PearlMesh->SetVisibility(HasPearl() && OpenAmount > 0.25f);
	}
}

bool ASpearfishGiantClam::CanInteract(const ASpearfishCharacter* Character) const
{
	return Character && !Character->IsBlackedOut() && HasPearl();
}

FText ASpearfishGiantClam::GetInteractPrompt(const ASpearfishCharacter* Character) const
{
	if (!bOpen)
	{
		return LOCTEXT("Shut", "Giant clam (shut - wait for it to open)");
	}
	return IsClosingSoon() ? LOCTEXT("Closing", "Take the pearl (it's closing!)") : LOCTEXT("Take", "Take the pearl");
}

void ASpearfishGiantClam::Interact(ASpearfishCharacter* Character)
{
	if (!HasAuthority() || !CanInteract(Character))
	{
		return;
	}
	if (!bOpen)
	{
		Character->NotifyOwner(LOCTEXT("ShutTight", "The clam is shut tight. Wait for it to open."), ESpearfishNoticeType::Info);
		return;
	}
	// Small latency allowance: the client saw the tremble a few frames late.
	if (GetPhaseRemaining() < SpearfishClamPrivate::ClosingWarningSeconds * 0.8f)
	{
		if (USpearfishOxygenComponent* Oxygen = Character->GetOxygen())
		{
			Oxygen->ApplyShock(SpearfishClamPrivate::SnapAirLoss, false);
		}
		SetOpen(false);
		Character->NotifyOwner(LOCTEXT("Snapped", "The clam snapped shut on your hand! You gasp and lose air."), ESpearfishNoticeType::Danger);
		return;
	}
	if (SpearfishCatch::CollectLoot(Character, PearlId, 1.f))
	{
		PearlId = NAME_None;
		SetOpen(false);
	}
}

#undef LOCTEXT_NAMESPACE
