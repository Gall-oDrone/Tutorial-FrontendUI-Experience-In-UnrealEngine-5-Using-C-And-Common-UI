// Diego Gallo All Rights Reserved


#include "AsyncActions/AsyncAction_PushSoftWidget.h"
#include "Subsystems/FrontendUISubsystem.h"
#include "Widgets/Widget_ActivatableBase.h"

UAsyncAction_PushSoftWidget* UAsyncAction_PushSoftWidget::PushSoftWidget(const UObject* WorldContextObject, APlayerController* OwningPlayerController, TSoftClassPtr<UWidget_ActivatableBase> InSoftWidgetClass, UPARAM(meta = (Categories = "Frontend.WidgetStack")) FGameplayTag InWidgetStackTag, bool bFocusOnNewlyPushedWidget)
{
	checkf(!InSoftWidgetClass.IsNull(), TEXT("PushSoftWidgetToStack was passed a null soft widget class"));

	if (GEngine)
	{
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		{
			UAsyncAction_PushSoftWidget* Node = NewObject<UAsyncAction_PushSoftWidget>();

			Node->CachedOwningWorld = World;
			Node->CachedOwningPC = OwningPlayerController;
			Node->CachedSoftWidgetClass = InSoftWidgetClass;
			Node->CachedWidgetStackTag = InWidgetStackTag;
			Node->bCachedFocusOnNewlyPushedWidget = bFocusOnNewlyPushedWidget;

			Node->RegisterWithGameInstance(World);

			return Node;
		}
	}

	return nullptr;
}

void UAsyncAction_PushSoftWidget::Activate()
{
	UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(CachedOwningWorld.Get());
	if (!FrontendUISubsystem)
	{
		SetReadyToDestroy();
		return;
	}

	FrontendUISubsystem->PushSoftWidgetToStackAsync(CachedWidgetStackTag, CachedSoftWidgetClass,
		[WeakThis = TWeakObjectPtr<UAsyncAction_PushSoftWidget>(this)](EAsyncPushWidgetState InPushState, UWidget_ActivatableBase* PushedWidget)
		{
			UAsyncAction_PushSoftWidget* This = WeakThis.Get();
			if (!This)
			{
				return;
			}

			switch (InPushState)
			{
			case EAsyncPushWidgetState::OnCreatedBeforePush:

				PushedWidget->SetOwningPlayer(This->CachedOwningPC.Get());

				This->OnWidgetCreatedBeforePush.Broadcast(PushedWidget);

				break;

			case EAsyncPushWidgetState::AfterPush:

				This->AfterPush.Broadcast(PushedWidget);

				if (This->bCachedFocusOnNewlyPushedWidget)
				{
					if (UWidget* WidgetToFocus = PushedWidget->GetDesiredFocusTarget())
					{
						WidgetToFocus->SetFocus();
					}
				}

				This->SetReadyToDestroy();

				break;
			default:
				break;
			}
		}
	);
}
