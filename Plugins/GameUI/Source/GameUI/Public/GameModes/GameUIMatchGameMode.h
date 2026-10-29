// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameUIMatchGameMode.generated.h"

/**
 * Game mode for gameplay maps. Only sets the player controller to the match controller;
 * pawn, HUD class and rules are the game's business. Subclass in Blueprint for the test map.
 */
UCLASS()
class GAMEUI_API AGameUIMatchGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGameUIMatchGameMode();
};
