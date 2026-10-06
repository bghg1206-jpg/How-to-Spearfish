#pragma once

// Declaration-level stand-ins for the UObject layer of Unreal Engine 5.5 (object model, pointers, casts,
// delegates, timers, replication macros). Used only for `clang -fsyntax-only` checks of the game module:
// signatures follow the real engine so that call-site mistakes are reported, but nothing here runs.

#include "UEStub/UEStubCore.h"

#include <type_traits>

class UClass;
class UObject;
class UWorld;
class ULevel;
class UPackage;
class UScriptStruct;
class UFunction;
class UEnum;
class UGameInstance;
class FProperty;
struct FPropertyChangedEvent;

enum EObjectFlags : uint32
{
	RF_NoFlags = 0,
	RF_Public = 1,
	RF_Standalone = 2,
	RF_Transactional = 8,
	RF_ClassDefaultObject = 16,
	RF_ArchetypeObject = 32,
	RF_Transient = 64
};
inline EObjectFlags operator|(EObjectFlags A, EObjectFlags B) { return static_cast<EObjectFlags>(static_cast<uint32>(A) | static_cast<uint32>(B)); }

enum class EGetWorldErrorMode
{
	ReturnNull,
	LogAndReturnNull,
	Assert
};

namespace EEndPlayReason
{
	enum Type : int
	{
		Destroyed,
		LevelTransition,
		EndPlayInEditor,
		RemovedFromWorld,
		Quit
	};
}

enum ENetRole : int
{
	ROLE_None,
	ROLE_SimulatedProxy,
	ROLE_AutonomousProxy,
	ROLE_Authority,
	ROLE_MAX
};

enum ENetMode : int
{
	NM_Standalone,
	NM_DedicatedServer,
	NM_ListenServer,
	NM_Client,
	NM_MAX
};

enum ELifetimeCondition : int
{
	COND_None = 0,
	COND_InitialOnly = 1,
	COND_OwnerOnly = 2,
	COND_SkipOwner = 3,
	COND_SimulatedOnly = 4,
	COND_AutonomousOnly = 5,
	COND_SimulatedOrPhysics = 6,
	COND_InitialOrOwner = 7,
	COND_Custom = 8,
	COND_ReplayOrOwner = 9,
	COND_ReplayOnly = 10,
	COND_SimulatedOnlyNoReplay = 11,
	COND_SimulatedOrPhysicsNoReplay = 12,
	COND_SkipReplay = 13,
	COND_Dynamic = 14,
	COND_Never = 15
};

enum ELifetimeRepNotifyCondition
{
	REPNOTIFY_OnChanged = 0,
	REPNOTIFY_Always = 1
};

struct FLifetimeProperty
{
	uint16 RepIndex = 0;
	ELifetimeCondition Condition = COND_None;
};

struct FDoRepLifetimeParams
{
	ELifetimeCondition Condition = COND_None;
	ELifetimeRepNotifyCondition RepNotifyCondition = REPNOTIFY_OnChanged;
	bool bIsPushBased = false;
};

// The macros check that the property exists on the class (a typo is a compile error in UE too).
#define DOREPLIFETIME(c, v) ((void)sizeof(((c*)nullptr)->v), (void)OutLifetimeProps)
#define DOREPLIFETIME_CONDITION(c, v, cond) ((void)sizeof(((c*)nullptr)->v), (void)static_cast<ELifetimeCondition>(cond), (void)OutLifetimeProps)
#define DOREPLIFETIME_CONDITION_NOTIFY(c, v, cond, rncond) \
	((void)sizeof(((c*)nullptr)->v), (void)static_cast<ELifetimeCondition>(cond), (void)static_cast<ELifetimeRepNotifyCondition>(rncond), (void)OutLifetimeProps)
#define DOREPLIFETIME_WITH_PARAMS(c, v, params) ((void)sizeof(((c*)nullptr)->v), (void)static_cast<const FDoRepLifetimeParams&>(params), (void)OutLifetimeProps)
#define DOREPLIFETIME_WITH_PARAMS_FAST(c, v, params) DOREPLIFETIME_WITH_PARAMS(c, v, params)

