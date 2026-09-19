// Diego Gallo All Rights Reserved


#include "Widgets/Widget_ServerBrowserScreen.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "CommonListView.h"
#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"
#include "FrontendMultiplayerSubsystem.h"
#include "FrontendSessionListEntryData.h"

void UWidget_ServerBrowserScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RegisterUIActionBinding(
		FBindUIActionArgs(
			ICommonInputModule::GetSettings().GetDefaultBackAction(),
			true,
			FSimpleDelegate::CreateUObject(this, &ThisClass::OnBackBoundActionTriggered)
		)
	);

	CommonButton_Refresh->OnClicked().AddUObject(this, &ThisClass::OnRefreshButtonClicked);

	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->OnSessionListUpdated.AddDynamic(this, &ThisClass::HandleSessionListUpdated);
		MultiplayerSubsystem->FindSessions();
	}
}

void UWidget_ServerBrowserScreen::OnRefreshButtonClicked()
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

	CommonListView_Sessions->SetListItems(ListItems);
}
