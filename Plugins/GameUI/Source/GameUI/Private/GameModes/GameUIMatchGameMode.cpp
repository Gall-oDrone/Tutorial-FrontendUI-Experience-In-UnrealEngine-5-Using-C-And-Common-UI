// Diego Gallo All Rights Reserved


#include "GameModes/GameUIMatchGameMode.h"
#include "Controllers/GameUIMatchPlayerController.h"

AGameUIMatchGameMode::AGameUIMatchGameMode()
{
	PlayerControllerClass = AGameUIMatchPlayerController::StaticClass();
}