// ------------------------------------------------------------------------------------ Object model

struct FObjectInitializer
{
	template <class T>
	FObjectInitializer const& SetDefaultSubobjectClass(FName SubobjectName) const;
	template <class T>
	FObjectInitializer const& DoNotCreateDefaultSubobject(FName SubobjectName) const;
	UObject* GetObj() const;
	static FObjectInitializer& Get();
};

class UObject
{
public:
	typedef UObject ThisClass;
	UObject();
	explicit UObject(const FObjectInitializer& ObjectInitializer);
	virtual ~UObject();
	static UClass* StaticClass();

	virtual UWorld* GetWorld() const;
	FString GetName() const;
	FName GetFName() const;
	uint32 GetUniqueID() const;
	FString GetPathName(const UObject* StopOuter = nullptr) const;
	FString GetFullName(const UObject* StopOuter = nullptr) const;
	UObject* GetOuter() const;
	template <class T>
	T* GetTypedOuter() const;
	UClass* GetClass() const;
	UPackage* GetPackage() const;
	bool IsA(const UClass* SomeBase) const;
	template <class T>
	bool IsA() const;
	template <class T>
	bool Implements() const;
	bool HasAnyFlags(EObjectFlags FlagsToCheck) const;
	void SetFlags(EObjectFlags NewFlags);
	void ClearFlags(EObjectFlags NewFlags);
	bool IsTemplate(EObjectFlags TemplateTypes = RF_ArchetypeObject) const;
	bool IsValidLowLevel() const;
	void MarkAsGarbage();
	void AddToRoot();
	void RemoveFromRoot();
	bool Rename(const TCHAR* NewName = nullptr, UObject* NewOuter = nullptr, uint32 Flags = 0);
	bool IsInGameThread() const;

	virtual void PostInitProperties();
	virtual void PostLoad();
	virtual void BeginDestroy();
	virtual void FinishDestroy();
	virtual void PostInitializeComponents();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;
	virtual bool IsSupportedForNetworking() const;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, struct FFrame* Stack);
	virtual int32 GetFunctionCallspace(UFunction* Function, struct FFrame* Stack);
	void SaveConfig(uint64 Flags = 0, const TCHAR* Filename = nullptr);
	void LoadConfig(UClass* ConfigClass = nullptr, const TCHAR* Filename = nullptr, uint32 PropagationFlags = 0, FProperty* PropertyToLoad = nullptr);
	void TryUpdateDefaultConfigFile(const FString& SpecificFileLocation = TEXT(""), bool bWarnIfFail = true);
	bool ProcessConsoleExec(const TCHAR* Cmd, class FOutputDevice& Ar, UObject* Executor);
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
#endif
};

class UField : public UObject
{
};

class UStruct : public UField
{
public:
	UStruct* GetSuperStruct() const;
	bool IsChildOf(const UStruct* SomeBase) const;
};

class UScriptStruct : public UStruct
{
};

class UClass : public UStruct
{
public:
	UObject* GetDefaultObject(bool bCreateIfNeeded = true) const;
	template <class T>
	T* GetDefaultObject() const;
	template <class T>
	bool IsChildOf() const;
	using UStruct::IsChildOf;
	UClass* GetSuperClass() const;
	bool HasAnyClassFlags(uint32 FlagsToCheck) const;
};

class UFunction : public UStruct
{
};

class UEnum : public UField
{
public:
	FString GetNameStringByValue(int64 InValue) const;
	FText GetDisplayNameTextByValue(int64 InValue) const;
	int64 GetValueByNameString(const FString& SearchString) const;
};

class UPackage : public UObject
{
};

class UInterface : public UObject
{
};

class IInterface
{
};

template <typename T>
UEnum* StaticEnum();
template <typename T>
UScriptStruct* StaticStruct();

UPackage* GetTransientPackage();

inline bool IsValid(const UObject* Test) { return Test != nullptr; }

template <class T>
class TObjectPtr
{
public:
	TObjectPtr() = default;
	TObjectPtr(std::nullptr_t) {}
	template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
	TObjectPtr(U* In) : Ptr(In) {}
	template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
	TObjectPtr(const TObjectPtr<U>& In) : Ptr(In.Get()) {}
	template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
	TObjectPtr& operator=(U* In) { Ptr = In; return *this; }
	TObjectPtr& operator=(std::nullptr_t) { Ptr = nullptr; return *this; }

