// Diego Gallo All Rights Reserved


#include "Widgets/Widget_HUDBase.h"

UWidget_HUDBase::UWidget_HUDBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Not focusable: a HUD is drawn over the game, never interacted with. It must still
	// *support* activation focus, because CommonUI only treats such widgets as an active
	// root - and only an active root's input config (Game mode, here) is applied. With this
	// false, closing a menu above the HUD left CommonUI with no root and stuck in Menu mode.
	SetIsFocusable(false);
	bIsBackHandler = false;
}

TOptional<FUIInputConfig> UWidget_HUDBase::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
}
