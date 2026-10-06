#include "FishAI/SpearfishFishBodyComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "World/SpearfishVisualSubsystem.h"

USpearfishFishBodyComponent::USpearfishFishBodyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

UStaticMeshComponent* USpearfishFishBodyComponent::AddPart(USceneComponent* Parent, int32 Shape, const FVector& Location, const FRotator& Rotation,
	const FVector& Size, const FLinearColor& Color, float Emissive)
{
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	if (!Visuals || !GetOwner())
	{
		return nullptr;
	}
	UStaticMeshComponent* Part = Visuals->AddPart(GetOwner(), Parent ? Parent : this, static_cast<ESpearfishShape>(Shape), Location, Rotation, Size, Color, false, Emissive);
	Part->SetCastShadow(Length > 60.f);
	return Part;
}

void USpearfishFishBodyComponent::Build(const FSpearfishFishSpeciesDef& Species, float LengthCm)
{
	if (bBuilt || !GetOwner())
	{
		return;
	}
	bBuilt = true;
	Plan = Species.Visual.BodyPlan;
	Length = LengthCm;
	TailBeatHz = Species.Visual.TailBeatHz;

	BodyPivot = NewObject<USceneComponent>(GetOwner());
	BodyPivot->SetupAttachment(this);
	BodyPivot->RegisterComponent();

	// Art mesh path: one mesh, animation expected in the material (WPO).
	if (!Species.Visual.Mesh.IsNull())
	{
		if (UStaticMesh* Mesh = Species.Visual.Mesh.LoadSynchronous())
		{
			UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(GetOwner());
			Part->SetStaticMesh(Mesh);
			Part->SetupAttachment(BodyPivot);
			Part->SetRelativeScale3D(FVector(Length / 100.f));
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Part->RegisterComponent();
			return;
		}
	}

	const FLinearColor Primary = Species.Visual.PrimaryColor;
	const FLinearColor Secondary = Species.Visual.SecondaryColor;
	const FLinearColor Eye(0.02f, 0.02f, 0.02f);
	const float L = Length;
	const float D = L * Species.Visual.BodyDepth;
	const float W = L * Species.Visual.BodyWidth;
	const int32 Cube = static_cast<int32>(ESpearfishShape::Cube);
	const int32 Sphere = static_cast<int32>(ESpearfishShape::Sphere);
	const int32 Cylinder = static_cast<int32>(ESpearfishShape::Cylinder);
	const int32 Cone = static_cast<int32>(ESpearfishShape::Cone);

	auto AddTail = [&](float Offset, float TailSize, const FLinearColor& Color)
	{
		TailPivot = NewObject<USceneComponent>(GetOwner());
		TailPivot->SetupAttachment(BodyPivot);
		TailPivot->SetRelativeLocation(FVector(-Offset, 0, 0));
		TailPivot->RegisterComponent();
		AddPart(TailPivot, Cube, FVector(-TailSize * 0.4f, 0, 0), FRotator::ZeroRotator, FVector(TailSize * 0.8f, 1.5f, TailSize), Color);
	};
	auto AddEyes = [&](float X, float Z, float Spread)
	{
		const float Size = FMath::Max(L * 0.05f, 1.2f);
		AddPart(BodyPivot, Sphere, FVector(X, Spread, Z), FRotator::ZeroRotator, FVector(Size), Eye);
		AddPart(BodyPivot, Sphere, FVector(X, -Spread, Z), FRotator::ZeroRotator, FVector(Size), Eye);
	};

	switch (Plan)
	{
	case ESpearfishBodyPlan::Torpedo:
	case ESpearfishBodyPlan::Oval:
	case ESpearfishBodyPlan::Bulky:
	{
		const float BodyLength = L * (Plan == ESpearfishBodyPlan::Bulky ? 0.82f : 0.78f);
		AddPart(BodyPivot, Sphere, FVector::ZeroVector, FRotator::ZeroRotator, FVector(BodyLength, W, D), Primary);
		// Belly / stripe in the secondary colour.
		AddPart(BodyPivot, Sphere, FVector(L * 0.02f, 0, -D * 0.12f), FRotator::ZeroRotator, FVector(BodyLength * 0.85f, W * 0.9f, D * 0.6f), Secondary);
		AddPart(BodyPivot, Cube, FVector(-L * 0.02f, 0, D * 0.46f), FRotator(-8, 0, 0), FVector(L * 0.32f, 1.2f, D * 0.32f), Secondary);
		if (Plan == ESpearfishBodyPlan::Bulky)
		{
			AddPart(BodyPivot, Cube, FVector(L * 0.36f, 0, -D * 0.05f), FRotator::ZeroRotator, FVector(L * 0.1f, W * 0.9f, D * 0.45f), Primary * 0.8f);
		}
		AddTail(BodyLength * 0.45f, FMath::Max(D * 0.9f, 4.f), Plan == ESpearfishBodyPlan::Torpedo ? Secondary : Primary);
		AddEyes(L * 0.3f, D * 0.1f, W * 0.42f);
		break;
	}
	case ESpearfishBodyPlan::Spiny:
	{
		AddPart(BodyPivot, Sphere, FVector::ZeroVector, FRotator::ZeroRotator, FVector(L * 0.7f, W, D), Primary);
		for (int32 Stripe = -2; Stripe <= 2; ++Stripe)
		{
			AddPart(BodyPivot, Cube, FVector(Stripe * L * 0.12f, 0, 0), FRotator::ZeroRotator, FVector(L * 0.04f, W * 1.05f, D * 1.02f), Secondary);
		}
		// Venomous spines and fan-like pectoral fins.
		for (int32 Spine = 0; Spine < 7; ++Spine)
		{
			const float X = (Spine - 3) * L * 0.07f;
			AddPart(BodyPivot, Cylinder, FVector(X, 0, D * 0.75f), FRotator(0, 0, (Spine - 3) * 6.f), FVector(0.6f, 0.6f, D * 0.9f), Secondary);
		}
		AddPart(BodyPivot, Cube, FVector(L * 0.05f, W * 0.9f, -D * 0.1f), FRotator(0, 0, 50), FVector(L * 0.35f, 1.f, D * 0.8f), Primary);
		AddPart(BodyPivot, Cube, FVector(L * 0.05f, -W * 0.9f, -D * 0.1f), FRotator(0, 0, -50), FVector(L * 0.35f, 1.f, D * 0.8f), Primary);
		AddTail(L * 0.33f, D * 0.6f, Secondary);
		AddEyes(L * 0.26f, D * 0.15f, W * 0.42f);
		break;
	}
	case ESpearfishBodyPlan::Eel:
	{
		// Chain of segments for an undulating body.
		const int32 Count = 6;
		USceneComponent* Parent = BodyPivot;
		const float SegmentLength = L / Count;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			USceneComponent* Joint = NewObject<USceneComponent>(GetOwner());
			Joint->SetupAttachment(Parent);
			Joint->SetRelativeLocation(FVector(Index == 0 ? L * 0.4f : -SegmentLength, 0, 0));
			Joint->RegisterComponent();
			const float Taper = 1.f - 0.1f * Index;
			AddPart(Joint, Sphere, FVector(-SegmentLength * 0.5f, 0, 0), FRotator::ZeroRotator, FVector(SegmentLength * 1.3f, W * Taper, D * Taper), Index % 2 == 0 ? Primary : Primary * 0.85f);
			Segments.Add(Joint);
			Parent = Joint;
		}
		AddEyes(L * 0.42f, D * 0.2f, W * 0.35f);
		break;
	}
	case ESpearfishBodyPlan::Disc:
	{
		AddPart(BodyPivot, Sphere, FVector::ZeroVector, FRotator::ZeroRotator, FVector(L * 0.75f, L * 0.7f, D), Primary);
		AddPart(BodyPivot, Sphere, FVector(0, 0, -D * 0.2f), FRotator::ZeroRotator, FVector(L * 0.7f, L * 0.65f, D * 0.6f), Secondary);
		TailPivot = NewObject<USceneComponent>(GetOwner());
		TailPivot->SetupAttachment(BodyPivot);
		TailPivot->SetRelativeLocation(FVector(-L * 0.35f, 0, 0));
		TailPivot->RegisterComponent();
		AddPart(TailPivot, Cylinder, FVector(-L * 0.3f, 0, 0), FRotator(90, 0, 0), FVector(1.5f, 1.5f, L * 0.6f), Primary * 0.7f);
		AddEyes(L * 0.2f, D * 0.5f, L * 0.08f);
		break;
	}
	case ESpearfishBodyPlan::Bell:
	{
		AddPart(BodyPivot, Sphere, FVector::ZeroVector, FRotator::ZeroRotator, FVector(L, L, L * 0.55f), Primary, 1.2f);
		AddPart(BodyPivot, Sphere, FVector(0, 0, -L * 0.05f), FRotator::ZeroRotator, FVector(L * 0.5f, L * 0.5f, L * 0.3f), Secondary, 2.f);
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const float Angle = Index * 60.f;
			USceneComponent* Joint = NewObject<USceneComponent>(GetOwner());
			Joint->SetupAttachment(BodyPivot);
			Joint->SetRelativeLocation(FRotator(0, Angle, 0).RotateVector(FVector(L * 0.3f, 0, -L * 0.15f)));
			Joint->RegisterComponent();
			AddPart(Joint, Cylinder, FVector(0, 0, -L * 0.6f), FRotator::ZeroRotator, FVector(0.8f, 0.8f, L * 1.2f), Secondary, 1.f);
			Tentacles.Add(Joint);
		}
		break;
	}
	case ESpearfishBodyPlan::Shark:
	{
		AddPart(BodyPivot, Sphere, FVector::ZeroVector, FRotator::ZeroRotator, FVector(L * 0.8f, W, D), Primary);
		AddPart(BodyPivot, Sphere, FVector(L * 0.05f, 0, -D * 0.15f), FRotator::ZeroRotator, FVector(L * 0.7f, W * 0.85f, D * 0.6f), Secondary);
		AddPart(BodyPivot, Cone, FVector(L * 0.02f, 0, D * 0.6f), FRotator(0, 0, 0), FVector(L * 0.14f, 1.5f, D * 0.9f), Primary * 0.8f);
		AddPart(BodyPivot, Cube, FVector(L * 0.12f, W * 0.7f, -D * 0.25f), FRotator(-10, -25, -25), FVector(L * 0.2f, L * 0.1f, 1.5f), Primary);
		AddPart(BodyPivot, Cube, FVector(L * 0.12f, -W * 0.7f, -D * 0.25f), FRotator(-10, 25, 25), FVector(L * 0.2f, L * 0.1f, 1.5f), Primary);
		TailPivot = NewObject<USceneComponent>(GetOwner());
		TailPivot->SetupAttachment(BodyPivot);
		TailPivot->SetRelativeLocation(FVector(-L * 0.38f, 0, 0));
		TailPivot->RegisterComponent();
		AddPart(TailPivot, Cube, FVector(-L * 0.06f, 0, D * 0.45f), FRotator(-35, 0, 0), FVector(L * 0.18f, 1.5f, D * 0.9f), Primary * 0.7f);
		AddPart(TailPivot, Cube, FVector(-L * 0.05f, 0, -D * 0.3f), FRotator(30, 0, 0), FVector(L * 0.12f, 1.5f, D * 0.6f), Primary * 0.7f);
		AddEyes(L * 0.3f, D * 0.1f, W * 0.4f);
		break;
	}
	default:
		break;
	}
}

