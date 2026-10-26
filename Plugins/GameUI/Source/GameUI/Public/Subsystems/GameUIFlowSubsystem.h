// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/EngineBaseTypes.h"
#include "GameUIFlowSubsystem.generated.h"

class UNetDriver;

/**
 * Owns the transitions between match and frontend that no single screen should own:
 * returning to the main menu, and recovering from a network or travel failure by going
 * back to the frontend with a message to show once it is up.
 *
 * The frontend controller calls ShowPendingDisconnectMessage() after it has pushed its
 * first screen, so the confirm dialog lands on a live Modal stack.
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

	/** If a failure sent us back to the frontend, push an Ok confirm screen with the reason and clear it. Safe to call every boot. */
	UFUNCTION(BlueprintCallable, Category = "Game UI|Flow")
	void ShowPendingDisconnectMessage();

private:
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	void ReturnToFrontendWithMessage(UWorld* World, const FText& InTitle, const FText& InMessage);

	FText PendingDisconnectTitle;
	FText PendingDisconnectMessage;
};
