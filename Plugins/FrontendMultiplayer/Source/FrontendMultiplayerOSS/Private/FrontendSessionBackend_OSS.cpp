// Diego Gallo All Rights Reserved


#include "FrontendSessionBackend_OSS.h"
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
#include "Engine/GameInstance.h"
#include "Engine/World.h"

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

void UFrontendSessionBackend_OSS::CreateSession(FName SessionName, int32 MaxPlayers)
{
	ReportOperationStarted(EFrontendSessionOperation::Creating);

	PendingDisplayName = SessionName.ToString();
	PendingMaxPlayers = FMath::Max(1, MaxPlayers);

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		ReportSessionCreated(false, FrontendOSS::NoSessionInterfaceText());
		return;
	}

	// A session left over from a previous match would make CreateSession fail; the
	// online service only allows one NAME_GameSession at a time.
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		LogDebug(TEXT("[oss] A previous GameSession still exists; destroying it before hosting"));
		DestroyStaleSessionThen(EFrontendOSSDeferredRequest::Create);
		return;
	}

	CreateSessionNow();
}

void UFrontendSessionBackend_OSS::FindSessions()
{
	ReportOperationStarted(EFrontendSessionOperation::Finding);

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
		LastSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	}

	FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleFindSessionsComplete));

	LogDebug(FString::Printf(TEXT("[oss] FindSessions (LAN=%s, max=%d)"), bLAN ? TEXT("true") : TEXT("false"), LastSearch->MaxSearchResults));

	if (!SessionInterface->FindSessions(0, LastSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		ReportFindSessionsFailed(LOCTEXT("FindRejected", "The online service rejected the session search."));
	}
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

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		ReportSessionJoined(false, FString(), FrontendOSS::NoSessionInterfaceText());
		return;
	}

	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		LogDebug(TEXT("[oss] A previous GameSession still exists; destroying it before joining"));
		DestroyStaleSessionThen(EFrontendOSSDeferredRequest::Join);
		return;
	}

	JoinSessionNow();
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
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return;
	}

	SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
	SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteHandle);
	SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
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
