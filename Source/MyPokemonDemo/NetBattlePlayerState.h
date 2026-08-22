#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NetBattleTypes.h"
#include "NetBattlePlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNetRosterChanged);

UCLASS(BlueprintType)
class MYPOKEMONDEMO_API ANetBattlePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ANetBattlePlayerState();

	UPROPERTY(ReplicatedUsing = OnRep_Roster, BlueprintReadOnly, Category = "Net Battle")
	TArray<FNetPokemonState> Roster;

	UPROPERTY(ReplicatedUsing = OnRep_Roster, BlueprintReadOnly, Category = "Net Battle")
	int32 ActivePokemonIndex = 0;

	UPROPERTY(ReplicatedUsing = OnRep_ActionSubmitted, BlueprintReadOnly, Category = "Net Battle")
	bool bActionSubmitted = false;

	UPROPERTY(BlueprintAssignable, Category = "Net Battle|Events")
	FOnNetRosterChanged OnRosterChanged;

	UFUNCTION(BlueprintPure, Category = "Net Battle")
	FNetPokemonState GetActivePokemon() const;

	UFUNCTION(BlueprintPure, Category = "Net Battle")
	bool HasUsablePokemon() const;

	UFUNCTION(BlueprintPure, Category = "Net Battle")
	bool CanSwitchTo(int32 TargetIndex) const;

	void InitializeDefaultRoster(int32 PlayerSlot);
	void SetRoster(const TArray<FNetPokemonState>& NewRoster);
	void SetActionSubmitted(bool bSubmitted);
	bool SwitchTo(int32 TargetIndex);
	int32 ApplyDamageToActive(int32 Damage);
	bool SwitchToFirstUsablePokemon();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_Roster();

	UFUNCTION()
	void OnRep_ActionSubmitted();

private:
	void NotifyRosterChanged();
};
