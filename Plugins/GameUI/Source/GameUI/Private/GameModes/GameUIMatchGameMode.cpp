// Diego Gallo All Rights Reserved


#include "GameModes/GameUIMatchGameMode.h"
#include "GameUILog.h"
#include "GameModes/GameUIMatchGameState.h"
#include "GameModes/GameUIMatchPlayerState.h"
#include "Controllers/GameUIMatchPlayerController.h"
#include "Settings/GameUIDeveloperSettings.h"
#include "FrontendMultiplayerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameSession.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "GameUILobby"

AGameUIMatchGameMode::AGameUIMatchGameMode()
{
	PlayerControllerClass = AGameUIMatchPlayerController::StaticClass();
	GameStateClass = AGameUIMatchGameState::StaticClass();
	PlayerStateClass = AGameUIMatchPlayerState::StaticClass();

	// Wait for an explicit StartMatch; refined from settings in InitGame.
	bDelayedStart = true;
}

void AGameUIMatchGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Settings are not safe to read in the constructor (CDO order); here they are.
	bDelayedStart = UGameUIDeveloperSettings::Get()->bUseLobby;
}

void AGameUIMatchGameMode::InitGameState()
{
	Super::InitGameState();

	AGameUIMatchGameState* MatchGameState = GetMatchGameState();
	if (!MatchGameState)
	{
		UE_LOG(LogGameUI, Error, TEXT("Game State Class must derive from AGameUIMatchGameState for the lobby to work (check BP_MatchGameMode > Classes)."));
		return;
	}

	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UGameInstance::GetSubsystem<UFrontendMultiplayerSubsystem>(GetGameInstance()))
	{
		MultiplayerSubsystem->OnServerSessionInfoChanged.RemoveAll(this);
		MultiplayerSubsystem->OnServerSessionInfoChanged.AddUObject(this, &ThisClass::RefreshSessionInfoFromSubsystem);
	}

	RefreshSessionInfoFromSubsystem();
}

void AGameUIMatchGameMode::RefreshSessionInfoFromSubsystem()
{
	AGameUIMatchGameState* MatchGameState = GetMatchGameState();
	if (!MatchGameState)
	{
		return;
	}

	FString SessionDisplayName;
	int32 SessionMaxPlayers = 0;
	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UGameInstance::GetSubsystem<UFrontendMultiplayerSubsystem>(GetGameInstance()))
	{
		MultiplayerSubsystem->GetActiveSessionInfo(SessionDisplayName, SessionMaxPlayers);
	}

	if (SessionDisplayName.IsEmpty())
	{
		SessionDisplayName = LOCTEXT("DefaultSessionName", "Match").ToString();
	}

	MatchGameState->SetSessionInfo(SessionDisplayName, SessionMaxPlayers);
}

void AGameUIMatchGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UGameInstance::GetSubsystem<UFrontendMultiplayerSubsystem>(GetGameInstance()))
	{
		const FString PlayerSessionId = UGameplayStatics::ParseOption(Options, TEXT("PlayerSessionId"));
		MultiplayerSubsystem->AcceptPlayerSession(PlayerSessionId, ErrorMessage);
	}
}

FString AGameUIMatchGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	FString ErrorMessage = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	const FString PlayerSessionId = UGameplayStatics::ParseOption(Options, TEXT("PlayerSessionId"));
	if (!PlayerSessionId.IsEmpty() && NewPlayerController)
	{
		if (AGameUIMatchPlayerState* MatchPlayerState = NewPlayerController->GetPlayerState<AGameUIMatchPlayerState>())
		{
			MatchPlayerState->SetPlayerSessionId(PlayerSessionId);
		}
	}

	return ErrorMessage;
}

void AGameUIMatchGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// The listen host is the server's own local player. Flag it once here so clients can
	// show the host mark and the Start button without guessing.
	if (NewPlayer && NewPlayer->IsLocalPlayerController())
	{
		if (AGameUIMatchPlayerState* MatchPlayerState = NewPlayer->GetPlayerState<AGameUIMatchPlayerState>())
		{
			MatchPlayerState->SetIsHost(true);
		}
	}

	// Dedicated server under a session service: PreLogin accepted the PlayerSessionId; now make
	// sure the identity that connected is the one the slot was reserved for. The answer comes
	// back asynchronously, so a mismatch is a kick rather than a refused login.
	if (GetNetMode() == NM_DedicatedServer && NewPlayer)
	{
		const AGameUIMatchPlayerState* MatchPlayerState = NewPlayer->GetPlayerState<AGameUIMatchPlayerState>();
		UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UGameInstance::GetSubsystem<UFrontendMultiplayerSubsystem>(GetGameInstance());
		if (MatchPlayerState && MultiplayerSubsystem && !MatchPlayerState->GetPlayerSessionId().IsEmpty())
		{
			TWeakObjectPtr<APlayerController> WeakPlayer(NewPlayer);
			const FString PlayerName = MatchPlayerState->GetPlayerName();
			MultiplayerSubsystem->VerifyPlayerSession(
				MatchPlayerState->GetPlayerSessionId(),
				MatchPlayerState->GetUniqueId().ToString(),
				FOnFrontendPlayerSessionVerified::CreateWeakLambda(this, [this, WeakPlayer, PlayerName](bool bMatches, const FString& Reason)
				{
					APlayerController* Player = WeakPlayer.Get();
					if (bMatches || !Player)
					{
						return;
					}
					UE_LOG(LogGameUI, Warning, TEXT("kicking %s: %s"), *PlayerName, *Reason);
					if (GameSession)
					{
						GameSession->KickPlayer(Player, FText::FromString(Reason));
					}
				}));
		}
	}
}

void AGameUIMatchGameMode::Logout(AController* Exiting)
{
	if (Exiting)
	{
		if (const AGameUIMatchPlayerState* MatchPlayerState = Exiting->GetPlayerState<AGameUIMatchPlayerState>())
		{
			const FString& PlayerSessionId = MatchPlayerState->GetPlayerSessionId();
			if (!PlayerSessionId.IsEmpty())
			{
				if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UGameInstance::GetSubsystem<UFrontendMultiplayerSubsystem>(GetGameInstance()))
				{
					// Placeholder scoring (MatchCompletionLeaderboardPoints): a player who leaves a match
					// that has started has finished it. Before RemovePlayerSession, which is where the
					// backend forgets the player. The backend skips players it did not verify.
					const int32 CompletionPoints = UGameUIDeveloperSettings::Get()->MatchCompletionLeaderboardPoints;
					if (CompletionPoints > 0 && GetNetMode() == NM_DedicatedServer && GetMatchState() == MatchState::InProgress)
					{
						MultiplayerSubsystem->AwardLeaderboardPoints(PlayerSessionId, MatchPlayerState->GetPlayerName(), CompletionPoints, ELeaderboardCategory::Main);
					}
					MultiplayerSubsystem->RemovePlayerSession(PlayerSessionId);
				}
			}
		}
	}

	Super::Logout(Exiting);

	// Dedicated server with nobody left. A one-session-per-process service (GameLift) ends
	// the session and recycles the process; otherwise reset an active match back to the lobby.
	if (GetNetMode() == NM_DedicatedServer && GetNumPlayers() == 0)
	{
		UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UGameInstance::GetSubsystem<UFrontendMultiplayerSubsystem>(GetGameInstance());
		const bool bBackendRecyclesProcess = MultiplayerSubsystem && MultiplayerSubsystem->NotifyServerSessionEmpty();
		if (!bBackendRecyclesProcess && GetMatchState() == MatchState::InProgress)
		{
			UE_LOG(LogGameUI, Log, TEXT("Last player left dedicated server during active match; restarting back to lobby."));
			RestartGame();
		}
	}
}

void AGameUIMatchGameMode::HandleMatchHasStarted()
{
	// Spawns every player that can restart and tells the online session the match began.
	Super::HandleMatchHasStarted();

	GetWorldTimerManager().ClearTimer(StartCountdownTimerHandle);
	if (AGameUIMatchGameState* MatchGameState = GetMatchGameState())
	{
		MatchGameState->SetStartCountdownEndServerTime(-1.f);
	}
}

