// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Controllers/FrontendPlayerController.h"
#include "GameUIMatchPlayerController.generated.h"

class UWidget_PrimaryLayout;
class UWidget_HUDBase;
class UWidget_PauseMenu;
class UWidget_LobbyScreen;
class AGameStateBase;
class AGameUIMatchGameState;

/**
 * Player controller for gameplay maps. On BeginPlay it creates the primary layout, pushes
 * the HUD onto the GameHud stack under a loading-screen hold, binds the pause action, and
 * - while the game state is WaitingToStart - pushes the lobby onto the GameMenu stack.
 * When the match starts the lobby closes and the HUD's Game input config takes over.
 *
 * Lobby actions (Ready, Start, Leave) are forwarded to the server through RPCs here; the
 * lobby widget never talks to the game mode directly.
 * The frontend map keeps its own (Blueprint) controller; the two flows stay separate.
 */
UCLASS()
class GAMEUI_API AGameUIMatchPlayerController : public AFrontendPlayerController
{
	GENERATED_BODY()

public:
	//~ Begin pause menu
	UFUNCTION(BlueprintCallable, Category = "Game UI|Pause Menu")
	void OpenPauseMenu();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Pause Menu")
	void ClosePauseMenu();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Pause Menu")
	void TogglePauseMenu();

	UFUNCTION(BlueprintPure, Category = "Game UI|Pause Menu")
	bool IsPauseMenuOpen() const { return bIsPauseMenuOpen; }
	//~ End pause menu

	//~ Begin lobby
	UFUNCTION(BlueprintCallable, Category = "Game UI|Lobby")
	void SetReady(bool bReady);

	UFUNCTION(BlueprintCallable, Category = "Game UI|Lobby")
	void ToggleReady();

	/** Host only; the server refuses anyone else and reports why through a modal. */
	UFUNCTION(BlueprintCallable, Category = "Game UI|Lobby")
	void RequestStartMatch();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Lobby")
	void CancelStartMatch();

	/** Leave the session and travel back to the frontend map. */
	UFUNCTION(BlueprintCallable, Category = "Game UI|Lobby")
	void LeaveMatch();

	UFUNCTION(BlueprintPure, Category = "Game UI|Lobby")
	bool IsLobbyOpen() const { return bIsLobbyOpen; }
	//~ End lobby

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

	//~ Begin APlayerController Interface
	virtual void SetupInputComponent() override;
	//~ End APlayerController Interface

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetReady(bool bReady);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestStartMatch();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerCancelStartMatch();

	/** The server declined a Start request; shown as an Ok modal on the requesting client. */
	UFUNCTION(Client, Reliable)
	void ClientStartMatchRejected(const FString& Reason);

private:
	void AddMatchMappingContext();
	void CreatePrimaryLayout();
	void PushHUD();

	void OnPauseActionTriggered();
	void HandlePauseMenuCloseRequested();
	void SetWorldPausedForMenu(bool bPaused);
	void FocusGameViewport();

	void BindToGameState(AGameStateBase* GameState);
	void HandleMatchPhaseChanged();
	void HandleLobbyStateChanged();
	void OpenLobby();
	void CloseLobby();
	void PushMatchInfoToHUD();

	UPROPERTY(Transient)
	TObjectPtr<UWidget_PrimaryLayout> CreatedPrimaryLayout;

	TWeakObjectPtr<UWidget_HUDBase> ActiveHUD;
	TWeakObjectPtr<UWidget_PauseMenu> ActivePauseMenu;
	TWeakObjectPtr<UWidget_LobbyScreen> ActiveLobby;
	TWeakObjectPtr<AGameUIMatchGameState> BoundGameState;

	FDelegateHandle GameStateSetHandle;

	bool bIsPauseMenuOpen = false;
	bool bIsLobbyOpen = false;
};
