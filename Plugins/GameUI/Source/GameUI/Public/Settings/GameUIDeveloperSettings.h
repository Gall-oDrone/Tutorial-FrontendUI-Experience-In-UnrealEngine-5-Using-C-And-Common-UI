// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameUIDeveloperSettings.generated.h"

class UWidget_PrimaryLayout;
class UWorld;
class UInputAction;
class UInputMappingContext;

/**
 * Project settings for this game's UI flow: which layout the in-match controller creates,
 * which map is the frontend, and how the pause menu is opened. All references are soft so
 * the module never hard-loads content.
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Game UI Settings"))
class GAMEUI_API UGameUIDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGameUIDeveloperSettings* Get() { return GetDefault<UGameUIDeveloperSettings>(); }

	/** Primary layout the in-match player controller creates. WBP_CUW_PrimaryLayout works for both frontend and match. */
	UPROPERTY(Config, EditAnywhere, Category = "Match")
	TSoftClassPtr<UWidget_PrimaryLayout> MatchPrimaryLayoutClass;

	/** Mapping context added for the local player when a match controller begins play (movement, pause...). */
	UPROPERTY(Config, EditAnywhere, Category = "Match")
	TSoftObjectPtr<UInputMappingContext> MatchMappingContext;

	UPROPERTY(Config, EditAnywhere, Category = "Match")
	int32 MatchMappingContextPriority = 0;

	/** The map the frontend lives in. "Return to Main Menu" and network-failure recovery travel here. */
	UPROPERTY(Config, EditAnywhere, Category = "Maps", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> FrontendMap;

	/** Input action that opens the pause menu. Tick "Trigger when Paused" on the asset so it still fires while paused. */
	UPROPERTY(Config, EditAnywhere, Category = "Pause Menu")
	TSoftObjectPtr<UInputAction> PauseAction;

	/** Pause the world while the pause menu is open. Only takes effect where the local player has authority (single player / listen host). */
	UPROPERTY(Config, EditAnywhere, Category = "Pause Menu")
	bool bPauseGameWhileMenuOpen = true;

	/** Hold players in a lobby (WaitingToStart) until the host starts the match. Off = spawn on arrival, as before. */
	UPROPERTY(Config, EditAnywhere, Category = "Lobby")
	bool bUseLobby = true;

	/** The host cannot start with fewer connected players than this. */
	UPROPERTY(Config, EditAnywhere, Category = "Lobby", meta = (EditCondition = "bUseLobby", ClampMin = "1", UIMin = "1", UIMax = "16"))
	int32 MinPlayersToStart = 1;

	/** Seconds between Start and the match beginning. Any player un-readying cancels it. 0 starts immediately. */
	UPROPERTY(Config, EditAnywhere, Category = "Lobby", meta = (EditCondition = "bUseLobby", ClampMin = "0", UIMin = "0", UIMax = "30"))
	float StartCountdownSeconds = 3.f;

	/** When there is no host player (dedicated server), start automatically once everyone is ready. Listen hosts still press Start. */
	UPROPERTY(Config, EditAnywhere, Category = "Lobby", meta = (EditCondition = "bUseLobby"))
	bool bAutoStartWhenAllReady = false;
};
