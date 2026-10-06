#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SpearfishGameInstance.generated.h"

class UNetDriver;

/**
 * Session flow without any map assets: every session runs on /Engine/Maps/Entry and the game mode alias
 * picks menu or gameplay. Solo is a standalone world; co-op is a listen server (direct IP for the slice,
 * Steam/EOS sessions can replace JoinGame later). Network and travel failures land back in the menu with a
 * readable message instead of a hang.
 */
UCLASS()
class HOWTOSPEARFISH_API USpearfishGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	static const TCHAR* SessionMap;
	static constexpr int32 DefaultPort = 7777;

	void StartSolo(bool bNewCampaign);
	void HostCoop(bool bNewCampaign);
	/** Address is "host" or "host:port". */
	void JoinGame(const FString& Address);
	void LeaveToMenu();
	/** Called by the menu game mode once the menu world is up. */
	void NotifyMenuReached();

	bool HasSave(bool bSolo) const;

	/** Last network/travel error, shown once by the main menu. */
	FText ConsumeLastError();
	void SetLastError(const FText& Error) { LastError = Error; }

private:
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	FText LastError;
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
	bool bLeaving = false;
};
