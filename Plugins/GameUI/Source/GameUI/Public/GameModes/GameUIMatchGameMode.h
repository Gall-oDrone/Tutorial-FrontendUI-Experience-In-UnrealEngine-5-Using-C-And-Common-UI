// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameUIMatchGameMode.generated.h"

class AGameUIMatchGameState;

/**
 * Game mode for gameplay maps. Uses AGameMode's match states as the lobby:
 *
 *   WaitingToStart  = lobby (bDelayedStart; nobody gets a pawn)
 *   InProgress      = match (StartMatch spawns everyone; late joiners spawn on arrival)
 *
 * Ready flags live on AGameUIMatchPlayerState, the countdown on AGameUIMatchGameState.
 * The listen host (the first local player on the server) is the only one who may start,
 * unless Game UI Settings -> Lobby -> Auto Start When All Ready is on (dedicated servers).
 *
 * Pawn, HUD class and rules remain the game's business. Subclass in Blueprint for the map.
 */
UCLASS()
class GAMEUI_API AGameUIMatchGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AGameUIMatchGameMode();

	/**
	 * Begin the start countdown (or start immediately if the countdown is 0). Only the host
	 * may call this. Returns false and a reason when the lobby is not ready.
	 */
	bool RequestStartMatch(AController* Requester, FText& OutFailReason);

	/** Stop a running countdown. Any player may trigger this indirectly by un-readying. */
	void CancelStartCountdown();

	/** True when every active player is ready and Min Players To Start is met. */
	bool CanStartMatch(FText& OutFailReason) const;

	/** Called by the game state whenever players or Ready flags change. */
	void HandleLobbyStateChanged();

protected:
	//~ Begin AGameModeBase Interface
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	//~ End AGameModeBase Interface

	//~ Begin AGameMode Interface
	virtual void HandleMatchHasStarted() override;
	//~ End AGameMode Interface

private:
	void BeginStartCountdown();
	void OnStartCountdownFinished();
	AGameUIMatchGameState* GetMatchGameState() const;

	FTimerHandle StartCountdownTimerHandle;
};
