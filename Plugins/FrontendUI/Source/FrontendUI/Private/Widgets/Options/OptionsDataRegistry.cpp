// Diego Gallo All Rights Reserved


#include "Widgets/Options/OptionsDataRegistry.h"
#include "FrontendUILog.h"
#include "Widgets/Options/DataObjects/ListDataObject_Collection.h"
#include "Widgets/Options/DataObjects/ListDataObject_String.h"
#include "Widgets/Options/OptionsDataInteractionHelper.h"
#include "FrontendSettings/FrontendGameUserSettings.h"
#include "FrontendFunctionLibrary.h"
#include "FrontendGameplayTags.h"
#include "Widgets/Options/DataObjects/ListDataObject_Scalar.h"
#include "Widgets/Options/DataObjects/ListDataObject_StringResolution.h"
#include "Internationalization/StringTableRegistry.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Widgets/Options/DataObjects/ListDataObject_KeyRemap.h"


#define LOCTEXT_NAMESPACE "FrontendUI"

#define MAKE_OPTIONS_DATA_CONTROL(SetterOrGetterFuncName) \
	MakeShared<FOptionsDataInteractionHelper>(GET_FUNCTION_NAME_STRING_CHECKED(UFrontendGameUserSettings,SetterOrGetterFuncName))

#define GET_DESCRIPTION(InKey) LOCTABLE("/FrontendUI/UI/StringTables/ST_OptionsScreenDescription.ST_OptionsScreenDescription", InKey)

void UOptionsDataRegistry::InitOptionsDataRegistry(ULocalPlayer* InOwningLocalPlayer)
{
	InitGameplayCollectionTab();
	InitAudioCollectionTab();
	InitVideoCollectionTab();
	InitControlCollectionTab(InOwningLocalPlayer);
}

TArray<UListDataObject_Base*> UOptionsDataRegistry::GetListSourceItemsBySelectedTabID(const FName& InSelectedTabID) const
{
	UListDataObject_Collection* const* FoundTabCollectionPtr = RegisteredOptionsTabCollections.FindByPredicate(
		[InSelectedTabID](UListDataObject_Collection* AvailableTabCollection)->bool
		{
			return AvailableTabCollection->GetDataID() == InSelectedTabID;
		}
	);

	checkf(FoundTabCollectionPtr, TEXT("No valid tab found under the ID %s"), *InSelectedTabID.ToString());

	UListDataObject_Collection* FoundTabCollection = *FoundTabCollectionPtr;

	TArray<UListDataObject_Base*> AllChildListItems;

	for (UListDataObject_Base* ChildListData : FoundTabCollection->GetAllChildListData())
	{
		if (!ChildListData)
		{
			continue;
		}

		AllChildListItems.Add(ChildListData);

		if (ChildListData->HasAnyChildListData())
		{
			FindChildListDataRecursively(ChildListData,AllChildListItems);
		}
	}

	return AllChildListItems;
}

void UOptionsDataRegistry::FindChildListDataRecursively(UListDataObject_Base* InParentData, TArray<UListDataObject_Base*>& OutFoundChildListData) const
{
	if (!InParentData || !InParentData->HasAnyChildListData())
	{
		return;
	}

	for (UListDataObject_Base* SubChildListData : InParentData->GetAllChildListData())
	{
		if (!SubChildListData)
		{
			continue;
		}

		OutFoundChildListData.Add(SubChildListData);

		if (SubChildListData->HasAnyChildListData())
		{
			FindChildListDataRecursively(SubChildListData, OutFoundChildListData);
		}
	}

}

void UOptionsDataRegistry::InitGameplayCollectionTab()
{
	UListDataObject_Collection* GameplayTabCollection = NewObject<UListDataObject_Collection>();
	GameplayTabCollection->SetDataID(FName("GameplayTabCollection"));
	GameplayTabCollection->SetDataDisplayName(LOCTEXT("Gameplay_e04c71", "Gameplay"));

	//This is the full code for constructor data interactor helper
	/*TSharedPtr<FOptionsDataInteractionHelper> ConstructedHelper = MakeShared<FOptionsDataInteractionHelper>(
		GET_FUNCTION_NAME_STRING_CHECKED(UFrontendGameUserSettings, GetCurrentGameDifficulty)
	);*/

	//Game Difficulty
	{
		UListDataObject_String* GameDifficulty = NewObject<UListDataObject_String>();
		GameDifficulty->SetDataID(FName("GameDifficulty"));
		GameDifficulty->SetDataDisplayName(LOCTEXT("Difficulty_7945d3", "Difficulty"));
		GameDifficulty->SetDescriptionRichText(LOCTEXT("AdjustsTheDifficultyOfTheGameExperienceN_d7b1a8", "Adjusts the difficulty of the game experience.\n\n<Bold>Easy: </> Focuses on the story experience. Provides the most relaxing combat.\n\n<Bold>Normal: </>Offers slightly harder combat experience.\n\n<Bold>Hard: </>Offers a much more challenging combat experience.\n\n<Bold>Very Hard: </>Provides the most challenging combat experience. Not recommended for first play through."));
		GameDifficulty->AddDynamicOption(TEXT("Easy"), LOCTEXT("Easy_00f031", "Easy"));
		GameDifficulty->AddDynamicOption(TEXT("Normal"), LOCTEXT("Normal_45e118", "Normal"));
		GameDifficulty->AddDynamicOption(TEXT("Hard"), LOCTEXT("Hard_20a899", "Hard"));
		GameDifficulty->AddDynamicOption(TEXT("Very Hard"), LOCTEXT("VeryHard_400817", "Very Hard"));
		GameDifficulty->SetDefaultValueFromString(TEXT("Normal"));
		GameDifficulty->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetCurrentGameDifficulty));
		GameDifficulty->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetCurrentGameDifficulty));
		GameDifficulty->SetShouldApplySettingsImmediately(true);

		GameplayTabCollection->AddChildListData(GameDifficulty);
	}

	// Game-specific rows (for example the tutorial's "Test Image Item") belong in a
	// UOptionsDataRegistry subclass registered via UFrontendDeveloperSettings; see
	// UGameUIOptionsDataRegistry in the GameUI plugin for the pattern.

	RegisteredOptionsTabCollections.Add(GameplayTabCollection);
}

