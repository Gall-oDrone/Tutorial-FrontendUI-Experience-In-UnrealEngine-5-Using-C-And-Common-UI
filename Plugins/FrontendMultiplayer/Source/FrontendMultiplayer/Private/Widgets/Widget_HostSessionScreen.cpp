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

	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->OnSessionOperationChanged.AddDynamic(this, &ThisClass::HandleSessionOperationChanged);
		CommonButton_Create->SetIsEnabled(!MultiplayerSubsystem->IsSessionOperationInProgress());
	}
}

UWidget* UWidget_HostSessionScreen::NativeGetDesiredFocusTarget() const
{
	return EditableTextBox_SessionName;
}

void UWidget_HostSessionScreen::OnBackBoundActionTriggered()
{
	DeactivateWidget();
}

void UWidget_HostSessionScreen::OnCreateButtonClicked()
{
	FString SessionNameString = EditableTextBox_SessionName->GetText().ToString().TrimStartAndEnd();
	if (SessionNameString.IsEmpty())
	{
		SessionNameString = TEXT("Session");
	}

	const int32 MaxPlayers = FMath::Max(1, static_cast<int32>(SpinBox_MaxPlayers->GetValue()));

	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->CreateSession(FName(*SessionNameString), MaxPlayers);
	}
}

void UWidget_HostSessionScreen::HandleSessionOperationChanged(EFrontendSessionOperation Operation, bool bInProgress)
{
	CommonButton_Create->SetIsEnabled(!bInProgress);
}
