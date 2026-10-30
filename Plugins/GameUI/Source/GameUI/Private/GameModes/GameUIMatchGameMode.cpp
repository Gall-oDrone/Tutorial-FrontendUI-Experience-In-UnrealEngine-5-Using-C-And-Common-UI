// Diego Gallo All Rights Reserved


#include "GameModes/GameUIMatchGameMode.h"
#include "GameModes/GameUIMatchGameState.h"
#include "GameModes/GameUIMatchPlayerState.h"
#include "Controllers/GameUIMatchPlayerController.h"
#include "Settings/GameUIDeveloperSettings.h"
#include "FrontendMultiplayerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
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
		UE_LOG(LogTemp, Error, TEXT("GameUI: Game State Class must derive from AGameUIMatchGameState for the lobby to work (check BP_MatchGameMode > Classes)."));
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
		UE_LOG(LogTemp, Log, TEXT("GameUI: start countdown cancelled (%s)"), *Reason.ToString());
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
		UE_LOG(LogTemp, Log, TEXT("GameUI: starting match immediately (no countdown)."));
		StartMatch();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("GameUI: start countdown began (%.1fs)."), CountdownSeconds);

	MatchGameState->SetStartCountdownEndServerTime(MatchGameState->GetServerWorldTimeSeconds() + CountdownSeconds);
	GetWorldTimerManager().SetTimer(StartCountdownTimerHandle, this, &ThisClass::OnStartCountdownFinished, CountdownSeconds, false);
}

void AGameUIMatchGameMode::OnStartCountdownFinished()
{
	FText Reason;
	if (!CanStartMatch(Reason))
	{
		UE_LOG(LogTemp, Log, TEXT("GameUI: countdown finished but the lobby is no longer ready (%s)."), *Reason.ToString());
		CancelStartCountdown();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("GameUI: countdown finished; starting match."));
	StartMatch();
}

AGameUIMatchGameState* AGameUIMatchGameMode::GetMatchGameState() const
{
	return GetGameState<AGameUIMatchGameState>();
}

#undef LOCTEXT_NAMESPACE
