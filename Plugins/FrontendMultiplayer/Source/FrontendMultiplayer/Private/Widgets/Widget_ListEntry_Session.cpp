// Diego Gallo All Rights Reserved


#include "Widgets/Widget_ListEntry_Session.h"
#include "Blueprint/UserListEntryLibrary.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/ListView.h"
#include "FrontendMultiplayerSubsystem.h"
#include "FrontendSessionListEntryData.h"

void UWidget_ListEntry_Session::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_Join->OnClicked().AddUObject(this, &ThisClass::OnJoinButtonClicked);
}

void UWidget_ListEntry_Session::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	CachedSessionData = Cast<UFrontendSessionListEntryData>(ListItemObject);
	if (!CachedSessionData)
	{
		CommonText_SessionInfo->SetText(FText::GetEmpty());
		return;
	}

	const FString SessionInfo = FString::Printf(
		TEXT("%s  (%d/%d)"),
		*CachedSessionData->SessionDisplayName,
		CachedSessionData->CurrentPlayers,
		CachedSessionData->MaxPlayers);

	CommonText_SessionInfo->SetText(FText::FromString(SessionInfo));
}

void UWidget_ListEntry_Session::OnJoinButtonClicked()
{
	UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this);
	if (!MultiplayerSubsystem || !CachedSessionData)
	{
		return;
	}

	int32 SessionIndex = INDEX_NONE;
	if (const UListView* OwningListView = Cast<UListView>(UUserListEntryLibrary::GetOwningListView(this)))
	{
		SessionIndex = OwningListView->GetIndexForItem(CachedSessionData);
	}

	MultiplayerSubsystem->JoinSession(SessionIndex);
}
