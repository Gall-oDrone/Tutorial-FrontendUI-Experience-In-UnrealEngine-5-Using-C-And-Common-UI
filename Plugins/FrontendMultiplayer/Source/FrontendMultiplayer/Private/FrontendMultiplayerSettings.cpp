// Diego Gallo All Rights Reserved


#include "FrontendMultiplayerSettings.h"
#include "Backends/FrontendSessionBackend_Stub.h"

UFrontendMultiplayerSettings::UFrontendMultiplayerSettings()
{
	SessionBackendClass = UFrontendSessionBackend_Stub::StaticClass();
}
