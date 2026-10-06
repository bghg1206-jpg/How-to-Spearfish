#pragma once

#include "CoreMinimal.h"
#include "Rules/SpearfishRulesTypes.h"
#include "UObject/ObjectPtr.h"
#include "SpearfishOrderTypes.generated.h"

class APlayerState;

/** One dish a guest is waiting for. Patience is a server-time deadline so clients can show live bars. */
USTRUCT(BlueprintType)
struct FSpearfishOrder
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Order")
	int32 OrderId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Order")
	int32 GuestId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Order")
	int32 SeatIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Order")
	FName RecipeId;

	UPROPERTY(BlueprintReadOnly, Category = "Order")
	FName CustomerId;

	UPROPERTY(BlueprintReadOnly, Category = "Order")
	ESpearfishOrderState State = ESpearfishOrderState::Waiting;

	UPROPERTY(BlueprintReadOnly, Category = "Order")
	float PatienceTotal = 120.f;

	/** Server time (GameState) when the guest gives up. */
	UPROPERTY(BlueprintReadOnly, Category = "Order")
	float PatienceDeadline = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Order")
	int32 DishId = INDEX_NONE;
};

UENUM(BlueprintType)
enum class ESpearfishDishState : uint8
{
	Prep,
	Ready
};

/** A dish in the kitchen: ingredients already taken from the cooler, steps done so far. */
USTRUCT(BlueprintType)
struct FSpearfishDish
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dish")
	int32 DishId = 0;

	/** INDEX_NONE when the guest left; the dish can still go to another guest who ordered it. */
	UPROPERTY(BlueprintReadOnly, Category = "Dish")
	int32 OrderId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Dish")
	FName RecipeId;

	UPROPERTY(BlueprintReadOnly, Category = "Dish")
	int32 CurrentStep = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dish")
	TArray<float> StepScores;

	UPROPERTY(BlueprintReadOnly, Category = "Dish")
	float IngredientQuality = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Dish")
	ESpearfishDishState State = ESpearfishDishState::Prep;

	/** Player currently working a step (others cannot grab the same step). */
	UPROPERTY(BlueprintReadOnly, Category = "Dish")
	TObjectPtr<APlayerState> LockedBy;

	UPROPERTY(BlueprintReadOnly, Category = "Dish")
	float LockExpires = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Dish")
	bool bAutoChef = false;

	/** Server only: the fish taken from the cooler (returned on cancel). */
	UPROPERTY(NotReplicated)
	TArray<FSpearfishItem> Ingredients;

	UPROPERTY(NotReplicated)
	float AutoStepRemaining = 0.f;
};
