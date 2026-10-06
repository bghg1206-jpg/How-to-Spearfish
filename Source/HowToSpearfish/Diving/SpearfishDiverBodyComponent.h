#pragma once

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "Equipment/SpearfishEquipmentComponent.h"
#include "SpearfishDiverBodyComponent.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

/**
 * Third-person body seen by the other player: wetsuit, tank, mask, fins, bag and speargun are separate
 * parts coloured by the equipped tier, so upgrades are visible on the character. Chefs get an apron and a
 * tall hat. Also emits exhale bubbles (visible to everyone - handy for spotting your diver from the boat).
 * Placeholder geometry; swap for a skeletal mesh when art exists.
 */
UCLASS(ClassGroup = (Spearfish))
class HOWTOSPEARFISH_API USpearfishDiverBodyComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	USpearfishDiverBodyComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ApplyAppearance(const FSpearfishDiverStats& Stats, ESpearfishRole Role);

	/** Breath out a burst of bubbles now (also happens automatically underwater). */
	void EmitBubbles(int32 Count);

private:
	struct FBubble
	{
		FVector Location = FVector::ZeroVector;
		float Speed = 60.f;
		float Phase = 0.f;
		float Size = 3.f;
	};

	void Build();
	void SetPartColor(UStaticMeshComponent* Part, const FLinearColor& Color);
	void UpdatePose(float DeltaTime);
	void UpdateBubbles(float DeltaTime);

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Torso;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Mask;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Tank;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> LegLeft;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> LegRight;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> FinLeft;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> FinRight;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Arms;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Gun;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> CatchBag;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ChefHat;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Apron;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Bubbles;

	TArray<FBubble> BubbleList;
	FSpearfishDiverStats Appearance;
	ESpearfishRole AppearanceRole = ESpearfishRole::None;
	float NextBreath = 0.f;
	float KickPhase = 0.f;
	float CurrentPitch = 0.f;
	bool bBuilt = false;
};
