/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __DEBUG_DATA_SERVICE_LOG_OUTPUT_DEVICE_H__
#define __DEBUG_DATA_SERVICE_LOG_OUTPUT_DEVICE_H__

#include "../../common/redSystem/types.h"
#include "../../common/redSystem/log.h"

#include "r4DebugDataService.h"

#if !defined( NO_TELEMETRY )
#if !defined( NO_DEBUG_DATA_SERVICE )
class CR4DebugDataServiceLogOutputDevice : public Red::System::Log::OutputDevice
{
public:
	CR4DebugDataServiceLogOutputDevice();
	virtual ~CR4DebugDataServiceLogOutputDevice();

	RED_INLINE void Initialise( CR4DebugDataService* debugDataService)
	{
		m_debugDataService = debugDataService;
	}

private:
	virtual void Write( const Red::System::Log::Message& message );

private:
	CR4DebugDataService* m_debugDataService;
};

#endif // NO_DEBUG_DATA_SERVICE
#endif // NO_TELEMETRY

#endif // __DEBUG_DATA_SERVICE_LOG_OUTPUT_DEVICE_H__
