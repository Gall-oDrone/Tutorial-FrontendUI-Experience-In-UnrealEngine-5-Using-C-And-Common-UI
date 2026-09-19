// Diego Gallo All Rights Reserved


#include "Widgets/Widget_HostSessionScreen.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"
#include "FrontendMultiplayerSubsystem.h"

void UWidget_HostSessionScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RegisterUIActionBinding(
		FBindUIActionArgs(
			ICommonInputModule::GetSettings().GetDefaultBackAction(),
			true,
			FSimpleDelegate::CreateUObject(this, &ThisClass::OnBackBoundActionTriggered)
		)
	);

	CommonButton_Create->OnClicked().AddUObject(this, &ThisClass::OnCreateButtonClicked);
}

void UWidget_HostSessionScreen::OnBackBoundActionTriggered()
{
	DeactivateWidget();
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
