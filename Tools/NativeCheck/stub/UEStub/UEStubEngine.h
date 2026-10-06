#pragma once

// Declaration-level stand-ins for the Engine module of UE 5.5: world, actors, components, pawns, controllers,
// game framework, collision, rendering components and HUD. Signatures follow the real engine headers.

#include "UEStub/UEStubShared.h"
#include "UEStub/UEStubUObject.h"

class AActor;
class APawn;
class ACharacter;
class AController;
class APlayerController;
class APlayerState;
class AGameModeBase;
class AGameStateBase;
class AGameSession;
class AHUD;
class APlayerCameraManager;
class AWorldSettings;
class UActorComponent;
class USceneComponent;
class UPrimitiveComponent;
class UInputComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPhysicalMaterial;
class UStaticMesh;
class USkeletalMesh;
class UTexture;
class UTexture2D;
class UTextureCube;
class UFont;
class UCanvas;
class ULocalPlayer;
class UPlayer;
class UNetDriver;
class UNetConnection;
class UGameViewportClient;
class UCharacterMovementComponent;
class UCapsuleComponent;
class USkeletalMeshComponent;
class UArrowComponent;
class SWidget;
struct FUniqueNetIdRepl;

// ------------------------------------------------------------------------------------ Basic enums

enum ETickingGroup : int
{
	TG_PrePhysics,
	TG_StartPhysics,
	TG_DuringPhysics,
	TG_EndPhysics,
	TG_PostPhysics,
	TG_PostUpdateWork,
	TG_LastDemotable,
	TG_NewlySpawned,
	TG_MAX
};

enum ELevelTick : int
{
	LEVELTICK_TimeOnly = 0,
	LEVELTICK_ViewportsOnly = 1,
	LEVELTICK_All = 2,
	LEVELTICK_PauseTick = 3
};

enum class ETeleportType : uint8
{
	None,
	TeleportPhysics,
	ResetPhysics
};

namespace EComponentMobility
{
	enum Type : int
	{
		Static,
		Stationary,
		Movable
	};
}

enum ECollisionChannel : int
{
	ECC_WorldStatic,
	ECC_WorldDynamic,
	ECC_Pawn,
	ECC_Visibility,
	ECC_Camera,
	ECC_PhysicsBody,
	ECC_Vehicle,
	ECC_Destructible,
	ECC_EngineTraceChannel1,
	ECC_EngineTraceChannel2,
	ECC_EngineTraceChannel3,
	ECC_EngineTraceChannel4,
	ECC_EngineTraceChannel5,
	ECC_EngineTraceChannel6,
	ECC_GameTraceChannel1,
	ECC_GameTraceChannel2,
	ECC_GameTraceChannel3,
	ECC_GameTraceChannel4,
	ECC_GameTraceChannel5,
	ECC_GameTraceChannel6,
	ECC_GameTraceChannel7,
	ECC_GameTraceChannel8,
	ECC_GameTraceChannel9,
	ECC_GameTraceChannel10,
	ECC_GameTraceChannel11,
	ECC_GameTraceChannel12,
	ECC_GameTraceChannel13,
	ECC_GameTraceChannel14,
	ECC_GameTraceChannel15,
	ECC_GameTraceChannel16,
	ECC_GameTraceChannel17,
	ECC_GameTraceChannel18,
	ECC_OverlapAll_Deprecated,
	ECC_MAX
};

enum ECollisionResponse : int
{
	ECR_Ignore,
	ECR_Overlap,
	ECR_Block,
	ECR_MAX
};

namespace ECollisionEnabled
{
	enum Type : int
	{
		NoCollision,
		QueryOnly,
		PhysicsOnly,
		QueryAndPhysics,
		ProbeOnly,
		QueryAndProbe
	};
}

enum class ESpawnActorCollisionHandlingMethod : uint8
{
	Undefined,
	AlwaysSpawn,
	AdjustIfPossibleButAlwaysSpawn,
	AdjustIfPossibleButDontSpawnIfColliding,
	DontSpawnIfColliding
};

enum class ESpawnActorScaleMethod : uint8
{
	OverrideRootScale,
	MultiplyWithRoot,
	SelectDefaultAtRuntime
};

enum class EAttachmentRule : uint8
{
	KeepRelative,
	KeepWorld,
	SnapToTarget
};

enum class EDetachmentRule : uint8
{
	KeepRelative,
	KeepWorld
};

enum EMovementMode : int
{
	MOVE_None,
	MOVE_Walking,
	MOVE_NavWalking,
	MOVE_Falling,
	MOVE_Swimming,
	MOVE_Flying,
	MOVE_Custom,
	MOVE_MAX
};

enum ETravelType : int
{
	TRAVEL_Absolute,
	TRAVEL_Partial,
	TRAVEL_Relative,
	TRAVEL_MAX
};

namespace ENetworkFailure
{
	enum Type : int
	{
		NetDriverAlreadyExists,
		NetDriverCreateFailure,
		NetDriverListenFailure,
		ConnectionLost,
		ConnectionTimeout,
		FailureReceived,
		OutdatedClient,
		OutdatedServer,
		PendingConnectionFailure,
		NetGuidMismatch,
		NetChecksumMismatch
	};
	const TCHAR* ToString(Type Value);
}

namespace ETravelFailure
{
	enum Type : int
	{
		NoLevel,
		LoadMapFailure,
		InvalidURL,
		PackageMissing,
		PackageVersion,
		NoDownload,
		TravelFailure,
		CheatCommands,
		PendingNetGameCreateFailure,
		CloudSaveFailure,
		ServerTravelFailure,
		ClientTravelFailure
	};
	const TCHAR* ToString(Type Value);
}

namespace EQuitPreference
{
	enum Type : int
	{
		Quit,
		Background
	};
}

enum class EMouseLockMode : uint8
{
	DoNotLock,
	LockOnCapture,
	LockAlways,
	LockInFullscreen
};

enum EHorizTextAligment : int
{
	EHTA_Left,
	EHTA_Center,
	EHTA_Right
};

enum EVerticalTextAligment : int
{
	EVRTA_TextTop,
	EVRTA_TextCenter,
	EVRTA_TextBottom,
	EVRTA_QuadTop
};

enum ESkyLightSourceType : int
{
	SLS_CapturedScene,
	SLS_SpecifiedCubemap,
	SLS_MAX
};

enum class ELightUnits : uint8
{
	Unitless,
	Candelas,
	Lumens,
	EV,
	Nits
};

namespace EWorldType
{
	enum Type : int
	{
		None,
		Game,
		Editor,
		PIE,
		EditorPreview,
		GamePreview,
		GameRPC,
		Inactive
	};
}

template <typename EnumType>
class TEnumAsByte
{
public:
	TEnumAsByte() = default;
	TEnumAsByte(EnumType InValue) : Value(static_cast<uint8>(InValue)) {}
	TEnumAsByte& operator=(EnumType InValue) { Value = static_cast<uint8>(InValue); return *this; }
	operator EnumType() const { return static_cast<EnumType>(Value); }
	EnumType GetValue() const { return static_cast<EnumType>(Value); }
	bool operator==(EnumType InValue) const { return static_cast<EnumType>(Value) == InValue; }
	bool operator!=(EnumType InValue) const { return static_cast<EnumType>(Value) != InValue; }

private:
	uint8 Value = 0;
};

// ------------------------------------------------------------------------------------- Networking

struct FVector_NetQuantize : public FVector
{
	FVector_NetQuantize() = default;
	FVector_NetQuantize(const FVector& In) : FVector(In) {}
	FVector_NetQuantize(double InX, double InY, double InZ) : FVector(InX, InY, InZ) {}
};
struct FVector_NetQuantize10 : public FVector
{
	FVector_NetQuantize10() = default;
	FVector_NetQuantize10(const FVector& In) : FVector(In) {}
	FVector_NetQuantize10(double InX, double InY, double InZ) : FVector(InX, InY, InZ) {}
};
struct FVector_NetQuantize100 : public FVector
{
	FVector_NetQuantize100() = default;
	FVector_NetQuantize100(const FVector& In) : FVector(In) {}
	FVector_NetQuantize100(double InX, double InY, double InZ) : FVector(InX, InY, InZ) {}
};
struct FVector_NetQuantizeNormal : public FVector
{
	FVector_NetQuantizeNormal() = default;
	FVector_NetQuantizeNormal(const FVector& In) : FVector(In) {}
	FVector_NetQuantizeNormal(double InX, double InY, double InZ) : FVector(InX, InY, InZ) {}
};

struct FUniqueNetId
{
	virtual ~FUniqueNetId();
	virtual FString ToString() const = 0;
	virtual bool IsValid() const = 0;
};

struct FUniqueNetIdWrapper
{
};

struct FUniqueNetIdRepl : public FUniqueNetIdWrapper
{
	bool IsValid() const;
	FString ToString() const;
	const FUniqueNetId* operator->() const;
	const FUniqueNetId& operator*() const;
	TSharedPtr<const FUniqueNetId> GetUniqueNetId() const;
	bool operator==(const FUniqueNetIdRepl& Other) const;
};

struct FURL
{
	FString Protocol;
	FString Host;
	int32 Port = 7777;
	FString Map;
	TArray<FString> Op;
	FString Portal;
	bool HasOption(const TCHAR* Test) const;
	const TCHAR* GetOption(const TCHAR* Match, const TCHAR* Default) const;
	void AddOption(const TCHAR* Str);
	FString ToString(bool FullyQualified = false) const;
};

// -------------------------------------------------------------------------------------- Collision

struct FHitResult
{
	int32 FaceIndex = 0;
	float Time = 1.f;
	float Distance = 0.f;
	FVector_NetQuantize Location;
	FVector_NetQuantize ImpactPoint;
	FVector_NetQuantizeNormal Normal;
	FVector_NetQuantizeNormal ImpactNormal;
	FVector_NetQuantize TraceStart;
	FVector_NetQuantize TraceEnd;
	float PenetrationDepth = 0.f;
	int32 Item = 0;
	FName BoneName;
	FName MyBoneName;
	TWeakObjectPtr<UPhysicalMaterial> PhysMaterial;
	uint8 bBlockingHit : 1;
	uint8 bStartPenetrating : 1;

	FHitResult();
	explicit FHitResult(float InTime);
	AActor* GetActor() const;
	UPrimitiveComponent* GetComponent() const;
	bool IsValidBlockingHit() const;
	void Reset(float InTime = 1.f, bool bPreserveTraceData = true);
};

struct FOverlapResult
{
	int32 ItemIndex = 0;
	uint32 bBlockingHit : 1;
	AActor* GetActor() const;
	UPrimitiveComponent* GetComponent() const;
};

