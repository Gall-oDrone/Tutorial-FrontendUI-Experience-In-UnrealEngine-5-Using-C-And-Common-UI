// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FrontendLeaderboardTypes.h"
#include "FrontendSessionTypes.h"
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
 * here. A backend reports results back through the Report* / LogDebug helpers; the
 * subsystem turns them into the delegates the screens listen to and performs the
 * map travel that follows a successful host or join. Screens never see the backend
 * directly, so swapping it is a settings change.
 *
 * Contract for implementers: every request must eventually be answered by exactly
 * one matching Report* call (success or failure), otherwise the subsystem stays
 * busy and the screens keep their buttons disabled.
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

	/**
	 * Host a session. SessionName is the user-facing name typed on the Host screen;
	 * backends are free to store it as a session setting rather than use it as the
	 * engine-level session identifier. Answer with ReportSessionCreated.
	 */
	virtual void CreateSession(FName SessionName, int32 MaxPlayers) PURE_VIRTUAL(UFrontendSessionBackend::CreateSession, );

	/** Answer with ReportSessionsFound or ReportFindSessionsFailed. */
	virtual void FindSessions() PURE_VIRTUAL(UFrontendSessionBackend::FindSessions, );

	/** SessionIndex addresses the array last passed to ReportSessionsFound. Answer with ReportSessionJoined. */
	virtual void JoinSession(int32 SessionIndex) PURE_VIRTUAL(UFrontendSessionBackend::JoinSession, );

	/**
	 * Leave or destroy the session this player is in, if any, so a new one can be
	 * hosted or joined. Answer with ReportSessionLeft once done. Default: nothing to leave.
	 */
	virtual void LeaveSession() {}

	/** Answer with ReportLeaderboard. */
	virtual void FetchLeaderboard(ELeaderboardCategory Category) PURE_VIRTUAL(UFrontendSessionBackend::FetchLeaderboard, );

	/** Shown in the API debug log when the backend is created. */
	virtual FString GetBackendDisplayName() const { return GetClass()->GetName(); }

	/**
	 * Describe the session this player currently hosts or has joined, for in-match UI
	 * (lobby title, player count). Return false if there is none.
	 */
	virtual bool GetActiveSessionInfo(FString& OutDisplayName, int32& OutMaxPlayers) const { return false; }

protected:
	UFrontendMultiplayerSubsystem* GetOwningSubsystem() const { return OwningSubsystem.Get(); }

	/** Mark the subsystem busy with Operation until the matching Report* call arrives. */
	void ReportOperationStarted(EFrontendSessionOperation Operation) const;

	/** On success the subsystem server-travels to UFrontendMultiplayerSettings::HostMap as a listen server. */
	void ReportSessionCreated(bool bSucceeded, const FText& ErrorText = FText::GetEmpty()) const;

	/** Deliver a session list to the screens (server browser). An empty array is a valid, successful result. */
	void ReportSessionsFound(const TArray<UFrontendSessionListEntryData*>& Sessions) const;

	void ReportFindSessionsFailed(const FText& ErrorText) const;

	/** On success the subsystem client-travels the first local player to ConnectString. */
	void ReportSessionJoined(bool bSucceeded, const FString& ConnectString, const FText& ErrorText = FText::GetEmpty()) const;

	void ReportSessionLeft() const;

	/** Deliver ranked leaderboard rows to the screens. Rows must arrive already ranked. */
	void ReportLeaderboard(ELeaderboardCategory Category, const TArray<UFrontendLeaderboardEntryData*>& Entries) const;

	/** Append a line to the API debug screen and the Output Log. */
	void LogDebug(const FString& LogLine) const;

private:
	TWeakObjectPtr<UFrontendMultiplayerSubsystem> OwningSubsystem;
};
