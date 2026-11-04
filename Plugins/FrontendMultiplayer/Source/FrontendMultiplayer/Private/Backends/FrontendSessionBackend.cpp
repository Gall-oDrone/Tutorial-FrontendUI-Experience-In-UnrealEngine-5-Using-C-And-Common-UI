// Diego Gallo All Rights Reserved


#include "Backends/FrontendSessionBackend.h"
#include "FrontendMultiplayerSubsystem.h"

void UFrontendSessionBackend::Initialize(UFrontendMultiplayerSubsystem* InOwningSubsystem)
{
	OwningSubsystem = InOwningSubsystem;
}

void UFrontendSessionBackend::ReportOperationStarted(EFrontendSessionOperation Operation) const
{
	if (UFrontendMultiplayerSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->NotifyOperationStarted(Operation);
	}
}

void UFrontendSessionBackend::ReportSessionCreated(bool bSucceeded, const FText& ErrorText) const
{
	if (UFrontendMultiplayerSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->NotifySessionCreated(bSucceeded, ErrorText);
	}
}

void UFrontendSessionBackend::ReportSessionsFound(const TArray<UFrontendSessionListEntryData*>& Sessions) const
{
	if (UFrontendMultiplayerSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->NotifySessionsFound(Sessions);
	}
}

void UFrontendSessionBackend::ReportFindSessionsFailed(const FText& ErrorText) const
{
	if (UFrontendMultiplayerSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->NotifyFindSessionsFailed(ErrorText);
	}
}

void UFrontendSessionBackend::ReportSessionJoined(bool bSucceeded, const FString& ConnectString, const FText& ErrorText) const
{
	if (UFrontendMultiplayerSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->NotifySessionJoined(bSucceeded, ConnectString, ErrorText);
	}
}

void UFrontendSessionBackend::ReportSessionLeft() const
{
	if (UFrontendMultiplayerSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->NotifySessionLeft();
	}
}

void UFrontendSessionBackend::ReportLoginChanged(bool bLoggedIn, const FText& ErrorText) const
{
	if (UFrontendMultiplayerSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->NotifyLoginChanged(bLoggedIn, ErrorText);
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
