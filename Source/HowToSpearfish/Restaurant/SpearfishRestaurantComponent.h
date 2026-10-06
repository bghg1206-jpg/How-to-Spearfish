#pragma once

#include "Components/ActorComponent.h"
#include "Core/SpearfishGameTypes.h"
#include "CoreMinimal.h"
#include "Orders/SpearfishOrderTypes.h"
#include "SpearfishRestaurantComponent.generated.h"

class APlayerState;
class ASpearfishBoat;
class ASpearfishCharacter;
class ASpearfishCustomer;
class ASpearfishStation;
struct FSpearfishCustomerDef;
struct FSpearfishRecipeDef;

DECLARE_MULTICAST_DELEGATE_OneParam(FSpearfishOrderEvent, const FSpearfishOrder& /*Order*/);

/** Aggregated "the kitchen still needs..." line (solo radio board, tablet summary). */
USTRUCT(BlueprintType)
struct FSpearfishKitchenNeed
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Kitchen")
	FName SpeciesId;

	UPROPERTY(BlueprintReadOnly, Category = "Kitchen")
	FName Category;

	UPROPERTY(BlueprintReadOnly, Category = "Kitchen")
	int32 Count = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Kitchen")
	float MinLengthCm = 0.f;
};

/**
 * The floating restaurant: guests arrive while the boat is anchored and the sign says OPEN, order dishes
 * from the unlocked menu (biased to their tastes and to fish that live in this region), and wait with
 * draining patience. Cooking is a chain of station minigames per dish; serving pays out with quality,
 * speed and tip, and moves reputation. Everything is server-authoritative; the arrays replicate for the
 * tablet and HUD. Order and dish ids are single-use, so a dish can never be paid twice.
 */
UCLASS(ClassGroup = (Spearfish), meta = (BlueprintSpawnableComponent))
class HOWTOSPEARFISH_API USpearfishRestaurantComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpearfishRestaurantComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- State ----------------------------------------------------------------------------------
	bool IsOpen() const { return bOpen; }
	const TArray<FSpearfishOrder>& GetOrders() const { return Orders; }
	const TArray<FSpearfishDish>& GetDishes() const { return Dishes; }
	const TArray<FSpearfishKitchenNeed>& GetKitchenNeeds() const { return KitchenNeeds; }
	const FSpearfishOrder* FindOrder(int32 OrderId) const;
	const FSpearfishDish* FindDish(int32 DishId) const;
	int32 GetSeatCount() const;
	int32 CountActiveGuests() const;
	float GetPatienceRemaining(const FSpearfishOrder& Order) const;
	float GetPatienceFraction(const FSpearfishOrder& Order) const;
	bool CanStartOrder(const FSpearfishOrder& Order) const;
	TArray<int32> GetMissingCounts(const FSpearfishOrder& Order) const;

	FText DescribeStationWork(const ASpearfishCharacter* Character, ESpearfishStationType Station) const;
	FText DescribeServe() const;

	// --- Server API -------------------------------------------------------------------------------
	void SetOpen(bool bInOpen);
	bool TryStartDish(int32 OrderId, APlayerState* By, bool bAuto, FText& OutReason);
	bool CancelDish(int32 DishId);
	bool TryBeginCookStep(ASpearfishCharacter* Cook, ASpearfishStation* Station);
	bool SubmitCookStep(int32 DishId, int32 StepIndex, float Score, APlayerState* By, bool bAuto);
	void AbortCookStep(int32 DishId, APlayerState* By);
	bool TryServe(ASpearfishCharacter* Server);
	bool ServeDish(int32 DishId, bool bAuto);
	void SetAutoStepRemaining(int32 DishId, float Seconds);
	void TickAutoStep(int32 DishId, float DeltaSeconds, float Skill);

	void BeginDay(int32 Day, int32 Seed);
	/** Night: no more guests; waiting guests leave (half penalty). */
	void CloseForNight();

	void OnGuestSeated(ASpearfishCustomer* Guest);
	void OnGuestLeft(ASpearfishCustomer* Guest);

	/** Server: fired whenever a guest places an order (auto-chef radio). */
	FSpearfishOrderEvent OnOrderPlaced;

private:
	ASpearfishBoat* GetBoat() const;
	float ServerNow() const;
	void TickArrivals(float DeltaTime);
	void TickPatience();
	void UpdateKitchenNeeds();
	void SpawnGuest();
	const FSpearfishCustomerDef* PickCustomer();
	FName PickRecipe(const FSpearfishCustomerDef& Customer);
	bool IsRecipeAvailable(const FSpearfishRecipeDef& Recipe) const;
	int32 FindFreeSeat() const;
	void ExpireOrder(FSpearfishOrder& Order, float PenaltyScale);
	void ReleaseGuestIfDone(int32 GuestId, bool bHappy);
	FSpearfishDish* FindDishMutable(int32 DishId);
	FSpearfishOrder* FindOrderMutable(int32 OrderId);

	UPROPERTY(Replicated)
	bool bOpen = false;

	UPROPERTY(Replicated)
	TArray<FSpearfishOrder> Orders;

	UPROPERTY(Replicated)
	TArray<FSpearfishDish> Dishes;

	UPROPERTY(Replicated)
	TArray<FSpearfishKitchenNeed> KitchenNeeds;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASpearfishCustomer>> Guests;

	int32 NextOrderId = 1;
	int32 NextDishId = 1;
	int32 NextGuestId = 1;
	float NextArrivalTime = 0.f;
	float NeedsTimer = 0.f;
	bool bCriticPending = false;
	FRandomStream Stream;
};
