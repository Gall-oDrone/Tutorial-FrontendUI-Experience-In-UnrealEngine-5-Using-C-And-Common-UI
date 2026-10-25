// Diego Gallo All Rights Reserved


#include "FrontendSettings/FrontendGameUserSettings.h"

UFrontendGameUserSettings::UFrontendGameUserSettings()
	: OverallVolume(1.f)
	, MusicVolume(1.f)
	, SoundFXVolume(1.f)
	, bAllowBackgroundAudio(false)
	, bUseHDRAudioMode(false)
{
}

UFrontendGameUserSettings* UFrontendGameUserSettings::TryGet()
{
	return GEngine ? Cast<UFrontendGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

UFrontendGameUserSettings* UFrontendGameUserSettings::Get()
{
	UFrontendGameUserSettings* Settings = TryGet();

	// The most common integration mistake is forgetting the ini line (or not restarting
	// the editor after adding it). Say exactly that instead of a bare cast failure.
	checkf(Settings,
		TEXT("FrontendUI requires the game user settings class to be UFrontendGameUserSettings or a subclass. ")
		TEXT("Set it in Config/DefaultEngine.ini under [/Script/Engine.Engine]: ")
		TEXT("GameUserSettingsClassName=/Script/FrontendUI.FrontendGameUserSettings - then restart the editor. ")
		TEXT("Current class: %s"),
		GEngine && GEngine->GetGameUserSettings() ? *GEngine->GetGameUserSettings()->GetClass()->GetName() : TEXT("none"));

	return Settings;
}

void UFrontendGameUserSettings::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();

	// Runs on startup after LoadSettings() and whenever ApplySettings() is called, so a
	// game's ApplyAudioSettings() override sees the persisted volumes without extra wiring.
	ApplyAudioSettings();
}

void UFrontendGameUserSettings::SetCurrentGameDifficulty(const FString& InNewDifficulty)
{
	if (CurrentGameDifficulty == InNewDifficulty)
	{
		return;
	}

	CurrentGameDifficulty = InNewDifficulty;
	OnGameDifficultyChanged.Broadcast(CurrentGameDifficulty);
}

void UFrontendGameUserSettings::SetOverallVolume(float InVolume)
{
	OverallVolume = InVolume;
	ApplyAudioSettings();
}

void UFrontendGameUserSettings::SetMusicVolume(float InVolume)
{
	MusicVolume = InVolume;
	ApplyAudioSettings();
}

void UFrontendGameUserSettings::SetSoundFXVolume(float InVolume)
{
	SoundFXVolume = InVolume;
	ApplyAudioSettings();
}

void UFrontendGameUserSettings::SetAllowBackgroundAudio(bool bIsAllowed)
{
	bAllowBackgroundAudio = bIsAllowed;
	ApplyAudioSettings();
}

void UFrontendGameUserSettings::SetUseHDRAudioMode(bool bIsAllowed)
{
	bUseHDRAudioMode = bIsAllowed;
	ApplyAudioSettings();
}

float UFrontendGameUserSettings::GetCurrentDisplayGamma() const
{
	if (GEngine)
	{
		return GEngine->GetDisplayGamma();
	}

	return 0.0f;
}

void UFrontendGameUserSettings::SetCurrentDisplayGamma(float InNewGamma)
{
	if (GEngine)
	{
		GEngine->DisplayGamma = InNewGamma;
	}
}
