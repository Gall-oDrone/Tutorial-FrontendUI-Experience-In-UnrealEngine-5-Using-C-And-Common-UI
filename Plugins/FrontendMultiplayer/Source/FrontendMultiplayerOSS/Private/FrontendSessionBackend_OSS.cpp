// Diego Gallo All Rights Reserved


#include "FrontendSessionBackend_OSS.h"
#include "FrontendMultiplayerOSSSettings.h"
#include "FrontendMultiplayerSettings.h"
#include "FrontendMultiplayerSubsystem.h"
#include "FrontendSessionListEntryData.h"
#include "FrontendLeaderboardEntryData.h"
#include "FrontendSimulatedData.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#define LOCTEXT_NAMESPACE "FrontendMultiplayerOSS"

namespace FrontendOSS
{
	/** Custom session setting carrying the name typed on the Host screen. */
	static const FName SettingKey_DisplayName(TEXT("FRONTEND_SESSION_DISPLAY_NAME"));

	static FText NoSessionInterfaceText()
	{
		return LOCTEXT("NoSessionInterface",
			"No online session interface is available. Enable an OnlineSubsystem plugin (e.g. OnlineSubsystemNull) and set [OnlineSubsystem] DefaultPlatformService in DefaultEngine.ini.");
	}

	static FText NoIdentityInterfaceText()
	{
		return LOCTEXT("NoIdentityInterface",
			"The online subsystem has no identity interface, so nobody can sign in. Check that the subsystem plugin is enabled and configured.");
	}

	static const TCHAR* BoolToString(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	static FText DescribeJoinResult(EOnJoinSessionCompleteResult::Type Result)
	{
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::SessionIsFull:
			return LOCTEXT("JoinSessionFull", "That session is full.");
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			return LOCTEXT("JoinSessionGone", "That session no longer exists. Refresh the list and try again.");
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
			return LOCTEXT("JoinNoAddress", "Could not resolve the host's address.");
		case EOnJoinSessionCompleteResult::AlreadyInSession:
			return LOCTEXT("JoinAlreadyIn", "You are already in a session. Leave it before joining another.");
		default:
			return LOCTEXT("JoinUnknown", "The online service rejected the join request.");
		}
	}

	static const TCHAR* JoinResultToString(EOnJoinSessionCompleteResult::Type Result)
	{
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::Success:                 return TEXT("Success");
		case EOnJoinSessionCompleteResult::SessionIsFull:           return TEXT("SessionIsFull");
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:     return TEXT("SessionDoesNotExist");
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress: return TEXT("CouldNotRetrieveAddress");
		case EOnJoinSessionCompleteResult::AlreadyInSession:        return TEXT("AlreadyInSession");
		default:                                                    return TEXT("UnknownError");
		}
	}
}

//~ Begin UFrontendSessionBackend Interface

void UFrontendSessionBackend_OSS::Shutdown()
{
	ClearDelegateHandles();
	LastSearch.Reset();
}

FString UFrontendSessionBackend_OSS::GetBackendDisplayName() const
{
	return FString::Printf(TEXT("OnlineSubsystem (%s)"), *GetSubsystemName().ToString());
}

bool UFrontendSessionBackend_OSS::GetActiveSessionInfo(FString& OutDisplayName, int32& OutMaxPlayers) const
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	const FNamedOnlineSession* NamedSession = SessionInterface.IsValid() ? SessionInterface->GetNamedSession(NAME_GameSession) : nullptr;
	if (!NamedSession)
	{
		return false;
	}

	if (!NamedSession->SessionSettings.Get(FrontendOSS::SettingKey_DisplayName, OutDisplayName) || OutDisplayName.IsEmpty())
	{
		OutDisplayName = NamedSession->OwningUserName;
	}

	OutMaxPlayers = NamedSession->SessionSettings.NumPublicConnections;
	return true;
}

FString UFrontendSessionBackend_OSS::GetHostTravelOptions() const
{
	// NetDriverEOS checks this option in InitListen and falls back to plain IP sockets,
	// so a LAN host stays reachable by address even when the EOS driver is configured.
	return IsLAN() ? TEXT("?bIsLanMatch") : TEXT("");
}

