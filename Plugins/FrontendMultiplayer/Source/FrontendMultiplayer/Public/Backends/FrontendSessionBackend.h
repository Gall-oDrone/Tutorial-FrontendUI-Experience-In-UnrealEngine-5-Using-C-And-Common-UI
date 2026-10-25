// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FrontendLeaderboardTypes.h"
#include "FrontendSessionBackend.generated.h"

class UFrontendMultiplayerSubsystem;
class UFrontendSessionListEntryData;
class UFrontendLeaderboardEntryData;

/**
 * The seam between the multiplayer screens and whatever actually hosts, finds and
 * joins sessions (OnlineSubsystem, EOS, GameLift, a stub...).
 *
 * UFrontendMultiplayerSubsystem instantiates the class named in
 * UFrontendMultiplayerSettings::SessionBackendClass and forwards every UI request
 * here. A backend reports results back through the Report* / LogDebug helpers,
 * which the subsystem turns into the delegates the screens already listen to.
 * Screens never see the backend directly, so swapping it is a settings change.
 */
UCLASS(Abstract)
class FRONTENDMULTIPLAYER_API UFrontendSessionBackend : public UObject
{
	GENERATED_BODY()

public:
	/** Called once by the owning subsystem right after construction. */
	virtual void Initialize(UFrontendMultiplayerSubsystem* InOwningSubsystem);

	/** Called when the owning subsystem shuts down. */
	virtual void Shutdown() {}

	virtual void CreateSession(FName SessionName, int32 MaxPlayers) PURE_VIRTUAL(UFrontendSessionBackend::CreateSession, );
	virtual void FindSessions() PURE_VIRTUAL(UFrontendSessionBackend::FindSessions, );
	virtual void JoinSession(int32 SessionIndex) PURE_VIRTUAL(UFrontendSessionBackend::JoinSession, );
	virtual void FetchLeaderboard(ELeaderboardCategory Category) PURE_VIRTUAL(UFrontendSessionBackend::FetchLeaderboard, );

protected:
	UFrontendMultiplayerSubsystem* GetOwningSubsystem() const { return OwningSubsystem.Get(); }

	/** Deliver a session list to the screens (server browser). */
	void ReportSessionList(const TArray<UFrontendSessionListEntryData*>& Sessions) const;

	/** Deliver ranked leaderboard rows to the screens. Rows must arrive already ranked. */
	void ReportLeaderboard(ELeaderboardCategory Category, const TArray<UFrontendLeaderboardEntryData*>& Entries) const;

	/** Append a line to the API debug screen and the Output Log. */
	void LogDebug(const FString& LogLine) const;

private:
	TWeakObjectPtr<UFrontendMultiplayerSubsystem> OwningSubsystem;
};
