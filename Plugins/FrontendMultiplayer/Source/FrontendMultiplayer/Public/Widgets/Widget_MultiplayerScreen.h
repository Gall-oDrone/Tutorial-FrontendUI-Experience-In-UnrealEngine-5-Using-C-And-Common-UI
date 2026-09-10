// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widget_MultiplayerScreen.generated.h"

class UCommonButtonBase;

/**
 * Hub screen for the multiplayer menu flow (host / join / back).
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDMULTIPLAYER_API UWidget_MultiplayerScreen : public UWidget_ActivatableBase
{
	GENERATED_BODY()

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

private:
	void OnHostButtonClicked();
	void OnJoinButtonClicked();
	void OnBackButtonClicked();

	//****** Bound Widgets ****** //
	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* Button_Host;

	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* Button_Join;

	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* Button_Back;
	//****** Bound Widgets ****** //
};
