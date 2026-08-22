#include "NetBattleWaitingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	UTextBlock* CreateWaitingText(UWidgetTree& Tree, FName Name, const FText& Text, int32 Size,
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

void UNetBattleWaitingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.03f, 1.0f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WaitingPanel"));
	Panel->SetBrushColor(FLinearColor(0.075f, 0.09f, 0.10f, 1.0f));
	Panel->SetPadding(FMargin(44.0f, 36.0f));
	Background->SetContent(Panel);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
	Panel->SetContent(Content);

	UTextBlock* Title = CreateWaitingText(*WidgetTree, TEXT("Title"),
		FText::FromString(TEXT("WAITING FOR OPPONENT")), 29, FLinearColor(0.92f, 0.96f, 0.96f, 1.0f));
	Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

	StatusText = CreateWaitingText(*WidgetTree, TEXT("StatusText"),
		FText::FromString(TEXT("Listen server is ready. Waiting for another player...")), 16,
		FLinearColor(0.35f, 0.78f, 0.72f, 1.0f));
	StatusText->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	PlayerCountText = CreateWaitingText(*WidgetTree, TEXT("PlayerCountText"),
		FText::FromString(TEXT("1 / 2 players connected")), 14,
		FLinearColor(0.68f, 0.71f, 0.72f, 1.0f));
	Content->AddChildToVerticalBox(PlayerCountText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));

	UButton* ReturnButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ReturnButton"));
	ReturnButton->SetBackgroundColor(FLinearColor(0.52f, 0.20f, 0.18f, 1.0f));
	ReturnButton->SetContent(CreateWaitingText(*WidgetTree, TEXT("ReturnButtonLabel"),
		FText::FromString(TEXT("Return to Menu")), 16, FLinearColor::White));
	ReturnButton->OnClicked.AddDynamic(this, &UNetBattleWaitingWidget::ReturnToMenu);

	USizeBox* ReturnButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ReturnButtonBox"));
	ReturnButtonBox->SetMinDesiredWidth(340.0f);
	ReturnButtonBox->SetHeightOverride(46.0f);
	ReturnButtonBox->SetContent(ReturnButton);
	Content->AddChildToVerticalBox(ReturnButtonBox);
}

void UNetBattleWaitingWidget::UpdateWaitingState(int32 ConnectedPlayers, bool bPreparingBattle)
{
	const int32 DisplayedPlayers = FMath::Clamp(ConnectedPlayers, 1, 2);
	if (PlayerCountText)
	{
		PlayerCountText->SetText(FText::Format(FText::FromString(TEXT("{0} / 2 players connected")),
			FText::AsNumber(DisplayedPlayers)));
	}
	if (StatusText)
	{
		StatusText->SetText(bPreparingBattle
			? FText::FromString(TEXT("Opponent connected. Preparing battle..."))
			: FText::FromString(TEXT("Listen server is ready. Waiting for another player...")));
	}
}

void UNetBattleWaitingWidget::ReturnToMenu()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/NetBattle/BattleMenuMap")), true);
}
