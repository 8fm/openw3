/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"

#include "r4DebugDataServiceLogOutputDevice.h"

#if !defined( NO_TELEMETRY )
#if !defined( NO_DEBUG_DATA_SERVICE )

CR4DebugDataServiceLogOutputDevice::CR4DebugDataServiceLogOutputDevice()
	:	m_debugDataService( NULL )
{
	SetUnsafeToCallOnCrash();
}

CR4DebugDataServiceLogOutputDevice::~CR4DebugDataServiceLogOutputDevice()
{

}

void CR4DebugDataServiceLogOutputDevice::Write( const Red::System::Log::Message& message )
{
	if( m_debugDataService  )
	{
		m_debugDataService->Log( message.channelText, message.text );
	}
}

#endif //NO_DEBUG_DATA_SERVICE
#endif //NO_TELEMETRY
