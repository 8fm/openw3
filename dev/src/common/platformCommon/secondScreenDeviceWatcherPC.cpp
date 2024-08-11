/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/redSystem/compilerExtensions.h"
#include "../../common/core/settings.h"

#if !defined( NO_SECOND_SCREEN )

#define WAINT_FOR_PING_TIME 15.0

#include "secondScreenDeviceWatcherPC.h"
#include "secondScreenDevicePC.h"

#include "../../common/core/jsonReader.h"
#include "../../common/core/jsonSimpleWriter.h"

static const Red::System::AnsiChar* COMMUNICATION_CHANNEL_NAME = "SecondScreenMock";

CSecondScreenDeviceWatcherPC::CSecondScreenDeviceWatcherPC( ISecondScreenDeviceDelegate* delegeate ) : CSecondScreenDeviceWatcher( delegeate )
{
	Red::Network::Manager::GetInstance()->RegisterListener( COMMUNICATION_CHANNEL_NAME, this );
}

CSecondScreenDeviceWatcherPC::~CSecondScreenDeviceWatcherPC() 
{
	Red::Network::Manager::GetInstance()->UnregisterListener( COMMUNICATION_CHANNEL_NAME, this );
}

void CSecondScreenDeviceWatcherPC::FindAllDevicesAsync()
{

}

void CSecondScreenDeviceWatcherPC::OnDeviceAddedPC( CSecondScreenDevicePC* device )
{
	if( m_delegate )
	{
		m_delegate->OnDeviceAdded( this, device );

		m_mutex.Acquire();
		m_lastResponsTimes[device->GetUuid()] = Red::System::Timer();
		m_mutex.Release();
	}
}

void CSecondScreenDeviceWatcherPC::OnDeviceRemovedPC( const CSecondScreenDevicePC& device )
{
	if( m_delegate )
	{
		m_delegate->OnDeviceRemoved( this, device );

		m_mutex.Acquire();
		THashMap<String, Red::System::Timer> ::iterator found = m_lastResponsTimes.Find( device.GetUuid() );
		if( found != m_lastResponsTimes.End() )
		{
			m_lastResponsTimes.Erase( found );
		}
		m_mutex.Release();
	}
}

void CSecondScreenDeviceWatcherPC::OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet )
{
	if( Red::System::StringCompare( channelName, COMMUNICATION_CHANNEL_NAME ) == 0 )
	{
		AnsiChar commandBuf[ SS_PC_MAX_MESSAGE_SIZE ];
		RED_VERIFY( packet.ReadString( commandBuf, ARRAY_COUNT_U32( commandBuf ) ) );
		const String& message = ANSI_TO_UNICODE( commandBuf );

		if( FilteringCommunicationMessages( message ) == false )
		{
			//! pass message to external system or drop it
		}
	}
}

Bool CSecondScreenDeviceWatcherPC::FilteringCommunicationMessages( const String& message )
{
	Bool communicationMessage = false;
	
	CJSONReaderUTF16 reader;

	if( reader.Read( message.AsChar() ) == true )
	{
		const CJSONDocumentUTF16& document = reader.GetDocument();
		if( IsConnectRequest( document ) == true ) 
		{			
			if( document.HasMember( TXT("uuid") ) == true )
			{
				CJSONValueRefUTF16 jsonUUID = document.GetMember( TXT("uuid") );
				CSecondScreenDevicePC* device = new CSecondScreenDevicePC( m_delegate, jsonUUID.GetString() );
				this->OnDeviceAddedPC( device );			

				// send answer
				SendAnswerOnConnectRequest( device->GetUuid() );
				communicationMessage = true;
			}
			
		}
		else if( IsPing( document ) == true ) 
		{
			
			if( document.HasMember( TXT("uuid") ) == true )
			{
				CJSONValueRefUTF16 jsonUUID = document.GetMember( TXT("uuid") );;
				m_mutex.Acquire();
				THashMap<String, Red::System::Timer>::iterator found = m_lastResponsTimes.Find( jsonUUID.GetString() );
				if( found != m_lastResponsTimes.End() )
				{
					found->m_second.Reset();
				}
				m_mutex.Release();

				SendAnswerOnPing( jsonUUID.GetString() );
				communicationMessage = true;
			}
		}
		else if( IsDisconnectRequest( document ) == true ) 
		{
			if( document.HasMember( TXT("uuid") ) == true )
			{
				CJSONValueRefUTF16 jsonUUID = document.GetMember( TXT("uuid") );
				this->OnDeviceRemovedPC( CSecondScreenDevicePC( m_delegate, jsonUUID.GetString() ) );

				// send answer
				SendAnswerOnDisconnectRequest( jsonUUID.GetString() );
				communicationMessage = true;
			}
		}
	}
	else
	{
		RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "PARSE ERROR" ) );
	}
	return communicationMessage;
}

Bool CSecondScreenDeviceWatcherPC::IsConnectRequest( const CJSONDocumentUTF16& document )
{
	if( document.HasMember( TXT("com_command") ) == true ) 
	{		
		CJSONValueRefUTF16 jsonCommand = document.GetMember( TXT("com_command") );
		String command = jsonCommand.GetString();
		if( command == TXT("connect") )
		{
			if( document.HasMember( TXT("uuid") ) == true ) 
			{
				return true;
			}
		}
	}
	return false;
}

