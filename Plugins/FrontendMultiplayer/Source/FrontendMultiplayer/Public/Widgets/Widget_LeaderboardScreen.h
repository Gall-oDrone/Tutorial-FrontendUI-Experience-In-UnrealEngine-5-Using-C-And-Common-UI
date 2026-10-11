// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "FrontendLeaderboardTypes.h"
#include "Widget_LeaderboardScreen.generated.h"

class UCommonListView;
class UFrontendTabListWidgetBase;
class UFrontendCommonButtonBase;
class UFrontendLeaderboardEntryData;

/**
 * Leaderboard: tabs choose a category, the list shows its ranked rows, and the
 * right-hand buttons sort, jump and page through them. Structurally the sibling of
 * UWidget_ServerBrowserScreen with Options' tab list on top.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDMULTIPLAYER_API UWidget_LeaderboardScreen : public UWidget_ActivatableBase
{
	GENERATED_BODY()

public:
	UWidget_LeaderboardScreen(const FObjectInitializer& ObjectInitializer);

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	//~ Begin UCommonActivatableWidget Interface
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	//~ End UCommonActivatableWidget Interface

	/**
	 * Called after the list is repopulated so the Blueprint can toggle its empty state.
	 * Event-driven rather than a Visibility binding, which would poll every frame.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Frontend Leaderboard Screen")
	void BP_OnLeaderboardUpdated(int32 NumEntries);

private:
	void RequestFetch();
	void RepopulateList();
	void ScrollByPages(int32 PageDelta);

	void OnRefreshBoundActionTriggered();
	void OnBackBoundActionTriggered();

	UFUNCTION()
	void OnLeaderboardTabSelected(FName TabId);

	UFUNCTION()
	void HandleLeaderboardUpdated(ELeaderboardCategory Category, const TArray<UFrontendLeaderboardEntryData*>& Entries);

	void OnJumpTopClicked();
	void OnJumpMeClicked();
	void OnJumpBottomClicked();
	void OnSortScoreClicked();
	void OnRefreshClicked();
	void OnPageUpClicked();
	void OnPageDownClicked();

	//****** Bound Widgets ****** //
	UPROPERTY(meta = (BindWidget))
	UFrontendTabListWidgetBase* TabListWidget_LeaderboardTabs;

	UPROPERTY(meta = (BindWidget))
	UCommonListView* CommonListView_Leaderboard;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_JumpTop;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_JumpMe;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_JumpBottom;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_SortScore;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_Refresh;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_PageUp;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_PageDown;
	//****** Bound Widgets ****** //

	/** One tab per entry, registered in order. Add a row here to add a leaderboard. */
	UPROPERTY(EditDefaultsOnly, Category = "Frontend Leaderboard Screen")
	TArray<FLeaderboardTabDefinition> LeaderboardTabs;

	/** Sort button labels, one per sort mode. */
	UPROPERTY(EditDefaultsOnly, Category = "Frontend Leaderboard Screen")
	TMap<ELeaderboardSortMode, FText> SortModeDisplayTexts;

	UPROPERTY(EditDefaultsOnly, Category = "Frontend Leaderboard Screen", meta = (RowType = "/Script/CommonUI.CommonInputActionDataBase"))
	FDataTableRowHandle RefreshAction;

	FUIActionBindingHandle RefreshActionHandle;

	ELeaderboardCategory CurrentCategory = ELeaderboardCategory::Main;
	ELeaderboardSortMode CurrentSortMode = ELeaderboardSortMode::ScoreDescending;

	/** Rows for the current category as the backend delivered them. */
	UPROPERTY(Transient)
	TArray<UFrontendLeaderboardEntryData*> SourceEntries;
};
