// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widget_ServerBrowserScreen.generated.h"

class UCommonListView;
class UFrontendSessionListEntryData;
class UWidget_SessionDetailsView;

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

	//~ Begin UCommonActivatableWidget Interface
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	//~ End UCommonActivatableWidget Interface

	/**
	 * Called after the list is repopulated so the Blueprint can toggle its empty state.
	 * Event-driven rather than a Visibility binding, which would poll every frame.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Frontend Server Browser Screen")
	void BP_OnSessionListUpdated(int32 NumSessions);

private:
	void OnRefreshBoundActionTriggered();
	void OnBackBoundActionTriggered();

	UFUNCTION()
	void HandleSessionListUpdated(const TArray<UFrontendSessionListEntryData*>& Sessions);

	void OnListViewItemHovered(UObject* InHoveredItem, bool bWasHovered);
	void OnListViewItemSelected(UObject* InSelectedItem);

	//****** Bound Widgets ****** //
	UPROPERTY(meta = (BindWidget))
	UCommonListView* CommonListView_Sessions;

	UPROPERTY(meta = (BindWidget))
	UWidget_SessionDetailsView* DetailsView_SessionInfo;
	//****** Bound Widgets ****** //

	UPROPERTY(EditDefaultsOnly, Category = "Frontend Server Browser Screen", meta = (RowType = "/Script/CommonUI.CommonInputActionDataBase"))
	FDataTableRowHandle RefreshAction;

	FUIActionBindingHandle RefreshActionHandle;
};
