// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FrontendSessionTypes.h"
#include "Widget_SignInPanel.generated.h"

class UFrontendCommonButtonBase;
class UCommonTextBlock;

/**
 * "Sign in" button + "Signed in as ..." label for the Multiplayer hub screen.
 *
 * Drop it into any screen; it wires itself to the multiplayer subsystem. On backends
 * with no account (LAN / stub) it collapses, so the same hub works for every service.
 * Sign-in is never required to play: Host and Find sign in lazily. The panel exists so
 * the browser round-trip happens at a moment the player chose, and so a name is on
 * screen before they host.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDMULTIPLAYER_API UWidget_SignInPanel : public UUserWidget
{
	GENERATED_BODY()

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget Interface

private:
	void OnSignInButtonClicked();
	void Refresh();

	UFUNCTION()
	void HandleLoginStateChanged(bool bIsLoggedIn);

	UFUNCTION()
	void HandleSessionOperationChanged(EFrontendSessionOperation Operation, bool bInProgress);

	//****** Bound Widgets ****** //
	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_SignIn;

	/** Optional: shows "Signed in as <name>" or "Not signed in". */
	UPROPERTY(meta = (BindWidgetOptional))
	UCommonTextBlock* CommonTextBlock_Status;
	//****** Bound Widgets ****** //
};
