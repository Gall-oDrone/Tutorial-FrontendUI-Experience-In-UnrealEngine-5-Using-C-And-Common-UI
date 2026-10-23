// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "FrontendGameUserSettings.generated.h"

/**
 * Settings the options screen reads and writes. Audio and difficulty are stored here
 * but intentionally not applied by the framework: a game subclasses this, points
 * GameUserSettingsClassName in DefaultEngine.ini at the subclass, and overrides
 * ApplyAudioSettings() / listens to OnGameDifficultyChanged to act on them.
 */
UCLASS()
class FRONTENDUI_API UFrontendGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
	
public:
	UFrontendGameUserSettings();

	static UFrontendGameUserSettings* Get();

	//~ Begin UGameUserSettings Interface
	virtual void ApplyNonResolutionSettings() override;
	//~ End UGameUserSettings Interface

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnGameDifficultyChanged, const FString& /*NewDifficulty*/);
	/** Fired after SetCurrentGameDifficulty stores a new value. The game decides what difficulty means. */
	FOnGameDifficultyChanged OnGameDifficultyChanged;

	//***** Gameplay Collection Tab ***** //
	UFUNCTION()
	FString GetCurrentGameDifficulty() const { return CurrentGameDifficulty; }

	UFUNCTION()
	void SetCurrentGameDifficulty(const FString& InNewDifficulty);
	//***** Gameplay Collection Tab ***** //

	//***** Audio Collection Tab ***** //
	UFUNCTION()
	float GetOverallVolume() const { return OverallVolume; }

	UFUNCTION()
	void SetOverallVolume(float InVolume);

	UFUNCTION()
	float GetMusicVolume() const { return MusicVolume; }

	UFUNCTION()
	void SetMusicVolume(float InVolume);
	
	UFUNCTION()
	float GetSoundFXVolume() const { return SoundFXVolume; }

	UFUNCTION()
	void SetSoundFXVolume(float InVolume);

	UFUNCTION()
	bool GetAllowBackgroundAudio() const { return bAllowBackgroundAudio; }
	
	UFUNCTION()
	void SetAllowBackgroundAudio(bool bIsAllowed);
	
	UFUNCTION()
	bool GetUseHDRAudioMode() const { return bUseHDRAudioMode; }
	
	UFUNCTION()
	void SetUseHDRAudioMode(bool bIsAllowed);
	//***** Audio Collection Tab ***** //
	
	//***** Video Collection Tab ***** //
	UFUNCTION()
	float GetCurrentDisplayGamma() const;

	UFUNCTION()
	void SetCurrentDisplayGamma(float InNewGamma);

	//***** Video Collection Tab ***** //

protected:
	/**
	 * Push the stored volume / audio flags into the game's audio system (sound classes,
	 * mixes, MetaSounds, Wwise...). Called after any audio setter and from
	 * ApplyNonResolutionSettings(), so it also runs on startup once settings are loaded.
	 * The framework does not know the game's audio setup, so the base implementation is empty.
	 */
	virtual void ApplyAudioSettings() {}

private:
	//***** Gameplay Collection Tab ***** //
	UPROPERTY(Config)
	FString CurrentGameDifficulty;
	//***** Gameplay Collection Tab ***** //

	//***** Audio Collection Tab ***** //
	UPROPERTY(Config)
	float OverallVolume;
	
	UPROPERTY(Config)
	float MusicVolume;
	
	UPROPERTY(Config)
	float SoundFXVolume;

	UPROPERTY(Config)
	bool bAllowBackgroundAudio;
	
	UPROPERTY(Config)
	bool bUseHDRAudioMode;
	//***** Audio Collection Tab ***** //
};