struct FCollisionShape
{
	static FCollisionShape MakeSphere(const float SphereRadius);
	static FCollisionShape MakeBox(const FVector& BoxHalfExtent);
	static FCollisionShape MakeCapsule(const float CapsuleRadius, const float CapsuleHalfHeight);
	bool IsNearlyZero() const;
	float GetSphereRadius() const;
};

struct FCollisionQueryParams
{
	FName TraceTag;
	bool bTraceComplex = false;
	bool bFindInitialOverlaps = true;
	bool bReturnFaceIndex = false;
	bool bReturnPhysicalMaterial = false;
	bool bIgnoreBlocks = false;
	bool bIgnoreTouches = false;
	FCollisionQueryParams();
	FCollisionQueryParams(FName InTraceTag, bool bInTraceComplex = false, const AActor* InIgnoreActor = nullptr);
	FCollisionQueryParams(FName InTraceTag, const TStatId& InStatId, bool bInTraceComplex = false, const AActor* InIgnoreActor = nullptr);
	void AddIgnoredActor(const AActor* InIgnoreActor);
	void AddIgnoredActors(const TArray<AActor*>& InIgnoreActors);
	void AddIgnoredActors(const TArray<const AActor*>& InIgnoreActors);
	void AddIgnoredComponent(const UPrimitiveComponent* InIgnoreComponent);
	void ClearIgnoredActors();
	static FCollisionQueryParams DefaultQueryParam;
};
#define SCENE_QUERY_STAT(TagName) FName(TEXT(#TagName)), TStatId()
#define SCENE_QUERY_STAT_ONLY(TagName) TStatId()

struct FCollisionObjectQueryParams
{
	enum InitType
	{
		AllObjects,
		AllStaticObjects,
		AllDynamicObjects
	};
	FCollisionObjectQueryParams();
	FCollisionObjectQueryParams(ECollisionChannel QueryChannel);
	FCollisionObjectQueryParams(InitType QueryType);
	void AddObjectTypesToQuery(ECollisionChannel QueryChannel);
	void RemoveObjectTypesToQuery(ECollisionChannel QueryChannel);
	static FCollisionObjectQueryParams DefaultObjectQueryParam;
};

struct FCollisionResponseParams
{
	FCollisionResponseParams(ECollisionResponse DefaultResponse = ECR_Block);
	static FCollisionResponseParams DefaultResponseParam;
};

struct FAttachmentTransformRules
{
	EAttachmentRule LocationRule;
	EAttachmentRule RotationRule;
	EAttachmentRule ScaleRule;
	bool bWeldSimulatedBodies;
	FAttachmentTransformRules(EAttachmentRule InRule, bool bInWeldSimulatedBodies);
	FAttachmentTransformRules(EAttachmentRule InLocationRule, EAttachmentRule InRotationRule, EAttachmentRule InScaleRule, bool bInWeldSimulatedBodies);
	static FAttachmentTransformRules KeepRelativeTransform;
	static FAttachmentTransformRules KeepWorldTransform;
	static FAttachmentTransformRules SnapToTargetNotIncludingScale;
	static FAttachmentTransformRules SnapToTargetIncludingScale;
};

struct FDetachmentTransformRules
{
	FDetachmentTransformRules(EDetachmentRule InRule, bool bInCallModify);
	static FDetachmentTransformRules KeepRelativeTransform;
	static FDetachmentTransformRules KeepWorldTransform;
};

// --------------------------------------------------------------------------------------- Ticking

struct FTickFunction
{
	TEnumAsByte<ETickingGroup> TickGroup;
	TEnumAsByte<ETickingGroup> EndTickGroup;
	uint8 bTickEvenWhenPaused : 1;
	uint8 bCanEverTick : 1;
	uint8 bStartWithTickEnabled : 1;
	uint8 bAllowTickOnDedicatedServer : 1;
	uint8 bHighPriority : 1;
	float TickInterval = 0.f;
	virtual ~FTickFunction();
	void SetTickFunctionEnable(bool bInEnabled);
	bool IsTickFunctionEnabled() const;
	void AddPrerequisite(UObject* TargetObject, FTickFunction& TargetTickFunction);
	void RemovePrerequisite(UObject* TargetObject, FTickFunction& TargetTickFunction);
};
struct FActorTickFunction : public FTickFunction
{
	AActor* Target = nullptr;
};
struct FActorComponentTickFunction : public FTickFunction
{
	UActorComponent* Target = nullptr;
};

// ------------------------------------------------------------------------------------------ World

struct FActorSpawnParameters
{
	FActorSpawnParameters();
	FName Name;
	AActor* Template = nullptr;
	AActor* Owner = nullptr;
	APawn* Instigator = nullptr;
	ULevel* OverrideLevel = nullptr;
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined;
	ESpawnActorScaleMethod TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;
	uint8 bNoFail : 1;
	uint8 bDeferConstruction : 1;
	uint8 bAllowDuringConstructionScript : 1;
	EObjectFlags ObjectFlags = RF_Transactional;
};

class FConstPlayerControllerIterator
{
public:
	explicit operator bool() const;
	FConstPlayerControllerIterator& operator++();
	const TWeakObjectPtr<APlayerController>& operator*() const;
	const TWeakObjectPtr<APlayerController>* operator->() const;
};

class FConstPawnIterator
{
public:
	explicit operator bool() const;
	FConstPawnIterator& operator++();
	const TWeakObjectPtr<APawn>& operator*() const;
	const TWeakObjectPtr<APawn>* operator->() const;
};

class UWorld : public UObject
{
public:
	TEnumAsByte<EWorldType::Type> WorldType;

	template <class T>
	T* SpawnActor(UClass* Class = T::StaticClass(), const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());
	template <class T>
	T* SpawnActor(UClass* Class, FVector const& Location, FRotator const& Rotation, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());
	template <class T>
	T* SpawnActor(UClass* Class, FVector const* Location, FRotator const* Rotation, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());
	template <class T>
	T* SpawnActor(UClass* Class, FTransform const& Transform, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());
	AActor* SpawnActor(UClass* Class, FTransform const* Transform, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());
	template <class T>
	T* SpawnActorDeferred(UClass* Class, FTransform const& Transform, AActor* Owner = nullptr, APawn* Instigator = nullptr,
		ESpawnActorCollisionHandlingMethod CollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined,
		ESpawnActorScaleMethod TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot);
	bool DestroyActor(AActor* Actor, bool bNetForce = false, bool bShouldModifyLevel = true);

