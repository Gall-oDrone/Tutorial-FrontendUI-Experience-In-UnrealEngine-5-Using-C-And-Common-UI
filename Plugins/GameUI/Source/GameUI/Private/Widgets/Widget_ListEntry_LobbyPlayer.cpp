// Diego Gallo All Rights Reserved


#include "Widgets/Widget_ListEntry_LobbyPlayer.h"
#include "Widgets/GameUILobbyPlayerEntryData.h"
#include "CommonTextBlock.h"

void UWidget_ListEntry_LobbyPlayer::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UGameUILobbyPlayerEntryData* EntryData = Cast<UGameUILobbyPlayerEntryData>(ListItemObject);
	if (!EntryData)
	{
		CommonText_PlayerName->SetText(FText::GetEmpty());
		BP_OnPlayerEntryUpdated(false, false, false);
		return;
	}

	CommonText_PlayerName->SetText(FText::FromString(EntryData->PlayerName));
	BP_OnPlayerEntryUpdated(EntryData->bIsReady, EntryData->bIsHost, EntryData->bIsLocalPlayer);
}
