/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#if !defined( NO_SECOND_SCREEN )

#include "../../common/core/jsonReader.h"

#include "../../common/platformCommon/secondScreenDeviceWatcher.h"
#include "../redNetwork/manager.h"
class CSecondScreenDevicePC;

//! CSecondScreenDeviceWatcherPC class serves as access point to second screen devices on PC platform
//! It also provide communication protocol between Game and Second Screen devices
class CSecondScreenDeviceWatcherPC: public CSecondScreenDeviceWatcher, public Red::Network::ChannelListener
{
public:
	CSecondScreenDeviceWatcherPC( ISecondScreenDeviceDelegate* delegeate );
	~CSecondScreenDeviceWatcherPC();

	void OnDeviceAddedPC( CSecondScreenDevicePC* device );
	void OnDeviceRemovedPC( const CSecondScreenDevicePC& device );

	//! CSecondScreenDeviceWatcher
public:
	virtual void FindAllDevicesAsync();
	virtual void Update( Float timeDelta );

	//! Red::Network::ChannelListener
public:
	virtual void OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet );

	//! functions used by communication protocol
private:
	//! return true if message is communication type 
	Bool FilteringCommunicationMessages( const String& message );

	//! return true when document contains this field: "com_command": "connected"
	Bool IsConnectRequest( const CJSONDocumentUTF16& document );
	void SendAnswerOnConnectRequest( const String& uuid );

	//! return true when document contains this field: "com_command": "disconnected"
	Bool IsDisconnectRequest( const CJSONDocumentUTF16& document );
	void SendAnswerOnDisconnectRequest( const String& uuid );

	//! return true when document contains this field: "com_command": "ping"
	Bool IsPing( const CJSONDocumentUTF16& document );
	void SendAnswerOnPing( const String& uuid );

private:
    THashMap<String, Red::System::Timer> m_lastResponsTimes;

	Red::Threads::CMutex				m_mutex;
};
#endif