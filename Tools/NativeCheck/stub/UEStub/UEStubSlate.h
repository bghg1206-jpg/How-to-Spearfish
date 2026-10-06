#pragma once

// Slate stand-ins: enough of the declarative syntax (SNew/SAssignNew, named arguments, slots, attributes
// bound to member functions, events) to type-check widget code the way the real macros would.

#include "UEStub/UEStubEngine.h"

class SWidget;

enum EHorizontalAlignment : int
{
	HAlign_Fill = 0,
	HAlign_Left = 1,
	HAlign_Center = 2,
	HAlign_Right = 3
};

enum EVerticalAlignment : int
{
	VAlign_Fill = 0,
	VAlign_Top = 1,
	VAlign_Center = 2,
	VAlign_Bottom = 3
};

namespace ETextJustify
{
	enum Type : int
	{
		Left,
		Center,
		Right
	};
}

struct EVisibility
{
	static const EVisibility Visible;
	static const EVisibility Collapsed;
	static const EVisibility Hidden;
	static const EVisibility HitTestInvisible;
	static const EVisibility SelfHitTestInvisible;
	bool operator==(const EVisibility& Other) const;
};

namespace ETextCommit
{
	enum Type : int
	{
		Default,
		OnEnter,
		OnUserMovedFocus,
		OnCleared
	};
}

enum class EFocusCause : uint8
{
	Mouse,
	Navigation,
	SetDirectly,
	Cleared,
	OtherWidgetLostFocus,
	WindowActivate
};

struct FMargin
{
	float Left = 0.f;
	float Top = 0.f;
	float Right = 0.f;
	float Bottom = 0.f;
	FMargin();
	FMargin(float UniformMargin);
	FMargin(float Horizontal, float Vertical);
	FMargin(float InLeft, float InTop, float InRight, float InBottom);
	FMargin(const FVector2D& InVector);
};

struct FSlateColor
{
	FSlateColor();
	FSlateColor(const FLinearColor& InColor);
	FSlateColor(const FColor& InColor);
	static FSlateColor UseForeground();
	static FSlateColor UseSubduedForeground();
	FLinearColor GetSpecifiedColor() const;
};

struct FSlateFontInfo
{
	float Size = 10.f;
	FName TypefaceFontName;
	FSlateFontInfo();
};

struct FSlateBrush
{
	FVector2D ImageSize;
	FSlateColor TintColor;
};

struct FOptionalSize
{
	FOptionalSize();
	FOptionalSize(const float SpecifiedSize);
	bool IsSet() const;
	float Get() const;
};

class ISlateStyle
{
public:
	virtual ~ISlateStyle();
	const FSlateBrush* GetBrush(const FName PropertyName, const ANSICHAR* Specifier = nullptr, const ISlateStyle* RequestingStyle = nullptr) const;
	FSlateFontInfo GetFontStyle(const FName PropertyName, const ANSICHAR* Specifier = nullptr) const;
	FSlateColor GetSlateColor(const FName PropertyName, const ANSICHAR* Specifier = nullptr, const ISlateStyle* RequestingStyle = nullptr) const;
	FLinearColor GetColor(const FName PropertyName, const ANSICHAR* Specifier = nullptr, const ISlateStyle* RequestingStyle = nullptr) const;
};

struct FFontOutlineSettings
{
	int32 OutlineSize = 0;
	FLinearColor OutlineColor;
	FFontOutlineSettings();
};

class FCoreStyle
{
public:
	static const ISlateStyle& Get();
	static FSlateFontInfo GetDefaultFontStyle(const FName InTypefaceFontName, const float InSize, const FFontOutlineSettings& InOutlineSettings = FFontOutlineSettings());
};

template <typename ObjectType>
class TAttribute
{
public:
	typedef TDelegate<ObjectType()> FGetter;

