// Diego Gallo All Rights Reserved

#pragma once

#include "NativeGameplayTags.h"

/**
 * Widget tags owned by the game. They live under Frontend.Widget so they appear in the
 * FrontendWidgetMap picker, exactly as FrontendMultiplayer registers its screens.
 */
namespace GameUIGameplayTags
{
	GAMEUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frontend_Widget_HUD);
	GAMEUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frontend_Widget_PauseMenu);
	GAMEUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frontend_Widget_LobbyScreen);
}
