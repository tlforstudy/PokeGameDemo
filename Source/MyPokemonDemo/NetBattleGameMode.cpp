#include "NetBattleGameMode.h"

#include "NetBattleGameState.h"
#include "NetBattlePlayerController.h"
#include "NetBattlePlayerState.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FName ResolveMoveTypeDisplayName(const FName Type)
	{
		if (Type.IsNone())
		{
			return NAME_None;
		}

		static TWeakObjectPtr<UEnum> MoveTypeEnum;
		if (!MoveTypeEnum.IsValid())
		{
			MoveTypeEnum = LoadObject<UEnum>(nullptr, TEXT("/Game/Enums/E_Movetype.E_Movetype"));
		}

		if (const UEnum* Enum = MoveTypeEnum.Get())
		{
			const int64 EnumValue = Enum->GetValueByName(Type);
			if (EnumValue != INDEX_NONE)
			{
				const FString DisplayName = Enum->GetDisplayNameTextByValue(EnumValue).ToString();
				if (!DisplayName.IsEmpty())
				{
					return FName(*DisplayName);
				}
			}
		}

		return Type;
	}

	FName NormalizeBattleType(const FName Type)
	{
		const FName ResolvedType = ResolveMoveTypeDisplayName(Type);
		FString Value = ResolvedType.ToString().ToLower();
		Value.ReplaceInline(TEXT(" "), TEXT(""));
		Value.ReplaceInline(TEXT("_"), TEXT(""));
		Value.ReplaceInline(TEXT("-"), TEXT(""));
		if (Value.Contains(TEXT("fire")))
		{
			return TEXT("Fire");
		}
		if (Value.Contains(TEXT("water")))
		{
			return TEXT("Water");
		}
		if (Value.Contains(TEXT("grass")))
		{
			return TEXT("Grass");
		}
		if (Value.Contains(TEXT("normal")))
		{
			return TEXT("Normal");
		}
		return ResolvedType;
	}

	FString GetPokemonLogId(const FNetPokemonState& Pokemon)
	{
		if (!Pokemon.SpeciesId.IsNone())
		{
			return Pokemon.SpeciesId.ToString();
		}

		const FString DisplayName = Pokemon.DisplayName.ToString();
		return DisplayName.IsEmpty() ? TEXT("Pokemon") : DisplayName;
	}

	FString GetMoveLogId(const FNetMoveState& Move)
	{
		if (!Move.MoveId.IsNone())
		{
			return Move.MoveId.ToString();
		}

		const FString DisplayName = Move.DisplayName.ToString();
		return DisplayName.IsEmpty() ? TEXT("Move") : DisplayName;
	}

	float GetTypeMultiplier(const FName MoveType, const FName TargetType)
	{
		const FName NormalizedMoveType = NormalizeBattleType(MoveType);
		const FName NormalizedTargetType = NormalizeBattleType(TargetType);
		if (NormalizedMoveType == NAME_None || NormalizedTargetType == NAME_None)
		{
			return 1.0f;
		}

		if ((NormalizedMoveType == TEXT("Fire") && NormalizedTargetType == TEXT("Grass"))
			|| (NormalizedMoveType == TEXT("Water") && NormalizedTargetType == TEXT("Fire"))
			|| (NormalizedMoveType == TEXT("Grass") && NormalizedTargetType == TEXT("Water")))
		{
			return 2.0f;
		}

		if ((NormalizedMoveType == TEXT("Fire") && NormalizedTargetType == TEXT("Water"))
			|| (NormalizedMoveType == TEXT("Water") && NormalizedTargetType == TEXT("Grass"))
			|| (NormalizedMoveType == TEXT("Grass") && NormalizedTargetType == TEXT("Fire")))
		{
			return 0.5f;
		}

		return 1.0f;
	}

	int32 CalculateAuthoritativeDamage(const FNetPokemonState& Attacker, const FNetPokemonState& Defender,
		const FNetMoveState& Move)
	{
		const int32 Power = FMath::Max(1, Move.Power);
		const int32 Attack = FMath::Max(1, Attacker.Attack);
		const int32 Defense = FMath::Max(1, Defender.Defense);

		const float BaseDamage = Attack * 0.1f * Power - Defense * 2.0f;
		const float TypeMultiplier = GetTypeMultiplier(Move.MoveType, Defender.PrimaryType);
		return FMath::Max(1, FMath::RoundToInt(BaseDamage * TypeMultiplier));
	}
}