	TAttribute();
	TAttribute(std::nullptr_t);
	template <typename OtherType, typename = std::enable_if_t<std::is_constructible_v<ObjectType, const OtherType&>>>
	TAttribute(const OtherType& InInitialValue);
	template <class SourceType>
	TAttribute(SourceType* InObject, typename FGetter::template TConstMethodPtr<SourceType> InMethod);
	static TAttribute Create(const FGetter& InGetter);
	static TAttribute Create(TFunction<ObjectType(void)>&& InLambda) { return TAttribute(); }
	const ObjectType& Get() const;
	const ObjectType& Get(const ObjectType& DefaultValue) const;
	void Set(const ObjectType& InNewValue);
	bool IsSet() const;
	bool IsBound() const;
	void Bind(const FGetter& InGetter);
};

class FReply
{
public:
	static FReply Handled();
	static FReply Unhandled();
	FReply& SetUserFocus(TSharedRef<SWidget> GiveMeFocus, EFocusCause ReasonFocusIsChanging = EFocusCause::SetDirectly, bool bInAllUsers = false);
	bool IsEventHandled() const;
};

typedef TDelegate<FReply()> FOnClicked;
typedef TDelegate<void()> FSimpleSlateEvent;
typedef TDelegate<void(const FText&)> FOnTextChanged;
typedef TDelegate<void(const FText&, ETextCommit::Type)> FOnTextCommitted;

// --------------------------------------------------------------------------------- Named args

template <typename WidgetType>
struct TSlateBaseNamedArgs
{
	typedef typename WidgetType::FArguments WidgetArgsType;
	WidgetArgsType& Me() { return static_cast<WidgetArgsType&>(*this); }

	WidgetArgsType& ToolTipText(const TAttribute<FText>& InToolTipText) { return Me(); }
	WidgetArgsType& IsEnabled(const TAttribute<bool>& InIsEnabled) { return Me(); }
	WidgetArgsType& Visibility(const TAttribute<EVisibility>& InVisibility) { return Me(); }
	WidgetArgsType& RenderOpacity(float InRenderOpacity) { return Me(); }
	WidgetArgsType& Tag(FName InTag) { return Me(); }
	WidgetArgsType& ForceVolatile(bool bInForceVolatile) { return Me(); }
};

#define SLATE_BEGIN_ARGS(InWidgetType) \
	public: \
	struct FArguments : public TSlateBaseNamedArgs<InWidgetType> \
	{ \
		typedef FArguments WidgetArgsType; \
		typedef InWidgetType WidgetType; \
		FArguments()

#define SLATE_USER_ARGS(InWidgetType) SLATE_BEGIN_ARGS(InWidgetType)

#define SLATE_END_ARGS() \
	};

#define SLATE_ARGUMENT(ArgType, ArgName) \
	ArgType _##ArgName; \
	WidgetArgsType& ArgName(ArgType InArg) \
	{ \
		_##ArgName = InArg; \
		return static_cast<WidgetArgsType&>(*this); \
	}

#define SLATE_ATTRIBUTE(AttrType, AttrName) \
	TAttribute<AttrType> _##AttrName; \
	WidgetArgsType& AttrName(const TAttribute<AttrType>& InAttribute) \
	{ \
		_##AttrName = InAttribute; \
		return static_cast<WidgetArgsType&>(*this); \
	} \
	template <class UserClass, typename... VarTypes> \
	WidgetArgsType& AttrName(UserClass* InUserObject, typename TAttribute<AttrType>::FGetter::template TConstMethodPtr<UserClass, VarTypes...> InFunc, VarTypes... Vars) \
	{ \
		return static_cast<WidgetArgsType&>(*this); \
	} \
	template <typename FunctorType> \
	WidgetArgsType& AttrName##_Lambda(FunctorType&& InFunctor) \
	{ \
		return static_cast<WidgetArgsType&>(*this); \
	}

