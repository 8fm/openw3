/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "saveServer.h"
#include "../core/chunkedLZ4File.h"
#include "../core/basicDataBlob.h"
#include "gameSaveManager.h"

// THIS IS JUST A DEBUG TOOL
// ...so please don't mind a bit of spaghetti code ;) in here
// RED_NETWORK is avoided intentionally

#ifdef SAVE_SERVER_ENABLED
void CSaveServer::Open()
{
	if ( m_running )
	{
		return;
	}

	Red::Network::Base::Initialize();
	m_serverSocketId = Red::Network::Base::Socket( RED_NET_PF_INET, RED_NET_SOCK_STREAM, RED_NET_IPPROTO_TCP );
	SS_NET_CALL( SetNonBlocking( m_serverSocketId ) )

	Red::Network::Address addr( 4600, Red::Network::Address::EFamily::Family_IPv4 );
	SS_NET_CALL( Bind( m_serverSocketId, addr.GetNative(), addr.GetNativeSize() ) )
	SS_NET_CALL( Listen( m_serverSocketId ) )

	m_running = true;
	m_updating = false;
	RED_LOG( SaveServer, TXT("Server up and running.") );
}

void CSaveServer::Update()
{
	if ( !m_running )
	{
		return;
	}

	if ( m_updating )
	{
		return;
	}

	struct SReenterGuard
	{
		Bool& m_val;
		SReenterGuard( Bool& val ) : m_val( val ) { val = true; }
		~SReenterGuard() { m_val = false; }
	} guard( m_updating );

	Red::Network::SocketId newClientSocketId;
	const Bool accepted = Red::Network::Base::Accept( m_serverSocketId, newClientSocketId, nullptr, nullptr );
	if ( accepted )
	{
		if ( m_connected )
		{
			CloseClient();
		}
		m_clientSocketId = newClientSocketId;
		m_connected = true;

		RED_LOG( SaveServer, TXT("accepted connection") ); 
	}
	else 
	{
		Int32 err = Red::Network::Base::GetLastError();
		if ( err != RED_NET_ERR_ACCEPT_OK )
		{
			RED_LOG( SaveServer, TXT("accept() error: %ld"), err );
		}

		if ( !m_connected )
		{
			return;
		}
	}

	Bool noCommand = false;
	Uint8 commandBuffer[ CMD_BUF_SIZE + 2 ];
	Red::MemoryZero( commandBuffer, CMD_BUF_SIZE + 2 );
	Uint32 bytesRecived = RecvAll( commandBuffer, CMD_BUF_SIZE, noCommand );
	if ( noCommand )
	{
		return;
	}

	// connection closed by peer?
	if ( bytesRecived < CMD_BUF_SIZE )
	{
		CloseClient();
		return;
	}

	const String command( ( Char* ) commandBuffer );
	RED_LOG( SaveServer, TXT("Received %ls command."), command.AsChar() );
	if ( command.EqualsNC( TXT("<LIST>") ) )
	{
		OnListCommand();
	}
	else if ( command.EqualsNC( TXT("<GRAB>") ) )
	{
		OnGrabCommand();
	}
	else if ( command.EqualsNC( TXT("<LOAD>") ) )
	{
		OnLoadCommand();
	}
	else
	{
		HALT("unimplemented command?")
	}
}

void CSaveServer::Close()
{
	if ( false == m_running )
	{
		return;
	}

	if ( m_connected )
	{
		Red::Network::Base::Close( m_clientSocketId );
		m_connected = false;
	}

	Red::Network::Base::Close( m_serverSocketId );
	m_running = false;

	RED_LOG( SaveServer, TXT("Server closed.") );
}

Uint32 CSaveServer::RecvAll( Uint8* buffer, Uint32 size, Bool& noDataYet )
{
	noDataYet = false;
	Uint32 totalRecv = 0;
	do 
	{
		Int32 bytesRecived = Red::Network::Base::Recv( m_clientSocketId, &buffer[ totalRecv ], size - totalRecv );
		if ( bytesRecived < 1 )
		{
			Int32 error = Red::Network::Base::GetLastError();
			if ( totalRecv == 0 )
			{
				if ( error == RED_NET_ERR_RECEIVE_OK )
				{
					noDataYet = true;
					return 0;
				}
			}
			else
			{
				if ( error == RED_NET_ERR_RECEIVE_OK )
				{
					Red::Threads::SleepOnCurrentThread( 10 );
					continue;
				}
			}

			RED_LOG( SaveServer, TXT("Recv() err: %ld"), error );
			return totalRecv;
		}

		totalRecv += Uint32( bytesRecived );
	} 
	while ( totalRecv < size );
	return totalRecv;
}

