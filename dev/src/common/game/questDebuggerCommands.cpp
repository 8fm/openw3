/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#if !defined( NO_DEBUG_SERVER ) 

#include "questDebuggerCommands.h"
#include "../engine/debugServerManager.h"
#include "../engine/debugServerHelpers.h"
#include "../engine/graphConnection.h"
#include "../engine/descriptionGraphBlock.h"

#include "../core/diskFile.h"
#include "../core/depot.h"

#include "questDebuggerPlugin.h"
#include "commonGameResource.h"
#include "questGraph.h"
#include "questGraphSocket.h"
#include "questScopeBlock.h"
#include "questsSystem.h"
#include "questThread.h"
#include "storySceneItems.h"

enum BlockType
{
	BT_QuestBlock,
	BT_DescriptionBlock,
};

enum BlockMessageGroupType
{
	GT_LeftSockets,
	GT_RightSockets,
	GT_Connections,
};

typedef TDynArray< CGraphBlock* >::const_iterator GraphBlockIterator;

#if defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_ORBIS )
const Uint32 LayoutBlocksPerMessage = 60;
const Uint32 DataBlocksPerMessage = 40;
const Uint32 LinesPerMessage = 75;
#else
const Uint32 LayoutBlocksPerMessage = 120;
const Uint32 DataBlocksPerMessage = 80;
const Uint32 LinesPerMessage = 150;
#endif

const String QuestBlockId = ToString( (Uint32)BT_QuestBlock );
const String DescriptionBlockId = ToString( (Uint32)BT_DescriptionBlock );

const String LeftSocketsId = ToString( (Uint32)GT_LeftSockets );
const String RightSocketsId = ToString( (Uint32)GT_RightSockets );
const String ConnectionsId = ToString( (Uint32)GT_Connections );

class IQuestPacket
{
public:
	virtual ~IQuestPacket() {}
	virtual void WriteString( const AnsiChar* data ) = 0;
	virtual void Send() = 0;
	virtual void Clear() = 0;
	virtual void SendEndingMessage( const AnsiChar* blockType ) = 0;
};

class CQuestFilePacket : public IQuestPacket
{
private:
	FILE* m_file;
	const AnsiChar* m_channelName;

public:
	CQuestFilePacket( const AnsiChar* path, const AnsiChar* channelName )
	{
		m_file = fopen( path, "w" );
		fputs( channelName, m_file );		
		putc( 0, m_file );
		m_channelName = channelName;
	}

	~CQuestFilePacket()
	{
		fflush( m_file );
		fclose( m_file );
	}

	void WriteString( const AnsiChar* data ) override
	{
		fputs( data, m_file );
		putc( 0, m_file );
	}

	void Send() override { }
	void Clear() override 
	{
		WriteString( m_channelName );
	}
	void SendEndingMessage( const AnsiChar* blockType ) override
	{
		WriteString( blockType );
	}
};

class CQuestNetworkPacket : public IQuestPacket
{
private:
	Red::Network::ChannelPacket* m_packet;
	const AnsiChar* m_channelName;

public:
	CQuestNetworkPacket( const AnsiChar* channelName )
	{
		m_packet = new Red::Network::ChannelPacket( channelName );
		m_channelName = channelName;
	}

	~CQuestNetworkPacket() 
	{
		delete m_packet;
		m_packet = nullptr;
	}

	void WriteString( const AnsiChar* data ) override
	{
		m_packet->WriteString( data );
	}

	void Send() override
	{
		DBGSRV().Send( m_channelName, *m_packet );
	}

	void Clear() override 
	{
		m_packet->Clear( m_channelName );
	}

	void SendEndingMessage( const AnsiChar* blockType ) 
	{
		Red::Network::ChannelPacket packet( m_channelName );
		packet.WriteString( blockType );
		DBGSRV().Send( m_channelName, packet );
	}
};
     
class CQuestDebuggerHelper
{
	typedef TDynArray< String >::const_iterator StringIterator;
	typedef TDynArray< CGraphSocket* >::const_iterator GraphSocketIterator;
	typedef TDynArray< CGraphConnection* >::const_iterator GraphConnectionIterator;

public:

	static CQuest* GetMainQuestByPointerString( const String& pointerString )
	{
		Uint64 pointerValue = CDebugServerHelpers::GetUidFromString( pointerString );
		return reinterpret_cast<CQuest*>( pointerValue );
	}

	static CQuest* GetMainQuestFromResources( const String& filePath )
	{
		THandle< CQuest > questResource = LoadResource< CQuest >( filePath );

		if ( questResource == nullptr )
		{
			ERR_GAME( TXT( "Quest resource not found. Cannot get the main quest!!!" ) );
			return nullptr;
		}
		return questResource;
	}

	static CQuestPhase* GetPhaseFromResources( const String& filePath )
	{
		THandle< CQuestPhase > questPhase = LoadResource< CQuestPhase >( filePath );

		if ( questPhase == nullptr )
		{
			ERR_GAME( TXT( "Quest phase resource not found. Cannot get the main quest!!!" ) );
			return nullptr;
		}
		return questPhase;
	}

#if !defined( NO_EDITOR_GRAPH_SUPPORT )

	static void WriteQuestBlockLayoutHeader( IQuestPacket& packet, const CQuestGraphBlock* block, const String& parentBlockGUID )
	{		
		packet.WriteString( UNICODE_TO_ANSI( parentBlockGUID.AsChar() ) );
		packet.WriteString( UNICODE_TO_ANSI( ToString( block->GetGUID() ).AsChar() ) );
		//LOG_ENGINE( TXT( "%ls" ), ToString( block->GetGUID() ).AsChar() );
		packet.WriteString( UNICODE_TO_ANSI( block->GetCaption().AsChar() ) );

		const Vector& pos = block->GetPosition();
		packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f" ), pos.X, pos.Y ).AsChar() ) );
		packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%i"), (int)block->GetBlockShape() ).AsChar() ) );

		Uint32 blockColor = block->GetClientColor().ToUint32();
		packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%u"), blockColor ).AsChar() ) );	
		packet.WriteString( UNICODE_TO_ANSI( block->GetComment().AsChar() ) );	
	}

#endif