#define SLATE_EVENT(DelegateName, EventName) \
	DelegateName _##EventName; \
	WidgetArgsType& EventName(const DelegateName& InDelegate) \
	{ \
		_##EventName = InDelegate; \
		return static_cast<WidgetArgsType&>(*this); \
	} \
	template <class UserClass, typename... VarTypes> \
	WidgetArgsType& EventName(UserClass* InUserObject, typename DelegateName::template TMethodPtr<UserClass, VarTypes...> InFunc, VarTypes... Vars) \
	{ \
		return static_cast<WidgetArgsType&>(*this); \
	} \
	template <class UserClass, typename... VarTypes> \
	WidgetArgsType& EventName(UserClass* InUserObject, typename DelegateName::template TConstMethodPtr<UserClass, VarTypes...> InFunc, VarTypes... Vars) \
	{ \
		return static_cast<WidgetArgsType&>(*this); \
	}

#define SLATE_DEFAULT_SLOT(DeclarationType, SlotName) \
	WidgetArgsType& operator[](TSharedRef<SWidget> InChild) \
	{ \
		return static_cast<WidgetArgsType&>(*this); \
	}

// ------------------------------------------------------------------------------------ Widgets

class SWidget : public TSharedFromThis<SWidget>
{
public:
	virtual ~SWidget();
	virtual bool SupportsKeyboardFocus() const;
	virtual bool HasKeyboardFocus() const;
	void SetEnabled(TAttribute<bool> InEnabledState);
	bool IsEnabled() const;
	void SetVisibility(TAttribute<EVisibility> InVisibility);
	EVisibility GetVisibility() const;
	void SetToolTipText(const TAttribute<FText>& ToolTipText);
	virtual FReply OnKeyDown(const struct FGeometry& MyGeometry, const struct FKeyEvent& InKeyEvent);
};

class SCompoundWidget : public SWidget
{
public:
	struct FCompoundWidgetOneChildSlot
	{
		FCompoundWidgetOneChildSlot& operator[](TSharedRef<SWidget> InChild);
		FCompoundWidgetOneChildSlot& HAlign(EHorizontalAlignment InHAlignment);
		FCompoundWidgetOneChildSlot& VAlign(EVerticalAlignment InVAlignment);
		FCompoundWidgetOneChildSlot& Padding(TAttribute<FMargin> InPadding);
	};

protected:
	FCompoundWidgetOneChildSlot ChildSlot;
};

class SLeafWidget : public SWidget
{
};

class SPanel : public SWidget
{
};

template <class WidgetType>
struct TSlateDecl
{
	TSlateDecl& Expose(TSharedPtr<WidgetType>& OutVarToInit);
	TSlateDecl& Expose(TSharedRef<WidgetType>& OutVarToInit);
	TSharedRef<WidgetType> operator<<=(const typename WidgetType::FArguments& InArgs) const;
};

#define SNew(WidgetType, ...) TSlateDecl<WidgetType>() <<= WidgetType::FArguments()
#define SAssignNew(ExposeAs, WidgetType, ...) TSlateDecl<WidgetType>().Expose(ExposeAs) <<= WidgetType::FArguments()

class STextBlock : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(STextBlock) {}
		SLATE_ATTRIBUTE(FText, Text)
		SLATE_ATTRIBUTE(FSlateFontInfo, Font)
		SLATE_ATTRIBUTE(FSlateColor, ColorAndOpacity)
		SLATE_ATTRIBUTE(FVector2D, ShadowOffset)
		SLATE_ATTRIBUTE(FLinearColor, ShadowColorAndOpacity)
		SLATE_ATTRIBUTE(float, WrapTextAt)
		SLATE_ATTRIBUTE(bool, AutoWrapText)
		SLATE_ATTRIBUTE(FMargin, Margin)
		SLATE_ATTRIBUTE(float, LineHeightPercentage)
		SLATE_ATTRIBUTE(ETextJustify::Type, Justification)
		SLATE_ATTRIBUTE(float, MinDesiredWidth)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetText(const TAttribute<FText>& InText);
	const FText& GetText() const;
	void SetColorAndOpacity(const TAttribute<FSlateColor>& InColorAndOpacity);
	void SetFont(const TAttribute<FSlateFontInfo>& InFont);
};