void UFrontendSessionBackend_OSS::CreateSession(FName SessionName, int32 MaxPlayers)
{
	ReportOperationStarted(EFrontendSessionOperation::Creating);

	PendingDisplayName = SessionName.ToString();
	PendingMaxPlayers = FMath::Max(1, MaxPlayers);

	if (!GetSessionInterface().IsValid())
	{
		ReportSessionCreated(false, FrontendOSS::NoSessionInterfaceText());
		return;
	}

	EnsureLoggedInThen(EFrontendOSSDeferredRequest::Create);
}

void UFrontendSessionBackend_OSS::FindSessions()
{
	ReportOperationStarted(EFrontendSessionOperation::Finding);

	if (!GetSessionInterface().IsValid())
	{
		ReportFindSessionsFailed(FrontendOSS::NoSessionInterfaceText());
		return;
	}

	EnsureLoggedInThen(EFrontendOSSDeferredRequest::Find);
}

void UFrontendSessionBackend_OSS::JoinSession(int32 SessionIndex)
{
	ReportOperationStarted(EFrontendSessionOperation::Joining);

	if (!LastSearch.IsValid() || !LastSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		ReportSessionJoined(false, FString(), LOCTEXT("JoinBadIndex", "That session is no longer in the list. Refresh and try again."));
		return;
	}

	PendingJoinIndex = SessionIndex;

	if (!GetSessionInterface().IsValid())
	{
		ReportSessionJoined(false, FString(), FrontendOSS::NoSessionInterfaceText());
		return;
	}

	EnsureLoggedInThen(EFrontendOSSDeferredRequest::Join);
}

void UFrontendSessionBackend_OSS::LeaveSession()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid() || !SessionInterface->GetNamedSession(NAME_GameSession))
	{
		LogDebug(TEXT("[oss] LeaveSession: no GameSession to leave"));
		return;
	}

	ReportOperationStarted(EFrontendSessionOperation::Leaving);
	DestroyStaleSessionThen(EFrontendOSSDeferredRequest::None);
}

void UFrontendSessionBackend_OSS::FetchLeaderboard(ELeaderboardCategory Category)
{
	const IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorldForOnline());
	const bool bHasLeaderboards = OnlineSubsystem && OnlineSubsystem->GetLeaderboardsInterface().IsValid();

	// Reading a real leaderboard needs a title-specific board definition and a logged-in
	// identity; neither exists for the Null subsystem. Until a service with boards is
	// configured, say so and optionally show the simulated rows.
	if (!bHasLeaderboards)
	{
		LogDebug(FString::Printf(TEXT("[oss] Leaderboards are not supported by %s"), *GetSubsystemName().ToString()));

		if (UFrontendMultiplayerSettings::Get()->bSimulateLeaderboardWhenUnsupported)
		{
			const TArray<UFrontendLeaderboardEntryData*> SimulatedEntries = FFrontendSimulatedData::MakeLeaderboard(this, Category);
			ReportLeaderboard(Category, SimulatedEntries);
			LogDebug(FString::Printf(TEXT("[oss] Showing %d simulated leaderboard row(s) (Simulate Leaderboard When Unsupported is on)"), SimulatedEntries.Num()));
		}
		else
		{
			ReportLeaderboard(Category, TArray<UFrontendLeaderboardEntryData*>());
		}
		return;
	}

	LogDebug(FString::Printf(TEXT("[oss] %s exposes a leaderboard interface but no board is configured for this title yet"), *GetSubsystemName().ToString()));
	ReportLeaderboard(Category, TArray<UFrontendLeaderboardEntryData*>());
}

//~ End UFrontendSessionBackend Interface

IOnlineSessionPtr UFrontendSessionBackend_OSS::GetSessionInterface() const
{
	return Online::GetSessionInterface(GetWorldForOnline());
}

IOnlineIdentityPtr UFrontendSessionBackend_OSS::GetIdentityInterface() const
{
	return Online::GetIdentityInterface(GetWorldForOnline());
}

