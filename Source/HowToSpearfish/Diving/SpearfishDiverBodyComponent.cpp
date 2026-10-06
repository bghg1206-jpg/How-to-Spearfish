#include "Diving/SpearfishDiverBodyComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/SpearfishOceanSubsystem.h"
#include "World/SpearfishVisualSubsystem.h"

namespace SpearfishDiverBodyPrivate
{
	constexpr int32 MaxBubbles = 48;
}

USpearfishDiverBodyComponent::USpearfishDiverBodyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.f;
}

void USpearfishDiverBodyComponent::BeginPlay()
{
	Super::BeginPlay();
	Build();
	ApplyAppearance(Appearance, AppearanceRole);
}

void USpearfishDiverBodyComponent::Build()
{
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	AActor* Owner = GetOwner();
	if (bBuilt || !Visuals || !Owner)
	{
		return;
	}
	bBuilt = true;

	Pivot = NewObject<USceneComponent>(Owner);
	Pivot->SetupAttachment(this);
	Pivot->RegisterComponent();

	const FLinearColor Skin(0.85f, 0.65f, 0.5f);
	auto Add = [this, Visuals, Owner](ESpearfishShape Shape, const FVector& Location, const FRotator& Rotation, const FVector& Size, const FLinearColor& Color)
	{
		UStaticMeshComponent* Part = Visuals->AddPart(Owner, Pivot, Shape, Location, Rotation, Size, Color, false);
		Part->SetOwnerNoSee(true);
		Part->SetCastShadow(true);
		return Part;
	};

	Torso = Add(ESpearfishShape::Cylinder, FVector(0, 0, 10), FRotator::ZeroRotator, FVector(38, 28, 62), FLinearColor(0.15f, 0.15f, 0.2f));
	Head = Add(ESpearfishShape::Sphere, FVector(0, 0, 58), FRotator::ZeroRotator, FVector(24, 24, 26), Skin);
	Mask = Add(ESpearfishShape::Cube, FVector(11, 0, 60), FRotator::ZeroRotator, FVector(8, 20, 10), FLinearColor(0.2f, 0.2f, 0.2f));
	Tank = Add(ESpearfishShape::Cylinder, FVector(-20, 0, 14), FRotator::ZeroRotator, FVector(18, 18, 56), FLinearColor(0.75f, 0.75f, 0.78f));
	LegLeft = Add(ESpearfishShape::Cylinder, FVector(0, -9, -48), FRotator::ZeroRotator, FVector(13, 13, 60), FLinearColor(0.15f, 0.15f, 0.2f));
	LegRight = Add(ESpearfishShape::Cylinder, FVector(0, 9, -48), FRotator::ZeroRotator, FVector(13, 13, 60), FLinearColor(0.15f, 0.15f, 0.2f));
	FinLeft = Add(ESpearfishShape::Cube, FVector(12, -9, -80), FRotator::ZeroRotator, FVector(34, 14, 3), FLinearColor(0.1f, 0.1f, 0.1f));
	FinRight = Add(ESpearfishShape::Cube, FVector(12, 9, -80), FRotator::ZeroRotator, FVector(34, 14, 3), FLinearColor(0.1f, 0.1f, 0.1f));
	Arms = Add(ESpearfishShape::Cube, FVector(16, 0, 26), FRotator::ZeroRotator, FVector(40, 46, 10), FLinearColor(0.15f, 0.15f, 0.2f));
	Gun = Add(ESpearfishShape::Cylinder, FVector(42, 10, 30), FRotator(90, 0, 0), FVector(5, 5, 80), FLinearColor(0.45f, 0.3f, 0.18f));
	CatchBag = Add(ESpearfishShape::Sphere, FVector(-4, 22, -14), FRotator::ZeroRotator, FVector(22, 18, 26), FLinearColor(0.3f, 0.5f, 0.3f));
	ChefHat = Add(ESpearfishShape::Cylinder, FVector(0, 0, 84), FRotator::ZeroRotator, FVector(22, 22, 30), FLinearColor::White);
	Apron = Add(ESpearfishShape::Cube, FVector(16, 0, -6), FRotator::ZeroRotator, FVector(4, 30, 50), FLinearColor(0.95f, 0.95f, 0.95f));

	Bubbles = NewObject<UInstancedStaticMeshComponent>(Owner);
	Bubbles->SetStaticMesh(Visuals->GetShape(ESpearfishShape::Sphere));
	Bubbles->SetMaterial(0, Visuals->GetColorMaterial(FLinearColor(0.85f, 0.95f, 1.f), 0.6f));
	Bubbles->SetUsingAbsoluteLocation(true);
	Bubbles->SetUsingAbsoluteRotation(true);
	Bubbles->SetUsingAbsoluteScale(true);
	Bubbles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bubbles->SetCastShadow(false);
	Bubbles->SetupAttachment(this);
	Bubbles->RegisterComponent();
	for (int32 Index = 0; Index < SpearfishDiverBodyPrivate::MaxBubbles; ++Index)
	{
		Bubbles->AddInstance(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::ZeroVector), true);
	}
}

void USpearfishDiverBodyComponent::SetPartColor(UStaticMeshComponent* Part, const FLinearColor& Color)
{
	if (Part)
	{
		if (USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this))
		{
			Part->SetMaterial(0, Visuals->GetColorMaterial(Color));
		}
	}
}

