// Diego Gallo All Rights Reserved


#include "Widgets/Widget_HostSessionScreen.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "FrontendMultiplayerSubsystem.h"

void UWidget_HostSessionScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CommonButton_Create->OnClicked().AddUObject(this, &ThisClass::OnCreateButtonClicked);
}

void UWidget_HostSessionScreen::OnCreateButtonClicked()
{
	const FString SessionNameString = EditableTextBox_SessionName->GetText().ToString();
	const int32 MaxPlayers = static_cast<int32>(SpinBox_MaxPlayers->GetValue());

	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->CreateSession(FName(*SessionNameString), MaxPlayers);
	}
}