ANetBattleGameMode::ANetBattleGameMode()
{
	GameStateClass = ANetBattleGameState::StaticClass();
	PlayerStateClass = ANetBattlePlayerState::StaticClass();
	PlayerControllerClass = ANetBattlePlayerController::StaticClass();
	HUDClass = nullptr;
	DefaultPawnClass = nullptr;
	bStartPlayersAsSpectators = true;
}

void ANetBattleGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (ErrorMessage.IsEmpty() && GetNumPlayers() >= 2)
	{
		ErrorMessage = TEXT("This battle already has two players.");
	}
}

void ANetBattleGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	const TArray<ANetBattlePlayerState*> Players = GetBattlePlayers();
	if (ANetBattlePlayerState* NewPlayerState = NewPlayer->GetPlayerState<ANetBattlePlayerState>())
	{
		InitializeRosterForPlayer(*NewPlayerState, FMath::Max(0, Players.IndexOfByKey(NewPlayerState)));
	}
	TryStartBattle();
}

TArray<FNetPokemonState> ANetBattleGameMode::BuildRosterForPlayer_Implementation(int32 PlayerSlot)
{
	return {};
}

void ANetBattleGameMode::InitializeRosterForPlayer(ANetBattlePlayerState& Player, int32 PlayerSlot)
{
	TArray<FNetPokemonState> ConfiguredRoster = BuildRosterForPlayer(PlayerSlot);
	if (ConfiguredRoster.IsEmpty())
	{
		Player.InitializeDefaultRoster(PlayerSlot);
		return;
	}

	for (int32 PokemonIndex = 0; PokemonIndex < ConfiguredRoster.Num(); ++PokemonIndex)
	{
		FNetPokemonState& Pokemon = ConfiguredRoster[PokemonIndex];
		const FString DisplayName = Pokemon.DisplayName.ToString();
		if (Pokemon.SpeciesId.IsNone() && !DisplayName.IsEmpty())
		{
			Pokemon.SpeciesId = FName(*DisplayName);
		}
		if (Pokemon.PokemonId.IsNone())
		{
			Pokemon.PokemonId = FName(*FString::Printf(TEXT("%s_P%d_%d"),
				*GetPokemonLogId(Pokemon), PlayerSlot + 1, PokemonIndex + 1));
		}
		Pokemon.PrimaryType = NormalizeBattleType(Pokemon.PrimaryType);
		for (FNetMoveState& Move : Pokemon.Moves)
		{
			const FString MoveDisplayName = Move.DisplayName.ToString();
			if (Move.MoveId.IsNone() && !MoveDisplayName.IsEmpty())
			{
				Move.MoveId = FName(*MoveDisplayName);
			}
			Move.MoveType = NormalizeBattleType(Move.MoveType);
		}
	}

	Player.SetRoster(ConfiguredRoster);
}

void ANetBattleGameMode::Logout(AController* Exiting)
{
	if (ANetBattlePlayerState* LeavingPlayer = Exiting ? Exiting->GetPlayerState<ANetBattlePlayerState>() : nullptr)
	{
		PendingActions.Remove(LeavingPlayer);
		if (PendingForcedSwitchPlayer == LeavingPlayer)
		{
			PendingForcedSwitchPlayer = nullptr;
		}
	}
	Super::Logout(Exiting);

	if (ANetBattleGameState* BattleState = GetGameState<ANetBattleGameState>())
	{
		BattleState->SetBattleState(ENetBattlePhase::WaitingForPlayers, 0, TEXT("Player left. Waiting for two players..."));
	}
}

