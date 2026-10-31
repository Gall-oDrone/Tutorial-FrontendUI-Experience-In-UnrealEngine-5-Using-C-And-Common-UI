// Diego Gallo All Rights Reserved


#include "Controllers/FrontendPlayerController.h"
#include "FrontendUILog.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraActor.h"
#include "FrontendSettings/FrontendGameUserSettings.h"

void AFrontendPlayerController::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);

	TArray<AActor*> FoundCameras;
	UGameplayStatics::GetAllActorsOfClassWithTag(this, ACameraActor::StaticClass(), FName("Default"), FoundCameras);

	if (!FoundCameras.IsEmpty())
	{
		SetViewTarget(FoundCameras[0]);
	}

	// Benchmark once on first run. Use TryGet so a host that has not yet configured
	// GameUserSettingsClassName sees a readable error and still reaches the first screen,
	// instead of crashing on possession before any UI is visible.
	UFrontendGameUserSettings* GameUserSettings = UFrontendGameUserSettings::TryGet();
	if (!GameUserSettings)
	{
		UE_LOG(LogFrontendUI, Error, TEXT("GameUserSettingsClassName is not UFrontendGameUserSettings (or a subclass). ")
			TEXT("Set it in Config/DefaultEngine.ini under [/Script/Engine.Engine] and restart the editor. Skipping hardware benchmark."));
		return;
	}

	if (GameUserSettings->GetLastCPUBenchmarkResult() == -1.f || GameUserSettings->GetLastGPUBenchmarkResult() == -1.f)
	{
		GameUserSettings->RunHardwareBenchmark();
		GameUserSettings->ApplyHardwareBenchmarkResults();
	}
}
