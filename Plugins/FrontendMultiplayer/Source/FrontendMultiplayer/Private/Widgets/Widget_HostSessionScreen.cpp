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

void UWidget_HostSessionScreen::NativeConstruct()
{
	// Paired with NativeDestruct. The widget stack pools this screen: every close destructs it
	// and every open constructs it again, while NativeOnInitialized runs once per instance, so
	// a subscription made there is gone after the first close. Before Super, so a widget set to
	// auto-activate is already listening when Super activates it.
	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->OnSessionOperationChanged.AddUniqueDynamic(this, &ThisClass::HandleSessionOperationChanged);
		// Whatever started or ended while the screen was closed was not heard: take the current state.
		CommonButton_Create->SetIsEnabled(!MultiplayerSubsystem->IsSessionOperationInProgress());
	}

	Super::NativeConstruct();
}

void UWidget_HostSessionScreen::NativeDestruct()
{
	// Subsystem delegates outlive this widget; unbind so a destroyed screen is never called.
	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->OnSessionOperationChanged.RemoveDynamic(this, &ThisClass::HandleSessionOperationChanged);
	}

	Super::NativeDestruct();
}

UWidget* UWidget_HostSessionScreen::NativeGetDesiredFocusTarget() const
{
	return EditableTextBox_SessionName;
}

void UWidget_HostSessionScreen::OnBackBoundActionTriggered()
{
	// Back while Create is still working (a queued GameLift placement can wait minutes for
	// capacity): give the request up if the backend can, so the player is not stuck watching a
	// disabled button or surprised by a travel after walking away. Backends that cannot cancel
	// (OnlineSubsystem calls) keep running and the screen just closes, as before.
	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		if (MultiplayerSubsystem->GetCurrentOperation() == EFrontendSessionOperation::Creating)
		{
			MultiplayerSubsystem->CancelSessionOperation();
		}
	}
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
