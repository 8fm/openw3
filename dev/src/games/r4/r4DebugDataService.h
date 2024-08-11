#pragma once

#include "../../common/core/redTelemetryService.h"

#if !defined( NO_TELEMETRY )
#if !defined( NO_DEBUG_DATA_SERVICE )

//! This class serves as access point for Debug Data Service interface.
//! It spawns jobs to update CR4DebugDataService in background
class CR4DebugDataService: public CRedTelemetryService
{
public:
	CR4DebugDataService();
	~CR4DebugDataService();

	void Log( const Char* channel, const Char* stringToSend );
};

typedef TSingleton< CR4DebugDataService, TDefaultLifetime, TCreateUsingNew > SDebugDataService;

#endif //NO_DEBUG_DATA_SERVICE
#endif //NO_TELEMETRY