	static void WriteQuestBlockDataHeader( IQuestPacket& packet, const CQuestGraphBlock* block, const String& parentBlockGUID )
	{		
		const Uint64 blockPointer = reinterpret_cast<Uint64>( block );
		packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( blockPointer ).AsChar() ) );
		packet.WriteString( UNICODE_TO_ANSI( parentBlockGUID.AsChar() ) );
		packet.WriteString( UNICODE_TO_ANSI( ToString( block->GetGUID() ).AsChar() ) );
	}

#if !defined( NO_EDITOR_GRAPH_SUPPORT )

	static void WriteDescriptionBlockLayoutHeader( IQuestPacket& packet, const CDescriptionGraphBlock* block, const String& parentBlockGUID )
	{
		packet.WriteString( UNICODE_TO_ANSI( parentBlockGUID.AsChar() ) );
		packet.WriteString( UNICODE_TO_ANSI( block->GetCaption().AsChar() ) );

		const Vector& pos = block->GetPosition();
		packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%0.3f %0.3f" ), pos.X, pos.Y ).AsChar() ) );

		Uint32 blockColor = block->GetClientColor().ToUint32();
		packet.WriteString( UNICODE_TO_ANSI( String::Printf( TXT( "%u"), blockColor ).AsChar() ) );	
		packet.WriteString( UNICODE_TO_ANSI( block->GetDescriptionText().AsChar() ) );	
	}

#endif

	static void WriteDescriptionBlockDataHeader( IQuestPacket& packet, const CDescriptionGraphBlock* block, const String& parentBlockGUID )
	{
		const Uint64 blockPointer = reinterpret_cast<Uint64>( block );
		packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( blockPointer ).AsChar() ) );
		packet.WriteString( UNICODE_TO_ANSI( parentBlockGUID.AsChar() ) );
	}

#if !defined( NO_EDITOR_GRAPH_SUPPORT )

	static void WriteLeftLayoutSockets( IQuestPacket& packet, const CQuestGraphBlock* block )
	{
		//PC_SCOPE( "WriteLeftSockets" )
		const TDynArray< CGraphSocket* >& blockSockets = block->GetSockets();
		const GraphSocketIterator blockSocketsEndIt = blockSockets.End();
		Uint32 socketsCount = 0;
		for ( GraphSocketIterator socketIt = blockSockets.Begin(); socketIt != blockSocketsEndIt; ++socketIt )
		{
			CGraphSocket* socket = *socketIt;
			if ( socket->GetPlacement() == LSP_Left && socket->HasConnections() )
				socketsCount++;
		}

		packet.WriteString( UNICODE_TO_ANSI( LeftSocketsId.AsChar() ) );
		packet.WriteString( UNICODE_TO_ANSI( ToString( socketsCount ).AsChar() ) );

		Uint32 socketIndex = 0;
		for ( GraphSocketIterator socketIt = blockSockets.Begin(); socketIt != blockSocketsEndIt; ++socketIt )
		{
			CGraphSocket* socket = *socketIt;
			if ( socket->GetPlacement() == LSP_Left && socket->HasConnections() )
			{
				const Uint64 socketPointer = reinterpret_cast<Uint64>( socket );
				packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( socketPointer ).AsChar() ) );
				packet.WriteString( UNICODE_TO_ANSI( socket->GetName().AsChar() ) );
				packet.WriteString( UNICODE_TO_ANSI( ToString( socketIndex ).AsChar() ) );
			}
			socketIndex++;
		}
	}

#endif
	
	static void WriteDataSockets( IQuestPacket& packet, const CQuestGraphBlock* block )
	{
		const TDynArray< CGraphSocket* >& blockSockets = block->GetSockets();
		const GraphSocketIterator blockSocketsEndIt = blockSockets.End();

		Uint32 socketIndex = 0;
		for ( GraphSocketIterator socketIt = blockSockets.Begin(); socketIt != blockSocketsEndIt; ++socketIt )
		{
			CGraphSocket* socket = *socketIt;
			if ( socket->HasConnections() )
			{
				const Uint64 socketPointer = reinterpret_cast<Uint64>( socket );
				packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( socketPointer ).AsChar() ) );
				packet.WriteString( UNICODE_TO_ANSI( ToString( socketIndex ).AsChar() ) );
			}
			socketIndex++;
		}
	}

#if !defined( NO_EDITOR_GRAPH_SUPPORT )

	static void WriteRightLayoutSocketsAndOutgoingConnections( IQuestPacket& packet, const CQuestGraphBlock* block )
	{
		//PC_SCOPE( "WriteRightSocketsAndOutgoingConnections" )
		TDynArray< CGraphConnection* > outgoingConnections;
		outgoingConnections.Reserve( 10 );
		const TDynArray< CGraphSocket* >& blockSockets = block->GetSockets();
		const GraphSocketIterator blockSocketsEndIt = blockSockets.End();
		Uint32 socketsCount = 0;
		for ( GraphSocketIterator socketIt = blockSockets.Begin(); socketIt != blockSocketsEndIt; ++socketIt )
		{
			CGraphSocket* socket = *socketIt;
			if ( socket->GetPlacement() == LSP_Right && socket->HasConnections() )
			{
				socketsCount++;

				const TDynArray< CGraphConnection* >& socketConnections = socket->GetConnections();
				for ( GraphConnectionIterator connectionIt = socketConnections.Begin(); connectionIt != socketConnections.End(); ++connectionIt )
					outgoingConnections.PushBack( *connectionIt );
			}
		}

		packet.WriteString( UNICODE_TO_ANSI( RightSocketsId.AsChar() ) );
		packet.WriteString( UNICODE_TO_ANSI( ToString( socketsCount ).AsChar() ) );
		Uint32 socketIndex = 0;
		for ( GraphSocketIterator socketIt = blockSockets.Begin(); socketIt != blockSocketsEndIt; ++socketIt )
		{
			CGraphSocket* socket = *socketIt;
			if ( socket->GetPlacement() == LSP_Right && socket->HasConnections() )
			{
				const Uint64 socketPointer = reinterpret_cast<Uint64>( socket );
				packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( socketPointer ).AsChar() ) );
				packet.WriteString( UNICODE_TO_ANSI( socket->GetName().AsChar() ) );
				packet.WriteString( UNICODE_TO_ANSI( ToString( socketIndex ).AsChar() ) );
			}			
			socketIndex++;
		}

		packet.WriteString( UNICODE_TO_ANSI( ConnectionsId.AsChar() ) );
		packet.WriteString( UNICODE_TO_ANSI( ToString( outgoingConnections.Size() ).AsChar() ) );

		const GraphConnectionIterator outgoingConnectionsEndIt = outgoingConnections.End();
		for ( GraphConnectionIterator connectionIt = outgoingConnections.Begin(); connectionIt != outgoingConnectionsEndIt; ++connectionIt )
		{
			CGraphConnection* connection = *connectionIt;
			const Uint64 connectionPointer = reinterpret_cast<Uint64>( connection );
			Bool isActive = connection->IsActive();
			const Uint64 sourceSocketPointer = reinterpret_cast<Uint64>( connection->GetSource( true ) );
			const Uint64 destSocketPointer = reinterpret_cast<Uint64>( connection->GetDestination( true ) );	
			packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( connectionPointer ).AsChar() ) );
			packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( sourceSocketPointer ).AsChar() ) );
			packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( destSocketPointer ).AsChar() ) );
			packet.WriteString( isActive ? "1" : "0" );
		}
	}