UWorld* UFrontendSessionBackend_OSS::GetWorldForOnline() const
{
	const UFrontendMultiplayerSubsystem* Subsystem = GetOwningSubsystem();
	const UGameInstance* GameInstance = Subsystem ? Subsystem->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetWorld() : nullptr;
}

FName UFrontendSessionBackend_OSS::GetSubsystemName() const
{
	const IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorldForOnline());
	return OnlineSubsystem ? OnlineSubsystem->GetSubsystemName() : NAME_None;
}

bool UFrontendSessionBackend_OSS::IsLAN() const
{
	// Null has no backend service, so sessions can only be found by LAN broadcast.
	// Every other subsystem advertises through its service and uses presence/lobbies.
	return GetSubsystemName() == NULL_SUBSYSTEM;
}

//~ Begin sign-in

bool UFrontendSessionBackend_OSS::IsLoggedIn() const
{
	if (!RequiresLogin())
	{
		return true;
	}

	IOnlineIdentityPtr Identity = GetIdentityInterface();
	return Identity.IsValid() && Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn;
}

void UFrontendSessionBackend_OSS::Login()
{
	ReportOperationStarted(EFrontendSessionOperation::LoggingIn);

	if (IsLoggedIn())
	{
		LogDebug(FString::Printf(TEXT("[oss] Login: already signed in on %s"), *GetSubsystemName().ToString()));
		ReportLoginChanged(true);
		return;
	}

	EnsureLoggedInThen(EFrontendOSSDeferredRequest::None);
}

FString UFrontendSessionBackend_OSS::GetLocalPlayerDisplayName() const
{
	IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity.IsValid() || Identity->GetLoginStatus(0) != ELoginStatus::LoggedIn)
	{
		return FString();
	}

	return Identity->GetPlayerNickname(0);
}

void UFrontendSessionBackend_OSS::EnsureLoggedInThen(EFrontendOSSDeferredRequest Request)
{
	if (IsLoggedIn())
	{
		ContinueAfterLogin(Request);
		return;
	}

	IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity.IsValid())
	{
		const FText ErrorText = FrontendOSS::NoIdentityInterfaceText();
		ReportLoginChanged(false, ErrorText);
		FailDeferredRequest(Request, ErrorText);
		return;
	}

	DeferredRequest = Request;

	if (bLoginInFlight)
	{
		// The owning subsystem serialises operations, so this is only reachable if a
		// backend caller bypasses it. Let the running sign-in pick the new request up.
		LogDebug(TEXT("[oss] Sign-in already in progress; the new request will run when it completes"));
		return;
	}

	// Bound once for the backend's lifetime so a sign-out (expired token, signed out
	// elsewhere) reaches the UI even when nothing is in flight.
	if (!LoginStatusChangedHandle.IsValid())
	{
		LoginStatusChangedHandle = Identity->AddOnLoginStatusChangedDelegate_Handle(0,
			FOnLoginStatusChangedDelegate::CreateUObject(this, &ThisClass::HandleLoginStatusChanged));
	}

	bHasFallbackCredentials = false;
	const FOnlineAccountCredentials FirstCredentials = BuildCredentials(bHasFallbackCredentials, FallbackCredentials);
	StartLogin(FirstCredentials);
}

