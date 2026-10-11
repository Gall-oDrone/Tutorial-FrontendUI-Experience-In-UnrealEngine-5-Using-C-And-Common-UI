// Diego Gallo All Rights Reserved


#include "Widgets/Widget_LeaderboardScreen.h"
#include "CommonListView.h"
#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"
#include "Widgets/Components/FrontendTabListWidgetBase.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "FrontendLeaderboardEntryData.h"
#include "FrontendMultiplayerSubsystem.h"

UWidget_LeaderboardScreen::UWidget_LeaderboardScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FLeaderboardTabDefinition MainTab;
	MainTab.TabID = TEXT("Main");
	MainTab.DisplayName = FText::FromString(TEXT("Leaderboard"));
	MainTab.Category = ELeaderboardCategory::Main;
	LeaderboardTabs.Add(MainTab);

	SortModeDisplayTexts.Add(ELeaderboardSortMode::ScoreDescending, FText::FromString(TEXT("Score: High to Low")));
	SortModeDisplayTexts.Add(ELeaderboardSortMode::ScoreAscending, FText::FromString(TEXT("Score: Low to High")));
}

void UWidget_LeaderboardScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!RefreshAction.IsNull())
	{
		RefreshActionHandle = RegisterUIActionBinding(
			FBindUIActionArgs(
				RefreshAction,
				true,
				FSimpleDelegate::CreateUObject(this, &ThisClass::OnRefreshBoundActionTriggered)
			)
		);
	}

	RegisterUIActionBinding(
		FBindUIActionArgs(
			ICommonInputModule::GetSettings().GetDefaultBackAction(),
			true,
			FSimpleDelegate::CreateUObject(this, &ThisClass::OnBackBoundActionTriggered)
		)
	);

	TabListWidget_LeaderboardTabs->OnTabSelected.AddUniqueDynamic(this, &ThisClass::OnLeaderboardTabSelected);

	CommonButton_JumpTop->OnClicked().AddUObject(this, &ThisClass::OnJumpTopClicked);
	CommonButton_JumpMe->OnClicked().AddUObject(this, &ThisClass::OnJumpMeClicked);
	CommonButton_JumpBottom->OnClicked().AddUObject(this, &ThisClass::OnJumpBottomClicked);
	CommonButton_SortScore->OnClicked().AddUObject(this, &ThisClass::OnSortScoreClicked);
	CommonButton_Refresh->OnClicked().AddUObject(this, &ThisClass::OnRefreshClicked);
	CommonButton_PageUp->OnClicked().AddUObject(this, &ThisClass::OnPageUpClicked);
	CommonButton_PageDown->OnClicked().AddUObject(this, &ThisClass::OnPageDownClicked);

	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->OnLeaderboardUpdated.AddDynamic(this, &ThisClass::HandleLeaderboardUpdated);
	}
}

void UWidget_LeaderboardScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	// After construction, so the button's own PreConstruct cannot overwrite it with
	// the placeholder Button Display Text authored in the Blueprint.
	RefreshSortButtonText();

	// Register once; the stack may re-activate a cached screen. Selecting the first tab
	// is what triggers the initial fetch, through OnLeaderboardTabSelected.
	for (const FLeaderboardTabDefinition& TabDefinition : LeaderboardTabs)
	{
		if (TabDefinition.TabID.IsNone() || TabListWidget_LeaderboardTabs->GetTabButtonBaseByID(TabDefinition.TabID) != nullptr)
		{
			continue;
		}

		TabListWidget_LeaderboardTabs->RequestRegisterTab(TabDefinition.TabID, TabDefinition.DisplayName);
	}

	if (TabListWidget_LeaderboardTabs->GetActiveTab().IsNone() && LeaderboardTabs.Num() > 0)
	{
		TabListWidget_LeaderboardTabs->SelectTabByID(LeaderboardTabs[0].TabID);
	}
}

UWidget* UWidget_LeaderboardScreen::NativeGetDesiredFocusTarget() const
{
	if (UObject* SelectedObject = CommonListView_Leaderboard->GetSelectedItem())
	{
		if (UUserWidget* SelectedEntryWidget = CommonListView_Leaderboard->GetEntryWidgetFromItem(SelectedObject))
		{
			return SelectedEntryWidget;
		}
	}

	return CommonListView_Leaderboard;
}

void UWidget_LeaderboardScreen::RequestFetch()
{
	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->FetchLeaderboard(CurrentCategory);
	}
}