	T* Get() const { return Ptr; }
	operator T*() const { return Ptr; }
	T* operator->() const { return Ptr; }
	T& operator*() const { return *Ptr; }
	explicit operator bool() const { return Ptr != nullptr; }
	bool operator!() const { return Ptr == nullptr; }
	template <class U>
	bool operator==(const TObjectPtr<U>& Other) const { return Ptr == Other.Get(); }
	template <class U>
	bool operator!=(const TObjectPtr<U>& Other) const { return Ptr != Other.Get(); }
	template <class U>
	bool operator==(U* Other) const { return Ptr == Other; }
	template <class U>
	bool operator!=(U* Other) const { return Ptr != Other; }
	bool operator==(std::nullptr_t) const { return Ptr == nullptr; }
	bool operator!=(std::nullptr_t) const { return Ptr != nullptr; }

private:
	T* Ptr = nullptr;
};
template <class T>
uint32 GetTypeHash(const TObjectPtr<T>& P) { return GetTypeHash(static_cast<const void*>(P.Get())); }

template <class T>
class TWeakObjectPtr
{
public:
	TWeakObjectPtr() = default;
	TWeakObjectPtr(std::nullptr_t) {}
	template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
	TWeakObjectPtr(U* In) : Ptr(In) {}
	template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
	TWeakObjectPtr(const TObjectPtr<U>& In) : Ptr(In.Get()) {}
	template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
	TWeakObjectPtr(const TWeakObjectPtr<U>& In) : Ptr(In.Get()) {}

	T* Get() const { return Ptr; }
	bool IsValid() const { return Ptr != nullptr; }
	bool IsStale() const { return false; }
	void Reset() { Ptr = nullptr; }
	T* operator->() const { return Ptr; }
	T& operator*() const { return *Ptr; }
	explicit operator bool() const { return Ptr != nullptr; }
	template <class U>
	bool operator==(const TWeakObjectPtr<U>& Other) const { return Ptr == Other.Get(); }
	template <class U>
	bool operator!=(const TWeakObjectPtr<U>& Other) const { return Ptr != Other.Get(); }
	template <class U>
	bool operator==(const U* Other) const { return Ptr == Other; }
	template <class U>
	bool operator!=(const U* Other) const { return Ptr != Other; }

private:
	T* Ptr = nullptr;
};
template <class T>
uint32 GetTypeHash(const TWeakObjectPtr<T>& P) { return GetTypeHash(static_cast<const void*>(P.Get())); }

struct FSoftObjectPath
{
	FSoftObjectPath();
	FSoftObjectPath(const FString& Path);
	FSoftObjectPath(const TCHAR* Path);
	FSoftObjectPath(const UObject* Object);
	bool IsValid() const;
	bool IsNull() const;
	FString ToString() const;
	FString GetAssetName() const;
	UObject* TryLoad() const;
	UObject* ResolveObject() const;
};

template <class T>
class TSoftObjectPtr
{
public:
	TSoftObjectPtr();
	TSoftObjectPtr(const FSoftObjectPath& Path);
	TSoftObjectPtr(T* Object);
	bool IsNull() const;
	bool IsValid() const;
	bool IsPending() const;
	T* Get() const;
	T* LoadSynchronous() const;
	FSoftObjectPath ToSoftObjectPath() const;
	FString ToString() const;
	FString GetAssetName() const;
	FString GetLongPackageName() const;
};

template <class T>
class TSoftClassPtr
{
public:
	TSoftClassPtr();
	TSoftClassPtr(const FSoftObjectPath& Path);
	bool IsNull() const;
	UClass* Get() const;
	UClass* LoadSynchronous() const;
	FSoftObjectPath ToSoftObjectPath() const;
};

