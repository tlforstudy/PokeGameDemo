#include "NetBattleMenuGameMode.h"

#include "NetBattleMenuPlayerController.h"

ANetBattleMenuGameMode::ANetBattleMenuGameMode()
{
	PlayerControllerClass = ANetBattleMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	bStartPlayersAsSpectators = true;
}