void ANetBattleGameMode::SubmitAction(ANetBattlePlayerController* Controller, ENetBattleActionType Type, int32 TargetIndex)
{
	ANetBattleGameState* BattleState = GetGameState<ANetBattleGameState>();
	ANetBattlePlayerState* PlayerState = Controller ? Controller->GetPlayerState<ANetBattlePlayerState>() : nullptr;
	if (!BattleState || !PlayerState)
	{
		return;
	}

	if (BattleState->BattlePhase == ENetBattlePhase::ChoosingForcedSwitch)
	{
		if (PlayerState != PendingForcedSwitchPlayer || Type != ENetBattleActionType::Switch
			|| !PlayerState->CanSwitchTo(TargetIndex))
		{
			return;
		}

		const int32 PreviousIndex = PlayerState->ActivePokemonIndex;
		if (!PlayerState->SwitchTo(TargetIndex))
		{
			return;
		}

		const FString SendOutEntry = FString::Printf(TEXT("去吧，%s！"),
			*GetPokemonLogId(PlayerState->GetActivePokemon()));
		PendingResolutionLog += SendOutEntry + TEXT(" ");
		BattleState->AppendBattleLog(SendOutEntry);
		FNetBattleCue SwitchCue;
		SwitchCue.Type = ENetBattleCueType::ForcedSwitch;
		SwitchCue.SourcePlayerId = PlayerState->GetPlayerId();
		SwitchCue.SourcePokemonIndex = PreviousIndex;
		SwitchCue.NewActivePokemonIndex = PlayerState->ActivePokemonIndex;
		SwitchCue.Message = PendingResolutionLog;
		SwitchCue.SuggestedDuration = 0.05f;
		BattleState->PublishBattleCue(SwitchCue);

		PlayerState->SetActionSubmitted(true);
		PendingForcedSwitchPlayer = nullptr;
		BattleState->SetBattleState(ENetBattlePhase::ResolvingTurn, BattleState->TurnNumber,
			TEXT("Sending out the selected Pokemon..."));
		FTimerHandle ForcedSwitchTimer;
		GetWorldTimerManager().SetTimer(ForcedSwitchTimer, this, &ANetBattleGameMode::CompleteForcedSwitch,
			SwitchCue.SuggestedDuration, false);
		return;
	}

	if (BattleState->BattlePhase != ENetBattlePhase::ChoosingActions || PlayerState->bActionSubmitted)
	{
		return;
	}

	if (Type == ENetBattleActionType::Switch && !PlayerState->CanSwitchTo(TargetIndex))
	{
		BattleState->SetBattleMessage(TEXT("Invalid switch. Choose a living reserve Pokemon."));
		return;
	}
	if (Type == ENetBattleActionType::Move
		&& !PlayerState->GetActivePokemon().Moves.IsValidIndex(TargetIndex))
	{
		BattleState->SetBattleMessage(TEXT("Invalid move. Choose an available move."));
		return;
	}
	if (Type != ENetBattleActionType::Move && Type != ENetBattleActionType::Switch)
	{
		return;
	}

	FNetBattleAction Action;
	Action.Type = Type;
	Action.MoveIndex = Type == ENetBattleActionType::Move ? TargetIndex : INDEX_NONE;
	Action.TargetIndex = TargetIndex;
	Action.ActingPokemonIndex = PlayerState->ActivePokemonIndex;
	PendingActions.Add(PlayerState, Action);
	PlayerState->SetActionSubmitted(true);
	BattleState->SetBattleMessage(TEXT("Action locked in. Waiting for the other player..."));

	const TArray<ANetBattlePlayerState*> Players = GetBattlePlayers();
	if (Players.Num() == 2 && Players.ContainsByPredicate([this](const ANetBattlePlayerState* Player)
	{
		return Player && !PendingActions.Contains(Player);
	}) == false)
	{
		ResolveTurn();
	}
}

TArray<ANetBattlePlayerState*> ANetBattleGameMode::GetBattlePlayers() const
{
	TArray<ANetBattlePlayerState*> Result;
	if (const ANetBattleGameState* BattleState = GetGameState<ANetBattleGameState>())
	{
		for (APlayerState* PlayerState : BattleState->PlayerArray)
		{
			if (ANetBattlePlayerState* BattlePlayer = Cast<ANetBattlePlayerState>(PlayerState))
			{
				Result.Add(BattlePlayer);
			}
		}
	}
	return Result;
}

void ANetBattleGameMode::TryStartBattle()
{
	TArray<ANetBattlePlayerState*> Players = GetBattlePlayers();
	ANetBattleGameState* BattleState = GetGameState<ANetBattleGameState>();
	if (!BattleState)
	{
		return;
	}

	if (Players.Num() == 2)
	{
		PendingActions.Reset();
		PendingForcedSwitchPlayer = nullptr;
		BattleState->ResetBattleLog();
		BattleState->AppendBattleLog(TEXT("战斗开始！"));
		for (int32 PlayerIndex = 0; PlayerIndex < Players.Num(); ++PlayerIndex)
		{
			InitializeRosterForPlayer(*Players[PlayerIndex], PlayerIndex);
		}
		BattleState->SetBattleState(ENetBattlePhase::ChoosingActions, 1, TEXT("Battle started. Choose an action."));
	}
	else
	{
		BattleState->SetBattleState(ENetBattlePhase::WaitingForPlayers, 0, TEXT("Waiting for two players..."));
	}
}

