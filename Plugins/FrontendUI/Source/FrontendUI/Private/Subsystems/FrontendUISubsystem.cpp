// Diego Gallo All Rights Reserved


#include "Subsystems/FrontendUISubsystem.h"
#include "Engine/AssetManager.h"
#include "Widgets/Widget_PrimaryLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widgets/Widget_ConfirmScreen.h"
#include "FrontendGameplayTags.h"
#include "FrontendFunctionLibrary.h"
#include "FrontendUILog.h"
#include "Engine/StreamableManager.h"

UFrontendUISubsystem* UFrontendUISubsystem::Get(const UObject* WorldContextObject)
{
	// Callers include widgets tearing down with their world; answer null instead of asserting.
	if (GEngine)
	{
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
		{
			return UGameInstance::GetSubsystem<UFrontendUISubsystem>(World->GetGameInstance());
		}
	}

	return nullptr;
}

bool UFrontendUISubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!CastChecked<UGameInstance>(Outer)->IsDedicatedServerInstance())
	{
		TArray<UClass*> FoundClasses;
		GetDerivedClasses(GetClass(), FoundClasses);

		return FoundClasses.IsEmpty();
	}

	return false;
}

void UFrontendUISubsystem::Deinitialize()
{
	// Loads still in flight would otherwise call back into a dead subsystem (PIE stop,
	// game exit while a screen is streaming in).
	for (const TSharedPtr<FStreamableHandle>& Handle : PendingWidgetLoads)
	{
		if (Handle.IsValid())
		{
			Handle->CancelHandle();
		}
	}
	PendingWidgetLoads.Empty();
	PendingConfirmScreenInfos.Empty();

	Super::Deinitialize();
}

void UFrontendUISubsystem::RegisterCreatedPrimaryLayout(UWidget_PrimaryLayout* InCreatedWidget)
{
	checkf(InCreatedWidget,
		TEXT("RegisterCreatedPrimaryLayout was passed a null widget. In the player controller, connect Create Widget's ")
		TEXT("Return Value to 'In Created Widget', and make sure that widget Blueprint's parent class is Widget_PrimaryLayout ")
		TEXT("(a Cast node between them means the parent class is wrong)."));

	CreatedPrimaryLayout = InCreatedWidget;
}

void UFrontendUISubsystem::PushSoftWidgetToStackAsync(const FGameplayTag& InWidgetStackTag, TSoftClassPtr<UWidget_ActivatableBase> InSoftWidgetClass,TFunction<void(EAsyncPushWidgetState, UWidget_ActivatableBase*)> AsyncPushStateCallback)
{
	check(!InSoftWidgetClass.IsNull());

	TWeakObjectPtr<UFrontendUISubsystem> WeakThis(this);
	TSharedPtr<FStreamableHandle> Handle = UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
		InSoftWidgetClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda(
			[InSoftWidgetClass, WeakThis, InWidgetStackTag, AsyncPushStateCallback]()
			{
				UFrontendUISubsystem* This = WeakThis.Get();
				if (!This)
				{
					return;
				}

				This->PendingWidgetLoads.RemoveAll([](const TSharedPtr<FStreamableHandle>& H) { return !H.IsValid() || H->HasLoadCompleted() || H->WasCanceled(); });

				UClass* LoadedWidgetClass = InSoftWidgetClass.Get();

				// Configuration slips must not take the game down. Both failures below are
				// loud (ensure = callstack + log) but recoverable: the push is simply skipped.
				if (!ensureAlwaysMsgf(LoadedWidgetClass,
					TEXT("FrontendUI: widget class '%s' did not load. Check its FrontendWidgetMap entry (Project Settings -> Game -> Frontend UI Settings): ")
					TEXT("the asset must exist at exactly that path and be saved to disk - standalone / -game instances cannot see unsaved assets."),
					*InSoftWidgetClass.ToString()))
				{
					return;
				}

				if (!ensureAlwaysMsgf(This->CreatedPrimaryLayout,
					TEXT("FrontendUI: no primary layout is registered while pushing '%s'. The owning player controller must create its Widget_PrimaryLayout ")
					TEXT("and call RegisterCreatedPrimaryLayout before anything is pushed."),
					*LoadedWidgetClass->GetName()))
				{
					return;
				}

				UCommonActivatableWidgetContainerBase* FoundWidgetStack = This->CreatedPrimaryLayout->FindWidgetStackByTag(InWidgetStackTag);
				if (!ensureAlwaysMsgf(FoundWidgetStack, TEXT("FrontendUI: no widget stack registered for tag '%s'."), *InWidgetStackTag.ToString()))
				{
					return;
				}

				UWidget_ActivatableBase* CreatedWidget = FoundWidgetStack->AddWidget<UWidget_ActivatableBase>(
					LoadedWidgetClass,
					[AsyncPushStateCallback](UWidget_ActivatableBase& CreatedWidgetInstance)
					{
						AsyncPushStateCallback(EAsyncPushWidgetState::OnCreatedBeforePush, &CreatedWidgetInstance);
					}
				);

				AsyncPushStateCallback(EAsyncPushWidgetState::AfterPush, CreatedWidget);
			}
		)
	);

	// Synchronous completion (already loaded) has already run the callback; track only loads still in flight.
	if (Handle.IsValid() && !Handle->HasLoadCompleted() && !Handle->WasCanceled())
	{
		PendingWidgetLoads.Add(Handle);
	}
}

void UFrontendUISubsystem::PushConfirmScreenToModalStackAsync(EConfirmScreenType InScreenType, const FText& InScreenTitle, const FText& InScreenMsg, TFunction<void(EConfirmScreenButtonType)> ButtonClickedCallback)
{
	UConfirmScreenInfoObject* CreatedInfoObject = nullptr;

	switch (InScreenType)
	{
	case EConfirmScreenType::Ok:
		CreatedInfoObject = UConfirmScreenInfoObject::CreateOKScreen(InScreenTitle,InScreenMsg);
		break;
	
	case EConfirmScreenType::YesNo:
		CreatedInfoObject = UConfirmScreenInfoObject::CreateYesNoScreen(InScreenTitle, InScreenMsg);
		break;

	case EConfirmScreenType::OkCancel:
		CreatedInfoObject = UConfirmScreenInfoObject::CreateOkCancelScreen(InScreenTitle, InScreenMsg);
		break;

	case EConfirmScreenType::Unknown:
		break;
	default:
		break;
	}

	check(CreatedInfoObject);

	// The info object has no owner until the screen reads it; keep it referenced across
	// the async load so a GC in between cannot collect it.
	PendingConfirmScreenInfos.Add(CreatedInfoObject);

	TWeakObjectPtr<UFrontendUISubsystem> WeakThis(this);
	PushSoftWidgetToStackAsync(
		FrontendGameplayTags::Frontend_WidgetStack_Modal,
		UFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(FrontendGameplayTags::Frontend_Widget_ConfirmScreen),
		[WeakThis, CreatedInfoObject, ButtonClickedCallback](EAsyncPushWidgetState InPushState, UWidget_ActivatableBase* PushedWidget)
		{
			if (InPushState == EAsyncPushWidgetState::OnCreatedBeforePush)
			{
				UWidget_ConfirmScreen* CreatedConfirmScreen = CastChecked<UWidget_ConfirmScreen>(PushedWidget);
				CreatedConfirmScreen->InitConfirmScreen(CreatedInfoObject, ButtonClickedCallback);
			}
			else if (UFrontendUISubsystem* This = WeakThis.Get())
			{
				This->PendingConfirmScreenInfos.Remove(CreatedInfoObject);
			}
		}
	);
}
