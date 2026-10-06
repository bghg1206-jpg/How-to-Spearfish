#pragma once

// InputCore + EnhancedInput stand-ins (UE 5.5 signatures).

#include "UEStub/UEStubEngine.h"

struct FKey
{
	FKey();
	FKey(const FName InName);
	FKey(const TCHAR* InName);
	FName GetFName() const;
	FText GetDisplayName(bool bLongDisplayName = true) const;
	bool IsValid() const;
	bool IsGamepadKey() const;
	bool IsMouseButton() const;
	bool operator==(const FKey& Other) const;
};

struct EKeys
{
	static const FKey AnyKey;
	static const FKey MouseX;
	static const FKey MouseY;
	static const FKey Mouse2D;
	static const FKey MouseScrollUp;
	static const FKey MouseScrollDown;
	static const FKey MouseWheelAxis;
	static const FKey LeftMouseButton;
	static const FKey RightMouseButton;
	static const FKey MiddleMouseButton;
	static const FKey ThumbMouseButton;
	static const FKey ThumbMouseButton2;
	static const FKey BackSpace;
	static const FKey Tab;
	static const FKey Enter;
	static const FKey Pause;
	static const FKey CapsLock;
	static const FKey Escape;
	static const FKey SpaceBar;
	static const FKey PageUp;
	static const FKey PageDown;
	static const FKey End;
	static const FKey Home;
	static const FKey Left;
	static const FKey Up;
	static const FKey Right;
	static const FKey Down;
	static const FKey Insert;
	static const FKey Delete;
	static const FKey Zero;
	static const FKey One;
	static const FKey Two;
	static const FKey Three;
	static const FKey Four;
	static const FKey Five;
	static const FKey Six;
	static const FKey Seven;
	static const FKey Eight;
	static const FKey Nine;
	static const FKey A;
	static const FKey B;
	static const FKey C;
	static const FKey D;
	static const FKey E;
	static const FKey F;
	static const FKey G;
	static const FKey H;
	static const FKey I;
	static const FKey J;
	static const FKey K;
	static const FKey L;
	static const FKey M;
	static const FKey N;
	static const FKey O;
	static const FKey P;
	static const FKey Q;
	static const FKey R;
	static const FKey S;
	static const FKey T;
	static const FKey U;
	static const FKey V;
	static const FKey W;
	static const FKey X;
	static const FKey Y;
	static const FKey Z;
	static const FKey F1;
	static const FKey F2;
	static const FKey F3;
	static const FKey F4;
	static const FKey F5;
	static const FKey F6;
	static const FKey F7;
	static const FKey F8;
	static const FKey F9;
	static const FKey F10;
	static const FKey F11;
	static const FKey F12;
	static const FKey LeftShift;
	static const FKey RightShift;
	static const FKey LeftControl;
	static const FKey RightControl;
	static const FKey LeftAlt;
	static const FKey RightAlt;
	static const FKey Gamepad_Left2D;
	static const FKey Gamepad_LeftX;
	static const FKey Gamepad_LeftY;
	static const FKey Gamepad_Right2D;
	static const FKey Gamepad_RightX;
	static const FKey Gamepad_RightY;
	static const FKey Gamepad_LeftTriggerAxis;
	static const FKey Gamepad_RightTriggerAxis;
	static const FKey Gamepad_LeftThumbstick;
	static const FKey Gamepad_RightThumbstick;
	static const FKey Gamepad_Special_Left;
	static const FKey Gamepad_Special_Right;
	static const FKey Gamepad_FaceButton_Bottom;
	static const FKey Gamepad_FaceButton_Right;
	static const FKey Gamepad_FaceButton_Left;
	static const FKey Gamepad_FaceButton_Top;
	static const FKey Gamepad_LeftShoulder;
	static const FKey Gamepad_RightShoulder;
	static const FKey Gamepad_LeftTrigger;
	static const FKey Gamepad_RightTrigger;
	static const FKey Gamepad_DPad_Up;
	static const FKey Gamepad_DPad_Down;
	static const FKey Gamepad_DPad_Right;
	static const FKey Gamepad_DPad_Left;
};

enum class EInputActionValueType : uint8
{
	Boolean,
	Axis1D,
	Axis2D,
	Axis3D
};

enum class ETriggerEvent : uint8
{
	None = 0x0,
	Triggered = 0x1,
	Started = 0x2,
	Ongoing = 0x4,
	Canceled = 0x8,
	Completed = 0x10
};

enum class EInputAxisSwizzle : uint8
{
	YXZ,
	ZYX,
	XZY,
	YZX,
	ZXY
};

struct FInputActionValue
{
	using Axis1D = float;
	using Axis2D = FVector2D;
	using Axis3D = FVector;

	FInputActionValue();
	FInputActionValue(bool bInValue);
	FInputActionValue(Axis1D InValue);
	FInputActionValue(Axis2D InValue);
	FInputActionValue(Axis3D InValue);

	template <typename T>
	T Get() const;
	float GetMagnitude() const;
	float GetMagnitudeSq() const;
	EInputActionValueType GetValueType() const;
	bool IsNonZero(float Tolerance = UE_KINDA_SMALL_NUMBER) const;
};

struct FInputActionInstance
{
	FInputActionValue GetValue() const;
	ETriggerEvent GetTriggerEvent() const;
	float GetElapsedTime() const;
	const class UInputAction* GetSourceAction() const;
};

