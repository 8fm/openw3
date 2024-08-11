/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#if !defined( NO_SECOND_SCREEN )

#include "secondScreenManager.h"
#include "secondScreenDevice.h"
#include "secondScreenDeviceWatcher.h"

#ifdef RED_PLATFORM_DURANGO
	#include "../../durango/platformDurango/secondScreenDeviceWatcherDurango.h"
#endif // RED_PLATFORM_DURANGO
#if defined( RED_PLATFORM_WIN64 ) && !defined( RED_FINAL_BUILD )
	#include "secondScreenDeviceWatcherPC.h"
#endif

#include "../../common/core/jsonReader.h"
#include "../../common/core/jsonSimpleWriter.h"
#include "../../common/core/jsonValue.h"
#include "../../common/engine/localizationManager.h"

static const size_t guildStringBufferSize = 36;

const Uint32 CSecondScreenManager::GAME_VERSION = 1;

void CSecondScreenManager::SendMessages( CSecondScreenDevice* device, TDynArray<String> messages )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendMessages );
#endif
	TDynArray< String >::const_iterator iter = messages.Begin();

	while( iter != messages.End() )
	{
		device->SendMessage( iter->AsChar() );
		++iter;
	}
}

String CSecondScreenManager::JsonValueToMessage( CJSONObjectUTF16& jsonObject ) const
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_JsonValueToMessage );
#endif
	jsonObject.AddMemberUint32( TXT("m_parts_count"), 1 );
	jsonObject.AddMemberUint32( TXT("m_part"), 0 );
	if( m_delegate )
	{
		jsonObject.AddMemberBool( TXT("gui_active"), m_delegate->IsGUIActive() );
	}
	else
	{
		jsonObject.AddMemberBool( TXT("gui_active"), false );
	}
	CJSONSimpleWriterUTF16 writer;

	writer.WriteObject( jsonObject );

	String message = writer.GetContent();
	RED_ASSERT( ( message.GetLength() < CSecondScreenDevice::s_maxMessageLength), String::Printf( TXT( "Second Screen: single json object plus message header can not be larger then: %i"), CSecondScreenDevice::s_maxMessageLength).AsChar() );

	return message;
}

void CSecondScreenManager::JsonArrayToMessageArray( const CJSONArrayUTF16& _jsonArray, const String arrayName, const THashMap<String, String>& messageProperties, TDynArray<String>& messages ) const
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_JsonArrayToMessageArray );
#endif
	const String messageHeader = String::Printf( TXT("{\"%s\":["), arrayName.AsChar() );
	String messagePropertiesString;

	for( THashMap<String, String>::const_iterator propertieIterator = messageProperties.Begin(); propertieIterator != messageProperties.End(); ++propertieIterator )
	{
		messagePropertiesString += String::Printf( TXT(",\"%s\":\"%s\""), propertieIterator->m_first.AsChar(), propertieIterator->m_second.AsChar() );
	}

	String messagePartsString = TXT( "],\"m_parts_count\":%i,\"m_part\":%i, \"gui_active\": " );
	
	if( m_delegate )
	{
		if( m_delegate->IsGUIActive() )
		{
			messagePartsString += TXT("true");
		}
		else
		{
			messagePartsString += TXT("false");
		}
	}
	else
	{
		messagePartsString += TXT("false");
	}

	const Uint32 messageHeaderSize = messageHeader.GetLength() + messagePartsString.GetLength() + 2 + 1; // +1 is a closing brace
	const Uint32 messagePropertiesStringSize = messagePropertiesString.GetLength();

	String questObjectString;
	
	CJSONSimpleWriterUTF16 writer;
	
	CJSONArrayRefUTF16 jsonArrayRef( _jsonArray );
	const MemSize arraySize = jsonArrayRef.Size();
	for( MemSize index = 0; index < arraySize; )
	{
		String messageBuilder;		
		while ( index < arraySize )
		{
			if( questObjectString.Size() > 0 )// questObjectString is not empty if was not enough space in previous message
			{
				messageBuilder = questObjectString; 
				questObjectString.Clear();
			}

			CJSONObjectRefUTF16 objectToWrite( jsonArrayRef.GetMemberAt( index ) );
			writer.Clear();
			writer.WriteObject( objectToWrite );

			++index;			

			questObjectString = writer.GetContent();

			const Uint32 questObjectStringSize = questObjectString.Size();
			
			RED_ASSERT( ( questObjectStringSize + messageHeaderSize + messagePropertiesStringSize < CSecondScreenDevice::s_maxMessageLength), String::Printf( TXT( "Second Screen: single json object plus message header can not be larger than: %i"), CSecondScreenDevice::s_maxMessageLength).AsChar() );
			
			const Uint32 messageBuilderSize = messageBuilder.Size();
			if( messageBuilderSize + questObjectStringSize + messageHeaderSize + messagePropertiesStringSize < CSecondScreenDevice::s_maxMessageLength )
			{
				if( messageBuilderSize > 0 )
				{
					messageBuilder += TXT(",");
				}
				messageBuilder += questObjectString;
				questObjectString.Clear();					
			}
			else
			{
				break;
			}
		}

		messages.PushBack( messageBuilder );
	}

	if( questObjectString.Size() > 0 )// questObjectString is not empty if was not enough space in previous message
	{
		messages.PushBack( questObjectString );
	}

	Uint32 part	= 0;
	const Uint32 parts_count = messages.Size();
	RED_ASSERT( parts_count <= 999, String::Printf( TXT( "Second Screen: messages count can not be larger than 999, currently is: %i"), parts_count ).AsChar() );

	const TDynArray<String>::iterator messagesEnd = messages.End();
	for ( TDynArray<String>::iterator iterMessage = messages.Begin(); iterMessage != messagesEnd; ++iterMessage )
	{
		*iterMessage = messageHeader + *iterMessage + String::Printf( messagePartsString.AsChar(), parts_count, part ) + messagePropertiesString + TXT( "}" );
		++part;
	}
}

void CSecondScreenManager::ConstructAreaMapPinJsonObject( CJSONObjectUTF16& jsonObject, const SSecondScreenMapPin& mapPin ) const
{
	jsonObject.AddMemberInt32( TXT( "id" ), mapPin.m_id);
	CJSONValueUTF16 tagObject( mapPin.m_tag.AsChar(), mapPin.m_tag.AsString().GetLength() );			
	jsonObject.AddMember( TXT( "t" ), tagObject );
	CJSONValueUTF16 typeObject( mapPin.m_type.AsChar(), mapPin.m_type.AsString().GetLength() );
	jsonObject.AddMember( TXT( "type" ), typeObject );
	jsonObject.AddMemberDouble( TXT( "x" ), mapPin.m_position.X );
	jsonObject.AddMemberDouble( TXT( "y" ), mapPin.m_position.Y );
	jsonObject.AddMemberDouble( TXT( "z" ), mapPin.m_position.Z );
	jsonObject.AddMemberUint32( TXT( "d" ), mapPin.m_isDiscovered ? 1 : 0 );
}

