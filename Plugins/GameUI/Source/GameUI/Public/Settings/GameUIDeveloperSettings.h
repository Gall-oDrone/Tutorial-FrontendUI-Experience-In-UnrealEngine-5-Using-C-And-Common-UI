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
};
