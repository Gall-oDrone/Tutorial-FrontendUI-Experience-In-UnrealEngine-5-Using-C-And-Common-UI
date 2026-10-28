// Diego Gallo All Rights Reserved


#include "Widgets/Widget_PauseMenu.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "Subsystems/FrontendUISubsystem.h"
#include "Subsystems/GameUIFlowSubsystem.h"
#include "FrontendFunctionLibrary.h"
#include "FrontendGameplayTags.h"
#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "GameUIPauseMenu"

UWidget_PauseMenu::UWidget_PauseMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Back is handled natively (NativeOnHandleBackAction) so there is exactly one close path.
	// Set here rather than in the Blueprint so it cannot be forgotten.
	bIsBackHandler = true;
	bIsBackActionDisplayedInActionBar = true;
}

void UWidget_PauseMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CommonButton_Resume->OnClicked().AddUObject(this, &ThisClass::OnResumeClicked);
	CommonButton_Options->OnClicked().AddUObject(this, &ThisClass::OnOptionsClicked);
	CommonButton_ReturnToMainMenu->OnClicked().AddUObject(this, &ThisClass::OnReturnToMainMenuClicked);
	CommonButton_Quit->OnClicked().AddUObject(this, &ThisClass::OnQuitClicked);

	if (!ToggleAction.IsNull())
	{
		ToggleActionHandle = RegisterUIActionBinding(
			FBindUIActionArgs(
				ToggleAction,
				true,
				FSimpleDelegate::CreateUObject(this, &ThisClass::RequestClose)
			)
		);
	}
}

void UWidget_PauseMenu::NativeOnActivated()
{
	Super::NativeOnActivated();

	// Re-armed on every activation: the same instance is reactivated when Options pops back to us.
	bCloseRequested = false;
}

bool UWidget_PauseMenu::NativeOnHandleBackAction()
{
	RequestClose();
	return true;
}

UWidget* UWidget_PauseMenu::NativeGetDesiredFocusTarget() const
{
	return CommonButton_Resume;
}

TOptional<FUIInputConfig> UWidget_PauseMenu::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UWidget_PauseMenu::RequestClose()
{
	if (bCloseRequested)
	{
		return;
	}
	bCloseRequested = true;

	// Deactivate first: CommonUI pops the stack and re-applies the HUD's input config
	// synchronously. The owner then restores viewport focus on top of that, not under it.
	DeactivateWidget();
	OnCloseRequested.Broadcast();
}

void UWidget_PauseMenu::OnResumeClicked()
{
	RequestClose();
}

void UWidget_PauseMenu::OnOptionsClicked()
{
	// The framework's Options screen is stack-agnostic: pushing it onto GameMenu means
	// its own Back returns here, not to the frontend.
	UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this);
	if (!FrontendUISubsystem)
	{
		return;
	}

	TWeakObjectPtr<UWidget_PauseMenu> WeakThis(this);

	FrontendUISubsystem->PushSoftWidgetToStackAsync(
		FrontendGameplayTags::Frontend_WidgetStack_GameMenu,
		UFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(FrontendGameplayTags::Frontend_Widget_OptionsScreen),
		[WeakThis](EAsyncPushWidgetState InPushState, UWidget_ActivatableBase* PushedWidget)
		{
			if (!WeakThis.IsValid() || !PushedWidget)
			{
				return;
			}

			if (InPushState == EAsyncPushWidgetState::OnCreatedBeforePush)
			{
				PushedWidget->SetOwningPlayer(WeakThis->GetOwningPlayer());
			}
			else if (InPushState == EAsyncPushWidgetState::AfterPush)
			{
				if (UWidget* WidgetToFocus = PushedWidget->GetDesiredFocusTarget())
				{
					WidgetToFocus->SetFocus();
				}
			}
		}
	);
}

void UWidget_PauseMenu::OnReturnToMainMenuClicked()
{
	UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this);
	if (!FrontendUISubsystem)
	{
		return;
	}

	TWeakObjectPtr<UWidget_PauseMenu> WeakThis(this);

	FrontendUISubsystem->PushConfirmScreenToModalStackAsync(
		EConfirmScreenType::YesNo,
		LOCTEXT("ReturnToMainMenuTitle", "Return to Main Menu"),
		LOCTEXT("ReturnToMainMenuMessage", "Leave the current game and return to the main menu?"),
		[WeakThis](EConfirmScreenButtonType ClickedButton)
		{
			if (ClickedButton != EConfirmScreenButtonType::Confirmed || !WeakThis.IsValid())
			{
				return;
			}

			if (UGameUIFlowSubsystem* FlowSubsystem = UGameUIFlowSubsystem::Get(WeakThis.Get()))
			{
				FlowSubsystem->ReturnToFrontend();
			}
		}
	);
}

void UWidget_PauseMenu::OnQuitClicked()
{
	UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this);
	if (!FrontendUISubsystem)
	{
		return;
	}

	TWeakObjectPtr<UWidget_PauseMenu> WeakThis(this);

	FrontendUISubsystem->PushConfirmScreenToModalStackAsync(
		EConfirmScreenType::YesNo,
		LOCTEXT("QuitTitle", "Quit Game"),
		LOCTEXT("QuitMessage", "Are you sure you want to quit to desktop?"),
		[WeakThis](EConfirmScreenButtonType ClickedButton)
		{
			if (ClickedButton != EConfirmScreenButtonType::Confirmed || !WeakThis.IsValid())
			{
				return;
			}

			UKismetSystemLibrary::QuitGame(WeakThis.Get(), WeakThis->GetOwningPlayer(), EQuitPreference::Quit, false);
		}
	);
}

#undef LOCTEXT_NAMESPACE
