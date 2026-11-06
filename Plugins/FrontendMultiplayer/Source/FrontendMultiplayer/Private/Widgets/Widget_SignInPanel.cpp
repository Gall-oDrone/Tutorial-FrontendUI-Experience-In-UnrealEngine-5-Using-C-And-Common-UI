// Diego Gallo All Rights Reserved


#include "Widgets/Widget_SignInPanel.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "CommonTextBlock.h"
#include "FrontendMultiplayerSubsystem.h"

#define LOCTEXT_NAMESPACE "FrontendMultiplayer"

void UWidget_SignInPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CommonButton_SignIn->OnClicked().AddUObject(this, &ThisClass::OnSignInButtonClicked);

	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->OnLoginStateChanged.AddDynamic(this, &ThisClass::HandleLoginStateChanged);
		MultiplayerSubsystem->OnSessionOperationChanged.AddDynamic(this, &ThisClass::HandleSessionOperationChanged);
	}

	Refresh();
}

void UWidget_SignInPanel::NativeDestruct()
{
	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->OnLoginStateChanged.RemoveDynamic(this, &ThisClass::HandleLoginStateChanged);
		MultiplayerSubsystem->OnSessionOperationChanged.RemoveDynamic(this, &ThisClass::HandleSessionOperationChanged);
	}

	Super::NativeDestruct();
}

void UWidget_SignInPanel::OnSignInButtonClicked()
{
	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this))
	{
		MultiplayerSubsystem->Login();
	}
}

void UWidget_SignInPanel::HandleLoginStateChanged(bool bIsLoggedIn)
{
	Refresh();
}

void UWidget_SignInPanel::HandleSessionOperationChanged(EFrontendSessionOperation Operation, bool bInProgress)
{
	// Any request may trigger a lazy sign-in, so the button follows the busy flag rather
	// than only the LoggingIn operation.
	Refresh();
}

void UWidget_SignInPanel::Refresh()
{
	UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UFrontendMultiplayerSubsystem::Get(this);
	if (!MultiplayerSubsystem || !MultiplayerSubsystem->RequiresLogin())
	{
		// LAN / stub: nothing to sign in to. The hub looks the same as before v1.0.
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	const bool bLoggedIn = MultiplayerSubsystem->IsLoggedIn();
	const bool bBusy = MultiplayerSubsystem->IsSessionOperationInProgress();
	const FString DisplayName = MultiplayerSubsystem->GetLocalPlayerDisplayName();

	// Once signed in the button has nothing left to do; keep it visible but disabled so
	// the layout does not jump, and let the label carry the state.
	CommonButton_SignIn->SetIsEnabled(!bLoggedIn && !bBusy);
	CommonButton_SignIn->SetButtonText(bLoggedIn
		? LOCTEXT("SignedIn", "Signed in")
		: LOCTEXT("SignIn", "Sign in"));

	if (CommonTextBlock_Status)
	{
		FText Status;
		if (bLoggedIn && !DisplayName.IsEmpty())
		{
			Status = FText::Format(LOCTEXT("SignedInAsFmt", "Signed in as {0}"), FText::FromString(DisplayName));
		}
		else if (bLoggedIn)
		{
			Status = LOCTEXT("SignedInNoName", "Signed in");
		}
		else if (bBusy && MultiplayerSubsystem->GetCurrentOperation() == EFrontendSessionOperation::LoggingIn)
		{
			Status = LOCTEXT("SigningIn", "Signing in... check your browser");
		}
		else
		{
			Status = LOCTEXT("NotSignedIn", "Not signed in");
		}
		CommonTextBlock_Status->SetText(Status);
	}
}

#undef LOCTEXT_NAMESPACE
