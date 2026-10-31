// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GameUIMatchGameState.generated.h"

class AGameUIMatchPlayerState;

/**
 * Game state for gameplay maps. AGameState already replicates MatchState; this adds the
 * lobby data the UI needs (session name/capacity, start countdown) and turns player
 * join/leave/ready changes into one delegate the lobby screen listens to.
 *
 * WaitingToStart is the lobby, InProgress is the match (see AGameUIMatchGameMode).
 */
UCLASS()
class GAMEUI_API AGameUIMatchGameState : public AGameState
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE(FOnMatchGameStateEvent);

	/** A player joined or left, a Ready/host flag flipped, or the session info arrived. */
	FOnMatchGameStateEvent OnLobbyStateChanged;

	/** MatchState changed (WaitingToStart -> InProgress -> ...). */
	FOnMatchGameStateEvent OnMatchPhaseChanged;

	/** The start countdown began, was cancelled or was retimed. */
	FOnMatchGameStateEvent OnStartCountdownChanged;

	/** True while the match is waiting for the host to start it. */
	UFUNCTION(BlueprintPure, Category = "Game UI|Lobby")
	bool IsInLobby() const;

	UFUNCTION(BlueprintPure, Category = "Game UI|Lobby")
	FString GetSessionDisplayName() const { return SessionDisplayName; }

	/** Capacity from the online session, or 0 if unknown. */
	UFUNCTION(BlueprintPure, Category = "Game UI|Lobby")
	int32 GetMaxPlayers() const { return MaxPlayers; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Lobby")
	int32 GetReadyPlayerCount() const;

	UFUNCTION(BlueprintPure, Category = "Game UI|Lobby")
	bool AreAllPlayersReady() const;

	/**
	 * True when the lobby may start: WaitingToStart, Min Players To Start met, everyone ready.
	 * Evaluated identically on server and clients (settings are config, state is replicated).
	 */
	UFUNCTION(BlueprintPure, Category = "Game UI|Lobby")
	bool CanStartMatch(FText& OutFailReason) const;

	UFUNCTION(BlueprintPure, Category = "Game UI|Lobby")
	bool IsStartCountdownActive() const { return StartCountdownEndServerTime >= 0.f; }

	/** Seconds until the match starts, 0 when no countdown is running. Valid on clients (uses server time). */
	UFUNCTION(BlueprintPure, Category = "Game UI|Lobby")
	float GetStartCountdownSecondsRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Game UI|Lobby")
	AGameUIMatchPlayerState* GetHostPlayerState() const;

	//~ Server-only setters, called by AGameUIMatchGameMode.
	void SetSessionInfo(const FString& InDisplayName, int32 InMaxPlayers);
	void SetStartCountdownEndServerTime(float InEndServerTime);

	/** Called by player states when their flags change, on server and clients. */
	void NotifyPlayerLobbyFlagsChanged(AGameUIMatchPlayerState* PlayerState);

protected:
	//~ Begin AActor Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End AActor Interface

	//~ Begin AGameStateBase Interface
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	//~ End AGameStateBase Interface

	//~ Begin AGameState Interface
	virtual void OnRep_MatchState() override;
	//~ End AGameState Interface

	UFUNCTION()
	void OnRep_SessionInfo();

	UFUNCTION()
	void OnRep_StartCountdownEndServerTime();

private:
	/** Broadcasts to the UI and, on the server, lets the game mode re-evaluate the countdown. */
	void BroadcastLobbyStateChanged();

	UPROPERTY(ReplicatedUsing = OnRep_SessionInfo)
	FString SessionDisplayName;

	UPROPERTY(ReplicatedUsing = OnRep_SessionInfo)
	int32 MaxPlayers = 0;

	/** Server world time at which the match starts; negative when no countdown is running. */
	UPROPERTY(ReplicatedUsing = OnRep_StartCountdownEndServerTime)
	float StartCountdownEndServerTime = -1.f;
};
