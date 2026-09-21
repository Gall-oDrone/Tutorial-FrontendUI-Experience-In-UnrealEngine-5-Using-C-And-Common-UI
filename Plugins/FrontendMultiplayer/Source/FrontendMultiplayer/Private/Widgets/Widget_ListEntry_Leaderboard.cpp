// Diego Gallo All Rights Reserved


#include "Widgets/Widget_ListEntry_Leaderboard.h"
#include "CommonTextBlock.h"
#include "FrontendLeaderboardEntryData.h"

void UWidget_ListEntry_Leaderboard::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UFrontendLeaderboardEntryData* EntryData = Cast<UFrontendLeaderboardEntryData>(ListItemObject);
	if (!EntryData)
	{
		CommonText_Rank->SetText(FText::GetEmpty());
		CommonText_PlayerName->SetText(FText::GetEmpty());
		CommonText_Score->SetText(FText::GetEmpty());
		BP_OnLocalPlayerStateChanged(false);
		return;
	}

	CommonText_Rank->SetText(FText::AsNumber(EntryData->Rank));
	CommonText_PlayerName->SetText(FText::FromString(EntryData->PlayerDisplayName));
	CommonText_Score->SetText(FText::AsNumber(EntryData->Score));

	BP_OnLocalPlayerStateChanged(EntryData->bIsLocalPlayer);
}