#endif

	static void SendEndingMessage( const char* channelName, const AnsiChar* blockType )
	{
		Red::Network::ChannelPacket packet( channelName );
		packet.WriteString( blockType );
		SendMessage( channelName, packet );
	}

	static void SendMessage( const char* channelName, Red::Network::ChannelPacket& packet )
	{
		Red::Network::Manager::GetInstance()->Send( channelName, packet );
	}

	template< typename T >
	static const T* GetPropertyValue( const CProperty* property, void* object )
	{
		const T* value = (const T*)property->GetOffsetPtr( object );
		return value;
	}

	static void SendLayoutRecursive( CQuestGraph* graph, const String& parentGUID, Uint32& messagesInPacket, Uint32& packetsCount, IQuestPacket& packet )
	{
#if !defined( NO_EDITOR_GRAPH_SUPPORT )
		//PC_SCOPE( "SendLayoutRecursive" )
		if ( graph == nullptr )
		{
			RED_LOG( "QuestDebugger", TXT( "Graph is null in questPhase GUID = %ls" ), parentGUID.AsChar() );
		}
		else 
		{
			const TDynArray< CGraphBlock* >& blocks = graph->GraphGetBlocks();
			GraphBlockIterator blocksEnd = blocks.End();
			for ( GraphBlockIterator it = blocks.Begin(); it != blocksEnd; ++it )
			{
				CGraphBlock* block = *it;
				if ( block == nullptr )
				{
					RED_LOG( "QuestDebugger", TXT( "%i: NULL quest block in questPhase GUID = %ls" ), parentGUID.AsChar() );
					continue;
				}

				const Bool isQuestGraphBlock = block->IsA< CQuestGraphBlock >();
				const Bool isDescriptionGraphBlock = isQuestGraphBlock ? false : block->IsA< CDescriptionGraphBlock >();
				if ( !isQuestGraphBlock && !isDescriptionGraphBlock )
					continue;

				packet.WriteString( "questBlockLayout" );
				packet.WriteString( UNICODE_TO_ANSI( block->GetClass()->GetName().AsChar() ) );

				if ( isQuestGraphBlock )
				{
					CQuestGraphBlock* questGraphBlock = static_cast< CQuestGraphBlock* >( block );
					CQuestDebuggerHelper::WriteQuestBlockLayoutHeader( packet, questGraphBlock, parentGUID );
					CQuestDebuggerHelper::WriteLeftLayoutSockets( packet, questGraphBlock );
					CQuestDebuggerHelper::WriteRightLayoutSocketsAndOutgoingConnections( packet, questGraphBlock );
				}
				else if ( isDescriptionGraphBlock )
				{
					CDescriptionGraphBlock* descriptionGraphBlock = static_cast< CDescriptionGraphBlock* >( block );
					CQuestDebuggerHelper::WriteDescriptionBlockLayoutHeader( packet, descriptionGraphBlock, parentGUID );
				}

				++messagesInPacket;
				if ( messagesInPacket % LayoutBlocksPerMessage == 0 )
				{
					packet.Send();
					packet.Clear();
					++packetsCount;
					messagesInPacket = 0;
				}

				if ( !block->IsA< CQuestScopeBlock >() )
					continue;

				CQuestScopeBlock* scope = static_cast< CQuestScopeBlock* >( block );
				const String guid = ToString( scope->GetGUID() );
				SendLayoutRecursive( scope->GetGraph(), guid, messagesInPacket, packetsCount, packet );
			}
		}

		if ( messagesInPacket != 0 )
		{
			packet.Send();
			packet.Clear();
			++packetsCount;
			messagesInPacket = 0;
		}		
#endif
	}

	static void SendDataRecursive( CQuestGraph* graph, const String& parentGUID, Uint32& messagesInPacket, Uint32& packetsCount, IQuestPacket& packet )
	{
		//PC_SCOPE( "SendDataRecursive" )
		if ( graph == nullptr )
		{
			RED_LOG( "QuestDebugger", TXT( "Graph is null in questPhase GUID = %ls" ), parentGUID.AsChar() );
		}
		else
		{
			const TDynArray< CGraphBlock* >& blocks = graph->GraphGetBlocks();
			GraphBlockIterator blocksEnd = blocks.End();
			for ( GraphBlockIterator it = blocks.Begin(); it != blocksEnd; ++it )
			{
				CGraphBlock* block = *it;
				if ( block == nullptr )
				{
					RED_LOG( "QuestDebugger", TXT( "NULL quest block in questPhase GUID = %ls" ), parentGUID.AsChar() );
					continue;
				}
				const Bool isQuestGraphBlock = block->IsA< CQuestGraphBlock >();
				const Bool isDescriptionGraphBlock = isQuestGraphBlock ? false : block->IsA< CDescriptionGraphBlock >();
				if ( !isQuestGraphBlock && !isDescriptionGraphBlock )
					continue;

				packet.WriteString( "questBlockData" );
				packet.WriteString( UNICODE_TO_ANSI( block->GetClass()->GetName().AsChar() ) );

				if ( isQuestGraphBlock )
				{
					CQuestGraphBlock* questGraphBlock = static_cast< CQuestGraphBlock* >( block );
					CQuestDebuggerHelper::WriteQuestBlockDataHeader( packet, questGraphBlock, parentGUID );
					CQuestDebuggerHelper::WriteDataSockets( packet, questGraphBlock );
				}
				else if ( isDescriptionGraphBlock )
				{
					CDescriptionGraphBlock* descriptionGraphBlock = static_cast< CDescriptionGraphBlock* >( block );
					CQuestDebuggerHelper::WriteDescriptionBlockDataHeader( packet, descriptionGraphBlock, parentGUID );
				}

				++messagesInPacket;
				if ( messagesInPacket % DataBlocksPerMessage == 0 )
				{
					packet.Send();
					packet.Clear();
					++packetsCount;
					messagesInPacket = 0;
				}
	
				if ( !block->IsA< CQuestScopeBlock >() )
					continue;

				CQuestScopeBlock* scope = static_cast< CQuestScopeBlock* >( block );
				const String guid = ToString( scope->GetGUID() );
				SendDataRecursive( scope->GetGraph(), guid, messagesInPacket, packetsCount, packet );
			}
		}

		if ( messagesInPacket != 0 )
		{
			packet.Send();
			packet.Clear();
			++packetsCount;
			messagesInPacket = 0;
		}		
	}
};

