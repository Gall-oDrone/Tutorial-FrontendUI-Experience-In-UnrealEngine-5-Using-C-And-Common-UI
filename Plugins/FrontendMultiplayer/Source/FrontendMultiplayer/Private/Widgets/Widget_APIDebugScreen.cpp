// Diego Gallo All Rights Reserved


#include "Widgets/Widget_APIDebugScreen.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/ScrollBox.h"
#include "FrontendMultiplayerSubsystem.h"

void UWidget_APIDebugScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_Back->OnClicked().AddUObject(this, &ThisClass::OnBackButtonClicked);

	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->OnAPIDebugLogUpdated.AddDynamic(this, &ThisClass::HandleAPIDebugLogUpdated);
	}
}

void UWidget_APIDebugScreen::OnBackButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[stub] APIDebugScreen Back clicked"));
}

void UWidget_APIDebugScreen::HandleAPIDebugLogUpdated(const FString& LogLine)
{
	const FString ExistingLog = Text_DebugLog->GetText().ToString();
	const FString UpdatedLog = ExistingLog.IsEmpty()
		? LogLine
		: ExistingLog + LINE_TERMINATOR + LogLine;

	Text_DebugLog->SetText(FText::FromString(UpdatedLog));
	ScrollBox_DebugLog->ScrollToEnd();
}