void CSecondScreenManager::ConstructGlobalStaticMapPinsJsonMessage( TDynArray<String>& messages ) const
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_ConstructGlobalStaticMapPinsJsonMessage );
#endif
	CJSONArrayUTF16 jsonArray;
	if( m_globalStaticMapPins.Size() > 0 )
	{
		jsonArray.Reserve( m_globalStaticMapPins.Size() );
	}

	for( TDynArray< SSecondScreenMapPin >::const_iterator iter = m_globalStaticMapPins.Begin(); iter != m_globalStaticMapPins.End(); ++iter )
	{
		CJSONObjectUTF16 globalFTPPinObject;

		ConstructAreaMapPinJsonObject( globalFTPPinObject, *iter );

		jsonArray.PushBack( globalFTPPinObject );
	}

	static const THashMap<String,String> emptyMessageProperties;
	JsonArrayToMessageArray( jsonArray, TXT("global_static_map_pins"), emptyMessageProperties,  messages );
}

void CSecondScreenManager::ConstructAreaMapPinsJsonArray( CJSONArrayUTF16& jsonArray, const TDynArray<SSecondScreenMapPin>& mapPinsArray  ) const
{
	if( mapPinsArray.Size() > 0 )
	{
		jsonArray.Reserve( mapPinsArray.Size() );
	}

	TDynArray< SSecondScreenMapPin >::const_iterator end = mapPinsArray.End();
	for( TDynArray< SSecondScreenMapPin >::const_iterator iter = mapPinsArray.Begin(); iter != end; ++iter )
	{
		CJSONObjectUTF16 areaMapPinJsonObject;

		ConstructAreaMapPinJsonObject( areaMapPinJsonObject, *iter );

		jsonArray.PushBack( areaMapPinJsonObject );
	}
}

void CSecondScreenManager::ConstructAreaStaticMapPinsJsonMessage( TDynArray<String>& messages, const Int32 areaType ) const
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_ConstructAreaStaticMapPinsJsonMessage );
#endif
	const TDynArray<SSecondScreenMapPin>& areaMapPins = m_areasStaticMapPins[ areaType ];

	CJSONArrayUTF16 jsonArray;

	ConstructAreaMapPinsJsonArray( jsonArray, areaMapPins );

	THashMap<String,String> messageProperties;
	messageProperties.Set( TXT("area"), String::Printf( TXT("%i"), areaType ) );
	JsonArrayToMessageArray( jsonArray, TXT("area_static_map_pins"), messageProperties, messages );
}

void CSecondScreenManager::ConstructActualAreaDynamicMapPinsJsonMessage( TDynArray<String>& messages ) const
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_ConstructActualAreaDynamicMapPinsJsonMessage );
#endif

	CJSONArrayUTF16 jsonArray;

	ConstructAreaMapPinsJsonArray( jsonArray, m_actualAreaDynamicMapPins );

	THashMap<String,String> messageProperties;
	messageProperties.Set( TXT("area"), String::Printf( TXT("%i"), m_actualArea ) );
	JsonArrayToMessageArray( jsonArray, TXT("area_dynamic_map_pins"), messageProperties, messages );
}

void CSecondScreenManager::ConstructQuestsStatusJsonMessage( TDynArray<String>& messages  ) const
{	
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_ConstructQuestsStatusJsonMessage );
#endif
	CJSONArrayUTF16 JSONValues;
	if( m_journalQuestEntrys.Size() > 0 )
	{
		JSONValues.Reserve( m_journalQuestEntrys.Size() );
	}	

	for ( THashMap< CGUID, CSecondScreenJournalQuestEntry*>::const_iterator questIterator = m_journalQuestEntrys.Begin(); questIterator != m_journalQuestEntrys.End(); ++questIterator )
	{	
		CJSONObjectUTF16 questObject;
		questIterator->m_second->ConstructJSON( questObject );
		JSONValues.PushBack( questObject );
	}

	static const THashMap<String,String> emptyMessageProperties;
	JsonArrayToMessageArray( JSONValues, TXT("quests"), emptyMessageProperties, messages );
}

void CSecondScreenManager::ConstructTrackedQuestUpdatedJsonMessage( String& message ) const
{
	Char guidStringBuffer[guildStringBufferSize];
	m_trackedQuest.ToString( guidStringBuffer, guildStringBufferSize );
		
	CJSONValueUTF16 jsobQuestGuid( guidStringBuffer, guildStringBufferSize - 1 );

	CJSONObjectUTF16 jsonTitle;
	jsonTitle.AddMember( TXT("q_guid"), jsobQuestGuid );

	CJSONObjectUTF16 jsonTrackedQuestUpdated;
	jsonTrackedQuestUpdated.AddMember( TXT("tracked_q_update"), jsonTitle );

	message = JsonValueToMessage( jsonTrackedQuestUpdated );
}

void CSecondScreenManager::ConstructTrackedQuestObjectiveUpdatedJsonMessage( String& message ) const
{
	Char guidStringBuffer[guildStringBufferSize];
	m_trackedQuest.ToString( guidStringBuffer, guildStringBufferSize );


	CJSONValueUTF16 jsobQuestGuid( guidStringBuffer, guildStringBufferSize - 1 );

	CJSONObjectUTF16 jsonTitle;
	jsonTitle.AddMember( TXT("q_guid"), jsobQuestGuid );

	CJSONObjectUTF16 jsonTrackedQuestUpdated;
	jsonTrackedQuestUpdated.AddMember( TXT("tracked_qo_update"), jsonTitle );

	message = JsonValueToMessage( jsonTrackedQuestUpdated );
}


void CSecondScreenManager::ConstructFastTravelEnableJsonMessage( String& message ) const
{
	CJSONObjectUTF16 jsonRootObject;
	jsonRootObject.AddMemberBool( TXT("fast_travel_enable"), m_fastTravelEnabled );

	message = JsonValueToMessage( jsonRootObject );
}

void CSecondScreenManager::ConstructTextFromGameJsonMessage( const Uint32 reqId, const Int32 stringId, const String& stringKey, TDynArray<String>& messages ) const
{
	String text = SLocalizationManager::GetInstance().GetString( stringId, true );
	if( text.Empty() == false )
	{
		ConstructJsonMessage( text, reqId, stringId, stringKey, messages );	
	}
	else
	{
		ConstructEmptyJsonMessage( reqId, messages );
	}
}

void CSecondScreenManager::ConstructJsonMessage( const String& text, const Uint32 reqId, const Int32 stringId, const String& stringKey, TDynArray<String>& messages ) const
{
	const Uint32 textBufferSize = text.GetLength()*sizeof(String::value_type);
	AnsiChar* encodedBuffer = new AnsiChar[GetEncodeBinaryArrayLen( textBufferSize)];
	EncodeBinaryArray( encodedBuffer, (const AnsiChar*)text.AsChar(), textBufferSize );

	String encodedString;
	encodedString = ANSI_TO_UNICODE( encodedBuffer );
	delete [] encodedBuffer;		

	String messageHeader = String::Printf( TXT("{ \"text_from_game\": {\"req_id\":%i, \"success\": true, \"string_id\":%i"), reqId, stringId );
	if( stringKey.Empty() == false )
	{
		messageHeader += String::Printf( TXT(",\"string_key\": \"%s\""), stringKey.AsChar() );
	}

	messageHeader += TXT( ", \"string_base64\": \""); 

	const String messagePartsString = TXT( "\"},\"m_parts_count\":%i,\"m_part\":%i" );
	const Uint32 messageHeaderSize = messageHeader.GetLength() + messagePartsString.GetLength() + 2 + 1; // +1 is a closing brace

	RED_ASSERT( ( messageHeaderSize < CSecondScreenDevice::s_maxMessageLength), String::Printf( TXT( "Second Screen: single json object plus message header can not be larger than: %i"), CSecondScreenDevice::s_maxMessageLength).AsChar() );

	if ( messageHeaderSize < CSecondScreenDevice::s_maxMessageLength )
	{		
		TDynArray<String> stringBase64Parts;
		SplitToParts( messageHeaderSize, encodedString, stringBase64Parts );

		const Uint32 stringBase64PartsCount = stringBase64Parts.Size();
		for( Uint32 i = 0; i < stringBase64PartsCount; ++i )
		{
			const String message = messageHeader + stringBase64Parts[i] + String::Printf( messagePartsString.AsChar(), stringBase64PartsCount, i ) + TXT( "}" );
			messages.PushBack( message );
		}
	}
}