void USpearfishFishBodyComponent::SetMotion(float InSpeedFraction, bool bInPanicked, bool bInExhausted)
{
	SpeedFraction = FMath::Clamp(InSpeedFraction, 0.f, 1.f);
	bPanicked = bInPanicked;
	bExhausted = bInExhausted;
}

void USpearfishFishBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bBuilt || !BodyPivot)
	{
		return;
	}

	const float Effort = bExhausted ? 0.2f : (bPanicked ? 2.4f : 0.6f + SpeedFraction * 1.4f);
	Phase += DeltaTime * TailBeatHz * Effort * UE_TWO_PI;
	const float Wave = FMath::Sin(Phase);

	// Exhausted fish list onto their side.
	Roll = FMath::FInterpTo(Roll, bExhausted ? 75.f : 0.f, DeltaTime, 2.f);

	switch (Plan)
	{
	case ESpearfishBodyPlan::Eel:
		for (int32 Index = 0; Index < Segments.Num(); ++Index)
		{
			Segments[Index]->SetRelativeRotation(FRotator(0.f, FMath::Sin(Phase - Index * 0.9f) * (8.f + Index * 3.f) * (bExhausted ? 0.3f : 1.f), 0.f));
		}
		BodyPivot->SetRelativeRotation(FRotator(0.f, 0.f, Roll));
		break;
	case ESpearfishBodyPlan::Disc:
		BodyPivot->SetRelativeRotation(FRotator(Wave * 4.f, 0.f, Wave * 8.f + Roll * 0.3f));
		if (TailPivot)
		{
			TailPivot->SetRelativeRotation(FRotator(0.f, FMath::Sin(Phase * 0.5f) * 15.f, 0.f));
		}
		break;
	case ESpearfishBodyPlan::Bell:
	{
		const float Pulse = 1.f + 0.12f * FMath::Sin(Phase * 0.7f);
		BodyPivot->SetRelativeScale3D(FVector(Pulse, Pulse, 2.f - Pulse));
		for (int32 Index = 0; Index < Tentacles.Num(); ++Index)
		{
			Tentacles[Index]->SetRelativeRotation(FRotator(FMath::Sin(Phase * 0.5f + Index) * 12.f, 0.f, FMath::Cos(Phase * 0.4f + Index) * 12.f));
		}
		break;
	}
	default:
		BodyPivot->SetRelativeRotation(FRotator(0.f, -Wave * 4.f * Effort, Roll));
		if (TailPivot)
		{
			TailPivot->SetRelativeRotation(FRotator(0.f, Wave * 28.f * FMath::Min(Effort, 1.5f), 0.f));
		}
		break;
	}
}