class SBorder : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBorder) {}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ARGUMENT(EHorizontalAlignment, HAlign)
		SLATE_ARGUMENT(EVerticalAlignment, VAlign)
		SLATE_ATTRIBUTE(FMargin, Padding)
		SLATE_ATTRIBUTE(const FSlateBrush*, BorderImage)
		SLATE_ATTRIBUTE(FSlateColor, BorderBackgroundColor)
		SLATE_ATTRIBUTE(FLinearColor, ColorAndOpacity)
		SLATE_ATTRIBUTE(FVector2D, DesiredSizeScale)
		SLATE_ATTRIBUTE(FSlateColor, ForegroundColor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetContent(TSharedRef<SWidget> InContent);
	void SetBorderBackgroundColor(TAttribute<FSlateColor> InColorAndOpacity);
	void SetPadding(TAttribute<FMargin> InPadding);
};

class SBox : public SPanel
{
public:
	SLATE_BEGIN_ARGS(SBox) {}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ARGUMENT(EHorizontalAlignment, HAlign)
		SLATE_ARGUMENT(EVerticalAlignment, VAlign)
		SLATE_ATTRIBUTE(FMargin, Padding)
		SLATE_ATTRIBUTE(FOptionalSize, WidthOverride)
		SLATE_ATTRIBUTE(FOptionalSize, HeightOverride)
		SLATE_ATTRIBUTE(FOptionalSize, MinDesiredWidth)
		SLATE_ATTRIBUTE(FOptionalSize, MinDesiredHeight)
		SLATE_ATTRIBUTE(FOptionalSize, MaxDesiredWidth)
		SLATE_ATTRIBUTE(FOptionalSize, MaxDesiredHeight)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetContent(const TSharedRef<SWidget>& InContent);
	void SetWidthOverride(TAttribute<FOptionalSize> InWidthOverride);
	void SetHeightOverride(TAttribute<FOptionalSize> InHeightOverride);
};

class SBoxPanel : public SPanel
{
};

template <class SlotArgsType>
struct TBoxSlotArgumentsBase
{
	SlotArgsType& Me() { return static_cast<SlotArgsType&>(*this); }
	SlotArgsType& HAlign(EHorizontalAlignment InHAlignment) { return Me(); }
	SlotArgsType& VAlign(EVerticalAlignment InVAlignment) { return Me(); }
	SlotArgsType& Padding(TAttribute<FMargin> InPadding) { return Me(); }
	SlotArgsType& Padding(float Uniform) { return Me(); }
	SlotArgsType& Padding(float Horizontal, float Vertical) { return Me(); }
	SlotArgsType& Padding(float Left, float Top, float Right, float Bottom) { return Me(); }
	SlotArgsType& operator[](TSharedRef<SWidget> InChildWidget) { return Me(); }
	SlotArgsType& Expose(struct FSlotBase*& OutVarToInit) { return Me(); }
};

class SVerticalBox : public SBoxPanel
{
public:
	class FSlot
	{
	public:
		struct FSlotArguments : public TBoxSlotArgumentsBase<FSlotArguments>
		{
			FSlotArguments& AutoHeight() { return *this; }
			FSlotArguments& FillHeight(TAttribute<float> StretchCoefficient) { return *this; }
			FSlotArguments& MaxHeight(TAttribute<float> InMaxHeight) { return *this; }
		};
	};
	static FSlot::FSlotArguments Slot();

	SLATE_BEGIN_ARGS(SVerticalBox) {}
		WidgetArgsType& operator+(FSlot::FSlotArguments& SlotToAdd) { return *this; }
		WidgetArgsType& operator+(FSlot::FSlotArguments&& SlotToAdd) { return *this; }
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	FSlot::FSlotArguments AddSlot();
	int32 RemoveSlot(const TSharedRef<SWidget>& SlotWidget);
	void ClearChildren();
};

