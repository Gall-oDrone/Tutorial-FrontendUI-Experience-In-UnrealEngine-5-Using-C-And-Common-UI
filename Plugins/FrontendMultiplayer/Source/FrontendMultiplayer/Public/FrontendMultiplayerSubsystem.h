// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FrontendMultiplayerSubsystem.generated.h"

class UFrontendSessionListEntryData;

/**
 * Session host/find/join entry point for the multiplayer menu flow.
 * Create/Find/Join are stubbed with simulated logs and fake session data
 * so the UI can be exercised before real networking / GameLift lands.
 */
UCLASS()
class FRONTENDMULTIPLAYER_API UFrontendMultiplayerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UFrontendMultiplayerSubsystem* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer")
	void CreateSession(FName SessionName, int32 MaxPlayers);

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer")
	void FindSessions();

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer")
	void JoinSession(int32 SessionIndex);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAPIDebugLogUpdated, const FString&, LogLine);
	UPROPERTY(BlueprintAssignable)
	FOnAPIDebugLogUpdated OnAPIDebugLogUpdated;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionListUpdated, const TArray<UFrontendSessionListEntryData*>&, Sessions);
	UPROPERTY(BlueprintAssignable)
	FOnSessionListUpdated OnSessionListUpdated;

private:
	void BroadcastDebugLog(const FString& LogLine);
};
