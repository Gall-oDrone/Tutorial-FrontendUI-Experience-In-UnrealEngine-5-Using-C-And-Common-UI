// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widget_ServerBrowserScreen.generated.h"

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
	void OnRefreshBoundActionTriggered();
	void OnBackBoundActionTriggered();

	UFUNCTION()
	void HandleSessionListUpdated(const TArray<UFrontendSessionListEntryData*>& Sessions);

	//****** Bound Widgets ****** //
	UPROPERTY(meta = (BindWidget))
	UCommonListView* CommonListView_Sessions;
	//****** Bound Widgets ****** //

	UPROPERTY(EditDefaultsOnly, Category = "Frontend Server Browser Screen", meta = (RowType = "/Script/CommonUI.CommonInputActionDataBase"))
	FDataTableRowHandle RefreshAction;

	FUIActionBindingHandle RefreshActionHandle;
};
