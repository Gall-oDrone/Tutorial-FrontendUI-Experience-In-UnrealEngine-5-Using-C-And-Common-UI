// Diego Gallo All Rights Reserved


#include "GameModes/GameUIMatchPlayerState.h"
#include "GameModes/GameUIMatchGameState.h"
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
	}
}

void AGameUIMatchPlayerState::SetReady(bool bInReady)
{
	if (!HasAuthority() || bIsReady == bInReady)
	{
		return;
	}

	bIsReady = bInReady;
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
