#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NetBattleWaitingWidget.generated.h"

class UTextBlock;

UCLASS()
class MYPOKEMONDEMO_API UNetBattleWaitingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void UpdateWaitingState(int32 ConnectedPlayers, bool bPreparingBattle);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlayerCountText;

	UFUNCTION()
	void ReturnToMenu();
};
