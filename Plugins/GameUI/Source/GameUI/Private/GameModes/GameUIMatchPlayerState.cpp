// Diego Gallo All Rights Reserved


#include "GameModes/GameUIMatchPlayerState.h"
#include "GameModes/GameUIMatchGameState.h"
#include "GameUILog.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

void AGameUIMatchPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGameUIMatchPlayerState, bIsReady);
	DOREPLIFETIME(AGameUIMatchPlayerState, bIsHost);
}

void AGameUIMatchPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	// Keeps the flags across seamless travel should a game ever enable it.
	if (AGameUIMatchPlayerState* MatchPlayerState = Cast<AGameUIMatchPlayerState>(PlayerState))
	{
		MatchPlayerState->bIsReady = bIsReady;
		MatchPlayerState->bIsHost = bIsHost;
		MatchPlayerState->PlayerSessionId = PlayerSessionId;
	}
}

void AGameUIMatchPlayerState::SetReady(bool bInReady)
{
	if (!HasAuthority() || bIsReady == bInReady)
	{
		return;
	}

	bIsReady = bInReady;
	UE_LOG(LogGameUI, Log, TEXT("lobby: %s is %s"), *GetPlayerName(), bIsReady ? TEXT("ready") : TEXT("not ready"));
	// OnRep is not called on the server; notify listeners here so server and client paths match.
	BroadcastLobbyFlagsChanged();
}

void AGameUIMatchPlayerState::SetIsHost(bool bInIsHost)
{
	if (!HasAuthority() || bIsHost == bInIsHost)
	{
		return;
	}

	bIsHost = bInIsHost;
	BroadcastLobbyFlagsChanged();
}

void AGameUIMatchPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();

	// A joining player's state usually replicates a frame or two before its PlayerName does,
	// and nothing else refreshes the lobby until the next ready / join / leave event - the row
	// sat blank for 17 s in one test log. Same notification as the lobby flags: the game state
	// rebroadcasts and the lobby screen re-reads the list.
	BroadcastLobbyFlagsChanged();
}

void AGameUIMatchPlayerState::OnRep_LobbyFlags()
{
	BroadcastLobbyFlagsChanged();
}

void AGameUIMatchPlayerState::BroadcastLobbyFlagsChanged()
{
	OnLobbyFlagsChanged.Broadcast(this);

	// The game state is the single place the lobby UI listens to. It may not have
	// replicated yet on a client; AddPlayerState will catch the UI up when it does.
	if (UWorld* World = GetWorld())
	{
		if (AGameUIMatchGameState* MatchGameState = World->GetGameState<AGameUIMatchGameState>())
		{
			MatchGameState->NotifyPlayerLobbyFlagsChanged(this);
		}
	}
}