FOnlineAccountCredentials UFrontendSessionBackend_OSS::BuildCredentials(bool& bOutHasFallback, FOnlineAccountCredentials& OutFallback) const
{
	const UFrontendMultiplayerOSSSettings* Settings = UFrontendMultiplayerOSSSettings::Get();
	bOutHasFallback = false;

	// 1. Command line. This is how LaunchInstances.bat gives every window its own
	//    Dev Auth Tool user; the same account cannot be signed in twice on EOS.
	if (Settings->bAllowCommandLineCredentials)
	{
		FString Type;
		if (FParse::Value(FCommandLine::Get(), TEXT("AUTH_TYPE="), Type) && !Type.IsEmpty())
		{
			FString Id;
			FString Token;
			FParse::Value(FCommandLine::Get(), TEXT("AUTH_LOGIN="), Id);
			FParse::Value(FCommandLine::Get(), TEXT("AUTH_PASSWORD="), Token);
			return FOnlineAccountCredentials(Type, Id, Token);
		}
	}

	const FOnlineAccountCredentials Configured(Settings->CredentialType, Settings->LoginId, Settings->LoginToken);

	// 2. A refresh token cached by an earlier interactive sign-in on this machine, so
	//    the player is not sent to the browser every launch. Falls back to 3 on failure.
	if (Settings->bTryPersistentAuthFirst && !Settings->CredentialType.Equals(TEXT("persistentauth"), ESearchCase::IgnoreCase))
	{
		bOutHasFallback = true;
		OutFallback = Configured;
		return FOnlineAccountCredentials(TEXT("persistentauth"), FString(), FString());
	}

	// 3. Project Settings > Game > Frontend Multiplayer OSS.
	return Configured;
}

void UFrontendSessionBackend_OSS::StartLogin(const FOnlineAccountCredentials& Credentials)
{
	IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity.IsValid())
	{
		bLoginInFlight = true;
		FinishLogin(false, TEXT("No identity interface"));
		return;
	}

	CurrentCredentialType = Credentials.Type;
	bLoginInFlight = true;
	const int32 AttemptSerial = ++LoginAttemptSerial;

	LoginCompleteHandle = Identity->AddOnLoginCompleteDelegate_Handle(0,
		FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::HandleLoginComplete));

	LogDebug(FString::Printf(TEXT("[oss] Login(%s) on %s%s"),
		*CurrentCredentialType, *GetSubsystemName().ToString(),
		bHasFallbackCredentials ? *FString::Printf(TEXT(", then %s if that fails"), *FallbackCredentials.Type) : TEXT("")));

	// Some subsystems fire OnLoginComplete synchronously (and may already have moved on
	// to the fallback attempt) before returning false; only report a rejection that
	// produced no callback for *this* attempt.
	if (!Identity->Login(0, Credentials) && bLoginInFlight && LoginAttemptSerial == AttemptSerial)
	{
		FinishLogin(false, TEXT("The login request was rejected"));
	}
}

void UFrontendSessionBackend_OSS::HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	if (LocalUserNum != 0)
	{
		return;
	}

	FinishLogin(bWasSuccessful, Error);
}

void UFrontendSessionBackend_OSS::FinishLogin(bool bWasSuccessful, const FString& Error)
{
	if (!bLoginInFlight)
	{
		return;
	}
	bLoginInFlight = false;

	if (IOnlineIdentityPtr Identity = GetIdentityInterface())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteHandle);
	}
	LoginCompleteHandle.Reset();

	LogDebug(FString::Printf(TEXT("[oss] OnLoginComplete (%s) -> %s%s%s"),
		*CurrentCredentialType, bWasSuccessful ? TEXT("OK") : TEXT("FAILED"),
		Error.IsEmpty() ? TEXT("") : TEXT(": "), *Error));

	if (!bWasSuccessful && bHasFallbackCredentials)
	{
		bHasFallbackCredentials = false;
		StartLogin(FallbackCredentials);
		return;
	}

	const EFrontendOSSDeferredRequest Request = DeferredRequest;
	DeferredRequest = EFrontendOSSDeferredRequest::None;

	if (bWasSuccessful)
	{
		LogDebug(FString::Printf(TEXT("[oss] Signed in as '%s'"), *GetLocalPlayerDisplayName()));
		ReportLoginChanged(true);
		ContinueAfterLogin(Request);
		return;
	}

	const FText ErrorText = FText::Format(
		LOCTEXT("LoginFailed", "Could not sign in to {0} ({1}): {2}"),
		FText::FromName(GetSubsystemName()),
		FText::FromString(CurrentCredentialType),
		FText::FromString(Error.IsEmpty() ? FString(TEXT("unknown error")) : Error));

	ReportLoginChanged(false, ErrorText);
	FailDeferredRequest(Request, ErrorText);
}

