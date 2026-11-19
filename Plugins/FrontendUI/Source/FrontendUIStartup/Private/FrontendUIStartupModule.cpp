// Diego Gallo All Rights Reserved

#include "FrontendUIStartup.h"

#include "Brushes/SlateDynamicImageBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreMisc.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#if !UE_SERVER
#include "MoviePlayer.h"
#endif
#include "PreLoadScreenBase.h"
#include "PreLoadScreenManager.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SCompoundWidget.h"

#include <atomic>

DEFINE_LOG_CATEGORY_STATIC(LogFrontendUIStartup, Log, All);

namespace FrontendUIStartupPrivate
{
	/**
	 * Set the moment the logo goes on screen - before the first map loads, so the frontend
	 * already has the answer when it decides about its own logo screen. Game thread.
	 */
	bool bStudioLogoShownAtStartup = false;

	const TCHAR* const ConfigSection = TEXT("FrontendUI.StartupLogo");

	/**
	 * [FrontendUI.StartupLogo] in DefaultGame.ini. Read with GConfig, not through a settings
	 * object: this runs in the PreEarlyLoadingScreen phase, before the class defaults of game
	 * modules exist.
	 */
	struct FStartupLogoConfig
	{
		bool bEnabled = true;
		FString ImagePath = TEXT("Splash/T_StudioLogo.png");
		float MinimumDisplaySeconds = 2.5f;
		float LogoScreenFraction = 0.6f;
		FLinearColor BackgroundColor = FLinearColor::Black;
		bool bPlayStartupMoviesAfterLogo = true;

		static FStartupLogoConfig Load()
		{
			FStartupLogoConfig Config;
			if (GConfig)
			{
				GConfig->GetBool(ConfigSection, TEXT("bEnabled"), Config.bEnabled, GGameIni);
				GConfig->GetString(ConfigSection, TEXT("ImagePath"), Config.ImagePath, GGameIni);
				GConfig->GetFloat(ConfigSection, TEXT("MinimumDisplaySeconds"), Config.MinimumDisplaySeconds, GGameIni);
				GConfig->GetFloat(ConfigSection, TEXT("LogoScreenFraction"), Config.LogoScreenFraction, GGameIni);
				GConfig->GetBool(ConfigSection, TEXT("bPlayStartupMoviesAfterLogo"), Config.bPlayStartupMoviesAfterLogo, GGameIni);

				FString ColorString;
				if (GConfig->GetString(ConfigSection, TEXT("BackgroundColor"), ColorString, GGameIni) && !ColorString.IsEmpty())
				{
					Config.BackgroundColor.InitFromString(ColorString);
				}
			}

			Config.MinimumDisplaySeconds = FMath::Clamp(Config.MinimumDisplaySeconds, 0.f, 30.f);
			Config.LogoScreenFraction = FMath::Clamp(Config.LogoScreenFraction, 0.05f, 1.f);
			return Config;
		}

		/** The image file as the file system (loose files or the pak) knows it; empty if not configured. */
		FString ResolveImagePath() const
		{
			if (ImagePath.IsEmpty())
			{
				return FString();
			}
			return FPaths::IsRelative(ImagePath) ? FPaths::Combine(FPaths::ProjectContentDir(), ImagePath) : ImagePath;
		}
	};

