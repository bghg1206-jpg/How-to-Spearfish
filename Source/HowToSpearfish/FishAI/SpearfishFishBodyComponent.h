#pragma once

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "Data/SpearfishDefinitions.h"
#include "SpearfishFishBodyComponent.generated.h"

class UStaticMeshComponent;

/**
 * Visual fish built from the species' body plan (or an art mesh when one is assigned) with procedural
 * swimming: tail beats scale with speed, panicking fish beat frantically, exhausted fish roll on their side,
 * eels undulate, rays flap, jellyfish pulse. Purely cosmetic and local on every machine.
 */
UCLASS(ClassGroup = (Spearfish))
class HOWTOSPEARFISH_API USpearfishFishBodyComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	USpearfishFishBodyComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Build(const FSpearfishFishSpeciesDef& Species, float LengthCm);

	/** 0..1 swim speed relative to burst speed, plus state flags for animation. */
	void SetMotion(float InSpeedFraction, bool bInPanicked, bool bInExhausted);

private:
	UStaticMeshComponent* AddPart(USceneComponent* Parent, int32 Shape, const FVector& Location, const FRotator& Rotation, const FVector& Size,
		const FLinearColor& Color, float Emissive = 0.f);

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> BodyPivot;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> TailPivot;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Segments;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Tentacles;

	ESpearfishBodyPlan Plan = ESpearfishBodyPlan::Oval;
	float Length = 40.f;
	float TailBeatHz = 2.f;
	float Phase = 0.f;
	float SpeedFraction = 0.f;
	float Roll = 0.f;
	bool bPanicked = false;
	bool bExhausted = false;
	bool bBuilt = false;
};