void UOptionsDataRegistry::InitAudioCollectionTab()
{
	UListDataObject_Collection* AudioTabCollection = NewObject<UListDataObject_Collection>();
	AudioTabCollection->SetDataID(FName("AudioTabCollection"));
	AudioTabCollection->SetDataDisplayName(LOCTEXT("Audio_acdac2", "Audio"));

	//Volume Category
	{
		UListDataObject_Collection* VolumeCategoryCollection = NewObject<UListDataObject_Collection>();
		VolumeCategoryCollection->SetDataID(FName("VolumeCategoryCollection"));
		VolumeCategoryCollection->SetDataDisplayName(LOCTEXT("Volume_3b18e8", "Volume"));

		AudioTabCollection->AddChildListData(VolumeCategoryCollection);

		//Overall Volume
		{
			UListDataObject_Scalar* OverallVolume = NewObject<UListDataObject_Scalar>();
			OverallVolume->SetDataID(FName("OverallVolume"));
			OverallVolume->SetDataDisplayName(LOCTEXT("OverallVolume_c6a603", "Overall Volume"));
			OverallVolume->SetDescriptionRichText(LOCTEXT("AdjustsTheOverallVolume_5d4bb8", "Adjusts the overall volume."));
			OverallVolume->SetDisplayValueRange(TRange<float>(0.f, 1.f));
			OverallVolume->SetOutputValueRange(TRange<float>(0.f, 2.f));
			OverallVolume->SetSliderStepSize(0.01f);
			OverallVolume->SetDefaultValueFromString(LexToString(1.f));
			OverallVolume->SetDisplayNumericType(ECommonNumericType::Percentage);
			OverallVolume->SetNumberFormattingOptions(UListDataObject_Scalar::NoDecimal()); //No Decimal: 50% //One Decimal: 50.5%
			OverallVolume->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetOverallVolume));
			OverallVolume->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetOverallVolume));
			OverallVolume->SetShouldApplySettingsImmediately(true);

			VolumeCategoryCollection->AddChildListData(OverallVolume);
		}

		//Sound FX Volume
		{
			UListDataObject_Scalar* SoundFXVolume = NewObject<UListDataObject_Scalar>();
			SoundFXVolume->SetDataID(FName("SoundFXVolume"));
			SoundFXVolume->SetDataDisplayName(LOCTEXT("SoundEffectsVolume_01edd0", "Sound Effects Volume"));
			SoundFXVolume->SetDescriptionRichText(LOCTEXT("AdjustsTheSoundEffectsVolume_5d9efb", "Adjusts the sound effects volume."));
			SoundFXVolume->SetDisplayValueRange(TRange<float>(0.f, 1.f));
			SoundFXVolume->SetOutputValueRange(TRange<float>(0.f, 2.f));
			SoundFXVolume->SetSliderStepSize(0.01f);
			SoundFXVolume->SetDefaultValueFromString(LexToString(1.f));
			SoundFXVolume->SetDisplayNumericType(ECommonNumericType::Percentage);
			SoundFXVolume->SetNumberFormattingOptions(UListDataObject_Scalar::NoDecimal()); //No Decimal: 50% //One Decimal: 50.5%
			SoundFXVolume->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetSoundFXVolume));
			SoundFXVolume->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetSoundFXVolume));
			SoundFXVolume->SetShouldApplySettingsImmediately(true);

			VolumeCategoryCollection->AddChildListData(SoundFXVolume);
		}

		//Music Volume
		{
			UListDataObject_Scalar* MusicVolume = NewObject<UListDataObject_Scalar>();
			MusicVolume->SetDataID(FName("MusicVolume"));
			MusicVolume->SetDataDisplayName(LOCTEXT("MusicVolume_3d6052", "Music Volume"));
			MusicVolume->SetDescriptionRichText(LOCTEXT("AdjustsTheMusicVolume_d295ad", "Adjusts the music volume."));
			MusicVolume->SetDisplayValueRange(TRange<float>(0.f, 1.f));
			MusicVolume->SetOutputValueRange(TRange<float>(0.f, 2.f));
			MusicVolume->SetSliderStepSize(0.01f);
			MusicVolume->SetDefaultValueFromString(LexToString(1.f));
			MusicVolume->SetDisplayNumericType(ECommonNumericType::Percentage);
			MusicVolume->SetNumberFormattingOptions(UListDataObject_Scalar::NoDecimal()); //No Decimal: 50% //One Decimal: 50.5%
			MusicVolume->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetMusicVolume));
			MusicVolume->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetMusicVolume));
			MusicVolume->SetShouldApplySettingsImmediately(true);

			VolumeCategoryCollection->AddChildListData(MusicVolume);
		}
	}

	// Sound Category
	{
		UListDataObject_Collection* SoundCategoryCollection = NewObject<UListDataObject_Collection>();
		SoundCategoryCollection->SetDataID(FName("SoundCategoryCollection"));
		SoundCategoryCollection->SetDataDisplayName(LOCTEXT("Sound_b4e3ef", "Sound"));

		AudioTabCollection->AddChildListData(SoundCategoryCollection);

		//Allow Background Audio
		{
			UListDataObject_StringBool* AllowBackgroundAudio = NewObject<UListDataObject_StringBool>();
			AllowBackgroundAudio->SetDataID(FName("AllowBackgroundAudio"));
			AllowBackgroundAudio->SetDataDisplayName(LOCTEXT("AllowBackgroundAudio_ac5c21", "Allow Background Audio"));
			AllowBackgroundAudio->OverrideTrueDisplayText(LOCTEXT("Enabled_df174a", "Enabled"));
			AllowBackgroundAudio->OverrideFalseDisplayText(LOCTEXT("Disabled_f4f447", "Disabled"));
			AllowBackgroundAudio->SetFalseAsDefaultValue();
			AllowBackgroundAudio->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetAllowBackgroundAudio));
			AllowBackgroundAudio->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetAllowBackgroundAudio));
			AllowBackgroundAudio->SetShouldApplySettingsImmediately(true);

			SoundCategoryCollection->AddChildListData(AllowBackgroundAudio);
		}

		//Use HDR Audio
		{
			UListDataObject_StringBool* UseHDRAudioMode = NewObject<UListDataObject_StringBool>();
			UseHDRAudioMode->SetDataID(FName("UseHDRAudioMode"));
			UseHDRAudioMode->SetDataDisplayName(LOCTEXT("UseHDRAudioMode_4db3a6", "Use HDR Audio Mode"));
			UseHDRAudioMode->OverrideTrueDisplayText(LOCTEXT("Enabled_df174a", "Enabled"));
			UseHDRAudioMode->OverrideFalseDisplayText(LOCTEXT("Disabled_f4f447", "Disabled"));
			UseHDRAudioMode->SetFalseAsDefaultValue();
			UseHDRAudioMode->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetUseHDRAudioMode));
			UseHDRAudioMode->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetUseHDRAudioMode));
			UseHDRAudioMode->SetShouldApplySettingsImmediately(true);

			SoundCategoryCollection->AddChildListData(UseHDRAudioMode);
		}
	}

	RegisteredOptionsTabCollections.Add(AudioTabCollection);
}

