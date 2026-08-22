#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NetBattleTypes.h"
#include "NetBattleVisualInterface.generated.h"

UINTERFACE(BlueprintType)
class MYPOKEMONDEMO_API UNetBattleVisualInterface : public UInterface
{
	GENERATED_BODY()
};

class MYPOKEMONDEMO_API INetBattleVisualInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Net Battle|Presentation")
	void InitializeNetBattleVisual(FNetPokemonState PokemonState, int32 OwnerPlayerId, int32 RosterIndex, bool bLocalSide);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Net Battle|Presentation")
	void RefreshNetBattleVisual(FNetPokemonState PokemonState);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Net Battle|Presentation")
	void PlayNetAttack(FNetBattleCue Cue);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Net Battle|Presentation")
	void PlayNetHit(FNetBattleCue Cue);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Net Battle|Presentation")
	void PlayNetFaint(FNetBattleCue Cue);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Net Battle|Presentation")
	void PlayNetSwitchIn(FNetBattleCue Cue);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Net Battle|Presentation")
	void PlayNetSwitchOut(FNetBattleCue Cue);
};
