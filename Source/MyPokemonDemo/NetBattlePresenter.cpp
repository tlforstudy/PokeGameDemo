#include "NetBattlePresenter.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "NetBattleGameState.h"
#include "NetBattlePlayerState.h"
#include "NetBattleVisualInterface.h"
#include "TimerManager.h"

ANetBattlePresenter::ANetBattlePresenter()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	LocalSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("LocalSpawnPoint"));
	LocalSpawnPoint->SetupAttachment(SceneRoot);
	LocalSpawnPoint->SetRelativeLocation(FVector(250.0, -150.0, 0.0));
	LocalSpawnPoint->SetRelativeRotation(FRotator(0.0, 135.0, 0.0));

	OpponentSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("OpponentSpawnPoint"));
	OpponentSpawnPoint->SetupAttachment(SceneRoot);
	OpponentSpawnPoint->SetRelativeLocation(FVector(-250.0, 150.0, 0.0));
	OpponentSpawnPoint->SetRelativeRotation(FRotator(0.0, -45.0, 0.0));

	BattleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BattleCamera"));
	BattleCamera->SetupAttachment(SceneRoot);
	BattleCamera->SetRelativeLocation(FVector(700.0, -700.0, 450.0));
	BattleCamera->SetRelativeRotation(FRotator(-20.0, 135.0, 0.0));
	BattleCamera->FieldOfView = 60.0f;
}

void ANetBattlePresenter::BeginPlay()
{
	Super::BeginPlay();
	if (bUseBattleCamera)
	{
		if (APlayerController* LocalController = GetWorld()->GetFirstPlayerController())
		{
			LocalController->SetViewTarget(this);
		}
	}

	RefreshPresentation();
	GetWorldTimerManager().SetTimer(RefreshTimer, this, &ANetBattlePresenter::RefreshPresentation, 0.25f, true);
}

void ANetBattlePresenter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RefreshTimer);
	if (BoundGameState)
	{
		BoundGameState->OnBattleCueReceived.RemoveDynamic(this, &ANetBattlePresenter::HandleBattleCue);
	}
	Super::EndPlay(EndPlayReason);
}

void ANetBattlePresenter::RefreshPresentation()
{
	ANetBattleGameState* BattleState = GetWorld()->GetGameState<ANetBattleGameState>();
	if (!BattleState)
	{
		return;
	}

	if (BoundGameState != BattleState)
	{
		if (BoundGameState)
		{
			BoundGameState->OnBattleCueReceived.RemoveDynamic(this, &ANetBattlePresenter::HandleBattleCue);
		}
		BoundGameState = BattleState;
		BoundGameState->OnBattleCueReceived.AddUniqueDynamic(this, &ANetBattlePresenter::HandleBattleCue);
	}

	const APlayerController* LocalController = GetWorld()->GetFirstPlayerController();
	const ANetBattlePlayerState* LocalPlayer = LocalController ? LocalController->GetPlayerState<ANetBattlePlayerState>() : nullptr;
	for (APlayerState* BasePlayerState : BattleState->PlayerArray)
	{
		ANetBattlePlayerState* Player = Cast<ANetBattlePlayerState>(BasePlayerState);
		if (!Player || Player->Roster.IsEmpty())
		{
			continue;
		}

		const bool bLocalSide = Player == LocalPlayer;
		for (int32 RosterIndex = 0; RosterIndex < Player->Roster.Num(); ++RosterIndex)
		{
			AActor* Visual = FindPokemonVisual(Player->GetPlayerId(), RosterIndex);
			if (!IsValid(Visual))
			{
				Visual = SpawnVisual(*Player, RosterIndex, bLocalSide);
			}
			if (IsValid(Visual) && Visual->GetClass()->ImplementsInterface(UNetBattleVisualInterface::StaticClass()))
			{
				INetBattleVisualInterface::Execute_RefreshNetBattleVisual(Visual, Player->Roster[RosterIndex]);
			}
		}
		const int32 PlayerId = Player->GetPlayerId();
		const int32* LastActiveIndex = LastReplicatedActiveIndices.Find(PlayerId);
		if (!LastActiveIndex || *LastActiveIndex != Player->ActivePokemonIndex)
		{
			SetPlayerActiveVisual(*Player, Player->ActivePokemonIndex);
			LastReplicatedActiveIndices.Add(PlayerId, Player->ActivePokemonIndex);
		}
	}
}

AActor* ANetBattlePresenter::FindPokemonVisual(int32 PlayerId, int32 RosterIndex) const
{
	const TObjectPtr<AActor>* Found = SpawnedVisuals.Find(MakeVisualKey(PlayerId, RosterIndex));
	return Found ? Found->Get() : nullptr;
}

