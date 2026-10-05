// Diego Gallo All Rights Reserved


#include "Widgets/Widget_ServerBrowserScreen.h"
#include "CommonListView.h"
#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"
#include "FrontendMultiplayerSubsystem.h"
#include "FrontendSessionListEntryData.h"
#include "Widgets/Widget_SessionDetailsView.h"

void UWidget_ServerBrowserScreen::NativeOnInitialized()
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

	CommonListView_Sessions->OnItemIsHoveredChanged().AddUObject(this, &ThisClass::OnListViewItemHovered);
	CommonListView_Sessions->OnItemSelectionChanged().AddUObject(this, &ThisClass::OnListViewItemSelected);

	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->OnSessionListUpdated.AddDynamic(this, &ThisClass::HandleSessionListUpdated);
		MultiplayerSubsystem->FindSessions();
	}
}

UWidget* UWidget_ServerBrowserScreen::NativeGetDesiredFocusTarget() const
{
	if (UObject* SelectedObject = CommonListView_Sessions->GetSelectedItem())
	{
		if (UUserWidget* SelectedEntryWidget = CommonListView_Sessions->GetEntryWidgetFromItem(SelectedObject))
		{
			return SelectedEntryWidget;
		}
	}

	return CommonListView_Sessions;
}

void UWidget_ServerBrowserScreen::OnRefreshBoundActionTriggered()
{
	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->FindSessions();
	}
}

void UWidget_ServerBrowserScreen::OnBackBoundActionTriggered()
{
	DeactivateWidget();
}

void UWidget_ServerBrowserScreen::HandleSessionListUpdated(const TArray<UFrontendSessionListEntryData*>& Sessions)
{
	TArray<UObject*> ListItems;
	ListItems.Reserve(Sessions.Num());

	for (UFrontendSessionListEntryData* SessionEntry : Sessions)
	{
		ListItems.Add(SessionEntry);
	}

	DetailsView_SessionInfo->ClearDetailsViewInfo();

	CommonListView_Sessions->SetListItems(ListItems);

	if (ListItems.Num() != 0)
	{
		CommonListView_Sessions->NavigateToIndex(0);
		CommonListView_Sessions->SetSelectedIndex(0);
	}

	BP_OnSessionListUpdated(ListItems.Num());
}

void UWidget_ServerBrowserScreen::OnListViewItemHovered(UObject* InHoveredItem, bool bWasHovered)
{
	if (!InHoveredItem)
	{
		return;
	}

	if (bWasHovered)
	{
		DetailsView_SessionInfo->UpdateDetailsViewInfo(Cast<UFrontendSessionListEntryData>(InHoveredItem));
	}
	else if (UFrontendSessionListEntryData* SelectedItem = CommonListView_Sessions->GetSelectedItem<UFrontendSessionListEntryData>())
	{
		DetailsView_SessionInfo->UpdateDetailsViewInfo(SelectedItem);
	}
}

void UWidget_ServerBrowserScreen::OnListViewItemSelected(UObject* InSelectedItem)
{
	if (!InSelectedItem)
	{
		return;
	}

	DetailsView_SessionInfo->UpdateDetailsViewInfo(Cast<UFrontendSessionListEntryData>(InSelectedItem));
}
