// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widget_HostSessionScreen.generated.h"

class UCommonButtonBase;
class UEditableTextBox;
class USpinBox;

/**
 * Host-session screen: name + max players, then create a (stubbed) session.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDMULTIPLAYER_API UWidget_HostSessionScreen : public UWidget_ActivatableBase
{
	GENERATED_BODY()

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

private:
	void OnCreateButtonClicked();
	void OnBackButtonClicked();

	//****** Bound Widgets ****** //
	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* EditableTextBox_SessionName;

	UPROPERTY(meta = (BindWidget))
	USpinBox* SpinBox_MaxPlayers;

	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* Button_Create;

	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* Button_Back;
	//****** Bound Widgets ****** //
};