class CDebugObjectProperties
{
public:
	static void SendPacketIfNeeded( Uint32& linesInPacket, Uint32& packetsCount, Red::Network::ChannelPacket& packet )
	{
		if ( linesInPacket % LinesPerMessage == 0 )
		{
			CQuestDebuggerHelper::SendMessage( RED_NET_CHANNEL_DEBUG_SERVER, packet );
			packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
			++packetsCount;
			packet.WriteString( "blockProperties" );
			linesInPacket = 1;
		}
	}

	static void WriteSimpleValue( Uint32& linesInPacket, Uint32& packetsCount, Red::Network::ChannelPacket& packet, const void* object, const IRTTIType* type )
	{
		String value;
		type->ToString( object, value );
		const String& finalString = String::Printf( TXT( "%ls %ls" ), type->GetName().AsChar(), value.AsChar() );
		packet.WriteString( UNICODE_TO_ANSI( finalString.AsChar() ) ); ++linesInPacket;
		SendPacketIfNeeded( linesInPacket, packetsCount, packet );
		//LOG_ENGINE( TXT( "%ls" ), finalString.AsChar() );
	}

	static void WritePropertyHeader( Uint32& linesInPacket, Uint32& packetsCount, Red::Network::ChannelPacket& packet, const CProperty* property )
	{
		String hint = TXT( "" );
#ifndef NO_EDITOR_PROPERTY_SUPPORT
		hint = property->GetHint();
#endif
		const String& finalString = String::Printf( TXT( "%ls %ls %ls" ), property->GetParent()->GetName().AsChar(), property->GetName().AsChar(), hint.AsChar() );
		packet.WriteString( UNICODE_TO_ANSI( finalString.AsChar() ) ); ++linesInPacket;
		SendPacketIfNeeded( linesInPacket, packetsCount, packet );
		//LOG_ENGINE( TXT( "%ls" ), finalString.AsChar() );
	}

	static void WriteDynamicPropertyHeader( Uint32& linesInPacket, Uint32& packetsCount, Red::Network::ChannelPacket& packet, const CName& className, const CName& propertyName )
	{
		String hint = TXT( "" );

		const String& finalString = String::Printf( TXT( "%ls %ls %ls" ), className.AsChar(), propertyName.AsChar(), hint.AsChar() );
		packet.WriteString( UNICODE_TO_ANSI( finalString.AsChar() ) ); ++linesInPacket;
		SendPacketIfNeeded( linesInPacket, packetsCount, packet );
		//LOG_ENGINE( TXT( "%ls" ), finalString.AsChar() );
	}

	static void WriteObjectProperties( Uint32& linesInPacket, Uint32& packetsCount, Red::Network::ChannelPacket& packet, const void* object, const IRTTIType* objectType, const String& resourcePath = String::EMPTY )
	{
		ERTTITypeType typeType = objectType->GetType();
		if ( typeType == RT_Simple || typeType == RT_Fundamental )
		{
			WriteSimple( linesInPacket, packetsCount, packet, object, objectType );
			return;
		}

		if ( typeType == RT_Enum )
		{
			WriteEnum( linesInPacket, packetsCount, packet, object, objectType );
			return;
		}

		if ( typeType == RT_Class )
		{			
			WriteClass( linesInPacket, packetsCount, packet, object, objectType, resourcePath );
			return;
		}

		if ( typeType == RT_Array )
		{
			WriteArray( linesInPacket, packetsCount, packet, object, objectType );
			return;
		}

		if ( typeType == RT_Pointer )
		{
			WritePointer( linesInPacket, packetsCount, packet, object, objectType );
			return;
		}
		
		if ( typeType == RT_Handle )
		{
			WriteHandle( linesInPacket, packetsCount, packet, object, objectType );
			return;
		}
		
		if ( typeType == RT_SoftHandle )
		{
			WriteSoftHandle( linesInPacket, packetsCount, packet, object, objectType );
		}
	}

	static void WriteSimple( Uint32& linesInPacket, Uint32& packetsCount, Red::Network::ChannelPacket& packet, const void* object, const IRTTIType* objectType )
	{
		WriteSimpleValue( linesInPacket, packetsCount, packet, object, objectType );
		SendPacketIfNeeded( linesInPacket, packetsCount, packet );
	}

	static void WriteEnum( Uint32& linesInPacket, Uint32& packetsCount, Red::Network::ChannelPacket& packet, const void* object, const IRTTIType* objectType )
	{
		String value;
		objectType->ToString( object, value );
		const String& finalString = String::Printf( TXT( "String %ls" ), value.AsChar() );
		packet.WriteString( UNICODE_TO_ANSI( finalString.AsChar() ) ); ++linesInPacket;
		//LOG_ENGINE( TXT( "%ls" ), finalString.AsChar() );
		SendPacketIfNeeded( linesInPacket, packetsCount, packet );
	}

