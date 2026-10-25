// Diego Gallo All Rights Reserved


#include "Backends/FrontendSessionBackend.h"
#include "FrontendMultiplayerSubsystem.h"

void UFrontendSessionBackend::Initialize(UFrontendMultiplayerSubsystem* InOwningSubsystem)
{
	OwningSubsystem = InOwningSubsystem;
}

void UFrontendSessionBackend::ReportSessionList(const TArray<UFrontendSessionListEntryData*>& Sessions) const
{
	if (UFrontendMultiplayerSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->OnSessionListUpdated.Broadcast(Sessions);
	}
}

void UFrontendSessionBackend::ReportLeaderboard(ELeaderboardCategory Category, const TArray<UFrontendLeaderboardEntryData*>& Entries) const
{
	if (UFrontendMultiplayerSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->OnLeaderboardUpdated.Broadcast(Category, Entries);
	}
}

void UFrontendSessionBackend::LogDebug(const FString& LogLine) const
{
	if (UFrontendMultiplayerSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->BroadcastDebugLog(LogLine);
	}
}
