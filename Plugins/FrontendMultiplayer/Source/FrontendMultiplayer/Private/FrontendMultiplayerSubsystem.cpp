// Diego Gallo All Rights Reserved


#include "FrontendMultiplayerSubsystem.h"
#include "FrontendMultiplayerLog.h"
#include "FrontendMultiplayerSettings.h"
#include "Backends/FrontendSessionBackend.h"
#include "Backends/FrontendSessionBackend_Stub.h"
#include "Subsystems/FrontendUISubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "FrontendMultiplayer"

UFrontendMultiplayerSubsystem* UFrontendMultiplayerSubsystem::Get(const UObject* WorldContextObject)
{
	if (GEngine)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
		return UGameInstance::GetSubsystem<UFrontendMultiplayerSubsystem>(World->GetGameInstance());
	}

	return nullptr;
}

void UFrontendMultiplayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CreateSessionBackend();
}

void UFrontendMultiplayerSubsystem::Deinitialize()
{
	if (ActiveBackend)
	{
		ActiveBackend->Shutdown();
		ActiveBackend = nullptr;
	}

	Super::Deinitialize();
}

void UFrontendMultiplayerSubsystem::CreateSessionBackend()
{
	const UFrontendMultiplayerSettings* Settings = UFrontendMultiplayerSettings::Get();

	UClass* BackendClass = nullptr;
	if (!Settings->SessionBackendClass.IsNull())
	{
		BackendClass = Settings->SessionBackendClass.LoadSynchronous();
	}

	// An unset, unloadable or abstract class must never leave the UI without a backend;
	// the stub keeps every screen functional and says so in the debug log.
	if (!BackendClass || BackendClass->HasAnyClassFlags(CLASS_Abstract))
	{
		if (!Settings->SessionBackendClass.IsNull())
		{
			UE_LOG(LogFrontendMultiplayer, Warning, TEXT("Session Backend Class '%s' could not be loaded; falling back to the stub backend."),
				*Settings->SessionBackendClass.ToString());
		}

		BackendClass = UFrontendSessionBackend_Stub::StaticClass();
	}

	ActiveBackend = NewObject<UFrontendSessionBackend>(this, BackendClass);
	ActiveBackend->Initialize(this);

	BroadcastDebugLog(FString::Printf(TEXT("Session backend: %s"), *ActiveBackend->GetBackendDisplayName()));
}

void UFrontendMultiplayerSubsystem::BroadcastDebugLog(const FString& LogLine)
{
	UE_LOG(LogFrontendMultiplayer, Log, TEXT("%s"), *LogLine);

	const FString StampedLine = FString::Printf(TEXT("[%s] %s"), *FDateTime::Now().ToString(TEXT("%H:%M:%S")), *LogLine);

	const int32 MaxHistory = FMath::Max(1, UFrontendMultiplayerSettings::Get()->APIDebugLogHistorySize);
	APIDebugLogHistory.Add(StampedLine);
	if (APIDebugLogHistory.Num() > MaxHistory)
	{
		APIDebugLogHistory.RemoveAt(0, APIDebugLogHistory.Num() - MaxHistory);
	}

	OnAPIDebugLogUpdated.Broadcast(StampedLine);
}

void UFrontendMultiplayerSubsystem::ClearAPIDebugLog()
{
	APIDebugLogHistory.Reset();
	OnAPIDebugLogCleared.Broadcast();
}

//~ Begin UI requests

void UFrontendMultiplayerSubsystem::CreateSession(FName SessionName, int32 MaxPlayers)
{
	if (ActiveBackend && CanStartOperation(EFrontendSessionOperation::Creating))
	{
		ActiveBackend->CreateSession(SessionName, MaxPlayers);
	}
}

void UFrontendMultiplayerSubsystem::FindSessions()
{
	if (ActiveBackend && CanStartOperation(EFrontendSessionOperation::Finding))
	{
		ActiveBackend->FindSessions();
	}
}

void UFrontendMultiplayerSubsystem::JoinSession(int32 SessionIndex)
{
	if (ActiveBackend && CanStartOperation(EFrontendSessionOperation::Joining))
	{
		ActiveBackend->JoinSession(SessionIndex);
	}
}

void UFrontendMultiplayerSubsystem::LeaveSession()
{
	// Deliberately not gated on CanStartOperation: leaving is how a stuck or stale
	// session gets cleaned up, so it must always reach the backend.
	if (ActiveBackend)
	{
		ActiveBackend->LeaveSession();
	}
}

void UFrontendMultiplayerSubsystem::FetchLeaderboard(ELeaderboardCategory Category)
{
	if (ActiveBackend)
	{
		ActiveBackend->FetchLeaderboard(Category);
	}
}

bool UFrontendMultiplayerSubsystem::GetActiveSessionInfo(FString& OutDisplayName, int32& OutMaxPlayers) const
{
	OutDisplayName.Reset();
	OutMaxPlayers = 0;

	return ActiveBackend ? ActiveBackend->GetActiveSessionInfo(OutDisplayName, OutMaxPlayers) : false;
}

//~ End UI requests

//~ Begin backend result sink

void UFrontendMultiplayerSubsystem::NotifyOperationStarted(EFrontendSessionOperation Operation)
{
	CurrentOperation = Operation;
	OnSessionOperationChanged.Broadcast(Operation, true);
}