	static void WriteClass( Uint32& linesInPacket, Uint32& packetsCount, Red::Network::ChannelPacket& packet, const void* object, const IRTTIType* objectType, const String& resourcePath = String::EMPTY )
	{
		const String& objectHeader = String::Printf( TXT( "[ %ls" ), objectType->GetName().AsChar() );
		packet.WriteString( UNICODE_TO_ANSI( objectHeader.AsChar() ) ); ++linesInPacket;
		//LOG_ENGINE( TXT( "%ls" ), objectHeader.AsChar() );

		const CClass *classPtr = static_cast< const CClass* >( objectType );
		TDynArray< CProperty* > properties;
		classPtr->GetProperties( properties );

		if ( !resourcePath.Empty() )
		{
			const String& finalString = String::Printf( TXT( "%ls resource_path Resource path" ), classPtr->GetName().AsChar() );
			packet.WriteString( UNICODE_TO_ANSI( finalString.AsChar() ) ); ++linesInPacket;
			packet.WriteString( UNICODE_TO_ANSI( resourcePath.AsChar() ) ); ++linesInPacket;
			//LOG_ENGINE( TXT( "%ls" ), finalString.AsChar() );
			//LOG_ENGINE( TXT( "%ls" ), resourcePath.AsChar() );
		}

		typedef TDynArray< CProperty* >::const_iterator PropertyIterator;
		const PropertyIterator endIt = properties.End();
		for ( PropertyIterator propertyIt = properties.Begin(); propertyIt != endIt; ++propertyIt )
		{
			const CProperty* property = *propertyIt;
			if ( !property->IsEditable() )
				continue;

			WritePropertyHeader( linesInPacket, packetsCount, packet, property );

			const void* ptrData = property->GetOffsetPtr( object );
			const IRTTIType* propertyType = property->GetType();
			WriteObjectProperties( linesInPacket, packetsCount, packet, ptrData, propertyType );
		}

		if ( classPtr->IsObject() )
		{
			WriteDynamicProperties( linesInPacket, packetsCount, packet, object, classPtr->GetName() );
		}

		packet.WriteString( "]" ); ++linesInPacket;
		//LOG_ENGINE( TXT( "}" ) );
	}

	static void WriteDynamicProperties( Uint32& linesInPacket, Uint32& packetsCount, Red::Network::ChannelPacket& packet, const void* object, const CName& className )
	{
		const CObject* objectToCheckForDynamicProperties = static_cast< const CObject* >( object );
		const IDynamicPropertiesSupplier* dynamicPropertiesSupplier = objectToCheckForDynamicProperties->QueryDynamicPropertiesSupplier();
		if ( dynamicPropertiesSupplier == nullptr )
			return;

		TDynArray< CName > dynamicProperties;
		dynamicPropertiesSupplier->GetDynamicProperties( dynamicProperties );

		const Uint32 dynamicPropertiesCount = dynamicProperties.Size();
		for ( Uint32 i = 0; i < dynamicPropertiesCount; ++i )
		{
			CName propertyName = dynamicProperties[i];
			CVariant propertyValue;
			if ( dynamicPropertiesSupplier->ReadDynamicProperty( propertyName, propertyValue ) )
			{
				if ( !propertyValue.IsValid() )
					continue;

				WriteDynamicPropertyHeader( linesInPacket, packetsCount, packet, className, propertyName );
				WriteObjectProperties( linesInPacket, packetsCount, packet, propertyValue.GetData(), propertyValue.GetRTTIType() );
			}
		}
	}

	static void WriteArray( Uint32& linesInPacket, Uint32& packetsCount, Red::Network::ChannelPacket& packet, const void* object, const IRTTIType* objectType )
	{
		const IRTTIBaseArrayType* arrayType = static_cast< const IRTTIBaseArrayType* >( objectType );
		IRTTIType* arrayInnerType = arrayType->ArrayGetInnerType();
		const Uint32 count = arrayType->ArrayGetArraySize( object );

		const String& arrayHeader = String::Printf( TXT( "{ %ls %u" ), arrayType->GetName().AsChar(), count );
		packet.WriteString( UNICODE_TO_ANSI( arrayHeader.AsChar() ) ); ++linesInPacket;
		//LOG_ENGINE( TXT( "%ls" ), arrayHeader.AsChar() );

		for ( Uint32 i = 0; i < count; ++i )
		{
			const void* element = arrayType->ArrayGetArrayElement( object, i );				
			WriteObjectProperties( linesInPacket, packetsCount, packet, element, arrayInnerType );
		}
		packet.WriteString( "}" ); ++linesInPacket;
		//LOG_ENGINE( TXT( "}" ) );
	}

	static void WritePointer( Uint32& linesInPacket, Uint32& packetsCount, Red::Network::ChannelPacket& packet, const void* object, const IRTTIType* objectType )
	{
		const CRTTIPointerType* pointerType = static_cast< const CRTTIPointerType* >( objectType );
		const CObject* pointedObject = static_cast< const CObject* >( pointerType->GetPointed( object ) );
		if ( pointedObject == nullptr )
		{
			packet.WriteString( "String NULL" ); ++linesInPacket;
			//LOG_ENGINE( TXT( "String NULL" ) );
			return;
		}

		const IRTTIType* pointedType = pointedObject->GetClass();
		WriteObjectProperties( linesInPacket, packetsCount, packet, pointedObject, pointedType );			
	}

	static void WriteHandle( Uint32& linesInPacket, Uint32& packetsCount, Red::Network::ChannelPacket& packet, const void* object, const IRTTIType* objectType )
	{
		String value;
		objectType->ToString( object, value );
		const String& resourcePath = String::Printf( TXT( "String %ls" ), value.AsChar() );

		const CRTTIHandleType* pointerType = static_cast< const CRTTIHandleType* >( objectType );
		const CObject* pointedObject = static_cast< const CObject* >( pointerType->GetPointed( (void*)object ) );
		if ( pointedObject == nullptr )
		{
			packet.WriteString( "String NULL" ); ++linesInPacket;
			//LOG_ENGINE( TXT( "String NULL" ) );
			return;
		}

		const IRTTIType* pointedType = pointedObject->GetClass();
		WriteObjectProperties( linesInPacket, packetsCount, packet, pointedObject, pointedType, resourcePath );			
	}

