/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include <eh.h>
#include <ppltasks.h>
#include <collection.h>
#include <Robuffer.h>
#include <Datetimeapi.h>

#include "userProfileDurango.h"

#include "../../common/engine/compressedFile.h"
#include "../../common/engine/gameSaveManager.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/engine/gameSession.h"

#include "../../common/core/contentManager.h"
#include "../../common/core/memoryFileReader.h"
#include "../../common/core/memoryFileWriter.h"
#include "../../common/core/tokenizer.h"
#include "../../common/core/chunkedLZ4File.h"
#include "../../common/core/fileProxy.h"
#include "../../common/core/configVarSystem.h"

#include "../../common/core/compression/compression.h"
#include "../../common/core/compression/lz4.h"

#include "../../common/game/gameSaver.h"
#include "../../common/game/commonGame.h"
#include "../../common/game/guiManager.h"

// Save-game compression values
const Uint32 c_saveGameCompressedChunkSize = 1024 * 1024;		// Size of chunks (uncompressed)
const Uint32 c_saveGameMaxCompressedChunks = 256;				// Max chunks

#ifdef RED_LOGGING_ENABLED
const Char* GetAsyncStatusTxtForLog( Windows::Foundation::AsyncStatus status )
{
	switch (status)
	{
	case Windows::Foundation::AsyncStatus::Started:		return TXT("Started");
	case Windows::Foundation::AsyncStatus::Completed:	return TXT("Completed");
	case Windows::Foundation::AsyncStatus::Canceled:	return TXT("Canceled");
	case Windows::Foundation::AsyncStatus::Error:		return TXT("Error");
	default:
		return TXT("<Unknown>");
	}
}
#endif

CUserProfileManagerDurango::SUserDialogRequest::SUserDialogRequest()
	: m_type( TYPE_Nothing )
	, m_buttonSet( BUTTONS_Ok )
	, m_status( STATUS_Initialized )
	, m_result( RESULT_Unknown )
{
}

CUserProfileManagerDurango::SUserDialogRequest::SUserDialogRequest( EMessageId type )
	: m_type( type )
	, m_status( STATUS_Initialized )
	, m_result( RESULT_Unknown )
{
	switch ( type )
	{
	case QUESTION_WouldYouLikeToContinueWithoutSaving:
	case QUESTION_DebugTest:
		m_buttonSet = BUTTONS_YesNo;
		break;
	case WARNING_LoadingFailedDamagedData:
	case WARNING_NoSpaceForSave:
	default:
		m_buttonSet = BUTTONS_Ok;
	}
}


String CUserProfileManagerDurango::SUserDialogRequest::GetLocalizedTitleString() const
{
	// THIS is temporary
	switch ( m_type )
	{
	case QUESTION_DebugTest:
		return TXT("[DEBUG TEST]");
	case QUESTION_WouldYouLikeToContinueWithoutSaving:
	case WARNING_LoadingFailedDamagedData:
	case WARNING_NoSpaceForSave:
	default:
		return TXT("");
	}
}

String CUserProfileManagerDurango::SUserDialogRequest::GetLocalizedMessageString() const
{
	switch ( m_type )
	{
	case QUESTION_WouldYouLikeToContinueWithoutSaving:
		return TXT("error_message_no_profile_x1"); 
	case WARNING_LoadingFailedDamagedData:
		return TXT("error_message_damaged_save"); 
	case WARNING_NoSpaceForSave:
		TXT("error_message_no_space"); 
	case QUESTION_DebugTest:
	default:
		return TXT("error_message_no_profile_x1"); 
	}
}


void CUserProfileManagerDurango::OnUserDialogCallback( Int32 messageId, Int32 actionId )
{
	if ( m_modalBox.m_type == messageId )
	{
		STATE_CHECK( STATE_ModalUI )

		// this is our message box
		m_modalBox.m_result = ( SUserDialogRequest::EResult ) actionId;
		m_modalBox.m_status = SUserDialogRequest::STATUS_Closed;
	}
}


void CUserProfileManagerDurango::OnSaveSystemUpdate()
{
	// handle UI requests
	if ( m_modalBox.IsValid() )
	{
		STATE_CHECK( STATE_ModalUI )

		if ( m_modalBox.m_status == SUserDialogRequest::STATUS_Closed )
		{
			OnModalUIBoxClosed();
			m_modalBox.Invalidate();
		}
		else if ( m_modalBox.m_status == SUserDialogRequest::STATUS_Initialized )
		{
			// check if we can display it
			if ( GCommonGame && GCommonGame->GetGuiManager() && GCommonGame->GetFlashPlayer() )
			{
				// public function ShowUserDialog( messageId : int, title : string, message : string, type : EUserDialogButtons ) : void
				const Bool found = CallFunction( GCommonGame->GetGuiManager(), CNAME( ShowUserDialog ), Int32( m_modalBox.m_type ), m_modalBox.GetLocalizedTitleString(), m_modalBox.GetLocalizedMessageString(), Int8( m_modalBox.m_buttonSet ) );
				if ( found )
				{
					m_modalBox.m_status = SUserDialogRequest::STATUS_Displayed;
					// ...and now we wait for the callback to happen
				}
			}
		}
	}

	#ifdef SAVE_SERVER_ENABLED
		if ( m_loadSaveState.GetValue() == STATE_Ready )
		{
			m_debugSaveServer.Update();
		}
	#endif // SAVE_SERVER_ENABLED

	// save infos updating
	ESaveListUpdateState state = ( ESaveListUpdateState ) m_saveInfosUpdateState.GetValue();
	switch ( state )
	{
	case LIST_NeedsUpdate:
		if ( m_saveLock == CGameSessionManager::GAMESAVELOCK_INVALID )
		{
			SGameSessionManager::GetInstance().CreateNoSaveLock( TXT("save list update"), m_saveLock, true, false );
		}

		if ( m_connectedStorageSpace && m_loadSaveState.GetValue() == STATE_Ready )
		{
			UpdateSaveInfos();
		}

		break;
	case LIST_Updating:
		break;
	case LIST_Updated_EventPending:
		m_saveInfosUpdateState.SetValue( LIST_Updated );
		GGame->CallEvent( CNAME( OnGameSaveListUpdated ) );
		// no break intentional
	case LIST_Updated:
		if ( m_saveLock != CGameSessionManager::GAMESAVELOCK_INVALID )
		{
			SGameSessionManager::GetInstance().ReleaseNoSaveLock( m_saveLock );
			m_saveLock = CGameSessionManager::GAMESAVELOCK_INVALID;
		}
		break;
	default:
		ASSERT( false ); // WTF?
	}
}



extern Bool GUseSaveHack;

