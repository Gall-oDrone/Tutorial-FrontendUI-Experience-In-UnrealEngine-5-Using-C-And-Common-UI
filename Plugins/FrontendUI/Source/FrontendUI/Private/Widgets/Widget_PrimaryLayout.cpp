// Diego Gallo All Rights Reserved


#include "Widgets/Widget_PrimaryLayout.h"

#include "FrontendDebugHelper.h"

UCommonActivatableWidgetContainerBase* UWidget_PrimaryLayout::FindWidgetStackByTag(const FGameplayTag& InTag) const
{
	checkf(RegisteredWidgetStackMap.Contains(InTag), TEXT("Can not find the widget stack by the tag %s"), *InTag.ToString());
	
	return RegisteredWidgetStackMap.FindRef(InTag);
}

void UWidget_PrimaryLayout::RegisterWidgetStack(UPARAM(meta = (Categories = "Frontend.WidgetStack")) FGameplayTag InStackTag, UCommonActivatableWidgetContainerBase* InStack)
{
	if (!IsDesignTime())
	{
		// Storing a null here surfaces much later as an access violation inside CommonUI's
		// AddWidget with no hint of the cause. Fail at the point of the mistake instead.
		checkf(InStack,
			TEXT("RegisterWidgetStack(%s) was passed a null stack. In the primary layout Blueprint, mark the ")
			TEXT("Common Activatable Widget Stack as 'Is Variable' and connect it to the 'In Stack' pin."),
			*InStackTag.ToString());

		if (!RegisteredWidgetStackMap.Contains(InStackTag))
		{
			RegisteredWidgetStackMap.Add(InStackTag, InStack);

			//Debug::Print(TEXT("Widget Stack Registered under the tag ") + InStackTag.ToString());

		}
	}
}
