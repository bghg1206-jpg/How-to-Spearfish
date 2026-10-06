#include "UI/SpearfishMainMenu.h"

#include "Core/SpearfishGameInstance.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SpearfishMainMenu"

namespace SpearfishMainMenuPrivate
{
	const FLinearColor Background(0.01f, 0.05f, 0.09f, 1.f);
	const FLinearColor Panel(0.02f, 0.12f, 0.18f, 0.92f);
	const FLinearColor Title(0.95f, 0.85f, 0.55f);
	const FLinearColor Body(0.82f, 0.9f, 0.95f);
	const FLinearColor Note(0.55f, 0.65f, 0.72f);
}

void SSpearfishMainMenu::Construct(const FArguments& InArgs)
{
	using namespace SpearfishMainMenuPrivate;
	GameInstance = InArgs._GameInstance;
	Status = InArgs._Message;
	if (const USpearfishGameInstance* Instance = GameInstance.Get())
	{
		bHasSoloSave = Instance->HasSave(true);
		bHasCoopSave = Instance->HasSave(false);
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(Background)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(560.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(Panel)
				.Padding(FMargin(36.f, 28.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Title", "How to Spearfish"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 40))
						.ColorAndOpacity(Title)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 22.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Subtitle", "Dive for it. Cook it. Serve it. Swap roles at night."))
						.Font(FCoreStyle::GetDefaultFontStyle("Italic", 13))
						.ColorAndOpacity(Body)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(STextBlock).Text(LOCTEXT("SoloHeader", "SOLO - you dive, the auto-chef cooks")).ColorAndOpacity(Title)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						MakeButton(LOCTEXT("ContinueSolo", "Continue solo campaign"), LOCTEXT("ContinueSoloTip", "Pick up from your last morning."),
							FOnClicked::CreateSP(this, &SSpearfishMainMenu::OnContinueSolo), bHasSoloSave)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SButton)
						.ContentPadding(FMargin(14.f, 8.f))
						.OnClicked(FOnClicked::CreateSP(this, &SSpearfishMainMenu::OnNewSolo))
						[
							SNew(STextBlock).Text(this, &SSpearfishMainMenu::GetNewSoloLabel).Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 6.f)
					[
						SNew(STextBlock).Text(LOCTEXT("CoopHeader", "CO-OP - one dives, one cooks (2 players)")).ColorAndOpacity(Title)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						MakeButton(LOCTEXT("ContinueCoop", "Host: continue co-op campaign"), LOCTEXT("ContinueCoopTip", "Your partner joins with your IP address (port 7777)."),
							FOnClicked::CreateSP(this, &SSpearfishMainMenu::OnContinueCoop), bHasCoopSave)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SButton)
						.ContentPadding(FMargin(14.f, 8.f))
						.OnClicked(FOnClicked::CreateSP(this, &SSpearfishMainMenu::OnNewCoop))
						[
							SNew(STextBlock).Text(this, &SSpearfishMainMenu::GetNewCoopLabel).Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 8.f, 0.f)
						[
							SAssignNew(AddressBox, SEditableTextBox)
							.HintText(LOCTEXT("AddressHint", "Host address, e.g. 192.168.1.20"))
							.Text(FText::FromString(TEXT("127.0.0.1")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							MakeButton(LOCTEXT("Join", "Join"), LOCTEXT("JoinTip", "Join a friend's boat."), FOnClicked::CreateSP(this, &SSpearfishMainMenu::OnJoin))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)
					[
						MakeButton(LOCTEXT("Quit", "Quit"), FText::GetEmpty(), FOnClicked::CreateSP(this, &SSpearfishMainMenu::OnQuit))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(this, &SSpearfishMainMenu::GetStatusText)
						.AutoWrapText(true)
						.ColorAndOpacity(FLinearColor(1.f, 0.7f, 0.4f))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Footer", "Vertical slice - placeholder art. F1 in game for controls."))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						.ColorAndOpacity(Note)
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SSpearfishMainMenu::MakeButton(const FText& Label, const FText& Tooltip, FOnClicked OnClicked, const TAttribute<bool>& Enabled)
{
	return SNew(SBox)
		.Padding(FMargin(0.f, 3.f))
		[
			SNew(SButton)
			.ContentPadding(FMargin(14.f, 8.f))
			.ToolTipText(Tooltip)
			.IsEnabled(Enabled)
			.OnClicked(OnClicked)
			[
				SNew(STextBlock).Text(Label).Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
			]
		];
}

FText SSpearfishMainMenu::GetNewSoloLabel() const
{
	if (bConfirmNewSolo)
	{
		return LOCTEXT("ConfirmNewSolo", "Click again: this replaces your solo save");
	}
	return LOCTEXT("NewSolo", "New solo campaign");
}

FText SSpearfishMainMenu::GetNewCoopLabel() const
{
	if (bConfirmNewCoop)
	{
		return LOCTEXT("ConfirmNewCoop", "Click again: this replaces your co-op save");
	}
	return LOCTEXT("NewCoop", "Host: new co-op campaign");
}

FReply SSpearfishMainMenu::OnContinueSolo()
{
	if (USpearfishGameInstance* Instance = GameInstance.Get())
	{
		Status = LOCTEXT("Loading", "Rowing out...");
		Instance->StartSolo(false);
	}
	return FReply::Handled();
}

FReply SSpearfishMainMenu::OnNewSolo()
{
	if (bHasSoloSave && !bConfirmNewSolo)
	{
		bConfirmNewSolo = true;
		return FReply::Handled();
	}
	if (USpearfishGameInstance* Instance = GameInstance.Get())
	{
		Status = LOCTEXT("LoadingNew", "Rowing out...");
		Instance->StartSolo(true);
	}
	return FReply::Handled();
}

FReply SSpearfishMainMenu::OnContinueCoop()
{
	if (USpearfishGameInstance* Instance = GameInstance.Get())
	{
		Status = LOCTEXT("Hosting", "Hosting... your partner can join now.");
		Instance->HostCoop(false);
	}
	return FReply::Handled();
}

FReply SSpearfishMainMenu::OnNewCoop()
{
	if (bHasCoopSave && !bConfirmNewCoop)
	{
		bConfirmNewCoop = true;
		return FReply::Handled();
	}
	if (USpearfishGameInstance* Instance = GameInstance.Get())
	{
		Status = LOCTEXT("HostingNew", "Hosting... your partner can join now.");
		Instance->HostCoop(true);
	}
	return FReply::Handled();
}

FReply SSpearfishMainMenu::OnJoin()
{
	if (USpearfishGameInstance* Instance = GameInstance.Get())
	{
		const FString Address = AddressBox.IsValid() ? AddressBox->GetText().ToString() : FString();
		Status = FText::Format(LOCTEXT("Joining", "Joining {0}..."), FText::FromString(Address));
		Instance->JoinGame(Address);
	}
	return FReply::Handled();
}

FReply SSpearfishMainMenu::OnQuit()
{
	if (USpearfishGameInstance* Instance = GameInstance.Get())
	{
		UKismetSystemLibrary::QuitGame(Instance, nullptr, EQuitPreference::Quit, false);
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
