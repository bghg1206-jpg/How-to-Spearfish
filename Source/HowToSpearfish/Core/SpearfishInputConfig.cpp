#include "Core/SpearfishInputConfig.h"

#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

namespace SpearfishInputPrivate
{
	void Map(UInputMappingContext* Context, UInputAction* Action, const FKey& Key, bool bSwizzle = false, bool bNegate = false, float Scale = 1.f)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		if (bSwizzle)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Context);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
		}
		if (!FMath::IsNearlyEqual(Scale, 1.f))
		{
			UInputModifierScalar* Scalar = NewObject<UInputModifierScalar>(Context);
			Scalar->Scalar = FVector(Scale, Scale, Scale);
			Mapping.Modifiers.Add(Scalar);
		}
	}
}

UInputAction* USpearfishInputConfig::MakeAction(const TCHAR* Name, bool bAxis2D)
{
	UInputAction* Action = NewObject<UInputAction>(this, FName(Name));
	Action->ValueType = bAxis2D ? EInputActionValueType::Axis2D : EInputActionValueType::Boolean;
	return Action;
}

void USpearfishInputConfig::Build()
{
	using namespace SpearfishInputPrivate;

	GameplayContext = NewObject<UInputMappingContext>(this, TEXT("IMC_SpearfishGameplay"));
	UIContext = NewObject<UInputMappingContext>(this, TEXT("IMC_SpearfishUI"));

	Move = MakeAction(TEXT("IA_Move"), true);
	Look = MakeAction(TEXT("IA_Look"), true);
	Ascend = MakeAction(TEXT("IA_Ascend"), false);
	Descend = MakeAction(TEXT("IA_Descend"), false);
	Sprint = MakeAction(TEXT("IA_Sprint"), false);
	Fire = MakeAction(TEXT("IA_Fire"), false);
	Reel = MakeAction(TEXT("IA_Reel"), false);
	Release = MakeAction(TEXT("IA_ReleaseLine"), false);
	Interact = MakeAction(TEXT("IA_Interact"), false);
	ToggleLight = MakeAction(TEXT("IA_ToggleLight"), false);
	Tablet = MakeAction(TEXT("IA_Tablet"), false);
	Journal = MakeAction(TEXT("IA_Journal"), false);
	Help = MakeAction(TEXT("IA_Help"), false);
	PushToTalk = MakeAction(TEXT("IA_PushToTalk"), false);
	Pause = MakeAction(TEXT("IA_Pause"), false);

	Map(GameplayContext, Move, EKeys::W, true);
	Map(GameplayContext, Move, EKeys::S, true, true);
	Map(GameplayContext, Move, EKeys::D);
	Map(GameplayContext, Move, EKeys::A, false, true);
	Map(GameplayContext, Move, EKeys::Gamepad_Left2D);
	Map(GameplayContext, Look, EKeys::Mouse2D);
	Map(GameplayContext, Look, EKeys::Gamepad_Right2D, false, false, 2.5f);
	Map(GameplayContext, Ascend, EKeys::SpaceBar);
	Map(GameplayContext, Ascend, EKeys::Gamepad_FaceButton_Bottom);
	Map(GameplayContext, Descend, EKeys::LeftControl);
	Map(GameplayContext, Descend, EKeys::C);
	Map(GameplayContext, Descend, EKeys::Gamepad_FaceButton_Right);
	Map(GameplayContext, Sprint, EKeys::LeftShift);
	Map(GameplayContext, Sprint, EKeys::Gamepad_LeftThumbstick);
	Map(GameplayContext, Fire, EKeys::LeftMouseButton);
	Map(GameplayContext, Fire, EKeys::Gamepad_RightTrigger);
	Map(GameplayContext, Reel, EKeys::RightMouseButton);
	Map(GameplayContext, Reel, EKeys::Gamepad_LeftTrigger);
	Map(GameplayContext, Release, EKeys::R);
	Map(GameplayContext, Release, EKeys::Gamepad_FaceButton_Top);
	Map(GameplayContext, Interact, EKeys::E);
	Map(GameplayContext, Interact, EKeys::Gamepad_FaceButton_Left);
	Map(GameplayContext, ToggleLight, EKeys::L);
	Map(GameplayContext, ToggleLight, EKeys::Gamepad_DPad_Down);
	Map(GameplayContext, Tablet, EKeys::Tab);
	Map(GameplayContext, Tablet, EKeys::Gamepad_Special_Left);
	Map(GameplayContext, Journal, EKeys::J);
	Map(GameplayContext, Journal, EKeys::Gamepad_DPad_Up);
	Map(GameplayContext, Help, EKeys::F1);
	Map(GameplayContext, Help, EKeys::H);
	Map(GameplayContext, PushToTalk, EKeys::V);
	Map(GameplayContext, Pause, EKeys::Escape);
	Map(GameplayContext, Pause, EKeys::Gamepad_Special_Right);

	const FKey CommKeys[QuickCommCount] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six };
	for (int32 Index = 0; Index < QuickCommCount; ++Index)
	{
		UInputAction* Comm = MakeAction(*FString::Printf(TEXT("IA_QuickComm%d"), Index + 1), false);
		Map(GameplayContext, Comm, CommKeys[Index]);
		QuickComms.Add(Comm);
	}

	UIUp = MakeAction(TEXT("IA_UI_Up"), false);
	UIDown = MakeAction(TEXT("IA_UI_Down"), false);
	UILeft = MakeAction(TEXT("IA_UI_Left"), false);
	UIRight = MakeAction(TEXT("IA_UI_Right"), false);
	UIConfirm = MakeAction(TEXT("IA_UI_Confirm"), false);
	UIBack = MakeAction(TEXT("IA_UI_Back"), false);

	Map(UIContext, UIUp, EKeys::W);
	Map(UIContext, UIUp, EKeys::Up);
	Map(UIContext, UIUp, EKeys::Gamepad_DPad_Up);
	Map(UIContext, UIDown, EKeys::S);
	Map(UIContext, UIDown, EKeys::Down);
	Map(UIContext, UIDown, EKeys::Gamepad_DPad_Down);
	Map(UIContext, UILeft, EKeys::A);
	Map(UIContext, UILeft, EKeys::Left);
	Map(UIContext, UILeft, EKeys::Gamepad_DPad_Left);
	Map(UIContext, UIRight, EKeys::D);
	Map(UIContext, UIRight, EKeys::Right);
	Map(UIContext, UIRight, EKeys::Gamepad_DPad_Right);
	Map(UIContext, UIConfirm, EKeys::Enter);
	Map(UIContext, UIConfirm, EKeys::SpaceBar);
	Map(UIContext, UIConfirm, EKeys::E);
	Map(UIContext, UIConfirm, EKeys::LeftMouseButton);
	Map(UIContext, UIConfirm, EKeys::Gamepad_FaceButton_Bottom);
	Map(UIContext, UIBack, EKeys::Escape);
	Map(UIContext, UIBack, EKeys::Tab);
	Map(UIContext, UIBack, EKeys::BackSpace);
	Map(UIContext, UIBack, EKeys::Gamepad_FaceButton_Right);
}