template <class T>
class TSubclassOf
{
public:
	TSubclassOf();
	TSubclassOf(UClass* From);
	template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
	TSubclassOf(const TSubclassOf<U>& From);
	TSubclassOf& operator=(UClass* From);
	UClass* Get() const;
	operator UClass*() const;
	UClass* operator->() const;
	UClass* operator*() const;
	T* GetDefaultObject() const;
};

template <class T>
T* NewObject(UObject* Outer = (UObject*)GetTransientPackage());
template <class T>
T* NewObject(UObject* Outer, FName Name, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool bCopyTransientsFromClassDefaults = false);
template <class T>
T* NewObject(UObject* Outer, const UClass* Class, FName Name = NAME_None, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr,
	bool bCopyTransientsFromClassDefaults = false);
template <class T>
const T* GetDefault();
template <class T>
const T* GetDefault(UClass* Class);
template <class T>
T* GetMutableDefault();
template <class T>
T* LoadObject(UObject* Outer, const TCHAR* Name, const TCHAR* Filename = nullptr, uint32 LoadFlags = 0, void* Sandbox = nullptr);
template <class T>
T* FindObject(UObject* Outer, const TCHAR* Name, bool ExactClass = false);
UObject* StaticLoadObject(UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename = nullptr, uint32 LoadFlags = 0);

// Cast copies const-ness from the source like the real TCastImpl.
template <class To, class From>
auto Cast(From* Src) -> std::conditional_t<std::is_const_v<From>, const To*, To*>;
template <class To, class From>
auto Cast(const TObjectPtr<From>& Src) -> std::conditional_t<std::is_const_v<From>, const To*, To*>;
template <class To, class From>
auto CastChecked(From* Src) -> std::conditional_t<std::is_const_v<From>, const To*, To*>;
template <class To, class From>
auto CastChecked(const TObjectPtr<From>& Src) -> std::conditional_t<std::is_const_v<From>, const To*, To*>;
template <class To, class From>
To* ExactCast(From* Src);

namespace ConstructorHelpers
{
	template <class T>
	struct FObjectFinder
	{
		T* Object = nullptr;
		explicit FObjectFinder(const TCHAR* ObjectToFind, uint32 InLoadFlags = 0);
		bool Succeeded() const;
	};
	template <class T>
	struct FObjectFinderOptional
	{
		explicit FObjectFinderOptional(const TCHAR* ObjectToFind, uint32 InLoadFlags = 0);
		T* Get();
		bool Succeeded();
	};
	template <class T>
	struct FClassFinder
	{
		TSubclassOf<T> Class;
		explicit FClassFinder(const TCHAR* ClassToFind);
		bool Succeeded();
	};
}

// --------------------------------------------------------------------------------------- Delegates

struct FDelegateHandle
{
	bool IsValid() const;
	void Reset();
	bool operator==(const FDelegateHandle& Other) const;
	bool operator!=(const FDelegateHandle& Other) const;
};

template <typename Signature>
class TDelegate;

template <typename RetType, typename... ParamTypes>
class TDelegate<RetType(ParamTypes...)>
{
public:
	template <class UserClass, typename... VarTypes>
	using TMethodPtr = RetType (UserClass::*)(ParamTypes..., VarTypes...);
	template <class UserClass, typename... VarTypes>
	using TConstMethodPtr = RetType (UserClass::*)(ParamTypes..., VarTypes...) const;

	TDelegate();
	TDelegate(std::nullptr_t);

