// Diego Gallo All Rights Reserved


#include "Widgets/Widget_MultiplayerScreen.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"

void UWidget_MultiplayerScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CommonButton_Host->OnClicked().AddUObject(this, &ThisClass::OnHostButtonClicked);
	CommonButton_Join->OnClicked().AddUObject(this, &ThisClass::OnJoinButtonClicked);
}

void UWidget_MultiplayerScreen::OnHostButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[stub] MultiplayerScreen Host clicked"));
}

void UWidget_MultiplayerScreen::OnJoinButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[stub] MultiplayerScreen Join clicked"));
}
