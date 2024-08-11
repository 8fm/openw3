/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../../common/redSystem/compilerExtensions.h"
#include "../../common/core/settings.h"

#if !defined( NO_SECOND_SCREEN )
#include "secondScreenDevicePC.h"
#include "../../common/redSystem/log.h"
#include "../../common/core/jsonReader.h"

const size_t CSecondScreenDevice::s_maxMessageLength = SS_PC_MAX_MESSAGE_SIZE;

static const Red::System::AnsiChar* COMMUNICATION_CHANNEL_NAME = "SecondScreenMock";

CSecondScreenDevicePC::CSecondScreenDevicePC( ISecondScreenDeviceDelegate* _delegate, const String& uuid ): CSecondScreenDevice( _delegate )
{
	m_uuid = uuid;
	Red::Network::Manager::GetInstance()->RegisterListener( COMMUNICATION_CHANNEL_NAME, this );
}

CSecondScreenDevicePC::~CSecondScreenDevicePC()
{
	Red::Network::Manager::GetInstance()->UnregisterListener( COMMUNICATION_CHANNEL_NAME, this );
}

Bool CSecondScreenDevicePC::SendMessageAsync( const Char* message ) const
{
	Red::Network::ChannelPacket packet( COMMUNICATION_CHANNEL_NAME );
	packet.WriteString( UNICODE_TO_ANSI( message ) );
	Red::Network::Manager::GetInstance()->Send( COMMUNICATION_CHANNEL_NAME, packet );
	return true;
}

bool CSecondScreenDevicePC::operator==( const CSecondScreenDevice& rhs )
{
	return (this->m_uuid == ((const CSecondScreenDevicePC*)(&rhs))->m_uuid);
}

void CSecondScreenDevicePC::OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet )
{
	AnsiChar commandBuf[ SS_PC_MAX_MESSAGE_SIZE ];

	RED_VERIFY( packet.ReadString( commandBuf, ARRAY_COUNT_U32( commandBuf ) ) );

	const String& message = ANSI_TO_UNICODE( commandBuf );

	//! return true if message is communication type 
	if( FilteringCommunicationMessages( message ) == false )
	{
		if( m_delegate )
		{
			m_delegate->OnMessageReceived( *this, message.AsChar() );
		}
	}
}

Bool CSecondScreenDevicePC::FilteringCommunicationMessages( const String& message )
{
	Bool communicationMessage = false;
	CJSONReaderUTF16 reader;

	if( reader.Read( message.AsChar() ) == true )
	{
		if( reader.GetDocument().HasMember( TXT("com_command") ) == true ) 
		{
			communicationMessage = true;
		}
	}
	return communicationMessage;
}

#endif