	template <class UserClass, typename... VarTypes>
	static TDelegate CreateUObject(UserClass* InUserObject, TMethodPtr<std::remove_const_t<UserClass>, std::decay_t<VarTypes>...> InFunc, VarTypes&&... Vars);
	template <class UserClass, typename... VarTypes>
	static TDelegate CreateUObject(UserClass* InUserObject, TConstMethodPtr<std::remove_const_t<UserClass>, std::decay_t<VarTypes>...> InFunc, VarTypes&&... Vars);
	template <class UserClass, typename... VarTypes>
	static TDelegate CreateSP(UserClass* InUserObject, TMethodPtr<std::remove_const_t<UserClass>, std::decay_t<VarTypes>...> InFunc, VarTypes&&... Vars);
	template <class UserClass, typename... VarTypes>
	static TDelegate CreateSP(UserClass* InUserObject, TConstMethodPtr<std::remove_const_t<UserClass>, std::decay_t<VarTypes>...> InFunc, VarTypes&&... Vars);
	template <class UserClass, typename... VarTypes>
	static TDelegate CreateRaw(UserClass* InUserObject, TMethodPtr<std::remove_const_t<UserClass>, std::decay_t<VarTypes>...> InFunc, VarTypes&&... Vars);
	template <typename FunctorType, typename... VarTypes>
	static TDelegate CreateLambda(FunctorType&& InFunctor, VarTypes&&... Vars) { return TDelegate(); }
	template <class UserClass, typename FunctorType, typename... VarTypes>
	static TDelegate CreateWeakLambda(UserClass* InUserObject, FunctorType&& InFunctor, VarTypes&&... Vars) { return TDelegate(); }
	template <typename... VarTypes>
	static TDelegate CreateStatic(RetType (*InFunc)(ParamTypes..., std::decay_t<VarTypes>...), VarTypes&&... Vars);
	template <class UserClass>
	static TDelegate CreateUFunction(UserClass* InUserObject, const FName& InFunctionName);

	template <class UserClass, typename... VarTypes>
	void BindUObject(UserClass* InUserObject, TMethodPtr<std::remove_const_t<UserClass>, std::decay_t<VarTypes>...> InFunc, VarTypes&&... Vars);
	template <class UserClass, typename... VarTypes>
	void BindUObject(UserClass* InUserObject, TConstMethodPtr<std::remove_const_t<UserClass>, std::decay_t<VarTypes>...> InFunc, VarTypes&&... Vars);
	template <class UserClass, typename... VarTypes>
	void BindSP(UserClass* InUserObject, TMethodPtr<std::remove_const_t<UserClass>, std::decay_t<VarTypes>...> InFunc, VarTypes&&... Vars);
	template <class UserClass, typename... VarTypes>
	void BindRaw(UserClass* InUserObject, TMethodPtr<std::remove_const_t<UserClass>, std::decay_t<VarTypes>...> InFunc, VarTypes&&... Vars);
	template <typename FunctorType, typename... VarTypes>
	void BindLambda(FunctorType&& InFunctor, VarTypes&&... Vars) {}
	template <class UserClass, typename FunctorType, typename... VarTypes>
	void BindWeakLambda(UserClass* InUserObject, FunctorType&& InFunctor, VarTypes&&... Vars) {}
	template <typename... VarTypes>
	void BindStatic(RetType (*InFunc)(ParamTypes..., std::decay_t<VarTypes>...), VarTypes&&... Vars);
	template <class UserClass>
	void BindUFunction(UserClass* InUserObject, const FName& InFunctionName);

	bool IsBound() const;
	RetType Execute(ParamTypes... Params) const;
	template <typename R = RetType, typename = std::enable_if_t<std::is_void_v<R>>>
	bool ExecuteIfBound(ParamTypes... Params) const;
	void Unbind();
	FDelegateHandle GetHandle() const;
};

template <typename Signature>
class TMulticastDelegate;

template <typename... ParamTypes>
class TMulticastDelegate<void(ParamTypes...)>
{
public:
	using FDelegate = TDelegate<void(ParamTypes...)>;
	template <class UserClass, typename... VarTypes>
	using TMethodPtr = void (UserClass::*)(ParamTypes..., VarTypes...);
	template <class UserClass, typename... VarTypes>
	using TConstMethodPtr = void (UserClass::*)(ParamTypes..., VarTypes...) const;

