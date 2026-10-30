// Diego Gallo All Rights Reserved


#include "GameModes/GameUIMatchGameState.h"
#include "GameModes/GameUIMatchGameMode.h"
#include "GameModes/GameUIMatchPlayerState.h"
#include "GameFramework/GameMode.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

void AGameUIMatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGameUIMatchGameState, SessionDisplayName);
	DOREPLIFETIME(AGameUIMatchGameState, MaxPlayers);
	DOREPLIFETIME(AGameUIMatchGameState, StartCountdownEndServerTime);
}

bool AGameUIMatchGameState::IsInLobby() const
{
	// EnteringMap is deliberately not the lobby: on a client it is the pre-replication
	// default, and a late joiner must not flash the lobby before InProgress arrives.
	return GetMatchState() == MatchState::WaitingToStart;
}

int32 AGameUIMatchGameState::GetReadyPlayerCount() const
{
	int32 ReadyCount = 0;
	for (APlayerState* PlayerState : PlayerArray)
	{
		const AGameUIMatchPlayerState* MatchPlayerState = Cast<AGameUIMatchPlayerState>(PlayerState);
		if (MatchPlayerState && !MatchPlayerState->IsInactive() && MatchPlayerState->IsReady())
		{
			++ReadyCount;
		}
	}
	return ReadyCount;
}

bool AGameUIMatchGameState::AreAllPlayersReady() const
{
	int32 ActivePlayers = 0;
	for (APlayerState* PlayerState : PlayerArray)
	{
		const AGameUIMatchPlayerState* MatchPlayerState = Cast<AGameUIMatchPlayerState>(PlayerState);
		if (!MatchPlayerState || MatchPlayerState->IsInactive())
		{
			continue;
		}

		++ActivePlayers;
		if (!MatchPlayerState->IsReady())
		{
			return false;
		}
	}
	return ActivePlayers > 0;
}

float AGameUIMatchGameState::GetStartCountdownSecondsRemaining() const
{
	if (!IsStartCountdownActive())
	{
		return 0.f;
	}

	return FMath::Max(0.f, StartCountdownEndServerTime - GetServerWorldTimeSeconds());
}

AGameUIMatchPlayerState* AGameUIMatchGameState::GetHostPlayerState() const
{
	for (APlayerState* PlayerState : PlayerArray)
	{
		AGameUIMatchPlayerState* MatchPlayerState = Cast<AGameUIMatchPlayerState>(PlayerState);
		if (MatchPlayerState && MatchPlayerState->IsHost())
		{
			return MatchPlayerState;
		}
	}
	return nullptr;
}

void AGameUIMatchGameState::SetSessionInfo(const FString& InDisplayName, int32 InMaxPlayers)
{
	if (!HasAuthority())
	{
		return;
	}

	SessionDisplayName = InDisplayName;
	MaxPlayers = InMaxPlayers;
	OnRep_SessionInfo();
}

void AGameUIMatchGameState::SetStartCountdownEndServerTime(float InEndServerTime)
{
	if (!HasAuthority() || FMath::IsNearlyEqual(StartCountdownEndServerTime, InEndServerTime))
	{
		return;
	}

	StartCountdownEndServerTime = InEndServerTime;
	OnRep_StartCountdownEndServerTime();
}

void AGameUIMatchGameState::NotifyPlayerLobbyFlagsChanged(AGameUIMatchPlayerState* PlayerState)
{
	BroadcastLobbyStateChanged();
}

void AGameUIMatchGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	BroadcastLobbyStateChanged();
}

void AGameUIMatchGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	BroadcastLobbyStateChanged();
}

void AGameUIMatchGameState::OnRep_MatchState()
{
	Super::OnRep_MatchState();
	OnMatchPhaseChanged.Broadcast();
}

void AGameUIMatchGameState::OnRep_SessionInfo()
{
	OnLobbyStateChanged.Broadcast();
}

void AGameUIMatchGameState::OnRep_StartCountdownEndServerTime()
{
	OnStartCountdownChanged.Broadcast();
}

void AGameUIMatchGameState::BroadcastLobbyStateChanged()
{
	OnLobbyStateChanged.Broadcast();

	// The server re-checks whether a running countdown is still valid (someone un-readied
	// or left) or, on a dedicated server, whether everyone is now ready.
	if (HasAuthority())
	{
		if (AGameUIMatchGameMode* MatchGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGameUIMatchGameMode>() : nullptr)
		{
			MatchGameMode->HandleLobbyStateChanged();
		}
	}
}
