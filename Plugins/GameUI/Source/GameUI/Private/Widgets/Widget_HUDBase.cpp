// Diego Gallo All Rights Reserved


#include "Widgets/Widget_HUDBase.h"

UWidget_HUDBase::UWidget_HUDBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// A HUD is drawn over the game, not interacted with: it must never steal focus from
	// gameplay or from a menu pushed above it.
	SetIsFocusable(false);
	bSupportsActivationFocus = false;
	bIsBackHandler = false;
}

TOptional<FUIInputConfig> UWidget_HUDBase::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanentlyIncludingInitialMouseDown);
}
