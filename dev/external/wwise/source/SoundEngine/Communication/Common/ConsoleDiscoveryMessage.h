/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/


#pragma once

#include "CommunicationDefines.h"
#include "AkPrivateTypes.h"


class Serializer;

struct DiscoveryMessage
{
	enum Type
	{
		TypeDiscoveryInvalid = -1,
		TypeDiscoveryRequest = 0,
		TypeDiscoveryResponse,
		TypeDiscoveryChannelsInitRequest,
		TypeDiscoveryChannelsInitResponse
	};

	DiscoveryMessage( Type in_eType );
	virtual ~DiscoveryMessage() {}

	virtual bool Serialize( Serializer& in_rSerializer ) const;
	virtual bool Deserialize( Serializer& in_rSerializer );

	static Type PeekType( int in_nBufferSize, Serializer& in_rSerializer );

	AkUInt32 m_uiMsgLength;
	Type m_type;
};

struct DiscoveryRequest : public DiscoveryMessage
{
	DiscoveryRequest( AkUInt16 in_usDiscoveryResponsePort = 0 );

	virtual bool Serialize( Serializer& in_rSerializer ) const;
	virtual bool Deserialize( Serializer& in_rSerializer );


	// Port on the authoring app side where we can respond to this request
	AkUInt16 m_usDiscoveryResponsePort;

	DECLARE_BASECLASS( DiscoveryMessage );
};

struct DiscoveryResponse : public DiscoveryMessage
{
	enum ConsoleState
	{
		ConsoleStateUnknown = -1,
		ConsoleStateBusy = 0,
		ConsoleStateAvailable
	};

	DiscoveryResponse();

	virtual bool Serialize( Serializer& in_rSerializer ) const;
	virtual bool Deserialize( Serializer& in_rSerializer );

	AkUInt32 m_uiProtocolVersion;
	CommunicationDefines::ConsoleType m_eConsoleType;
	const char* m_pszConsoleName;
	ConsoleState m_eConsoleState;
	const char* m_pszControllerName;

	DECLARE_BASECLASS( DiscoveryMessage );
};


struct DiscoveryChannelsInitRequest : public DiscoveryMessage
{
	DiscoveryChannelsInitRequest( AkUInt16 in_usDiscoveryResponsePort = 0, const char* in_pszControllerName = NULL );

	virtual bool Serialize( Serializer& in_rSerializer ) const;
	virtual bool Deserialize( Serializer& in_rSerializer );

	// Port on the authoring app side where we can respond to this request
	AkUInt16 m_usDiscoveryResponsePort;

	// Name of the machine on which the authoring app is running
	const char* m_pszControllerName;

	DECLARE_BASECLASS( DiscoveryMessage );
};

// Initialize communication channels and return assigned port IDs
struct DiscoveryChannelsInitResponse : public DiscoveryMessage
{
	DiscoveryChannelsInitResponse();

	virtual bool Serialize( Serializer& in_rSerializer ) const;
	virtual bool Deserialize( Serializer& in_rSerializer );

	AkUInt16 m_usCommandPort;
	AkUInt16 m_usNotificationPort;

	DECLARE_BASECLASS( DiscoveryMessage );
};
