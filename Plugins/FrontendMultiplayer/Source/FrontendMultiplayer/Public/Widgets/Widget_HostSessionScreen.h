// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "FrontendSessionTypes.h"
#include "Widget_HostSessionScreen.generated.h"

class UFrontendCommonButtonBase;
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

	//~ Begin UCommonActivatableWidget Interface
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	//~ End UCommonActivatableWidget Interface

private:
	void OnCreateButtonClicked();
	void OnBackBoundActionTriggered();

	/** Create is disabled for the duration of any session request so a double click cannot host twice. */
	UFUNCTION()
	void HandleSessionOperationChanged(EFrontendSessionOperation Operation, bool bInProgress);

	//****** Bound Widgets ****** //
	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* EditableTextBox_SessionName;

	UPROPERTY(meta = (BindWidget))
	USpinBox* SpinBox_MaxPlayers;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_Create;
	//****** Bound Widgets ****** //
};
