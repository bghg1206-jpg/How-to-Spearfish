#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpearfishCustomer.generated.h"

class ASpearfishBoat;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class ESpearfishGuestState : uint8
{
	Arriving,
	Seated,
	Leaving
};

/**
 * A restaurant guest. Walks from the gangway to a seat in boat space (simulated locally on every machine
 * from replicated state, attached to the boat so they ride along), orders when seated, shows patience
 * above their head, and leaves happy or grumpy.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishCustomer : public AActor
{
	GENERATED_BODY()

public:
	ASpearfishCustomer();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	/** Server: before FinishSpawning. */
	void InitGuest(FName InCustomerId, int32 InSeatIndex, int32 InGuestId, ASpearfishBoat* InBoat, int32 InSeed);

	/** Server. */
	void Leave(bool bInHappy);

	FName GetCustomerId() const { return CustomerId; }
	int32 GetSeatIndex() const { return SeatIndex; }
	int32 GetGuestId() const { return GuestId; }
	ESpearfishGuestState GetGuestState() const { return GuestState; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Boat();

	void BuildBody();
	void AttachToBoat();
	FVector GetTargetRelative() const;
	void UpdateMood();

	UPROPERTY(VisibleAnywhere, Category = "Guest")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Guest")
	TObjectPtr<UTextRenderComponent> MoodText;

	UPROPERTY(Replicated)
	FName CustomerId;

	UPROPERTY(Replicated)
	int32 SeatIndex = 0;

	UPROPERTY(Replicated)
	int32 GuestId = 0;

	UPROPERTY(Replicated)
	int32 Seed = 0;

	UPROPERTY(Replicated)
	ESpearfishGuestState GuestState = ESpearfishGuestState::Arriving;

	UPROPERTY(Replicated)
	bool bHappy = true;

	UPROPERTY(ReplicatedUsing = OnRep_Boat)
	TObjectPtr<ASpearfishBoat> Boat;

	FVector RelativeLocation = FVector::ZeroVector;
	bool bBuilt = false;
	bool bReportedSeated = false;
	float MoodTimer = 0.f;
};
