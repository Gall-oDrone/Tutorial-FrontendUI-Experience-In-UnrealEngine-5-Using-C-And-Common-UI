// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "FrontendSettings/FrontendGameUserSettings.h"
#include "GameUIGameUserSettings.generated.h"

/**
 * This game's user settings. FrontendUI stores volumes and difficulty; this class is
 * where they are actually applied to the game's audio and gameplay systems.
 *
 * Activate it in Config/DefaultEngine.ini:
 *   [/Script/Engine.Engine]
 *   GameUserSettingsClassName=/Script/GameUI.GameUIGameUserSettings
 *
 * Note: switching the class changes the section name under which GameUserSettings.ini
 * is saved, so previously saved values start from defaults once.
 */
UCLASS()
class GAMEUI_API UGameUIGameUserSettings : public UFrontendGameUserSettings
{
	GENERATED_BODY()

protected:
	//~ Begin UFrontendGameUserSettings Interface
	virtual void ApplyAudioSettings() override;
	//~ End UFrontendGameUserSettings Interface
};