Uint32 CSaveServer::SendAll( Uint8* buffer, Uint32 size )
{
	Uint32 totalSent = 0;
	do 
	{
		Int32 bytesSent = Red::Network::Base::Send( m_clientSocketId, &buffer[ totalSent ], size - totalSent );
		if ( bytesSent < 0 )
		{
			Int32 error = Red::Network::Base::GetLastError();
			if ( error == RED_NET_ERR_SEND_OK )
			{
				Red::Threads::SleepOnCurrentThread( 10 );
				continue;
			}

			RED_LOG( SaveServer, TXT("Send() err: %ld"), error );
			return totalSent;
		}

		totalSent += Uint32( bytesSent );
	} 
	while ( totalSent < size );
	return totalSent;
}

void CSaveServer::ZeroResponse()
{
	Uint32 zero = 0;
	Uint32 bytesSent = SendAll( ( Uint8* ) &zero, 4 );
	if ( bytesSent < 4 )
	{
		Red::Network::Base::Close( m_clientSocketId );
		m_connected = false;
	}
}

void CSaveServer::CloseClient()
{
	RED_LOG( SaveServer, TXT("Closing client connection.") );
	Red::Network::Base::Close( m_clientSocketId );
	m_connected = false;
}

void CSaveServer::OnListCommand()
{
	TDynArray< SSavegameInfo > infos;
	GUserProfileManager->GetSaveFiles( infos );

	const Uint32 singleStructSize = 3 + 64;
	const Uint32 msgBufferSize = singleStructSize * infos.Size() + 1;
	Uint8* msgBuffer = ( Uint8* ) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Network, msgBufferSize );
	if ( nullptr == msgBuffer )
	{
		CloseClient();
		return;
	}
	Red::MemoryZero( msgBuffer, msgBufferSize );

	Uint32 n = 0;
	ASSERT( infos.Size() < 256 );
	msgBuffer[ n++ ] = Uint8( infos.Size() );
	for ( const auto& info : infos )
	{
		msgBuffer[ n++ ] = Uint8( info.m_slotIndex );
		msgBuffer[ n++ ] = Uint8( info.m_slotType );
		msgBuffer[ n++ ] = Uint8( info.m_filename.GetLength() );
		ASSERT( info.m_filename.GetLength() < 32 );
		Red::MemoryCopy( &msgBuffer[ n ], info.m_filename.AsChar(), Clamp< Uint32 >( info.m_filename.GetLength(), 1, 32 ) * sizeof( Char ) );
		n += 64;
	}

	Uint32 bytesSent = SendAll( msgBuffer, msgBufferSize );

	RED_MEMORY_FREE( MemoryPool_Default, MC_Network, msgBuffer );
	if ( bytesSent < msgBufferSize )
	{
		CloseClient();
		return;
	}

	RED_LOG( SaveServer, TXT("<LIST> succesful.") );
}

