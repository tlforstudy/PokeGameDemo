#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NetBattleTypes.h"
#include "NetBattleGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNetBattleStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNetBattleLogChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetBattleCueReceived, FNetBattleCue, Cue);

UCLASS(BlueprintType)
class MYPOKEMONDEMO_API ANetBattleGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ANetBattleGameState();

	UPROPERTY(ReplicatedUsing = OnRep_BattleState, BlueprintReadOnly, Category = "Net Battle")
	ENetBattlePhase BattlePhase = ENetBattlePhase::WaitingForPlayers;

	UPROPERTY(ReplicatedUsing = OnRep_BattleState, BlueprintReadOnly, Category = "Net Battle")
	int32 TurnNumber = 0;

	UPROPERTY(ReplicatedUsing = OnRep_BattleState, BlueprintReadOnly, Category = "Net Battle")
	FString BattleMessage = TEXT("Waiting for two players...");

	UPROPERTY(BlueprintAssignable, Category = "Net Battle|Events")
	FOnNetBattleStateChanged OnBattleStateChanged;

	UPROPERTY(ReplicatedUsing = OnRep_BattleLog, BlueprintReadOnly, Category = "Net Battle|Log")
	FString ReplicatedBattleLogText;

	UPROPERTY(BlueprintAssignable, Category = "Net Battle|Events")
	FOnNetBattleLogChanged OnBattleLogChanged;

	UPROPERTY(ReplicatedUsing = OnRep_BattleCues, BlueprintReadOnly, Category = "Net Battle|Presentation")
	TArray<FNetBattleCue> BattleCues;

	UPROPERTY(BlueprintAssignable, Category = "Net Battle|Events")
	FOnNetBattleCueReceived OnBattleCueReceived;

	void SetBattleState(ENetBattlePhase NewPhase, int32 NewTurnNumber, const FString& NewMessage);
	void SetBattleMessage(const FString& NewMessage);
	void ResetBattleLog();
	void AppendBattleLog(const FString& NewEntry);
	FNetBattleCue PublishBattleCue(FNetBattleCue Cue);

	UFUNCTION(BlueprintCallable, Category = "Net Battle|Log")
	void RefreshBattleLog();

	UFUNCTION(BlueprintPure, Category = "Net Battle")
	class ANetBattlePlayerState* FindBattlePlayerById(int32 PlayerId) const;

	UFUNCTION(BlueprintPure, Category = "Net Battle")
	class ANetBattlePlayerState* GetOpponentPlayer(const class ANetBattlePlayerState* LocalPlayer) const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_BattleState();

	UFUNCTION()
	void OnRep_BattleCues();

	UFUNCTION()
	void OnRep_BattleLog();

private:
	int32 NextCueId = 1;
	int32 LastProcessedCueId = 0;
	TArray<FNetBattleCue> PendingPresentationCues;
	TArray<FString> ServerBattleLogEntries;
	bool bPresentingCue = false;

	void NotifyBattleStateChanged();
	void NotifyBattleLogChanged();
	void QueuePresentationCue(const FNetBattleCue& Cue);
	void PlayNextPresentationCue();
};