class SHorizontalBox : public SBoxPanel
{
public:
	class FSlot
	{
	public:
		struct FSlotArguments : public TBoxSlotArgumentsBase<FSlotArguments>
		{
			FSlotArguments& AutoWidth() { return *this; }
			FSlotArguments& FillWidth(TAttribute<float> StretchCoefficient) { return *this; }
			FSlotArguments& MaxWidth(TAttribute<float> InMaxWidth) { return *this; }
		};
	};
	static FSlot::FSlotArguments Slot();

	SLATE_BEGIN_ARGS(SHorizontalBox) {}
		WidgetArgsType& operator+(FSlot::FSlotArguments& SlotToAdd) { return *this; }
		WidgetArgsType& operator+(FSlot::FSlotArguments&& SlotToAdd) { return *this; }
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	FSlot::FSlotArguments AddSlot();
	void ClearChildren();
};

class SButton : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SButton) {}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ATTRIBUTE(FText, Text)
		SLATE_ARGUMENT(EHorizontalAlignment, HAlign)
		SLATE_ARGUMENT(EVerticalAlignment, VAlign)
		SLATE_ATTRIBUTE(FMargin, ContentPadding)
		SLATE_ATTRIBUTE(FSlateColor, ButtonColorAndOpacity)
		SLATE_ATTRIBUTE(FSlateColor, ForegroundColor)
		SLATE_ATTRIBUTE(FVector2D, DesiredSizeScale)
		SLATE_ARGUMENT(bool, IsFocusable)
		SLATE_EVENT(FOnClicked, OnClicked)
		SLATE_EVENT(FSimpleSlateEvent, OnPressed)
		SLATE_EVENT(FSimpleSlateEvent, OnReleased)
		SLATE_EVENT(FSimpleSlateEvent, OnHovered)
		SLATE_EVENT(FSimpleSlateEvent, OnUnhovered)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	bool IsPressed() const;
};

class SEditableTextBox : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SEditableTextBox) {}
		SLATE_ATTRIBUTE(FText, Text)
		SLATE_ATTRIBUTE(FText, HintText)
		SLATE_ATTRIBUTE(FSlateFontInfo, Font)
		SLATE_ATTRIBUTE(FSlateColor, ForegroundColor)
		SLATE_ATTRIBUTE(bool, IsReadOnly)
		SLATE_ATTRIBUTE(bool, IsPassword)
		SLATE_ATTRIBUTE(bool, SelectAllTextWhenFocused)
		SLATE_ATTRIBUTE(FMargin, Padding)
		SLATE_ATTRIBUTE(ETextJustify::Type, Justification)
		SLATE_EVENT(FOnTextChanged, OnTextChanged)
		SLATE_EVENT(FOnTextCommitted, OnTextCommitted)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	FText GetText() const;
	void SetText(const TAttribute<FText>& InNewText);
	void SetHintText(const TAttribute<FText>& InHintText);
};

class SSpacer : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SSpacer) {}
		SLATE_ATTRIBUTE(FVector2D, Size)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};

class SWeakWidget : public SWidget
{
public:
	SLATE_BEGIN_ARGS(SWeakWidget) {}
		SLATE_ARGUMENT(TSharedPtr<SWidget>, PossiblyNullContent)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetContent(const TSharedRef<SWidget>& InWidget);
};

class FSlateApplication
{
public:
	static FSlateApplication& Get();
	static bool IsInitialized();
	bool SetKeyboardFocus(const TSharedPtr<SWidget>& OptionalWidgetToFocus, EFocusCause ReasonFocusIsChanging = EFocusCause::SetDirectly);
	void SetAllUserFocusToGameViewport(EFocusCause ReasonFocusIsChanging = EFocusCause::SetDirectly);
	void ClearKeyboardFocus(const EFocusCause ReasonFocusIsChanging = EFocusCause::SetDirectly);
};