void UOptionsDataRegistry::InitVideoCollectionTab()
{
	UListDataObject_Collection* VideoTabCollection = NewObject<UListDataObject_Collection>();
	VideoTabCollection->SetDataID(FName("VideoTabCollection"));
	VideoTabCollection->SetDataDisplayName(LOCTEXT("Video_bc17c1", "Video"));

	UListDataObject_StringEnum* CreatedWindowMode = nullptr;

	//Display Category
	{
		UListDataObject_Collection* DisplayCategoryCollection = NewObject<UListDataObject_Collection>();
		DisplayCategoryCollection->SetDataID(FName("DisplayCategoryCollection"));
		DisplayCategoryCollection->SetDataDisplayName(LOCTEXT("Display_574ff9", "Display"));

		VideoTabCollection->AddChildListData(DisplayCategoryCollection);

		FOptionsDataEditConditionDescriptor PackageBuildOnlyCondition;
		PackageBuildOnlyCondition.SetEditConditionFunc(
			[]()->bool
			{
				const bool bIsInEditor = GIsEditor || GIsPlayInEditorWorld;

				return !bIsInEditor;
			}
		);
		PackageBuildOnlyCondition.SetDisabledRichReason(TEXT("\n\n<Disabled>This setting can only be adjusted in a packaged build.</>"));

		//Window Mode
		{
			/*const FText WindowModeDesc = LOCTABLE("/FrontendUI/UI/StringTables/ST_OptionsScreenDescription.ST_OptionsScreenDescription", "WindowModeDescKey");*/

			UListDataObject_StringEnum* WindowMode = NewObject<UListDataObject_StringEnum>();
			WindowMode->SetDataID(FName("WindowMode"));
			WindowMode->SetDataDisplayName(LOCTEXT("WindowMode_03df46", "Window Mode"));
			WindowMode->SetDescriptionRichText(GET_DESCRIPTION("WindowModeDescKey"));
			WindowMode->AddEnumOption(EWindowMode::Fullscreen, LOCTEXT("FullscreenMode_55d050", "Fullscreen Mode"));
			WindowMode->AddEnumOption(EWindowMode::WindowedFullscreen, LOCTEXT("BorderlessWindow_560788", "Borderless Window"));
			WindowMode->AddEnumOption(EWindowMode::Windowed, LOCTEXT("Windowed_7d22ec", "Windowed"));
			WindowMode->SetDefaultValueFromEnumOption(EWindowMode::WindowedFullscreen);
			WindowMode->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetFullscreenMode));
			WindowMode->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetFullscreenMode));
			WindowMode->SetShouldApplySettingsImmediately(true);

			WindowMode->AddEditCondition(PackageBuildOnlyCondition);

			CreatedWindowMode = WindowMode;

			DisplayCategoryCollection->AddChildListData(WindowMode);
		}

		//Screen Resolution
		{
			UListDataObject_StringResolution* ScreenResolution = NewObject<UListDataObject_StringResolution>();
			ScreenResolution->SetDataID(FName("ScreenResolution"));
			ScreenResolution->SetDataDisplayName(LOCTEXT("ScreenResolution_390a92", "Screen Resolution"));
			ScreenResolution->SetDescriptionRichText(GET_DESCRIPTION("ScreenResolutionsDescKey"));
			ScreenResolution->InitResolutionValues();
			ScreenResolution->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetScreenResolution));
			ScreenResolution->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetScreenResolution));
			ScreenResolution->SetShouldApplySettingsImmediately(true);

			ScreenResolution->AddEditCondition(PackageBuildOnlyCondition);

			FOptionsDataEditConditionDescriptor WindowModeEditCondition;
			WindowModeEditCondition.SetEditConditionFunc(
				[CreatedWindowMode]()->bool
				{
					const bool bIsBorderlessWindow = CreatedWindowMode->GetCurrentValueAsEnum<EWindowMode::Type>() == EWindowMode::WindowedFullscreen;

					return !bIsBorderlessWindow;
				}
			);
			WindowModeEditCondition.SetDisabledRichReason(TEXT("\n\n<Disabled>Screen Resolution is not adjustable when the 'Window Mode' is set to Borderless Window. The value must match with the maximum allowed resolution.</>"));
			WindowModeEditCondition.SetDisabledForcedStringValue(ScreenResolution->GetMaximumAllowedResolution());

			ScreenResolution->AddEditCondition(WindowModeEditCondition);

			ScreenResolution->AddEditDependencyData(CreatedWindowMode);

			DisplayCategoryCollection->AddChildListData(ScreenResolution);
		}
	}

	//Graphics Category
	{
		UListDataObject_Collection* GraphicsCategoryCollection = NewObject<UListDataObject_Collection>();
		GraphicsCategoryCollection->SetDataID(FName("GraphicsCategoryCollection"));
		GraphicsCategoryCollection->SetDataDisplayName(LOCTEXT("Graphics_ab255c", "Graphics"));

		VideoTabCollection->AddChildListData(GraphicsCategoryCollection);

		//Display Gamma
		{
			UListDataObject_Scalar* DisplayGamma = NewObject<UListDataObject_Scalar>();
			DisplayGamma->SetDataID(FName("DisplayGamma"));
			DisplayGamma->SetDataDisplayName(LOCTEXT("Brightness_e1a2b6", "Brightness"));
			DisplayGamma->SetDescriptionRichText(GET_DESCRIPTION("DisplayGammaDescKey"));
			DisplayGamma->SetDisplayValueRange(TRange<float>(0.f, 1.f));
			DisplayGamma->SetOutputValueRange(TRange<float>(1.7f, 2.7f)); // The default value Unreal has is: 2.2f
			DisplayGamma->SetSliderStepSize(0.01f);
			DisplayGamma->SetDisplayNumericType(ECommonNumericType::Percentage);
			DisplayGamma->SetNumberFormattingOptions(UListDataObject_Scalar::NoDecimal());
			DisplayGamma->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetCurrentDisplayGamma));
			DisplayGamma->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetCurrentDisplayGamma));
			DisplayGamma->SetDefaultValueFromString(LexToString(2.2f));

			GraphicsCategoryCollection->AddChildListData(DisplayGamma);
		}

		UListDataObject_StringInteger* CreatedOverallQuality = nullptr;

		//Overall Quality
		{
			UListDataObject_StringInteger* OverallQuality = NewObject<UListDataObject_StringInteger>();
			OverallQuality->SetDataID(FName("OverallQuality"));
			OverallQuality->SetDataDisplayName(LOCTEXT("OverallQuality_e85506", "Overall Quality"));
			OverallQuality->SetDescriptionRichText(GET_DESCRIPTION("OverallQualityDescKey"));
			OverallQuality->AddIntegerOption(0, LOCTEXT("Low_a12494", "Low"));
			OverallQuality->AddIntegerOption(1, LOCTEXT("Medium_d40496", "Medium"));
			OverallQuality->AddIntegerOption(2, LOCTEXT("High_b1a595", "High"));
			OverallQuality->AddIntegerOption(3, LOCTEXT("Epic_b3efd0", "Epic"));
			OverallQuality->AddIntegerOption(4, LOCTEXT("Cinematic_bf9d43", "Cinematic"));
			OverallQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetOverallScalabilityLevel));
			OverallQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetOverallScalabilityLevel));
			OverallQuality->SetShouldApplySettingsImmediately(true);

			GraphicsCategoryCollection->AddChildListData(OverallQuality);

			CreatedOverallQuality = OverallQuality;
		}

		//Resolution Scale
		{
			UListDataObject_Scalar* ResolutionScale = NewObject<UListDataObject_Scalar>();
			ResolutionScale->SetDataID(FName("ResolutionScale"));
			ResolutionScale->SetDataDisplayName(LOCTEXT("3DResolution_fb0fd1", "3D Resolution"));
			ResolutionScale->SetDescriptionRichText(GET_DESCRIPTION("ResolutionScaleDescKey"));
			ResolutionScale->SetDisplayValueRange(TRange<float>(0.f, 1.f));
			ResolutionScale->SetOutputValueRange(TRange<float>(0.f, 1.f));
			ResolutionScale->SetSliderStepSize(0.01f);
			ResolutionScale->SetDisplayNumericType(ECommonNumericType::Percentage);
			ResolutionScale->SetNumberFormattingOptions(UListDataObject_Scalar::NoDecimal());
			ResolutionScale->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetResolutionScaleNormalized));
			ResolutionScale->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetResolutionScaleNormalized));
			ResolutionScale->SetShouldApplySettingsImmediately(true);

			GraphicsCategoryCollection->AddEditDependencyData(CreatedOverallQuality);

			GraphicsCategoryCollection->AddChildListData(ResolutionScale);
		}

		//Global Illumination Quality
		{
			UListDataObject_StringInteger* GlobalIlluminationQuality = NewObject<UListDataObject_StringInteger>();
			GlobalIlluminationQuality->SetDataID(FName("GlobalIlluminationQuality"));
			GlobalIlluminationQuality->SetDataDisplayName(LOCTEXT("GlobalIllumination_d589f2", "Global Illumination"));
			GlobalIlluminationQuality->SetDescriptionRichText(GET_DESCRIPTION("GlobalIlluminationQualityDescKey"));
			GlobalIlluminationQuality->AddIntegerOption(0, LOCTEXT("Low_a12494", "Low"));
			GlobalIlluminationQuality->AddIntegerOption(1, LOCTEXT("Medium_d40496", "Medium"));
			GlobalIlluminationQuality->AddIntegerOption(2, LOCTEXT("High_b1a595", "High"));
			GlobalIlluminationQuality->AddIntegerOption(3, LOCTEXT("Epic_b3efd0", "Epic"));
			GlobalIlluminationQuality->AddIntegerOption(4, LOCTEXT("Cinematic_bf9d43", "Cinematic"));
			GlobalIlluminationQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetGlobalIlluminationQuality));
			GlobalIlluminationQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetGlobalIlluminationQuality));
			GlobalIlluminationQuality->SetShouldApplySettingsImmediately(true);

			GlobalIlluminationQuality->AddEditDependencyData(CreatedOverallQuality);
			
			CreatedOverallQuality->AddEditDependencyData(GlobalIlluminationQuality);

			GraphicsCategoryCollection->AddChildListData(GlobalIlluminationQuality);
		}

		//Shadown Quality
		{
			UListDataObject_StringInteger* ShadowQuality = NewObject<UListDataObject_StringInteger>();
			ShadowQuality->SetDataID(FName("ShadowQuality"));
			ShadowQuality->SetDataDisplayName(LOCTEXT("ShadowQuality_c300d6", "Shadow Quality"));
			ShadowQuality->SetDescriptionRichText(GET_DESCRIPTION("ShadowQualityDescKey"));
			ShadowQuality->AddIntegerOption(0, LOCTEXT("Low_a12494", "Low"));
			ShadowQuality->AddIntegerOption(1, LOCTEXT("Medium_d40496", "Medium"));
			ShadowQuality->AddIntegerOption(2, LOCTEXT("High_b1a595", "High"));
			ShadowQuality->AddIntegerOption(3, LOCTEXT("Epic_b3efd0", "Epic"));
			ShadowQuality->AddIntegerOption(4, LOCTEXT("Cinematic_bf9d43", "Cinematic"));
			ShadowQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetShadowQuality));
			ShadowQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetShadowQuality));
			ShadowQuality->SetShouldApplySettingsImmediately(true);

			ShadowQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(ShadowQuality);

			GraphicsCategoryCollection->AddChildListData(ShadowQuality);
		}

		//AntiAliasing Quality
		{
			UListDataObject_StringInteger* AntiAliasingQuality = NewObject<UListDataObject_StringInteger>();
			AntiAliasingQuality->SetDataID(FName("AntiAliasingQuality"));
			AntiAliasingQuality->SetDataDisplayName(LOCTEXT("AntiAliasingQuality_ce5a77", "AntiAliasing Quality"));
			AntiAliasingQuality->SetDescriptionRichText(GET_DESCRIPTION("AntiAliasingDescKey"));
			AntiAliasingQuality->AddIntegerOption(0, LOCTEXT("Low_a12494", "Low"));
			AntiAliasingQuality->AddIntegerOption(1, LOCTEXT("Medium_d40496", "Medium"));
			AntiAliasingQuality->AddIntegerOption(2, LOCTEXT("High_b1a595", "High"));
			AntiAliasingQuality->AddIntegerOption(3, LOCTEXT("Epic_b3efd0", "Epic"));
			AntiAliasingQuality->AddIntegerOption(4, LOCTEXT("Cinematic_bf9d43", "Cinematic"));
			AntiAliasingQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetAntiAliasingQuality));
			AntiAliasingQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetAntiAliasingQuality));
			AntiAliasingQuality->SetShouldApplySettingsImmediately(true);

			AntiAliasingQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(AntiAliasingQuality);

			GraphicsCategoryCollection->AddChildListData(AntiAliasingQuality);
		}

		//View Distance Quality
		{
			UListDataObject_StringInteger* ViewDistanceQuality = NewObject<UListDataObject_StringInteger>();
			ViewDistanceQuality->SetDataID(FName("ViewDistanceQuality"));
			ViewDistanceQuality->SetDataDisplayName(LOCTEXT("ViewDistance_d59a49", "View Distance"));
			ViewDistanceQuality->SetDescriptionRichText(GET_DESCRIPTION("ViewDistanceDescKey"));
			ViewDistanceQuality->AddIntegerOption(0, LOCTEXT("Near_fcc451", "Near"));
			ViewDistanceQuality->AddIntegerOption(1, LOCTEXT("Medium_d40496", "Medium"));
			ViewDistanceQuality->AddIntegerOption(2, LOCTEXT("Far_49b2cf", "Far"));
			ViewDistanceQuality->AddIntegerOption(3, LOCTEXT("VeryFar_512df8", "Very Far"));
			ViewDistanceQuality->AddIntegerOption(4, LOCTEXT("Cinematic_bf9d43", "Cinematic"));
			ViewDistanceQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetViewDistanceQuality));
			ViewDistanceQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetViewDistanceQuality));
			ViewDistanceQuality->SetShouldApplySettingsImmediately(true);

			ViewDistanceQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(ViewDistanceQuality);

			GraphicsCategoryCollection->AddChildListData(ViewDistanceQuality);
		}

		//Texture Quality
		{
			UListDataObject_StringInteger* TextureQuality = NewObject<UListDataObject_StringInteger>();
			TextureQuality->SetDataID(FName("TextureQuality"));
			TextureQuality->SetDataDisplayName(LOCTEXT("TextureQuality_96e585", "Texture Quality"));
			TextureQuality->SetDescriptionRichText(GET_DESCRIPTION("TextureQualityDescKey"));
			TextureQuality->AddIntegerOption(0, LOCTEXT("Low_a12494", "Low"));
			TextureQuality->AddIntegerOption(1, LOCTEXT("Medium_d40496", "Medium"));
			TextureQuality->AddIntegerOption(2, LOCTEXT("High_b1a595", "High"));
			TextureQuality->AddIntegerOption(3, LOCTEXT("Epic_b3efd0", "Epic"));
			TextureQuality->AddIntegerOption(4, LOCTEXT("Cinematic_bf9d43", "Cinematic"));
			TextureQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetTextureQuality));
			TextureQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetTextureQuality));
			TextureQuality->SetShouldApplySettingsImmediately(true);

			TextureQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(TextureQuality);

			GraphicsCategoryCollection->AddChildListData(TextureQuality);
		}

		//Visual Effects Quality
		{
			UListDataObject_StringInteger* VisualEffectQuality = NewObject<UListDataObject_StringInteger>();
			VisualEffectQuality->SetDataID(FName("VisualEffectQuality"));
			VisualEffectQuality->SetDataDisplayName(LOCTEXT("VisualEffectQuality_0efb3c", "Visual Effect Quality"));
			VisualEffectQuality->SetDescriptionRichText(GET_DESCRIPTION("VisualEffectQualityDescKey"));
			VisualEffectQuality->AddIntegerOption(0, LOCTEXT("Low_a12494", "Low"));
			VisualEffectQuality->AddIntegerOption(1, LOCTEXT("Medium_d40496", "Medium"));
			VisualEffectQuality->AddIntegerOption(2, LOCTEXT("High_b1a595", "High"));
			VisualEffectQuality->AddIntegerOption(3, LOCTEXT("Epic_b3efd0", "Epic"));
			VisualEffectQuality->AddIntegerOption(4, LOCTEXT("Cinematic_bf9d43", "Cinematic"));
			VisualEffectQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetVisualEffectQuality));
			VisualEffectQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetVisualEffectQuality));
			VisualEffectQuality->SetShouldApplySettingsImmediately(true);

			VisualEffectQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(VisualEffectQuality);

			GraphicsCategoryCollection->AddChildListData(VisualEffectQuality);
		}

		//Reflection Quality
		{
			UListDataObject_StringInteger* ReflectionQuality = NewObject<UListDataObject_StringInteger>();
			ReflectionQuality->SetDataID(FName("ReflectionQuality"));
			ReflectionQuality->SetDataDisplayName(LOCTEXT("ReflectionQuality_26864c", "Reflection Quality"));
			ReflectionQuality->SetDescriptionRichText(GET_DESCRIPTION("ReflectionQualityDescKey"));
			ReflectionQuality->AddIntegerOption(0, LOCTEXT("Low_a12494", "Low"));
			ReflectionQuality->AddIntegerOption(1, LOCTEXT("Medium_d40496", "Medium"));
			ReflectionQuality->AddIntegerOption(2, LOCTEXT("High_b1a595", "High"));
			ReflectionQuality->AddIntegerOption(3, LOCTEXT("Epic_b3efd0", "Epic"));
			ReflectionQuality->AddIntegerOption(4, LOCTEXT("Cinematic_bf9d43", "Cinematic"));
			ReflectionQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetReflectionQuality));
			ReflectionQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetReflectionQuality));
			ReflectionQuality->SetShouldApplySettingsImmediately(true);

			ReflectionQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(ReflectionQuality);

			GraphicsCategoryCollection->AddChildListData(ReflectionQuality);
		}

		//Post Processing Quality
		{
			UListDataObject_StringInteger* PostProcessingQuality = NewObject<UListDataObject_StringInteger>();
			PostProcessingQuality->SetDataID(FName("PostProcessingQuality"));
			PostProcessingQuality->SetDataDisplayName(LOCTEXT("PostProcessingQuality_c5a7a1", "Post Processing Quality"));
			PostProcessingQuality->SetDescriptionRichText(GET_DESCRIPTION("PostProcessingQualityDescKey"));
			PostProcessingQuality->AddIntegerOption(0, LOCTEXT("Low_a12494", "Low"));
			PostProcessingQuality->AddIntegerOption(1, LOCTEXT("Medium_d40496", "Medium"));
			PostProcessingQuality->AddIntegerOption(2, LOCTEXT("High_b1a595", "High"));
			PostProcessingQuality->AddIntegerOption(3, LOCTEXT("Epic_b3efd0", "Epic"));
			PostProcessingQuality->AddIntegerOption(4, LOCTEXT("Cinematic_bf9d43", "Cinematic"));
			PostProcessingQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetPostProcessingQuality));
			PostProcessingQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetPostProcessingQuality));
			PostProcessingQuality->SetShouldApplySettingsImmediately(true);

			PostProcessingQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(PostProcessingQuality);

			GraphicsCategoryCollection->AddChildListData(PostProcessingQuality);
		}
	}

	//Advanced graphics Category
	{
		UListDataObject_Collection* AdvancedGraphicsCategoryCollection = NewObject<UListDataObject_Collection>();
		AdvancedGraphicsCategoryCollection->SetDataID(FName("AdvancedGraphicsCategoryCollection"));
		AdvancedGraphicsCategoryCollection->SetDataDisplayName(LOCTEXT("AdvancedGraphics_ef2779", "Advanced Graphics"));

		VideoTabCollection->AddChildListData(AdvancedGraphicsCategoryCollection);

		//Vertical Sync
		{
			UListDataObject_StringBool* VerticalSync = NewObject<UListDataObject_StringBool>();
			VerticalSync->SetDataID(FName("VerticalSync"));
			VerticalSync->SetDataDisplayName(LOCTEXT("VSync_c785dc", "V-Sync"));
			VerticalSync->SetDescriptionRichText(GET_DESCRIPTION("VerticalSyncDescKey"));
			VerticalSync->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(IsVSyncEnabled));
			VerticalSync->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetVSyncEnabled));
			VerticalSync->SetFalseAsDefaultValue();
			VerticalSync->SetShouldApplySettingsImmediately(true);

			FOptionsDataEditConditionDescriptor FullscreenOnlyCondition;
			FullscreenOnlyCondition.SetEditConditionFunc(
				[CreatedWindowMode]()->bool
				{
					return CreatedWindowMode->GetCurrentValueAsEnum<EWindowMode::Type>() == EWindowMode::Fullscreen;
				}
			);
			FullscreenOnlyCondition.SetDisabledRichReason(TEXT("\n\n<Disabled>This option is only available when 'Window Mode' is set to 'Fullscreen'.</>"));
			FullscreenOnlyCondition.SetDisabledForcedStringValue(TEXT("false"));

			VerticalSync->AddEditCondition(FullscreenOnlyCondition);

			AdvancedGraphicsCategoryCollection->AddChildListData(VerticalSync);

		}
		
		//Frame Rate Limit
		{
			UListDataObject_String* FrameRateLimit = NewObject<UListDataObject_String>();
			FrameRateLimit->SetDataID(FName("FrameRateLimit"));
			FrameRateLimit->SetDataDisplayName(LOCTEXT("FrameRateLimit_ccc7ef", "Frame Rate Limit"));
			FrameRateLimit->SetDescriptionRichText(GET_DESCRIPTION("FrameRateLimitDescKey"));
			FrameRateLimit->AddDynamicOption(LexToString(30.f),LOCTEXT("30FPS_7d5927", "30 FPS"));
			FrameRateLimit->AddDynamicOption(LexToString(60.f),LOCTEXT("60FPS_aa38e5", "60 FPS"));
			FrameRateLimit->AddDynamicOption(LexToString(90.f),LOCTEXT("90FPS_cfd80e", "90 FPS"));
			FrameRateLimit->AddDynamicOption(LexToString(120.f),LOCTEXT("120FPS_04283a", "120 FPS"));
			FrameRateLimit->AddDynamicOption(LexToString(0.f),LOCTEXT("NoLimit_4e4ff4", "No Limit"));
			FrameRateLimit->SetDefaultValueFromString(LexToString(0.f));
			FrameRateLimit->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetFrameRateLimit));
			FrameRateLimit->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetFrameRateLimit));
			FrameRateLimit->SetShouldApplySettingsImmediately(true);

			AdvancedGraphicsCategoryCollection->AddChildListData(FrameRateLimit);
		}
	}

	RegisteredOptionsTabCollections.Add(VideoTabCollection);
}