ELoadGameResult CUserProfileManagerDurango::InitGameLoading( const SSavegameInfo& info )
{
	Int32 loadSaveState = m_loadSaveState.GetValue();
	if ( loadSaveState == STATE_NoSaveMode || loadSaveState == STATE_None )
	{
		return LOAD_Error;
	}

	RED_FATAL_ASSERT( m_connectedStorageSpace, "Save system has not been initialised properly for current user" );

	if ( m_screenshotDataReadAction )
	{
		m_screenshotDataReadAction->Cancel();
	}

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );

	STATE_CHECK( STATE_Ready )	
	ASSERT( m_initializedLoadSaveInfoIndex < 0 );
	m_currentLoadOpProgress.SetValue( LOAD_NotInitialized );

	for ( Int32 i = 0; i < m_saveInfos.SizeInt(); ++i )
	{
		if ( m_saveInfos[ i ].m_filename.EqualsNC( info.m_filename ) )
		{
			m_initializedLoadSaveInfoIndex = i;
			break;
		}
	}

	if ( GUseSaveHack && m_initializedLoadSaveInfoIndex < 0 )
	{
		m_initializedLoadSaveInfoIndex = m_saveInfos.SizeInt();
		m_saveInfos.PushBack( info );
	}

	if ( false == GUseSaveHack && m_initializedLoadSaveInfoIndex >= 0 )
	{
		const String& filename = m_saveInfos[ m_initializedLoadSaveInfoIndex ].m_filename;
		Platform::String^ containerName = ref new Platform::String( filename.AsChar(), filename.GetLength() );
		auto container = m_connectedStorageSpace->CreateContainer( containerName );
		m_currentLoadOpProgress.SetValue( LOAD_Initializing );
		SET_STATE( STATE_InitializeLoad );

		m_saveBuffer->Length = 0;
		auto reads = ref new Platform::Collections::Map< Platform::String^, Windows::Storage::Streams::IBuffer^ > ();
		reads->Insert( "con", m_contentDescriptorBuffer );
		reads->Insert( "sav", m_saveBuffer );
		reads->Insert( "scr", m_screenshotBuffer );

		auto op = container->ReadAsync( reads->GetView() );
		op->Completed = ref new Windows::Foundation::AsyncActionCompletedHandler(
			[=] ( Windows::Foundation::IAsyncAction^ a, Windows::Foundation::AsyncStatus status )
			{
				LOG_ENGINE(TXT("InitGameLoading AsyncStatus: %ls"), GetAsyncStatusTxtForLog( status ) );

				switch ( status )
				{
					case Windows::Foundation::AsyncStatus::Completed:
						OnLoadGameInitialized();	
					break;

					case Windows::Foundation::AsyncStatus::Error:
					case Windows::Foundation::AsyncStatus::Canceled:
					default:
					    OnLoadGameInitFailed();
					    break;
				}
			}
		);
	}

	if ( GUseSaveHack )
	{
		OnLoadGameInitialized();
	}

	return ( ELoadGameResult ) m_currentLoadOpProgress.GetValue();
}


TDynArray< CName > CUserProfileManagerDurango::GetContentRequiredByLastSave() const 
{
	Uint32 totalLength = m_contentDescriptorBuffer->Length;
	ASSERT( totalLength > 0 );

	TDynArray< CName > requiredContent;

	// we will only read, so const_cast<> here is better than making this method non-const
	const AnsiChar* strBuf = ( const AnsiChar* ) ( const_cast< CUserProfileManagerDurango* > ( this )->GetBufferByteAccess( m_contentDescriptorBuffer ) );
	if ( nullptr == strBuf )
	{
		return requiredContent;
	}

	Uint32 length = 0;
	const AnsiChar* currentStr = strBuf;
	for ( Uint32 i = 0; i < totalLength; ++i )
	{
		if ( '\0' == strBuf[ i ] )
		{
			if ( length > 0 )
			{
				requiredContent.PushBack( CName( ANSI_TO_UNICODE( currentStr ) ) );
				length = 0;
				currentStr = strBuf + i + 1;
			}
			else
			{
				HALT("Unexpected null value inside content descriptor buffer. Please DEBUG.");
			}
		}
		else
		{
			++length;
		}
	}

	// W3 hack and failsafe. Note that scripts expect to get the last content as highest, but if missing launch0 then
	// realistically can only happen with content0, conten1, and content2 but no content3
	if ( GContentManager )
	{
		const CName resolvedLaunchZero  = GContentManager->GetResolvedLaunchZero();
		if ( resolvedLaunchZero )
		{
			requiredContent.PushBackUnique( resolvedLaunchZero );
		}
	}

	return requiredContent;
}


ELoadGameResult CUserProfileManagerDurango::GetLoadGameProgress() const
{
	return ( ELoadGameResult ) m_currentLoadOpProgress.GetValue();
}

void CUserProfileManagerDurango::OnLoadGameInitialized()
{
	SET_STATE( STATE_LoadInitialized );
	if ( m_loadSaveState.GetValue() == STATE_LoadInitialized )
	{
		// verify content availability
		TDynArray< CName > requiredContent = GetContentRequiredByLastSave();
		for ( auto name : requiredContent )
		{
			if ( false == GContentManager->IsContentAvailable( name ) )
			{
				m_initializedLoadSaveInfoIndex = -1;
				SET_STATE( STATE_Ready );

				m_currentLoadOpProgress.SetValue( LOAD_MissingContent );
				return;
			}
		}
	
		m_currentLoadOpProgress.SetValue( LOAD_ReadyToLoad );
	}
}

void CUserProfileManagerDurango::OnLoadGameInitFailed()
{
	m_initializedLoadSaveInfoIndex = -1;
	m_currentLoadOpProgress.SetValue( LOAD_Error );
	RequestModalUIBox( SUserDialogRequest::WARNING_LoadingFailedDamagedData );
}


void CUserProfileManagerDurango::CancelGameLoading()
{
	m_initializedLoadSaveInfoIndex = -1;
	m_currentLoadOpProgress.SetValue( LOAD_NotInitialized );
	if ( m_loadSaveState.GetValue() != STATE_NoSaveMode )
	{
		SET_STATE( STATE_Ready );
	}
}


IFile* CUserProfileManagerDurango::CreateSaveFileReader(const Bool rawFile)
{
	if ( m_loadSaveState.GetValue() == STATE_NoSaveMode )
	{
		return nullptr;
	}

	if ( m_screenshotDataReadAction )
	{
		m_screenshotDataReadAction->Cancel();
	}

	ASSERT( m_currentLoadOpProgress.GetValue() == LOAD_ReadyToLoad );
	ASSERT( m_initializedLoadSaveInfoIndex >= 0 );
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );
	const SSavegameInfo& info = m_saveInfos[ m_initializedLoadSaveInfoIndex ];
	m_currentLoadOpProgress.SetValue( LOAD_Loading );

	if( GUseSaveHack )
	{
		IFile* rawFile = GFileManager->CreateFileReader( String( TXT("d:\\gamesaves\\") ) + info.GetFileName() + info.GetSaveExtension(), FOF_AbsolutePath | FOF_Buffered );
		if( rawFile != nullptr )
		{
			// use the raw file reader
			if ( rawFile )
				return rawFile;

			Uint32 headMagic = 0;
			*rawFile << headMagic;
			if( headMagic == SAVE_NEW_FORMAT_HEADER )
			{
				return new CChunkedLZ4FileReader( rawFile );
			}
			else
			{
				delete rawFile;
				return new CCompressedFileReader( String( TXT("d:\\gamesaves\\") ) + info.GetFileName() + info.GetSaveExtension() );
			}
		}
	}
	else
	{
		STATE_CHECK( STATE_LoadInitialized )
		Uint8* bytes = GetBufferByteAccess( m_saveBuffer );
		if ( nullptr == bytes )
		{
			return nullptr;
		}

		// use the raw file reader
		if ( rawFile )
			return new CMemoryFileReaderExternalBuffer( bytes, m_saveBuffer->Length );

		const Uint32* header = ( const Uint32* ) bytes;
		if( header[ 0 ] == SAVE_NEW_FORMAT_HEADER )
		{
			CMemoryFileReaderExternalBuffer* winRtBufferWrapper = new CMemoryFileReaderExternalBuffer( bytes, m_saveBuffer->Length );
			RED_FATAL_ASSERT( winRtBufferWrapper, "Failed to create external buffer reader" );
			if( winRtBufferWrapper )
			{
				winRtBufferWrapper->Seek( sizeof( Uint32 ) );
				return new CChunkedLZ4FileReader( winRtBufferWrapper );
			}
		}
		else if ( header[ 0 ] == 'CLZ4' )
		{
			CMemoryFileReaderWithBuffer* fileReader = new CMemoryFileReaderWithBuffer( header[ 1 ] ); 
			RED_FATAL_ASSERT( fileReader->GetData() != nullptr, "Failed to allocate %d bytes for save game decompression", header[ 1 ] );

			// old compression format (no chunks)
			Red::Core::Decompressor::CLZ4 decompressor;
			decompressor.Initialize( bytes + 8, ( void* ) fileReader->GetData(), m_saveBuffer->Length, header[ 1 ] );
			TIMER_BLOCK( decompress )
				decompressor.Decompress();
			END_TIMER_BLOCK( decompress )

			return fileReader;
		}
		else if( header[ 0 ] == 'LZ4C' )
		{
			CMemoryFileReaderWithBuffer* fileReader = new CMemoryFileReaderWithBuffer( header[ 1 ] ); 
			RED_FATAL_ASSERT( fileReader->GetData() != nullptr, "Failed to allocate %d bytes for save game decompression", header[ 1 ] );

			SChunkedLZ4Compressor< DEFAULT_LZ4_CHUNK_SZIE > decompressor( bytes + 8, fileReader->GetData(),  m_saveBuffer->Length - 8, header[ 1 ] );
			TIMER_BLOCK( decompress )
				decompressor.Decompress();
			END_TIMER_BLOCK( decompress )

			return fileReader;
		}
	}

	return nullptr;
}