class UInputModifier : public UObject
{
};

class UInputModifierNegate : public UInputModifier
{
public:
	bool bX = true;
	bool bY = true;
	bool bZ = true;
};

class UInputModifierSwizzleAxis : public UInputModifier
{
public:
	EInputAxisSwizzle Order = EInputAxisSwizzle::YXZ;
};

class UInputModifierScalar : public UInputModifier
{
public:
	FVector Scalar = FVector::OneVector;
};

class UInputModifierDeadZone : public UInputModifier
{
public:
	float LowerThreshold = 0.2f;
	float UpperThreshold = 1.f;
};

class UInputTrigger : public UObject
{
};

class UInputTriggerPressed : public UInputTrigger
{
};

class UInputTriggerReleased : public UInputTrigger
{
};

class UInputTriggerHold : public UInputTrigger
{
public:
	float HoldTimeThreshold = 1.f;
};

class UInputAction : public UObject
{
public:
	EInputActionValueType ValueType = EInputActionValueType::Boolean;
	bool bConsumeInput = true;
	bool bTriggerWhenPaused = false;
	FText ActionDescription;
	TArray<TObjectPtr<UInputTrigger>> Triggers;
	TArray<TObjectPtr<UInputModifier>> Modifiers;
};

struct FEnhancedActionKeyMapping
{
	TArray<TObjectPtr<UInputTrigger>> Triggers;
	TArray<TObjectPtr<UInputModifier>> Modifiers;
	TObjectPtr<const UInputAction> Action;
	FKey Key;
};

class UInputMappingContext : public UObject
{
public:
	FText ContextDescription;
	const TArray<FEnhancedActionKeyMapping>& GetMappings() const;
	FEnhancedActionKeyMapping& GetMapping(const int32 MappingIndex);
	FEnhancedActionKeyMapping& MapKey(const UInputAction* Action, FKey ToKey);
	void UnmapKey(const UInputAction* Action, FKey Key);
	void UnmapAllKeysFromAction(const UInputAction* Action);
	void UnmapAll();
};

struct FModifyContextOptions
{
	uint8 bIgnoreAllPressedKeysUntilRelease : 1;
	uint8 bForceImmediately : 1;
	uint8 bNotifyUserSettings : 1;
	FModifyContextOptions();
};

class IEnhancedInputSubsystemInterface
{
public:
	virtual ~IEnhancedInputSubsystemInterface();
	virtual void AddMappingContext(const UInputMappingContext* MappingContext, int32 Priority, const FModifyContextOptions& Options = FModifyContextOptions());
	virtual void RemoveMappingContext(const UInputMappingContext* MappingContext, const FModifyContextOptions& Options = FModifyContextOptions());
	virtual void ClearAllMappings();
	virtual bool HasMappingContext(const UInputMappingContext* MappingContext) const;
	virtual bool HasMappingContext(const UInputMappingContext* MappingContext, int32& OutFoundPriority) const;
	virtual TArray<FKey> QueryKeysMappedToAction(const UInputAction* Action) const;
};

class ULocalPlayerSubsystem : public USubsystem
{
public:
	ULocalPlayer* GetLocalPlayer() const;
	template <class T>
	T* GetLocalPlayer() const;
};

class UEnhancedInputLocalPlayerSubsystem : public ULocalPlayerSubsystem, public IEnhancedInputSubsystemInterface
{
};

typedef TDelegate<void()> FEnhancedInputActionHandlerSignature;
typedef TDelegate<void(const FInputActionValue&)> FEnhancedInputActionHandlerValueSignature;
typedef TDelegate<void(const FInputActionInstance&)> FEnhancedInputActionHandlerInstanceSignature;

struct FEnhancedInputActionEventBinding
{
	const UInputAction* GetAction() const;
	ETriggerEvent GetTriggerEvent() const;
	uint32 GetHandle() const;
};

class UEnhancedInputComponent : public UInputComponent
{
public:
	template <class UserClass, typename... VarTypes>
	FEnhancedInputActionEventBinding& BindAction(const UInputAction* Action, ETriggerEvent TriggerEvent, UserClass* Object,
		typename FEnhancedInputActionHandlerSignature::template TMethodPtr<UserClass, VarTypes...> Func, VarTypes... Vars);
	template <class UserClass, typename... VarTypes>
	FEnhancedInputActionEventBinding& BindAction(const UInputAction* Action, ETriggerEvent TriggerEvent, UserClass* Object,
		typename FEnhancedInputActionHandlerValueSignature::template TMethodPtr<UserClass, VarTypes...> Func, VarTypes... Vars);
	template <class UserClass, typename... VarTypes>
	FEnhancedInputActionEventBinding& BindAction(const UInputAction* Action, ETriggerEvent TriggerEvent, UserClass* Object,
		typename FEnhancedInputActionHandlerInstanceSignature::template TMethodPtr<UserClass, VarTypes...> Func, VarTypes... Vars);
	template <class UserClass>
	FEnhancedInputActionEventBinding& BindAction(const UInputAction* Action, ETriggerEvent TriggerEvent, UserClass* Object, const FName FunctionName);
	bool RemoveBindingByHandle(const uint32 Handle);
	void ClearActionEventBindings();
	void ClearActionBindings();
};