	static void WriteSoftHandle( Uint32& linesInPacket, Uint32& packetsCount, Red::Network::ChannelPacket& packet, const void* object, const IRTTIType* objectType )
	{
		String value;
		objectType->ToString( object, value );
		const String& resourcePath = String::Printf( TXT( "String %ls" ), value.AsChar() );

		const CRTTISoftHandleType* pointerType = static_cast< const CRTTISoftHandleType* >( objectType );
		const CObject* pointedObject = static_cast< const CObject* >( pointerType->GetPointed( (void*)object ) );
		if ( pointedObject == nullptr )
		{
			packet.WriteString( "String NULL" ); ++linesInPacket;
			//LOG_ENGINE( TXT( "String NULL" ) );
			return;
		}

		const IRTTIType* pointedType = pointedObject->GetClass();
		WriteObjectProperties( linesInPacket, packetsCount, packet, pointedObject, pointedType, resourcePath );			
	}
};

Uint32 CDebugServerCommandGetQuestLayout::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	//PC_SCOPE( "GetQuestLayout" )
	if ( data.Empty() )
		return 0;

	const String& questResourcePath = data[0];
	CQuest* mainQuest = CQuestDebuggerHelper::GetMainQuestFromResources( questResourcePath );
	if ( mainQuest == nullptr )
		return 0;
	
	CQuestGraph* mainQuestGraph = mainQuest->GetGraph();		

	Uint32 packetsCount = 0;

	const String mainQuestId = TXT( "0" );
	CQuestNetworkPacket packet( RED_NET_CHANNEL_RES_SERVER );

	Uint32 messagesInPacket = 0;
#if !defined( NO_EDITOR_GRAPH_SUPPORT )
	CQuestDebuggerHelper::SendLayoutRecursive( mainQuestGraph, mainQuestId, messagesInPacket, packetsCount, packet );
#endif
	packet.SendEndingMessage( "questBlockLayout" );

	return ++packetsCount;
}


Uint32 CDebugServerCommandGetQuestData::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	//PC_SCOPE( "GetQuestData" );
	if ( !GGame->GetActiveWorld() )
		return 0;

	if ( data.Empty() )
		return 0;

	const String& questPointerString = data[0];
	CQuest* mainQuest = CQuestDebuggerHelper::GetMainQuestByPointerString( questPointerString );
	if ( mainQuest == nullptr )
		return 0;

	CQuestGraph* mainQuestGraph = mainQuest->GetGraph();		

	Uint32 packetsCount = 0;

	const String mainQuestId = TXT( "0" );

	CQuestNetworkPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );

	Uint32 messagesInPacket = 0;
	CQuestDebuggerHelper::SendDataRecursive( mainQuestGraph, mainQuestId, messagesInPacket, packetsCount, packet );
	packet.SendEndingMessage( "questBlockData" );

	return ++packetsCount;
}



Uint32 CDebugServerCommandToggleConnection::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	//
	// TODO
	//
	//if ( !GGame->GetActiveWorld() )
	//	return 0;

	//const String& arg = data[0];
	//Uint64 uid = CDebugServerHelpers::GetUidFromString( arg );

	//CGraphConnection* graphConnection = reinterpret_cast<CGraphConnection*>( uid );
	//if ( graphConnection != nullptr )
	//	graphConnection->SetActive( !graphConnection->IsActive() );

	return 0;
}



Uint32 CDebugServerCommandToggleBreakpoint::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{	
	if ( !GGame->GetActiveWorld() )
		return 0;

	Uint64 pointer = CDebugServerHelpers::GetUidFromString( data[0] );
	CGUID guid = CGUID::Create( ( data[1] ).AsChar() );

	CQuestDebuggerPlugin* questDebuggerPlugin = static_cast< CQuestDebuggerPlugin* >( owner );
	if ( questDebuggerPlugin == nullptr )
		return 0;

	questDebuggerPlugin->ToggleBreakpoint( pointer, guid );
	return 0;
}

Uint32 CDebugServerCommandStartInteractionDialog::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	if ( !GGame->GetActiveWorld() )
		return 0;

	CQuestDebuggerPlugin* questDebuggerPlugin = static_cast< CQuestDebuggerPlugin* >( owner );
	if ( questDebuggerPlugin == nullptr )
		return 0;

	Uint64 pointer = CDebugServerHelpers::GetUidFromString( data[0] );
	questDebuggerPlugin->StartInteractionDialog( pointer );

	return 0;
}


Uint32 CDebugServerCommandContinueFromBreakpoint::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{	
	if ( !GGame->GetActiveWorld() )
		return 0;

	CQuestDebuggerPlugin* questDebuggerPlugin = static_cast< CQuestDebuggerPlugin* >( owner );
	if ( questDebuggerPlugin == nullptr )
		return 0;

	questDebuggerPlugin->ContinueFromBreakpoint();
	return 0;
}


Uint32 CDebugServerCommandContinueFromPin::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{	
	if ( !GGame->GetActiveWorld() )
		return 0;

	CQuestDebuggerPlugin* questDebuggerPlugin = static_cast< CQuestDebuggerPlugin* >( owner );
	if ( questDebuggerPlugin == nullptr )
		return 0;

	const Uint64 questThreadAddress = CDebugServerHelpers::GetUidFromString( data[0] );
	const Uint64 questBlockAddress = CDebugServerHelpers::GetUidFromString( data[1] );
	const String& socketName = data[2];
	const String& socketDirection = data[3];

	CQuestThread* questThread = reinterpret_cast< CQuestThread* >( questThreadAddress );
	if ( questThread == nullptr )
		return 0;

	CQuestGraphBlock* questGraphBlock = reinterpret_cast< CQuestGraphBlock* >( questBlockAddress );
	if ( questGraphBlock == nullptr )
		return 0;

	if ( socketName.Empty() )
		return 0;

	if ( socketDirection.Empty() )
		return 0;

	questDebuggerPlugin->ContinueFromPin( questThread, questGraphBlock, socketName, socketDirection );

	return 0;
}


Uint32 CDebugServerCommandKillSignal::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	if ( !GGame->GetActiveWorld() )
		return 0;

	CQuestDebuggerPlugin* questDebuggerPlugin = static_cast< CQuestDebuggerPlugin* >( owner );
	if ( questDebuggerPlugin == nullptr )
		return 0;

	const Uint64 questThreadAddress = CDebugServerHelpers::GetUidFromString( data[0] );
	const Uint64 questBlockAddress = CDebugServerHelpers::GetUidFromString( data[1] );

	CQuestThread* questThread = reinterpret_cast< CQuestThread* >( questThreadAddress );
	if ( questThread == nullptr )
		return 0;

	CQuestGraphBlock* questGraphBlock = reinterpret_cast< CQuestGraphBlock* >( questBlockAddress );
	if ( questGraphBlock == nullptr )
		return 0;


	questDebuggerPlugin->KillSignal( questThread, questGraphBlock );

	return 0;
}


