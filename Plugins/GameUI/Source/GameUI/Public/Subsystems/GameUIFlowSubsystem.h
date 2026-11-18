// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/EngineBaseTypes.h"
#include "FrontendSessionTypes.h"
#include "GameUIFlowSubsystem.generated.h"

class UNetDriver;

/**
 * Owns the transitions between match and frontend that no single screen should own:
 * returning to the main menu, and recovering from a network or travel failure by going
 * back to the frontend with a message to show once it is up.
 *
 * The frontend controller calls ShowPendingDisconnectMessage() after it has pushed its
 * first screen, so the confirm dialog lands on a live Modal stack.
 *
 * Reconnect: when the connection to a running match is lost (not a refused join, not a
 * kick) and the session backend can rejoin the session, that dialog asks "Reconnect?"
 * instead of only reporting the loss; Yes joins the same session again.
 */
UCLASS()
class GAMEUI_API UGameUIFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UGameUIFlowSubsystem* Get(const UObject* WorldContextObject);

	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem Interface

	/** Unpause and travel the local player to UGameUIDeveloperSettings::FrontendMap. */
	UFUNCTION(BlueprintCallable, Category = "Game UI|Flow")
	void ReturnToFrontend();

	UFUNCTION(BlueprintPure, Category = "Game UI|Flow")
	bool HasPendingDisconnectMessage() const { return !PendingDisconnectMessage.IsEmpty(); }

	/**
	 * If a failure sent us back to the frontend, push a confirm screen with the reason and clear
	 * it: Ok, or Yes / No when a reconnect is on offer (Yes rejoins the session). Safe to call every boot.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game UI|Flow")
	void ShowPendingDisconnectMessage();

	/** True if the pending message will offer to reconnect. */
	UFUNCTION(BlueprintPure, Category = "Game UI|Flow")
	bool IsReconnectOffered() const { return bPendingReconnectOffer; }

	/**
	 * The server removed this player and said why (APlayerController::ClientWasKicked, which
	 * arrives just before the connection closes). The lost connection that follows is then
	 * reported with this reason and without a reconnect offer.
	 */
	void NotifyKickedFromMatch(const FText& Reason);

private:
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void ReturnToFrontendWithMessage(UWorld* World, const FText& InTitle, const FText& InMessage, bool bOfferReconnect);

	/** The player answered Yes to the reconnect offer. */
	void RequestReconnect();
	void ClearReconnectLoadingScreenHold();

	UFUNCTION()
	void HandleSessionOperationChanged(EFrontendSessionOperation Operation, bool bInProgress);

	FText PendingDisconnectTitle;
	FText PendingDisconnectMessage;

	/** The pending message is about a match connection the backend can pick up again. */
	bool bPendingReconnectOffer = false;

	/** Reason of a kick that has been announced but whose connection loss has not arrived yet, and when it was announced. */
	FText PendingKickReason;
	double PendingKickReasonSeconds = 0.0;

	/** The loading screen is being held up while the rejoin request is in flight. */
	bool bReconnectLoadingScreenHoldActive = false;

	/**
	 * Set when a trip back to the frontend has been issued (by a failure or by the player)
	 * and no map has finished loading since. The engine reports one lost connection as two
	 * broadcasts (FailureReceived, then ConnectionLost) in the same frame; only the first
	 * should tear the session down and pick the message. Cleared on PostLoadMapWithWorld.
	 */
	bool bReturnToFrontendInFlight = false;
};
