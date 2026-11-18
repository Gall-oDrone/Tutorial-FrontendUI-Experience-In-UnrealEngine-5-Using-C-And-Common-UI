// Diego Gallo All Rights Reserved


#include "Subsystems/GameUIFlowSubsystem.h"
#include "GameUILog.h"
#include "Settings/GameUIDeveloperSettings.h"
#include "Subsystems/FrontendUISubsystem.h"
#include "Subsystems/FrontendLoadingScreenSubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "FrontendMultiplayerSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "GameUIFlow"

namespace
{
	const FName ReconnectLoadingScreenHoldTag(TEXT("GameUI.Reconnect"));

	/** A kick is announced right before the connection closes; an announcement older than this belongs to no failure. */
	constexpr double KickReasonLifetimeSeconds = 15.0;
}

#if !UE_BUILD_SHIPPING
// Test hook for the reconnect flow: the client reports a lost connection exactly as the engine
// would (the engine's own handler and this subsystem both react), without needing a real outage.
// The server only sees this client disconnect.
static FAutoConsoleCommandWithWorld GGameUISimulateConnectionLostCommand(
	TEXT("GameUI.SimulateConnectionLost"),
	TEXT("On a client in a match: report a lost connection the way the engine does, to exercise the return to the frontend and the reconnect offer."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (!GEngine || !World || World->GetNetMode() != NM_Client || !World->GetNetDriver())
		{
			UE_LOG(LogGameUI, Warning, TEXT("GameUI.SimulateConnectionLost: only on a client that is connected to a match."));
			return;
		}

		GEngine->BroadcastNetworkFailure(World, World->GetNetDriver(), ENetworkFailure::ConnectionLost,
			TEXT("Simulated connection loss (GameUI.SimulateConnectionLost)."));
	}));
#endif

UGameUIFlowSubsystem* UGameUIFlowSubsystem::Get(const UObject* WorldContextObject)
{
	if (GEngine)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
		return UGameInstance::GetSubsystem<UGameUIFlowSubsystem>(World->GetGameInstance());
	}

	return nullptr;
}

void UGameUIFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);

	// The reconnect covers its request with the loading screen until the join is answered.
	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = Collection.InitializeDependency<UFrontendMultiplayerSubsystem>())
	{
		MultiplayerSubsystem->OnSessionOperationChanged.AddDynamic(this, &ThisClass::HandleSessionOperationChanged);
	}
}

void UGameUIFlowSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnTravelFailure().RemoveAll(this);
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UGameInstance::GetSubsystem<UFrontendMultiplayerSubsystem>(GetGameInstance()))
	{
		MultiplayerSubsystem->OnSessionOperationChanged.RemoveAll(this);
	}
	ClearReconnectLoadingScreenHold();

	Super::Deinitialize();
}

void UGameUIFlowSubsystem::ReturnToFrontend()
{
	if (bReturnToFrontendInFlight)
	{
		UE_LOG(LogGameUI, Verbose, TEXT("ReturnToFrontend ignored; a trip to the frontend is already in flight."));
		return;
	}

	const UGameUIDeveloperSettings* Settings = UGameUIDeveloperSettings::Get();

	if (Settings->FrontendMap.IsNull())
	{
		UE_LOG(LogGameUI, Error, TEXT("Game UI Settings -> Maps -> Frontend Map is not set; cannot return to the frontend."));
		return;
	}

	APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (!PC)
	{
		UE_LOG(LogGameUI, Error, TEXT("no local player controller to travel with."));
		return;
	}

	// A paused world would carry the pause flag into the loading screen.
	UGameplayStatics::SetGamePaused(PC, false);

	// Tear down the online session (host: destroy, client: leave) so the service lets
	// this player host or join again from the frontend.
	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UGameInstance::GetSubsystem<UFrontendMultiplayerSubsystem>(GetGameInstance()))
	{
		MultiplayerSubsystem->LeaveSession();
	}

	const FString FrontendMapPackage = Settings->FrontendMap.ToSoftObjectPath().GetLongPackageName();
	bReturnToFrontendInFlight = true;
	PC->ClientTravel(FrontendMapPackage, ETravelType::TRAVEL_Absolute);
}

