// Diego Gallo All Rights Reserved


#include "Widgets/Options/Widget_KeyRemapScreen.h"
#include "CommonRichTextBlock.h"
#include "Framework/Application/IInputProcessor.h"

#include "FrontendDebugHelper.h"

class FKeyRemapScreenInputPreprocessor : public IInputProcessor
{
public:
	FKeyRemapScreenInputPreprocessor(ECommonInputType InInputTypeToListenTo)
		: CachedInputTypeToListenTo(InInputTypeToListenTo)
	{

	}

DECLARE_DELEGATE_OneParam(FOnInputPreProcessorKeyPressedDelegate, const FKey& /*PressedKey*/)
FOnInputPreProcessorKeyPressedDelegate OnInputPreProcessorKeyPressed;

DECLARE_DELEGATE_OneParam(FOnInputPreProcessorKeySelectCanceledDelegate, const FString& /*CanceledReason*/)
FOnInputPreProcessorKeySelectCanceledDelegate OnInputPreProcessorKeySelectCanceled;

protected:
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
	{

	}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		/*Debug::Print(TEXT("Pressed Key: ") + InKeyEvent.GetKey().GetDisplayName().ToString());

		UEnum* StaticCommonInputType = StaticEnum<ECommonInputType>();

		Debug::Print(TEXT("Desired Input Key Type: ") + StaticCommonInputType->GetValueAsString(CachedInputTypeToListenTo));*/

		ProcessPressedKey(InKeyEvent.GetKey());

		return true;
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		/*Debug::Print(TEXT("Pressed Key: ") + MouseEvent.GetEffectingButton().GetDisplayName().ToString());*/

		ProcessPressedKey(MouseEvent.GetEffectingButton());

		return true;
	}

	void ProcessPressedKey(const FKey& InPressedKey)
	{
		if (InPressedKey == EKeys::Escape)
		{
			OnInputPreProcessorKeySelectCanceled.ExecuteIfBound(TEXT("Key Remap has been canceled"));

			return;
		}

		switch (CachedInputTypeToListenTo)
		{
		case ECommonInputType::MouseAndKeyboard:
			
			if (InPressedKey.IsGamepadKey())
			{
				OnInputPreProcessorKeySelectCanceled.ExecuteIfBound(TEXT("Detected Gamepad Key pressed for keyboard inputs. Key Remap has been canceled."));

				return;
			}

			break;
		case ECommonInputType::Gamepad:

			if (!InPressedKey.IsGamepadKey())
			{
				OnInputPreProcessorKeySelectCanceled.ExecuteIfBound(TEXT("Detected non Gamepad Key pressed for Gamepad inputs. Key Remap has been canceled."));

				return;
			}

			break;
		default:
			break;
		}

		OnInputPreProcessorKeyPressed.ExecuteIfBound(InPressedKey);
	}

private:
	ECommonInputType CachedInputTypeToListenTo;
};

void UWidget_KeyRemapScreen::SetDesiredInputTypeToFilter(ECommonInputType InDesiredInputType)
{
	CachedDesiredInputType = InDesiredInputType;
}

void UWidget_KeyRemapScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	CachedInputPreprocessor = MakeShared<FKeyRemapScreenInputPreprocessor>(CachedDesiredInputType);

	FSlateApplication::Get().RegisterInputPreProcessor(CachedInputPreprocessor, -1);
}

void UWidget_KeyRemapScreen::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();

	if (CachedInputPreprocessor)
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(CachedInputPreprocessor);

		CachedInputPreprocessor.Reset();
	}
}