void USpearfishDiverBodyComponent::ApplyAppearance(const FSpearfishDiverStats& Stats, ESpearfishRole Role)
{
	Appearance = Stats;
	AppearanceRole = Role;
	if (!bBuilt)
	{
		return;
	}

	const bool bDiver = Role == ESpearfishRole::Diver;
	SetPartColor(Torso, Stats.SuitColor);
	SetPartColor(LegLeft, Stats.SuitColor);
	SetPartColor(LegRight, Stats.SuitColor);
	SetPartColor(Arms, Stats.SuitColor);
	SetPartColor(Mask, Stats.MaskColor);
	SetPartColor(Tank, Stats.TankColor);
	SetPartColor(FinLeft, Stats.FinsColor);
	SetPartColor(FinRight, Stats.FinsColor);
	SetPartColor(Gun, Stats.GunColor);
	SetPartColor(CatchBag, Stats.BagColor);

	// Higher tiers read as bigger, sleeker kit.
	Tank->SetRelativeScale3D(FVector(18, 18, 56 + Stats.TankTier * 7) / 100.0);
	const float FinLength = 34.f + Stats.FinsTier * 8.f;
	FinLeft->SetRelativeScale3D(FVector(FinLength, 14, 3) / 100.0);
	FinRight->SetRelativeScale3D(FVector(FinLength, 14, 3) / 100.0);
	Gun->SetRelativeScale3D(FVector(5, 5, 80 + Stats.GunTier * 14) / 100.0);

	Tank->SetVisibility(bDiver && Stats.bHasTank);
	FinLeft->SetVisibility(bDiver);
	FinRight->SetVisibility(bDiver);
	Mask->SetVisibility(true);
	Gun->SetVisibility(bDiver && Stats.bHasSpeargun);
	CatchBag->SetVisibility(bDiver);
	ChefHat->SetVisibility(Role == ESpearfishRole::Chef);
	Apron->SetVisibility(Role == ESpearfishRole::Chef);
}

void USpearfishDiverBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bBuilt)
	{
		return;
	}
	UpdatePose(DeltaTime);
	UpdateBubbles(DeltaTime);
}

void USpearfishDiverBodyComponent::UpdatePose(float DeltaTime)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Pivot)
	{
		return;
	}
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const bool bSwimming = Movement && Movement->MovementMode == MOVE_Flying;
	const float AimPitch = FMath::Clamp(static_cast<float>(FRotator::NormalizeAxis(Character->GetBaseAimRotation().Pitch)), -70.f, 70.f);
	const float TargetPitch = bSwimming ? -90.f + AimPitch * 0.8f : 0.f;
	CurrentPitch = FMath::FInterpTo(CurrentPitch, TargetPitch, DeltaTime, 4.f);
	Pivot->SetRelativeRotation(FRotator(CurrentPitch, 0.f, 0.f));

	const float Speed = Movement ? static_cast<float>(Movement->Velocity.Size()) : 0.f;
	KickPhase += DeltaTime * (bSwimming ? 2.f + Speed / 60.f : 0.f);
	const float Kick = bSwimming ? FMath::Sin(KickPhase) * FMath::Clamp(0.25f + Speed / 400.f, 0.f, 1.f) * 25.f : 0.f;
	FinLeft->SetRelativeRotation(FRotator(Kick, 0.f, 0.f));
	FinRight->SetRelativeRotation(FRotator(-Kick, 0.f, 0.f));
}

void USpearfishDiverBodyComponent::EmitBubbles(int32 Count)
{
	const FVector HeadLocation = Head ? Head->GetComponentLocation() : GetComponentLocation();
	for (int32 Index = 0; Index < Count && BubbleList.Num() < SpearfishDiverBodyPrivate::MaxBubbles; ++Index)
	{
		FBubble Bubble;
		Bubble.Location = HeadLocation + FVector(FMath::FRandRange(-6.f, 6.f), FMath::FRandRange(-6.f, 6.f), FMath::FRandRange(0.f, 10.f));
		Bubble.Speed = FMath::FRandRange(70.f, 120.f);
		Bubble.Phase = FMath::FRandRange(0.f, 6.28f);
		Bubble.Size = FMath::FRandRange(2.f, 5.5f);
		BubbleList.Add(Bubble);
	}
}

void USpearfishDiverBodyComponent::UpdateBubbles(float DeltaTime)
{
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	const float SeaLevel = Ocean ? Ocean->GetSeaLevel() : 0.f;
	const FVector HeadLocation = Head ? Head->GetComponentLocation() : GetComponentLocation();
	const bool bHeadUnderwater = static_cast<float>(HeadLocation.Z) < SeaLevel - 10.f;

	NextBreath -= DeltaTime;
	if (bHeadUnderwater && NextBreath <= 0.f)
	{
		EmitBubbles(FMath::RandRange(5, 8));
		NextBreath = FMath::FRandRange(3.f, 4.2f);
	}

	for (FBubble& Bubble : BubbleList)
	{
		Bubble.Phase += DeltaTime * 6.f;
		Bubble.Location += FVector(FMath::Sin(Bubble.Phase) * 12.f, FMath::Cos(Bubble.Phase * 0.7f) * 12.f, Bubble.Speed) * DeltaTime;
		Bubble.Size = FMath::Min(Bubble.Size + DeltaTime * 0.4f, 8.f);
	}
	BubbleList.RemoveAll([SeaLevel](const FBubble& Bubble) { return static_cast<float>(Bubble.Location.Z) > SeaLevel; });

	for (int32 Index = 0; Index < SpearfishDiverBodyPrivate::MaxBubbles; ++Index)
	{
		FTransform Transform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::ZeroVector);
		if (BubbleList.IsValidIndex(Index))
		{
			const FBubble& Bubble = BubbleList[Index];
			Transform = FTransform(FRotator::ZeroRotator, Bubble.Location, FVector(Bubble.Size / 100.f));
		}
		Bubbles->UpdateInstanceTransform(Index, Transform, true, Index == SpearfishDiverBodyPrivate::MaxBubbles - 1, true);
	}
}
