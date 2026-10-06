#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "Equipment/SpearfishEquipmentComponent.h"
#include "Rules/LineRules.h"
#include "SpeargunComponent.generated.h"

class ASpearfishCharacter;
class ASpearfishFish;
class ASpearfishHarpoon;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ESpeargunState : uint8
{
	Unarmed,
	Loaded,
	Flying,
	Attached,
	Retrieving,
	Reloading
};

UENUM(BlueprintType)
enum class ESpearfishLineTarget : uint8
{
	None,
	Fish,
	Character,
	Physics,
	World
};

/** Everything clients need to draw the line and show tension; replicated from the server. */
USTRUCT(BlueprintType)
struct FSpearfishLineNetState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Line")
	ESpeargunState State = ESpeargunState::Unarmed;

	UPROPERTY(BlueprintReadOnly, Category = "Line")
	ESpearfishLineTarget Target = ESpearfishLineTarget::None;

	UPROPERTY(BlueprintReadOnly, Category = "Line")
	TObjectPtr<AActor> TargetActor;

	/** Points where the line bends around rocks, coral and wrecks (gun side first). */
	UPROPERTY(BlueprintReadOnly, Category = "Line")
	TArray<FVector_NetQuantize10> WrapPoints;

	UPROPERTY(BlueprintReadOnly, Category = "Line")
	float LengthCm = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Line")
	float Tension = 0.f;

	/** 0..1 how close the line is to snapping. */
	UPROPERTY(BlueprintReadOnly, Category = "Line")
	float Stress = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Line")
	bool bTaut = false;

	UPROPERTY(BlueprintReadOnly, Category = "Line")
	bool bReeling = false;

	UPROPERTY(BlueprintReadOnly, Category = "Line")
	float ReloadRemaining = 0.f;

	/** Acceleration the line applies to the shooter (direction * magnitude). */
	UPROPERTY(BlueprintReadOnly, Category = "Line")
	FVector_NetQuantize10 ShooterPull;

	/** Acceleration applied to a harpooned player. */
	UPROPERTY(BlueprintReadOnly, Category = "Line")
	FVector_NetQuantize10 TargetPull;
};

/**
 * The diver's speargun: fire, reel, release, bag. Server-authoritative; clients get the line state and
 * draw the line locally. The shaft stays physically connected through a line that wraps around terrain,
 * tows divers (big fish), pulls loose objects, grapples the diver toward a stuck shaft and - for co-op
 * chaos - can clip onto the other player and reel them in.
 */
UCLASS(ClassGroup = (Spearfish), meta = (BlueprintSpawnableComponent))
class HOWTOSPEARFISH_API USpeargunComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpeargunComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** All machines, from the replicated loadout. */
	void ApplyStats(const FSpearfishDiverStats& InStats);

	// --- Local input ----------------------------------------------------------------------------
	void RequestFire();
	void SetReeling(bool bInReeling);
	void RequestRelease();

	// --- Server API -----------------------------------------------------------------------------
	void ForceRelease();
	void OnHarpoonHit(const FHitResult& Hit);
	void OnHarpoonExpired();
	void OnHarpoonReturned();
	/** Bags the exhausted fish on the line. Returns true on success. */
	bool TryBagHookedFish();
	/** Removes an exhausted hooked fish from the line for landing straight into the cooler. */
	ASpearfishFish* DetachFishForLanding();
	/** A predator stole the fish off the line. */
	void OnFishStolen();

	// --- Queries --------------------------------------------------------------------------------
	const FSpearfishLineNetState& GetLine() const { return Line; }
	ESpeargunState GetState() const { return Line.State; }
	ASpearfishFish* GetHookedFish() const;
	bool HasExhaustedFishOnLine() const;
	float GetReloadFraction() const;
	const FSpearfishDiverStats& GetStats() const { return Stats; }

	/** Gameplay muzzle: identical on server and owning client. */
	FVector GetMuzzleLocation() const;
	/** Where the line visually starts (first-person barrel for the owner, third-person gun for others). */
	FVector GetVisualMuzzleLocation() const;

	FVector GetShooterPullAcceleration() const;
	FVector GetTargetPullAcceleration(const AActor* Target) const;

	UFUNCTION(Server, Reliable)
	void ServerFire(FVector_NetQuantize Origin, FVector_NetQuantizeNormal Direction);

	UFUNCTION(Server, Reliable)
	void ServerSetReeling(bool bInReeling);

	UFUNCTION(Server, Reliable)
	void ServerRelease();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFireEffects(FVector_NetQuantize Origin);

private:
	ASpearfishCharacter* GetCharacter() const;
	void TickAttachedLine(float DeltaTime);
	void UpdateWraps(const FVector& Gun, const FVector& End);
	bool IsSegmentBlocked(const FVector& From, const FVector& To, FHitResult& OutHit) const;
	float ComputePathLength(const FVector& Gun, const FVector& End) const;
	void BeginRetrieve(bool bFishEscapes);
	void BeginReload();
	void ClearTarget(bool bFishEscapes);
	void Snap(bool bTangled);

	void BuildFirstPersonGun();
	void UpdateFirstPersonGun(float DeltaTime);
	void UpdateLineVisuals();
	void SetSegmentCount(int32 Count);

	UFUNCTION()
	void OnRep_Line();

	UPROPERTY(ReplicatedUsing = OnRep_Line)
	FSpearfishLineNetState Line;

	UPROPERTY(Replicated)
	TObjectPtr<ASpearfishHarpoon> Harpoon;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> LineSegments;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> FirstPersonRoot;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> FirstPersonBarrel;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> FirstPersonStock;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> FirstPersonShaft;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> FirstPersonBand;

	FSpearfishDiverStats Stats;
	FSpearfishLineState LineSim;
	bool bServerReeling = false;
	bool bLocalReeling = false;
	float AttachedTime = 0.f;
	float ShotQuality = 0.5f;
	float Recoil = 0.f;
	int32 LastSegmentColorStep = -1;
};