void CUserProfileManagerDurango::FinalizeGameLoading()
{
	STATE_CHECK( STATE_LoadInitialized )
	m_initializedLoadSaveInfoIndex = -1;
	m_currentLoadOpProgress.SetValue( LOAD_NotInitialized );
	SET_STATE( STATE_Ready );
}

void CUserProfileManagerDurango::RequestScreenshotDataForReading( const SSavegameInfo& info )
{
	if ( m_loadSaveState.GetValue() == STATE_NoSaveMode )
	{
		return;
	}

	// block here until we finish saving op
	// for some (undocumented) reason, reading one container while saving the other breaks the latter
	// (and we're never getting into the completion handler !!!)
	// so the best thing we can do is just wait for it to finish before we start
	while ( m_loadSaveState.GetValue() == STATE_Saving )
	{
		Red::Threads::SleepOnCurrentThread( 10 );
	}

	if ( m_screenshotDataReadAction )
	{
		m_screenshotDataReadAction->Cancel();
	}

	STATE_CHECK( STATE_Ready )

	const String& filename = info.m_filename;
	Platform::String^ containerName = ref new Platform::String( filename.AsChar(), filename.GetLength() );
	auto container = m_connectedStorageSpace->CreateContainer( containerName );

	m_screenshotBuffer->Length = 0;
	auto reads = ref new Platform::Collections::Map< Platform::String^, Windows::Storage::Streams::IBuffer^ > ();
	reads->Insert( "scr", m_screenshotBuffer );

	m_screenshotDataReadAction = container->ReadAsync( reads->GetView() );
	m_screenshotDataReadAction->Completed = ref new Windows::Foundation::AsyncActionCompletedHandler(
		[=] ( Windows::Foundation::IAsyncAction^ a, Windows::Foundation::AsyncStatus status )
		{
			switch ( status )
			{
				LOG_ENGINE(TXT("RequestScreenshotDataForReading AsyncStatus: %ls"), GetAsyncStatusTxtForLog( status ) );

				case Windows::Foundation::AsyncStatus::Completed:
					OnScreenshotDataRead( true );	
					break;

				case Windows::Foundation::AsyncStatus::Error:
				case Windows::Foundation::AsyncStatus::Canceled:
				default:
				    OnScreenshotDataRead( false );
				    break;
			}
		}
	);
}

Bool CUserProfileManagerDurango::IsScreenshotDataReadyForReading()
{
	return m_screenshotDataRead.GetValue();
}

void CUserProfileManagerDurango::OnScreenshotDataRead( Bool success )
{
	m_screenshotDataRead.SetValue( success );
}

void CUserProfileManagerDurango::DoneReadingScreenshotData()
{
	m_screenshotDataReadAction = nullptr;
	m_screenshotDataRead.SetValue( false );
}


IFile* CUserProfileManagerDurango::CreateScreenshotDataReader()
{
	if ( false == GUseSaveHack && m_loadSaveState.GetValue() != STATE_NoSaveMode && IsScreenshotDataReadyForReading() )
	{
		return new CMemoryFileReaderExternalBuffer( GetScreenshotBuffer(), m_screenshotBuffer->Length );
	}
	else
	{
		return new CMemoryFileReaderExternalBuffer( CGameSaver::DEFAULT_SCREENSHOT_DATA, CGameSaver::DEFAULT_SCREENSHOT_SIZE );
	}
}

void CUserProfileManagerDurango::FinalizeGameSaving( IGameSaver* gameSaver )
{
	// TODO: remove save hack (this is only for dev)
	if ( GUseSaveHack )
	{
		gameSaver->Close();
	}
	else
	{
		Int32 loadSaveState = m_loadSaveState.GetValue();
		if ( loadSaveState == STATE_NoSaveMode || loadSaveState == STATE_None )
		{
			return;
		}

		RED_FATAL_ASSERT( m_connectedStorageSpace, "Save system has not been initialised properly for current user" );

		// proper implementation
		ASSERT( m_initializedLoadSaveInfoIndex >= 0 );
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );
		const SSavegameInfo& info = m_saveInfos[ m_initializedLoadSaveInfoIndex ];

		SET_STATE( STATE_Saving );

		// Finalise the compression (fills the save buffer + closes internal file 'handles')
		gameSaver->Close();
		m_writerCreated = false;

		// Update the winRt buffer, ready for upload to connected storage
		Uint32 saveGameDataSize = gameSaver->GetDataSize();
		m_saveBuffer->Length = saveGameDataSize;

		RED_LOG( Save, TXT("Save %ls size: %ld\n"), info.GetFileName().AsChar(), saveGameDataSize );

		// Upload buffers to cloud storage
		const String& saveName = info.GetFileName();
		Platform::String^ containerName = ref new Platform::String( saveName.AsChar(), saveName.GetLength() );

		String questName = String::Printf( TXT("%lx"), info.m_displayNameIndex );
		Platform::String^ displayName = ref new Platform::String( questName.AsChar(), questName.GetLength() );

		DoSave( containerName, displayName );

		// save object will be deleted in the completion handler
	}
}
  
void CUserProfileManagerDurango::DoSave( Platform::String^ containerName, Platform::String^ displayName )
{
	// save required content
	// for maximum simplicity, we will require all content that was activated in the moment of saving 
	{
		Uint32 length, totalLength = 0;
		const AnsiChar* str;
		AnsiChar* buf = ( AnsiChar* ) GetBufferByteAccess( m_contentDescriptorBuffer );
		if ( nullptr == buf )  // this should never happen
		{
			CancelGameSaving();
			return;	
		}

		TDynArray< CName > requiredContent = GContentManager->GetActivatedContent();
		for ( auto name : requiredContent )
		{
			str = name.AsAnsiChar();
			length = ( Uint32 ) Red::StringLength( str );
		
			if ( length + totalLength + 1 > MAX_CONTENTDESCRIPTOR_SIZE )
			{
				HALT("Increase MAX_CONTENTDESCRIPTOR_SIZE now.");
				break;
			}

			Red::MemoryCopy( buf + totalLength, str, length + 1 );
			totalLength += length + 1;
		}
		m_contentDescriptorBuffer->Length = totalLength;
	}

	// little hack in here
	// if screenshot buffer is empty - pretend it's not
	// otherwise when exporting save out of console, "scr" blob is ignored as empty, and then after re-importing it's missing, which causes error during load
	if ( m_screenshotBuffer->Length == 0 )
	{
		m_screenshotBuffer->Length = 1;
	}

    auto container = m_connectedStorageSpace->CreateContainer( containerName );
    auto updates = ref new Platform::Collections::Map< Platform::String^, IBuffer^ > ();
	updates->Insert( "con", m_contentDescriptorBuffer );
    updates->Insert( "sav", m_saveBuffer );
	updates->Insert( "scr", m_screenshotBuffer );

    auto op = container->SubmitUpdatesAsync( updates->GetView(), nullptr, displayName );

    op->Completed = ref new Windows::Foundation::AsyncActionCompletedHandler(
		[=] ( Windows::Foundation::IAsyncAction^ a, Windows::Foundation::AsyncStatus status )
		{
			LOG_ENGINE(TXT("DoSave AsyncStatus: %ls"), GetAsyncStatusTxtForLog( status ) );

			OnSaveCompleted( status == Windows::Foundation::AsyncStatus::Completed );
		}
	);
}

