#include "build.h"

#include "r4DebugDataService.h"
#include "../../common/core/redTelemetryServicesManager.h"

#if !defined( NO_TELEMETRY )
#if !defined( NO_DEBUG_DATA_SERVICE )

//////////////////////////////////////////////////////////////////////////
//! This class is a singleton, so its creation is thread-safe
CR4DebugDataService::CR4DebugDataService() : CRedTelemetryService( TXT("debug_data_service"), Telemetry::BT_DD_SERVICE_API )
{

}

//////////////////////////////////////////////////////////////////////////
CR4DebugDataService::~CR4DebugDataService()
{

}

//////////////////////////////////////////////////////////////////////////
void CR4DebugDataService::Log( const Char* channel, const Char* stringToSend )
{
	IRedTelemetryServiceInterface* telemetryService = SRedTelemetryServicesManager::GetInstance().GetService( TXT("telemetry") );
	if( telemetryService != NULL )
	{
		String sessionId = telemetryService->GetSessionId( Telemetry::BT_RED_TEL_API );
		if( sessionId.Empty() == FALSE )
		{
			GetInterface()->SetExternalSessionId( Telemetry::BT_DD_SERVICE_API, sessionId );
		}
	}
	
	GetInterface()->LogV_WS( channel, TXT("LOG"), stringToSend );
}

#endif //NO_DEBUG_DATA_SERVICE
#endif //NO_TELEMETRY