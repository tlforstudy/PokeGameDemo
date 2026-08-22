#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NetBattleTypes.h"
#include "NetBattleGameMode.generated.h"

class ANetBattlePlayerController;
class ANetBattlePlayerState;

UCLASS()
class MYPOKEMONDEMO_API ANetBattleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANetBattleGameMode();

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Net Battle|Setup")
	TArray<FNetPokemonState> BuildRosterForPlayer(int32 PlayerSlot);
	virtual TArray<FNetPokemonState> BuildRosterForPlayer_Implementation(int32 PlayerSlot);

	void SubmitAction(ANetBattlePlayerController* Controller, ENetBattleActionType Type, int32 TargetIndex);

private:
	TMap<TObjectPtr<ANetBattlePlayerState>, FNetBattleAction> PendingActions;

	TArray<ANetBattlePlayerState*> GetBattlePlayers() const;
	void InitializeRosterForPlayer(ANetBattlePlayerState& Player, int32 PlayerSlot);
	void TryStartBattle();
	void ResolveTurn();
	void ResolveNextMove();
	void BeginForcedSwitchChoice();
	void CompleteForcedSwitch();
	float ResolveSwitch(ANetBattlePlayerState& Player, const FNetBattleAction& Action, FString& Log);
	float ResolveMove(ANetBattlePlayerState& Attacker, ANetBattlePlayerState& Defender, const FNetBattleAction& Action, FString& Log, bool& bBattleEnded);
	void BeginNextTurn(FString PreviousTurnLog);
	void EndBattle(ANetBattlePlayerState& Winner, const FString& FinalLog);

	TArray<TObjectPtr<ANetBattlePlayerState>> PendingMoveOrder;
	int32 PendingMoveIndex = 0;
	FString PendingResolutionLog;
	TObjectPtr<ANetBattlePlayerState> PendingForcedSwitchPlayer;
};