IFileEx* CUserProfileManagerDurango::CreateSaveFileRawWriter( const SSavegameInfo& saveInfo )
{
	IFileEx* resultingFile = nullptr;

	if ( GUseSaveHack )		// If using save-hack, just write directly to disk
	{
		GFileManager->CreatePath( TXT("d:\\gamesaves\\") ); 

		IFile* platformFile = GFileManager->CreateFileWriter( String( TXT("d:\\gamesaves\\") ) + saveInfo.GetFileName() + saveInfo.GetSaveExtension(), FOF_AbsolutePath );
		RED_FATAL_ASSERT( platformFile, "Failed to open save game file for writing" );
		if( !platformFile )
		{
			return nullptr;
		}

		// Wrap it in a proxy to match interface
		resultingFile = new CFileProxyEx( platformFile );
	}
	else
	{
		if ( m_loadSaveState.GetValue() == STATE_NoSaveMode )
		{
			return nullptr;
		}

		ASSERT( nullptr != m_connectedStorageSpace );

		// We wrap the WinRT buffer directly with a IFile writer. On save, we stream straight to the WinRT buffer, then just upload on completion
		void* winRtBuffer = GetBufferByteAccess( m_saveBuffer );
		m_saveBuffer->Length = 0;
		RED_FATAL_ASSERT( winRtBuffer, "Failed to get byte buffer access for save game" );
		RED_FATAL_ASSERT( m_saveBuffer->Capacity >= SAVE_BUFFER_SIZE, "Buffer is not big enough?!" );
		if( !winRtBuffer )
		{
			return nullptr;
		}

		resultingFile = new CMemoryFileWriterExternalBuffer( winRtBuffer, SAVE_BUFFER_SIZE );
	}

	m_writerCreated = true;

	// We write the header to the raw buffer, so we can test for it when creating reader without having to decompress anything
	Uint32 magicHeader = SAVE_NEW_FORMAT_HEADER;
	*resultingFile << magicHeader;

	return resultingFile;
}

IFileEx* CUserProfileManagerDurango::CreateSaveFileWriter()
{
	STATE_CHECK( STATE_SaveInitialized )
	ASSERT( m_initializedLoadSaveInfoIndex >= 0 );
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );
	const SSavegameInfo& info = m_saveInfos[ m_initializedLoadSaveInfoIndex ];

	// Get an IFile which hides the underlying buffer
	IFileEx* rawFileBuffer = CreateSaveFileRawWriter( info );

	// Wrap it with compressed data writer
	if( rawFileBuffer )
	{
		return new CChunkedLZ4FileWriter( c_saveGameCompressedChunkSize, c_saveGameMaxCompressedChunks, rawFileBuffer );
	}
	else
	{
		return nullptr;
	}
}

void CUserProfileManagerDurango::OnSaveCompleted( Bool successful )
{
	STATE_CHECK( STATE_Saving )
	QueueEvent( successful ? EUserEvent::UE_GameSaved : EUserEvent::UE_GameSaveFailed );

	m_initializedLoadSaveInfoIndex = -1;
	SET_STATE( STATE_Ready );

	m_saveInfosUpdateState.SetValue( LIST_NeedsUpdate );
}

void* CUserProfileManagerDurango::GetScreenshotBuffer()
{
	Uint8* bytes = GetBufferByteAccess( m_screenshotBuffer );
	if ( nullptr == bytes )
	{
		return nullptr;
	}

	return bytes;
}

void CUserProfileManagerDurango::OnScreenshotDone( Uint32 realSize )
{
	if ( realSize < 1024 )
	{
		m_screenshotBuffer->Length = CGameSaver::DEFAULT_SCREENSHOT_SIZE;
		Red::MemoryCopy( GetScreenshotBuffer(), GGame->GetGameSaver()->GetDefaultScreenshotData(), GGame->GetGameSaver()->GetDefaultScreenshotDataSize() );
	}
	else
	{
		m_screenshotBuffer->Length = realSize;
	}

	// BONUS! debug file dump
	// uncomment this, build&run, type xbcp.exe Xd:\ss.png c:\ss.png /X/title in the command line and you'll have a png file on your pc for debug
	/*
	IFile* file = GFileManager->CreateFileWriter( TXT("D:\\ss.png"), FOF_AbsolutePath );
	if ( file )
	{
		file->Serialize( GetBufferByteAccess( m_screenshotBuffer ), realSize );
		delete file;
	}
	*/
}

