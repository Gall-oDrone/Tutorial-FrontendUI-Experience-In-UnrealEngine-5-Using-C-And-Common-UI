// Diego Gallo All Rights Reserved


#include "Backends/FrontendSessionBackend_Stub.h"
#include "FrontendSimulatedData.h"
#include "FrontendLeaderboardEntryData.h"
#include "FrontendSessionListEntryData.h"

#define LOCTEXT_NAMESPACE "FrontendMultiplayer"

void UFrontendSessionBackend_Stub::CreateSession(FName SessionName, int32 MaxPlayers)
{
	ReportOperationStarted(EFrontendSessionOperation::Creating);

	LogDebug(FString::Printf(
		TEXT("[stub] CreateSession(%s, %d) called"),
		*SessionName.ToString(),
		MaxPlayers));

	SimulatedSessionName = SessionName.ToString();
	SimulatedMaxPlayers = FMath::Max(1, MaxPlayers);

	LogDebug(TEXT("[stub] Session created (simulated); travelling to Host Map as listen server"));
	ReportSessionCreated(true);
}

void UFrontendSessionBackend_Stub::FindSessions()
{
	ReportOperationStarted(EFrontendSessionOperation::Finding);

	LogDebug(TEXT("[stub] FindSessions() called"));

	const TArray<UFrontendSessionListEntryData*> FakeSessions = FFrontendSimulatedData::MakeSessions(this);

	ReportSessionsFound(FakeSessions);
	LogDebug(FString::Printf(TEXT("[stub] FindSessions returned %d simulated session(s)"), FakeSessions.Num()));
}

void UFrontendSessionBackend_Stub::JoinSession(int32 SessionIndex)
{
	ReportOperationStarted(EFrontendSessionOperation::Joining);

	LogDebug(FString::Printf(TEXT("[stub] JoinSession(%d) called; nothing to connect to"), SessionIndex));

	ReportSessionJoined(false, FString(),
		LOCTEXT("StubCannotJoin", "The simulated backend has no sessions to join. Select a real Session Backend Class in Project Settings > Game > Frontend Multiplayer Settings."));
}

void UFrontendSessionBackend_Stub::LeaveSession()
{
	LogDebug(TEXT("[stub] LeaveSession() called; clearing the simulated session"));
	SimulatedSessionName.Reset();
	SimulatedMaxPlayers = 0;
}

bool UFrontendSessionBackend_Stub::GetActiveSessionInfo(FString& OutDisplayName, int32& OutMaxPlayers) const
{
	if (SimulatedSessionName.IsEmpty())
	{
		return false;
	}

	OutDisplayName = SimulatedSessionName;
	OutMaxPlayers = SimulatedMaxPlayers;
	return true;
}

void UFrontendSessionBackend_Stub::FetchLeaderboard(ELeaderboardCategory Category)
{
	LogDebug(FString::Printf(TEXT("[stub] FetchLeaderboard(%s) called"), *UEnum::GetDisplayValueAsText(Category).ToString()));

	const TArray<UFrontendLeaderboardEntryData*> FakeEntries = FFrontendSimulatedData::MakeLeaderboard(this, Category);

	ReportLeaderboard(Category, FakeEntries);
	LogDebug(FString::Printf(TEXT("[stub] FetchLeaderboard returned %d simulated entr(ies)"), FakeEntries.Num()));
}

#undef LOCTEXT_NAMESPACE
