// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widget_ServerBrowserScreen.generated.h"

class UCommonButtonBase;
class UCommonListView;
class UFrontendSessionListEntryData;

/**
 * Server browser: lists found sessions and refreshes via the multiplayer subsystem.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDMULTIPLAYER_API UWidget_ServerBrowserScreen : public UWidget_ActivatableBase
{
	GENERATED_BODY()

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

private:
	void OnRefreshButtonClicked();
	void OnBackButtonClicked();

	UFUNCTION()
	void HandleSessionListUpdated(const TArray<UFrontendSessionListEntryData*>& Sessions);

	//****** Bound Widgets ****** //
	UPROPERTY(meta = (BindWidget))
	UCommonListView* CommonListView_Sessions;

	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* Button_Refresh;

	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* Button_Back;
	//****** Bound Widgets ****** //
};