void UFrontendSessionBackend_OSS::ContinueAfterLogin(EFrontendOSSDeferredRequest Request)
{
	switch (Request)
	{
	case EFrontendOSSDeferredRequest::Create:
		CreateSessionAfterLogin();
		break;

	case EFrontendOSSDeferredRequest::Find:
		FindSessionsNow();
		break;

	case EFrontendOSSDeferredRequest::Join:
		JoinSessionAfterLogin();
		break;

	default:
		// Explicit Login(): ReportLoginChanged has already closed the operation.
		break;
	}
}

void UFrontendSessionBackend_OSS::FailDeferredRequest(EFrontendOSSDeferredRequest Request, const FText& ErrorText)
{
	switch (Request)
	{
	case EFrontendOSSDeferredRequest::Create:
		ReportSessionCreated(false, ErrorText);
		break;

	case EFrontendOSSDeferredRequest::Find:
		ReportFindSessionsFailed(ErrorText);
		break;

	case EFrontendOSSDeferredRequest::Join:
		ReportSessionJoined(false, FString(), ErrorText);
		break;

	default:
		break;
	}
}

void UFrontendSessionBackend_OSS::HandleLoginStatusChanged(int32 LocalUserNum, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& NewId)
{
	if (LocalUserNum != 0)
	{
		return;
	}

	LogDebug(FString::Printf(TEXT("[oss] OnLoginStatusChanged %s -> %s"), ELoginStatus::ToString(OldStatus), ELoginStatus::ToString(NewStatus)));

	// A completed sign-in is reported from FinishLogin; only sign-outs are surfaced here.
	if (OldStatus == ELoginStatus::LoggedIn && NewStatus != ELoginStatus::LoggedIn && !bLoginInFlight)
	{
		ReportLoginChanged(false);
	}
}

//~ End sign-in

void UFrontendSessionBackend_OSS::CreateSessionAfterLogin()
{
	// A session left over from a previous match would make CreateSession fail; the
	// online service only allows one NAME_GameSession at a time.
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession))
	{
		LogDebug(TEXT("[oss] A previous GameSession still exists; destroying it before hosting"));
		DestroyStaleSessionThen(EFrontendOSSDeferredRequest::Create);
		return;
	}

	CreateSessionNow();
}

void UFrontendSessionBackend_OSS::CreateSessionNow()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		ReportSessionCreated(false, FrontendOSS::NoSessionInterfaceText());
		return;
	}

	const UFrontendMultiplayerSettings* Settings = UFrontendMultiplayerSettings::Get();
	const bool bLAN = IsLAN();

	FOnlineSessionSettings SessionSettings;
	SessionSettings.NumPublicConnections = PendingMaxPlayers;
	SessionSettings.NumPrivateConnections = 0;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowJoinInProgress = Settings->bAllowJoinInProgress;
	SessionSettings.bIsLANMatch = bLAN;
	SessionSettings.bIsDedicated = false;
	SessionSettings.bUsesPresence = !bLAN;
	SessionSettings.bAllowJoinViaPresence = !bLAN;
	SessionSettings.bUseLobbiesIfAvailable = !bLAN;
	SessionSettings.bAllowInvites = true;

	const FString MapName = Settings->HostMap.IsNull() ? FString() : Settings->HostMap.GetAssetName();
	SessionSettings.Set(SETTING_MAPNAME, MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(FrontendOSS::SettingKey_DisplayName, PendingDisplayName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateSessionComplete));

	LogDebug(FString::Printf(TEXT("[oss] CreateSession '%s' (%d players, LAN=%s, map=%s)"),
		*PendingDisplayName, PendingMaxPlayers, bLAN ? TEXT("true") : TEXT("false"), *MapName));

	if (!SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		ReportSessionCreated(false, LOCTEXT("CreateRejected", "The online service rejected the session request."));
	}
}