void CSecondScreenDeviceWatcherPC::SendAnswerOnConnectRequest( const String& uuid )
{
	CJSONObjectUTF16 jsonRootObject;
	CJSONValueUTF16 jsonCommandStr(TXT("connected"), 9);

	jsonRootObject.AddMember( TXT("com_command"), jsonCommandStr );

	CJSONValueUTF16 jsonUuidStr( uuid.AsChar(), uuid.GetLength() );

	jsonRootObject.AddMember( TXT("uuid"), jsonUuidStr );

	CJSONSimpleWriterUTF16 writer;

	writer.WriteObject( jsonRootObject );

	String messageToSend = writer.GetContent();

	Red::Network::ChannelPacket packet( COMMUNICATION_CHANNEL_NAME );
	packet.WriteString( UNICODE_TO_ANSI( messageToSend.AsChar() ) );
	Red::Network::Manager::GetInstance()->Send( COMMUNICATION_CHANNEL_NAME, packet );
}

Bool CSecondScreenDeviceWatcherPC::IsDisconnectRequest( const CJSONDocumentUTF16& document )
{
	if( document.HasMember( TXT("com_command") ) == true ) 
	{
		CJSONValueRefUTF16 jsonCommand = document.GetMember( TXT("com_command") );
		String command = jsonCommand.GetString();
		if( command == TXT("disconnect") )
		{
			if( document.HasMember( TXT("uuid") ) == true ) 
			{
				return true;
			}
		}
	}
	return false;
}

void CSecondScreenDeviceWatcherPC::SendAnswerOnDisconnectRequest( const String& uuid )
{
	CJSONObjectUTF16 jsonRootObject;
	CJSONValueUTF16 jsonCommandStr( TXT("disconnected"), 12 );

	jsonRootObject.AddMember( TXT("com_command"), jsonCommandStr );

	CJSONValueUTF16 jsonUuidStr( uuid.AsChar(), uuid.GetLength() );

	jsonRootObject.AddMember( TXT("uuid"), jsonUuidStr );

	CJSONSimpleWriterUTF16 writer;

	writer.WriteObject( jsonRootObject );

	String messageToSend = writer.GetContent();

	Red::Network::ChannelPacket packet( COMMUNICATION_CHANNEL_NAME );
	packet.WriteString( UNICODE_TO_ANSI( messageToSend.AsChar() ) );
	Red::Network::Manager::GetInstance()->Send( COMMUNICATION_CHANNEL_NAME, packet );
}

Bool CSecondScreenDeviceWatcherPC::IsPing( const CJSONDocumentUTF16& document )
{
	if( document.HasMember( TXT("com_command") ) == true ) 
	{
		CJSONValueRefUTF16 jsonCommand = document.GetMember( TXT("com_command") );
		String command = jsonCommand.GetString();
		if( command == TXT("ping") )
		{
			if( document.HasMember( TXT("uuid") ) == true ) 
			{
				return true;
			}
		}
	}
	return false;
}

void CSecondScreenDeviceWatcherPC::SendAnswerOnPing( const String& uuid )
{
	CJSONObjectUTF16 jsonRootObject;
	CJSONValueUTF16 jsonCommandStr(TXT("pong"));

	jsonRootObject.AddMember( TXT("com_command"), jsonCommandStr );

	CJSONValueUTF16 jsonUuidStr( uuid.AsChar(), uuid.GetLength() );

	jsonRootObject.AddMember( TXT("uuid"), jsonUuidStr );

	CJSONSimpleWriterUTF16 writer;

	writer.WriteObject( jsonRootObject );

	String messageToSend = writer.GetContent();

	Red::Network::ChannelPacket packet( COMMUNICATION_CHANNEL_NAME );
	packet.WriteString( UNICODE_TO_ANSI( messageToSend.AsChar() ) );
	Red::Network::Manager::GetInstance()->Send( COMMUNICATION_CHANNEL_NAME, packet );
}

void CSecondScreenDeviceWatcherPC::Update( Float timeDelta )
{
	//! remove dead devices
	TDynArray<String> m_toRemove;
	
	m_mutex.Acquire();
	THashMap<String, Red::System::Timer>::iterator end = m_lastResponsTimes.End();
	for( THashMap<String, Red::System::Timer>::iterator iter = m_lastResponsTimes.Begin(); iter != end; ++iter )
	{
		Double lastPingDelta =  iter->m_second.GetSeconds();
		if( lastPingDelta > WAINT_FOR_PING_TIME )
		{
			m_toRemove.PushBack( iter->m_first );
		}
	}
	m_mutex.Release();

	TDynArray<String>::iterator endToRemove = m_toRemove.End();
	for( TDynArray<String>::iterator iter = m_toRemove.Begin(); iter != endToRemove; ++iter )
	{
		this->OnDeviceRemovedPC( CSecondScreenDevicePC( m_delegate, *iter ) );
	}
}

#endif