void CSecondScreenManager::SplitToParts( const Uint32 messageHeaderSize, const String& encodedString, TDynArray<String>& stringBase64Parts ) const
{
	Uint32 maxCharCountToSend = (Uint32)(CSecondScreenDevice::s_maxMessageLength - messageHeaderSize);
	Uint32 encodedStringOffset = 0;
	const MemSize encodedStringLength = encodedString.GetLength();
	while ( encodedStringOffset < encodedStringLength )
	{
		Uint32 charCountToSend = (Uint32)(encodedStringLength - encodedStringOffset);
		if( charCountToSend > maxCharCountToSend )
		{
			charCountToSend = maxCharCountToSend;
		}
		const String stringBase64Part( encodedString.AsChar() + encodedStringOffset, charCountToSend );

		stringBase64Parts.PushBack( stringBase64Part );

		encodedStringOffset += charCountToSend;
	}
}

void CSecondScreenManager::ConstructEmptyJsonMessage( Uint32 reqId, TDynArray<String>& messages ) const
{
	CJSONObjectUTF16 jsonRootObject;

	CJSONObjectUTF16 jsonTextObject;

	jsonTextObject.AddMemberInt32( TXT("req_id"), reqId );
	jsonTextObject.AddMemberBool( TXT("success"), false );

	jsonRootObject.AddMember( TXT("text_from_game"), jsonTextObject );

	const String message = JsonValueToMessage( jsonRootObject );
	messages.PushBack( message );
}

CSecondScreenManager::CSecondScreenManager(): m_Initialized( false ), m_delegate(NULL)
{
	RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "Manager created." ) );
	Init();
}

CSecondScreenManager::~CSecondScreenManager()
{
	Shutdown();


	RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "Manager destroyed." ) );
}

void CSecondScreenManager::ClearGameData()
{
	m_gameState				= GS_NONE;
	m_playerPositionX = m_playerPositionY = m_playerPositionZ = 0.0f;
	m_actualArea = -1;
	m_actualAreaFileName.Clear();
	m_globalStaticMapPins.Clear();
	m_areasStaticMapPins.Clear();
	m_actualAreaDynamicMapPins.Clear();
	
	for( THashMap< CGUID, CSecondScreenJournalQuestEntry*>::iterator journalQuestEntryIterator = m_journalQuestEntrys.Begin(); journalQuestEntryIterator != m_journalQuestEntrys.End(); ++journalQuestEntryIterator )
	{
		delete journalQuestEntryIterator->m_second;
	}
	m_journalQuestEntrys.Clear();
	m_fastTravelEnabled = false;
}

void CSecondScreenManager::Update( Float timeDelta )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_Update );
#endif

	OnHandleRecievedMessages();
	
	m_mutex.Acquire();

	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* device = *it;
		if( m_delegate )
		{
			if( device->SendConnectionEstablish() ) // connection_establish
			{
	#ifdef SECOND_SCREEN_PROFILE_CODE
				PC_SCOPE( SS_Update_SendConnectionEstablish);
	#endif
				String gameState;
				if( GameStateToString( gameState ) )
				{
					String message;

					CJSONObjectUTF16 jsonRootObject;

					CJSONValueUTF16 positionX( m_playerPositionX );
					CJSONValueUTF16 positionY( m_playerPositionY );
					CJSONValueUTF16 positionZ( m_playerPositionZ );


					CJSONObjectUTF16 worldObject;
					CJSONValueUTF16 worldFileNameObject( m_actualAreaFileName.AsChar(), m_actualAreaFileName.GetLength() );
					worldObject.AddMemberInt32( TXT( "area" ), m_actualArea );
					worldObject.AddMember( TXT( "area_file_name" ), worldFileNameObject );

					CJSONObjectUTF16 playerPosition;
					playerPosition.AddMember( TXT( "x" ), positionX );
					playerPosition.AddMember( TXT( "y" ), positionY );
					playerPosition.AddMember( TXT( "z" ), positionZ );

					CJSONValueUTF16 gameStateObject( gameState.AsChar(), gameState.GetLength() );

					CJSONObjectUTF16 conectionEstablised;

					conectionEstablised.AddMemberUint32( TXT( "version" ),	 GAME_VERSION );
					conectionEstablised.AddMember( TXT( "actual_area" ),	 worldObject );
					conectionEstablised.AddMember( TXT( "player_position" ), playerPosition );
					conectionEstablised.AddMember( TXT( "game_state" ),		 gameStateObject );
					conectionEstablised.AddMemberString( TXT( "language" ), SLocalizationManager::GetInstance().GetCurrentLocale().AsChar() );

					jsonRootObject.AddMember( TXT( "conection_established" ), conectionEstablised ); 

					message = JsonValueToMessage( jsonRootObject );

					device->SendMessage( message.AsChar() );
				}

				TDynArray<String> messages;

				ConstructGlobalStaticMapPinsJsonMessage( messages );

				SendMessages( device, messages );

				for ( THashMap< Int32, TDynArray<SSecondScreenMapPin> >::const_iterator it=m_areasStaticMapPins.Begin(); it!=m_areasStaticMapPins.End(); ++it )
				{
					messages.Clear();

					ConstructAreaStaticMapPinsJsonMessage( messages, it->m_first );

					SendMessages( device, messages );
				}
				messages.Clear();

				ConstructActualAreaDynamicMapPinsJsonMessage( messages );

				SendMessages( device, messages );

				messages.Clear();

				ConstructQuestsStatusJsonMessage( messages );

				SendMessages( device, messages );

				if( m_trackedQuest.IsZero() == false )
				{
					String message;

					ConstructTrackedQuestUpdatedJsonMessage( message );

					device->SendMessage( message.AsChar() );
				}

				if( m_trackedQuestObjective.IsZero() == false )
				{
					String message;
				
					ConstructTrackedQuestObjectiveUpdatedJsonMessage( message );

					device->SendMessage( message.AsChar() );
				}
			
				String message;

				ConstructFastTravelEnableJsonMessage( message );

				device->SendMessage( message.AsChar() );

				device->ConnectionEstablishSent();
			}
		}
		device->Tick( timeDelta );		
	}
	
	m_deviceWatcher->Update(timeDelta);

	m_mutex.Release();
		
}

void CSecondScreenManager::OnDeviceAdded( CSecondScreenDeviceWatcher* sender, CSecondScreenDevice* device )
{
	//RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "Manager add new device." ) );

	m_mutex.Acquire();
	m_devicesList.PushBack( device );
	m_mutex.Release();
}

