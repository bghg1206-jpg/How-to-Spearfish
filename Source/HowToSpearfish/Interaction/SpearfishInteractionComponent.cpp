#include "Interaction/SpearfishInteractionComponent.h"

#include "CollisionShape.h"
#include "Diving/SpearfishCharacter.h"
#include "Engine/World.h"
#include "HowToSpearfish.h"
#include "Interaction/SpearfishInteractable.h"

USpearfishInteractionComponent::USpearfishInteractionComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
}

ASpearfishCharacter* USpearfishInteractionComponent::GetCharacter() const
{
	return Cast<ASpearfishCharacter>(GetOwner());
}

void USpearfishInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const ASpearfishCharacter* Character = GetCharacter();
	if (!Character || !Character->IsLocallyControlled())
	{
		return;
	}
	FocusedActor = FindFocus();
}

AActor* USpearfishInteractionComponent::FindFocus() const
{
	const ASpearfishCharacter* Character = GetCharacter();
	UWorld* World = GetWorld();
	if (!Character || !World || !Character->CanInteractNow())
	{
		return nullptr;
	}

	const FVector Start = Character->GetEyeLocation();
	const FVector End = Start + Character->GetAimDirection() * TraceDistance;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpearfishInteract), false, Character);
	FHitResult Hit;
	if (!World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Interact, FCollisionShape::MakeSphere(TraceRadius), Params))
	{
		return nullptr;
	}

	AActor* HitActor = Hit.GetActor();
	ISpearfishInteractable* Interactable = Cast<ISpearfishInteractable>(HitActor);
	if (!Interactable || !Interactable->CanInteract(Character))
	{
		return nullptr;
	}
	if (FVector::Dist(Start, Hit.ImpactPoint) > Interactable->GetInteractRange())
	{
		return nullptr;
	}
	return HitActor;
}

FText USpearfishInteractionComponent::GetFocusedPrompt() const
{
	const ISpearfishInteractable* Interactable = Cast<ISpearfishInteractable>(FocusedActor.Get());
	return Interactable ? Interactable->GetInteractPrompt(GetCharacter()) : FText::GetEmpty();
}

void USpearfishInteractionComponent::TryInteract()
{
	if (AActor* Target = FocusedActor.Get())
	{
		ServerInteract(Target);
	}
}

void USpearfishInteractionComponent::ServerInteract_Implementation(AActor* Target)
{
	ASpearfishCharacter* Character = GetCharacter();
	ISpearfishInteractable* Interactable = Cast<ISpearfishInteractable>(Target);
	if (!Character || !Interactable || !Character->CanInteractNow())
	{
		return;
	}

	// Generous server-side range check to absorb latency; still blocks interacting across the map.
	const float Distance = static_cast<float>(FVector::Dist(Character->GetEyeLocation(), Target->GetActorLocation()));
	if (Distance > Interactable->GetInteractRange() + 400.f)
	{
		UE_LOG(LogSpearfish, Verbose, TEXT("Interact rejected: %s too far (%.0f)"), *Target->GetName(), Distance);
		return;
	}
	if (!Interactable->CanInteract(Character))
	{
		return;
	}
	Interactable->Interact(Character);
}
