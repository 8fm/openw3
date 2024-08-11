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


#include "stdafx.h"

#include "ConsoleDiscoveryMessage.h"
#include "Serializer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

DiscoveryMessage::DiscoveryMessage( Type in_eType )
	: m_type( in_eType )
{
}

bool DiscoveryMessage::Serialize( Serializer& in_rSerializer ) const
{
	return in_rSerializer.Put( m_uiMsgLength )
		&& in_rSerializer.Put( (AkInt32)m_type );
}

bool DiscoveryMessage::Deserialize( Serializer& in_rSerializer )
{
	return in_rSerializer.Get( m_uiMsgLength )
		&& in_rSerializer.Get( (AkInt32&)m_type );
}

DiscoveryMessage::Type DiscoveryMessage::PeekType( int in_nBufferSize, Serializer& in_rSerializer )
{
	static const int minSize = sizeof(AkUInt32) // skip m_uiMsgLength
	                         + sizeof(AkInt32); // type itself

	if ( in_nBufferSize < minSize )
		return TypeDiscoveryInvalid;

	// This will ensure that the current position is restored when we're done
	Serializer::AutoSetDataPeeking peek( in_rSerializer );

	// Skip msg length
	AkUInt32 uiMsgLength;
	in_rSerializer.Get( uiMsgLength );

	// Read type
	Type eType;
	in_rSerializer.Get( (AkInt32&)eType );

	return eType;
}

DiscoveryRequest::DiscoveryRequest( AkUInt16 in_usDiscoveryResponsePort )
	: DiscoveryMessage( TypeDiscoveryRequest )
	, m_usDiscoveryResponsePort( in_usDiscoveryResponsePort )
{
}

bool DiscoveryRequest::Serialize( Serializer& in_rSerializer ) const
{
	return __base::Serialize( in_rSerializer )
		&& in_rSerializer.Put( m_usDiscoveryResponsePort );
}

bool DiscoveryRequest::Deserialize( Serializer& in_rSerializer )
{
	return __base::Deserialize( in_rSerializer )
		&& in_rSerializer.Get( m_usDiscoveryResponsePort );
}

DiscoveryResponse::DiscoveryResponse()
	: DiscoveryMessage( TypeDiscoveryResponse )
	, m_uiProtocolVersion( 0 )
	, m_eConsoleType( CommunicationDefines::ConsoleUnknown )
	, m_pszConsoleName( NULL )
	, m_eConsoleState( ConsoleStateUnknown )
	, m_pszControllerName( NULL )

{
}

bool DiscoveryResponse::Serialize( Serializer& in_rSerializer ) const
{
	return __base::Serialize( in_rSerializer )
		&& in_rSerializer.Put( m_uiProtocolVersion )
		&& in_rSerializer.Put( (AkInt32)m_eConsoleType )
		&& in_rSerializer.Put( m_pszConsoleName )
		&& in_rSerializer.Put( (AkInt32)m_eConsoleState )
		&& in_rSerializer.Put( m_pszControllerName );
}

bool DiscoveryResponse::Deserialize( Serializer& in_rSerializer )
{
	AkInt32 iRead = 0;

	return __base::Deserialize( in_rSerializer )
		&& in_rSerializer.Get( m_uiProtocolVersion )
		&& in_rSerializer.Get( (AkInt32&)m_eConsoleType )
		&& in_rSerializer.Get( (char*&)m_pszConsoleName, iRead )
		&& in_rSerializer.Get( (AkInt32&)m_eConsoleState )
		&& in_rSerializer.Get( (char*&)m_pszControllerName, iRead );
}



DiscoveryChannelsInitRequest::DiscoveryChannelsInitRequest(
	AkUInt16 in_usDiscoveryResponsePort,
	const char* in_pszControllerName
)
	: DiscoveryMessage( TypeDiscoveryChannelsInitRequest )
	, m_usDiscoveryResponsePort( in_usDiscoveryResponsePort )
	, m_pszControllerName( in_pszControllerName )
{
}

bool DiscoveryChannelsInitRequest::Serialize( Serializer& in_rSerializer ) const
{
	return __base::Serialize( in_rSerializer )
		&& in_rSerializer.Put( m_usDiscoveryResponsePort )
		&& in_rSerializer.Put( m_pszControllerName );
}

bool DiscoveryChannelsInitRequest::Deserialize( Serializer& in_rSerializer )
{
	AkInt32 iRead = 0;
	return __base::Deserialize( in_rSerializer )
		&& in_rSerializer.Get( m_usDiscoveryResponsePort )
		&& in_rSerializer.Get( (char*&)m_pszControllerName, iRead );
}



DiscoveryChannelsInitResponse::DiscoveryChannelsInitResponse()
	: DiscoveryMessage( TypeDiscoveryChannelsInitResponse )
	, m_usCommandPort( 0 )
	, m_usNotificationPort( 0 )
{
}

bool DiscoveryChannelsInitResponse::Serialize( Serializer& in_rSerializer ) const
{
	return __base::Serialize( in_rSerializer )
		&& in_rSerializer.Put( m_usCommandPort )
		&& in_rSerializer.Put( m_usNotificationPort );
}

bool DiscoveryChannelsInitResponse::Deserialize( Serializer& in_rSerializer )
{
	return __base::Deserialize( in_rSerializer )
		&& in_rSerializer.Get( m_usCommandPort )
		&& in_rSerializer.Get( m_usNotificationPort );
}