void CSecondScreenManager::OnDeviceRemoved( CSecondScreenDeviceWatcher* sender, const CSecondScreenDevice& device )
{
	m_mutex.Acquire();
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		if( *obj == device )
		{
			delete obj;
			m_devicesList.Erase( it );
			//RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "Manager remove device." ) );
			break;
		}		
	}
	m_mutex.Release();
}

void CSecondScreenManager::OnMessageReceived( const CSecondScreenDevice& device, const Char* message )
{
	m_mutexReceive.Acquire();
	CSecondScreenReceiveMessage receiveMessage;
	receiveMessage.mFromDevice = &device;
	receiveMessage.mMessage = message;
	m_receivedMessages.PushBack( receiveMessage );
	m_mutexReceive.Release();
}


void CSecondScreenManager::OnHandleRecievedMessages()
{
	m_mutexReceive.Acquire();
	while ( m_receivedMessages.Empty() == false )
	{
		CJSONReaderUTF16 reader;	
		CSecondScreenReceiveMessage receiveMessage = m_receivedMessages.PopBack();
		if( reader.Read( receiveMessage.mMessage.AsChar() ) == true )
		{
			if( reader.GetDocument().HasMember( TXT("fast_travel") ) == true ) 
			{
				CJSONObjectRefUTF16 jsonObject( reader.GetDocument().GetMember( TXT("fast_travel") ) );
				OnHandleFastTravelMessage( jsonObject );
			}
			else if( reader.GetDocument().HasMember( TXT("set_tracked_q") ) == true )
			{
				CJSONObjectRefUTF16 jsonObject( reader.GetDocument().GetMember( TXT("set_tracked_q") ) );
				OnHandleSetTrackedQuestMessage( jsonObject );
			}
			else if( reader.GetDocument().HasMember( TXT("get_text") ) == true )
			{
				CJSONObjectRefUTF16 jsonObject( reader.GetDocument().GetMember( TXT("get_text") ) );
				OnHandleGetTextMessage( jsonObject, receiveMessage.mFromDevice );
			}
		}
		else
		{
			RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "PARSE ERROR" ) );
		}
	}
	m_mutexReceive.Release();
}

void CSecondScreenManager::OnHandleFastTravelMessage( const CJSONObjectRefUTF16& jsonObject )
{
	if(    jsonObject.HasMember( TXT("t") )
		&& jsonObject.HasMember( TXT("area") ) )
	{
		CJSONValueRefUTF16 tagJsonValue( jsonObject.GetMember( TXT("t") ) );
		CJSONValueRefUTF16 areaJsonValue( jsonObject.GetMember( TXT("area") ) );

		CName mapPinTag( tagJsonValue.GetString() );
		Int32 areaType = areaJsonValue.GetInt32();
		if( areaType == m_actualArea )
		{
			if( m_delegate )
			{
				m_delegate->OnHandleFastTravel( mapPinTag, m_actualArea, true );
			}
			else
			{
				RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "ISecondScreenManagerDelegate not set !!!" ) );
			}
		}
		else 
		{
			if( m_delegate )
			{
				m_delegate->OnHandleFastTravel( mapPinTag, areaType, false );
			}
			else
			{
				RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "ISecondScreenManagerDelegate not set !!!" ) );
			}
		}
	}
	else
	{
		RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "PARSE ERROR" ) );
	}
}

void CSecondScreenManager::OnHandleSetTrackedQuestMessage( const CJSONObjectRefUTF16& jsonObject )
{
	CJSONValueRefUTF16 questGuidJsonValue( jsonObject.GetMember( TXT("q_guid") ) );

	CGUID questGUID( questGuidJsonValue.GetString() );

	if( m_delegate )
	{
		m_delegate->OnHandleTrackQuest( questGUID );
	}
}

void CSecondScreenManager::OnHandleGetTextMessage( const CJSONObjectRefUTF16& jsonObject, const CSecondScreenDevice* toDevice )
{
	CJSONValueRefUTF16 reqIdJsonValue( jsonObject.GetMember( TXT("req_id") ) );

	Uint32 reqId = reqIdJsonValue.GetUint32();

	if( jsonObject.HasMember( TXT("string_id") ) )
	{
		CJSONValueRefUTF16 idJsonValue( jsonObject.GetMember( TXT("string_id") ) );

		Int32 stringId = idJsonValue.GetInt32();

		TDynArray<String> messages;

		//! CONSTRUCT JSON OBJECT

		ConstructTextFromGameJsonMessage( reqId, stringId, String::EMPTY, messages );
		
		//! SEND MESSAGE TO DEVICES	

		m_mutex.Acquire();
		TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
		TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
		for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
		{
			CSecondScreenDevice* obj = *it;
			if( toDevice == obj)
			{
				SendMessages( obj, messages );
				break;
			}				
		}
		m_mutex.Release();

	}
	else if( jsonObject.HasMember( TXT("string_key") ) )
	{
		CJSONValueRefUTF16 stringIdJsonValue( jsonObject.GetMember( TXT("string_key") ) );

		String stringKey = stringIdJsonValue.GetString();

		TDynArray<String> messages;

		//! CONSTRUCT JSON OBJECT

		Int32 stringId = SLocalizationManager::GetInstance().GetStringIdByStringKey( stringKey );

		ConstructTextFromGameJsonMessage( reqId, stringId, stringKey, messages );
	
		//! SEND MESSAGE TO DEVICES				

		m_mutex.Acquire();
		TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
		TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
		for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
		{
			CSecondScreenDevice* obj = *it;
			if( toDevice == obj)
			{
				SendMessages( obj, messages );	
				break;
			}
		}
		m_mutex.Release();
	}
	else
	{
		RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "PARSE ERROR" ) );
	}
}

void CSecondScreenManager::SendState( const GAME_STATE state )
{

#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_Update_changeState);
#endif

	if( m_gameState != state )
	{
		//! UPDATE GAME STATE
		m_gameState = state;
		
		String gameState;
		if( GameStateToString( gameState ) )
		{
			//! CONSTRUCT JSON OBJECT

			CJSONObjectUTF16 jsonRootObject;

			CJSONValueUTF16 gameStateObject( gameState.AsChar(), gameState.GetLength() );			

			jsonRootObject.AddMember( TXT( "state_change" ), gameStateObject );

			String message = JsonValueToMessage( jsonRootObject );

			//! SEND MESSAGE TO DEVICES

			m_mutex.Acquire();
			TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
			TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
			for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
			{
				CSecondScreenDevice* obj = *it;
				obj->SendMessage( message.AsChar() );		
			}
			m_mutex.Release();			
		}

		if( m_gameState == GS_NONE )// on end game reset all game informations
		{
			m_mutex.Acquire();
			ClearGameData();
			m_mutex.Release();
		}
	}
}

Bool CSecondScreenManager::GameStateToString( String& gameState ) const
{
	if( m_gameState == GS_NONE )
	{
		gameState = TXT( "GS_NONE" );
		return true;
	}
	else if( m_gameState == GS_MAIN_MENU )
	{
		gameState = TXT( "GS_MAIN_MENU" );
		return true;
	}
	else if( m_gameState == GS_GAME_SESSION )
	{
		gameState = TXT( "GS_GAME_SESSION" );
		return true;
	}
	return false;
}

const Char * CSecondScreenManager::JournalQuesStatusToString( Int32 status )
{
	if( status == 0 )
	{		 
		return TXT( "QUEST_INACTIVE" );
	}
	if( status == 1 )
	{
		return TXT( "QUEST_ACTIVE" );
	}
	if( status == 2 )
	{
		return TXT( "QUEST_SUCCESS" );
	}
	if( status == 3 )
	{
		return TXT( "QUEST_FAILED" );
	}	
	return TXT("");
}

