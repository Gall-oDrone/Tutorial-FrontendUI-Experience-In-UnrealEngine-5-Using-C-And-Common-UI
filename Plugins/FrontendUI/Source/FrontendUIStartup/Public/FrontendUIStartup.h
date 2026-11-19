// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"

/**
 * Startup logo: an engine-loading pre-load screen (FPreLoadScreenBase) that shows one image,
 * centred on a plain background, from the moment the engine can draw until it has finished
 * loading - and for a minimum time, so it is never a flash. It runs before any UObject of the
 * game exists, so the image is a plain file under Content/ and the settings are read straight
 * from DefaultGame.ini:
 *
 *   [FrontendUI.StartupLogo]
 *   bEnabled=True
 *   ImagePath=Splash/T_StudioLogo.png          ; relative to the project's Content directory
 *   MinimumDisplaySeconds=2.5
 *   LogoScreenFraction=0.6                      ; share of the window's limiting side
 *   BackgroundColor=(R=0.0,G=0.0,B=0.0,A=1.0)
 *   bPlayStartupMoviesAfterLogo=True            ; False: no startup movies at all
 *
 * The engine shows either such a screen or its startup movies while it loads, not both. The
 * module therefore holds the startup movies back and plays them when the logo is done, in the
 * first frame after the engine's init; the frontend appears after them, as it does without
 * the logo.
 *
 * Only in game builds that render: not in the editor, not on a dedicated server, not with
 * -NoLoadingScreen and not on Linux (the engine disables pre-load screens there). In those
 * cases nothing is touched and the startup movies play the engine's own way.
 */
namespace FrontendUIStartup
{
	/**
	 * True from the moment the startup logo is on screen in this run, which is before the
	 * first map loads. The frontend uses it to skip its own logo screen, which would show the
	 * same logo a second time. Game thread.
	 */
	FRONTENDUISTARTUP_API bool WasStudioLogoShownAtStartup();
}