	/** The logo, as large as LogoScreenFraction of the window allows, centred on a plain background. */
	class SStartupLogoWidget : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SStartupLogoWidget)
			: _LogoBrush(nullptr)
			, _LogoSize(FVector2D(1.0, 1.0))
			, _LogoScreenFraction(0.6f)
			, _BackgroundColor(FLinearColor::Black)
			{}
			SLATE_ARGUMENT(const FSlateBrush*, LogoBrush)
			SLATE_ARGUMENT(FVector2D, LogoSize)
			SLATE_ARGUMENT(float, LogoScreenFraction)
			SLATE_ARGUMENT(FLinearColor, BackgroundColor)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			const float LogoWidth = FMath::Max(1.f, static_cast<float>(InArgs._LogoSize.X));
			const float LogoHeight = FMath::Max(1.f, static_cast<float>(InArgs._LogoSize.Y));
			const float Fraction = FMath::Clamp(InArgs._LogoScreenFraction, 0.05f, 1.f);

			// No window size is needed: the logo sits in a box that is 1 / Fraction times its
			// size, and that box is scaled as a whole to fit the window. The logo then covers
			// Fraction of whichever side of the window limits it, at any resolution.
			ChildSlot
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(InArgs._BackgroundColor)
				.Padding(0.f)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFit)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(LogoWidth / Fraction)
						.HeightOverride(LogoHeight / Fraction)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SBox)
							.WidthOverride(LogoWidth)
							.HeightOverride(LogoHeight)
							[
								SNew(SImage)
								.Image(InArgs._LogoBrush)
							]
						]
					]
				]
			];
		}
	};

	/**
	 * Engine-loading pre-load screen: on screen from the moment the engine plays it (after the
	 * PreLoadingScreen modules, once the rendering thread runs - the same point in editor
	 * binaries run with -game and in packaged builds) until the engine has finished loading,
	 * including the first map, and the minimum display time has passed. The engine blocks on
	 * IsDone() at the end of its init, which is what makes the minimum hold.
	 */
	class FStartupLogoScreen : public FPreLoadScreenBase
	{
	public:
		FStartupLogoScreen(const FString& InImagePath, const FStartupLogoConfig& InConfig)
			: ImagePath(InImagePath)
			, Config(InConfig)
		{
		}

		//~ Begin IPreLoadScreen Interface
		virtual void OnPlay(TWeakPtr<SWindow> TargetWindow) override
		{
			FPreLoadScreenBase::OnPlay(TargetWindow);

			// The texture is made here, on the game thread, rather than when the module starts:
			// by the time the engine plays this screen the RHI is fully initialised and the
			// rendering thread runs, in editor binaries (-game) and packaged builds alike.
			const FName BrushName(*ImagePath);
			FIntPoint ImageSize(0, 0);
			if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
			{
				ImageSize = FSlateApplication::Get().GetRenderer()->GenerateDynamicImageResource(BrushName);
			}

			if (ImageSize.X <= 0 || ImageSize.Y <= 0)
			{
				UE_LOG(LogFrontendUIStartup, Warning, TEXT("startup logo: '%s' could not be loaded as an image (PNG expected); nothing is shown."), *ImagePath);
				return;
			}

			const FVector2D LogoSize(static_cast<double>(ImageSize.X), static_cast<double>(ImageSize.Y));
			LogoBrush = MakeShareable(new FSlateDynamicImageBrush(BrushName, LogoSize));
			LogoWidget = SNew(SStartupLogoWidget)
				.LogoBrush(LogoBrush.Get())
				.LogoSize(LogoSize)
				.LogoScreenFraction(Config.LogoScreenFraction)
				.BackgroundColor(Config.BackgroundColor);

			PlayStartSeconds.store(FPlatformTime::Seconds());
			bIsShowingLogo.store(true);
			bStudioLogoShownAtStartup = true;

			UE_LOG(LogFrontendUIStartup, Log, TEXT("startup logo: showing '%s' (%dx%d) for at least %.1fs while the engine loads."),
				*ImagePath, ImageSize.X, ImageSize.Y, Config.MinimumDisplaySeconds);
		}

		virtual bool IsDone() const override
		{
			// Asked from the game thread and the render thread. The base answers "engine loading
			// finished"; on top of that the logo stays for its minimum time.
			if (!FPreLoadScreenBase::IsDone())
			{
				return false;
			}
			if (!bIsShowingLogo.load())
			{
				return true;
			}
			return (FPlatformTime::Seconds() - PlayStartSeconds.load()) >= static_cast<double>(Config.MinimumDisplaySeconds);
		}

		virtual void OnStop() override
		{
			if (bIsShowingLogo.load())
			{
				UE_LOG(LogFrontendUIStartup, Log, TEXT("startup logo: done after %.1fs."), FPlatformTime::Seconds() - PlayStartSeconds.load());
			}
			FPreLoadScreenBase::OnStop();
		}

		virtual void CleanUp() override
		{
			// Game thread, from the pre-load screen manager's teardown. The widget goes first:
			// it points at the brush. Releasing the brush releases the texture.
			LogoWidget.Reset();
			LogoBrush.Reset();
			FPreLoadScreenBase::CleanUp();
		}

		// A little sleep per tick while the engine waits for the minimum time, instead of spinning.
		virtual float GetAddedTickDelay() override { return 0.01f; }

		virtual EPreLoadScreenTypes GetPreLoadScreenType() const override { return EPreLoadScreenTypes::EngineLoadingScreen; }

		virtual TSharedPtr<SWidget> GetWidget() override { return LogoWidget; }
		virtual const TSharedPtr<const SWidget> GetWidget() const override { return LogoWidget; }
		//~ End IPreLoadScreen Interface

	private:
		FString ImagePath;
		FStartupLogoConfig Config;

		TSharedPtr<FSlateDynamicImageBrush> LogoBrush;
		TSharedPtr<SWidget> LogoWidget;

		std::atomic<double> PlayStartSeconds{ 0.0 };
		std::atomic<bool> bIsShowingLogo{ false };
	};
}

bool FrontendUIStartup::WasStudioLogoShownAtStartup()
{
	return FrontendUIStartupPrivate::bStudioLogoShownAtStartup;
}

class FFrontendUIStartupModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		using namespace FrontendUIStartupPrivate;

		// Pre-load screens are a game-build, rendering-only affair. The manager does not exist in
		// the editor, on a dedicated server, in commandlets or with -NoLoadingScreen.
		if (GIsEditor || IsRunningDedicatedServer() || IsRunningCommandlet() || !FApp::CanEverRender())
		{
			return;
		}

		FPreLoadScreenManager* PreLoadScreenManager = FPreLoadScreenManager::Get();
		if (!PreLoadScreenManager)
		{
			return;
		}

		Config = FStartupLogoConfig::Load();
		if (!Config.bEnabled)
		{
			UE_LOG(LogFrontendUIStartup, Log, TEXT("startup logo: disabled ([%s] bEnabled=False)."), ConfigSection);
			return;
		}

		const FString ImagePath = Config.ResolveImagePath();
		if (ImagePath.IsEmpty() || !FPaths::FileExists(ImagePath))
		{
			UE_LOG(LogFrontendUIStartup, Warning, TEXT("startup logo: image '%s' not found ([%s] ImagePath, relative to Content/); nothing is registered."), *ImagePath, ConfigSection);
			return;
		}

		LogoScreen = MakeShared<FStartupLogoScreen>(ImagePath, Config);
		PreLoadScreenManager->RegisterPreLoadScreen(LogoScreen);
		CleanUpHandle = PreLoadScreenManager->OnPreLoadScreenManagerCleanUp.AddRaw(this, &FFrontendUIStartupModule::HandlePreLoadScreenManagerCleanUp);

#if !UE_SERVER
		// The engine shows either an engine-loading pre-load screen or its startup movies
		// ([/Script/MoviePlayer.MoviePlayerSettings] StartupMovies), never one after the other.
		// It has just read the movies from the ini - this module is loaded right after that, in
		// the PreEarlyLoadingScreen phase - and a packaged build would now initialise the movie
		// player for them as "early startup movies", which makes the movie start on top of the
		// logo when the first map loads. So take them away from the movie player here, in every
		// kind of build, and hand them back once the logo is done (HandlePreLoadScreenManagerCleanUp).
		if (IsMoviePlayerEnabled())
		{
			if (IGameMoviePlayer* MoviePlayer = GetMoviePlayer())
			{
				MoviePlayer->SetupLoadingScreen(FLoadingScreenAttributes());
				bStartupMoviesDeferred = true;
			}
		}
#endif

		UE_LOG(LogFrontendUIStartup, Log, TEXT("startup logo: registered '%s'%s."), *ImagePath,
			bStartupMoviesDeferred ? TEXT("; the startup movies wait until it is done") : TEXT(""));
	}

	virtual void ShutdownModule() override
	{
		if (CleanUpHandle.IsValid())
		{
			if (FPreLoadScreenManager* PreLoadScreenManager = FPreLoadScreenManager::Get())
			{
				PreLoadScreenManager->OnPreLoadScreenManagerCleanUp.Remove(CleanUpHandle);
			}
			CleanUpHandle.Reset();
		}
		LogoScreen.Reset();
	}

private:
	/**
	 * The engine is through with pre-load screens: in the first frame after its init, right
	 * after it has initialised the movie player for the game window - or while it shuts down,
	 * if it never got that far. The manager has already called CleanUp() on the screen.
	 */
	void HandlePreLoadScreenManagerCleanUp()
	{
		CleanUpHandle.Reset();
		LogoScreen.Reset();

#if !UE_SERVER
		if (!bStartupMoviesDeferred)
		{
			return;
		}
		bStartupMoviesDeferred = false;

		if (IsEngineExitRequested() || !IsMoviePlayerEnabled())
		{
			return;
		}

		if (!Config.bPlayStartupMoviesAfterLogo)
		{
			UE_LOG(LogFrontendUIStartup, Log, TEXT("startup logo: startup movies dropped (bPlayStartupMoviesAfterLogo=False)."));
			return;
		}

		IGameMoviePlayer* MoviePlayer = GetMoviePlayer();
		if (!MoviePlayer || !MoviePlayer->IsInitialized() || MoviePlayer->IsMovieCurrentlyPlaying() || MoviePlayer->LoadingScreenIsPrepared())
		{
			// Not ready for them, or something else has set up a loading screen of its own in
			// the meantime: leave the movie player alone.
			UE_LOG(LogFrontendUIStartup, Log, TEXT("startup logo: the movie player is not free; startup movies skipped."));
			return;
		}

		// Hand the startup movies back (the attributes are empty, so this reads the ini again)
		// and play them now. This blocks, the way the engine's own wait at the end of its init
		// does when there is no logo.
		MoviePlayer->SetupLoadingScreenFromIni();
		if (MoviePlayer->LoadingScreenIsPrepared() && MoviePlayer->PlayMovie())
		{
			UE_LOG(LogFrontendUIStartup, Log, TEXT("startup logo: playing the startup movies now."));
			MoviePlayer->WaitForMovieToFinish(false);
		}
		else
		{
			// Nothing started (no movie file, no decoder). Left queued, the movies would start
			// at the next map load instead: in the middle of the first Host or Join.
			UE_LOG(LogFrontendUIStartup, Log, TEXT("startup logo: no startup movie could be played."));
			MoviePlayer->SetupLoadingScreen(FLoadingScreenAttributes());
		}
#endif
	}

	FrontendUIStartupPrivate::FStartupLogoConfig Config;
	TSharedPtr<FrontendUIStartupPrivate::FStartupLogoScreen> LogoScreen;
	FDelegateHandle CleanUpHandle;

	/** True between taking the startup movies away from the movie player and handing them back. */
	bool bStartupMoviesDeferred = false;
};

IMPLEMENT_MODULE(FFrontendUIStartupModule, FrontendUIStartup)
