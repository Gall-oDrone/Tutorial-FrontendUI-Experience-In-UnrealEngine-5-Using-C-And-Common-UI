// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "FrontendSessionTypes.h"
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

	/** True while a search is running; false once results (or a failure) arrive. Drive a spinner or "Searching..." label from this. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Frontend Server Browser Screen")
	void BP_OnSearchStateChanged(bool bIsSearching);

private:
	void OnRefreshBoundActionTriggered();
	void OnBackBoundActionTriggered();

	UFUNCTION()
	void HandleSessionListUpdated(const TArray<UFrontendSessionListEntryData*>& Sessions);

	UFUNCTION()
	void HandleSessionOperationChanged(EFrontendSessionOperation Operation, bool bInProgress);

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
