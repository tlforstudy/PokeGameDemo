#include "NetBattleMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	const FName NetBattleMapName(TEXT("/Game/NetBattle/NetBattleMap"));
	const FName SinglePlayerMapName(TEXT("/Game/scene/Demo_Map"));

	void PrepareForMapTravel(UNetBattleMenuWidget& Menu)
	{
		if (APlayerController* PlayerController = Menu.GetOwningPlayer())
		{
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
			PlayerController->bShowMouseCursor = false;
			PlayerController->FlushPressedKeys();
		}
		Menu.RemoveFromParent();
	}

	UTextBlock* MakeText(UWidgetTree& Tree, FName Name, const FText& Text, int32 Size,
		const FLinearColor& Color)
	{
		UTextBlock* TextBlock = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TextBlock->SetText(Text);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = Size;
		TextBlock->SetFont(Font);
		TextBlock->SetJustification(ETextJustify::Center);
		return TextBlock;
	}
}

void UNetBattleMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(FLinearColor(0.025f, 0.03f, 0.035f, 1.0f));
	Background->SetPadding(FMargin(48.0f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuPanel"));
	Panel->SetBrushColor(FLinearColor(0.08f, 0.095f, 0.105f, 0.98f));
	Panel->SetPadding(FMargin(36.0f, 30.0f));
	Background->SetContent(Panel);

	UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Menu"));
	Panel->SetContent(Menu);

	UTextBlock* Title = MakeText(*WidgetTree, TEXT("Title"), FText::FromString(TEXT("NETWORK BATTLE")), 32,
		FLinearColor(0.92f, 0.96f, 0.96f, 1.0f));
	Menu->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

	UTextBlock* Subtitle = MakeText(*WidgetTree, TEXT("Subtitle"),
		FText::FromString(TEXT("Authoritative two-player battle demo")), 15,
		FLinearColor(0.35f, 0.78f, 0.72f, 1.0f));
	Menu->AddChildToVerticalBox(Subtitle)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));

	UButton* HostButton = AddMenuButton(Menu, FText::FromString(TEXT("Host Battle")), TEXT("HostButton"));
	HostButton->OnClicked.AddDynamic(this, &UNetBattleMenuWidget::HostBattle);

	UTextBlock* AddressLabel = MakeText(*WidgetTree, TEXT("AddressLabel"),
		FText::FromString(TEXT("Server address")), 14, FLinearColor(0.72f, 0.75f, 0.76f, 1.0f));
	AddressLabel->SetJustification(ETextJustify::Left);
	Menu->AddChildToVerticalBox(AddressLabel)->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 6.0f));

	AddressInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("AddressInput"));
	AddressInput->SetText(FText::FromString(TEXT("127.0.0.1")));
	AddressInput->SetHintText(FText::FromString(TEXT("127.0.0.1 or LAN IPv4")));
	AddressInput->SetSelectAllTextWhenFocused(true);
	USizeBox* AddressBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AddressBox"));
	AddressBox->SetMinDesiredWidth(380.0f);
	AddressBox->SetHeightOverride(40.0f);
	AddressBox->SetContent(AddressInput);
	Menu->AddChildToVerticalBox(AddressBox)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	UButton* JoinButton = AddMenuButton(Menu, FText::FromString(TEXT("Join Battle")), TEXT("JoinButton"));
	JoinButton->OnClicked.AddDynamic(this, &UNetBattleMenuWidget::JoinBattle);

	UButton* SoloButton = AddMenuButton(Menu, FText::FromString(TEXT("Single Player Demo")), TEXT("SoloButton"));
	SoloButton->OnClicked.AddDynamic(this, &UNetBattleMenuWidget::OpenSinglePlayerDemo);

	UButton* ExitButton = AddMenuButton(Menu, FText::FromString(TEXT("Exit")), TEXT("ExitButton"));
	ExitButton->OnClicked.AddDynamic(this, &UNetBattleMenuWidget::ExitGame);

	StatusText = MakeText(*WidgetTree, TEXT("StatusText"),
		FText::FromString(TEXT("Host a match or enter the host's IPv4 address.")), 13,
		FLinearColor(0.66f, 0.69f, 0.70f, 1.0f));
	StatusText->SetAutoWrapText(true);
	Menu->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
}

UButton* UNetBattleMenuWidget::AddMenuButton(UVerticalBox* Parent, const FText& Label, FName WidgetName)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), WidgetName);
	Button->SetBackgroundColor(FLinearColor(0.16f, 0.44f, 0.42f, 1.0f));

	UTextBlock* LabelText = MakeText(*WidgetTree, FName(*(WidgetName.ToString() + TEXT("Label"))), Label, 17,
		FLinearColor::White);
	Button->SetContent(LabelText);
	USizeBox* ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
		FName(*(WidgetName.ToString() + TEXT("Box"))));
	ButtonBox->SetMinDesiredWidth(380.0f);
	ButtonBox->SetHeightOverride(48.0f);
	ButtonBox->SetContent(Button);
	Parent->AddChildToVerticalBox(ButtonBox)->SetPadding(FMargin(0.0f, 5.0f));
	return Button;
}

void UNetBattleMenuWidget::SetStatus(const FText& Message, const FLinearColor& Color)
{
	if (StatusText)
	{
		StatusText->SetText(Message);
		StatusText->SetColorAndOpacity(FSlateColor(Color));
	}
}

void UNetBattleMenuWidget::HostBattle()
{
	SetStatus(FText::FromString(TEXT("Starting listen server...")), FLinearColor(0.35f, 0.78f, 0.72f, 1.0f));
	PrepareForMapTravel(*this);
	UGameplayStatics::OpenLevel(this, NetBattleMapName, true, TEXT("listen"));
}

void UNetBattleMenuWidget::JoinBattle()
{
	FString Address = AddressInput ? AddressInput->GetText().ToString() : FString();
	Address.TrimStartAndEndInline();
	if (Address.IsEmpty())
	{
		SetStatus(FText::FromString(TEXT("Enter a server IPv4 address.")), FLinearColor(0.90f, 0.35f, 0.30f, 1.0f));
		return;
	}

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		SetStatus(FText::Format(FText::FromString(TEXT("Connecting to {0}...")), FText::FromString(Address)),
			FLinearColor(0.35f, 0.78f, 0.72f, 1.0f));
		PrepareForMapTravel(*this);
		PlayerController->ClientTravel(Address, TRAVEL_Absolute);
	}
}

void UNetBattleMenuWidget::OpenSinglePlayerDemo()
{
	PrepareForMapTravel(*this);
	UGameplayStatics::OpenLevel(this, SinglePlayerMapName, true);
}

void UNetBattleMenuWidget::ExitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