Uint32 CDebugServerCommandGetQuestBlockProperties::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	if ( !GGame->GetActiveWorld() )
		return 0;

	const String& arg = data[0];
	Uint64 uid = CDebugServerHelpers::GetUidFromString( arg );

	CObject* questBlock = reinterpret_cast<CObject*>( uid );
	if ( questBlock == nullptr )
		return 0;

	Uint32 packetsCount = 0;
	Uint32 linesInPacket = 0;
		
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	packet.WriteString( "blockProperties" ); ++linesInPacket;
	CDebugObjectProperties::WriteObjectProperties( linesInPacket, packetsCount, packet, questBlock, questBlock->GetClass() );
	if ( linesInPacket != 0 )
	{
		CQuestDebuggerHelper::SendMessage( RED_NET_CHANNEL_DEBUG_SERVER, packet );
		packet.Clear( RED_NET_CHANNEL_DEBUG_SERVER );
		++packetsCount;
		linesInPacket = 0;
	}		
	CQuestDebuggerHelper::SendEndingMessage( RED_NET_CHANNEL_DEBUG_SERVER, "blockProperties" );

	return packetsCount;
}


Uint32 CDebugServerCommandGetCallstack::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	if ( !GGame->GetActiveWorld() )
		return 0;

	CQuestDebuggerPlugin* questDebuggerPlugin = static_cast< CQuestDebuggerPlugin* >( owner );
	if ( questDebuggerPlugin == nullptr )
		return 0;

	questDebuggerPlugin->SendCallstack();
	return 0;
}


//void ObjectPropertiesTest::Test()
//{
//	//const String path = TXT( "quests\\part_1\\q103_daughter.w2phase" );
//	const String path = TXT( "quests\\witcher3_structure.w2phase" );	
//	CQuestPhase* questPhase = CQuestDebuggerHelper::GetPhaseFromResources( path );
//	if ( questPhase == nullptr )
//		return;
//
//	CQuestGraph* mainQuestGraph = questPhase->GetGraph();	
//	//Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
//	//for ( GraphBlockIterator it = blocks.Begin(); it != blocksEnd; ++it )
//	//{
//	//	CGraphBlock* graphBlock = *it;
//	//	if ( !graphBlock->IsA< CQuestScopeBlock >() )
//	//		continue;
//
//	//	CQuestScopeBlock* scope = static_cast< CQuestScopeBlock* >( graphBlock );
//	//	mainQuestGraph = scope->GetGraph();
//	//	const String guid = ToString( scope->GetGUID() );
//	//	if ( guid == TXT( "66E200F6-4DBEEC71-13AB05BA-7353DF4F" ) )
//	//	{
//	//		/*RED_LOG( "ERROR", TXT( "Something wrong with questPhase GUID = %ls" ), guid.AsChar() );
//	//		RED_LOG( "ERROR", TXT( "%p" ), graph );
//	//		const TDynArray< CGraphBlock* >& graphBlocks = graph->GraphGetBlocks();
//	//		GraphBlockIterator graphBlocksEnd = graphBlocks.End();
//	//		Uint32 count = 0;
//	//		for ( GraphBlockIterator it = graphBlocks.Begin(); it != graphBlocksEnd; ++it )
//	//		{
//	//			CGraphBlock* graphBlock = *it;
//	//			RED_LOG( "ERROR", TXT( "%i: %p" ), ++count, graphBlock );
//	//		}*/
//	//	}
//	//}
//}

Bool CQuestLayoutDumper::Dump( const String& questPath, const String& dumpPath )
{
	CQuest* mainQuest = CQuestDebuggerHelper::GetMainQuestFromResources( questPath );
	if ( mainQuest == nullptr )
		return false;

	CQuestGraph* mainQuestGraph = mainQuest->GetGraph();		

	Uint32 packetsCount = 0;

	const String mainQuestId = TXT( "0" );	
	
	CQuestFilePacket packet( UNICODE_TO_ANSI( dumpPath.AsChar() ), RED_NET_CHANNEL_RES_SERVER );

	Uint32 messagesInPacket = 0;
	CQuestDebuggerHelper::SendLayoutRecursive( mainQuestGraph, mainQuestId, messagesInPacket, packetsCount, packet );
	packet.SendEndingMessage( "questBlockLayout" );
	return true;
}

void GetDirectoriesByName( const CDirectory* directory, const String& searchedDirectoryName, TDynArray< CDirectory* >& foundDirectories )
{
	const TDirs& subDirectories = directory->GetDirectories();
	for ( auto dir : subDirectories )
	{
		if ( dir->GetName() == searchedDirectoryName )
		{
			foundDirectories.PushBack( dir );
		}
		else
		{
			GetDirectoriesByName( dir, searchedDirectoryName, foundDirectories );
		}
	}
}

Bool CQuestAppearancesDumper::Dump( const String& dumpPath, const String& type, void (*logCallback)( const String& ) )
{
	TDynArray< CDiskFile* > entities;
	if ( type == TXT( "characters" ) )
	{
		GetQuestEntities( entities );
		ProcessQuestEntities( dumpPath, entities, logCallback );
	}
	else if ( type == TXT( "scenes") )
	{
		GetScenesEntities( entities );
		ProcessSceneEntities( dumpPath, entities, logCallback );
	}

	return true;
}

void CQuestAppearancesDumper::ProcessQuestEntities( const String& dumpPath, TDynArray< CDiskFile* >& entities, void (*logCallback)( const String& ) )
{
	const Uint32 entitiesCount = entities.Size();
	Uint32 current = 0;
	String outputCSV = TXT( "Entity file;Appearance name;Is enabled\n" );
	for ( auto entityFile : entities )
	{		
		String logMessage = String::Printf( TXT( "(%i/%i) %ls" ), ++current, entitiesCount, entityFile->GetAbsolutePath().AsChar() );
		logCallback( logMessage );
		ProcessEntityFile( outputCSV, entityFile );
	}

	GFileManager->SaveStringToFileWithUTF8( dumpPath, outputCSV );
}