void ANetBattleGameMode::ResolveTurn()
{
	TArray<ANetBattlePlayerState*> Players = GetBattlePlayers();
	ANetBattleGameState* BattleState = GetGameState<ANetBattleGameState>();
	if (Players.Num() != 2 || !BattleState)
	{
		return;
	}

	BattleState->SetBattleState(ENetBattlePhase::ResolvingTurn, BattleState->TurnNumber, TEXT("Resolving turn..."));
	FString Log;
	float PresentationDuration = 0.0f;
	PresentationDuration += ResolveSwitch(*Players[0], PendingActions.FindRef(Players[0]), Log);
	PresentationDuration += ResolveSwitch(*Players[1], PendingActions.FindRef(Players[1]), Log);

	TArray<ANetBattlePlayerState*> MoveOrder = Players;
	MoveOrder.Sort([](const ANetBattlePlayerState& A, const ANetBattlePlayerState& B)
	{
		return A.GetActivePokemon().Speed > B.GetActivePokemon().Speed;
	});

	// Resolve equal-speed actions in a server-only random order. The server is
	// authoritative, so clients receive the same resulting cue order.
	for (int32 Start = 0; Start < MoveOrder.Num();)
	{
		int32 End = Start + 1;
		while (End < MoveOrder.Num()
			&& MoveOrder[End]->GetActivePokemon().Speed == MoveOrder[Start]->GetActivePokemon().Speed)
		{
			++End;
		}
		if (End - Start > 1)
		{
			for (int32 Index = End - 1; Index > Start; --Index)
			{
				const int32 SwapIndex = FMath::RandRange(Start, Index);
				MoveOrder.Swap(Index, SwapIndex);
			}
		}
		Start = End;
	}

	PendingMoveOrder.Reset();
	for (ANetBattlePlayerState* Player : MoveOrder)
	{
		PendingMoveOrder.Add(Player);
	}
	PendingMoveIndex = 0;
	PendingResolutionLog = Log;

	// Switch cues are published before the first move. Wait for their
	// presentation duration before resolving the first attack.
	FTimerHandle MoveTimer;
	GetWorldTimerManager().SetTimer(MoveTimer, this, &ANetBattleGameMode::ResolveNextMove,
		FMath::Max(0.05f, PresentationDuration), false);
}

void ANetBattleGameMode::ResolveNextMove()
{
	ANetBattleGameState* BattleState = GetGameState<ANetBattleGameState>();
	if (!BattleState || PendingMoveIndex >= PendingMoveOrder.Num())
	{
		BeginNextTurn(PendingResolutionLog);
		return;
	}

	ANetBattlePlayerState* Attacker = PendingMoveOrder[PendingMoveIndex++];
	if (!Attacker)
	{
		ResolveNextMove();
		return;
	}

	TArray<ANetBattlePlayerState*> Players = GetBattlePlayers();
	ANetBattlePlayerState* Defender = nullptr;
	for (ANetBattlePlayerState* Player : Players)
	{
		if (Player && Player != Attacker)
		{
			Defender = Player;
			break;
		}
	}

	const FNetBattleAction Action = PendingActions.FindRef(Attacker);
	if (!Defender || Action.Type != ENetBattleActionType::Move
		|| Action.ActingPokemonIndex != Attacker->ActivePokemonIndex
		|| Attacker->GetActivePokemon().IsFainted())
	{
		ResolveNextMove();
		return;
	}

	bool bBattleEnded = false;
	const float CueDuration = ResolveMove(*Attacker, *Defender, Action, PendingResolutionLog, bBattleEnded);
	if (bBattleEnded)
	{
		EndBattle(*Attacker, PendingResolutionLog);
		return;
	}
	if (PendingForcedSwitchPlayer)
	{
		FTimerHandle ForcedSwitchChoiceTimer;
		GetWorldTimerManager().SetTimer(ForcedSwitchChoiceTimer, this,
			&ANetBattleGameMode::BeginForcedSwitchChoice, FMath::Max(0.05f, CueDuration), false);
		return;
	}

	FTimerHandle NextMoveTimer;
	GetWorldTimerManager().SetTimer(NextMoveTimer, this, &ANetBattleGameMode::ResolveNextMove,
		FMath::Max(0.05f, CueDuration), false);
}

