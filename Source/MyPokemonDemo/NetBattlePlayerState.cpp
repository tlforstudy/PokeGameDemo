#include "NetBattlePlayerState.h"

#include "Net/UnrealNetwork.h"

ANetBattlePlayerState::ANetBattlePlayerState()
{
	bReplicates = true;
}

FNetPokemonState ANetBattlePlayerState::GetActivePokemon() const
{
	return Roster.IsValidIndex(ActivePokemonIndex) ? Roster[ActivePokemonIndex] : FNetPokemonState();
}

bool ANetBattlePlayerState::HasUsablePokemon() const
{
	return Roster.ContainsByPredicate([](const FNetPokemonState& Pokemon)
	{
		return !Pokemon.IsFainted();
	});
}

bool ANetBattlePlayerState::CanSwitchTo(int32 TargetIndex) const
{
	return Roster.IsValidIndex(TargetIndex)
		&& TargetIndex != ActivePokemonIndex
		&& !Roster[TargetIndex].IsFainted();
}

void ANetBattlePlayerState::InitializeDefaultRoster(int32 PlayerSlot)
{
	check(HasAuthority());

	Roster.Reset();
	const FString Prefix = PlayerSlot == 0 ? TEXT("A") : TEXT("B");

	FNetPokemonState Pikachu;
	Pikachu.PokemonId = FName(*FString::Printf(TEXT("Pikachu_%s"), *Prefix));
	Pikachu.SpeciesId = TEXT("Pikachu");
	Pikachu.DisplayName = FText::FromString(TEXT("Pikachu"));
	Pikachu.PrimaryType = TEXT("Grass");
	Pikachu.Level = 20;
	Pikachu.MaxHP = 100;
	Pikachu.CurrentHP = Pikachu.MaxHP;
	Pikachu.Attack = 35;
	Pikachu.Defense = 30;
	Pikachu.Speed = 90;
	FNetMoveState Ember;
	Ember.MoveId = TEXT("Ember");
	Ember.DisplayName = FText::FromString(TEXT("Ember"));
	Ember.Power = 40;
	Ember.Accuracy = 80;
	Ember.MaxPP = 25;
	Ember.MoveType = TEXT("Fire");
	Ember.Category = TEXT("Special");
	Pikachu.Moves = { Ember };

	FNetPokemonState Turtle;
	Turtle.PokemonId = FName(*FString::Printf(TEXT("Turtle_%s"), *Prefix));
	Turtle.SpeciesId = TEXT("Turtle");
	Turtle.DisplayName = FText::FromString(TEXT("Turtle"));
	Turtle.PrimaryType = TEXT("Water");
	Turtle.Level = 20;
	Turtle.MaxHP = 130;
	Turtle.CurrentHP = Turtle.MaxHP;
	Turtle.Attack = 45;
	Turtle.Defense = 45;
	Turtle.Speed = 40;
	FNetMoveState Tackle;
	Tackle.MoveId = TEXT("Tackle");
	Tackle.DisplayName = FText::FromString(TEXT("Tackle"));
	Tackle.Power = 25;
	Tackle.Accuracy = 100;
	Tackle.MaxPP = 35;
	Tackle.MoveType = TEXT("Normal");
	Tackle.Category = TEXT("Physical");
	Turtle.Moves = { Tackle };

	SetRoster({Pikachu, Turtle});
}

void ANetBattlePlayerState::SetRoster(const TArray<FNetPokemonState>& NewRoster)
{
	check(HasAuthority());
	Roster = NewRoster;
	ActivePokemonIndex = 0;
	bActionSubmitted = false;
	NotifyRosterChanged();
}

void ANetBattlePlayerState::SetActionSubmitted(bool bSubmitted)
{
	check(HasAuthority());
	bActionSubmitted = bSubmitted;
	NotifyRosterChanged();
}

bool ANetBattlePlayerState::SwitchTo(int32 TargetIndex)
{
	check(HasAuthority());
	if (!CanSwitchTo(TargetIndex))
	{
		return false;
	}

	ActivePokemonIndex = TargetIndex;
	NotifyRosterChanged();
	return true;
}

int32 ANetBattlePlayerState::ApplyDamageToActive(int32 Damage)
{
	check(HasAuthority());
	if (!Roster.IsValidIndex(ActivePokemonIndex))
	{
		return 0;
	}

	FNetPokemonState& ActivePokemon = Roster[ActivePokemonIndex];
	const int32 PreviousHP = ActivePokemon.CurrentHP;
	ActivePokemon.CurrentHP = FMath::Clamp(PreviousHP - FMath::Max(0, Damage), 0, ActivePokemon.MaxHP);
	NotifyRosterChanged();
	return PreviousHP - ActivePokemon.CurrentHP;
}

bool ANetBattlePlayerState::SwitchToFirstUsablePokemon()
{
	check(HasAuthority());
	for (int32 Index = 0; Index < Roster.Num(); ++Index)
	{
		if (!Roster[Index].IsFainted())
		{
			ActivePokemonIndex = Index;
			NotifyRosterChanged();
			return true;
		}
	}
	return false;
}

void ANetBattlePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANetBattlePlayerState, Roster);
	DOREPLIFETIME(ANetBattlePlayerState, ActivePokemonIndex);
	DOREPLIFETIME(ANetBattlePlayerState, bActionSubmitted);
}

void ANetBattlePlayerState::OnRep_Roster()
{
	NotifyRosterChanged();
}

void ANetBattlePlayerState::OnRep_ActionSubmitted()
{
	NotifyRosterChanged();
}

void ANetBattlePlayerState::NotifyRosterChanged()
{
	OnRosterChanged.Broadcast();
	if (HasAuthority())
	{
		ForceNetUpdate();
	}
}
