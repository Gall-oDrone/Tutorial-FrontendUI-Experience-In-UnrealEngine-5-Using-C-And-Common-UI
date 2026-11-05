// Diego Gallo All Rights Reserved


#include "Widgets/Options/ListEntries/Widget_ListEntry_KeyRemap.h"
#include "Widgets/Options/DataObjects/ListDataObject_KeyRemap.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "Subsystems/FrontendUISubsystem.h"
#include "FrontendGameplayTags.h"
#include "FrontendFunctionLibrary.h"
#include "Widgets/Options/Widget_KeyRemapScreen.h"

#define LOCTEXT_NAMESPACE "FrontendUI"

void UWidget_ListEntry_KeyRemap::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CommonButton_RemapKey->OnClicked().AddUObject(this, &ThisClass::OnRemapKeyButtonClicked);
	CommonButton_ResetKeyBinding->OnClicked().AddUObject(this, &ThisClass::OnResetKeyBindingButtonClicked);
}

void UWidget_ListEntry_KeyRemap::OnOwningListDataObjectSet(UListDataObject_Base* InOwningListDataObject)
{
	Super::OnOwningListDataObjectSet(InOwningListDataObject);

	CachedOwningKeyRemapDataObject = CastChecked<UListDataObject_KeyRemap>(InOwningListDataObject);

	CommonButton_RemapKey->SetButtonDisplayImage(CachedOwningKeyRemapDataObject->GetIconFromCurrentKey());
}

void UWidget_ListEntry_KeyRemap::OnOwningListDataObjectModified(UListDataObject_Base* OwningModifiedData, EOptionsListDataModifyReason ModifyReason)
{
	if (CachedOwningKeyRemapDataObject)
	{
		CommonButton_RemapKey->SetButtonDisplayImage(CachedOwningKeyRemapDataObject->GetIconFromCurrentKey());
	}
}

void UWidget_ListEntry_KeyRemap::OnRemapKeyButtonClicked()
{
	SelectThisEntryWidget();

	UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this);
	if (!FrontendUISubsystem)
	{
		return;
	}

	FrontendUISubsystem->PushSoftWidgetToStackAsync(
		FrontendGameplayTags::Frontend_WidgetStack_Modal,
		UFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(FrontendGameplayTags::Frontend_Widget_KeyRemapScreen),
		[WeakThis = TWeakObjectPtr<UWidget_ListEntry_KeyRemap>(this)](EAsyncPushWidgetState PushState, UWidget_ActivatableBase* PushedWidget)
		{
			UWidget_ListEntry_KeyRemap* This = WeakThis.Get();
			if (This && PushState == EAsyncPushWidgetState::OnCreatedBeforePush)
			{
				UWidget_KeyRemapScreen* CreatedKeyRemapScreen = CastChecked<UWidget_KeyRemapScreen>(PushedWidget);
				CreatedKeyRemapScreen->OnKeyRemapScreenKeyPressed.BindUObject(This, &ThisClass::OnKeyToRemapPressed);
				CreatedKeyRemapScreen->OnKeyRemapScreenKeySelectCanceled.BindUObject(This, &ThisClass::OnKeyRemapCanceled);

				if (This->CachedOwningKeyRemapDataObject)
				{
					CreatedKeyRemapScreen->SetDesiredInputTypeToFilter(This->CachedOwningKeyRemapDataObject->GetDesiredInputKeyType());
				}
			}
		}
	);
}

void UWidget_ListEntry_KeyRemap::OnResetKeyBindingButtonClicked()
{
	SelectThisEntryWidget();

	if (!CachedOwningKeyRemapDataObject)
	{
		return;
	}

	const FText MappingName = CachedOwningKeyRemapDataObject->GetDataDisplayName();

	// Already the default key: tell the player instead of silently doing nothing.
	if (!CachedOwningKeyRemapDataObject->CanResetBackToDefaultValue())
	{
		UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this);
		if (!FrontendUISubsystem)
		{
			return;
		}

		FrontendUISubsystem->PushConfirmScreenToModalStackAsync(
			EConfirmScreenType::Ok,
			LOCTEXT("ResetKeyMappingTitle", "Reset Key Mapping"),
			FText::Format(LOCTEXT("KeyAlreadyDefaultFmt", "The key binding for {0} is already set to its default."), MappingName),
			[](EConfirmScreenButtonType ClickedButton){}
		);

		return;
	}

	UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this);
	if (!FrontendUISubsystem)
	{
		return;
	}

	FrontendUISubsystem->PushConfirmScreenToModalStackAsync(
		EConfirmScreenType::YesNo,
		LOCTEXT("ResetKeyMappingTitle", "Reset Key Mapping"),
		FText::Format(LOCTEXT("ResetKeyMappingMessageFmt", "Are you sure you want to reset the key binding for {0}?"), MappingName),
		[WeakThis = TWeakObjectPtr<UWidget_ListEntry_KeyRemap>(this)](EConfirmScreenButtonType ClickedButton)
		{
			UWidget_ListEntry_KeyRemap* This = WeakThis.Get();
			if (This && This->CachedOwningKeyRemapDataObject && ClickedButton == EConfirmScreenButtonType::Confirmed)
			{
				This->CachedOwningKeyRemapDataObject->TryResetBackToDefaultValue();
			}
		}
	);
}

void UWidget_ListEntry_KeyRemap::OnKeyToRemapPressed(const FKey& PressedKey)
{
	if (CachedOwningKeyRemapDataObject)
	{
		CachedOwningKeyRemapDataObject->BindNewInputKey(PressedKey);
	}
}

void UWidget_ListEntry_KeyRemap::OnKeyRemapCanceled(const FString& CanceledReason)
{
	UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this);
	if (!FrontendUISubsystem)
	{
		return;
	}

	FrontendUISubsystem->PushConfirmScreenToModalStackAsync(
		EConfirmScreenType::Ok,
		LOCTEXT("KeyRemapTitle", "Key Remap"),
		FText::FromString(CanceledReason),
		[](EConfirmScreenButtonType ClickedButton) {}
	);
}

#undef LOCTEXT_NAMESPACE