ESaveGameResult CUserProfileManagerDurango::InitGameSaving( SSavegameInfo& info )
{
	Int32 loadSaveState = m_loadSaveState.GetValue();
	if ( loadSaveState == STATE_NoSaveMode || loadSaveState == STATE_None )
	{
		return SAVE_Error;
	}

	RED_FATAL_ASSERT( m_connectedStorageSpace, "Save system has not been initialised properly for current user" );

	STATE_CHECK( STATE_Ready )
	ASSERT( m_initializedLoadSaveInfoIndex < 0 );

	// check if the user requested specific save slot
	m_initializedLoadSaveInfoIndex = FindSaveInfoIndex( ESaveGameType( info.m_slotType ), info.m_slotIndex );

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );

	// auto-assign slot if needed
	if ( m_initializedLoadSaveInfoIndex < 0 )
	{
		if ( info.IsAutoSave() )
		{
			if ( info.m_slotIndex < 0 || info.m_slotIndex >= NUM_AUTOSAVE_SLOTS )
			{
				// find oldest or free autosave slot
				TBitSet64< NUM_AUTOSAVE_SLOTS > freeslots;
				freeslots.SetAll();

				Red::System::DateTime oldest;
				oldest.SetDateRaw( Uint32( -1 ) );
				oldest.SetTimeRaw( Uint32( -1 ) );
				Int16 oldestInfoIndex = -1;

				// check all save infos for autosaves
				for ( Uint32 i = 0; i < m_saveInfos.Size(); ++i	)
				{
					if ( m_saveInfos[ i ].IsAutoSave() )
					{
						ASSERT( m_saveInfos[ i ].m_slotIndex >= 0 ); // don't assert on too big index in case we decrease the limit in future

						if ( m_saveInfos[ i ].m_slotIndex <  0 || m_saveInfos[ i ].m_slotIndex >= NUM_AUTOSAVE_SLOTS )
						{
							// this one have invalid index, this is an error, skip it
							continue;
						}

						// mark this slot as used
						freeslots.Clear( Uint32( m_saveInfos[ i ].m_slotIndex ) );

						if ( oldest > m_saveInfos[ i ].m_timeStamp )
						{
							// this one is older
							oldest = m_saveInfos[ i ].m_timeStamp;
							oldestInfoIndex = Int16( i );
						}
					}
				}

				// ok, now see if any autosave slot is free
				const Uint32 freeSlotToUse = freeslots.FindNextSet( 0 );
				if ( freeSlotToUse < NUM_AUTOSAVE_SLOTS )
				{
					// just use first free slot
					info.m_slotIndex = Int16( freeSlotToUse );
				}
				else
				{
					// take up the oldest slot
					ASSERT( oldestInfoIndex >= 0 );
					info.m_slotIndex = m_saveInfos[ oldestInfoIndex ].m_slotIndex;
					m_initializedLoadSaveInfoIndex = oldestInfoIndex;
				}	
			}
		}
		else if ( info.IsCheckPoint() )
		{
			if ( info.m_slotIndex < 0 || info.m_slotIndex >= NUM_CHECKPOINT_SLOTS )
			{
				// find oldest or free checkpoint slot
				TBitSet64< NUM_CHECKPOINT_SLOTS > freeslots;
				freeslots.SetAll();

				Red::System::DateTime oldest;
				oldest.SetDateRaw( Uint32( -1 ) );
				oldest.SetTimeRaw( Uint32( -1 ) );
				Int16 oldestInfoIndex = -1;

				// check all save infos for autosaves
				for ( Uint32 i = 0; i < m_saveInfos.Size(); ++i	)
				{
					if ( m_saveInfos[ i ].IsCheckPoint() )
					{
						ASSERT( m_saveInfos[ i ].m_slotIndex >= 0 ); // don't assert on too big index in case we decrease the limit in future

						if ( m_saveInfos[ i ].m_slotIndex <  0 || m_saveInfos[ i ].m_slotIndex >= NUM_CHECKPOINT_SLOTS )
						{
							// this one have invalid index, this is an error, skip it
							continue;
						}

						// mark this slot as used
						freeslots.Clear( Uint32( m_saveInfos[ i ].m_slotIndex ) );

						if ( oldest > m_saveInfos[ i ].m_timeStamp )
						{
							// this one is older
							oldest = m_saveInfos[ i ].m_timeStamp;
							oldestInfoIndex = Int16( i );
						}
					}
				}

				// ok, now see if any autosave slot is free
				const Uint32 freeSlotToUse = freeslots.FindNextSet( 0 );
				if ( freeSlotToUse < NUM_CHECKPOINT_SLOTS )
				{
					// just use first free slot
					info.m_slotIndex = Int16( freeSlotToUse );
				}
				else
				{
					// take up the oldest slot
					ASSERT( oldestInfoIndex >= 0 );
					info.m_slotIndex = m_saveInfos[ oldestInfoIndex ].m_slotIndex;
					m_initializedLoadSaveInfoIndex = oldestInfoIndex;
				}	
			}
		}
		else // quick or manual saves share the slot sapce
		{
			if ( info.m_slotIndex < 0 || info.m_slotIndex >= NUM_QUICKMANUAL_SLOTS )
			{
				// find oldest or free quick or manual slot
				TBitSet64< NUM_QUICKMANUAL_SLOTS > freeslots;
				freeslots.SetAll();

				Red::System::DateTime oldest;
				oldest.SetDateRaw( Uint32( -1 ) );
				oldest.SetTimeRaw( Uint32( -1 ) );
				Int16 oldestInfoIndex = -1;

				// check all save infos for non-autosaves
				for ( Uint32 i = 0; i < m_saveInfos.Size(); ++i	)
				{
					if ( m_saveInfos[ i ].IsQMSave() )
					{
						ASSERT( m_saveInfos[ i ].m_slotIndex >= 0 ); // don't assert on too big index in case we decrease the limit in future

						if ( m_saveInfos[ i ].m_slotIndex <  0 || m_saveInfos[ i ].m_slotIndex >= NUM_QUICKMANUAL_SLOTS )
						{
							// this one have invalid index, this is an error, skip it
							continue;
						}

						// mark this slot as used
						freeslots.Clear( Uint32( m_saveInfos[ i ].m_slotIndex ) );

						if ( oldest > m_saveInfos[ i ].m_timeStamp )
						{
							// this one is older
							oldest = m_saveInfos[ i ].m_timeStamp;
							oldestInfoIndex = Int16( i );
						}
					}
				}

				// ok, now see if any autosave slot is free
				const Uint32 freeSlotToUse = freeslots.FindNextSet( 0 );
				if ( freeSlotToUse < NUM_QUICKMANUAL_SLOTS )
				{
					// just use first free slot
					info.m_slotIndex = Int16( freeSlotToUse );
				}
				else
				{
					// take up the oldest slot
					ASSERT( oldestInfoIndex >= 0 );
					info.m_slotIndex = m_saveInfos[ oldestInfoIndex ].m_slotIndex;
					m_initializedLoadSaveInfoIndex = oldestInfoIndex;
				}	
			}
		}
	}

	// ganerate the filename
	GenerateSaveFileName( info );

	// now we have a valid slot index in info.m_slotIndex, and in m_initializedLoadSaveInfoIndex:
	// -1 if we have taken a free slot and need to add a new save info
	// valid index of m_saveInfos if we have taken an occupied slot and we need to replace save info in the array
	if ( m_initializedLoadSaveInfoIndex < 0 )
	{									
		RED_LOG( Save, TXT("Took a free slot %ld, remaining quota: %lld"), m_saveInfos.SizeInt() ); 

		m_initializedLoadSaveInfoIndex = m_saveInfos.SizeInt();
		m_saveInfos.PushBack( info );
	}
	else
	{
		m_saveInfos[ m_initializedLoadSaveInfoIndex ] = info;
	}

	SET_STATE( STATE_SaveInitialized );
	return SAVE_ReadyToSave;
}

ESaveGameResult CUserProfileManagerDurango::GetSaveGameProgress() const
{
	Int32 state = m_loadSaveState.GetValue();
	switch ( state )
	{
		case STATE_InitializeLoad:	
		case STATE_LoadInitialized:	
		case STATE_ModalUI:					
		case STATE_Updating:			
		case STATE_DeletingSave:
		case STATE_None:
		case STATE_NoSaveMode:
			return SAVE_Error;
		case STATE_SaveInitialized:
			return m_writerCreated ? SAVE_Saving : SAVE_ReadyToSave;
		case STATE_Saving:
			return SAVE_Saving;		
		case STATE_Ready:
		default:
			return SAVE_NotInitialized;								
	};
}

void CUserProfileManagerDurango::CancelGameSaving()
{
	STATE_CHECK( STATE_SaveInitialized )

	m_initializedLoadSaveInfoIndex = -1;
	m_writerCreated = false;

	UpdateSaveInfos();
}

void CUserProfileManagerDurango::GenerateSaveFileName( SSavegameInfo& info ) 
{
	if ( GUseSaveHack )
	{
		if ( info.m_customFilename )
		{
			return;
		}
	}
	else
	{
		info.m_customFilename = false; // no custom filenames on durango
	}

	const Char* prefix = TXT("");
	switch ( info.m_slotType )
	{
	case SGT_AutoSave:
		prefix = TXT("AutoSave_");
		break;
	case SGT_QuickSave:
		if ( GUseSaveHack )
		{
			prefix = TXT("QuickSave_");
			break;
		}
		// no brake after if intentional
	case SGT_Manual:
		prefix = TXT("ManualSave_");
		break;
	case SGT_CheckPoint:
	case SGT_ForcedCheckPoint:
		prefix = TXT("CheckPoint_");
		break;
	}

	if ( GUseSaveHack )
	{
		info.m_filename = String::Printf( TXT("%ls%d_%lx_%lx_%lx"),
			prefix, info.m_slotIndex, info.m_displayNameIndex, info.m_timeStamp.GetDateRaw(), info.m_timeStamp.GetTimeRaw() );
	}
	else
	{
		info.m_filename = String::Printf( TXT("%ls%d"),	prefix, info.m_slotIndex );
	}
}

