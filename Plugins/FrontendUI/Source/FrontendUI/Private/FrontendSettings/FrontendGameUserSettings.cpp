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

UFrontendGameUserSettings* UFrontendGameUserSettings::Get()
{
	if (GEngine)
	{
		return CastChecked<UFrontendGameUserSettings>(GEngine->GetGameUserSettings());
	}

	return nullptr;
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
