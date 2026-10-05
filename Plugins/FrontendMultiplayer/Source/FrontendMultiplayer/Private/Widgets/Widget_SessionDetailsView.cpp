// Diego Gallo All Rights Reserved


#include "Widgets/Widget_SessionDetailsView.h"
#include "CommonTextBlock.h"
#include "CommonRichTextBlock.h"
#include "FrontendSessionListEntryData.h"

void UWidget_SessionDetailsView::UpdateDetailsViewInfo(UFrontendSessionListEntryData* InSessionData)
{
	if (!InSessionData)
	{
		return;
	}

	CommonTextBlock_Title->SetText(FText::FromString(InSessionData->SessionDisplayName));

	// <Bold> must exist as a row in the rich text widgets' Text Style Set.
	const FString Description = FString::Printf(
		TEXT("Host: <Bold>%s</>\n\nMap: <Bold>%s</>\n\nPlayers: <Bold>%d / %d</>\n\nPing: <Bold>%d ms</>"),
		*InSessionData->HostName,
		*InSessionData->MapName,
		InSessionData->CurrentPlayers,
		InSessionData->MaxPlayers,
		InSessionData->PingInMs
	);

	CommonRichText_Description->SetText(FText::FromString(Description));

	CommonRichText_DisabledReason->SetText(InSessionData->IsFull() ? FText::FromString(TEXT("This session is full.")) : FText::GetEmpty());
}

void UWidget_SessionDetailsView::ClearDetailsViewInfo()
{
	CommonTextBlock_Title->SetText(FText::GetEmpty());
	CommonRichText_Description->SetText(FText::GetEmpty());
	CommonRichText_DisabledReason->SetText(FText::GetEmpty());
}

void UWidget_SessionDetailsView::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ClearDetailsViewInfo();
}
