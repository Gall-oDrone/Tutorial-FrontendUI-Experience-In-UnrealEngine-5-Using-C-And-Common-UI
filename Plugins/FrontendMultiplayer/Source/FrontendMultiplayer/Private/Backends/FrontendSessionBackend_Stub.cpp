// Diego Gallo All Rights Reserved


#include "Backends/FrontendSessionBackend_Stub.h"
#include "FrontendLeaderboardEntryData.h"
#include "FrontendSessionListEntryData.h"

void UFrontendSessionBackend_Stub::CreateSession(FName SessionName, int32 MaxPlayers)
{
	LogDebug(FString::Printf(
		TEXT("[stub] CreateSession(%s, %d) called"),
		*SessionName.ToString(),
		MaxPlayers));

	LogDebug(TEXT("[stub] Session created (simulated)"));
}

void UFrontendSessionBackend_Stub::FindSessions()
{
	LogDebug(TEXT("[stub] FindSessions() called"));

	TArray<UFrontendSessionListEntryData*> FakeSessions;

	UFrontendSessionListEntryData* SessionA = NewObject<UFrontendSessionListEntryData>(this);
	SessionA->SessionDisplayName = TEXT("Desert Outpost");
	SessionA->CurrentPlayers = 2;
	SessionA->MaxPlayers = 4;
	SessionA->HostName = TEXT("Raven");
	SessionA->MapName = TEXT("Dunes");
	SessionA->PingInMs = 38;
	FakeSessions.Add(SessionA);

	UFrontendSessionListEntryData* SessionB = NewObject<UFrontendSessionListEntryData>(this);
	SessionB->SessionDisplayName = TEXT("Night Raid");
	SessionB->CurrentPlayers = 1;
	SessionB->MaxPlayers = 8;
	SessionB->HostName = TEXT("Nightjar");
	SessionB->MapName = TEXT("Harbor");
	SessionB->PingInMs = 72;
	FakeSessions.Add(SessionB);

	UFrontendSessionListEntryData* SessionC = NewObject<UFrontendSessionListEntryData>(this);
	SessionC->SessionDisplayName = TEXT("Training Grounds");
	SessionC->CurrentPlayers = 3;
	SessionC->MaxPlayers = 3;
	SessionC->HostName = TEXT("Atlas");
	SessionC->MapName = TEXT("Proving Grounds");
	SessionC->PingInMs = 15;
	FakeSessions.Add(SessionC);

	ReportSessionList(FakeSessions);
	LogDebug(FString::Printf(TEXT("[stub] FindSessions returned %d simulated session(s)"), FakeSessions.Num()));
}

void UFrontendSessionBackend_Stub::JoinSession(int32 SessionIndex)
{
	LogDebug(FString::Printf(TEXT("[stub] JoinSession(%d) called"), SessionIndex));
	LogDebug(TEXT("[stub] Session joined (simulated)"));
}

void UFrontendSessionBackend_Stub::FetchLeaderboard(ELeaderboardCategory Category)
{
	LogDebug(FString::Printf(TEXT("[stub] FetchLeaderboard(%s) called"), *UEnum::GetDisplayValueAsText(Category).ToString()));

	// Rows arrive already ranked, highest score first. Rank is the backend's
	// responsibility; the screen may reorder rows for display but never renumbers them.
	static const TCHAR* const SimulatedNames[] =
	{
		TEXT("Valkyrie"), TEXT("Ronin"),    TEXT("Cinder"),   TEXT("Specter"),  TEXT("Longshot"),
		TEXT("Nomad"),    TEXT("Havoc"),    TEXT("Wraith"),   TEXT("Tempest"),  TEXT("Ember"),
		TEXT("Raven"),    TEXT("Atlas"),    TEXT("Nightjar"), TEXT("Falcon"),   TEXT("Mirage"),
		TEXT("Blitz"),    TEXT("You"),      TEXT("Onyx"),     TEXT("Saber"),    TEXT("Quill"),
		TEXT("Drift"),    TEXT("Vortex"),   TEXT("Helix"),    TEXT("Gale"),     TEXT("Juno"),
		TEXT("Kestrel"),  TEXT("Lynx"),     TEXT("Mako"),     TEXT("Nova"),     TEXT("Orion"),
		TEXT("Pike"),     TEXT("Rook"),     TEXT("Sable"),    TEXT("Talon"),    TEXT("Umbra"),
		TEXT("Vesper"),   TEXT("Warden"),   TEXT("Zephyr"),   TEXT("Cobalt"),   TEXT("Dusk")
	};

	const int32 SimulatedRowCount = static_cast<int32>(UE_ARRAY_COUNT(SimulatedNames));
	// Different categories get different numbers so a future Ranked / Unranked split is visibly distinct.
	const int32 CategorySeed = static_cast<int32>(Category) * 7;

	TArray<UFrontendLeaderboardEntryData*> FakeEntries;
	FakeEntries.Reserve(SimulatedRowCount);

	int32 Score = 24000 - CategorySeed * 100;
	for (int32 RowIndex = 0; RowIndex < SimulatedRowCount; ++RowIndex)
	{
		UFrontendLeaderboardEntryData* Entry = NewObject<UFrontendLeaderboardEntryData>(this);
		Entry->Rank = RowIndex + 1;
		Entry->PlayerDisplayName = SimulatedNames[RowIndex];
		Entry->Score = Score;
		Entry->bIsLocalPlayer = FCString::Strcmp(SimulatedNames[RowIndex], TEXT("You")) == 0;
		FakeEntries.Add(Entry);

		Score -= 300 + ((RowIndex * 37 + CategorySeed) % 250);
	}

	ReportLeaderboard(Category, FakeEntries);
	LogDebug(FString::Printf(TEXT("[stub] FetchLeaderboard returned %d simulated entr(ies)"), FakeEntries.Num()));
}