void CSecondScreenManager::SendPlayerPosition( const Float x, const Float y, const Float z, const Float rotation )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendPlayerPosition );
#endif

	//! UPDATE PLAYER POSITION

	m_playerPositionX = x;
	m_playerPositionY = y;
	m_playerPositionZ = z;
	m_playerRotation = rotation;

	//! CONSTRUCT JSON OBJECT

	CJSONObjectUTF16 jsonRootObject;

	CJSONValueUTF16 positionX( m_playerPositionX );
	CJSONValueUTF16 positionY( m_playerPositionY );
	CJSONValueUTF16 positionZ( m_playerPositionZ );
	CJSONValueUTF16 rot( m_playerRotation );

	CJSONObjectUTF16 playerPosition;
	playerPosition.AddMember( TXT( "x" ), positionX );
	playerPosition.AddMember( TXT( "y" ), positionY );
	playerPosition.AddMember( TXT( "z" ), positionZ );
	playerPosition.AddMember( TXT( "rotation" ), rot );
		
	jsonRootObject.AddMember( TXT( "player_position" ), playerPosition );

	String message = JsonValueToMessage( jsonRootObject );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendGlobalStaticMapPins( const TDynArray<SSecondScreenMapPin>& globalMapPins )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendGlobalStaticMapPins );
#endif
	TDynArray<String> messages;	

	//! UPDATE GLOBAL FTP MAP PINS

	m_globalStaticMapPins = globalMapPins;

	//! CONSTRUCT JSON OBJECT

	ConstructGlobalStaticMapPinsJsonMessage( messages );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();		
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		SendMessages( obj, messages );	
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendAreaStaticMapPins( const Int32 areaType, const TDynArray<SSecondScreenMapPin>& areaMapPins )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendAreaStaticMapPins );
#endif
	TDynArray<String> messages;
	
	//! UPDATE AREA MAP PINS

	m_areasStaticMapPins.Set( areaType, areaMapPins );
	
	//! CONSTRUCT JSON OBJECT

	ConstructAreaStaticMapPinsJsonMessage( messages, areaType );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		SendMessages( obj, messages );			
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendActualAreaDynamicMapPin( const SSecondScreenMapPin& mapPin )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendActualAreaDynamicMapPin );
#endif

	//! ADD OR UPDATE ACTUAL AREA DYNAMIC MAP PIN
	
	TDynArray<SSecondScreenMapPin>::iterator actualAreaDynamicMapPinsEnd, foundMapPin;
	actualAreaDynamicMapPinsEnd = foundMapPin = m_actualAreaDynamicMapPins.End();
	for( TDynArray<SSecondScreenMapPin>::iterator iterMapPin = m_actualAreaDynamicMapPins.Begin(); iterMapPin != actualAreaDynamicMapPinsEnd; ++iterMapPin )
	{
		if( (*iterMapPin).m_id == mapPin.m_id )
		{
			foundMapPin = iterMapPin;
			break;
		}
	}

	if( foundMapPin == actualAreaDynamicMapPinsEnd )
	{
		m_actualAreaDynamicMapPins.PushBack( mapPin );
	}
		
	//! CONSTRUCT JSON OBJECT

	CJSONObjectUTF16 jsonRootObject;

	CJSONObjectUTF16 mapPinJsonObject;

	ConstructAreaMapPinJsonObject( mapPinJsonObject, mapPin );
	mapPinJsonObject.AddMemberInt32( TXT( "area" ), m_actualArea );

	jsonRootObject.AddMember( TXT( "area_dynamic_map_pin"), mapPinJsonObject );

	String message = JsonValueToMessage( jsonRootObject );
	
	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );			
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendRemoveActualAreaDynamicMapPin( const Int32 id )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendRemoveActualAreaDynamicMapPin );
#endif

	//! REMOVE ACTUAL AREA DYNAMIC MAP PIN
	
	TDynArray<SSecondScreenMapPin>::iterator endActualAreaMapPins = m_actualAreaDynamicMapPins.End();
	for( TDynArray<SSecondScreenMapPin>::iterator iterMapPin = m_actualAreaDynamicMapPins.Begin(); iterMapPin != endActualAreaMapPins; ++iterMapPin )
	{
		if( (*iterMapPin).m_id == id )
		{
			m_actualAreaDynamicMapPins.Erase( iterMapPin );
			break;
		}
	}

	//! CONSTRUCT JSON OBJECT

	CJSONObjectUTF16 jsonRootObject;

	CJSONObjectUTF16 mapPinJsonObject;

	mapPinJsonObject.AddMemberInt32( TXT( "id" ), id );

	jsonRootObject.AddMember( TXT( "area_dynamic_map_pin_remove"), mapPinJsonObject );

	String message = JsonValueToMessage( jsonRootObject );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );			
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendMoveActualAreaDynamicMapPin( const Int32 id, const CName& type, const Vector& position )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendActualAreaDynamicMapPin );
#endif
	Bool sendMessage = false;

	//! MOVE ACTUAL AREA DYNAMIC MAP PIN

	TDynArray<SSecondScreenMapPin>::iterator actualAreaDynamicMapPinsEnd, foundMapPin;
	actualAreaDynamicMapPinsEnd = foundMapPin = m_actualAreaDynamicMapPins.End();
	for( TDynArray<SSecondScreenMapPin>::iterator iterMapPin = m_actualAreaDynamicMapPins.Begin(); iterMapPin != actualAreaDynamicMapPinsEnd; ++iterMapPin )
	{
		if( (*iterMapPin).m_id == id )
		{
			foundMapPin = iterMapPin;
			break;
		}
	}

	if( foundMapPin != actualAreaDynamicMapPinsEnd )
	{
		Float distance2D = ((*foundMapPin).m_position.DistanceTo2D( position ) );

		if( distance2D > 2.0f )
		{
			(*foundMapPin).m_position = position;
			sendMessage = true;
		}
	}

	if( sendMessage )
	{			
		//! CONSTRUCT JSON OBJECT

		CJSONObjectUTF16 jsonRootObject;

		CJSONObjectUTF16 mapPinJsonObject;

		mapPinJsonObject.AddMemberInt32( TXT( "id" ), id);
		CJSONValueUTF16 typeJsonValue( type.AsChar(), type.AsString().GetLength() );			
		mapPinJsonObject.AddMember( TXT( "type" ), typeJsonValue );
		mapPinJsonObject.AddMemberDouble( TXT( "x" ), position.X );
		mapPinJsonObject.AddMemberDouble( TXT( "y" ), position.Y );
		mapPinJsonObject.AddMemberDouble( TXT( "z" ), position.Z );
		mapPinJsonObject.AddMemberInt32( TXT( "area" ), m_actualArea );

		jsonRootObject.AddMember( TXT( "area_dynamic_map_pin_move"), mapPinJsonObject );

		String message = JsonValueToMessage( jsonRootObject );

		//! SEND MESSAGE TO DEVICES

		m_mutex.Acquire();
		TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
		TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
		for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
		{
			CSecondScreenDevice* obj = *it;
			obj->SendMessage( message.AsChar() );			
		}
		m_mutex.Release();
	}
}

