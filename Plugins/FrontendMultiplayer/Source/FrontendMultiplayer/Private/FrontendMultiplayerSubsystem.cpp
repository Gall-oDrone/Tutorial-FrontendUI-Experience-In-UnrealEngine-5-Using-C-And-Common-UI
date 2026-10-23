// Diego Gallo All Rights Reserved


#include "FrontendMultiplayerSubsystem.h"
#include "FrontendMultiplayerSettings.h"
#include "Backends/FrontendSessionBackend.h"
#include "Backends/FrontendSessionBackend_Stub.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

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
		BackendClass = UFrontendSessionBackend_Stub::StaticClass();
	}

	ActiveBackend = NewObject<UFrontendSessionBackend>(this, BackendClass);
	ActiveBackend->Initialize(this);

	BroadcastDebugLog(FString::Printf(TEXT("Session backend: %s"), *BackendClass->GetName()));
}

void UFrontendMultiplayerSubsystem::BroadcastDebugLog(const FString& LogLine)
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *LogLine);

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

void UFrontendMultiplayerSubsystem::CreateSession(FName SessionName, int32 MaxPlayers)
{
	if (ActiveBackend)
	{
		ActiveBackend->CreateSession(SessionName, MaxPlayers);
	}
}

void UFrontendMultiplayerSubsystem::FindSessions()
{
	if (ActiveBackend)
	{
		ActiveBackend->FindSessions();
	}
}

void UFrontendMultiplayerSubsystem::JoinSession(int32 SessionIndex)
{
	if (ActiveBackend)
	{
		ActiveBackend->JoinSession(SessionIndex);
	}
}

void UFrontendMultiplayerSubsystem::FetchLeaderboard(ELeaderboardCategory Category)
{
	if (ActiveBackend)
	{
		ActiveBackend->FetchLeaderboard(Category);
	}
}
