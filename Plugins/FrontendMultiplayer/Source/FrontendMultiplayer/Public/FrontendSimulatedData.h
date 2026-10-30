// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "FrontendLeaderboardTypes.h"

class UFrontendLeaderboardEntryData;
class UFrontendSessionListEntryData;

/**
 * Simulated session and leaderboard rows. Used by the stub backend, and by real
 * backends whose online service has no leaderboard interface (see
 * UFrontendMultiplayerSettings::bSimulateLeaderboardWhenUnsupported).
 */
struct FRONTENDMULTIPLAYER_API FFrontendSimulatedData
{
	/** Three fixed sessions with varied occupancy, one of them full. */
	static TArray<UFrontendSessionListEntryData*> MakeSessions(UObject* Outer);

	/** Forty rows, already ranked highest score first; one row is flagged as the local player. */
	static TArray<UFrontendLeaderboardEntryData*> MakeLeaderboard(UObject* Outer, ELeaderboardCategory Category);
};
