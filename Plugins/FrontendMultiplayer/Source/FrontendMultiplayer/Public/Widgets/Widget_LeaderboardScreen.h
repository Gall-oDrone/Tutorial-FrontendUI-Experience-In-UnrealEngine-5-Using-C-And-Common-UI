// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widget_LeaderboardScreen.generated.h"

class UCommonListView;
class UFrontendLeaderboardEntryData;

/**
 * Leaderboard: lists ranked players and refreshes via the multiplayer subsystem.
 * Structurally the sibling of UWidget_ServerBrowserScreen — same list view, same
 * optional Refresh bound action, same default Back registration.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDMULTIPLAYER_API UWidget_LeaderboardScreen : public UWidget_ActivatableBase
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
	UFUNCTION(BlueprintImplementableEvent, Category = "Frontend Leaderboard Screen")
	void BP_OnLeaderboardUpdated(int32 NumEntries);

private:
	void OnRefreshBoundActionTriggered();
	void OnBackBoundActionTriggered();

	UFUNCTION()
	void HandleLeaderboardUpdated(const TArray<UFrontendLeaderboardEntryData*>& Entries);

	//****** Bound Widgets ****** //
	UPROPERTY(meta = (BindWidget))
	UCommonListView* CommonListView_Leaderboard;
	//****** Bound Widgets ****** //

	UPROPERTY(EditDefaultsOnly, Category = "Frontend Leaderboard Screen", meta = (RowType = "/Script/CommonUI.CommonInputActionDataBase"))
	FDataTableRowHandle RefreshAction;

	FUIActionBindingHandle RefreshActionHandle;
};