void CSecondScreenManager::SendUpdateActualAreaDynamicMapPin( const SSecondScreenMapPin& mapPin )
{
	//! ADD OR UPDATE ACTUAL AREA DYNAMIC MAP PIN

	TDynArray<SSecondScreenMapPin>::iterator actualAreaDynamicMapPinsEnd, foundMapPin;
	actualAreaDynamicMapPinsEnd = foundMapPin = m_actualAreaDynamicMapPins.End();
	for( TDynArray<SSecondScreenMapPin>::iterator iterMapPin = m_actualAreaDynamicMapPins.Begin(); iterMapPin != actualAreaDynamicMapPinsEnd; ++iterMapPin )
	{
		if( (*iterMapPin).m_id == mapPin.m_id )
		{
			foundMapPin = iterMapPin;
			break;
		}
	}

	if( foundMapPin == actualAreaDynamicMapPinsEnd )
	{
		m_actualAreaDynamicMapPins.PushBack( mapPin );
	}
	else
	{
		(*foundMapPin) = mapPin;
	}

	//! CONSTRUCT JSON OBJECT

	CJSONObjectUTF16 jsonRootObject;

	CJSONObjectUTF16 mapPinJsonObject;

	ConstructAreaMapPinJsonObject( mapPinJsonObject, mapPin );
	mapPinJsonObject.AddMemberInt32( TXT( "area" ), m_actualArea );

	jsonRootObject.AddMember( TXT( "area_dynamic_map_pin_update"), mapPinJsonObject );

	String message = JsonValueToMessage( jsonRootObject );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );			
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendStaticMapPinDiscovered( const CName& tag  )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendStaticMapPinDiscovered );
#endif
	TDynArray< SSecondScreenMapPin >::iterator iter = m_globalStaticMapPins.Begin();

	while( iter != m_globalStaticMapPins.End() )
	{
		if( iter->m_tag == tag )
		{
			iter->m_isDiscovered = true;
			break;
		}
		++iter;
	}

	for ( THashMap< Int32, TDynArray<SSecondScreenMapPin> >::iterator it_areas=m_areasStaticMapPins.Begin(); it_areas!=m_areasStaticMapPins.End(); ++it_areas )
	{
		TDynArray< SSecondScreenMapPin >::iterator it_pins = it_areas->m_second.Begin();
		while( it_pins != it_areas->m_second.End() )
		{
			if( it_pins->m_tag == tag )
			{
				it_pins->m_isDiscovered = true;
				break;
			}
			++it_pins;
		}
	}

	//! CONSTRUCT JSON OBJECT

	CJSONObjectUTF16 jsonRootObject;

	CJSONValueUTF16 mapPinDiscoveredObject( tag.AsChar(), tag.AsString().GetLength() );			

	jsonRootObject.AddMember( TXT( "static_map_pin_discovered" ), mapPinDiscoveredObject );

	String message = JsonValueToMessage( jsonRootObject );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();

}

void CSecondScreenManager::ClearFileNameToAreaMapping()
{
	m_fileNameToAreaMapping.Clear();
}

void CSecondScreenManager::SetFileNameToAreaMapping( const String& fileName, const Int32 areaIndex )
{
	m_fileNameToAreaMapping.Set( fileName, areaIndex );
}

void CSecondScreenManager::SendChangeArea( const String& areaName )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendChangeArea );
#endif
	
	//! SET ACTUAL AREA

	m_actualAreaFileName = areaName;
	if( m_actualAreaFileName.Empty() == false )
	{
		if( m_fileNameToAreaMapping.KeyExist( areaName ) )
		{
			m_actualArea = m_fileNameToAreaMapping[areaName];
		}
		else 
		{
			m_actualArea = -1;
		}
		
		//! CONSTRUCT JSON OBJECT

		CJSONObjectUTF16 jsonRootObject;
		CJSONObjectUTF16 worldObject;
		CJSONValueUTF16 worldFileNameObject( m_actualAreaFileName.AsChar(), m_actualAreaFileName.GetLength() );
		worldObject.AddMemberInt32( TXT( "area" ), m_actualArea );
		worldObject.AddMember( TXT( "area_file_name" ), worldFileNameObject );
		jsonRootObject.AddMember( TXT( "area_change" ), worldObject );

		String message = JsonValueToMessage( jsonRootObject );

		//! SEND MESSAGE TO DEVICES

		m_mutex.Acquire();
		TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
		TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
		for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
		{
			CSecondScreenDevice* obj = *it;
			obj->SendMessage( message.AsChar() );		
		}
		m_mutex.Release();
	}
	else
	{
		m_actualArea = -1;
	}
}

CSecondScreenManager::CSecondScreenJournalQuestEntry* CSecondScreenManager::GetJournalQuestEntry( const Int32 type, const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const Int32 status  )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_GetJournalQuestEntry );
#endif
	CSecondScreenJournalQuestEntry* journalQuestEntry = NULL;

	m_mutex.Acquire();
	THashMap< CGUID, CSecondScreenJournalQuestEntry*>::const_iterator foundJournalQuestEntry = m_journalQuestEntrys.Find( guid );
	if( foundJournalQuestEntry != m_journalQuestEntrys.End() )
	{
		journalQuestEntry = foundJournalQuestEntry->m_second;
	}
	else
	{
		journalQuestEntry = new CSecondScreenJournalQuestEntry( type, guid, parentGuid, tag, name );
		m_journalQuestEntrys.Set( guid, journalQuestEntry );
	}	
	m_mutex.Release();

	journalQuestEntry->m_status = status;

	return journalQuestEntry;
}