void UWidget_LeaderboardScreen::RepopulateList()
{
	TArray<UFrontendLeaderboardEntryData*> SortedEntries = SourceEntries;

	// Display order only. Rank values stay as the backend assigned them.
	SortedEntries.Sort([this](const UFrontendLeaderboardEntryData& A, const UFrontendLeaderboardEntryData& B)
	{
		return CurrentSortMode == ELeaderboardSortMode::ScoreDescending ? A.Score > B.Score : A.Score < B.Score;
	});

	TArray<UObject*> ListItems;
	ListItems.Reserve(SortedEntries.Num());
	for (UFrontendLeaderboardEntryData* Entry : SortedEntries)
	{
		ListItems.Add(Entry);
	}

	CommonListView_Leaderboard->SetListItems(ListItems);
	CommonListView_Leaderboard->ScrollToTop();

	BP_OnLeaderboardUpdated(ListItems.Num());
}

void UWidget_LeaderboardScreen::RefreshSortButtonText()
{
	const FText* SortText = SortModeDisplayTexts.Find(CurrentSortMode);
	CommonButton_SortScore->SetButtonText(SortText ? *SortText : UEnum::GetDisplayValueAsText(CurrentSortMode));
}

void UWidget_LeaderboardScreen::ScrollByPages(int32 PageDelta)
{
	// One page is the number of rows currently visible, minus one so the last row
	// of the previous page stays visible as an anchor.
	const int32 VisibleRows = CommonListView_Leaderboard->GetDisplayedEntryWidgets().Num();
	const float PageSize = static_cast<float>(FMath::Max(1, VisibleRows - 1));

	const float NewOffset = FMath::Max(0.f, CommonListView_Leaderboard->GetScrollOffset() + PageSize * PageDelta);
	CommonListView_Leaderboard->SetScrollOffset(NewOffset);
}

void UWidget_LeaderboardScreen::OnRefreshBoundActionTriggered()
{
	RequestFetch();
}

void UWidget_LeaderboardScreen::OnBackBoundActionTriggered()
{
	DeactivateWidget();
}

void UWidget_LeaderboardScreen::OnLeaderboardTabSelected(FName TabId)
{
	const FLeaderboardTabDefinition* FoundTab = LeaderboardTabs.FindByPredicate(
		[TabId](const FLeaderboardTabDefinition& TabDefinition) { return TabDefinition.TabID == TabId; });

	if (!FoundTab)
	{
		return;
	}

	CurrentCategory = FoundTab->Category;
	SourceEntries.Empty();
	RepopulateList();
	RequestFetch();
}

void UWidget_LeaderboardScreen::HandleLeaderboardUpdated(ELeaderboardCategory Category, const TArray<UFrontendLeaderboardEntryData*>& Entries)
{
	if (Category != CurrentCategory)
	{
		return;
	}

	SourceEntries = Entries;
	RepopulateList();
}

void UWidget_LeaderboardScreen::OnJumpTopClicked()
{
	CommonListView_Leaderboard->ScrollToTop();
}

void UWidget_LeaderboardScreen::OnJumpMeClicked()
{
	const TArray<UObject*>& ListItems = CommonListView_Leaderboard->GetListItems();

	for (int32 ItemIndex = 0; ItemIndex < ListItems.Num(); ++ItemIndex)
	{
		const UFrontendLeaderboardEntryData* Entry = Cast<UFrontendLeaderboardEntryData>(ListItems[ItemIndex]);
		if (Entry && Entry->bIsLocalPlayer)
		{
			CommonListView_Leaderboard->ScrollIndexIntoView(ItemIndex);
			return;
		}
	}
}

void UWidget_LeaderboardScreen::OnJumpBottomClicked()
{
	CommonListView_Leaderboard->ScrollToBottom();
}

void UWidget_LeaderboardScreen::OnSortScoreClicked()
{
	CurrentSortMode = CurrentSortMode == ELeaderboardSortMode::ScoreDescending
		? ELeaderboardSortMode::ScoreAscending
		: ELeaderboardSortMode::ScoreDescending;

	RefreshSortButtonText();
	RepopulateList();
}

void UWidget_LeaderboardScreen::OnRefreshClicked()
{
	RequestFetch();
}

void UWidget_LeaderboardScreen::OnPageUpClicked()
{
	ScrollByPages(-1);
}

void UWidget_LeaderboardScreen::OnPageDownClicked()
{
	ScrollByPages(1);
}