void UFrontendMultiplayerSubsystem::NotifySessionCreated(bool bSucceeded, const FText& ErrorText)
{
	if (!bSucceeded)
	{
		FailOperation(EFrontendSessionOperation::Creating, LOCTEXT("CreateFailedTitle", "Could Not Host"), ErrorText);
		return;
	}

	FinishOperation(EFrontendSessionOperation::Creating);
	TravelToHostMap();
}

void UFrontendMultiplayerSubsystem::NotifySessionsFound(const TArray<UFrontendSessionListEntryData*>& Sessions)
{
	FinishOperation(EFrontendSessionOperation::Finding);
	OnSessionListUpdated.Broadcast(Sessions);
}

void UFrontendMultiplayerSubsystem::NotifyFindSessionsFailed(const FText& ErrorText)
{
	// Clear the list so the browser shows its empty state rather than stale rows.
	OnSessionListUpdated.Broadcast(TArray<UFrontendSessionListEntryData*>());
	FailOperation(EFrontendSessionOperation::Finding, LOCTEXT("FindFailedTitle", "Search Failed"), ErrorText);
}

void UFrontendMultiplayerSubsystem::NotifySessionJoined(bool bSucceeded, const FString& ConnectString, const FText& ErrorText)
{
	if (!bSucceeded)
	{
		FailOperation(EFrontendSessionOperation::Joining, LOCTEXT("JoinFailedTitle", "Could Not Join"), ErrorText);
		return;
	}

	FinishOperation(EFrontendSessionOperation::Joining);
	TravelToConnectString(ConnectString);
}

void UFrontendMultiplayerSubsystem::NotifySessionLeft()
{
	FinishOperation(EFrontendSessionOperation::Leaving);
}

//~ End backend result sink

bool UFrontendMultiplayerSubsystem::CanStartOperation(EFrontendSessionOperation Operation)
{
	if (CurrentOperation == EFrontendSessionOperation::None)
	{
		return true;
	}

	BroadcastDebugLog(FString::Printf(TEXT("Ignored %s: %s is still in progress."),
		*UEnum::GetDisplayValueAsText(Operation).ToString(),
		*UEnum::GetDisplayValueAsText(CurrentOperation).ToString()));

	return false;
}

void UFrontendMultiplayerSubsystem::FinishOperation(EFrontendSessionOperation Operation)
{
	// A backend may answer an operation the subsystem never saw start (e.g. a
	// LeaveSession issued while something else was running). Only clear our own.
	if (CurrentOperation == Operation)
	{
		CurrentOperation = EFrontendSessionOperation::None;
	}

	OnSessionOperationChanged.Broadcast(Operation, false);
}

void UFrontendMultiplayerSubsystem::FailOperation(EFrontendSessionOperation Operation, const FText& Title, const FText& Message)
{
	FinishOperation(Operation);

	const FText SafeMessage = Message.IsEmpty() ? LOCTEXT("UnknownSessionError", "The online service reported an unknown error.") : Message;

	BroadcastDebugLog(FString::Printf(TEXT("%s: %s"), *Title.ToString(), *SafeMessage.ToString()));

	if (UFrontendUISubsystem* FrontendUISubsystem = UGameInstance::GetSubsystem<UFrontendUISubsystem>(GetGameInstance()))
	{
		FrontendUISubsystem->PushConfirmScreenToModalStackAsync(EConfirmScreenType::Ok, Title, SafeMessage, [](EConfirmScreenButtonType) {});
	}

	OnSessionOperationFailed.Broadcast(Operation, Title, SafeMessage);
}

void UFrontendMultiplayerSubsystem::TravelToHostMap()
{
	const UFrontendMultiplayerSettings* Settings = UFrontendMultiplayerSettings::Get();

	if (Settings->HostMap.IsNull())
	{
		FailOperation(EFrontendSessionOperation::Creating,
			LOCTEXT("CreateFailedTitle", "Could Not Host"),
			LOCTEXT("HostMapNotSet", "Host Map is not set. Choose the match map in Project Settings > Game > Frontend Multiplayer Settings."));
		return;
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		FailOperation(EFrontendSessionOperation::Creating,
			LOCTEXT("CreateFailedTitle", "Could Not Host"),
			LOCTEXT("NoWorldToTravel", "There is no world to travel from."));
		return;
	}

	// "?listen" turns the loaded map into a listen server; the online session created by
	// the backend outlives the map change because it belongs to the online subsystem.
	const FString TravelURL = Settings->HostMap.ToSoftObjectPath().GetLongPackageName() + TEXT("?listen");

	BroadcastDebugLog(FString::Printf(TEXT("ServerTravel %s"), *TravelURL));
	World->ServerTravel(TravelURL);
}

void UFrontendMultiplayerSubsystem::TravelToConnectString(const FString& ConnectString)
{
	if (ConnectString.IsEmpty())
	{
		FailOperation(EFrontendSessionOperation::Joining,
			LOCTEXT("JoinFailedTitle", "Could Not Join"),
			LOCTEXT("EmptyConnectString", "The online service did not return an address for this session."));
		return;
	}

	APlayerController* PlayerController = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (!PlayerController)
	{
		FailOperation(EFrontendSessionOperation::Joining,
			LOCTEXT("JoinFailedTitle", "Could Not Join"),
			LOCTEXT("NoPlayerToTravel", "There is no local player to travel with."));
		return;
	}

	BroadcastDebugLog(FString::Printf(TEXT("ClientTravel %s"), *ConnectString));
	PlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
}

#undef LOCTEXT_NAMESPACE
