// Diego Gallo All Rights Reserved


#include "Backends/FrontendSessionBackend_FixedAddress.h"
#include "FrontendMultiplayerSubsystem.h"
#include "FrontendMultiplayerSettings.h"
#include "FrontendSessionListEntryData.h"
#include "FrontendSimulatedData.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#define LOCTEXT_NAMESPACE "FrontendMultiplayer"

//~ Begin UFrontendFixedAddressSettings

UFrontendFixedAddressSettings::UFrontendFixedAddressSettings()
{
	CategoryName = TEXT("Game");
}

FString UFrontendFixedAddressSettings::ResolveServerAddress() const
{
	FString Override;
	if (FParse::Value(FCommandLine::Get(), TEXT("ServerAddress="), Override) && !Override.IsEmpty())
	{
		return Override;
	}

	return ServerAddress;
}

//~ End UFrontendFixedAddressSettings

//~ Begin UFrontendSessionBackend Interface

void UFrontendSessionBackend_FixedAddress::CreateSession(FName SessionName, int32 MaxPlayers)
{
	ReportOperationStarted(EFrontendSessionOperation::Creating);

	const FString Address = UFrontendFixedAddressSettings::Get()->ResolveServerAddress();

	LogDebug(FString::Printf(TEXT("[fixed] CreateSession '%s' (%d players) -> joining the dedicated server at %s instead"),
		*SessionName.ToString(), MaxPlayers, *Address));

	if (Address.IsEmpty())
	{
		ReportSessionCreated(false, LOCTEXT("FixedNoAddress",
			"No dedicated server address is configured. Set it in Project Settings > Game > Frontend Multiplayer Fixed Address or pass -ServerAddress=host:port."));
		return;
	}

	ReportSessionCreatedRemote(Address);
}

void UFrontendSessionBackend_FixedAddress::FindSessions()
{
	ReportOperationStarted(EFrontendSessionOperation::Finding);

	const UFrontendFixedAddressSettings* Settings = UFrontendFixedAddressSettings::Get();
	const FString Address = Settings->ResolveServerAddress();

	LogDebug(FString::Printf(TEXT("[fixed] FindSessions -> %s"), *Address));

	TArray<UFrontendSessionListEntryData*> Sessions;
	if (!Address.IsEmpty())
	{
		UFrontendSessionListEntryData* Entry = NewObject<UFrontendSessionListEntryData>(this);
		Entry->SessionDisplayName = Settings->SessionDisplayName;
		Entry->HostName = Address;
		Entry->MaxPlayers = Settings->MaxPlayers;
		// Occupancy is unknown without a query service; 0 keeps the row joinable.
		Entry->CurrentPlayers = 0;
		Entry->PingInMs = 0;

		const UFrontendMultiplayerSettings* MultiplayerSettings = UFrontendMultiplayerSettings::Get();
		Entry->MapName = MultiplayerSettings->HostMap.IsNull() ? FString() : MultiplayerSettings->HostMap.GetAssetName();

		Sessions.Add(Entry);
	}

	ReportSessionsFound(Sessions);
}

void UFrontendSessionBackend_FixedAddress::JoinSession(int32 SessionIndex)
{
	ReportOperationStarted(EFrontendSessionOperation::Joining);

	const FString Address = UFrontendFixedAddressSettings::Get()->ResolveServerAddress();

	LogDebug(FString::Printf(TEXT("[fixed] JoinSession index %d -> %s"), SessionIndex, *Address));

	if (SessionIndex != 0 || Address.IsEmpty())
	{
		ReportSessionJoined(false, FString(), LOCTEXT("FixedBadIndex", "That server is no longer in the list. Refresh and try again."));
		return;
	}

	ReportSessionJoined(true, Address);
}

void UFrontendSessionBackend_FixedAddress::LeaveSession()
{
	// Nothing is registered anywhere; the travel back to the frontend drops the connection.
	LogDebug(TEXT("[fixed] LeaveSession"));
	ReportSessionLeft();
}

void UFrontendSessionBackend_FixedAddress::FetchLeaderboard(ELeaderboardCategory Category)
{
	// A bare dedicated server has no leaderboard service. Same policy as the OSS backend on Null.
	if (UFrontendMultiplayerSettings::Get()->bSimulateLeaderboardWhenUnsupported)
	{
		LogDebug(FString::Printf(TEXT("[fixed] FetchLeaderboard(%s): no leaderboard service, returning simulated rows"), *UEnum::GetDisplayValueAsText(Category).ToString()));
		ReportLeaderboard(Category, FFrontendSimulatedData::MakeLeaderboard(this, Category));
	}
	else
	{
		LogDebug(FString::Printf(TEXT("[fixed] FetchLeaderboard(%s): no leaderboard service"), *UEnum::GetDisplayValueAsText(Category).ToString()));
		ReportLeaderboard(Category, TArray<UFrontendLeaderboardEntryData*>());
	}
}

FString UFrontendSessionBackend_FixedAddress::GetBackendDisplayName() const
{
	return FString::Printf(TEXT("FixedAddress (%s)"), *UFrontendFixedAddressSettings::Get()->ResolveServerAddress());
}

bool UFrontendSessionBackend_FixedAddress::GetActiveSessionInfo(FString& OutDisplayName, int32& OutMaxPlayers) const
{
	// Asked by the match game mode on the server and by in-match UI on clients. Both read
	// the same project settings, so the lobby title and cap agree without replication.
	if (!IsInNetworkedWorld())
	{
		return false;
	}

	const UFrontendFixedAddressSettings* Settings = UFrontendFixedAddressSettings::Get();
	OutDisplayName = Settings->SessionDisplayName;
	OutMaxPlayers = Settings->MaxPlayers;
	return true;
}

//~ End UFrontendSessionBackend Interface

bool UFrontendSessionBackend_FixedAddress::IsInNetworkedWorld() const
{
	const UFrontendMultiplayerSubsystem* Subsystem = GetOwningSubsystem();
	const UGameInstance* GameInstance = Subsystem ? Subsystem->GetGameInstance() : nullptr;
	const UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;

	return World && World->GetNetMode() != NM_Standalone;
}

#undef LOCTEXT_NAMESPACE
