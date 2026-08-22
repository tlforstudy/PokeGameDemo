#include "NetBattleGameState.h"

#include "NetBattlePlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ANetBattleGameState::ANetBattleGameState()
{
	bReplicates = true;
}

void ANetBattleGameState::SetBattleState(ENetBattlePhase NewPhase, int32 NewTurnNumber, const FString& NewMessage)
{
	check(HasAuthority());
	BattlePhase = NewPhase;
	TurnNumber = NewTurnNumber;
	BattleMessage = NewMessage;
	NotifyBattleStateChanged();
	ForceNetUpdate();
}

void ANetBattleGameState::SetBattleMessage(const FString& NewMessage)
{
	check(HasAuthority());
	BattleMessage = NewMessage;
	NotifyBattleStateChanged();
	ForceNetUpdate();
}

void ANetBattleGameState::ResetBattleLog()
{
	check(HasAuthority());
	ServerBattleLogEntries.Reset();
	ReplicatedBattleLogText.Reset();
	NotifyBattleLogChanged();
	ForceNetUpdate();
}

void ANetBattleGameState::AppendBattleLog(const FString& NewEntry)
{
	check(HasAuthority());
	if (NewEntry.IsEmpty())
	{
		return;
	}

	ServerBattleLogEntries.Add(NewEntry);
	while (ServerBattleLogEntries.Num() > 24)
	{
		ServerBattleLogEntries.RemoveAt(0);
	}
	ReplicatedBattleLogText = FString::Join(ServerBattleLogEntries, TEXT("\n"));
	NotifyBattleLogChanged();
	ForceNetUpdate();
}

void ANetBattleGameState::RefreshBattleLog()
{
	NotifyBattleLogChanged();
}

FNetBattleCue ANetBattleGameState::PublishBattleCue(FNetBattleCue Cue)
{
	check(HasAuthority());
	Cue.CueId = NextCueId++;
	Cue.TurnNumber = TurnNumber;
	BattleCues.Add(Cue);
	while (BattleCues.Num() > 32)
	{
		BattleCues.RemoveAt(0);
	}

	LastProcessedCueId = Cue.CueId;
	QueuePresentationCue(Cue);
	ForceNetUpdate();
	return Cue;
}

ANetBattlePlayerState* ANetBattleGameState::FindBattlePlayerById(int32 PlayerId) const
{
	for (APlayerState* PlayerState : PlayerArray)
	{
		if (ANetBattlePlayerState* BattlePlayer = Cast<ANetBattlePlayerState>(PlayerState))
		{
			if (BattlePlayer->GetPlayerId() == PlayerId)
			{
				return BattlePlayer;
			}
		}
	}
	return nullptr;
}

ANetBattlePlayerState* ANetBattleGameState::GetOpponentPlayer(const ANetBattlePlayerState* LocalPlayer) const
{
	for (APlayerState* PlayerState : PlayerArray)
	{
		ANetBattlePlayerState* BattlePlayer = Cast<ANetBattlePlayerState>(PlayerState);
		if (BattlePlayer && BattlePlayer != LocalPlayer)
		{
			return BattlePlayer;
		}
	}
	return nullptr;
}

void ANetBattleGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANetBattleGameState, BattlePhase);
	DOREPLIFETIME(ANetBattleGameState, TurnNumber);
	DOREPLIFETIME(ANetBattleGameState, BattleMessage);
	DOREPLIFETIME(ANetBattleGameState, ReplicatedBattleLogText);
	DOREPLIFETIME(ANetBattleGameState, BattleCues);
}

void ANetBattleGameState::OnRep_BattleState()
{
	NotifyBattleStateChanged();
}

void ANetBattleGameState::OnRep_BattleCues()
{
	for (const FNetBattleCue& Cue : BattleCues)
	{
		if (Cue.CueId > LastProcessedCueId)
		{
			LastProcessedCueId = Cue.CueId;
			QueuePresentationCue(Cue);
		}
	}
}

void ANetBattleGameState::OnRep_BattleLog()
{
	NotifyBattleLogChanged();
}

void ANetBattleGameState::NotifyBattleStateChanged()
{
	OnBattleStateChanged.Broadcast();
}

void ANetBattleGameState::NotifyBattleLogChanged()
{
	OnBattleLogChanged.Broadcast();
}

void ANetBattleGameState::QueuePresentationCue(const FNetBattleCue& Cue)
{
	PendingPresentationCues.Add(Cue);
	if (!bPresentingCue)
	{
		PlayNextPresentationCue();
	}
}

void ANetBattleGameState::PlayNextPresentationCue()
{
	if (PendingPresentationCues.IsEmpty())
	{
		bPresentingCue = false;
		return;
	}

	bPresentingCue = true;
	const FNetBattleCue Cue = PendingPresentationCues[0];
	PendingPresentationCues.RemoveAt(0);
	OnBattleCueReceived.Broadcast(Cue);

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ANetBattleGameState::PlayNextPresentationCue,
		FMath::Max(0.05f, Cue.SuggestedDuration), false);
}
