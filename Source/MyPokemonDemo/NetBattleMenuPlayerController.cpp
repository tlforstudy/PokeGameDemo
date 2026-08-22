#include "NetBattleMenuPlayerController.h"

#include "NetBattleMenuWidget.h"

void ANetBattleMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	bShowMouseCursor = true;
	MenuWidget = CreateWidget<UNetBattleMenuWidget>(this, UNetBattleMenuWidget::StaticClass());
	if (MenuWidget)
	{
		MenuWidget->AddToViewport(100);
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
}