void CUserProfileManagerDurango::GetSaveFiles( TDynArray< SSavegameInfo >& files ) const 
{
	ASSERT( m_saveInfosUpdateState.GetValue() == LIST_Updated );
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );

	if ( GUseSaveHack )
	{
 		TDynArray< String > allSaveFiles;
		GFileManager->FindFiles( TXT("d:\\gamesaves\\"), TXT("*.sav"), allSaveFiles, false );

		// Output files
		files.Resize( allSaveFiles.Size() );
		for ( Uint32 i=0; i<allSaveFiles.Size(); i++ )
		{
			CFilePath filePath( allSaveFiles[ i ] );
			files[ i ].m_filename = filePath.GetFileName();
			files[ i ].m_timeStamp = GFileManager->GetFileTime( String( TXT("d:\\gamesaves\\") ) + files[ i ].GetFileName() + files[ i ].GetSaveExtension() );
			ParseFilename( files[ i ] );
		}
	}
	else
	{
		if ( m_loadSaveState.GetValue() == STATE_NoSaveMode )
		{
			files.Clear();
			return;
		}

		// the list is already prepared 	
		STATE_CHECK( STATE_Ready )
		files = m_saveInfos;
	}
}

Bool CUserProfileManagerDurango::GetLatestSaveFile( SSavegameInfo& info ) const 
{
	ASSERT( m_saveInfosUpdateState.GetValue() == LIST_Updated );

	if ( m_loadSaveState.GetValue() == STATE_NoSaveMode )
	{
		return false;
	}

	Bool retVal = false;
	info.Clear();
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );
	for ( const auto& save : m_saveInfos )
	{
		if ( save.m_timeStamp > info.m_timeStamp )
		{
			retVal = true;
			info = save;
		}
	}
	return retVal;
}

void CUserProfileManagerDurango::UpdateSaveInfos()
{
	using namespace Windows::Xbox::Storage;

	ASSERT( m_saveInfosUpdateState.GetValue() == LIST_NeedsUpdate );

	SET_STATE( STATE_Updating );
	m_saveInfosUpdateState.SetValue( LIST_Updating );

	ContainerInfoQueryResult^ res = m_connectedStorageSpace->CreateContainerInfoQuery( nullptr );
	auto op = res->GetContainerInfo2Async();
	op->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler< Windows::Foundation::Collections::IVectorView< ContainerInfo2 >^ >(		
		[=]( Windows::Foundation::IAsyncOperation< Windows::Foundation::Collections::IVectorView< ContainerInfo2 >^ >^ operation, Windows::Foundation::AsyncStatus status )
		{
			LOG_ENGINE(TXT("UpdateSaveInfos AsyncStatus: %ls"), GetAsyncStatusTxtForLog( status ) );

			switch( status )
			{
				case Windows::Foundation::AsyncStatus::Completed:
					{
						Windows::Foundation::Collections::IVectorView< ContainerInfo2 >^ inf = operation->GetResults();

						Char *end;
						Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );
						m_saveInfos.ClearFast();
						for ( auto ci : inf )
						{
							if ( ci.Name == TXT("settings" ) )
							{
								continue;
							}

							SSavegameInfo& info = m_saveInfos[ Uint32( m_saveInfos.Grow( 1 ) ) ];
							info.Clear();
							info.m_filename = ci.Name->Data();

							CDateTime dt;
							dt.ImportFromOldFileTimeFormat(	ci.LastModifiedTime.UniversalTime );
							info.m_timeStamp = dt;

							Red::StringToInt( info.m_displayNameIndex, ci.DisplayName->Data(), &end, Red::System::Base::BaseSixteen );
							ParseFilename( info );
						}

						OnSaveInfosUpdated();
					}
					break;
				case Windows::Foundation::AsyncStatus::Error:
				case Windows::Foundation::AsyncStatus::Canceled:

					extern Windows::UI::Core::CoreDispatcher^ GCoreDispatcher;
					GCoreDispatcher->RunAsync
					(
						Windows::UI::Core::CoreDispatcherPriority::Normal,
						ref new Windows::UI::Core::DispatchedHandler
						(
							[ this ]()
							{
								LOG_ENGINE(TXT("UpdateSaveInfos calling RefreshStateComplete( eRS_ConnectedStorage ) on error/cancellation"));
								RefreshStateComplete( eRS_ConnectedStorage );
							}
						)
					);
					break;
			}
		}
	);
}

void CUserProfileManagerDurango::OnSaveInfosUpdated()
{
	SET_STATE( STATE_Ready );
	m_saveInfosUpdateState.SetValue( LIST_Updated_EventPending );
}

Bool CUserProfileManagerDurango::AreSavesInitialized() const
{
	return m_loadSaveState.GetValue() == STATE_Ready;
}

Int16 CUserProfileManagerDurango::GetNumSaveSlots( ESaveGameType type ) const
{
	switch ( type )
	{
		case SGT_AutoSave:
			return NUM_AUTOSAVE_SLOTS;
		case SGT_QuickSave:
		case SGT_Manual:
			return NUM_QUICKMANUAL_SLOTS;
		case SGT_CheckPoint:
		case SGT_ForcedCheckPoint:
			return NUM_CHECKPOINT_SLOTS;
		case SGT_None:
		default:
			ASSERT( false );
			return -1;
	}
}

Bool CUserProfileManagerDurango::GetSaveInSlot( ESaveGameType type, Int16 slot, SSavegameInfo& info ) const
{
	ASSERT( m_saveInfosUpdateState.GetValue() == LIST_Updated );

	if ( m_loadSaveState.GetValue() == STATE_NoSaveMode )
	{
		return false;
	}

	STATE_CHECK( STATE_Ready )

	Int32 index = FindSaveInfoIndex( type, slot );
	if ( index >= 0 )
	{
		info = m_saveInfos[ index ];
		return true;
	}

	return false;
}

Int32 CUserProfileManagerDurango::FindSaveInfoIndex( ESaveGameType type, Int16 slotIndex ) const
{
	ASSERT( m_saveInfosUpdateState.GetValue() == LIST_Updated );

 	// validate
	if ( type == SGT_None )
	{
		return -1;
	}

	if ( slotIndex < 0 )
	{
		return -1;
	}

	if ( type == SGT_ForcedCheckPoint )
	{
		type = SGT_CheckPoint;
	}

	if ( type == SGT_QuickSave )
	{
		type = SGT_Manual;
	}

	// search
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );
	for ( Int32 i = 0; i < m_saveInfos.SizeInt(); ++i )
	{
		const SSavegameInfo& si = m_saveInfos[ i ];
		if ( si.m_slotType == type && si.m_slotIndex == slotIndex )
		{
			return i;
		}
	}

	return -1;
}

void CUserProfileManagerDurango::DeleteSaveGame( const SSavegameInfo& info )
{
	if ( m_loadSaveState.GetValue() == STATE_NoSaveMode )
	{
		return;
	}

	STATE_CHECK( STATE_Ready )

	const String& saveName = info.GetFileName();
	Platform::String^ containerName = ref new Platform::String( saveName.AsChar(), saveName.GetLength() );

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );
	for ( Uint32 i = 0; i < m_saveInfos.Size(); ++i )
	{
		if ( m_saveInfos[ i ].GetFileName() == info.GetFileName() )
		{
			m_saveInfos.Erase( m_saveInfos.Begin() + i );
			break;
		}
	}

	SET_STATE( STATE_DeletingSave );

	auto op = m_connectedStorageSpace->DeleteContainerAsync( containerName );
	op->Completed = ref new Windows::Foundation::AsyncActionCompletedHandler
	(
		[=]( Windows::Foundation::IAsyncAction^ asyncInfo, Windows::Foundation::AsyncStatus status )
		{
			LOG_ENGINE(TXT("DeleteContainerAsync AsyncStatus: %ls"), GetAsyncStatusTxtForLog( status ) );

			// regardless of the result, we should just refresh the list
			OnSaveGameDeleted();
		}
	);
}

