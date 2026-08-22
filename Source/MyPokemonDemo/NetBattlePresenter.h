#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetBattleTypes.h"
#include "NetBattlePresenter.generated.h"

class ANetBattleGameState;
class ANetBattlePlayerState;
class UCameraComponent;
class USceneComponent;

USTRUCT(BlueprintType)
struct MYPOKEMONDEMO_API FNetPokemonVisualDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Net Battle|Presentation")
	FName DisplayName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Net Battle|Presentation")
	TSubclassOf<AActor> VisualClass;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPresenterCue, FNetBattleCue, Cue);

UCLASS(Blueprintable)
class MYPOKEMONDEMO_API ANetBattlePresenter : public AActor
{
	GENERATED_BODY()

public:
	ANetBattlePresenter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Net Battle|Presentation")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Net Battle|Presentation")
	TObjectPtr<USceneComponent> LocalSpawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Net Battle|Presentation")
	TObjectPtr<USceneComponent> OpponentSpawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Net Battle|Presentation")
	TObjectPtr<UCameraComponent> BattleCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Net Battle|Presentation")
	TArray<FNetPokemonVisualDefinition> VisualDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Net Battle|Presentation")
	bool bUseBattleCamera = true;

	UPROPERTY(BlueprintAssignable, Category = "Net Battle|Events")
	FOnPresenterCue OnPresenterCue;

	UFUNCTION(BlueprintCallable, Category = "Net Battle|Presentation")
	void RefreshPresentation();

	UFUNCTION(BlueprintPure, Category = "Net Battle|Presentation")
	AActor* FindPokemonVisual(int32 PlayerId, int32 RosterIndex) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TMap<int64, TObjectPtr<AActor>> SpawnedVisuals;

	// Avoid reapplying a stale replicated active index while a switch cue is
	// already presenting the new visual locally.
	TMap<int32, int32> LastReplicatedActiveIndices;

	UPROPERTY(Transient)
	TObjectPtr<ANetBattleGameState> BoundGameState;

	FTimerHandle RefreshTimer;

	UFUNCTION()
	void HandleBattleCue(FNetBattleCue Cue);

	TSubclassOf<AActor> FindVisualClass(const FNetPokemonState& Pokemon) const;
	AActor* SpawnVisual(ANetBattlePlayerState& Player, int32 RosterIndex, bool bLocalSide);
	void SetPlayerActiveVisual(ANetBattlePlayerState& Player, int32 NewActiveIndex);
	static int64 MakeVisualKey(int32 PlayerId, int32 RosterIndex);
	void ExecuteVisualEvent(AActor* Visual, ENetBattleCueType CueType, const FNetBattleCue& Cue, bool bTargetEvent) const;
};