void UFrontendSessionBackend_OSS::FindSessionsNow()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		ReportFindSessionsFailed(FrontendOSS::NoSessionInterfaceText());
		return;
	}

	const UFrontendMultiplayerSettings* Settings = UFrontendMultiplayerSettings::Get();
	const bool bLAN = IsLAN();

	LastSearch = MakeShared<FOnlineSessionSearch>();
	LastSearch->bIsLanQuery = bLAN;
	LastSearch->MaxSearchResults = FMath::Max(1, Settings->MaxSearchResults);
	if (!bLAN)
	{
		// The host sets bUseLobbiesIfAvailable, so on EOS the match is a lobby, and EOS
		// only returns lobbies when the search asks for them. Steam ignores the key.
		LastSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
		LastSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}

	FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleFindSessionsComplete));

	LogDebug(FString::Printf(TEXT("[oss] FindSessions (LAN=%s, lobbies=%s, max=%d)"),
		FrontendOSS::BoolToString(bLAN), FrontendOSS::BoolToString(!bLAN), LastSearch->MaxSearchResults));

	if (!SessionInterface->FindSessions(0, LastSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		ReportFindSessionsFailed(LOCTEXT("FindRejected", "The online service rejected the session search."));
	}
}

void UFrontendSessionBackend_OSS::JoinSessionAfterLogin()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession))
	{
		LogDebug(TEXT("[oss] A previous GameSession still exists; destroying it before joining"));
		DestroyStaleSessionThen(EFrontendOSSDeferredRequest::Join);
		return;
	}

	JoinSessionNow();
}

void UFrontendSessionBackend_OSS::JoinSessionNow()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		ReportSessionJoined(false, FString(), FrontendOSS::NoSessionInterfaceText());
		return;
	}

	if (!LastSearch.IsValid() || !LastSearch->SearchResults.IsValidIndex(PendingJoinIndex))
	{
		ReportSessionJoined(false, FString(), LOCTEXT("JoinBadIndex", "That session is no longer in the list. Refresh and try again."));
		return;
	}

	const FOnlineSessionSearchResult& Result = LastSearch->SearchResults[PendingJoinIndex];

	JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinSessionComplete));

	LogDebug(FString::Printf(TEXT("[oss] JoinSession index %d (host %s)"), PendingJoinIndex, *Result.Session.OwningUserName));

	if (!SessionInterface->JoinSession(0, NAME_GameSession, Result))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		ReportSessionJoined(false, FString(), LOCTEXT("JoinRejected", "The online service rejected the join request."));
	}
}

void UFrontendSessionBackend_OSS::DestroyStaleSessionThen(EFrontendOSSDeferredRequest Request)
{
	DeferredRequest = Request;

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		HandleDestroySessionComplete(NAME_GameSession, false);
		return;
	}

	LogDebug(TEXT("[oss] DestroySession GameSession"));

	if (!SessionInterface->DestroySession(NAME_GameSession, FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroySessionComplete)))
	{
		HandleDestroySessionComplete(NAME_GameSession, false);
	}
}

void UFrontendSessionBackend_OSS::ClearDelegateHandles()
{
	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	}

	if (IOnlineIdentityPtr Identity = GetIdentityInterface())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteHandle);
		Identity->ClearOnLoginStatusChangedDelegate_Handle(0, LoginStatusChangedHandle);
	}

	bLoginInFlight = false;
	LoginCompleteHandle.Reset();
	LoginStatusChangedHandle.Reset();
}

//~ Begin IOnlineSession completion handlers

void UFrontendSessionBackend_OSS::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
	}

	LogDebug(FString::Printf(TEXT("[oss] OnCreateSessionComplete %s -> %s"), *SessionName.ToString(), bWasSuccessful ? TEXT("OK") : TEXT("FAILED")));

	if (!bWasSuccessful)
	{
		ReportSessionCreated(false, LOCTEXT("CreateFailed", "The online service could not create the session. Check the Output Log for details."));
		return;
	}

	// StartSession flips the session to InProgress so bAllowJoinInProgress is honoured.
	// It is fire-and-forget: the host travels now and the completion is only logged.
	if (SessionInterface.IsValid())
	{
		StartSessionCompleteHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(
			FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleStartSessionComplete));
		SessionInterface->StartSession(NAME_GameSession);
	}

	ReportSessionCreated(true);
}

