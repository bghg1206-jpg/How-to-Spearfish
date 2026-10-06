#pragma once

#include "Core/SpearfishGameTypes.h"
#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GameFramework/Actor.h"
#include "SpearfishBoat.generated.h"

class ASpearfishCharacter;
class ASpearfishStation;
class UPointLightComponent;
class USpearfishAutoChefComponent;
class USpearfishCatchStorageComponent;
class USpearfishRestaurantComponent;

USTRUCT()
struct FSpearfishBoatNetState
{
	GENERATED_BODY()

	UPROPERTY()
	FVector_NetQuantize10 Location;

	UPROPERTY()
	float Yaw = 0.f;

	UPROPERTY()
	float Speed = 0.f;
};

/**
 * The floating restaurant and dive boat: kitchen, dining deck, bunks, helm and dive ladder in one hull.
 * Kinematic, server-driven (driver input or autopilot), replicated as a compact state and smoothed on
 * clients so passengers ride steadily. Guests only come aboard while anchored. A lit dive flag and a
 * weighted descent line make the boat easy to find from below.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishBoat : public AActor
{
	GENERATED_BODY()

public:
	ASpearfishBoat();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	// --- Layout (world transforms) --------------------------------------------------------------
	USceneComponent* GetHelmSeat() const { return HelmSeat; }
	FTransform GetHelmExitTransform() const;
	FTransform GetLadderTopTransform() const;
	FTransform GetLadderWaterTransform() const;
	FTransform GetDeckSpawnTransform(int32 SeatIndex) const;
	FTransform GetCustomerSeatTransform(int32 SeatIndex) const;
	FVector GetBoardingPoint() const;
	/** Relative (boat space) helpers for customers walking around the deck. */
	FVector GetRelativeBoardingPoint() const;
	FVector GetRelativeSeatPoint(int32 SeatIndex) const;
	int32 GetMaxSeats() const { return 8; }

	// --- Components -----------------------------------------------------------------------------
	USpearfishCatchStorageComponent* GetCooler() const { return Cooler; }
	USpearfishRestaurantComponent* GetRestaurant() const { return Restaurant; }
	USpearfishAutoChefComponent* GetAutoChef() const { return AutoChef; }
	const TArray<TObjectPtr<ASpearfishStation>>& GetStations() const { return Stations; }
	ASpearfishStation* FindStation(ESpearfishStationType Type, int32 Index = 0) const;
	ASpearfishStation* FindBedOccupiedBy(const ASpearfishCharacter* Character) const;

	// --- Driving (server) -----------------------------------------------------------------------
	void SetDriver(ASpearfishCharacter* InDriver);
	ASpearfishCharacter* GetDriver() const { return Driver; }
	void SetDriverInput(float InThrottle, float InSteer);
	bool IsAnchored() const { return bAnchored; }
	void SetAnchored(bool bInAnchored);
	float GetSpeed() const { return NetState.Speed; }
	bool IsMoving() const { return FMath::Abs(NetState.Speed) > 40.f; }
	/** Sail to a point and drop anchor there (solo "bring the boat", auto-captain). */
	void SetAutopilotTarget(const FVector& Target);
	bool HasAutopilot() const { return bAutopilot; }
	void PlaceAt(const FVector& Location, float Yaw);

	/** Server: spawns stations. Called once by the game mode after spawning the boat. */
	void SpawnStations();

protected:
	virtual void BeginPlay() override;

private:
	void BuildHull();
	void SimulateServer(float DeltaSeconds);
	void SmoothClient(float DeltaSeconds);
	ASpearfishStation* SpawnStation(ESpearfishStationType Type, int32 Index, const FVector& RelativeLocation, float RelativeYaw);

	UPROPERTY(VisibleAnywhere, Category = "Boat")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Boat")
	TObjectPtr<USceneComponent> HelmSeat;

	UPROPERTY(VisibleAnywhere, Category = "Boat")
	TObjectPtr<UPointLightComponent> MastLight;

	UPROPERTY(VisibleAnywhere, Category = "Boat")
	TObjectPtr<USpearfishCatchStorageComponent> Cooler;

	UPROPERTY(VisibleAnywhere, Category = "Boat")
	TObjectPtr<USpearfishRestaurantComponent> Restaurant;

	UPROPERTY(VisibleAnywhere, Category = "Boat")
	TObjectPtr<USpearfishAutoChefComponent> AutoChef;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> DescentLine;

	UPROPERTY(Replicated)
	FSpearfishBoatNetState NetState;

	UPROPERTY(Replicated)
	bool bAnchored = true;

	UPROPERTY(Replicated)
	TObjectPtr<ASpearfishCharacter> Driver;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<ASpearfishStation>> Stations;

	float Throttle = 0.f;
	float Steer = 0.f;
	float Speed = 0.f;
	float Yaw = 0.f;
	bool bAutopilot = false;
	FVector AutopilotTarget = FVector::ZeroVector;
	float ShallowWarningCooldown = 0.f;
	bool bHullBuilt = false;
};
