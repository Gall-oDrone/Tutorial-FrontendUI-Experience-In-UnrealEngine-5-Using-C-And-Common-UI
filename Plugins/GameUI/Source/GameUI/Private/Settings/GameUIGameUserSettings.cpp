// Diego Gallo All Rights Reserved


#include "Settings/GameUIGameUserSettings.h"

void UGameUIGameUserSettings::ApplyAudioSettings()
{
	Super::ApplyAudioSettings();

	// Called after any volume setter and on startup once settings are loaded.
	// Wire the stored values into the game's audio here, e.g. with Sound Classes:
	//
	//   UGameplayStatics::SetSoundMixClassOverride(this, MasterMix, MasterClass, GetOverallVolume(), 1.f, 0.f);
	//   UGameplayStatics::SetSoundMixClassOverride(this, MasterMix, MusicClass,  GetMusicVolume(),   1.f, 0.f);
	//   UGameplayStatics::SetSoundMixClassOverride(this, MasterMix, SFXClass,    GetSoundFXVolume(), 1.f, 0.f);
	//   UGameplayStatics::PushSoundMixModifier(this, MasterMix);
	//
	// The mix and classes would be soft references on a GameUI developer settings class
	// so this module never hard-references content.
}