bool AGameUIMatchGameMode::CanStartMatch(FText& OutFailReason) const
{
	const AGameUIMatchGameState* MatchGameState = GetMatchGameState();
	if (!MatchGameState)
	{
		OutFailReason = LOCTEXT("NoGameState", "The match state is not available.");
		return false;
	}

	// One rule, shared with the lobby UI on clients.
	return MatchGameState->CanStartMatch(OutFailReason);
}

bool AGameUIMatchGameMode::RequestStartMatch(AController* Requester, FText& OutFailReason)
{
	if (!HasAuthority())
	{
		OutFailReason = LOCTEXT("NotAuthority", "Only the server can start the match.");
		return false;
	}

	const AGameUIMatchPlayerState* RequesterState = Requester ? Requester->GetPlayerState<AGameUIMatchPlayerState>() : nullptr;
	if (!RequesterState || !RequesterState->IsHost())
	{
		OutFailReason = LOCTEXT("NotHost", "Only the host can start the match.");
		return false;
	}

	AGameUIMatchGameState* MatchGameState = GetMatchGameState();
	if (MatchGameState && MatchGameState->IsStartCountdownActive())
	{
		OutFailReason = FText::GetEmpty();
		return true;
	}

	if (!CanStartMatch(OutFailReason))
	{
		return false;
	}

	BeginStartCountdown();
	return true;
}

void AGameUIMatchGameMode::CancelStartCountdown()
{
	GetWorldTimerManager().ClearTimer(StartCountdownTimerHandle);

	if (AGameUIMatchGameState* MatchGameState = GetMatchGameState())
	{
		MatchGameState->SetStartCountdownEndServerTime(-1.f);
	}
}

void AGameUIMatchGameMode::HandleLobbyStateChanged()
{
	if (GetMatchState() != MatchState::WaitingToStart)
	{
		return;
	}

	AGameUIMatchGameState* MatchGameState = GetMatchGameState();
	if (!MatchGameState)
	{
		return;
	}

	FText Reason;
	const bool bCanStart = CanStartMatch(Reason);

	// Someone un-readied or left mid-countdown: stop it so nobody is dragged into a match.
	if (MatchGameState->IsStartCountdownActive() && !bCanStart)
	{
		UE_LOG(LogGameUI, Log, TEXT("start countdown cancelled (%s)"), *Reason.ToString());
		CancelStartCountdown();
		return;
	}

	// With no host player (dedicated server) nobody can press Start, so the lobby starts
	// itself once everyone is ready, if the project opted in.
	const bool bHasHost = MatchGameState->GetHostPlayerState() != nullptr;
	if (bCanStart && !bHasHost && !MatchGameState->IsStartCountdownActive() && UGameUIDeveloperSettings::Get()->bAutoStartWhenAllReady)
	{
		BeginStartCountdown();
	}
}

void AGameUIMatchGameMode::BeginStartCountdown()
{
	AGameUIMatchGameState* MatchGameState = GetMatchGameState();
	if (!MatchGameState)
	{
		return;
	}

	const float CountdownSeconds = FMath::Max(0.f, UGameUIDeveloperSettings::Get()->StartCountdownSeconds);
	if (CountdownSeconds <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogGameUI, Log, TEXT("starting match immediately (no countdown)."));
		StartMatch();
		return;
	}

	UE_LOG(LogGameUI, Log, TEXT("start countdown began (%.1fs)."), CountdownSeconds);

	MatchGameState->SetStartCountdownEndServerTime(MatchGameState->GetServerWorldTimeSeconds() + CountdownSeconds);
	GetWorldTimerManager().SetTimer(StartCountdownTimerHandle, this, &ThisClass::OnStartCountdownFinished, CountdownSeconds, false);
}

void AGameUIMatchGameMode::OnStartCountdownFinished()
{
	FText Reason;
	if (!CanStartMatch(Reason))
	{
		UE_LOG(LogGameUI, Log, TEXT("countdown finished but the lobby is no longer ready (%s)."), *Reason.ToString());
		CancelStartCountdown();
		return;
	}

	UE_LOG(LogGameUI, Log, TEXT("countdown finished; starting match."));
	StartMatch();
}

AGameUIMatchGameState* AGameUIMatchGameMode::GetMatchGameState() const
{
	return GetGameState<AGameUIMatchGameState>();
}

#undef LOCTEXT_NAMESPACE
