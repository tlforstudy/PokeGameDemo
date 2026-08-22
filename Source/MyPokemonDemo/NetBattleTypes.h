#pragma once

#include "CoreMinimal.h"
#include "NetBattleTypes.generated.h"

UENUM(BlueprintType)
enum class ENetBattlePhase : uint8
{
	WaitingForPlayers,
	ChoosingActions,
	ResolvingTurn,
	ChoosingForcedSwitch,
	BattleEnded
};

UENUM(BlueprintType)
enum class ENetBattleActionType : uint8
{
	None,
	Move,
	Switch
};

UENUM(BlueprintType)
enum class ENetBattleCueType : uint8
{
	Attack,
	Switch,
	Faint,
	ForcedSwitch,
	BattleEnded
};

USTRUCT(BlueprintType)
struct MYPOKEMONDEMO_API FNetMoveState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	FName MoveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	int32 Power = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	int32 Accuracy = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	int32 MaxPP = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	int32 Priority = 0;

	// Names keep this network state independent from Blueprint-only enum classes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	FName MoveType = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	FName Category = NAME_None;
};

USTRUCT(BlueprintType)
struct MYPOKEMONDEMO_API FNetPokemonState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	FName PokemonId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	FName SpeciesId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	FName PrimaryType = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	int32 MaxHP = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	int32 CurrentHP = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	int32 Attack = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	int32 Defense = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	int32 Speed = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net Battle")
	TArray<FNetMoveState> Moves;

	bool IsFainted() const
	{
		return CurrentHP <= 0;
	}
};

USTRUCT(BlueprintType)
struct MYPOKEMONDEMO_API FNetBattleAction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Net Battle")
	ENetBattleActionType Type = ENetBattleActionType::None;

	UPROPERTY(BlueprintReadWrite, Category = "Net Battle")
	int32 MoveIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category = "Net Battle")
	int32 TargetIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category = "Net Battle")
	int32 ActingPokemonIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct MYPOKEMONDEMO_API FNetBattleCue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Net Battle|Presentation")
	int32 CueId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Net Battle|Presentation")
	int32 TurnNumber = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Net Battle|Presentation")
	ENetBattleCueType Type = ENetBattleCueType::Attack;

	UPROPERTY(BlueprintReadOnly, Category = "Net Battle|Presentation")
	int32 SourcePlayerId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Net Battle|Presentation")
	int32 TargetPlayerId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Net Battle|Presentation")
	int32 SourcePokemonIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Net Battle|Presentation")
	int32 TargetPokemonIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Net Battle|Presentation")
	int32 NewActivePokemonIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Net Battle|Presentation")
	int32 Damage = 0;

	// Authoritative HP before this cue. Presentation code can animate from this
	// value to TargetHPAfter without being overwritten by replicated roster data.
	UPROPERTY(BlueprintReadOnly, Category = "Net Battle|Presentation")
	int32 TargetHPBefore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Net Battle|Presentation")
	int32 TargetHPAfter = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Net Battle|Presentation")
	FString Message;

	UPROPERTY(BlueprintReadOnly, Category = "Net Battle|Presentation", meta = (ClampMin = "0.05"))
	float SuggestedDuration = 1.0f;
};
