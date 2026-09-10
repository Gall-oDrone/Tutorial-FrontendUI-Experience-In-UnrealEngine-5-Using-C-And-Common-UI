// Diego Gallo All Rights Reserved


#include "Widgets/Widget_MultiplayerScreen.h"
#include "CommonButtonBase.h"

void UWidget_MultiplayerScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_Host->OnClicked().AddUObject(this, &ThisClass::OnHostButtonClicked);
	Button_Join->OnClicked().AddUObject(this, &ThisClass::OnJoinButtonClicked);
	Button_Back->OnClicked().AddUObject(this, &ThisClass::OnBackButtonClicked);
}

void UWidget_MultiplayerScreen::OnHostButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[stub] MultiplayerScreen Host clicked"));
}

void UWidget_MultiplayerScreen::OnJoinButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[stub] MultiplayerScreen Join clicked"));
}

void UWidget_MultiplayerScreen::OnBackButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[stub] MultiplayerScreen Back clicked"));
}