float ANetBattleGameMode::ResolveSwitch(ANetBattlePlayerState& Player, const FNetBattleAction& Action, FString& Log)
{
	const int32 PreviousIndex = Player.ActivePokemonIndex;
	const FString PreviousPokemonId = GetPokemonLogId(Player.GetActivePokemon());
	if (Action.Type == ENetBattleActionType::Switch && Player.SwitchTo(Action.TargetIndex))
	{
		const FString RecallEntry = FString::Printf(TEXT("回来吧，%s！"), *PreviousPokemonId);
		const FString SendOutEntry = FString::Printf(TEXT("去吧，%s！"),
			*GetPokemonLogId(Player.GetActivePokemon()));
		Log += RecallEntry + TEXT(" ") + SendOutEntry + TEXT(" ");
		GetGameState<ANetBattleGameState>()->AppendBattleLog(RecallEntry);
		GetGameState<ANetBattleGameState>()->AppendBattleLog(SendOutEntry);
		FNetBattleCue Cue;
		Cue.Type = ENetBattleCueType::Switch;
		Cue.SourcePlayerId = Player.GetPlayerId();
		Cue.SourcePokemonIndex = PreviousIndex;
		Cue.NewActivePokemonIndex = Player.ActivePokemonIndex;
		Cue.Message = Log;
		Cue.SuggestedDuration = 0.05f;
		GetGameState<ANetBattleGameState>()->PublishBattleCue(Cue);
		return Cue.SuggestedDuration;
	}
	return 0.0f;
}

float ANetBattleGameMode::ResolveMove(ANetBattlePlayerState& Attacker, ANetBattlePlayerState& Defender, const FNetBattleAction& Action, FString& Log, bool& bBattleEnded)
{
	bBattleEnded = false;
	float Duration = 0.0f;
	const int32 AttackerIndex = Attacker.ActivePokemonIndex;
	const int32 DefenderIndex = Defender.ActivePokemonIndex;
	const FNetPokemonState& ActivePokemon = Attacker.GetActivePokemon();
	const FNetMoveState& Move = ActivePokemon.Moves[Action.MoveIndex];
	const FNetPokemonState DefenderBefore = Defender.GetActivePokemon();
	const FString AttackerId = GetPokemonLogId(ActivePokemon);
	const FString DefenderId = GetPokemonLogId(DefenderBefore);
	const FString MoveId = GetMoveLogId(Move);
	const int32 Accuracy = FMath::Clamp(Move.Accuracy, 0, 100);
	const bool bHit = Accuracy >= 100 || FMath::RandRange(1, 100) <= Accuracy;
	const int32 DamageDone = bHit ? Defender.ApplyDamageToActive(
		CalculateAuthoritativeDamage(ActivePokemon, DefenderBefore, Move)) : 0;
	const FString MoveEntry = bHit
		? FString::Printf(TEXT("%s 使用了 %s！"), *AttackerId, *MoveId)
		: FString::Printf(TEXT("%s 使用了 %s，但没有命中！"), *AttackerId, *MoveId);
	Log += MoveEntry + TEXT(" ");
	GetGameState<ANetBattleGameState>()->AppendBattleLog(MoveEntry);
	if (bHit)
	{
		const float TypeMultiplier = GetTypeMultiplier(Move.MoveType, DefenderBefore.PrimaryType);
		UE_LOG(LogTemp, Display, TEXT("NetBattle type check: Move=%s Target=%s Multiplier=%.2f"),
			*Move.MoveType.ToString(), *DefenderBefore.PrimaryType.ToString(), TypeMultiplier);
		if (TypeMultiplier > 1.0f)
		{
			Log += TEXT("效果拔群！ ");
			GetGameState<ANetBattleGameState>()->AppendBattleLog(TEXT("效果拔群！"));
		}
		else if (TypeMultiplier < 1.0f)
		{
			Log += TEXT("效果不太理想…… ");
			GetGameState<ANetBattleGameState>()->AppendBattleLog(TEXT("效果不太理想……"));
		}
	}

	FNetBattleCue AttackCue;
	AttackCue.Type = ENetBattleCueType::Attack;
	AttackCue.SourcePlayerId = Attacker.GetPlayerId();
	AttackCue.TargetPlayerId = Defender.GetPlayerId();
	AttackCue.SourcePokemonIndex = AttackerIndex;
	AttackCue.TargetPokemonIndex = DefenderIndex;
	AttackCue.Damage = DamageDone;
	AttackCue.TargetHPBefore = DefenderBefore.CurrentHP;
	AttackCue.TargetHPAfter = Defender.GetActivePokemon().CurrentHP;
	AttackCue.Message = Log;
	AttackCue.SuggestedDuration = 1.4f;
	GetGameState<ANetBattleGameState>()->PublishBattleCue(AttackCue);
	Duration += AttackCue.SuggestedDuration;
	if (!bHit)
	{
		return Duration;
	}

	if (Defender.GetActivePokemon().IsFainted())
	{
		const FString FaintEntry = FString::Printf(TEXT("%s 倒下了！"), *DefenderId);
		Log += FaintEntry + TEXT(" ");
		GetGameState<ANetBattleGameState>()->AppendBattleLog(FaintEntry);
		FNetBattleCue FaintCue;
		FaintCue.Type = ENetBattleCueType::Faint;
		FaintCue.TargetPlayerId = Defender.GetPlayerId();
		FaintCue.TargetPokemonIndex = DefenderIndex;
		FaintCue.Message = Log;
		FaintCue.SuggestedDuration = 0.8f;
		GetGameState<ANetBattleGameState>()->PublishBattleCue(FaintCue);
		Duration += FaintCue.SuggestedDuration;

		if (!Defender.HasUsablePokemon())
		{
			bBattleEnded = true;
			return Duration;
		}

		PendingForcedSwitchPlayer = &Defender;
	}
	return Duration;
}