void CSaveServer::OnGrabCommand()
{
	Uint8 commandData[2];
	Bool noData = false;
	do 
	{
		Uint32 bytesRecived = RecvAll( commandData, 2, noData );
		if ( !noData && bytesRecived < 2 )
		{
			CloseClient();
			return;
		}
	} 
	while ( noData );

	Int16 slot = commandData[ 0 ];
	ESaveGameType type = ( ESaveGameType ) commandData[ 1 ];

	SSavegameInfo info;
	GUserProfileManager->GetSaveInSlot( type, slot, info );
	if ( false == info.IsValid() )
	{
		ZeroResponse();
		return;
	}

	const auto ret = GUserProfileManager->InitGameLoading( info );
	if ( ret != ELoadGameResult::LOAD_Initializing && ret != ELoadGameResult::LOAD_ReadyToLoad )
	{
		ZeroResponse();
		return;
	}

	Int32 timeout = 100;
	for ( ; timeout > 0; --timeout )
	{
		GUserProfileManager->Update();
		if ( GUserProfileManager->GetLoadGameProgress() == ELoadGameResult::LOAD_ReadyToLoad )
		{
			break;
		}
		Red::Threads::SleepOnCurrentThread( 100 );
	}

	if ( timeout == 0 )
	{
		ZeroResponse();
		return;
	}

	IFile* reader = GUserProfileManager->CreateSaveFileReader( /*raw*/ true );
	if ( !reader )
	{
		ZeroResponse();
		return;
	}

	Uint32 size = (Uint32) reader->GetSize();
	DataBlobPtr data( new CDataBlob( size ) );
	reader->Serialize( data->GetData(), data->GetDataSize() );

	delete reader;
	GUserProfileManager->FinalizeGameLoading();

	Uint32 bytesSent = SendAll( ( Uint8* ) &size, 4 );
	if ( bytesSent < 4 )
	{
		CloseClient();
		return;
	}

	bytesSent = SendAll( ( Uint8* ) data->GetData(), data->GetDataSize() );
	if ( bytesSent < data->GetDataSize() )
	{
		CloseClient();
		return;
	}

	GUserProfileManager->RequestScreenshotDataForReading( info );

	timeout = 100;
	for ( ; timeout > 0; --timeout )
	{
		GUserProfileManager->Update();
		if ( GUserProfileManager->IsScreenshotDataReadyForReading() )
		{
			break;
		}
		Red::Threads::SleepOnCurrentThread( 100 );
	}

	if ( timeout == 0 )
	{
		ZeroResponse();
		return;
	}

	reader = GUserProfileManager->CreateScreenshotDataReader();
	if ( !reader )
	{
		ZeroResponse();
		return;
	}

	size = (Uint32) reader->GetSize();
	DataBlobPtr scrdata( new CDataBlob( size ) );
	reader->Serialize( scrdata->GetData(), scrdata->GetDataSize() );

	delete reader;
	GUserProfileManager->DoneReadingScreenshotData();

	bytesSent = SendAll( ( Uint8* ) &size, 4 );
	if ( bytesSent < 4 )
	{
		CloseClient();
		return;
	}

	bytesSent = SendAll( ( Uint8* ) scrdata->GetData(), size );
	if ( bytesSent < size )
	{
		CloseClient();
		return;
	}

	RED_LOG( SaveServer, TXT("<GRAB> succesful.") );
}

void CSaveServer::OnLoadCommand()
{
	Uint32 bufSize = 0;
	Bool noData = false;
	do 
	{
		Uint32 bytesRecived = RecvAll( ( Uint8* ) &bufSize, 4, noData );
		if ( !noData && bytesRecived < 4 )
		{
			CloseClient();
			return;
		}
	} 
	while ( noData );

	DataBlobPtr data( new CDataBlob( bufSize ) );
	do 
	{
		Uint32 bytesRecived = RecvAll( ( Uint8* )data->GetData(), bufSize, noData );
		if ( !noData && bytesRecived < bufSize )
		{
			CloseClient();
			return;
		}
	} 
	while ( noData );

	IFile* reader = nullptr;
	IFile* fileReaderRaw = new CMemoryFileReaderExternalBuffer( data->GetData(), bufSize );
	Uint32 magicHeader = 0;
	*fileReaderRaw << magicHeader;
	if ( magicHeader == SAVE_NEW_FORMAT_HEADER )
	{
		// The file is new format save-game
		reader = new CChunkedLZ4FileReader( fileReaderRaw );
	}

	if ( reader )
	{
		if ( GGame->IsActive() )
		{
			GGame->RequestGameEnd();
		}

		GGame->TryEndGame();

		IGameLoader* gameLoader = SGameSaveManager::GetInstance().CreateDebugLoader( reader );
		if ( !gameLoader )
		{
			RED_LOG( SaveServer, TXT("Unable to create debug loader. Not loading.") );
			return;
		}

		SGameSessionManager::GetInstance().RestoreSession( gameLoader, false );
		RED_LOG( SaveServer, TXT("<LOAD> succesful.") );
	}
	else
	{
		RED_LOG( SaveServer, TXT("Save in old format. Not loading.") );
	}
}

#endif //SAVE_SERVER_ENABLED
