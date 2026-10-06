#include "Core/SpearfishGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HowToSpearfish.h"
#include "Kismet/GameplayStatics.h"
#include "SaveSystem/SpearfishSaveGame.h"

#define LOCTEXT_NAMESPACE "SpearfishGameInstance"

const TCHAR* USpearfishGameInstance::SessionMap = TEXT("/Engine/Maps/Entry");

void USpearfishGameInstance::Init()
{
	Super::Init();
	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &USpearfishGameInstance::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &USpearfishGameInstance::HandleTravelFailure);
	}
}

void USpearfishGameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}
	Super::Shutdown();
}

void USpearfishGameInstance::StartSolo(bool bNewCampaign)
{
	bLeaving = false;
	FString Options = TEXT("game=Spearfish?mode=solo");
	if (bNewCampaign)
	{
		Options += TEXT("?new=1");
	}
	UE_LOG(LogSpearfish, Log, TEXT("Starting solo session (%s)"), *Options);
	UGameplayStatics::OpenLevel(this, FName(SessionMap), true, Options);
}

void USpearfishGameInstance::HostCoop(bool bNewCampaign)
{
	bLeaving = false;
	FString Options = TEXT("listen?game=Spearfish?mode=coop");
	if (bNewCampaign)
	{
		Options += TEXT("?new=1");
	}
	UE_LOG(LogSpearfish, Log, TEXT("Hosting co-op session on port %d (%s)"), DefaultPort, *Options);
	UGameplayStatics::OpenLevel(this, FName(SessionMap), true, Options);
}

void USpearfishGameInstance::JoinGame(const FString& Address)
{
	FString Target = Address.TrimStartAndEnd();
	if (Target.IsEmpty())
	{
		Target = TEXT("127.0.0.1");
	}
	if (!Target.Contains(TEXT(":")))
	{
		Target += FString::Printf(TEXT(":%d"), DefaultPort);
	}
	bLeaving = false;
	UE_LOG(LogSpearfish, Log, TEXT("Joining %s"), *Target);
	if (APlayerController* Controller = GetFirstLocalPlayerController())
	{
		Controller->ClientTravel(Target, TRAVEL_Absolute);
	}
}

void USpearfishGameInstance::NotifyMenuReached()
{
	bLeaving = false;
}

void USpearfishGameInstance::LeaveToMenu()
{
	if (bLeaving)
	{
		return;
	}
	bLeaving = true;
	UGameplayStatics::OpenLevel(this, FName(SessionMap), true, TEXT("game=Menu"));
}

bool USpearfishGameInstance::HasSave(bool bSolo) const
{
	const USpearfishSaveSubsystem* Saves = GetSubsystem<USpearfishSaveSubsystem>();
	return Saves && Saves->HasSave(bSolo);
}

FText USpearfishGameInstance::ConsumeLastError()
{
	FText Error = LastError;
	LastError = FText::GetEmpty();
	return Error;
}

void USpearfishGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogSpearfish, Warning, TEXT("Network failure (%s): %s"), ENetworkFailure::ToString(FailureType), *ErrorString);
	if (FailureType == ENetworkFailure::ConnectionLost || FailureType == ENetworkFailure::ConnectionTimeout)
	{
		SetLastError(LOCTEXT("ConnectionLost", "The connection to your partner was lost."));
	}
	else
	{
		SetLastError(FText::Format(LOCTEXT("NetworkError", "Network error: {0}"), FText::FromString(ErrorString)));
	}
	// The engine itself sends a failed client back to the default map, which boots into the menu game mode.
	bLeaving = false;
}

void USpearfishGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogSpearfish, Warning, TEXT("Travel failure (%s): %s"), ETravelFailure::ToString(FailureType), *ErrorString);
	SetLastError(FText::Format(LOCTEXT("TravelError", "Could not join: {0}"), FText::FromString(ErrorString)));
	bLeaving = false;
}

#undef LOCTEXT_NAMESPACE