void CUserProfileManagerDurango::OnSaveGameDeleted()
{
	m_saveInfosUpdateState.SetValue( LIST_NeedsUpdate );
	SET_STATE( STATE_Ready );
}

void CUserProfileManagerDurango::PrepareConnectedStorage()
{
	RED_FATAL_ASSERT( m_activeUserObject, "Cannot prepare connected storage when there is no user logged in" );

	SET_STATE( STATE_Updating );
	m_userSettingsState.SetValue( SETTINGS_Loading );
	m_currentLoadOpProgress.SetValue( LOAD_NotInitialized );

	if( m_connectedStorageSpaceAsyncOperation )
	{
		m_connectedStorageSpaceAsyncOperation->Cancel();
	}

	m_connectedStorageSpaceAsyncOperation = ConnectedStorageSpace::GetForUserAsync( m_activeUserObject );
	m_connectedStorageSpaceAsyncOperation->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler< ConnectedStorageSpace^ >
	(
		[=]( Windows::Foundation::IAsyncOperation< ConnectedStorageSpace^ >^ operation, Windows::Foundation::AsyncStatus status )
		{
			LOG_ENGINE(TXT("PrepareConnectedStorage AsyncStatus: %ls"), GetAsyncStatusTxtForLog( status ) );

			switch( status )
			{
				case Windows::Foundation::AsyncStatus::Completed:
					m_connectedStorageSpace = operation->GetResults();
					UpdateUserSettings();
					UpdateSaveInfos();
					break;

				case Windows::Foundation::AsyncStatus::Error:
				case Windows::Foundation::AsyncStatus::Canceled:
					m_userSettingsState.SetValue( SETTINGS_NotAvailable );

					extern Windows::UI::Core::CoreDispatcher^ GCoreDispatcher;
					GCoreDispatcher->RunAsync
					(
						Windows::UI::Core::CoreDispatcherPriority::Normal,
						ref new Windows::UI::Core::DispatchedHandler
						(
							[ this ]()
							{
								LOG_ENGINE(TXT("PrepareConnectedStorage calling RefreshStateComplete( eRS_ConnectedStorage ) on error/cancellation"));
								RefreshStateComplete( eRS_ConnectedStorage );
							}
						)
					);

					break;
			}

			m_connectedStorageSpaceAsyncOperation = nullptr;
		}
	);
}

void CUserProfileManagerDurango::RequestModalUIBox( SUserDialogRequest::EMessageId type )
{
	SUserDialogRequest newRequest( type );
	ASSERT( newRequest.IsValid() );
	ASSERT( false == m_modalBox.IsValid() );

	m_modalBox = newRequest;
	SET_STATE( STATE_ModalUI );
}


void CUserProfileManagerDurango::OnModalUIBoxClosed()
{
	ASSERT( m_modalBox.IsValid() );

	switch ( m_modalBox.m_type )
	{
		case SUserDialogRequest::QUESTION_WouldYouLikeToContinueWithoutSaving:
		{
			SET_STATE( STATE_NoSaveMode );

			if ( m_modalBox.m_result == SUserDialogRequest::RESULT_No )
			{
				PromptUserSignInAsync( GetFirstEngagedGamepad() );
			}
			break;
		}
		case SUserDialogRequest::WARNING_LoadingFailedDamagedData:
		case SUserDialogRequest::WARNING_NoSpaceForSave:
		{
			SET_STATE( STATE_Ready );
			break;
		}
		case SUserDialogRequest::QUESTION_DebugTest:
		{
			if ( m_modalBox.m_result == SUserDialogRequest::RESULT_Yes )
			{
				RED_LOG( Save, TXT("Debug yes") );
			}
			else
			{
				RED_LOG( Save, TXT("Debug no") );
			}
			break;
		}
	}
}


void CUserProfileManagerDurango::ParseFilename( SSavegameInfo& info )
{
	ASSERT( false == info.m_filename.Empty() );

	if ( GUseSaveHack )
	{
		if ( info.m_customFilename )
		{
			info.m_slotIndex = 0;
			info.m_slotType = SGT_Manual;
			return;
		}
	}

	// Filename format:
	// Type_Slot_DisplayNameIndex_Date_Time
	// Where: 
	// Type : AutoSave, QuickSave, or ManualSave
	// Slot : int
	//
	// +++++++++++++ hack version ++++++++++++++++
	// DisplayNameIndex: uint
	// Date: raw hex uint
	// Time: raw hex uint
	
	CTokenizer tokenizer( info.m_filename, TXT("_") );
	if ( tokenizer.GetNumTokens() != ( GUseSaveHack ? 5 : 2 ) )
	{
		RED_LOG( Save, TXT("Cannot parse save filename: %s"), info.m_filename.AsChar() );
		info.m_customFilename = true;
		info.m_slotType = SGT_Manual;
		return;
	}

	const String firstToken = tokenizer.GetToken( 0 );
	if ( firstToken.EqualsNC( TXT("AutoSave") ) )
	{
		info.m_slotType = SGT_AutoSave;
	}
	else if ( GUseSaveHack && firstToken.EqualsNC( TXT("QuickSave") ) )
	{
		info.m_slotType = SGT_QuickSave;
	}
	else if ( firstToken.EqualsNC(  TXT("ManualSave") ) )
	{
		info.m_slotType = SGT_Manual;
	}
	else if ( firstToken.EqualsNC(  TXT("CheckPoint") ) )
	{
		info.m_slotType = SGT_CheckPoint;
	}
	else
	{
		info.m_slotType = SGT_Manual;
		info.m_customFilename = true;
		RED_LOG( Save, TXT("Cannot parse save filename: %s"), info.m_filename.AsChar() );
		return;
	}

	#define SSGI_VERIFY_PARSE_STEP( x ) if ( false == x ) { info.m_customFilename = true; info.m_slotType = SGT_Manual; RED_LOG( Save, TXT("Cannot parse save filename: %s"), info.m_filename.AsChar() ); return; }
	
	String secondToken = tokenizer.GetToken( 1 );
	Int32 number;
	Char *end;
	SSGI_VERIFY_PARSE_STEP( Red::StringToInt( number, secondToken.AsChar(), &end, Red::System::Base::BaseTen ) || number < -1 || number > SLOT_SANITY_CHECK )
	info.m_slotIndex = Int16( number );

	if ( GUseSaveHack )
	{
		String thirdToken = tokenizer.GetToken( 2 );
		Uint32 unumber;
		SSGI_VERIFY_PARSE_STEP( Red::StringToInt( unumber, thirdToken.AsChar(), &end, Red::System::Base::BaseSixteen ) )
		info.m_displayNameIndex = unumber;

		String fourthToken = tokenizer.GetToken( 3 );
		SSGI_VERIFY_PARSE_STEP( Red::StringToInt( unumber, fourthToken.AsChar(), &end, Red::System::Base::BaseSixteen ) )
		info.m_timeStamp.SetDateRaw( unumber );

		String fifthToken = tokenizer.GetToken( 4 );
		SSGI_VERIFY_PARSE_STEP( Red::StringToInt( unumber, fifthToken.AsChar(), &end, Red::System::Base::BaseSixteen ) )
		info.m_timeStamp.SetTimeRaw( unumber );
	}
}