	bool LineTraceSingleByChannel(FHitResult& OutHit, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel,
		const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool LineTraceMultiByChannel(TArray<FHitResult>& OutHits, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel,
		const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool LineTraceSingleByObjectType(FHitResult& OutHit, const FVector& Start, const FVector& End, const FCollisionObjectQueryParams& ObjectQueryParams,
		const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool LineTraceTestByChannel(const FVector& Start, const FVector& End, ECollisionChannel TraceChannel,
		const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool SweepSingleByChannel(FHitResult& OutHit, const FVector& Start, const FVector& End, const FQuat& Rot, ECollisionChannel TraceChannel,
		const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam,
		const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool SweepSingleByObjectType(FHitResult& OutHit, const FVector& Start, const FVector& End, const FQuat& Rot,
		const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionShape& CollisionShape,
		const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool SweepMultiByChannel(TArray<FHitResult>& OutHits, const FVector& Start, const FVector& End, const FQuat& Rot, ECollisionChannel TraceChannel,
		const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam,
		const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool SweepTestByChannel(const FVector& Start, const FVector& End, const FQuat& Rot, ECollisionChannel TraceChannel, const FCollisionShape& CollisionShape,
		const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool OverlapMultiByChannel(TArray<FOverlapResult>& OutOverlaps, const FVector& Pos, const FQuat& Rot, ECollisionChannel TraceChannel,
		const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam,
		const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool OverlapMultiByObjectType(TArray<FOverlapResult>& OutOverlaps, const FVector& Pos, const FQuat& Rot, const FCollisionObjectQueryParams& ObjectQueryParams,
		const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool OverlapAnyTestByChannel(const FVector& Pos, const FQuat& Rot, ECollisionChannel TraceChannel, const FCollisionShape& CollisionShape,
		const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;

	double GetTimeSeconds() const;
	double GetRealTimeSeconds() const;
	double GetUnpausedTimeSeconds() const;
	double GetAudioTimeSeconds() const;
	float GetDeltaSeconds() const;
	FTimerManager& GetTimerManager() const;
	AGameStateBase* GetGameState() const;
	template <class T>
	T* GetGameState() const;
	AGameModeBase* GetAuthGameMode() const;
	template <class T>
	T* GetAuthGameMode() const;
	APlayerController* GetFirstPlayerController() const;
	template <class T>
	T* GetFirstPlayerController() const;
	ULocalPlayer* GetFirstLocalPlayerFromController() const;
	FConstPlayerControllerIterator GetPlayerControllerIterator() const;
	int32 GetNumPlayerControllers() const;
	ENetMode GetNetMode() const;
	bool IsNetMode(ENetMode Mode) const;
	UNetDriver* GetNetDriver() const;
	UGameInstance* GetGameInstance() const;
	template <class T>
	T* GetGameInstance() const;
	template <class T>
	T* GetGameInstanceChecked() const;
	template <class T>
	T* GetSubsystem() const;
	template <class T>
	static T* GetSubsystem(const UWorld* World);
	UGameViewportClient* GetGameViewport() const;
	AWorldSettings* GetWorldSettings(bool bCheckStreamingPersistent = false, bool bChecked = true) const;
	bool IsPlayInEditor() const;
	bool IsGameWorld() const;
	bool IsEditorWorld() const;
	bool IsPaused() const;
	bool HasBegunPlay() const;
	bool AreActorsInitialized() const;
	bool IsTearingDown() const;
	float GetGravityZ() const;
	float GetDefaultGravityZ() const;
	bool ServerTravel(const FString& InURL, bool bAbsolute = false, bool bShouldSkipGameNotify = false);
	bool Listen(FURL& InURL);
	FString GetMapName() const;
	ULevel* GetCurrentLevel() const;
	bool IsInSeamlessTravel() const;
};

extern UWorld* GWorld;

// -------------------------------------------------------------------------------------- Actor

class UActorComponent : public UObject
{
public:
	UActorComponent();
	explicit UActorComponent(const FObjectInitializer& ObjectInitializer);

	FActorComponentTickFunction PrimaryComponentTick;
	TArray<FName> ComponentTags;
	uint8 bAutoActivate : 1;
	uint8 bWantsInitializeComponent : 1;

	virtual UWorld* GetWorld() const override;
	AActor* GetOwner() const;
	template <class T>
	T* GetOwner() const;
	ENetRole GetOwnerRole() const;
	ENetMode GetNetMode() const;
	bool IsNetMode(ENetMode InNetMode) const;
	bool IsNetSimulating() const;
	bool IsRegistered() const;
	bool HasBegunPlay() const;
	bool IsBeingDestroyed() const;
	bool IsActive() const;
	bool GetIsReplicated() const;
	void SetIsReplicated(bool ShouldReplicate);
	void SetIsReplicatedByDefault(const bool bNewReplicates);
	void RegisterComponent();
	void UnregisterComponent();
	virtual void DestroyComponent(bool bPromoteChildren = false);
	virtual void Activate(bool bReset = false);
	virtual void Deactivate();
	void SetActive(bool bNewActive, bool bReset = false);
	virtual void SetComponentTickEnabled(bool bEnabled);
	bool IsComponentTickEnabled() const;
	void SetComponentTickInterval(float TickInterval);
	void SetTickGroup(ETickingGroup NewTickGroup);
	void AddTickPrerequisiteActor(AActor* PrerequisiteActor);
	void AddTickPrerequisiteComponent(UActorComponent* PrerequisiteComponent);
	void SetCanEverAffectNavigation(bool bRelevant);
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);
	virtual void InitializeComponent();
	virtual void UninitializeComponent();
	virtual void BeginPlay();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
	virtual void OnRegister();
	virtual void OnUnregister();
	virtual void OnComponentCreated();
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void MarkRenderStateDirty();
	template <class T>
	T* CreateDefaultSubobject(FName SubobjectName, bool bTransient = false);
};

class USceneComponent : public UActorComponent
{
public:
	USceneComponent();
	explicit USceneComponent(const FObjectInitializer& ObjectInitializer);

	TEnumAsByte<EComponentMobility::Type> Mobility;

	void SetupAttachment(USceneComponent* InParent, FName InSocketName = NAME_None);
	bool AttachToComponent(USceneComponent* InParent, const FAttachmentTransformRules& AttachmentRules, FName InSocketName = NAME_None);
	void DetachFromComponent(const FDetachmentTransformRules& DetachmentRules);
	USceneComponent* GetAttachParent() const;
	AActor* GetAttachmentRootActor() const;
	FName GetAttachSocketName() const;
	const TArray<TObjectPtr<USceneComponent>>& GetAttachChildren() const;
	void GetChildrenComponents(bool bIncludeAllDescendants, TArray<USceneComponent*>& Children) const;
	int32 GetNumChildrenComponents() const;
	USceneComponent* GetChildComponent(int32 ChildIndex) const;

	void SetRelativeLocation(FVector NewLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetRelativeRotation(FRotator NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetRelativeRotation(const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetRelativeLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetRelativeLocationAndRotation(FVector NewLocation, const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetRelativeScale3D(FVector NewScale3D);
	void SetRelativeTransform(const FTransform& NewTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	FVector GetRelativeLocation() const;
	FRotator GetRelativeRotation() const;
	FVector GetRelativeScale3D() const;
	FTransform GetRelativeTransform() const;
	void AddRelativeLocation(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddRelativeRotation(FRotator DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddLocalOffset(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddLocalRotation(FRotator DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddWorldOffset(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddWorldRotation(FRotator DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetWorldLocation(FVector NewLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetWorldRotation(FRotator NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetWorldRotation(const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetWorldLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetWorldLocationAndRotation(FVector NewLocation, const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetWorldScale3D(FVector NewScale);
	void SetWorldTransform(const FTransform& NewTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	FVector GetComponentLocation() const;
	FRotator GetComponentRotation() const;
	FQuat GetComponentQuat() const;
	FVector GetComponentScale() const;
	const FTransform& GetComponentTransform() const;
	FVector GetComponentVelocity() const;
	FVector GetForwardVector() const;
	FVector GetRightVector() const;
	FVector GetUpVector() const;
	FVector GetSocketLocation(FName InSocketName) const;
	FRotator GetSocketRotation(FName InSocketName) const;
	FTransform GetSocketTransform(FName InSocketName, int32 TransformSpace = 0) const;
	void SetMobility(EComponentMobility::Type NewMobility);
	void SetUsingAbsoluteLocation(bool bInAbsoluteLocation);
	void SetUsingAbsoluteRotation(bool bInAbsoluteRotation);
	void SetUsingAbsoluteScale(bool bInAbsoluteScale);
	void SetAbsolute(bool bNewAbsoluteLocation = false, bool bNewAbsoluteRotation = false, bool bNewAbsoluteScale = false);
	void SetVisibility(bool bNewVisibility, bool bPropagateToChildren = false);
	void ToggleVisibility(bool bPropagateToChildren = false);
	bool IsVisible() const;
	void SetHiddenInGame(bool NewHidden, bool bPropagateToChildren = false);
	void UpdateComponentToWorld();
};

class UPrimitiveComponent : public USceneComponent
{
public:
	UPrimitiveComponent();
	explicit UPrimitiveComponent(const FObjectInitializer& ObjectInitializer);

	uint8 CastShadow : 1;
	uint8 bCastDynamicShadow : 1;
	uint8 bOwnerNoSee : 1;
	uint8 bOnlyOwnerSee : 1;
	uint8 bAffectDistanceFieldLighting : 1;
	uint8 bReceivesDecals : 1;
	uint8 bRenderCustomDepth : 1;
	float LDMaxDrawDistance = 0.f;
	TArray<TObjectPtr<AActor>> MoveIgnoreActors;

	void SetCollisionEnabled(ECollisionEnabled::Type NewType);
	ECollisionEnabled::Type GetCollisionEnabled() const;
	void SetCollisionProfileName(FName InCollisionProfileName, bool bUpdateOverlaps = true);
	FName GetCollisionProfileName() const;
	void SetCollisionObjectType(ECollisionChannel Channel);
	ECollisionChannel GetCollisionObjectType() const;
	void SetCollisionResponseToAllChannels(ECollisionResponse NewResponse);
	void SetCollisionResponseToChannel(ECollisionChannel Channel, ECollisionResponse NewResponse);
	ECollisionResponse GetCollisionResponseToChannel(ECollisionChannel Channel) const;
	void SetGenerateOverlapEvents(bool bInGenerateOverlapEvents);
	void SetNotifyRigidBodyCollision(bool bNewNotifyRigidBodyCollision);
	void IgnoreActorWhenMoving(AActor* Actor, bool bShouldIgnore);
	void IgnoreComponentWhenMoving(UPrimitiveComponent* Component, bool bShouldIgnore);

	void SetSimulatePhysics(bool bSimulate);
	bool IsSimulatingPhysics(FName BoneName = NAME_None) const;
	void SetEnableGravity(bool bGravityEnabled);
	bool IsGravityEnabled() const;
	void AddForce(FVector Force, FName BoneName = NAME_None, bool bAccelChange = false);
	void AddForceAtLocation(FVector Force, FVector Location, FName BoneName = NAME_None);
	void AddImpulse(FVector Impulse, FName BoneName = NAME_None, bool bVelChange = false);
	void AddImpulseAtLocation(FVector Impulse, FVector Location, FName BoneName = NAME_None);
	void AddTorqueInRadians(FVector Torque, FName BoneName = NAME_None, bool bAccelChange = false);
	void AddAngularImpulseInRadians(FVector Impulse, FName BoneName = NAME_None, bool bVelChange = false);
	void SetPhysicsLinearVelocity(FVector NewVel, bool bAddToCurrent = false, FName BoneName = NAME_None);
	FVector GetPhysicsLinearVelocity(FName BoneName = NAME_None);
	void SetPhysicsAngularVelocityInDegrees(FVector NewAngVel, bool bAddToCurrent = false, FName BoneName = NAME_None);
	FVector GetPhysicsAngularVelocityInDegrees(FName BoneName = NAME_None) const;
	void SetPhysicsMaxAngularVelocityInDegrees(float NewMaxAngVel, bool bAddToCurrent = false, FName BoneName = NAME_None);
	float GetMass() const;
	void SetMassOverrideInKg(FName BoneName = NAME_None, float MassInKg = 1.f, bool bOverrideMass = true);
	void SetMassScale(FName BoneName = NAME_None, float InMassScale = 1.f);
	void SetLinearDamping(float InDamping);
	float GetLinearDamping() const;
	void SetAngularDamping(float InDamping);
	float GetAngularDamping() const;
	void SetUseCCD(bool InUseCCD, FName BoneName = NAME_None);
	void WakeRigidBody(FName BoneName = NAME_None);
	void PutRigidBodyToSleep(FName BoneName = NAME_None);
	bool RigidBodyIsAwake(FName BoneName = NAME_None) const;

	virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* Material);
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const;
	virtual int32 GetNumMaterials() const;
	UMaterialInstanceDynamic* CreateAndSetMaterialInstanceDynamic(int32 ElementIndex);
	UMaterialInstanceDynamic* CreateDynamicMaterialInstance(int32 ElementIndex, UMaterialInterface* SourceMaterial = nullptr, FName OptionalName = NAME_None);
	void SetCastShadow(bool NewCastShadow);
	void SetCastContactShadow(bool bInCastContactShadow);
	void SetCastHiddenShadow(bool NewCastHiddenShadow);
	void SetOwnerNoSee(bool bNewOwnerNoSee);
	void SetOnlyOwnerSee(bool bNewOnlyOwnerSee);
	void SetRenderCustomDepth(bool bValue);
	void SetCustomDepthStencilValue(int32 Value);
	void SetReceivesDecals(bool bNewReceivesDecals);
	void SetCullDistance(float NewCullDistance);
	void SetTranslucentSortPriority(int32 NewTranslucentSortPriority);
	void SetAffectDistanceFieldLighting(bool NewAffectDistanceFieldLighting);
	void SetAffectDynamicIndirectLighting(bool bNewAffectDynamicIndirectLighting);
	void SetAffectIndirectLightingWhileHidden(bool bNewAffectIndirectLightingWhileHidden);
	void SetVisibleInRayTracing(bool bNewVisibleInRayTracing);
	void SetVisibleInSceneCaptureOnly(bool bValue);
	void SetHiddenInSceneCapture(bool bValue);
	void SetRenderInMainPass(bool bValue);
	void SetRenderInDepthPass(bool bValue);
	void SetCustomPrimitiveDataFloat(int32 DataIndex, float Value);
	void SetCustomPrimitiveDataVector4(int32 DataIndex, FVector4 Value);
	void SetDefaultCustomPrimitiveDataFloat(int32 DataIndex, float Value);
};

class UShapeComponent : public UPrimitiveComponent
{
public:
	FColor ShapeColor;
	uint8 bDrawOnlyIfSelected : 1;
	void SetLineThickness(float Thickness);
};

class UBoxComponent : public UShapeComponent
{
public:
	void SetBoxExtent(FVector InBoxExtent, bool bUpdateOverlaps = true);
	FVector GetScaledBoxExtent() const;
	FVector GetUnscaledBoxExtent() const;
	void InitBoxExtent(const FVector& InExtent);
};

class USphereComponent : public UShapeComponent
{
public:
	void SetSphereRadius(float InSphereRadius, bool bUpdateOverlaps = true);
	float GetScaledSphereRadius() const;
	float GetUnscaledSphereRadius() const;
	void InitSphereRadius(float InSphereRadius);
};

class UCapsuleComponent : public UShapeComponent
{
public:
	void InitCapsuleSize(float InRadius, float InHalfHeight);
	void SetCapsuleSize(float InRadius, float InHalfHeight, bool bUpdateOverlaps = true);
	void SetCapsuleRadius(float Radius, bool bUpdateOverlaps = true);
	void SetCapsuleHalfHeight(float HalfHeight, bool bUpdateOverlaps = true);
	float GetScaledCapsuleRadius() const;
	float GetScaledCapsuleHalfHeight() const;
	float GetUnscaledCapsuleRadius() const;
	float GetUnscaledCapsuleHalfHeight() const;
	void GetScaledCapsuleSize(float& OutRadius, float& OutHalfHeight) const;
};

class UMeshComponent : public UPrimitiveComponent
{
public:
	virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* Material) override;
	void SetScalarParameterValueOnMaterials(const FName ParameterName, const float ParameterValue);
	void SetVectorParameterValueOnMaterials(const FName ParameterName, const FVector ParameterValue);
	TArray<UMaterialInterface*> GetMaterials() const;
};

class UStaticMeshComponent : public UMeshComponent
{
public:
	UStaticMeshComponent();
	explicit UStaticMeshComponent(const FObjectInitializer& ObjectInitializer);
	virtual bool SetStaticMesh(UStaticMesh* NewMesh);
	UStaticMesh* GetStaticMesh() const;
	void SetForcedLodModel(int32 NewForcedLodModel);
	void SetEvaluateWorldPositionOffset(bool NewValue);
};

class UInstancedStaticMeshComponent : public UStaticMeshComponent
{
public:
	int32 InstanceStartCullDistance = 0;
	int32 InstanceEndCullDistance = 0;
	int32 NumCustomDataFloats = 0;

	virtual int32 AddInstance(const FTransform& InstanceTransform, bool bWorldSpace = false);
	virtual TArray<int32> AddInstances(const TArray<FTransform>& InstanceTransforms, bool bShouldReturnIndices, bool bWorldSpace = false, bool bUpdateNavigation = true);
	virtual bool UpdateInstanceTransform(int32 InstanceIndex, const FTransform& NewInstanceTransform, bool bWorldSpace = false, bool bMarkRenderStateDirty = false,
		bool bTeleport = false);
	virtual bool BatchUpdateInstancesTransforms(int32 StartInstanceIndex, const TArray<FTransform>& NewInstancesTransforms, bool bWorldSpace = false,
		bool bMarkRenderStateDirty = false, bool bTeleport = false);
	virtual bool BatchUpdateInstancesTransform(int32 StartInstanceIndex, int32 NumInstances, const FTransform& NewInstancesTransform, bool bWorldSpace = false,
		bool bMarkRenderStateDirty = false, bool bTeleport = false);
	virtual bool RemoveInstance(int32 InstanceIndex);
	virtual void ClearInstances();
	int32 GetInstanceCount() const;
	bool GetInstanceTransform(int32 InstanceIndex, FTransform& OutInstanceTransform, bool bWorldSpace = false) const;
	void SetCullDistances(int32 StartCullDistance, int32 EndCullDistance);
	virtual void SetNumCustomDataFloats(int32 InNumCustomDataFloats);
	virtual bool SetCustomDataValue(int32 InstanceIndex, int32 CustomDataIndex, float CustomDataValue, bool bMarkRenderStateDirty = false);
};

class UHierarchicalInstancedStaticMeshComponent : public UInstancedStaticMeshComponent
{
public:
	uint8 bAutoRebuildTreeOnInstanceChanges : 1;
	bool BuildTreeIfOutdated(bool Async, bool ForceUpdate);
};

class USkinnedMeshComponent : public UMeshComponent
{
};

class USkeletalMeshComponent : public USkinnedMeshComponent
{
public:
	void SetSkeletalMesh(USkeletalMesh* NewMesh, bool bReinitPose = true);
};

class UTextRenderComponent : public UPrimitiveComponent
{
public:
	void SetText(const FText& Value);
	void SetTextRenderColor(FColor Value);
	void SetWorldSize(float Value);
	void SetXScale(float Value);
	void SetYScale(float Value);
	void SetHorizSpacingAdjust(float Value);
	void SetHorizontalAlignment(EHorizTextAligment Value);
	void SetVerticalAlignment(EVerticalTextAligment Value);
	void SetTextMaterial(UMaterialInterface* Material);
	void SetFont(UFont* Value);
};

class UArrowComponent : public UPrimitiveComponent
{
};

// ------------------------------------------------------------------------------------- Lights

class ULightComponentBase : public USceneComponent
{
public:
	float Intensity = 0.f;
	FColor LightColor;
	uint8 CastShadows : 1;
	uint8 bAffectsWorld : 1;
	void SetCastShadows(bool bNewValue);
	void SetCastVolumetricShadow(bool bNewValue);
	void SetAffectReflection(bool bNewValue);
	void SetAffectGlobalIllumination(bool bNewValue);
	void SetCastRaytracedShadow(bool bNewValue);
	void SetSamplesPerPixel(int32 NewValue);
};

class ULightComponent : public ULightComponentBase
{
public:
	float Temperature = 6500.f;
	uint8 bUseTemperature : 1;
	void SetIntensity(float NewIntensity);
	void SetLightColor(FLinearColor NewLightColor, bool bSRGB = true);
	void SetLightFColor(FColor NewLightColor);
	void SetTemperature(float NewTemperature);
	void SetUseTemperature(bool bNewValue);
	void SetIndirectLightingIntensity(float NewIntensity);
	void SetVolumetricScatteringIntensity(float NewIntensity);
	void SetAffectTranslucentLighting(bool bNewValue);
	void SetTransmission(bool bNewValue);
	void SetEnableLightShaftBloom(bool bNewValue);
	void SetBloomScale(float NewValue);
	void SetBloomThreshold(float NewValue);
	void SetBloomTint(FColor NewValue);
	void SetShadowBias(float NewValue);
	void SetSpecularScale(float NewValue);
	void SetLightFunctionMaterial(UMaterialInterface* NewLightFunctionMaterial);
};

class ULocalLightComponent : public ULightComponent
{
public:
	float AttenuationRadius = 1000.f;
	void SetAttenuationRadius(float NewRadius);
	void SetIntensityUnits(ELightUnits NewIntensityUnits);
	void SetInverseExposureBlend(float NewInverseExposureBlend);
};

class UPointLightComponent : public ULocalLightComponent
{
public:
	void SetLightFalloffExponent(float NewLightFalloffExponent);
	void SetSourceRadius(float bNewValue);
	void SetSoftSourceRadius(float bNewValue);
	void SetSourceLength(float NewValue);
	void SetUseInverseSquaredFalloff(bool bNewValue);
};

class USpotLightComponent : public UPointLightComponent
{
public:
	float InnerConeAngle = 0.f;
	float OuterConeAngle = 44.f;
	void SetInnerConeAngle(float NewInnerConeAngle);
	void SetOuterConeAngle(float NewOuterConeAngle);
};

class UDirectionalLightComponent : public ULightComponent
{
public:
	uint8 bAtmosphereSunLight : 1;
	int32 AtmosphereSunLightIndex = 0;
	void SetAtmosphereSunLight(bool bNewValue);
	void SetAtmosphereSunLightIndex(int32 NewValue);
	void SetDynamicShadowDistanceMovableLight(float NewValue);
	void SetDynamicShadowDistanceStationaryLight(float NewValue);
	void SetDynamicShadowCascades(int32 NewValue);
	void SetCascadeDistributionExponent(float NewValue);
	void SetLightSourceAngle(float NewValue);
	void SetLightSourceSoftAngle(float NewValue);
	void SetShadowAmount(float NewValue);
	void SetEnableLightShaftOcclusion(bool bNewValue);
	void SetOcclusionMaskDarkness(float NewValue);
	void SetForwardShadingPriority(int32 NewValue);
	void SetCastCloudShadows(bool bNewValue);
};

class USkyLightComponent : public ULightComponentBase
{
public:
	TEnumAsByte<ESkyLightSourceType> SourceType;
	bool bRealTimeCapture = false;
	bool bLowerHemisphereIsBlack = true;
	FLinearColor LowerHemisphereColor;
	void SetIntensity(float NewIntensity);
	void SetLightColor(FLinearColor NewLightColor);
	void SetIndirectLightingIntensity(float NewIntensity);
	void SetVolumetricScatteringIntensity(float NewIntensity);
	void SetLowerHemisphereColor(const FLinearColor& InLowerHemisphereColor);
	void SetCubemap(UTextureCube* NewCubemap);
	void SetRealTimeCapture(bool bInRealTimeCapture);
	void SetMinOcclusion(float InMinOcclusion);
	void SetOcclusionTint(const FColor& InTint);
	void RecaptureSky();
};

class USkyAtmosphereComponent : public USceneComponent
{
public:
	void SetBottomRadius(float NewValue);
	void SetAtmosphereHeight(float NewValue);
	void SetGroundAlbedo(const FColor& NewValue);
	void SetRayleighScatteringScale(float NewValue);
	void SetRayleighScattering(FLinearColor NewValue);
	void SetRayleighExponentialDistribution(float NewValue);
	void SetMieScatteringScale(float NewValue);
	void SetMieScattering(FLinearColor NewValue);
	void SetMieAbsorptionScale(float NewValue);
	void SetMieAnisotropy(float NewValue);
	void SetMieExponentialDistribution(float NewValue);
	void SetOtherAbsorptionScale(float NewValue);
	void SetOtherAbsorption(FLinearColor NewValue);
	void SetSkyLuminanceFactor(FLinearColor NewValue);
	void SetAerialPespectiveViewDistanceScale(float NewValue);
	void SetHeightFogContribution(float NewValue);
	void SetMultiScatteringFactor(float NewValue);
};

class UExponentialHeightFogComponent : public USceneComponent
{
public:
	void SetFogDensity(float Value);
	void SetFogHeightFalloff(float Value);
	void SetFogInscatteringColor(FLinearColor Value);
	void SetDirectionalInscatteringExponent(float Value);
	void SetDirectionalInscatteringStartDistance(float Value);
	void SetDirectionalInscatteringColor(FLinearColor Value);
	void SetFogMaxOpacity(float Value);
	void SetStartDistance(float Value);
	void SetEndDistance(float Value);
	void SetFogCutoffDistance(float Value);
	void SetVolumetricFog(bool bNewValue);
	void SetVolumetricFogScatteringDistribution(float NewValue);
	void SetVolumetricFogExtinctionScale(float NewValue);
	void SetVolumetricFogAlbedo(FColor NewValue);
	void SetVolumetricFogEmissive(FLinearColor NewValue);
	void SetVolumetricFogDistance(float NewValue);
	void SetVolumetricFogStartDistance(float NewValue);
	void SetVolumetricFogNearFadeInDistance(float NewValue);
	void SetSecondFogDensity(float Value);
	void SetSecondFogHeightFalloff(float Value);
	void SetSecondFogHeightOffset(float Value);
};

// ------------------------------------------------------------------------------------ Camera / PP

struct FWeightedBlendable
{
	float Weight = 0.f;
	TObjectPtr<UObject> Object;
	FWeightedBlendable();
	FWeightedBlendable(float InWeight, UObject* InObject);
};

struct FWeightedBlendables
{
	TArray<FWeightedBlendable> Array;
};

struct FPostProcessSettings
{
	uint32 bOverride_WhiteTemp : 1;
	uint32 bOverride_WhiteTint : 1;
	uint32 bOverride_ColorSaturation : 1;
	uint32 bOverride_ColorContrast : 1;
	uint32 bOverride_ColorGamma : 1;
	uint32 bOverride_ColorGain : 1;
	uint32 bOverride_ColorOffset : 1;
	uint32 bOverride_SceneColorTint : 1;
	uint32 bOverride_SceneFringeIntensity : 1;
	uint32 bOverride_BloomIntensity : 1;
	uint32 bOverride_BloomThreshold : 1;
	uint32 bOverride_VignetteIntensity : 1;
	uint32 bOverride_GrainIntensity : 1;
	uint32 bOverride_FilmGrainIntensity : 1;
	uint32 bOverride_AutoExposureBias : 1;
	uint32 bOverride_AutoExposureMinBrightness : 1;
	uint32 bOverride_AutoExposureMaxBrightness : 1;
	uint32 bOverride_AmbientOcclusionIntensity : 1;
	uint32 bOverride_DepthOfFieldFocalDistance : 1;
	uint32 bOverride_DepthOfFieldFstop : 1;
	uint32 bOverride_LensFlareIntensity : 1;
	uint32 bOverride_IndirectLightingIntensity : 1;
	uint32 bOverride_MotionBlurAmount : 1;

	float WhiteTemp = 6500.f;
	float WhiteTint = 0.f;
	FVector4 ColorSaturation;
	FVector4 ColorContrast;
	FVector4 ColorGamma;
	FVector4 ColorGain;
	FVector4 ColorOffset;
	FLinearColor SceneColorTint;
	float SceneFringeIntensity = 0.f;
	float BloomIntensity = 0.675f;
	float BloomThreshold = -1.f;
	float VignetteIntensity = 0.4f;
	float GrainIntensity = 0.f;
	float FilmGrainIntensity = 0.f;
	float AutoExposureBias = 1.f;
	float AutoExposureMinBrightness = 0.03f;
	float AutoExposureMaxBrightness = 8.f;
	float AmbientOcclusionIntensity = 0.5f;
	float DepthOfFieldFocalDistance = 0.f;
	float DepthOfFieldFstop = 4.f;
	float LensFlareIntensity = 1.f;
	float IndirectLightingIntensity = 1.f;
	float MotionBlurAmount = 0.5f;
	FWeightedBlendables WeightedBlendables;

	void AddBlendable(UObject* InBlendableObject, float InWeight);
	void RemoveBlendable(UObject* InBlendableObject);
};

class UCameraComponent : public USceneComponent
{
public:
	float FieldOfView = 90.f;
	float PostProcessBlendWeight = 1.f;
	FPostProcessSettings PostProcessSettings;
	uint8 bUsePawnControlRotation : 1;
	uint8 bConstrainAspectRatio : 1;
	void SetFieldOfView(float InFieldOfView);
	void SetPostProcessBlendWeight(float InPostProcessBlendWeight);
	void SetConstraintAspectRatio(bool bInConstrainAspectRatio);
};

// ---------------------------------------------------------------------------------- Materials

class UMaterialInterface : public UObject
{
public:
	class UMaterial* GetMaterial();
};

class UMaterial : public UMaterialInterface
{
};

class UMaterialInstance : public UMaterialInterface
{
};

class UMaterialInstanceDynamic : public UMaterialInstance
{
public:
	static UMaterialInstanceDynamic* Create(UMaterialInterface* ParentMaterial, UObject* InOuter);
	static UMaterialInstanceDynamic* Create(UMaterialInterface* ParentMaterial, UObject* InOuter, FName Name);
	void SetVectorParameterValue(FName ParameterName, FLinearColor Value);
	void SetScalarParameterValue(FName ParameterName, float Value);
	void SetTextureParameterValue(FName ParameterName, UTexture* Value);
	float K2_GetScalarParameterValue(FName ParameterName);
};

class UStaticMesh : public UObject
{
public:
	FBox GetBoundingBox() const;
	UMaterialInterface* GetMaterial(int32 MaterialIndex) const;
};

class USkeletalMesh : public UObject
{
};

class UTexture : public UObject
{
};

class UTexture2D : public UTexture
{
};

class UTextureCube : public UTexture
{
};

class UFont : public UObject
{
};

class UPhysicalMaterial : public UObject
{
};

// ---------------------------------------------------------------------------------------- Actor

class AActor : public UObject
{
public:
	AActor();
	explicit AActor(const FObjectInitializer& ObjectInitializer);

	FActorTickFunction PrimaryActorTick;
	TArray<FName> Tags;
	float InitialLifeSpan = 0.f;
	float CustomTimeDilation = 1.f;
	uint8 bAlwaysRelevant : 1;
	uint8 bOnlyRelevantToOwner : 1;
	uint8 bNetLoadOnClient : 1;
	uint8 bFindCameraComponentWhenViewTarget : 1;
	uint8 bBlockInput : 1;
	TObjectPtr<UInputComponent> InputComponent;

	virtual UWorld* GetWorld() const override;
	template <class T>
	T* GetGameInstance() const;
	class UGameInstance* GetGameInstance() const;
	FTimerManager& GetWorldTimerManager() const;

	virtual void PostInitializeComponents() override;
	virtual void OnConstruction(const FTransform& Transform);
	virtual void Tick(float DeltaSeconds);
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
	virtual void Destroyed();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(class IRepChangedPropertyTracker& ChangedPropertyTracker);
	virtual void OnRep_ReplicatedMovement();
	virtual void PostNetReceiveLocationAndRotation();
	virtual void PostNetInit();
	virtual void OnRep_Owner();
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor);
	virtual void NotifyActorEndOverlap(AActor* OtherActor);
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation,
		FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit);
	virtual void CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult);
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const;
	virtual FVector GetVelocity() const;
	virtual void SetOwner(AActor* NewOwner);
	AActor* GetOwner() const;
	template <class T>
	T* GetOwner() const;
	APawn* GetInstigator() const;
	AController* GetInstigatorController() const;

	bool HasAuthority() const;
	ENetRole GetLocalRole() const;
	ENetRole GetRemoteRole() const;
	ENetMode GetNetMode() const;
	bool IsNetMode(ENetMode Mode) const;
	bool GetIsReplicated() const;
	void SetReplicates(bool bInReplicates);
	void SetReplicatingMovement(bool bInReplicateMovement);
	bool IsReplicatingMovement() const;
	void SetNetUpdateFrequency(float Frequency);
	float GetNetUpdateFrequency() const;
	void SetMinNetUpdateFrequency(float MinFrequency);
	float GetMinNetUpdateFrequency() const;
	void SetNetCullDistanceSquared(float DistanceSq);
	float GetNetCullDistanceSquared() const;
	void SetNetPriority(float InNetPriority);
	void ForceNetUpdate();
	void FlushNetDormancy();
	UNetConnection* GetNetConnection() const;
	class UPlayer* GetNetOwningPlayer();

	USceneComponent* GetRootComponent() const;
	bool SetRootComponent(USceneComponent* NewRootComponent);
	FVector GetActorLocation() const;
	FRotator GetActorRotation() const;
	FQuat GetActorQuat() const;
	FVector GetActorScale3D() const;
	FVector GetActorForwardVector() const;
	FVector GetActorRightVector() const;
	FVector GetActorUpVector() const;
	const FTransform& GetActorTransform() const;
	void GetActorBounds(bool bOnlyCollidingComponents, FVector& Origin, FVector& BoxExtent, bool bIncludeFromChildActors = false) const;
	bool SetActorLocation(const FVector& NewLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	bool SetActorRotation(FRotator NewRotation, ETeleportType Teleport = ETeleportType::None);
	bool SetActorRotation(const FQuat& NewRotation, ETeleportType Teleport = ETeleportType::None);
	bool SetActorLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
		ETeleportType Teleport = ETeleportType::None);
	bool SetActorLocationAndRotation(FVector NewLocation, const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
		ETeleportType Teleport = ETeleportType::None);
	bool SetActorTransform(const FTransform& NewTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetActorScale3D(FVector NewScale3D);
	void SetActorRelativeLocation(FVector NewRelativeLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetActorRelativeRotation(FRotator NewRelativeRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetActorRelativeRotation(const FQuat& NewRelativeRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetActorRelativeTransform(const FTransform& NewRelativeTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr,
		ETeleportType Teleport = ETeleportType::None);
	void SetActorRelativeScale3D(FVector NewRelativeScale);
	void AddActorWorldOffset(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddActorWorldRotation(FRotator DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddActorLocalOffset(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddActorLocalRotation(FRotator DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	virtual bool TeleportTo(const FVector& DestLocation, const FRotator& DestRotation, bool bIsATest = false, bool bNoCheck = false);
	float GetDistanceTo(const AActor* OtherActor) const;
	float GetSquaredDistanceTo(const AActor* OtherActor) const;
	float GetHorizontalDistanceTo(const AActor* OtherActor) const;

	bool AttachToActor(AActor* ParentActor, const FAttachmentTransformRules& AttachmentRules, FName SocketName = NAME_None);
	bool AttachToComponent(USceneComponent* Parent, const FAttachmentTransformRules& AttachmentRules, FName SocketName = NAME_None);
	void DetachFromActor(const FDetachmentTransformRules& DetachmentRules);
	AActor* GetAttachParentActor() const;
	void GetAttachedActors(TArray<AActor*>& OutActors, bool bResetArray = true, bool bRecursivelyIncludeAttachedActors = false) const;

	virtual bool Destroy(bool bNetForce = false, bool bShouldModifyLevel = true);
	void FinishSpawning(const FTransform& Transform, bool bIsDefaultTransform = false, const struct FComponentInstanceDataCache* InstanceDataCache = nullptr,
		ESpawnActorScaleMethod TransformScaleMethod = ESpawnActorScaleMethod::OverrideRootScale);
	bool IsActorBeingDestroyed() const;
	bool HasActorBegunPlay() const;
	bool IsActorInitialized() const;
	void SetLifeSpan(float InLifespan);
	void SetActorHiddenInGame(bool bNewHidden);
	bool IsHidden() const;
	void SetActorEnableCollision(bool bNewActorEnableCollision);
	bool GetActorEnableCollision() const;
	void SetActorTickEnabled(bool bEnabled);
	bool IsActorTickEnabled() const;
	void SetActorTickInterval(float TickInterval);
	float GetActorTickInterval() const;
	void SetCanBeDamaged(bool bInCanBeDamaged);
	void AddTickPrerequisiteActor(AActor* PrerequisiteActor);
	void AddTickPrerequisiteComponent(UActorComponent* PrerequisiteComponent);
	bool IsOwnedBy(const AActor* TestOwner) const;
	bool ActorHasTag(FName Tag) const;
	float GetGameTimeSinceCreation() const;

	void AddInstanceComponent(UActorComponent* Component);
	void RemoveInstanceComponent(UActorComponent* Component);
	void AddOwnedComponent(UActorComponent* Component);
	template <class T>
	T* FindComponentByClass() const;
	UActorComponent* FindComponentByClass(const TSubclassOf<UActorComponent> ComponentClass) const;
	template <class T>
	void GetComponents(TArray<T*>& OutComponents, bool bIncludeFromChildActors = false) const;
	template <class T>
	T* CreateDefaultSubobject(FName SubobjectName, bool bTransient = false);
	void EnableInput(APlayerController* PlayerController);
	void DisableInput(APlayerController* PlayerController);

protected:
	uint8 bReplicates : 1;
	TObjectPtr<USceneComponent> RootComponent;
	virtual void BeginPlay();
};

// --------------------------------------------------------------------------------- Pawn & friends

class UMovementComponent : public UActorComponent
{
public:
	FVector Velocity;
	TObjectPtr<USceneComponent> UpdatedComponent;
	TObjectPtr<UPrimitiveComponent> UpdatedPrimitive;
	uint8 bConstrainToPlane : 1;
	virtual float GetMaxSpeed() const;
	virtual float GetGravityZ() const;
	virtual void StopMovementImmediately();
	bool IsExceedingMaxSpeed(float MaxSpeed) const;
	bool SafeMoveUpdatedComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult& OutHit, ETeleportType Teleport = ETeleportType::None);
	bool SafeMoveUpdatedComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult& OutHit, ETeleportType Teleport = ETeleportType::None);
	bool MoveUpdatedComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit = nullptr, ETeleportType Teleport = ETeleportType::None);
	virtual float SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact = false);
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.f, const FVector& MoveDelta = FVector::ZeroVector);
	virtual void UpdateComponentVelocity();
};

struct FMovementProperties
{
	uint8 bCanCrouch : 1;
	uint8 bCanJump : 1;
	uint8 bCanWalk : 1;
	uint8 bCanSwim : 1;
	uint8 bCanFly : 1;
};

struct FNavAgentProperties : public FMovementProperties
{
	float AgentRadius = -1.f;
	float AgentHeight = -1.f;
	float AgentStepHeight = -1.f;
};

class UNavMovementComponent : public UMovementComponent
{
public:
	FNavAgentProperties NavAgentProps;
};

class UPawnMovementComponent : public UNavMovementComponent
{
public:
	APawn* GetPawnOwner() const;
	FVector GetPendingInputVector() const;
	FVector GetLastInputVector() const;
	FVector ConsumeInputVector();
	virtual void AddInputVector(FVector WorldVector, bool bForce = false);
};

class APawn : public AActor
{
public:
	APawn();
	explicit APawn(const FObjectInitializer& ObjectInitializer);

	float BaseEyeHeight = 64.f;
	uint32 bUseControllerRotationPitch : 1;
	uint32 bUseControllerRotationYaw : 1;
	uint32 bUseControllerRotationRoll : 1;
	TSubclassOf<AController> AIControllerClass;

	virtual void PossessedBy(AController* NewController);
	virtual void UnPossessed();
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent);
	virtual void PawnClientRestart();
	virtual void Restart();
	virtual void OnRep_Controller();
	virtual void OnRep_PlayerState();
	virtual void NotifyControllerChanged();
	virtual FRotator GetBaseAimRotation() const;
	virtual FVector GetPawnViewLocation() const;
	virtual FRotator GetViewRotation() const;
	virtual UPawnMovementComponent* GetMovementComponent() const;
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue = 1.0f, bool bForce = false);
	virtual void AddControllerYawInput(float Val);
	virtual void AddControllerPitchInput(float Val);
	virtual void AddControllerRollInput(float Val);
	FVector GetPendingMovementInputVector() const;
	FVector GetLastMovementInputVector() const;
	FVector ConsumeMovementInputVector();
	FRotator GetControlRotation() const;
	AController* GetController() const;
	template <class T>
	T* GetController() const;
	APlayerState* GetPlayerState() const;
	template <class T>
	T* GetPlayerState() const;
	bool IsLocallyControlled() const;
	bool IsPlayerControlled() const;
	bool IsBotControlled() const;
	bool IsControlled() const;
	APlayerController* GetLocalViewingPlayerController() const;
};

#define MIN_TICK_TIME 1e-6f

struct FFindFloorResult
{
	uint32 bBlockingHit : 1;
	uint32 bWalkableFloor : 1;
	uint32 bLineTrace : 1;
	float FloorDist = 0.f;
	float LineDist = 0.f;
	FHitResult HitResult;
	bool IsWalkableFloor() const;
	float GetDistanceToFloor() const;
	void Clear();
};

struct FRootMotionSourceGroup
{
	bool HasActiveRootMotionSources() const;
	bool HasOverrideVelocity() const;
	bool HasAdditiveVelocity() const;
	bool HasVelocity() const;
};

class FNetworkPredictionData_Client
{
public:
	virtual ~FNetworkPredictionData_Client();
};

class FSavedMove_Character;
typedef TSharedPtr<FSavedMove_Character> FSavedMovePtr;
class FNetworkPredictionData_Client_Character;

class FSavedMove_Character
{
public:
	FSavedMove_Character();
	virtual ~FSavedMove_Character();

	enum CompressedFlags
	{
		FLAG_JumpPressed = 0x01,
		FLAG_WantsToCrouch = 0x02,
		FLAG_Reserved_1 = 0x04,
		FLAG_Reserved_2 = 0x08,
		FLAG_Custom_0 = 0x10,
		FLAG_Custom_1 = 0x20,
		FLAG_Custom_2 = 0x40,
		FLAG_Custom_3 = 0x80
	};

	TWeakObjectPtr<ACharacter> CharacterOwner;
	uint32 bPressedJump : 1;
	uint32 bWantsToCrouch : 1;
	float DeltaTime = 0.f;
	FVector Acceleration;

	virtual void Clear();
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData);
	virtual void PrepMoveFor(ACharacter* C);
	virtual uint8 GetCompressedFlags() const;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const;
	virtual void CombineWith(const FSavedMove_Character* OldMove, ACharacter* InCharacter, APlayerController* PC, const FVector& OldStartLocation);
};

class FNetworkPredictionData_Client_Character : public FNetworkPredictionData_Client
{
public:
	FNetworkPredictionData_Client_Character(const UCharacterMovementComponent& ClientMovement);
	virtual ~FNetworkPredictionData_Client_Character();
	virtual FSavedMovePtr AllocateNewMove();
};

class UCharacterMovementComponent : public UPawnMovementComponent
{
public:
	UCharacterMovementComponent();
	explicit UCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	TEnumAsByte<EMovementMode> MovementMode;
	uint8 CustomMovementMode = 0;
	float GravityScale = 1.f;
	float MaxStepHeight = 45.f;
	float JumpZVelocity = 420.f;
	float AirControl = 0.05f;
	float MaxWalkSpeed = 600.f;
	float MaxWalkSpeedCrouched = 300.f;
	float MaxSwimSpeed = 300.f;
	float MaxFlySpeed = 600.f;
	float MaxCustomMovementSpeed = 600.f;
	float MaxAcceleration = 2048.f;
	float BrakingFrictionFactor = 2.f;
	float BrakingFriction = 0.f;
	float BrakingDecelerationWalking = 2048.f;
	float BrakingDecelerationFalling = 0.f;
	float BrakingDecelerationSwimming = 0.f;
	float BrakingDecelerationFlying = 0.f;
	float GroundFriction = 8.f;
	float FallingLateralFriction = 0.f;
	float Buoyancy = 1.f;
	float PerchRadiusThreshold = 0.f;
	uint8 bUseSeparateBrakingFriction : 1;
	uint8 bOrientRotationToMovement : 1;
	uint8 bUseControllerDesiredRotation : 1;
	uint8 bCanWalkOffLedges : 1;
	uint8 bCheatFlying : 1;
	uint8 bApplyGravityWhileJumping : 1;
	FRotator RotationRate;
	FVector PendingLaunchVelocity;

	virtual void SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode = 0);
	virtual void DisableMovement();
	virtual void StopMovementImmediately() override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const;
	virtual float GetMaxBrakingDeceleration() const;
	virtual float GetGravityZ() const override;
	virtual bool IsFalling() const;
	virtual bool IsFlying() const;
	virtual bool IsSwimming() const;
	virtual bool IsMovingOnGround() const;
	virtual bool IsWalking() const;
	virtual bool IsCrouching() const;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration);
	virtual void AddImpulse(FVector Impulse, bool bVelocityChange = false);
	virtual void AddForce(FVector Force);
	virtual void Launch(FVector const& LaunchVel);
	virtual void ClearAccumulatedForces();
	virtual void ApplyAccumulatedForces(float DeltaSeconds);
	FVector GetCurrentAcceleration() const;
	FVector GetLastUpdateVelocity() const;
	void SetWalkableFloorAngle(float InWalkableFloorAngle);
	bool HasValidData() const;
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const;
	virtual bool CanAttemptJump() const;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds);
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds);
	bool HasAnimRootMotion() const;
	virtual void FindFloor(const FVector& CapsuleLocation, FFindFloorResult& OutFloorResult, bool bCanUseCachedLocation, const FHitResult* DownwardSweepResult = nullptr) const;
	FFindFloorResult CurrentFloor;
	FRootMotionSourceGroup CurrentRootMotion;
	class FNetworkPredictionData_Client_Character* GetPredictionData_Client_Character() const;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	ACharacter* GetCharacterOwner() const;

protected:
	TObjectPtr<ACharacter> CharacterOwner;
	FVector Acceleration;
	mutable FNetworkPredictionData_Client_Character* ClientPredictionData = nullptr;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode);
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity);
	virtual void UpdateFromCompressedFlags(uint8 Flags);
	virtual void PerformMovement(float DeltaTime);
	virtual void PhysWalking(float deltaTime, int32 Iterations);
	virtual void PhysFalling(float deltaTime, int32 Iterations);
	virtual void PhysFlying(float deltaTime, int32 Iterations);
	virtual void PhysSwimming(float deltaTime, int32 Iterations);
	virtual void PhysCustom(float deltaTime, int32 Iterations);
	virtual void StartNewPhysics(float deltaTime, int32 Iterations);
	virtual FVector ConstrainInputAcceleration(const FVector& InputAcceleration) const;
	virtual FVector ScaleInputAcceleration(const FVector& InputAcceleration) const;
	virtual void ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration);
	virtual FVector ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const;
};

class ACharacter : public APawn
{
public:
	ACharacter();
	explicit ACharacter(const FObjectInitializer& ObjectInitializer);

	static FName MeshComponentName;
	static FName CharacterMovementComponentName;
	static FName CapsuleComponentName;

	UCharacterMovementComponent* GetCharacterMovement() const;
	template <class T>
	T* GetCharacterMovement() const;
	UCapsuleComponent* GetCapsuleComponent() const;
	USkeletalMeshComponent* GetMesh() const;
	UArrowComponent* GetArrowComponent() const;
	virtual UPawnMovementComponent* GetMovementComponent() const override;
	virtual void Jump();
	virtual void StopJumping();
	virtual bool CanJump() const;
	virtual void LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride);
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0);
	UPrimitiveComponent* GetMovementBase() const;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void PostInitializeComponents() override;
	virtual void Landed(const FHitResult& Hit);
	virtual void Crouch(bool bClientSimulation = false);
	virtual void UnCrouch(bool bClientSimulation = false);
};

class AController : public AActor
{
public:
	TObjectPtr<APlayerState> PlayerState;

	APawn* GetPawn() const;
	template <class T>
	T* GetPawn() const;
	ACharacter* GetCharacter() const;
	template <class T>
	T* GetPlayerState() const;
	virtual void Possess(APawn* InPawn);
	virtual void UnPossess();
	virtual FRotator GetControlRotation() const;
	virtual void SetControlRotation(const FRotator& NewRotation);
	virtual void SetIgnoreMoveInput(bool bNewMoveInput);
	virtual void SetIgnoreLookInput(bool bNewLookInput);
	virtual void ResetIgnoreInputFlags();
	virtual void StopMovement();
	bool IsLocalController() const;
	bool IsLocalPlayerController() const;
	bool IsPlayerController() const;
	virtual void GetPlayerViewPoint(FVector& Location, FRotator& Rotation) const;
	virtual bool LineOfSightTo(const AActor* Other, FVector ViewPoint = FVector::ZeroVector, bool bAlternateChecks = false) const;
	virtual void ClientSetRotation(FRotator NewRotation, bool bResetCamera = false);
	virtual void ClientSetLocation(FVector NewLocation, FRotator NewRotation);

protected:
	virtual void OnPossess(APawn* InPawn);
	virtual void OnUnPossess();
};

struct FInputModeDataBase
{
	virtual ~FInputModeDataBase();
};

struct FInputModeGameOnly : public FInputModeDataBase
{
	FInputModeGameOnly& SetConsumeCaptureMouseDown(bool InConsumeCaptureMouseDown);
};

struct FInputModeUIOnly : public FInputModeDataBase
{
	FInputModeUIOnly& SetWidgetToFocus(TSharedPtr<SWidget> InWidgetToFocus);
	FInputModeUIOnly& SetLockMouseToViewportBehavior(EMouseLockMode InMouseLockMode);
};

struct FInputModeGameAndUI : public FInputModeDataBase
{
	FInputModeGameAndUI& SetWidgetToFocus(TSharedPtr<SWidget> InWidgetToFocus);
	FInputModeGameAndUI& SetLockMouseToViewportBehavior(EMouseLockMode InMouseLockMode);
	FInputModeGameAndUI& SetHideCursorDuringCapture(bool InHideCursorDuringCapture);
};

class APlayerCameraManager : public AActor
{
public:
	float ViewPitchMin = -89.9f;
	float ViewPitchMax = 89.9f;
	FVector GetCameraLocation() const;
	FRotator GetCameraRotation() const;
	virtual float GetFOVAngle() const;
	virtual void StartCameraFade(float FromAlpha, float ToAlpha, float Duration, FLinearColor Color, bool bShouldFadeAudio = false, bool bHoldWhenFinished = false);
	virtual void StopCameraFade();
	virtual void SetManualCameraFade(float InFadeAmount, FLinearColor Color, bool bInFadeAudio);
};

class APlayerController : public AController
{
public:
	APlayerController();
	explicit APlayerController(const FObjectInitializer& ObjectInitializer);

	TObjectPtr<APlayerCameraManager> PlayerCameraManager;
	TObjectPtr<UPlayer> Player;
	uint32 bShowMouseCursor : 1;
	uint32 bEnableClickEvents : 1;
	uint32 bEnableMouseOverEvents : 1;

	ULocalPlayer* GetLocalPlayer() const;
	virtual void PlayerTick(float DeltaTime);
	virtual void SetInputMode(const FInputModeDataBase& InData);
	virtual void ClientTravel(const FString& URL, enum ETravelType TravelType, bool bSeamless = false, FGuid MapPackageGuid = FGuid());
	bool ProjectWorldLocationToScreen(FVector WorldLocation, FVector2D& ScreenLocation, bool bPlayerViewportRelative = false) const;
	bool DeprojectScreenPositionToWorld(float ScreenX, float ScreenY, FVector& WorldLocation, FVector& WorldDirection) const;
	void GetViewportSize(int32& SizeX, int32& SizeY) const;
	AHUD* GetHUD() const;
	template <class T>
	T* GetHUD() const;
	virtual void ToggleSpeaking(bool bInSpeaking);
	virtual void StartTalking();
	virtual void StopTalking();
	virtual FString ConsoleCommand(const FString& Command, bool bWriteToLog = true);
	virtual void ClientMessage(const FString& S, FName Type = NAME_None, float MsgLifeTime = 0.f);
	virtual void ClientReturnToMainMenuWithTextReason(const FText& ReturnReason);
	virtual bool SetPause(bool bPause);
	virtual bool IsPaused() const;
	bool IsInputKeyDown(const struct FKey Key) const;
	bool WasInputKeyJustPressed(const struct FKey Key) const;
	bool GetMousePosition(float& LocationX, float& LocationY) const;
	virtual void AddYawInput(float Val);
	virtual void AddPitchInput(float Val);
	virtual void SetViewTarget(AActor* NewViewTarget);
	virtual void SetViewTargetWithBlend(AActor* NewViewTarget, float BlendTime = 0, int32 BlendFunc = 0, float BlendExp = 0, bool bLockOutgoing = false);
	virtual void StartSpectatingOnly();
	virtual void ClientRestart(APawn* NewPawn);
	virtual void AcknowledgePossession(APawn* P);
	virtual void PostInitializeComponents() override;

protected:
	virtual void SetupInputComponent();
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
};

class APlayerState : public AActor
{
public:
	FString GetPlayerName() const;
	virtual void SetPlayerName(const FString& S);
	int32 GetPlayerId() const;
	void SetPlayerId(int32 NewId);
	const FUniqueNetIdRepl& GetUniqueId() const;
	APawn* GetPawn() const;
	template <class T>
	T* GetPawn() const;
	APlayerController* GetPlayerController() const;
	AController* GetOwningController() const;
	bool IsInactive() const;
	bool IsABot() const;
	bool IsSpectator() const;
	bool IsOnlyASpectator() const;
	float GetScore() const;
	void SetScore(const float NewScore);
	virtual void Reset();
	virtual void ClientInitialize(AController* C);
	virtual void OnRep_PlayerName();

protected:
	virtual void CopyProperties(APlayerState* PlayerState);
	virtual void OverrideWith(APlayerState* PlayerState);
};

class AInfo : public AActor
{
};

class AGameStateBase : public AInfo
{
public:
	TArray<TObjectPtr<APlayerState>> PlayerArray;
	TSubclassOf<AGameModeBase> GameModeClass;
	TObjectPtr<AGameModeBase> AuthorityGameMode;

	virtual double GetServerWorldTimeSeconds() const;
	virtual bool HasBegunPlay() const;
	virtual bool HasMatchStarted() const;
	virtual void HandleBeginPlay();
	virtual void AddPlayerState(APlayerState* PlayerState);
	virtual void RemovePlayerState(APlayerState* PlayerState);
	APlayerState* GetPlayerStateFromUniqueNetId(const FUniqueNetIdWrapper& InPlayerId) const;
	float GetPlayerStartTime(AController* Controller) const;
};

class AGameSession : public AInfo
{
public:
	int32 MaxSpectators = 2;
	int32 MaxPlayers = 16;
	int32 MaxPartySize = 0;
	virtual bool AtCapacity(bool bSpectator);
	virtual FString ApproveLogin(const FString& Options);
};

class AGameModeBase : public AInfo
{
public:
	AGameModeBase();
	explicit AGameModeBase(const FObjectInitializer& ObjectInitializer);

	FString OptionsString;
	TSubclassOf<AGameSession> GameSessionClass;
	TSubclassOf<AGameStateBase> GameStateClass;
	TSubclassOf<APlayerController> PlayerControllerClass;
	TSubclassOf<APlayerState> PlayerStateClass;
	TSubclassOf<AHUD> HUDClass;
	TSubclassOf<APawn> DefaultPawnClass;
	TSubclassOf<APawn> SpectatorClass;
	TObjectPtr<AGameSession> GameSession;
	TObjectPtr<AGameStateBase> GameState;
	uint32 bUseSeamlessTravel : 1;
	uint32 bStartPlayersAsSpectators : 1;
	uint32 bPauseable : 1;

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage);
	virtual void InitGameState();
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage);
	virtual APlayerController* Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options,
		const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage);
	virtual void PostLogin(APlayerController* NewPlayer);
	virtual void Logout(AController* Exiting);
	virtual void StartPlay();
	virtual bool HasMatchStarted() const;
	virtual bool HasMatchEnded() const;
	virtual void RestartPlayer(AController* NewPlayer);
	virtual void RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot);
	virtual void RestartPlayerAtTransform(AController* NewPlayer, const FTransform& SpawnTransform);
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot);
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform);
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController);
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player);
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player);
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer);
	virtual void GenericPlayerInitialization(AController* C);
	virtual bool MustSpectate_Implementation(APlayerController* NewPlayerController) const;
	int32 GetNumPlayers();
	int32 GetNumSpectators();
	template <class T>
	T* GetGameState() const;
	virtual void ReturnToMainMenuHost();
	virtual void ProcessServerTravel(const FString& URL, bool bAbsolute = false);
};

class AHUD : public AActor
{
public:
	TObjectPtr<APlayerController> PlayerOwner;
	TObjectPtr<UCanvas> Canvas;
	uint8 bShowHUD : 1;

	virtual void DrawHUD();
	virtual void PostRender();
	void DrawRect(FLinearColor RectColor, float ScreenX, float ScreenY, float ScreenW, float ScreenH);
	void DrawText(const FString& Text, FLinearColor TextColor, float ScreenX, float ScreenY, UFont* Font = nullptr, float Scale = 1.f, bool bScalePosition = false);
	void DrawLine(float StartScreenX, float StartScreenY, float EndScreenX, float EndScreenY, FLinearColor LineColor, float LineThickness = 0.f);
	void DrawTexture(UTexture* Texture, float ScreenX, float ScreenY, float ScreenW, float ScreenH, float TextureU, float TextureV, float TextureUWidth,
		float TextureVHeight, FLinearColor TintColor = FLinearColor::White, int32 BlendMode = 0, float Scale = 1.f, bool bScalePosition = false,
		float Rotation = 0.f, FVector2D RotPivot = FVector2D::ZeroVector);
	void GetTextSize(const FString& Text, float& OutWidth, float& OutHeight, UFont* Font = nullptr, float Scale = 1.f) const;
	FVector Project(FVector Location, bool bClampToZeroPlane = true) const;
	void Deproject(float ScreenX, float ScreenY, FVector& WorldPosition, FVector& WorldDirection) const;
	APlayerController* GetOwningPlayerController() const;
	APawn* GetOwningPawn() const;
};

class UCanvas : public UObject
{
public:
	float OrgX = 0.f;
	float OrgY = 0.f;
	float ClipX = 0.f;
	float ClipY = 0.f;
	FColor DrawColor;
	int32 SizeX = 0;
	int32 SizeY = 0;
	FVector Project(FVector Location, bool bClampToNearPlane = true) const;
	void SetDrawColor(FColor const& C);
	void SetDrawColor(uint8 R, uint8 G, uint8 B, uint8 A = 255);
	float DrawText(const UFont* InFont, const FString& InText, float X, float Y, float XScale = 1.f, float YScale = 1.f);
	void TextSize(const UFont* InFont, const FString& InText, float& XL, float& YL, float ScaleX = 1.f, float ScaleY = 1.f);
	void K2_DrawLine(FVector2D ScreenPositionA = FVector2D::ZeroVector, FVector2D ScreenPositionB = FVector2D::ZeroVector, float Thickness = 1.0f,
		FLinearColor RenderColor = FLinearColor::White);
	void K2_DrawBox(FVector2D ScreenPosition, FVector2D ScreenSize, float Thickness = 1.0f, FLinearColor RenderColor = FLinearColor::White);
};

// ----------------------------------------------------------------------------- Engine / instance

typedef TMulticastDelegate<void(UWorld*, UNetDriver*, ENetworkFailure::Type, const FString&)> FOnNetworkFailure;
typedef TMulticastDelegate<void(UWorld*, ETravelFailure::Type, const FString&)> FOnTravelFailure;

class UEngine : public UObject
{
public:
	TObjectPtr<UGameViewportClient> GameViewport;

	UFont* GetTinyFont();
	UFont* GetSmallFont();
	UFont* GetMediumFont();
	UFont* GetLargeFont();
	UFont* GetSubtitleFont();
	FOnNetworkFailure& OnNetworkFailure();
	FOnTravelFailure& OnTravelFailure();
	void AddOnScreenDebugMessage(int32 Key, float TimeToDisplay, FColor DisplayColor, const FString& DebugMessage, bool bNewerOnTop = true,
		const FVector2D& TextScale = FVector2D::UnitVector);
	UWorld* GetWorldFromContextObject(const UObject* Object, EGetWorldErrorMode ErrorMode) const;
	APlayerController* GetFirstLocalPlayerController(const UWorld* InWorld);
	bool IsEditor();
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, class FOutputDevice& Ar);
};

extern UEngine* GEngine;
extern bool GIsEditor;

class UPlayer : public UObject
{
public:
	TObjectPtr<APlayerController> PlayerController;
	APlayerController* GetPlayerController(const UWorld* InWorld) const;
};

class ULocalPlayer : public UPlayer
{
public:
	TObjectPtr<UGameViewportClient> ViewportClient;
	template <class T>
	T* GetSubsystem() const;
	template <class T>
	static T* GetSubsystem(const ULocalPlayer* LocalPlayer);
	UGameInstance* GetGameInstance() const;
	int32 GetControllerId() const;
};

class UGameViewportClient : public UObject
{
public:
	virtual void AddViewportWidgetContent(TSharedRef<SWidget> ViewportContent, const int32 ZOrder = 0);
	virtual void RemoveViewportWidgetContent(TSharedRef<SWidget> ViewportContent);
	virtual void RemoveAllViewportWidgets();
	void GetViewportSize(FVector2D& OutViewportSize) const;
	UWorld* GetWorld() const override;
};

class UGameInstance : public UObject
{
public:
	virtual void Init();
	virtual void Shutdown();
	virtual void OnStart();
	virtual void ReturnToMainMenu();
	virtual UWorld* GetWorld() const override;
	template <class T>
	T* GetSubsystem() const;
	template <class T>
	static T* GetSubsystem(const UGameInstance* GameInstance);
	UEngine* GetEngine() const;
	UGameViewportClient* GetGameViewportClient() const;
	APlayerController* GetFirstLocalPlayerController(const UWorld* World = nullptr) const;
	ULocalPlayer* GetFirstGamePlayer() const;
	const TArray<ULocalPlayer*>& GetLocalPlayers() const;
	FTimerManager& GetTimerManager() const;
	bool IsDedicatedServerInstance() const;
};

class USaveGame : public UObject
{
};

class AWorldSettings : public AInfo
{
public:
	float WorldGravityZ = 0.f;
	TSubclassOf<AGameModeBase> DefaultGameMode;
};

class UNetDriver : public UObject
{
};

class UNetConnection : public UPlayer
{
};

class UInputComponent : public UActorComponent
{
};

// ------------------------------------------------------------------------------------- Libraries

class UBlueprintFunctionLibrary : public UObject
{
};

class UGameplayStatics : public UBlueprintFunctionLibrary
{
public:
	static FString ParseOption(FString Options, const FString& Key);
	static bool HasOption(FString Options, const FString& InKey);
	static int32 GetIntOption(const FString& Options, const FString& Key, int32 DefaultValue);
	static void OpenLevel(const UObject* WorldContextObject, FName LevelName, bool bAbsolute = true, FString Options = FString(TEXT("")));
	static APlayerController* GetPlayerController(const UObject* WorldContextObject, int32 PlayerIndex);
	static ACharacter* GetPlayerCharacter(const UObject* WorldContextObject, int32 PlayerIndex);
	static APawn* GetPlayerPawn(const UObject* WorldContextObject, int32 PlayerIndex);
	static APlayerCameraManager* GetPlayerCameraManager(const UObject* WorldContextObject, int32 PlayerIndex);
	static AGameModeBase* GetGameMode(const UObject* WorldContextObject);
	static AGameStateBase* GetGameState(const UObject* WorldContextObject);
	static UGameInstance* GetGameInstance(const UObject* WorldContextObject);
	static void GetAllActorsOfClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors);
	static double GetTimeSeconds(const UObject* WorldContextObject);
	static double GetRealTimeSeconds(const UObject* WorldContextObject);
	static float GetWorldDeltaSeconds(const UObject* WorldContextObject);
	static bool SaveGameToSlot(USaveGame* SaveGameObject, const FString& SlotName, const int32 UserIndex);
	static USaveGame* LoadGameFromSlot(const FString& SlotName, const int32 UserIndex);
	static bool DoesSaveGameExist(const FString& SlotName, const int32 UserIndex);
	static bool DeleteGameInSlot(const FString& SlotName, const int32 UserIndex);
	static USaveGame* CreateSaveGameObject(TSubclassOf<USaveGame> SaveGameClass);
	static bool ProjectWorldToScreen(APlayerController const* Player, const FVector& WorldPosition, FVector2D& ScreenPosition, bool bPlayerViewportRelative = false);
	static void SetGamePaused(const UObject* WorldContextObject, bool bPaused);
};

