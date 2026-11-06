// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FrontendTypes/FrontendEnumTypes.h"
#include "FrontendUISubsystem.generated.h"

class UWidget_PrimaryLayout;
struct FGameplayTag;
class UWidget_ActivatableBase;
class UFrontendCommonButtonBase;
class UConfirmScreenInfoObject;

enum class EAsyncPushWidgetState : uint8
{
	OnCreatedBeforePush,
	AfterPush
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnButtonDescriptionTextUpdatedDelegate, UFrontendCommonButtonBase*, BroadcastingButton, FText, DescriptionText);
/**
 * 
 */
UCLASS()
class FRONTENDUI_API UFrontendUISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	static UFrontendUISubsystem* Get(const UObject* WorldContextObject);

	//~ Begin USubsystem Interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	//~ End USubsystem Interface

	UFUNCTION(BlueprintCallable)
	void RegisterCreatedPrimaryLayout(UWidget_PrimaryLayout* InCreatedWidget);

	void PushSoftWidgetToStackAsync(const FGameplayTag& InWidgetStackTag, TSoftClassPtr<UWidget_ActivatableBase> InSoftWidgetClass,TFunction<void(EAsyncPushWidgetState,UWidget_ActivatableBase*)> AsyncPushStateCallback);

	void PushConfirmScreenToModalStackAsync(EConfirmScreenType InScreenType, const FText& InScreenTitle, const FText& InScreenMsg, TFunction<void(EConfirmScreenButtonType)> ButtonClickedCallback);
	
	UPROPERTY(BlueprintAssignable)
	FOnButtonDescriptionTextUpdatedDelegate OnButtonDescriptionTextUpdated;
private:
	UPROPERTY(Transient)
	UWidget_PrimaryLayout* CreatedPrimaryLayout;

	/** Widget class loads in flight; cancelled in Deinitialize so no callback outlives the subsystem. */
	TArray<TSharedPtr<struct FStreamableHandle>> PendingWidgetLoads;

	/** Confirm-screen descriptions waiting for their screen to load; the only reference that keeps them alive. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UConfirmScreenInfoObject>> PendingConfirmScreenInfos;

};