Bool CUserProfileManagerDurango::LoadUserSettings( StringAnsi& settingsString )
{
#ifdef RED_LOGGING_ENABLED
	CTimeCounter timer;
#endif
	while ( m_userSettingsState.GetValue() == SETTINGS_Loading )
	{
		Red::Threads::YieldCurrentThread();
	}

	Bool returnValue( false );
	if ( m_userSettingsState.GetValue() == SETTINGS_Ready )
	{
		Uint8* bytes = GetBufferByteAccess( m_userSettingsBuffer );
		if ( bytes )
		{
			settingsString.Resize( m_userSettingsBuffer->Length + 1 ); 
			Red::MemoryCopy( settingsString.Data(), bytes, m_userSettingsBuffer->Length );
			settingsString[ m_userSettingsBuffer->Length ] = 0;
			returnValue = true;
		}
	}

	LOG_ENGINE(TXT("CUserProfileManagerDurango::LoadUserSettings finished in %1.2f sec"), timer.GetTimePeriod());

	return returnValue;	 
}

Bool CUserProfileManagerDurango::SaveUserSettings( const StringAnsi& settingsString )
{
	// This should be changed to PostSignOut or SignOutCompleted event in future.
	// Right now there is a chance that we can save settings after UE_SignOut event (but then no settings exists)
	if( m_ignoreSaveUserSettingsRequest.GetValue() == true )
	{
		return false;
	}

	if ( m_loadSaveState.GetValue() != STATE_Ready )
	{
		return false;
	}

	m_userSettingsBuffer->Length = Min( settingsString.GetLength(), MAX_USERSETTINGS_SIZE );
	Uint8* bytes = GetBufferByteAccess( m_userSettingsBuffer );
	if ( nullptr == bytes )
	{
		return false;
	}

	m_userSettingsState.SetValue( SETTINGS_Saving );

	Red::MemoryCopy( bytes, settingsString.Data(), m_userSettingsBuffer->Length );

	auto container = m_connectedStorageSpace->CreateContainer( ref new Platform::String( TXT("settings") ) );
    auto updates = ref new Platform::Collections::Map< Platform::String^, IBuffer^ > ();
    updates->Insert( "str", m_userSettingsBuffer );

    auto op = container->SubmitUpdatesAsync( updates->GetView(), nullptr );
	op->Completed = ref new Windows::Foundation::AsyncActionCompletedHandler(
		[=] ( Windows::Foundation::IAsyncAction^ a, Windows::Foundation::AsyncStatus status )
		{
			LOG_ENGINE(TXT("SaveUserSettings AsyncStatus: %ls"), GetAsyncStatusTxtForLog( status ) );

			switch ( status )
			{
				case Windows::Foundation::AsyncStatus::Completed:
					OnUserSettingsSaved();	
				break;

				case Windows::Foundation::AsyncStatus::Error:
				case Windows::Foundation::AsyncStatus::Canceled:
				default:
					OnUserSettingsSavingFailed();
					break;
			}
		}
	);

	return true;
}

void CUserProfileManagerDurango::UpdateUserSettings()
{
	m_userSettingsState.SetValue( SETTINGS_Loading );

	Platform::String^ containerName = ref new Platform::String( TXT("settings") );
	auto container = m_connectedStorageSpace->CreateContainer( containerName );

	m_userSettingsBuffer->Length = 0;
	auto reads = ref new Platform::Collections::Map< Platform::String^, Windows::Storage::Streams::IBuffer^ > ();
	reads->Insert( "str", m_userSettingsBuffer );

	auto op = container->ReadAsync( reads->GetView() );
	op->Completed = ref new Windows::Foundation::AsyncActionCompletedHandler
	(
		[ this ]( Windows::Foundation::IAsyncAction^, Windows::Foundation::AsyncStatus status )
		{
			LOG_ENGINE(TXT("UpdateUserSettings AsyncStatus: %ls"), GetAsyncStatusTxtForLog( status ) );

			extern Windows::UI::Core::CoreDispatcher^ GCoreDispatcher;

			GCoreDispatcher->RunAsync
			(
				Windows::UI::Core::CoreDispatcherPriority::Normal,
				ref new Windows::UI::Core::DispatchedHandler
				(
					[ this, status ]()
					{
						switch ( status )
						{
							case Windows::Foundation::AsyncStatus::Completed:
								OnUserSettingsLoaded();
							break;

							case Windows::Foundation::AsyncStatus::Error:
							case Windows::Foundation::AsyncStatus::Canceled:
							default:
								OnUserSettingsLoadingFailed();
								break;
						}
					}
				)
			);
		}
	);
}

void CUserProfileManagerDurango::OnUserSettingsLoaded()
{
	m_userSettingsState.SetValue( SETTINGS_Ready );

	RefreshStateComplete( eRS_ConnectedStorage );
}


void CUserProfileManagerDurango::OnUserSettingsLoadingFailed()
{
	RED_LOG( Save, TXT("Can't load user settings from storage.") );
	m_userSettingsState.SetValue( SETTINGS_NotAvailable );

	RefreshStateComplete( eRS_ConnectedStorage );
}

void CUserProfileManagerDurango::OnUserSettingsSaved()
{
	m_userSettingsState.SetValue( SETTINGS_Ready );
}


void CUserProfileManagerDurango::OnUserSettingsSavingFailed()
{
	RED_LOG( Save, TXT("Can't save user settings in storage.") );
	m_userSettingsState.SetValue( SETTINGS_NotAvailable );
}

Uint8* CUserProfileManagerDurango::GetBufferByteAccess( IBuffer^ buf )
{
	using namespace Windows::Storage::Streams;
	IUnknown* unknown = reinterpret_cast< IUnknown* > ( buf );
	Microsoft::WRL::ComPtr< IBufferByteAccess > bufferByteAccess;
	HRESULT hr = unknown->QueryInterface( _uuidof( IBufferByteAccess ), &bufferByteAccess );
	if ( FAILED( hr ) )
	{
		HALT( "Failed to get the byte access to save buffer. DEBUG!" );
		return nullptr;
	}

	Uint8* bytes = nullptr;
	bufferByteAccess->Buffer( &bytes );
	return bytes;
}

String CUserProfileManagerDurango::BuildFullDisplayNameForSave( const SSavegameInfo& info ) const
{
	// sorry, can't find any API that allows me to use default user locale to convert the datetime to string properly and WORKS on X1
	// TEMPSHIT "solution" is this:

	return String::Printf( TXT("%ls - %02ld.%02ld.%04ld %02ld:%02ld:%02ld"), info.GetDisplayName().AsChar(),
		info.m_timeStamp.GetDay() + 1, info.m_timeStamp.GetMonth() + 1, info.m_timeStamp.GetYear(), info.m_timeStamp.GetHour(), info.m_timeStamp.GetMinute(), info.m_timeStamp.GetSecond() );

	// PLEASE NOTE that this ekhem.. "solution"... also doesn't take into account the fact, that local time isn't always the same as UTC... :(
	// ( sadness = 100 )
	//
	// ok, jokes aside...
	//
	// TODO: replace this IMPL with the proper one as soon as i figure out what API is the one i should use here.
}

Bool CUserProfileManagerDurango::IsSaveSystemBusy() const
{
	if( m_initializedLoadSaveInfoIndex != -1 )
	{
		return true;
	}
	else
	{
		switch( m_loadSaveState.GetValue() )
		{
		case STATE_DeletingSave:
		case STATE_Saving:

			return true;
		}
	}

		return false;
}