void CSecondScreenManager::SendJournalQuestGroupStatus( const CGUID& guid, const CName& tag, const String& name, const Uint32 stringIndex_title, const Int32 status )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendJournalQuestGroupStatus );
#endif
	//! UPDATE JOURNAL OBJECT

	CSecondScreenJournalQuestEntry* journalQuestEntry = GetJournalQuestEntry( CSecondScreenJournalQuestEntry::JQE_QuestGroup, guid, guid, tag, name, status );	
	journalQuestEntry->m_properties.Set( TXT( "title_string_index" ), String::Printf( TXT("%u"), stringIndex_title ) );

	//! CONSTRUCT JSON OBJECT

	CJSONObjectUTF16 jsonQuestObject;
	journalQuestEntry->ConstructJSON( jsonQuestObject );

	CJSONObjectUTF16 jsonRootObject;
	jsonRootObject.AddMember( TXT("quest"), jsonQuestObject );

	String message = JsonValueToMessage( jsonRootObject );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();	
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendJournalQuestStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const Int32 type, const Int32 area, const Uint32 stringIndex_title, const Int32 status, Bool silent )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendJournalQuestStatus );
#endif
	//! UPDATE JOURNAL OBJECT

	CSecondScreenJournalQuestEntry* journalQuestEntry = GetJournalQuestEntry( CSecondScreenJournalQuestEntry::JQE_Quest, guid, parentGuid, tag, name, status );	
	journalQuestEntry->m_properties.Set( TXT( "type" ), String::Printf( TXT("%i"), type ) );
	journalQuestEntry->m_properties.Set( TXT( "area" ), String::Printf( TXT("%i"), area ) );
	journalQuestEntry->m_properties.Set( TXT( "title_string_index" ), String::Printf( TXT("%u"), stringIndex_title ) );
	journalQuestEntry->m_properties.Set( TXT( "silent" ), silent ? TXT("true") : TXT("false") );

	//! CONSTRUCT JSON OBJECT

	CJSONObjectUTF16 jsonQuestObject;
	journalQuestEntry->ConstructJSON( jsonQuestObject );

	CJSONObjectUTF16 jsonRootObject;
	jsonRootObject.AddMember( TXT("quest"), jsonQuestObject );

	String message = JsonValueToMessage( jsonRootObject );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();	
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendJournalQuestPhaseStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const Int32 status )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendJournalQuestPhaseStatus );
#endif
	//! UPDATE JOURNAL OBJECT

	CSecondScreenJournalQuestEntry* journalQuestEntry = GetJournalQuestEntry( CSecondScreenJournalQuestEntry::JQE_QuestPhase, guid, parentGuid, tag, name, status );	
		
	CJSONObjectUTF16 jsonQuestObject;
	journalQuestEntry->ConstructJSON( jsonQuestObject );

	//! CONSTRUCT JSON OBJECT

	CJSONObjectUTF16 jsonRootObject;
	jsonRootObject.AddMember( TXT("quest"), jsonQuestObject );

	String message = JsonValueToMessage( jsonRootObject );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();	
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendJournalQuestObjectiveStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const Uint32 stringIndex_title, const String& image, const Int32 area, const Int32 counterType, const Int32 count, const Int32 status )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendJournalQuestObjectiveStatus );
#endif
	//! UPDATE JOURNAL OBJECT

	CSecondScreenJournalQuestEntry* journalQuestEntry = GetJournalQuestEntry( CSecondScreenJournalQuestEntry::JQE_QuestObjective, guid, parentGuid, tag, name, status );	
	journalQuestEntry->m_properties.Set( TXT( "title_string_index" ), String::Printf( TXT("%u"), stringIndex_title ) );
	journalQuestEntry->m_properties.Set( TXT( "image" ), image );
	journalQuestEntry->m_properties.Set( TXT( "area" ), String::Printf( TXT("%i"), area ) );
	journalQuestEntry->m_properties.Set( TXT( "counterType" ), String::Printf( TXT("%i"), counterType ) );
	journalQuestEntry->m_properties.Set( TXT( "count" ), String::Printf( TXT("%i"), count ) );

	//! CONSTRUCT JSON OBJECT

	CJSONObjectUTF16 jsonQuestObject;
	journalQuestEntry->ConstructJSON( jsonQuestObject );

	CJSONObjectUTF16 jsonRootObject;
	jsonRootObject.AddMember( TXT("quest"), jsonQuestObject );

	String message = JsonValueToMessage( jsonRootObject );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();	
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendJournalQuestItemTagStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const CName& item, const Int32 status )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendJournalQuestItemTagStatus );
#endif
	//! UPDATE JOURNAL OBJECT

	CSecondScreenJournalQuestEntry* journalQuestEntry = GetJournalQuestEntry( CSecondScreenJournalQuestEntry::JQE_QuestItemTag, guid, parentGuid, tag, name, status );	
	journalQuestEntry->m_properties.Set( TXT( "item" ), item.AsString() );

	//! CONSTRUCT JSON OBJECT

	CJSONObjectUTF16 jsonQuestObject;
	journalQuestEntry->ConstructJSON( jsonQuestObject );
	
	CJSONObjectUTF16 jsonRootObject;
	jsonRootObject.AddMember( TXT("quest"), jsonQuestObject );

	String message = JsonValueToMessage( jsonRootObject );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();	
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendJournalQuestEnemyTagStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const String& enemyTag, const Int32 status )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendJournalQuestEnemyTagStatus );
#endif
	//! UPDATE JOURNAL OBJECT

	CSecondScreenJournalQuestEntry* journalQuestEntry = GetJournalQuestEntry( CSecondScreenJournalQuestEntry::JQE_QuestEnemyTag, guid, parentGuid, tag, name, status );	
	journalQuestEntry->m_properties.Set( TXT( "enemy_tag" ), enemyTag );

	//! CONSTRUCT JSON OBJECT

	CJSONObjectUTF16 jsonQuestObject;
	journalQuestEntry->ConstructJSON( jsonQuestObject );
	
	CJSONObjectUTF16 jsonRootObject;
	jsonRootObject.AddMember( TXT("quest"), jsonQuestObject );

	String message = JsonValueToMessage( jsonRootObject );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();	
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendJournalQuestMapPinStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const CName& mapPinID, const Int32 status ) 
{	
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendJournalQuestMapPinStatus );
#endif
	//! UPDATE JOURNAL OBJECT

	CSecondScreenJournalQuestEntry* journalQuestEntry = GetJournalQuestEntry( CSecondScreenJournalQuestEntry::JQE_QuestMapPin, guid, parentGuid, tag, name, status );	
	journalQuestEntry->m_properties.Set( TXT( "map_pin_id" ), mapPinID.AsString() );

	//! CONSTRUCT JSON OBJECT

	CJSONObjectUTF16 jsonQuestObject;
	journalQuestEntry->ConstructJSON( jsonQuestObject );
	
	CJSONObjectUTF16 jsonRootObject;
	jsonRootObject.AddMember( TXT("quest"), jsonQuestObject );

	String message = JsonValueToMessage( jsonRootObject );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();	
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendJournalQuestDescriptionGroupStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const Int32 status )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendJournalQuestDescriptionGroupStatus );
#endif
	//! UPDATE JOURNAL OBJECT

	CSecondScreenJournalQuestEntry* journalQuestEntry = GetJournalQuestEntry( CSecondScreenJournalQuestEntry::JQE_QuestDescriptionGroup, guid, parentGuid, tag, name, status );	

	//! CONSTRUCT JSON OBJECT

	CJSONObjectUTF16 jsonQuestObject;
	journalQuestEntry->ConstructJSON( jsonQuestObject );

	CJSONObjectUTF16 jsonRootObject;
	jsonRootObject.AddMember( TXT("quest"), jsonQuestObject );

	String message = JsonValueToMessage( jsonRootObject );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();	
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendJournalQuestDescriptionEntryStatus( const CGUID& guid, const CGUID& parentGuid, const CName& tag, const String& name, const Int32 stringIndex_description, const Int32 status ) 
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendJournalQuestDescriptionEntryStatus );
#endif
	//! UPDATE JOURNAL OBJECT

	CSecondScreenJournalQuestEntry* journalQuestEntry = GetJournalQuestEntry( CSecondScreenJournalQuestEntry::JQE_QuestDescriptionEntry, guid, parentGuid, tag, name, status );	
	journalQuestEntry->m_properties.Set( TXT( "description_stringIndex" ), String::Printf( TXT("%i"), stringIndex_description ) );

	//! CONSTRUCT JSON OBJECT
	
	CJSONObjectUTF16 jsonQuestObject;
	journalQuestEntry->ConstructJSON( jsonQuestObject );
	
	CJSONObjectUTF16 jsonRootObject;
	jsonRootObject.AddMember( TXT("quest"), jsonQuestObject );

	String message = JsonValueToMessage( jsonRootObject );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();	
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendJournalTrackedQuestUpdated( const CGUID& guid )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendJournalTrackedQuestUpdated );
#endif
	m_trackedQuest = guid;

	String message;
	
	//! CONSTRUCT JSON OBJECT
	
	ConstructTrackedQuestUpdatedJsonMessage( message );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();	
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();
}