void UGameUIFlowSubsystem::ShowPendingDisconnectMessage()
{
	if (!HasPendingDisconnectMessage())
	{
		return;
	}

	const FText Title = PendingDisconnectTitle;
	const FText Message = PendingDisconnectMessage;
	const bool bOfferReconnect = bPendingReconnectOffer;
	PendingDisconnectTitle = FText::GetEmpty();
	PendingDisconnectMessage = FText::GetEmpty();
	bPendingReconnectOffer = false;

	UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this);
	if (!FrontendUISubsystem)
	{
		return;
	}

	if (bOfferReconnect)
	{
		TWeakObjectPtr<UGameUIFlowSubsystem> WeakThis(this);

		FrontendUISubsystem->PushConfirmScreenToModalStackAsync(
			EConfirmScreenType::YesNo,
			Title,
			FText::Format(LOCTEXT("ReconnectPromptFmt", "{0}\n\nReconnect to the match?"), Message),
			[WeakThis](EConfirmScreenButtonType ClickedButton)
			{
				if (ClickedButton == EConfirmScreenButtonType::Confirmed && WeakThis.IsValid())
				{
					WeakThis->RequestReconnect();
				}
			}
		);
		return;
	}

	FrontendUISubsystem->PushConfirmScreenToModalStackAsync(
		EConfirmScreenType::Ok,
		Title,
		Message,
		[](EConfirmScreenButtonType) {}
	);
}

void UGameUIFlowSubsystem::NotifyKickedFromMatch(const FText& Reason)
{
	PendingKickReason = Reason.IsEmpty() ? LOCTEXT("KickedNoReason", "The server removed you from the match.") : Reason;
	PendingKickReasonSeconds = FPlatformTime::Seconds();

	UE_LOG(LogGameUI, Warning, TEXT("removed from the match by the server: %s"), *PendingKickReason.ToString());
}

void UGameUIFlowSubsystem::RequestReconnect()
{
	UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UGameInstance::GetSubsystem<UFrontendMultiplayerSubsystem>(GetGameInstance());
	if (!MultiplayerSubsystem)
	{
		return;
	}

	UE_LOG(LogGameUI, Log, TEXT("reconnect requested by the player."));
	MultiplayerSubsystem->RejoinLastSession();

	// Getting a new slot takes a moment and the menu behind the dialog has no progress display
	// for it, so the loading screen covers the wait. Only while the request really is in flight:
	// an answer that already arrived (a refusal) has put up its own dialog and nothing follows.
	if (MultiplayerSubsystem->GetCurrentOperation() == EFrontendSessionOperation::Joining)
	{
		if (UFrontendLoadingScreenSubsystem* LoadingScreenSubsystem = UGameInstance::GetSubsystem<UFrontendLoadingScreenSubsystem>(GetGameInstance()))
		{
			LoadingScreenSubsystem->RegisterLoadingScreenHold(ReconnectLoadingScreenHoldTag, LOCTEXT("ReconnectingReason", "Reconnecting").ToString());
			bReconnectLoadingScreenHoldActive = true;
		}
	}
}

void UGameUIFlowSubsystem::ClearReconnectLoadingScreenHold()
{
	if (!bReconnectLoadingScreenHoldActive)
	{
		return;
	}
	bReconnectLoadingScreenHoldActive = false;

	if (UFrontendLoadingScreenSubsystem* LoadingScreenSubsystem = UGameInstance::GetSubsystem<UFrontendLoadingScreenSubsystem>(GetGameInstance()))
	{
		LoadingScreenSubsystem->ClearLoadingScreenHold(ReconnectLoadingScreenHoldTag);
	}
}

void UGameUIFlowSubsystem::HandleSessionOperationChanged(EFrontendSessionOperation Operation, bool bInProgress)
{
	// The rejoin was answered: on success the travel's own loading screen takes over, on
	// failure the subsystem's dialog must be visible.
	if (Operation == EFrontendSessionOperation::Joining && !bInProgress)
	{
		ClearReconnectLoadingScreenHold();
	}
}

void UGameUIFlowSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (!World || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	if (bReturnToFrontendInFlight)
	{
		// The engine reports one disconnect as FailureReceived and then ConnectionLost;
		// the first report carries the specific reason and has already started the trip.
		UE_LOG(LogGameUI, Verbose, TEXT("network failure %s - %s ignored; already returning to the frontend."), ENetworkFailure::ToString(FailureType), *ErrorString);
		return;
	}

	// A kick is announced by the server just before it closes the connection; the failure that
	// follows is that kick, and its reason is the one worth showing.
	const bool bWasKicked = !PendingKickReason.IsEmpty() && (FPlatformTime::Seconds() - PendingKickReasonSeconds) <= KickReasonLifetimeSeconds;
	const FText KickReason = bWasKicked ? PendingKickReason : FText::GetEmpty();
	PendingKickReason = FText::GetEmpty();

	if (bWasKicked)
	{
		UE_LOG(LogGameUI, Warning, TEXT("network failure %s - %s, after the server removed this player. Returning to the frontend."), ENetworkFailure::ToString(FailureType), *ErrorString);
		ReturnToFrontendWithMessage(World, LOCTEXT("KickedTitle", "Removed From Match"), KickReason, /*bOfferReconnect=*/false);
		return;
	}

	// Reconnect is for a connection that was up and then dropped: the game net driver of a
	// client (a join that never got in fails on the pending driver), lost or timed out (a
	// FailureReceived is the server saying why it is ending things), in a session the backend
	// can join again.
	const bool bWasConnectedToMatch = NetDriver && NetDriver->NetDriverName == NAME_GameNetDriver && NetDriver->ServerConnection != nullptr;
	const bool bConnectionDropped = FailureType == ENetworkFailure::ConnectionLost || FailureType == ENetworkFailure::ConnectionTimeout;
	const UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UGameInstance::GetSubsystem<UFrontendMultiplayerSubsystem>(GetGameInstance());
	const bool bOfferReconnect = bWasConnectedToMatch && bConnectionDropped && MultiplayerSubsystem && MultiplayerSubsystem->CanRejoinLastSession();

	UE_LOG(LogGameUI, Warning, TEXT("network failure %s - %s. Returning to the frontend%s."), ENetworkFailure::ToString(FailureType), *ErrorString,
		bOfferReconnect ? TEXT(" with a reconnect offer") : TEXT(""));

	const FText Message = ErrorString.IsEmpty()
		? FText::FromString(ENetworkFailure::ToString(FailureType))
		: FText::FromString(ErrorString);

	ReturnToFrontendWithMessage(World, LOCTEXT("DisconnectedTitle", "Disconnected"), Message, bOfferReconnect);
}

void UGameUIFlowSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (!World || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	if (bReturnToFrontendInFlight)
	{
		UE_LOG(LogGameUI, Verbose, TEXT("travel failure %s - %s ignored; already returning to the frontend."), ETravelFailure::ToString(FailureType), *ErrorString);
		return;
	}

	UE_LOG(LogGameUI, Warning, TEXT("travel failure %s - %s. Returning to the frontend."), ETravelFailure::ToString(FailureType), *ErrorString);

	const FText Message = ErrorString.IsEmpty()
		? FText::FromString(ETravelFailure::ToString(FailureType))
		: FText::FromString(ErrorString);

	ReturnToFrontendWithMessage(World, LOCTEXT("TravelFailedTitle", "Connection Failed"), Message, /*bOfferReconnect=*/false);
}

void UGameUIFlowSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	// Whatever map arrived (ours, or the engine's own fallback to the default map after a
	// failure it handled itself), the trip is over and new failures count again.
	if (LoadedWorld && LoadedWorld->GetGameInstance() == GetGameInstance())
	{
		bReturnToFrontendInFlight = false;

		// A map has loaded, so whatever the reconnect was waiting for is over.
		ClearReconnectLoadingScreenHold();
	}
}

void UGameUIFlowSubsystem::ReturnToFrontendWithMessage(UWorld* World, const FText& InTitle, const FText& InMessage, bool bOfferReconnect)
{
	PendingDisconnectTitle = InTitle;
	PendingDisconnectMessage = InMessage;
	bPendingReconnectOffer = bOfferReconnect;

	ReturnToFrontend();
}

#undef LOCTEXT_NAMESPACE
