/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __NETWORKED_LOG_OUTPUT_DEVICE_H__
#define __NETWORKED_LOG_OUTPUT_DEVICE_H__

#ifdef RED_NETWORK_ENABLED

#include "../redSystem/types.h"
#include "../redSystem/log.h"
#include "../redNetwork/manager.h"

class CNetworkedLogOutputDevice : public Red::System::Log::OutputDevice
{
public:
	CNetworkedLogOutputDevice();
	virtual ~CNetworkedLogOutputDevice();

	RED_INLINE void Initialise( Red::Network::Manager* networkSystem )
	{
		m_networkSystem = networkSystem;

		if( m_networkSystem )
		{
			m_networkSystem->CreateChannel( "log" );
		}
	}

private:
	virtual void Write( const Red::System::Log::Message& message );

private:
	Red::Network::Manager* m_networkSystem;
};

#endif // RED_NETWORK_ENABLED

#endif // __NETWORKED_LOG_OUTPUT_DEVICE_H__
