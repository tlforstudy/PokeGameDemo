#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NetBattleMenuPlayerController.generated.h"

UCLASS()
class MYPOKEMONDEMO_API ANetBattleMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<class UNetBattleMenuWidget> MenuWidget;
};
