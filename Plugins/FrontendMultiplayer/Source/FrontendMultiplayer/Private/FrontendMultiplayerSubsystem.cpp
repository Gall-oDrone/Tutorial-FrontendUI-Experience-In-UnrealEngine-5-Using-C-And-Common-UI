// Diego Gallo All Rights Reserved


#include "FrontendMultiplayerSubsystem.h"
#include "FrontendLeaderboardEntryData.h"
#include "FrontendSessionListEntryData.h"
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

void UFrontendMultiplayerSubsystem::BroadcastDebugLog(const FString& LogLine)
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *LogLine);
	OnAPIDebugLogUpdated.Broadcast(LogLine);
}

void UFrontendMultiplayerSubsystem::CreateSession(FName SessionName, int32 MaxPlayers)
{
	BroadcastDebugLog(FString::Printf(
		TEXT("[stub] CreateSession(%s, %d) called"),
		*SessionName.ToString(),
		MaxPlayers));

	BroadcastDebugLog(TEXT("[stub] Session created (simulated)"));
}

void UFrontendMultiplayerSubsystem::FindSessions()
{
	BroadcastDebugLog(TEXT("[stub] FindSessions() called"));

	TArray<UFrontendSessionListEntryData*> FakeSessions;

	UFrontendSessionListEntryData* SessionA = NewObject<UFrontendSessionListEntryData>(this);
	SessionA->SessionDisplayName = TEXT("Desert Outpost");
	SessionA->CurrentPlayers = 2;
	SessionA->MaxPlayers = 4;
	FakeSessions.Add(SessionA);

	UFrontendSessionListEntryData* SessionB = NewObject<UFrontendSessionListEntryData>(this);
	SessionB->SessionDisplayName = TEXT("Night Raid");
	SessionB->CurrentPlayers = 1;
	SessionB->MaxPlayers = 8;
	FakeSessions.Add(SessionB);

	UFrontendSessionListEntryData* SessionC = NewObject<UFrontendSessionListEntryData>(this);
	SessionC->SessionDisplayName = TEXT("Training Grounds");
	SessionC->CurrentPlayers = 3;
	SessionC->MaxPlayers = 3;
	FakeSessions.Add(SessionC);

	OnSessionListUpdated.Broadcast(FakeSessions);
	BroadcastDebugLog(FString::Printf(TEXT("[stub] FindSessions returned %d simulated session(s)"), FakeSessions.Num()));
}

void UFrontendMultiplayerSubsystem::JoinSession(int32 SessionIndex)
{
	BroadcastDebugLog(FString::Printf(TEXT("[stub] JoinSession(%d) called"), SessionIndex));
	BroadcastDebugLog(TEXT("[stub] Session joined (simulated)"));
}

void UFrontendMultiplayerSubsystem::FetchLeaderboard()
{
	BroadcastDebugLog(TEXT("[stub] FetchLeaderboard() called"));

	// Rows arrive already ranked. Ordering is the backend's responsibility, so the
	// screen stays a pure renderer and never re-sorts what it is handed.
	struct FSimulatedLeaderboardRow
	{
		const TCHAR* PlayerDisplayName;
		int32 Score;
		bool bIsLocalPlayer;
	};

	static const FSimulatedLeaderboardRow SimulatedRows[] =
	{
		{ TEXT("Valkyrie"),    18420, false },
		{ TEXT("Ronin"),       16075, false },
		{ TEXT("Cinder"),      14930, false },
		{ TEXT("You"),         13115, true  },
		{ TEXT("Longshot"),    11860, false },
		{ TEXT("Nomad"),        9540, false }
	};

	const int32 SimulatedRowCount = static_cast<int32>(UE_ARRAY_COUNT(SimulatedRows));

	TArray<UFrontendLeaderboardEntryData*> FakeEntries;
	FakeEntries.Reserve(SimulatedRowCount);

	for (int32 RowIndex = 0; RowIndex < SimulatedRowCount; ++RowIndex)
	{
		const FSimulatedLeaderboardRow& SimulatedRow = SimulatedRows[RowIndex];

		UFrontendLeaderboardEntryData* Entry = NewObject<UFrontendLeaderboardEntryData>(this);
		Entry->Rank = RowIndex + 1;
		Entry->PlayerDisplayName = SimulatedRow.PlayerDisplayName;
		Entry->Score = SimulatedRow.Score;
		Entry->bIsLocalPlayer = SimulatedRow.bIsLocalPlayer;
		FakeEntries.Add(Entry);
	}

	OnLeaderboardUpdated.Broadcast(FakeEntries);
	BroadcastDebugLog(FString::Printf(TEXT("[stub] FetchLeaderboard returned %d simulated entr(ies)"), FakeEntries.Num()));
}