void UOptionsDataRegistry::InitControlCollectionTab(ULocalPlayer* InOwningLocalPlayer)
{
	UListDataObject_Collection* ControlTabCollection = NewObject<UListDataObject_Collection>();
	ControlTabCollection->SetDataID(FName("ControlTabCollection"));
	ControlTabCollection->SetDataDisplayName(LOCTEXT("Control_ea1d3d", "Control"));


	UEnhancedInputLocalPlayerSubsystem* EISubsystem = InOwningLocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	check(EISubsystem);

	UEnhancedInputUserSettings* EIUserSettings = EISubsystem->GetUserSettings();

	// Key remapping is built on Enhanced Input's user settings, which are an opt-in project
	// feature. A host that has not enabled them gets no Controls tab rather than a crash.
	if (!EIUserSettings)
	{
		UE_LOG(LogFrontendUI, Warning,
			TEXT("Enhanced Input user settings are disabled, so the Options screen has no Controls tab. ")
			TEXT("Enable them in Project Settings -> Engine -> Enhanced Input -> User Settings -> Enable User Settings ")
			TEXT("(DefaultInput.ini: [/Script/EnhancedInput.EnhancedInputDeveloperSettings] bEnableUserSettings=True) and restart the editor."));
		return;
	}

	//Keyboard Mouse Category
	{
		UListDataObject_Collection* KeyboardMouseCategoryCollection = NewObject<UListDataObject_Collection>();
		KeyboardMouseCategoryCollection->SetDataID(FName("KeyboardMouseCategoryCollection"));
		KeyboardMouseCategoryCollection->SetDataDisplayName(LOCTEXT("KeyboardMouse_1e5788", "Keyboard & Mouse"));

		ControlTabCollection->AddChildListData(KeyboardMouseCategoryCollection);

		//Keyboard mouse inputs
		{
			FPlayerMappableKeyQueryOptions KeyboardMouseOnly;
			KeyboardMouseOnly.KeyToMatch = EKeys::S;
			KeyboardMouseOnly.bMatchBasicKeyTypes = true;

			/*FPlayerMappableKeyQueryOptions GamepadOnly;
			GamepadOnly.KeyToMatch = EKeys::Gamepad_FaceButton_Bottom;
			GamepadOnly.bMatchBasicKeyTypes = true;*/

			for (const TPair<FString, TObjectPtr<UEnhancedPlayerMappableKeyProfile>>& ProfilePair : EIUserSettings->GetAllAvailableKeyProfiles())
			{
				UEnhancedPlayerMappableKeyProfile* MappableKeyProfile = ProfilePair.Value;

				check(MappableKeyProfile);

				for (const TPair<FName, FKeyMappingRow>& MappingRowPair : MappableKeyProfile->GetPlayerMappingRows())
				{
					for (const FPlayerKeyMapping& KeyMapping : MappingRowPair.Value.Mappings)
					{
						
						if (MappableKeyProfile->DoesMappingPassQueryOptions(KeyMapping, KeyboardMouseOnly))
						{

							UListDataObject_KeyRemap* KeyRemapDataObject = NewObject<UListDataObject_KeyRemap>();
							KeyRemapDataObject->SetDataID(KeyMapping.GetMappingName());
							KeyRemapDataObject->SetDataDisplayName(KeyMapping.GetDisplayName());
							KeyRemapDataObject->InitKeyRemapData(EIUserSettings,MappableKeyProfile,ECommonInputType::MouseAndKeyboard,KeyMapping);

							KeyboardMouseCategoryCollection->AddChildListData(KeyRemapDataObject);
						}
					}
				}
			}
		}

	}

	//Gamepad Category
	{
		UListDataObject_Collection* GamepadCategoryCollection = NewObject<UListDataObject_Collection>();
		GamepadCategoryCollection->SetDataID(FName("GamepadCategoryCollection"));
		GamepadCategoryCollection->SetDataDisplayName(LOCTEXT("Gamepad_4dde00", "Gamepad"));

		ControlTabCollection->AddChildListData(GamepadCategoryCollection);

		//Gamepad inputs
		{
			FPlayerMappableKeyQueryOptions GamepadOnly;
			GamepadOnly.KeyToMatch = EKeys::Gamepad_FaceButton_Bottom;
			GamepadOnly.bMatchBasicKeyTypes = true;

			for (const TPair<FString, TObjectPtr<UEnhancedPlayerMappableKeyProfile>>& ProfilePair : EIUserSettings->GetAllAvailableKeyProfiles())
			{
				UEnhancedPlayerMappableKeyProfile* MappableKeyProfile = ProfilePair.Value;

				check(MappableKeyProfile);

				for (const TPair<FName, FKeyMappingRow>& MappingRowPair : MappableKeyProfile->GetPlayerMappingRows())
				{
					for (const FPlayerKeyMapping& KeyMapping : MappingRowPair.Value.Mappings)
					{
						if (MappableKeyProfile->DoesMappingPassQueryOptions(KeyMapping, GamepadOnly))
						{

							UListDataObject_KeyRemap* KeyRemapDataObject = NewObject<UListDataObject_KeyRemap>();
							KeyRemapDataObject->SetDataID(KeyMapping.GetMappingName());
							KeyRemapDataObject->SetDataDisplayName(KeyMapping.GetDisplayName());
							KeyRemapDataObject->InitKeyRemapData(EIUserSettings, MappableKeyProfile, ECommonInputType::Gamepad, KeyMapping);

							GamepadCategoryCollection->AddChildListData(KeyRemapDataObject);
						}
					}
				}
			}
		}


	}

	RegisteredOptionsTabCollections.Add(ControlTabCollection);
}

#undef LOCTEXT_NAMESPACE