void CSecondScreenManager::SendJournalTrackedQuestObjectiveUpdated( const CGUID& guid )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendJournalTrackedQuestObjectiveUpdated );
#endif
	m_trackedQuestObjective = guid;

	String message;
	
	//! CONSTRUCT JSON OBJECT

	ConstructTrackedQuestObjectiveUpdatedJsonMessage( message );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();	
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendFastTravelEnable()
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendFastTravelEnabled );
#endif
	m_fastTravelEnabled = true;

	//! CONSTRUCT JSON OBJECT

	String message;

	ConstructFastTravelEnableJsonMessage( message );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();	
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();
}

void CSecondScreenManager::SendFastTravelDisable()
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_SendFastTravelDisable );
#endif
	m_fastTravelEnabled = false;

	//! CONSTRUCT JSON OBJECT

	String message;

	ConstructFastTravelEnableJsonMessage( message );

	//! SEND MESSAGE TO DEVICES

	m_mutex.Acquire();	
	TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
	TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();
	for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
	{
		CSecondScreenDevice* obj = *it;
		obj->SendMessage( message.AsChar() );		
	}
	m_mutex.Release();
}

void CSecondScreenManager::Init()
{
	if( m_Initialized == false )
	{
		ClearGameData();

	#ifdef RED_PLATFORM_DURANGO
		m_deviceWatcher = new CSecondScreenDeviceWatcherDurango(this);
	#endif // RED_PLATFORM_DURANGO

	#if defined( RED_PLATFORM_WIN64 ) && !defined( RED_FINAL_BUILD )
		m_deviceWatcher = new CSecondScreenDeviceWatcherPC(this);
	#endif 

		if( m_deviceWatcher != NULL )
		{
			m_deviceWatcher->FindAllDevicesAsync();
		}

		m_Initialized = true;
	}
}

void CSecondScreenManager::Shutdown()
{
	if( m_Initialized == true )
	{
		ClearGameData();
		ClearFileNameToAreaMapping();

		if( m_deviceWatcher != NULL )
		{
			delete m_deviceWatcher;
			m_deviceWatcher = NULL;
		}

		TList< CSecondScreenDevice* >::iterator start = m_devicesList.Begin();
		TList< CSecondScreenDevice* >::iterator end = m_devicesList.End();

		for ( TList< CSecondScreenDevice* >::iterator it=start; it!=end; ++it )
		{
			CSecondScreenDevice* obj = *it;
			delete obj;
		}

		m_devicesList.Clear();
		m_Initialized = false;
	}
}

MemSize CSecondScreenManager::EncodeBinaryArray( AnsiChar *dest, const AnsiChar *src, MemSize srcLen ) const
{
	Int32 i;
	AnsiChar *p;

	static const char encode_array[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	p = dest;
	for (i = 0; i < srcLen - 2; i += 3) {
		*p++ = encode_array[(src[i] >> 2) & 0x3F];
		*p++ = encode_array[((src[i] & 0x3) << 4) |
			((Int32) (src[i + 1] & 0xF0) >> 4)];
		*p++ = encode_array[((src[i + 1] & 0xF) << 2) |
			((Int32) (src[i + 2] & 0xC0) >> 6)];
		*p++ = encode_array[src[i + 2] & 0x3F];
	}
	if (i < srcLen) {
		*p++ = encode_array[(src[i] >> 2) & 0x3F];
		if (i == (srcLen - 1)) {
			*p++ = encode_array[((src[i] & 0x3) << 4)];
			*p++ = '=';
		}
		else {
			*p++ = encode_array[((src[i] & 0x3) << 4) |
				((Int32) (src[i + 1] & 0xF0) >> 4)];
			*p++ = encode_array[((src[i + 1] & 0xF) << 2)];
		}
		*p++ = '=';
	}

	*p++ = '\0';
	return (MemSize)( p - dest);
}

MemSize CSecondScreenManager::GetEncodeBinaryArrayLen( MemSize srcLen ) const
{
	return ((srcLen + 2) / 3 * 4) + 1;
}

void CSecondScreenManager::CSecondScreenJournalQuestEntry::ConstructJSON( CJSONObjectUTF16& jsonObject )
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( SS_ConstructJSON );
#endif
	Char guidStringBuffer[guildStringBufferSize];

	m_guid.ToString( guidStringBuffer, guildStringBufferSize );	

	CJSONValueUTF16 questGuid( guidStringBuffer, guildStringBufferSize - 1 );
	jsonObject.AddMember( TXT("q_guid"), questGuid );

	m_parentGuid.ToString( guidStringBuffer, guildStringBufferSize );	
	CJSONValueUTF16 questParentGuid( guidStringBuffer, guildStringBufferSize - 1 );
	jsonObject.AddMember( TXT("q_parent_guid"), questParentGuid );

	const Char* questTypeStr = QuestEntryTypeToString( m_type );
	CJSONValueUTF16 questType( questTypeStr, Red::System::StringLength(questTypeStr) );
	jsonObject.AddMember( TXT("q_type"), questType );

	CJSONValueUTF16 questTag( m_tag.AsChar(), m_tag.AsString().GetLength() );
	jsonObject.AddMember( TXT("q_tag"), questTag );

	CJSONValueUTF16 questName( m_name.AsChar(), m_name.GetLength() );
	jsonObject.AddMember( TXT("q_name"), questName );
	
	const Char* questStatusStr = CSecondScreenManager::JournalQuesStatusToString( m_status );
	CJSONValueUTF16 questStatus( questStatusStr, Red::System::StringLength(questStatusStr) );
	jsonObject.AddMember( TXT("q_status"), questStatus );

	CJSONObjectUTF16 questProperties;
	for( THashMap<String, String>::const_iterator propertiesIterator = m_properties.Begin(); propertiesIterator != m_properties.End(); ++propertiesIterator )
	{
		CJSONValueUTF16 questPropertieValue( propertiesIterator->m_second.AsChar(), propertiesIterator->m_second.GetLength() );
		questProperties.AddMember( propertiesIterator->m_first.AsChar(), questPropertieValue );
	}
	jsonObject.AddMember( TXT("q_properties"), questProperties );
}

const Char* CSecondScreenManager::CSecondScreenJournalQuestEntry::QuestEntryTypeToString( Int32 type )
{
	if( type == JQE_QuestGroup )
	{		 
		return TXT( "JQE_QuestGroup" );
	}
	else if( type == JQE_Quest )
	{		 
		return TXT( "JQE_Quest" );
	}
	else if( type == JQE_QuestPhase )
	{		 
		return TXT( "JQE_QuestPhase" );
	}
	else if( type == JQE_QuestObjective )
	{		 
		return TXT( "JQE_QuestObjective" );
	}
	else if( type == JQE_QuestItemTag )
	{		 
		return TXT( "JQE_QuestItemTag" );
	}
	else if( type == JQE_QuestEnemyTag )
	{		 
		return TXT( "JQE_QuestEnemyTag" );
	}
	else if( type == JQE_QuestMapPin )
	{		 
		return TXT( "JQE_QuestMapPin" );
	}
	else if( type == JQE_QuestDescriptionGroup )
	{		 
		return TXT( "JQE_QuestDescriptionGroup" );
	}
	else if( type == JQE_QuestDescriptionEntry )
	{		 
		return TXT( "JQE_QuestDescriptionEntry" );
	}
	return TXT("");
}

#endif

