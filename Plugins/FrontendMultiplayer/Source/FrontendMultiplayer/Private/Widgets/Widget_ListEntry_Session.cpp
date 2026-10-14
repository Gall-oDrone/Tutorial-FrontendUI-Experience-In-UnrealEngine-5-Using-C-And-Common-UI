// Diego Gallo All Rights Reserved


#include "Widgets/Widget_ListEntry_Session.h"
#include "CommonTextBlock.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "Components/ListView.h"
#include "FrontendMultiplayerSubsystem.h"
#include "FrontendSessionListEntryData.h"

void UWidget_ListEntry_Session::NativeOnListEntryWidgetHovered(bool bWasHovered)
{
	BP_OnToggleEntryWidgetHighlightState(bWasHovered || (GetListItem() && IsListItemSelected()));
}

void UWidget_ListEntry_Session::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CommonButton_Join->OnClicked().AddUObject(this, &ThisClass::OnJoinButtonClicked);

	// Selectable so the row's selection can show on Join through the button style's
	// Selected text style; otherwise a selected button stops accepting clicks.
	CommonButton_Join->SetIsSelectable(true);
	CommonButton_Join->SetIsInteractableWhenSelected(true);
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

void UWidget_ListEntry_Session::NativeOnItemSelectionChanged(bool bIsSelected)
{
	IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);

	// SetIsSelected(false) is a no-op on a non-toggleable button; ClearSelection is not.
	if (bIsSelected)
	{
		CommonButton_Join->SetIsSelected(true, false);
	}
	else
	{
		CommonButton_Join->ClearSelection();
	}

	BP_OnToggleEntryWidgetHighlightState(bIsSelected);
}

void UWidget_ListEntry_Session::NativeOnEntryReleased()
{
	IUserObjectListEntry::NativeOnEntryReleased();

	CommonButton_Join->ClearSelection();

	NativeOnListEntryWidgetHovered(false);
}

void UWidget_ListEntry_Session::OnJoinButtonClicked()
{
	UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this);
	if (!MultiplayerSubsystem || !CachedSessionData)
	{
		return;
	}

	int32 SessionIndex = INDEX_NONE;
	if (const UListView* OwningListView = Cast<UListView>(GetOwningListView()))
	{
		SessionIndex = OwningListView->GetIndexForItem(CachedSessionData);
	}

	MultiplayerSubsystem->JoinSession(SessionIndex);
}
