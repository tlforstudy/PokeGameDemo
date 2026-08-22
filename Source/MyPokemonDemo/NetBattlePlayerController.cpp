#include "NetBattlePlayerController.h"

#include "InputCoreTypes.h"
#include "Engine/World.h"
#include "NetBattleGameMode.h"
#include "NetBattleGameState.h"
#include "NetBattlePlayerState.h"
#include "NetBattleWaitingWidget.h"
#include "TimerManager.h"

ANetBattlePlayerController::ANetBattlePlayerController()
{
	bShowMouseCursor = true;
}

void ANetBattlePlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		WaitingWidget = CreateWidget<UNetBattleWaitingWidget>(this, UNetBattleWaitingWidget::StaticClass());
		if (WaitingWidget)
		{
			WaitingWidget->AddToViewport(1000);
		}
		GetWorldTimerManager().SetTimer(WaitingScreenTimer, this,
			&ANetBattlePlayerController::RefreshWaitingScreen, 0.2f, true, 0.0f);

		InitialBattleLogRefreshAttempts = 0;
		GetWorldTimerManager().SetTimer(InitialBattleLogRefreshTimer, this,
			&ANetBattlePlayerController::RefreshInitialBattleLog, 0.25f, true, 0.0f);
	}
}

void ANetBattlePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(InitialBattleLogRefreshTimer);
	GetWorldTimerManager().ClearTimer(WaitingScreenTimer);
	if (WaitingWidget)
	{
		WaitingWidget->RemoveFromParent();
		WaitingWidget = nullptr;
	}
	if (ANetBattleGameState* BattleState = GetNetBattleGameState())
	{
		BattleState->OnBattleCueReceived.RemoveDynamic(this, &ANetBattlePlayerController::HandleBattleCue);
	}
	Super::EndPlay(EndPlayReason);
}

void ANetBattlePlayerController::RefreshWaitingScreen()
{
	ANetBattleGameState* BattleState = GetNetBattleGameState();
	if (!BattleState)
	{
		return;
	}

	const int32 ConnectedPlayers = BattleState->PlayerArray.Num();
	if (BattleState->BattlePhase == ENetBattlePhase::ChoosingActions && ConnectedPlayers >= 2)
	{
		HideWaitingScreen();
		return;
	}

	if (WaitingWidget)
	{
		WaitingWidget->UpdateWaitingState(ConnectedPlayers, ConnectedPlayers >= 2);
	}
}

void ANetBattlePlayerController::HideWaitingScreen()
{
	GetWorldTimerManager().ClearTimer(WaitingScreenTimer);
	if (WaitingWidget)
	{
		WaitingWidget->RemoveFromParent();
		WaitingWidget = nullptr;
	}
}

void ANetBattlePlayerController::RefreshInitialBattleLog()
{
	if (ANetBattleGameState* BattleState = GetNetBattleGameState())
	{
		BattleState->OnBattleCueReceived.AddUniqueDynamic(this, &ANetBattlePlayerController::HandleBattleCue);
		if (BattleState->OnBattleLogChanged.IsBound())
		{
			BattleState->RefreshBattleLog();
			GetWorldTimerManager().ClearTimer(InitialBattleLogRefreshTimer);
			return;
		}
	}

	++InitialBattleLogRefreshAttempts;
	if (InitialBattleLogRefreshAttempts >= 40)
	{
		GetWorldTimerManager().ClearTimer(InitialBattleLogRefreshTimer);
	}
}

void ANetBattlePlayerController::HandleBattleCue(FNetBattleCue Cue)
{
	if (Cue.Type != ENetBattleCueType::BattleEnded)
	{
		return;
	}

	const ANetBattlePlayerState* LocalPlayerState = GetPlayerState<ANetBattlePlayerState>();
	OnBattleResult.Broadcast(LocalPlayerState && LocalPlayerState->GetPlayerId() == Cue.SourcePlayerId);
}

ANetBattleGameState* ANetBattlePlayerController::GetNetBattleGameState() const
{
	return GetWorld() ? GetWorld()->GetGameState<ANetBattleGameState>() : nullptr;
}

void ANetBattlePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(EKeys::A, IE_Pressed, this, &ANetBattlePlayerController::SubmitFirstMove);
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ANetBattlePlayerController::SwitchToFirstPokemon);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ANetBattlePlayerController::SwitchToSecondPokemon);
}

void ANetBattlePlayerController::SubmitMove(int32 MoveIndex)
{
	ServerSubmitMove(MoveIndex);
}

void ANetBattlePlayerController::SubmitFirstMove()
{
	SubmitMove(0);
}

void ANetBattlePlayerController::SubmitSwitch(int32 TargetIndex)
{
	ServerSubmitSwitch(TargetIndex);
}

void ANetBattlePlayerController::ServerSubmitMove_Implementation(int32 MoveIndex)
{
	if (ANetBattleGameMode* GameMode = GetWorld()->GetAuthGameMode<ANetBattleGameMode>())
	{
		GameMode->SubmitAction(this, ENetBattleActionType::Move, MoveIndex);
	}
}

void ANetBattlePlayerController::ServerSubmitSwitch_Implementation(int32 TargetIndex)
{
	if (ANetBattleGameMode* GameMode = GetWorld()->GetAuthGameMode<ANetBattleGameMode>())
	{
		GameMode->SubmitAction(this, ENetBattleActionType::Switch, TargetIndex);
	}
}

void ANetBattlePlayerController::SwitchToFirstPokemon()
{
	SubmitSwitch(0);
}

void ANetBattlePlayerController::SwitchToSecondPokemon()
{
	SubmitSwitch(1);
}
