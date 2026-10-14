// Diego Gallo All Rights Reserved


#include "Widgets/Widget_APIDebugScreen.h"
#include "CommonTextBlock.h"
#include "Components/ScrollBox.h"
#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"
#include "FrontendMultiplayerSubsystem.h"

void UWidget_APIDebugScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RegisterUIActionBinding(
		FBindUIActionArgs(
			ICommonInputModule::GetSettings().GetDefaultBackAction(),
			true,
			FSimpleDelegate::CreateUObject(this, &ThisClass::OnBackBoundActionTriggered)
		)
	);

	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->OnAPIDebugLogUpdated.AddDynamic(this, &ThisClass::HandleAPIDebugLogUpdated);
		MultiplayerSubsystem->OnAPIDebugLogCleared.AddDynamic(this, &ThisClass::HandleAPIDebugLogCleared);
	}
}

void UWidget_APIDebugScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	// The widget stack pools instances, so the lines logged while this screen was
	// closed are only visible if the text is rebuilt from the history on every open.
	RebuildLogFromHistory();
}

void UWidget_APIDebugScreen::OnBackBoundActionTriggered()
{
	DeactivateWidget();
}

void UWidget_APIDebugScreen::HandleAPIDebugLogUpdated(const FString& LogLine)
{
	RebuildLogFromHistory();
}

void UWidget_APIDebugScreen::HandleAPIDebugLogCleared()
{
	RebuildLogFromHistory();
}

void UWidget_APIDebugScreen::RebuildLogFromHistory()
{
	const UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this);
	if (!MultiplayerSubsystem)
	{
		return;
	}

	const FString FullLog = FString::Join(MultiplayerSubsystem->GetAPIDebugLogHistory(), LINE_TERMINATOR);
	CommonTextBlock_DebugLog->SetText(FText::FromString(FullLog));
	ScrollBox_DebugLog->ScrollToEnd();
}
