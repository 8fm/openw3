#pragma once
#if !defined( NO_SECOND_SCREEN )

#include "secondScreenDevice.h"
#include "../redNetwork/manager.h"
#include "../../common/core/jsonAllocator.h"

#define SS_PC_MAX_MESSAGE_SIZE  65532

//! This class serves as access point to second screen devices.
class CSecondScreenDevicePC: public CSecondScreenDevice, public Red::Network::ChannelListener
{
public:
	CSecondScreenDevicePC( ISecondScreenDeviceDelegate* _delegate, const String& uuid );
	virtual ~CSecondScreenDevicePC();

	const String& GetUuid() const { return m_uuid; }
	
	bool operator==( const CSecondScreenDevice& rhs );

	//! Red::Network::ChannelListener
public:
	virtual void OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet );

	//! CSecondScreenDevice
private:
	Bool SendMessageAsync( const Char* message ) const;

private:
	//! return true if message is communication type 
	Bool FilteringCommunicationMessages( const String& message );

private:
    String m_uuid; //! this variable is need to distinguish the devices
};
#endif