void UFrontendSessionBackend_OSS::HandleStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteHandle);
	}

	LogDebug(FString::Printf(TEXT("[oss] OnStartSessionComplete %s -> %s"), *SessionName.ToString(), bWasSuccessful ? TEXT("OK") : TEXT("FAILED")));
}

void UFrontendSessionBackend_OSS::HandleFindSessionsComplete(bool bWasSuccessful)
{
	if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}

	if (!bWasSuccessful || !LastSearch.IsValid())
	{
		LogDebug(TEXT("[oss] OnFindSessionsComplete -> FAILED"));
		ReportFindSessionsFailed(LOCTEXT("FindFailed", "The session search failed. Check the Output Log for details."));
		return;
	}

	TArray<UFrontendSessionListEntryData*> Sessions;
	Sessions.Reserve(LastSearch->SearchResults.Num());

	for (const FOnlineSessionSearchResult& Result : LastSearch->SearchResults)
	{
		if (!Result.IsValid())
		{
			continue;
		}

		const FOnlineSessionSettings& ResultSettings = Result.Session.SessionSettings;

		UFrontendSessionListEntryData* Entry = NewObject<UFrontendSessionListEntryData>(this);

		FString DisplayName;
		if (!ResultSettings.Get(FrontendOSS::SettingKey_DisplayName, DisplayName) || DisplayName.IsEmpty())
		{
			DisplayName = Result.Session.OwningUserName;
		}
		Entry->SessionDisplayName = DisplayName;

		FString MapName;
		ResultSettings.Get(SETTING_MAPNAME, MapName);
		Entry->MapName = MapName;

		Entry->MaxPlayers = ResultSettings.NumPublicConnections;
		Entry->CurrentPlayers = FMath::Clamp(ResultSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections, 0, ResultSettings.NumPublicConnections);
		Entry->HostName = Result.Session.OwningUserName;
		Entry->PingInMs = Result.PingInMs;

		Sessions.Add(Entry);
	}

	LogDebug(FString::Printf(TEXT("[oss] OnFindSessionsComplete -> %d session(s)"), Sessions.Num()));
	ReportSessionsFound(Sessions);
}

void UFrontendSessionBackend_OSS::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	}

	LogDebug(FString::Printf(TEXT("[oss] OnJoinSessionComplete %s -> %s"), *SessionName.ToString(), FrontendOSS::JoinResultToString(Result)));

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		ReportSessionJoined(false, FString(), FrontendOSS::DescribeJoinResult(Result));
		return;
	}

	FString ConnectString;
	if (!SessionInterface.IsValid() || !SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString))
	{
		ReportSessionJoined(false, FString(), FrontendOSS::DescribeJoinResult(EOnJoinSessionCompleteResult::CouldNotRetrieveAddress));
		return;
	}

	ReportSessionJoined(true, ConnectString);
}

void UFrontendSessionBackend_OSS::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	LogDebug(FString::Printf(TEXT("[oss] OnDestroySessionComplete %s -> %s"), *SessionName.ToString(), bWasSuccessful ? TEXT("OK") : TEXT("FAILED")));

	const EFrontendOSSDeferredRequest Request = DeferredRequest;
	DeferredRequest = EFrontendOSSDeferredRequest::None;

	switch (Request)
	{
	case EFrontendOSSDeferredRequest::Create:
		if (bWasSuccessful)
		{
			CreateSessionNow();
		}
		else
		{
			ReportSessionCreated(false, LOCTEXT("DestroyBeforeCreateFailed", "Could not leave the previous session, so a new one cannot be hosted."));
		}
		break;

	case EFrontendOSSDeferredRequest::Join:
		if (bWasSuccessful)
		{
			JoinSessionNow();
		}
		else
		{
			ReportSessionJoined(false, FString(), LOCTEXT("DestroyBeforeJoinFailed", "Could not leave the previous session, so a new one cannot be joined."));
		}
		break;

	default:
		ReportSessionLeft();
		break;
	}
}

//~ End IOnlineSession completion handlers

#undef LOCTEXT_NAMESPACE
