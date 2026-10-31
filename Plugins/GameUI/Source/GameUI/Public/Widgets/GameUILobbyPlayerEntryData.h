// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameUILobbyPlayerEntryData.generated.h"

/**
 * Read-only data for one row of the lobby player list. Rebuilt from the game state's
 * PlayerArray whenever it changes; rows are never mutated in place.
 */
UCLASS(BlueprintType)
class GAMEUI_API UGameUILobbyPlayerEntryData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Game UI|Lobby")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "Game UI|Lobby")
	int32 PlayerId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Game UI|Lobby")
	bool bIsReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Game UI|Lobby")
	bool bIsHost = false;

	UPROPERTY(BlueprintReadOnly, Category = "Game UI|Lobby")
	bool bIsLocalPlayer = false;
};
