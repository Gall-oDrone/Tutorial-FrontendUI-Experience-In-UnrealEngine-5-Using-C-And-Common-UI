// Diego Gallo All Rights Reserved


#include "Widgets/Widget_LeaderboardScreen.h"
#include "CommonListView.h"
#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"
#include "FrontendLeaderboardEntryData.h"
#include "FrontendMultiplayerSubsystem.h"

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

	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->OnLeaderboardUpdated.AddDynamic(this, &ThisClass::HandleLeaderboardUpdated);
		MultiplayerSubsystem->FetchLeaderboard();
	}
}

void UWidget_LeaderboardScreen::OnRefreshBoundActionTriggered()
{
	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->FetchLeaderboard();
	}
}

void UWidget_LeaderboardScreen::OnBackBoundActionTriggered()
{
	DeactivateWidget();
}

void UWidget_LeaderboardScreen::HandleLeaderboardUpdated(const TArray<UFrontendLeaderboardEntryData*>& Entries)
{
	TArray<UObject*> ListItems;
	ListItems.Reserve(Entries.Num());

	for (UFrontendLeaderboardEntryData* Entry : Entries)
	{
		ListItems.Add(Entry);
	}

	CommonListView_Leaderboard->SetListItems(ListItems);
}
