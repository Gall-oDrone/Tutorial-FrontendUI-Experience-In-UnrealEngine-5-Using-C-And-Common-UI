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
	}
}

void UWidget_APIDebugScreen::OnBackBoundActionTriggered()
{
	DeactivateWidget();
}

void UWidget_APIDebugScreen::HandleAPIDebugLogUpdated(const FString& LogLine)
{
	const FString ExistingLog = CommonTextBlock_DebugLog->GetText().ToString();
	const FString UpdatedLog = ExistingLog.IsEmpty()
		? LogLine
		: ExistingLog + LINE_TERMINATOR + LogLine;

	CommonTextBlock_DebugLog->SetText(FText::FromString(UpdatedLog));
	ScrollBox_DebugLog->ScrollToEnd();
}