void CQuestAppearancesDumper::ProcessSceneEntities( const String& dumpPath, TDynArray< CDiskFile* >& entities, void (*logCallback)( const String& ) )
{
	const Uint32 entitiesCount = entities.Size();
	Uint32 current = 0;
	String outputCSV = TXT( "Entity file;Actor name;Appearance name\n" );
	for ( auto entityFile : entities )
	{		
		String logMessage = String::Printf( TXT( "(%i/%i) %ls" ), ++current, entitiesCount, entityFile->GetAbsolutePath().AsChar() );
		logCallback( logMessage );
		ProcessSceneFile( outputCSV, entityFile );
	}

	GFileManager->SaveStringToFileWithUTF8( dumpPath, outputCSV );
}

void CQuestAppearancesDumper::GetQuestEntities( TDynArray< CDiskFile* >& entities )
{
	TDynArray< CDirectory* > charactersDirectories;
	//String questsPath = TXT( "quests" );
	//String charactersDirectoryName = TXT( "characters" );
	/*GetDirectoriesByName( questsDirectory, charactersDirectoryName, charactersDirectories );
	for ( auto dir : charactersDirectories )
	{
		dir->CollectFiles( entities, TXT( ".w2ent" ), true, false );
	}*/
	String questsPath = TXT( "gameplay/community/community_npcs" );
	String charactersDirectoryName = TXT( "community_npcs" );	
	CDirectory* questsDirectory = GDepot->FindPath( questsPath );
	questsDirectory->CollectFiles( entities, TXT( ".w2ent" ), true, false );
}

void CQuestAppearancesDumper::GetScenesEntities( TDynArray< CDiskFile* >& entities )
{
	TDynArray< CDirectory* > charactersDirectories;
	String questsPath = TXT( "quests" );
	String charactersDirectoryName = TXT( "scenes" );
	CDirectory* questsDirectory = GDepot->FindLocalDirectory( questsPath );
	GetDirectoriesByName( questsDirectory, charactersDirectoryName, charactersDirectories );
	for ( auto dir : charactersDirectories )
	{
		dir->CollectFiles( entities, TXT( ".w2scene" ), true, false );
	}
}

void CQuestAppearancesDumper::ProcessEntityFile( String& outputCSV, CDiskFile* entityFile )
{
	Bool wasLoaded = entityFile->IsLoaded();
	if( !entityFile->Load() )
		return;

	CResource* res = entityFile->GetResource();
	const String entityDepotFile = entityFile->GetDepotPath();
	if ( res->IsA<CEntityTemplate>() )
	{
		CEntityTemplate* entityTemplate = static_cast< CEntityTemplate* >( res );
		ProcessEntityTemplateAppearances( outputCSV, entityDepotFile, entityTemplate );
	}
	entityFile->Unload();
}

void CQuestAppearancesDumper::ProcessEntityTemplateAppearances( String& outputCSV, const String& entityDepotFile, const CEntityTemplate* entityTemplate )
{
	TDynArray< const CEntityAppearance* > appearances;
	entityTemplate->GetAllAppearances( appearances );
	for ( auto appearance : appearances )
	{
		String enabledText = entityTemplate->IsAppearanceEnabled( appearance->GetName() ) ? TXT( "enabled" ) : TXT( "disabled" );
		outputCSV += TXT( "\"" ) + entityDepotFile + TXT( "\"" ) + TXT( ";" );
		outputCSV += TXT( "\"" ) + appearance->GetName().AsString() + TXT( "\"" ) + TXT( ";" );
		outputCSV += TXT( "\"" ) + enabledText + TXT( "\"" ) + TXT( "\n" );
	}
}

void CQuestAppearancesDumper::ProcessSceneFile( String& outputCSV, CDiskFile* entityFile )
{
	Bool wasLoaded = entityFile->IsLoaded();
	if( !entityFile->Load() )
		return;

	CResource* res = entityFile->GetResource();
	const String entityDepotFile = entityFile->GetDepotPath();
	if ( res->IsA<CStoryScene>() )
	{
		CStoryScene* storyScene = static_cast< CStoryScene* >( res );
		ProcessStorySceneAppearances( outputCSV, entityDepotFile, storyScene );
	}
	entityFile->Unload();
}

void CQuestAppearancesDumper::ProcessStorySceneAppearances( String& outputCSV, const String& sceneDepotFile, const CStoryScene* storyScene )
{
	const TDynArray< CStorySceneActor* >& actorsDef = storyScene->GetSceneActorsDefinitions();
	for ( auto actorDef : actorsDef )
	{		
		const TDynArray< CName >& actorAppearances = actorDef->m_appearanceFilter;
		const String entityName = actorDef->m_id.AsString();
		if ( actorAppearances.Size() == 0 )
		{
			if ( actorDef->m_dontSearchByVoicetag )
			{
				outputCSV += TXT( "\"" ) + sceneDepotFile + TXT( "\"" ) + TXT( ";" );
				outputCSV += TXT( "\"" ) + entityName + TXT( "\"" ) + TXT( ";" );
				outputCSV += TXT( "\"DONT SEARCH BY VOICETAG\"\n" );
			}
			else
			{
				TDynArray< CStoryScene::CTemplateWithAppearance > templates;
				storyScene->GetAppearancesForVoicetag( actorDef->m_id, templates );
				for ( auto templateWithAppearance : templates )
				{
					outputCSV += TXT( "\"" ) + sceneDepotFile + TXT( "\"" ) + TXT( ";" );
					outputCSV += TXT( "\"" ) + entityName + TXT( "\"" ) + TXT( ";" );
					outputCSV += TXT( "\"" ) + templateWithAppearance.m_appearance.AsString() + TXT( "\"" ) + TXT ( "\n" );
				}
			}

			continue;
		}
		for ( auto appearance : actorAppearances )
		{			
			outputCSV += TXT( "\"" ) + sceneDepotFile + TXT( "\"" ) + TXT( ";" );
			outputCSV += TXT( "\"" ) + entityName + TXT( "\"" ) + TXT( ";" );
			outputCSV += TXT( "\"" ) + appearance.AsString() + TXT( "\"" ) + TXT( "\n" );	
		}
	}
}

#endif
