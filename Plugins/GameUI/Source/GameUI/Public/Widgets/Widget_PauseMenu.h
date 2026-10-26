// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widget_PauseMenu.generated.h"

class UFrontendCommonButtonBase;

/**
 * In-match pause menu on Frontend.WidgetStack.GameMenu.
 *
 * Resume / Back close it. Options pushes the framework's Options screen onto the same
 * stack, so it returns here on Back. Return to Main Menu and Quit confirm first.
 * Pausing the world is the match player controller's job; this widget only reports
 * that it wants to close through OnCloseRequested.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class GAMEUI_API UWidget_PauseMenu : public UWidget_ActivatableBase
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE(FOnPauseMenuCloseRequested);
	FOnPauseMenuCloseRequested OnCloseRequested;

	/** Close via Resume, Back, or code. Notifies the owner first, then deactivates. */
	UFUNCTION(BlueprintCallable, Category = "Game UI|Pause Menu")
	void RequestClose();

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	//~ Begin UCommonActivatableWidget Interface
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	//~ End UCommonActivatableWidget Interface

private:
	void OnResumeClicked();
	void OnOptionsClicked();
	void OnReturnToMainMenuClicked();
	void OnQuitClicked();

	//****** Bound Widgets ****** //
	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_Resume;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_Options;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_ReturnToMainMenu;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_Quit;
	//****** Bound Widgets ****** //
};
