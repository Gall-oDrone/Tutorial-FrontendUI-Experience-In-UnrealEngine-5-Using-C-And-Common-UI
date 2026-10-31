// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widget_HUDBase.generated.h"

/**
 * Base for the in-match HUD. Lives on Frontend.WidgetStack.GameHud, never takes focus,
 * and keeps the game in Game input mode so gameplay input is unaffected by it being up.
 * Subclass in Blueprint and add the actual widgets; react to the pause menu through
 * BP_OnPauseMenuVisibilityChanged.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class GAMEUI_API UWidget_HUDBase : public UWidget_ActivatableBase
{
	GENERATED_BODY()

public:
	UWidget_HUDBase(const FObjectInitializer& ObjectInitializer);

	/** Called by the match player controller when the pause menu opens or closes (hide crosshairs, dim, etc.). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|HUD")
	void BP_OnPauseMenuVisibilityChanged(bool bIsPauseMenuOpen);

	/** Session name and occupancy, sent on arrival and whenever a player joins or leaves. MaxPlayers is 0 if unknown. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|HUD")
	void BP_OnMatchInfoChanged(const FText& SessionName, int32 CurrentPlayers, int32 MaxPlayers);

protected:
	//~ Begin UCommonActivatableWidget Interface
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	//~ End UCommonActivatableWidget Interface
};