void ANetBattlePresenter::HandleBattleCue(FNetBattleCue Cue)
{
	OnPresenterCue.Broadcast(Cue);

	AActor* SourceVisual = FindPokemonVisual(Cue.SourcePlayerId, Cue.SourcePokemonIndex);
	AActor* TargetVisual = FindPokemonVisual(Cue.TargetPlayerId, Cue.TargetPokemonIndex);
	if (Cue.Type == ENetBattleCueType::Switch || Cue.Type == ENetBattleCueType::ForcedSwitch)
	{
		ExecuteVisualEvent(SourceVisual, Cue.Type, Cue, false);
		if (BoundGameState)
		{
			if (ANetBattlePlayerState* Player = BoundGameState->FindBattlePlayerById(Cue.SourcePlayerId))
			{
				// There is no switch-out animation yet, so avoid showing both Pokemon
				// at the same time. A future animation can replace this with a timed handoff.
				if (SourceVisual)
				{
					SourceVisual->SetActorHiddenInGame(true);
				}
				AActor* NewVisual = FindPokemonVisual(Cue.SourcePlayerId, Cue.NewActivePokemonIndex);
				if (NewVisual)
				{
					NewVisual->SetActorHiddenInGame(false);
				}
				ExecuteVisualEvent(NewVisual, Cue.Type, Cue, true);
				SetPlayerActiveVisual(*Player, Cue.NewActivePokemonIndex);
			}
		}
		return;
	}

	ExecuteVisualEvent(SourceVisual, Cue.Type, Cue, false);
	ExecuteVisualEvent(TargetVisual, Cue.Type, Cue, true);
}

TSubclassOf<AActor> ANetBattlePresenter::FindVisualClass(const FNetPokemonState& Pokemon) const
{
	const FName PokemonName(*Pokemon.DisplayName.ToString());
	for (const FNetPokemonVisualDefinition& Definition : VisualDefinitions)
	{
		if (Definition.DisplayName == PokemonName)
		{
			return Definition.VisualClass;
		}
	}
	return nullptr;
}

AActor* ANetBattlePresenter::SpawnVisual(ANetBattlePlayerState& Player, int32 RosterIndex, bool bLocalSide)
{
	if (!Player.Roster.IsValidIndex(RosterIndex))
	{
		return nullptr;
	}

	const TSubclassOf<AActor> VisualClass = FindVisualClass(Player.Roster[RosterIndex]);
	const USceneComponent* SpawnPoint = bLocalSide ? LocalSpawnPoint : OpponentSpawnPoint;
	if (!VisualClass || !SpawnPoint)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Visual = GetWorld()->SpawnActor<AActor>(VisualClass, SpawnPoint->GetComponentTransform(), SpawnParameters);
	if (!Visual)
	{
		return nullptr;
	}

	// Each machine owns its presentation-only Pokemon actors. The replicated roster
	// and battle cues are the shared source of truth, never these visual actors.
	Visual->SetReplicates(false);
	Visual->SetReplicateMovement(false);
	SpawnedVisuals.Add(MakeVisualKey(Player.GetPlayerId(), RosterIndex), Visual);
	Visual->SetActorHiddenInGame(RosterIndex != Player.ActivePokemonIndex);
	Visual->SetActorEnableCollision(false);
	if (Visual->GetClass()->ImplementsInterface(UNetBattleVisualInterface::StaticClass()))
	{
		INetBattleVisualInterface::Execute_InitializeNetBattleVisual(Visual, Player.Roster[RosterIndex],
			Player.GetPlayerId(), RosterIndex, bLocalSide);
	}
	return Visual;
}

void ANetBattlePresenter::SetPlayerActiveVisual(ANetBattlePlayerState& Player, int32 NewActiveIndex)
{
	for (int32 RosterIndex = 0; RosterIndex < Player.Roster.Num(); ++RosterIndex)
	{
		if (AActor* Visual = FindPokemonVisual(Player.GetPlayerId(), RosterIndex))
		{
			Visual->SetActorHiddenInGame(RosterIndex != NewActiveIndex);
		}
	}
}

int64 ANetBattlePresenter::MakeVisualKey(int32 PlayerId, int32 RosterIndex)
{
	return (static_cast<int64>(PlayerId) << 32) | static_cast<uint32>(RosterIndex);
}

void ANetBattlePresenter::ExecuteVisualEvent(AActor* Visual, ENetBattleCueType CueType, const FNetBattleCue& Cue, bool bTargetEvent) const
{
	if (!IsValid(Visual) || !Visual->GetClass()->ImplementsInterface(UNetBattleVisualInterface::StaticClass()))
	{
		return;
	}

	switch (CueType)
	{
	case ENetBattleCueType::Attack:
		if (bTargetEvent)
		{
			if (Cue.Damage > 0)
			{
				INetBattleVisualInterface::Execute_PlayNetHit(Visual, Cue);
			}
		}
		else
		{
			INetBattleVisualInterface::Execute_PlayNetAttack(Visual, Cue);
		}
		break;
	case ENetBattleCueType::Faint:
		if (bTargetEvent)
		{
			INetBattleVisualInterface::Execute_PlayNetFaint(Visual, Cue);
		}
		break;
	case ENetBattleCueType::Switch:
	case ENetBattleCueType::ForcedSwitch:
		if (bTargetEvent)
		{
			INetBattleVisualInterface::Execute_PlayNetSwitchIn(Visual, Cue);
		}
		else
		{
			INetBattleVisualInterface::Execute_PlayNetSwitchOut(Visual, Cue);
		}
		break;
	default:
		break;
	}
}
