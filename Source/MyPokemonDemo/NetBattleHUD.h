#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NetBattleHUD.generated.h"

UCLASS()
class MYPOKEMONDEMO_API ANetBattleHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