	FDelegateHandle Add(const FDelegate& InNewDelegate);
	template <class UserClass, typename... VarTypes>
	FDelegateHandle AddUObject(UserClass* InUserObject, TMethodPtr<std::remove_const_t<UserClass>, std::decay_t<VarTypes>...> InFunc, VarTypes&&... Vars);
	template <class UserClass, typename... VarTypes>
	FDelegateHandle AddUObject(UserClass* InUserObject, TConstMethodPtr<std::remove_const_t<UserClass>, std::decay_t<VarTypes>...> InFunc, VarTypes&&... Vars);
	template <class UserClass, typename... VarTypes>
	FDelegateHandle AddSP(UserClass* InUserObject, TMethodPtr<std::remove_const_t<UserClass>, std::decay_t<VarTypes>...> InFunc, VarTypes&&... Vars);
	template <class UserClass, typename... VarTypes>
	FDelegateHandle AddRaw(UserClass* InUserObject, TMethodPtr<std::remove_const_t<UserClass>, std::decay_t<VarTypes>...> InFunc, VarTypes&&... Vars);
	template <typename FunctorType, typename... VarTypes>
	FDelegateHandle AddLambda(FunctorType&& InFunctor, VarTypes&&... Vars) { return FDelegateHandle(); }
	template <class UserClass, typename FunctorType, typename... VarTypes>
	FDelegateHandle AddWeakLambda(UserClass* InUserObject, FunctorType&& InFunctor, VarTypes&&... Vars) { return FDelegateHandle(); }
	template <typename... VarTypes>
	FDelegateHandle AddStatic(void (*InFunc)(ParamTypes..., std::decay_t<VarTypes>...), VarTypes&&... Vars);
	template <class UserClass>
	FDelegateHandle AddUFunction(UserClass* InUserObject, const FName& InFunctionName);

	bool Remove(FDelegateHandle Handle);
	int32 RemoveAll(const void* InUserObject);
	void Clear();
	bool IsBound() const;
	bool IsBoundToObject(const void* InUserObject) const;
	void Broadcast(ParamTypes... Params) const;
};

#define FUNC_DECLARE_DELEGATE(DelegateName, ReturnType, ...) typedef TDelegate<ReturnType(__VA_ARGS__)> DelegateName;
#define FUNC_DECLARE_MULTICAST_DELEGATE(MulticastDelegateName, ReturnType, ...) typedef TMulticastDelegate<ReturnType(__VA_ARGS__)> MulticastDelegateName;

#define DECLARE_DELEGATE(DelegateName) FUNC_DECLARE_DELEGATE(DelegateName, void)
#define DECLARE_DELEGATE_OneParam(DelegateName, P1) FUNC_DECLARE_DELEGATE(DelegateName, void, P1)
#define DECLARE_DELEGATE_TwoParams(DelegateName, P1, P2) FUNC_DECLARE_DELEGATE(DelegateName, void, P1, P2)
#define DECLARE_DELEGATE_ThreeParams(DelegateName, P1, P2, P3) FUNC_DECLARE_DELEGATE(DelegateName, void, P1, P2, P3)
#define DECLARE_DELEGATE_RetVal(R, DelegateName) FUNC_DECLARE_DELEGATE(DelegateName, R)
#define DECLARE_DELEGATE_RetVal_OneParam(R, DelegateName, P1) FUNC_DECLARE_DELEGATE(DelegateName, R, P1)
#define DECLARE_DELEGATE_RetVal_TwoParams(R, DelegateName, P1, P2) FUNC_DECLARE_DELEGATE(DelegateName, R, P1, P2)
#define DECLARE_MULTICAST_DELEGATE(DelegateName) FUNC_DECLARE_MULTICAST_DELEGATE(DelegateName, void)
#define DECLARE_MULTICAST_DELEGATE_OneParam(DelegateName, P1) FUNC_DECLARE_MULTICAST_DELEGATE(DelegateName, void, P1)
#define DECLARE_MULTICAST_DELEGATE_TwoParams(DelegateName, P1, P2) FUNC_DECLARE_MULTICAST_DELEGATE(DelegateName, void, P1, P2)
#define DECLARE_MULTICAST_DELEGATE_ThreeParams(DelegateName, P1, P2, P3) FUNC_DECLARE_MULTICAST_DELEGATE(DelegateName, void, P1, P2, P3)
#define DECLARE_MULTICAST_DELEGATE_FourParams(DelegateName, P1, P2, P3, P4) FUNC_DECLARE_MULTICAST_DELEGATE(DelegateName, void, P1, P2, P3, P4)

typedef TDelegate<void()> FSimpleDelegate;
typedef TMulticastDelegate<void()> FSimpleMulticastDelegate;

// ------------------------------------------------------------------------------------------- Timers