class UKismetSystemLibrary : public UBlueprintFunctionLibrary
{
public:
	static void QuitGame(const UObject* WorldContextObject, class APlayerController* SpecificPlayer, TEnumAsByte<EQuitPreference::Type> QuitPreference,
		bool bIgnorePlatformRestrictions);
	static bool IsServer(const UObject* WorldContextObject);
	static bool IsDedicatedServer(const UObject* WorldContextObject);
	static bool IsStandalone(const UObject* WorldContextObject);
	static void ExecuteConsoleCommand(const UObject* WorldContextObject, const FString& Command, class APlayerController* SpecificPlayer = nullptr);
	static FString GetPlatformUserName();
	static void PrintString(const UObject* WorldContextObject, const FString& InString = FString(TEXT("Hello")), bool bPrintToScreen = true, bool bPrintToLog = true,
		FLinearColor TextColor = FLinearColor(0.0f, 0.66f, 1.0f), float Duration = 2.f, const FName Key = NAME_None);
};

// ------------------------------------------------------------------------------- Actor iteration

enum class EActorIteratorFlags
{
	AllActors = 0x00000000,
	OnlyActiveLevels = 0x00000001,
	OnlySelectedActors = 0x00000002,
	SkipPendingKill = 0x00000004
};

template <class T>
class TActorIterator
{
public:
	explicit TActorIterator(const UWorld* InWorld, TSubclassOf<T> InClass = T::StaticClass(), EActorIteratorFlags InFlags = EActorIteratorFlags::SkipPendingKill);
	explicit operator bool() const;
	TActorIterator& operator++();
	T* operator*() const;
	T* operator->() const;
};

template <class T>
class TActorRange
{
public:
	explicit TActorRange(const UWorld* InWorld, TSubclassOf<T> InClass = T::StaticClass(), EActorIteratorFlags InFlags = EActorIteratorFlags::SkipPendingKill);
	TActorIterator<T> begin() const;
	TActorIterator<T> end() const;
};
