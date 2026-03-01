// Diego Gallo All Rights Reserved


#include "FrontendSettings/FrontendGameUserSettings.h"

UFrontendGameUserSettings::UFrontendGameUserSettings()
	: OverallVolume(1.f)
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

float UFrontendGameUserSettings::SetOverallVolume(float InVolume)
{
	OverallVolume = InVolume;

	//The actual logic for controlling the volume goes here
}
