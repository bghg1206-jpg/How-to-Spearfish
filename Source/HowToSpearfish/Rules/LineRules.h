#pragma once

#include "CoreMinimal.h"
#include "LineRules.generated.h"

/**
 * Harpoon line model. The line behaves like a reel with drag:
 *  - when the hooked thing pulls harder than the drag, line pays out (up to the reel's max length);
 *  - otherwise reeling winds line in, slower under load;
 *  - sustained tension above the break force at full length snaps the line.
 * Tension also drains fish stamina and pulls the diver, so big fish tow divers around.
 */
USTRUCT(BlueprintType)
struct FSpearfishLineSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line")
	float MaxLengthCm = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line")
	float MinLengthCm = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line")
	float ReelSpeedCm = 160.f;

	/** Extra tension the reel adds while winding against a taut line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line")
	float ReelForce = 160.f;

	/** Fish pull above this pays line out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line")
	float DragForce = 220.f;

	/** Sustained tension above this at full length snaps the line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line")
	float BreakForce = 420.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line")
	float PayoutSpeedCm = 280.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line")
	float SnapGraceSeconds = 0.8f;
};

USTRUCT(BlueprintType)
struct FSpearfishLineState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Line")
	float LengthCm = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Line")
	float Tension = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Line")
	float OverloadSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Line")
	bool bTaut = false;

	UPROPERTY(BlueprintReadOnly, Category = "Line")
	bool bSnapped = false;
};

struct FSpearfishLineInput
{
	/** Path length along the line (including wrap points) from the gun to the harpoon. */
	float DistanceCm = 0.f;
	/** Force the hooked thing exerts away from the diver (0 when it swims toward the diver). */
	float PullAwayForce = 0.f;
	bool bReeling = false;
	float DeltaSeconds = 0.f;
};

namespace SpearfishLine
{
	constexpr float TautSlackCm = 8.f;
	constexpr float InstantSnapMultiplier = 1.6f;

	void Attach(FSpearfishLineState& State, float InitialDistanceCm, const FSpearfishLineSpec& Spec);

	void Step(FSpearfishLineState& State, const FSpearfishLineInput& In, const FSpearfishLineSpec& Spec);

	/** 0..1 for UI: how close the line is to breaking. */
	float StressFraction(const FSpearfishLineState& State, const FSpearfishLineSpec& Spec);

	/** Acceleration (cm/s^2) the line applies to the diver for the current tension. */
	float DiverPullAcceleration(float Tension, float PullScale);
}