struct FTimerHandle
{
	bool IsValid() const;
	void Invalidate();
	bool operator==(const FTimerHandle& Other) const;
};
typedef TDelegate<void()> FTimerDelegate;

class FTimerManager
{
public:
	template <class UserClass>
	void SetTimer(FTimerHandle& InOutHandle, UserClass* InObj, typename FTimerDelegate::template TMethodPtr<UserClass> InTimerMethod, float InRate,
		bool InbLoop = false, float InFirstDelay = -1.f);
	template <class UserClass>
	void SetTimer(FTimerHandle& InOutHandle, UserClass* InObj, typename FTimerDelegate::template TConstMethodPtr<UserClass> InTimerMethod, float InRate,
		bool InbLoop = false, float InFirstDelay = -1.f);
	void SetTimer(FTimerHandle& InOutHandle, FTimerDelegate const& InDelegate, float InRate, bool InbLoop, float InFirstDelay = -1.f);
	void SetTimer(FTimerHandle& InOutHandle, TFunction<void(void)>&& Callback, float InRate, bool InbLoop, float InFirstDelay = -1.f);
	FTimerHandle SetTimerForNextTick(FTimerDelegate const& InDelegate);
	FTimerHandle SetTimerForNextTick(TFunction<void(void)>&& Callback);
	void ClearTimer(FTimerHandle& InHandle);
	void ClearAllTimersForObject(void const* Object);
	void PauseTimer(FTimerHandle InHandle);
	void UnPauseTimer(FTimerHandle InHandle);
	bool IsTimerActive(FTimerHandle InHandle) const;
	bool TimerExists(FTimerHandle InHandle) const;
	float GetTimerRemaining(FTimerHandle InHandle) const;
	float GetTimerElapsed(FTimerHandle InHandle) const;
};

// ----------------------------------------------------------------------------------- Subsystems

class USubsystem : public UObject
{
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const;
	virtual void Initialize(class FSubsystemCollectionBase& Collection);
	virtual void Deinitialize();
};

class FSubsystemCollectionBase
{
public:
	USubsystem* InitializeDependency(TSubclassOf<USubsystem> SubsystemClass);
	template <class TSubsystemClass>
	TSubsystemClass* InitializeDependency();
};

class UGameInstanceSubsystem : public USubsystem
{
public:
	UGameInstance* GetGameInstance() const;
};

class UWorldSubsystem : public USubsystem
{
public:
	virtual void PostInitialize();
	virtual void OnWorldBeginPlay(UWorld& InWorld);
	virtual bool DoesSupportWorldType(int32 WorldType) const;
	UWorld* GetWorldRef() const;
};

enum class ETickableTickType : uint8
{
	Conditional,
	Always,
	Never
};

class FTickableGameObject
{
public:
	virtual ~FTickableGameObject();
	virtual void Tick(float DeltaTime) = 0;
	virtual ETickableTickType GetTickableTickType() const;
	virtual bool IsTickable() const;
	virtual bool IsTickableWhenPaused() const;
	virtual bool IsTickableInEditor() const;
	virtual struct TStatId GetStatId() const = 0;
	virtual UWorld* GetTickableGameObjectWorld() const;
};

struct TStatId
{
};
#define RETURN_QUICK_DECLARE_CYCLE_STAT(StatId, Group) return TStatId();
#define DECLARE_CYCLE_STAT(CounterName, StatId, GroupId)
#define SCOPE_CYCLE_COUNTER(Stat)
#define QUICK_SCOPE_CYCLE_COUNTER(Stat)
#define TRACE_CPUPROFILER_EVENT_SCOPE(Name)

class UTickableWorldSubsystem : public UWorldSubsystem, public FTickableGameObject
{
public:
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual TStatId GetStatId() const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	bool IsInitialized() const;
};

class UDeveloperSettings : public UObject
{
protected:
	FName CategoryName;
	FName SectionName;

public:
	virtual FName GetContainerName() const;
	virtual FName GetCategoryName() const;
	virtual FName GetSectionName() const;
#if WITH_EDITOR
	virtual FText GetSectionText() const;
	virtual FText GetSectionDescription() const;
#endif
};
