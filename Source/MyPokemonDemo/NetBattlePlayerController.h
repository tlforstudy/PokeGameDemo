#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NetBattleTypes.h"
#include "NetBattlePlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetBattleResult, bool, bLocalPlayerWon);

UCLASS(BlueprintType)
class MYPOKEMONDEMO_API ANetBattlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANetBattlePlayerController();

	UFUNCTION(BlueprintPure, Category = "Net Battle")
	class ANetBattleGameState* GetNetBattleGameState() const;

	UFUNCTION(BlueprintCallable, Category = "Net Battle")
	void SubmitMove(int32 MoveIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Net Battle")
	void SubmitSwitch(int32 TargetIndex);

	UPROPERTY(BlueprintAssignable, Category = "Net Battle|Events")
	FOnNetBattleResult OnBattleResult;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RefreshInitialBattleLog();
	void RefreshWaitingScreen();
	void HideWaitingScreen();

	UFUNCTION()
	void HandleBattleCue(FNetBattleCue Cue);
	FTimerHandle InitialBattleLogRefreshTimer;
	int32 InitialBattleLogRefreshAttempts = 0;

	UPROPERTY(Transient)
	TObjectPtr<class UNetBattleWaitingWidget> WaitingWidget;

	FTimerHandle WaitingScreenTimer;

	UFUNCTION(Server, Reliable)
	void ServerSubmitMove(int32 MoveIndex);

	UFUNCTION(Server, Reliable)
	void ServerSubmitSwitch(int32 TargetIndex);

protected:
	virtual void SetupInputComponent() override;

private:
	void SubmitFirstMove();
	void SwitchToFirstPokemon();
	void SwitchToSecondPokemon();
};