void ANetBattleGameMode::BeginForcedSwitchChoice()
{
	ANetBattleGameState* BattleState = GetGameState<ANetBattleGameState>();
	if (!BattleState || !PendingForcedSwitchPlayer || !PendingForcedSwitchPlayer->HasUsablePokemon())
	{
		PendingForcedSwitchPlayer = nullptr;
		BeginNextTurn(PendingResolutionLog);
		return;
	}

	PendingForcedSwitchPlayer->SetActionSubmitted(false);
	BattleState->SetBattleState(ENetBattlePhase::ChoosingForcedSwitch, BattleState->TurnNumber,
		TEXT("Choose a Pokemon to continue the battle."));
}

void ANetBattleGameMode::CompleteForcedSwitch()
{
	BeginNextTurn(PendingResolutionLog);
}

void ANetBattleGameMode::BeginNextTurn(FString PreviousTurnLog)
{
	ANetBattleGameState* BattleState = GetGameState<ANetBattleGameState>();
	if (!BattleState)
	{
		return;
	}

	PendingActions.Reset();
	PendingForcedSwitchPlayer = nullptr;
	for (ANetBattlePlayerState* Player : GetBattlePlayers())
	{
		Player->SetActionSubmitted(false);
	}
	BattleState->SetBattleState(ENetBattlePhase::ChoosingActions, BattleState->TurnNumber + 1, PreviousTurnLog);
}

void ANetBattleGameMode::EndBattle(ANetBattlePlayerState& Winner, const FString& FinalLog)
{
	PendingActions.Reset();
	PendingForcedSwitchPlayer = nullptr;
	if (ANetBattleGameState* BattleState = GetGameState<ANetBattleGameState>())
	{
		const FString VictoryEntry = FString::Printf(TEXT("战斗结束，%s获胜！"), *Winner.GetPlayerName());
		const FString VictoryMessage = FinalLog + VictoryEntry;
		BattleState->AppendBattleLog(VictoryEntry);
		FNetBattleCue Cue;
		Cue.Type = ENetBattleCueType::BattleEnded;
		Cue.SourcePlayerId = Winner.GetPlayerId();
		Cue.Message = VictoryMessage;
		Cue.SuggestedDuration = 1.0f;
		BattleState->PublishBattleCue(Cue);
		BattleState->SetBattleState(ENetBattlePhase::BattleEnded, BattleState->TurnNumber, VictoryMessage);
	}
}
