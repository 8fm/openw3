/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include <locale.h>
#include "userProfileOrbis.h"
#include "orbisApiCall.h"
#include <common_dialog.h>
#include <libsysmodule.h>
#include <system_service.h>

#include "../../common/engine/compressedFile.h"
#include "../../common/engine/gameSaveManager.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/engine/gameSession.h"

#include "../../common/core/contentManager.h"
#include "../../common/core/memoryFileReader.h"
#include "../../common/core/memoryFileWriter.h"
#include "../../common/core/tokenizer.h"
#include "../../common/core/chunkedLZ4File.h"

#include "../../common/core/compression/compression.h"
#include "../../common/core/compression/lz4.h"

#include "../../common/game/gameSaver.h"
#include "../../common/game/commonGame.h"
#include "../../common/game/guiManager.h"
#include "streamingInstallerOrbis.h"

#pragma comment( lib, "libSceSaveData_stub_weak.a" )
#pragma comment( lib, "libSceSaveDataDialog_stub_weak.a" )

// Save-game compression values
const Uint32 c_saveGameCompressedChunkSize = 1024 * 1024;			// Size of chunks (uncompressed)
const Uint32 c_saveGameMaxCompressedChunks = 256;					// Max chunks
const Uint32 c_saveGameCompressedDataBufferSize = 3 * 1024 * 1024;	// initial size of data buffer for compression results 	

class CDelayedFileWriter : public CMemoryFileWriter
{
protected:
	String													m_platformFilePath;
	TDynArray< Uint8, MC_Gameplay, MemoryPool_Default >&	m_data;

public:
	CDelayedFileWriter( const String& absolutePath, TDynArray< Uint8, MC_Gameplay, MemoryPool_Default >& data )
		: CMemoryFileWriter( data )
		, m_platformFilePath( absolutePath )
		, m_data( data )
	{
		data.ClearFast();
	}

	~CDelayedFileWriter()
	{
		IFile* platformFile = GFileManager->CreateFileWriter( m_platformFilePath, FOF_AbsolutePath );
		RED_FATAL_ASSERT( platformFile, "Failed to open save game file for writing" );
		
		if ( platformFile )
		{
			platformFile->Serialize( m_data.Data(), m_data.Size() );
			delete platformFile;
		}

		m_data.ClearFast();
	}
};

Bool CUserProfileManagerOrbis::RequiresPriorityIO() const
{
	// At least this is where the save system is expected to be updated, so query its state here too for consistency
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only!" );

	return ( m_currentSaveOpProgress != SAVE_NotInitialized ) || IsDoingAsyncOp() || ( false == m_settingsToSave.Empty() );
}

ELoadGameResult CUserProfileManagerOrbis::InitGameLoading( const SSavegameInfo& info )
{
	RED_LOG( Profile, TXT("InitGameLoading() called, m_initializedLoadSaveInfoIndex was %d"), m_initializedLoadSaveInfoIndex );

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );

	ASSERT( m_initializedLoadSaveInfoIndex < 0 );
	ASSERT( LOAD_NotInitialized == m_currentLoadOpProgress );
	ASSERT( SAVE_NotInitialized == m_currentSaveOpProgress );
	ASSERT( SCR_Requested != m_screenshotReadingRequestStatus.GetValue() );
	ASSERT( !IsDoingAsyncOp() );

	if ( !HasActiveUser() )
	{
		RED_LOG( Profile, TXT("Trying to load game while no user is signed in.") );
		m_currentLoadOpProgress = LOAD_Error;
		return LOAD_Error;
	}

	for ( Int32 i = 0; i < m_saveInfos.SizeInt(); ++i )
	{
		if ( m_saveInfos[ i ].m_filename.EqualsNC( info.m_filename ) )
		{
			m_initializedLoadSaveInfoIndex = i;
			break;
		}
	}

	if ( m_initializedLoadSaveInfoIndex >= 0 )
	{
		CreateMountTask();
		{
			m_mountTask->SetDirName( info.GetFileName().AsChar() );
			m_mountTask->SetMountMode( SCE_SAVE_DATA_MOUNT_MODE_RDONLY );
		}
		RunMountTask();

		m_currentLoadOpProgress = LOAD_Initializing;
		return LOAD_Initializing;		
	}

	m_currentLoadOpProgress = LOAD_Error;
	return LOAD_Error;
}


void CUserProfileManagerOrbis::UpdateGameLoadingInit()
{
	RED_LOG( Profile, TXT("UpdateGameLoadingInit() called, m_initializedLoadSaveInfoIndex was %d"), m_initializedLoadSaveInfoIndex );

	ASSERT( m_initializedLoadSaveInfoIndex >= 0 );
	ASSERT( m_currentLoadOpProgress == LOAD_Initializing );
	ASSERT( m_mountTask );

	if ( !m_mountTask->IsFinished() )
	{
		return;
	}

	if ( false == m_mountTask->GetResult() )
	{
		m_currentLoadOpProgress = LOAD_Error;
		return;
	}

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );
	SSavegameInfo& info = m_saveInfos[ m_initializedLoadSaveInfoIndex ];

	// load and verify required content
	{
		m_requiredContentString.Clear();
		TDynArray< Uint8 > data;
	
		String filePath = ANSI_TO_UNICODE( m_saveDataMountResult.mountPoint.data );
		filePath += L'/';
		filePath += info.GetFileName();
		filePath += info.GetRequiredContentExtension();

		GFileManager->LoadFileToBuffer( filePath, data, false );
		m_requiredContentString.Resize( data.Size() + 1 );
		Red::MemoryCopy( m_requiredContentString.Data(), data.Data(), data.Size() );
		m_requiredContentString[ data.Size() ] = '\0';

		TDynArray< CName > requiredContent = GetContentRequiredByLastSave();
		for ( auto name : requiredContent )
		{
			if ( false == GContentManager->IsContentAvailable( name ) )
			{
				m_initializedLoadSaveInfoIndex = -1;
				m_currentLoadOpProgress = LOAD_MissingContent;

				CreateUnMountTask();
				RunUnMountTask();

				return;
			}
		}

		m_currentLoadOpProgress = LOAD_ReadyToLoad;
	}
}

ELoadGameResult CUserProfileManagerOrbis::GetLoadGameProgress() const 
{
	RED_LOG( Profile, TXT("GetLoadGameProgress() called, m_currentLoadOpProgress was %d"), m_currentLoadOpProgress );
	return m_currentLoadOpProgress;
}

void CUserProfileManagerOrbis::CancelGameLoading()
{
	RED_LOG( Profile, TXT("CancelGameLoading() called, m_initializedLoadSaveInfoIndex was %d"), m_initializedLoadSaveInfoIndex );

	m_initializedLoadSaveInfoIndex = -1;
	m_currentLoadOpProgress = LOAD_NotInitialized; 

	if ( IsSaveDataMounted() )
	{
		// finish current async op now
		while ( IsDoingAsyncOp() )
		{
			UpdateSaveSystem();
			if ( IsDoingAsyncOp() )
			{
				Red::Threads::SleepOnCurrentThread( 10 );
			}
		}

		CreateUnMountTask();
		RunUnMountTask();
	}
}

IFile* CUserProfileManagerOrbis::CreateSaveFileReader(const Bool rawFile)
{
	RED_LOG( Profile, TXT("CreateSaveFileReader() called, m_initializedLoadSaveInfoIndex was %d, m_currentLoadOpProgress was %d"), m_initializedLoadSaveInfoIndex, m_currentLoadOpProgress );

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );

	ASSERT( m_currentLoadOpProgress == LOAD_ReadyToLoad );
	ASSERT( m_initializedLoadSaveInfoIndex >= 0 );
	ASSERT( m_saveDataMountResult.mountPoint.data[ 0 ] == '/' ); 
	ASSERT( SCR_Requested != m_screenshotReadingRequestStatus.GetValue() );

	const SSavegameInfo& info = m_saveInfos[ m_initializedLoadSaveInfoIndex ];

	String filePath = ANSI_TO_UNICODE( m_saveDataMountResult.mountPoint.data );
	filePath += L'/';
	filePath += info.GetFileName();
	filePath += info.GetSaveExtension();

	m_currentLoadOpProgress = LOAD_Loading;

	IFile* fileReaderRaw = GFileManager->CreateFileReader( filePath, FOF_AbsolutePath );
	if( !fileReaderRaw )
	{
		RED_FATAL_ASSERT( false, "Failed to open save file for reading" );
		return nullptr;
	}

	// use the raw file reader
	if ( rawFile )
		return fileReaderRaw;

	// Create save file reader based on header value.
	Uint32 magicHeader = 0;
	*fileReaderRaw << magicHeader;
	if( magicHeader == SAVE_NEW_FORMAT_HEADER )
	{
		// The file is new format save-game
		return new CChunkedLZ4FileReader( fileReaderRaw );
	}
	else
	{
		// Old format saves use compressed file
		delete fileReaderRaw;
		return new CCompressedFileReader( filePath );
	}
}

void CUserProfileManagerOrbis::RequestScreenshotDataForReading( const SSavegameInfo& info )
{
	RED_LOG( Profile, TXT("RequestScreenshotDataForReading() called, m_initializedLoadSaveInfoIndex was %d, m_screenshotReadingRequestStatus was %d"), m_initializedLoadSaveInfoIndex, m_screenshotReadingRequestStatus.GetValue() );

	ASSERT( SCR_Requested != m_screenshotReadingRequestStatus.GetValue() );
	Int32 listIndex = FindSaveInfoIndex( info.m_slotType, info.m_slotIndex );
	if ( listIndex >= 0 )
	{
		m_screenshotReadingRequestStatus.SetValue( SCR_Requested );
		m_screenshotReadingRequestInfoIndex.SetValue( listIndex );
	}
	else
	{
		m_screenshotReadingRequestStatus.SetValue( SCR_Failed );
	}
}

Bool CUserProfileManagerOrbis::IsScreenshotDataReadyForReading()
{
	RED_LOG( Profile, TXT("IsScreenshotDataReadyForReading() called, m_initializedLoadSaveInfoIndex was %d, m_screenshotReadingRequestStatus was %d"), m_initializedLoadSaveInfoIndex, m_screenshotReadingRequestStatus.GetValue() );
	return m_screenshotReadingRequestStatus.GetValue() == SCR_Ready;
}

IFile* CUserProfileManagerOrbis::CreateScreenshotDataReader()
{
	RED_LOG( Profile, TXT("CreateScreenshotDataReader() called, m_initializedLoadSaveInfoIndex was %d, m_screenshotReadingRequestStatus was %d"), m_initializedLoadSaveInfoIndex, m_screenshotReadingRequestStatus.GetValue() );
	if ( IsScreenshotDataReadyForReading() )
	{
		return new CMemoryFileReaderExternalBuffer( GetScreenshotBuffer(), m_realScreenshotSize );
	}
	else
	{
		return new CMemoryFileReaderExternalBuffer( CGameSaver::DEFAULT_SCREENSHOT_DATA, CGameSaver::DEFAULT_SCREENSHOT_SIZE );
	}
}

void CUserProfileManagerOrbis::DoneReadingScreenshotData()
{
	RED_LOG( Profile, TXT("DoneReadingScreenshotData() called, m_initializedLoadSaveInfoIndex was %d, m_screenshotReadingRequestStatus was %d"), m_initializedLoadSaveInfoIndex, m_screenshotReadingRequestStatus.GetValue() );
	m_screenshotReadingRequestStatus.SetValue( SCR_NotRequested );
}

void CUserProfileManagerOrbis::DoSyncReadScreenshotData(const SSavegameInfo &info)
{
	RED_LOG( Profile, TXT("DoSyncReadScreenshotData() called, m_initializedLoadSaveInfoIndex was %d, m_screenshotReadingRequestStatus was %d"), m_initializedLoadSaveInfoIndex, m_screenshotReadingRequestStatus.GetValue() );

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );

	ASSERT( m_initializedLoadSaveInfoIndex < 0 );
	ASSERT( m_saveDataMountResult.mountPoint.data[ 0 ] == '\0' ); 
	ASSERT( m_currentLoadOpProgress == LOAD_NotInitialized );
	ASSERT( SAVE_NotInitialized == m_currentSaveOpProgress );
	ASSERT( SCR_Requested == m_screenshotReadingRequestStatus.GetValue() );

	if ( !HasActiveUser() )
	{
		m_screenshotReadingRequestStatus.SetValue( SCR_Failed );
		RED_LOG( Save, TXT("Trying to load screenshot data while no user is signed in.") );
		return;
	}

	SceSaveDataDirName dirName;
	{
		Red::StringCopy( dirName.data, UNICODE_TO_ANSI( info.GetFileName().AsChar() ), SCE_SAVE_DATA_DIRNAME_DATA_MAXSIZE );
	}

	SceSaveDataMount mount;
	{
		Red::MemoryZero( &mount, sizeof( mount ) );
		mount.userId = m_userId;
		mount.titleId = nullptr; // use default
		mount.dirName = &dirName;
		mount.mountMode = SCE_SAVE_DATA_MOUNT_MODE_RDONLY;
	}

	SceSaveDataMountResult result;
	{
		Red::MemoryZero( &result, sizeof( result ) );
	}

	if ( false == MountSaveData( &mount, &result, true, false ) )
	{
		m_screenshotReadingRequestStatus.SetValue( SCR_Failed );
		return;
	}

	SceSaveDataIcon icon;
	{
		Red::MemoryZero( &icon, sizeof( icon ) );
		icon.buf = ( void* ) m_screenshotBuffer.GetData();
		icon.bufSize = CGameSaver::SCREENSHOT_BUFFER_SIZE;
	}

	const Int32 errorCode = sceSaveDataLoadIcon( &result.mountPoint, &icon );
	if ( errorCode < SCE_OK )
	{
		HandleSaveDataErrorCode( errorCode, mount.dirName, SCE_SAVE_DATA_DIALOG_TYPE_LOAD );
	}

	UnmountSaveData( &result, false );
	m_realScreenshotSize = icon.dataSize;
	m_screenshotReadingRequestStatus.SetValue( SCR_Ready );
}


void CUserProfileManagerOrbis::FinalizeGameLoading()
{
	ASSERT( m_initializedLoadSaveInfoIndex >= 0 );
	ASSERT( m_saveDataMountResult.mountPoint.data[ 0 ] == '/' ); 
	ASSERT( m_currentLoadOpProgress == LOAD_Loading );
	ASSERT( SCR_Requested != m_screenshotReadingRequestStatus.GetValue() );

	m_initializedLoadSaveInfoIndex = -1;
	m_currentLoadOpProgress = LOAD_NotInitialized; // no enum value for "finished"

	CreateUnMountTask();
	RunUnMountTask();
}

TDynArray< CName > CUserProfileManagerOrbis::GetContentRequiredByLastSave() const 
{
	TDynArray< CName > requiredContent;
	Uint32 length = 0, totalLength = m_requiredContentString.GetLength();
	const AnsiChar* strBuf = m_requiredContentString.AsChar();
	const AnsiChar* currentStr = m_requiredContentString.AsChar();
	AnsiChar buf[ 256 ];
	for ( Uint32 i = 0; i < totalLength; ++i )
	{
		if ( ';' == strBuf[ i ] )
		{
			if ( length > 0 )
			{
				length = Clamp< Uint32 >( length, 1, 255 );
				Red::MemoryCopy( buf, currentStr, length );
				buf[ length ] = '\0';

				requiredContent.PushBack( CName( ANSI_TO_UNICODE( buf ) ) );
				length = 0;
				currentStr = strBuf + i + 1;
			}
			else
			{
				HALT("Unexpected delimiter value inside content descriptor string. Please DEBUG.");
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

namespace Orbis
{
	extern CStreamingInstallerOrbis* GStreamingInstaller;
}

ESaveGameResult CUserProfileManagerOrbis::InitGameSaving( SSavegameInfo& info )
{
	if ( Orbis::GStreamingInstaller )
	{
		Orbis::GStreamingInstaller->SuspendInstallerAsync(); // suspend now so I/O bandwidth available ASAP. It'll autoresume itself RequiresPriorityIO() false.
	}

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );

	ASSERT( m_initializedLoadSaveInfoIndex < 0 );
	ASSERT( m_currentLoadOpProgress == LOAD_NotInitialized );
	ASSERT( SAVE_NotInitialized == m_currentSaveOpProgress );
	ASSERT( SCR_Requested != m_screenshotReadingRequestStatus.GetValue() );

	if ( !HasActiveUser() )
	{
		RED_LOG( Profile, TXT("Trying to save game while no user is signed in.") );
		return SAVE_Error;
	}

	// check if the user requested specific save slot
	m_initializedLoadSaveInfoIndex = FindSaveInfoIndex( ESaveGameType( info.m_slotType ), info.m_slotIndex );

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
				// find oldest or free autosave slot
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
	Bool createSaveData = false;
	if ( m_initializedLoadSaveInfoIndex < 0 )
	{
		m_initializedLoadSaveInfoIndex = m_saveInfos.SizeInt();
		m_saveInfos.PushBack( info );
		createSaveData = true;
	}
	else
	{
		m_saveInfos[ m_initializedLoadSaveInfoIndex ] = info;
	}

	RED_LOG( Profile, TXT("InitGameSaving(): creating mount task, m_initializedLoadSaveInfoIndex=%d, current mount point=%s"), m_initializedLoadSaveInfoIndex, m_saveDataMountResult.mountPoint.data );

	SceSaveDataDirName dirName;
	{
		Red::StringCopy( dirName.data, UNICODE_TO_ANSI( info.GetFileName().AsChar() ), SCE_SAVE_DATA_DIRNAME_DATA_MAXSIZE );
	}

	CreateMountTask();
	{
		m_mountTask->SetDirName( info.GetFileName().AsChar() );
		m_mountTask->SetBlocks( NUM_BLOCKS_PER_SAVE_SLOT );
		m_mountTask->SetMountMode( ( createSaveData ? SCE_SAVE_DATA_MOUNT_MODE_CREATE : 0 ) | SCE_SAVE_DATA_MOUNT_MODE_RDWR );

		// if we're overwriting existing save data, let it fail in the worst case.
		// existing save data can be corrupted, so we have to delete it, but first we need to find that out by trying to mount it
		if ( false == createSaveData )
		{
			m_mountTask->SetCanFail( true );
		}
	}
	RunMountTask();

	if ( info.m_slotType == SGT_Manual || info.m_slotType == SGT_QuickSave ) // tyoes triggered by user (no silent fail)
	{
		UpdateUserActionsLock( true );
	}

	m_currentSaveOpProgress = SAVE_Initializing;
	return SAVE_Initializing;
}


void CUserProfileManagerOrbis::UpdateGameSavingInit()
{
	RED_LOG( Profile, TXT("UpdateGameSavingInit() called, m_initializedLoadSaveInfoIndex=%d, current mount point=%s"), m_initializedLoadSaveInfoIndex, m_saveDataMountResult.mountPoint.data );

	ASSERT( m_initializedLoadSaveInfoIndex >= 0 );
	ASSERT( m_currentSaveOpProgress == SAVE_Initializing );
	
	const SSavegameInfo& info = m_saveInfos[ m_initializedLoadSaveInfoIndex ];

	if ( !m_mountTask )
	{
		// if mounting is not initiated, it means this is a second attempt after deleting the data
		CreateMountTask();
		{
			m_mountTask->SetDirName( info.GetFileName().AsChar() );
			m_mountTask->SetBlocks( NUM_BLOCKS_PER_SAVE_SLOT );
			m_mountTask->SetMountMode( SCE_SAVE_DATA_MOUNT_MODE_CREATE | SCE_SAVE_DATA_MOUNT_MODE_RDWR );
		}
		RunMountTask();
	}

	if ( !m_mountTask->IsFinished() )
	{
		return;
	}

	if ( false == m_mountTask->GetResult() )
	{
		UpdateUserActionsLock( false );

		if ( m_mountTask->CanFail() )
		{
			// this means existing save data was corrupted, delete it and retry, this time with m_canFail = false
			OnMountTaskFinished();
			
			// extra hack to avoid one specific situation, where we could display that message on loading screen
			if ( GGame->IsLoadingScreenShown() )
			{
				RED_LOG( Save, TXT("NOT calling OnCorruptedSaveDataOverwrite() event - there is a loading screen, so we can't display a user message anyway.") );
				m_currentSaveOpProgress = SAVE_Error;
				return;
			}
			else
			{
				CallFunction( GCommonGame->GetGuiManager(), CNAME( OnCorruptedSaveDataOverwrite ) );
				m_displayingCorruptedSaveOverwriteMessage = true;
			}

			return;
		}

		m_currentSaveOpProgress = SAVE_Error;
		return;
	}

	m_currentSaveOpProgress = SAVE_ReadyToSave;
}

ESaveGameResult CUserProfileManagerOrbis::GetSaveGameProgress() const 
{
	return m_currentSaveOpProgress;
}

void CUserProfileManagerOrbis::CancelGameSaving()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );

	RED_LOG( Profile, TXT("CancelGameSaving() called, m_initializedLoadSaveInfoIndex=%d, current mount point=%s"), m_initializedLoadSaveInfoIndex, m_saveDataMountResult.mountPoint.data );

	UpdateUserActionsLock( false );

	if ( IsMounting() )
	{
		// let it finish
		while ( !m_mountTask->IsFinished() )
		{
			Red::Threads::YieldCurrentThread(); 
		}

		// clean up
		m_mountTask->Release();
		m_mountTask = nullptr;
	}

	if ( IsSaveDataMounted() )
	{
		if ( SAVE_Initializing == m_currentSaveOpProgress || SAVE_ReadyToSave == m_currentSaveOpProgress )
		{
			// the savedata have been mounted, but no files inside have been touched yet
			// there are 2 options now: 1) a new savedata was created - we will have to delete it, as it's empty and we cannot leave it like this
			//							2) we were trying to overwrite existing savedata - just unmount it, files haven't been touched

			const SSavegameInfo& info = m_saveInfos[ m_initializedLoadSaveInfoIndex ];
			String filePath = ANSI_TO_UNICODE( m_saveDataMountResult.mountPoint.data );
			filePath += L'/';
			filePath += info.GetFileName();
			filePath += info.GetSaveExtension();

			if ( GFileManager->FileExist( filePath ) )
			{
				// this is option 2)
				CreateUnMountTask();
				RunUnMountTask();
			}
			else
			{
				// this is option 1)
				UnmountSaveData( &m_saveDataMountResult, false );
				CreateDeleteSaveDataTask();
				{
					m_deleteSaveDataTask->SetDirName( info.GetFileName().AsChar() );
				}
				RunDeleteSaveDataTask();
			}	
		}
		else
		{
			// cancelled at saving stage
			CreateUnMountTask();
			RunUnMountTask();
		}
	}

	m_initializedLoadSaveInfoIndex = -1;
	m_currentSaveOpProgress = SAVE_NotInitialized;
	m_saveInfosUpdateState.SetValue( LIST_NeedsUpdate );
}

IFileEx* CUserProfileManagerOrbis::CreateSaveFileWriter()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );

	ASSERT( m_initializedLoadSaveInfoIndex >= 0 );
	ASSERT( m_saveDataMountResult.mountPoint.data[ 0 ] == '/' ); 
	ASSERT( SAVE_ReadyToSave == m_currentSaveOpProgress );
	ASSERT( SCR_Requested != m_screenshotReadingRequestStatus.GetValue() );

	RED_LOG( Profile, TXT("CreateSaveFileWriter() called, m_initializedLoadSaveInfoIndex=%d, current mount point=%s"), m_initializedLoadSaveInfoIndex, m_saveDataMountResult.mountPoint.data );

	const SSavegameInfo& info = m_saveInfos[ m_initializedLoadSaveInfoIndex ];

	String filePath = ANSI_TO_UNICODE( m_saveDataMountResult.mountPoint.data );
	filePath += L'/';
	filePath += info.GetFileName();
	filePath += info.GetSaveExtension();

	IFile* delayedFile = new CDelayedFileWriter( filePath, m_saveBuffer );
	RED_FATAL_ASSERT( delayedFile, "Failed to open save game file for writing" );
	if ( nullptr == delayedFile )
	{
		return nullptr;
	}

	m_currentSaveOpProgress = SAVE_Saving;

	Uint32 magicHeader = SAVE_NEW_FORMAT_HEADER;
	*delayedFile << magicHeader;

	// Wrap it in compression
	return new CChunkedLZ4FileWriter( c_saveGameCompressedChunkSize, c_saveGameMaxCompressedChunks, delayedFile );
}

void* CUserProfileManagerOrbis::GetScreenshotBuffer()
{
	return m_screenshotBuffer.GetData();
}

void CUserProfileManagerOrbis::OnScreenshotDone( Uint32 realSize )
{
	ASSERT( SCR_Requested != m_screenshotReadingRequestStatus.GetValue() );
	RED_LOG( Save, TXT("Screenshot taken, size: %ld"), realSize );
	m_realScreenshotSize = realSize;
}

void CUserProfileManagerOrbis::FinalizeGameSaving( IGameSaver* saverToFinalize )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );

	ASSERT( m_initializedLoadSaveInfoIndex >= 0 );
	ASSERT( m_saveDataMountResult.mountPoint.data[ 0 ] == '/' ); 
	ASSERT( SAVE_Saving == m_currentSaveOpProgress );
	ASSERT( SCR_Requested != m_screenshotReadingRequestStatus.GetValue() );

	RED_LOG( Profile, TXT("FinalizeGameSaving() called, m_initializedLoadSaveInfoIndex=%d, current mount point=%s"), m_initializedLoadSaveInfoIndex, m_saveDataMountResult.mountPoint.data );

	const SSavegameInfo& info = m_saveInfos[ m_initializedLoadSaveInfoIndex ];

	saverToFinalize->Close();

	Int32 errorCode = SCE_OK;
	const Bool haveValidScreenshot = ( m_realScreenshotSize > 1024 ); // if it's too small then it's most likely a blackscreen

	SceSaveDataIcon icon;
	{
		Red::MemoryZero( &icon, sizeof( icon ) );
		icon.buf = haveValidScreenshot ? m_screenshotBuffer.GetData() : const_cast< Uint8* > ( GGame->GetGameSaver()->GetDefaultScreenshotData() ); // why, Sony, why?
		icon.bufSize = CGameSaver::SCREENSHOT_BUFFER_SIZE;
		icon.dataSize = haveValidScreenshot ? m_realScreenshotSize : GGame->GetGameSaver()->GetDefaultScreenshotDataSize(); 
	}

	ORBIS_SYS_CALL( sceSaveDataSaveIcon( &m_saveDataMountResult.mountPoint, &icon ) );

	m_initializedLoadSaveInfoIndex = -1;
	m_currentSaveOpProgress = SAVE_NotInitialized;

	SceSaveDataParam param;
	{
		Red::MemoryZero( &param, sizeof( param ) );
		BuildFullDisplayNameForSave( param.title, SCE_SAVE_DATA_TITLE_MAXSIZE, info );
		param.userParam = info.m_displayNameIndex;
	}

	errorCode = sceSaveDataSetParam( &m_saveDataMountResult.mountPoint, SCE_SAVE_DATA_PARAM_TYPE_ALL, &param, sizeof( param ) );
	if ( errorCode < SCE_OK )
	{
		RED_LOG( Save, TXT("SaveData set param error: %08lx"), errorCode );
	}

	// save required content
	// for maximum simplicity, we will require all content that was activated in the moment of saving 
	{
		m_requiredContentString.ClearFast();
		TDynArray< CName > requiredContent = GContentManager->GetActivatedContent();
		for ( auto name : requiredContent )
		{
			m_requiredContentString += name.AsAnsiChar();
			m_requiredContentString += ';';
		}

		String filePath = ANSI_TO_UNICODE( m_saveDataMountResult.mountPoint.data );
		filePath += L'/';
		filePath += info.GetFileName();
		filePath += info.GetRequiredContentExtension();

		GFileManager->SaveAnsiStringToFile( filePath, m_requiredContentString, false );
	}

	UnmountSaveData( &m_saveDataMountResult, false ); // unmount synchronously here - we're on a thread either way

	m_saveInfosUpdateState.SetValue( LIST_NeedsUpdate );

	QueueEvent( EUserEvent::UE_GameSaved );

	UpdateUserActionsLock( false );
}

void CUserProfileManagerOrbis::GetSaveFiles( TDynArray< SSavegameInfo >& files ) const 
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );

	// just copy the list, it is kept in sync all the time 
	files = m_saveInfos;
}

Bool CUserProfileManagerOrbis::GetLatestSaveFile( SSavegameInfo& info ) const 
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );

	Bool retVal = false;
	info.Clear();
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

void CUserProfileManagerOrbis::DeleteSaveGame( const SSavegameInfo& info )
{
	m_deleteQueue.Push( info );
}

void CUserProfileManagerOrbis::DoDeleteSaveData( SceSaveDataDirName* dirName )
{
	SceSaveDataDelete del;
	{
		Red::MemoryZero( &del, sizeof( del ) );
		del.userId = m_userId;
		del.dirName = dirName;
	}

	Int32 errorCode = sceSaveDataDelete( &del );
	if ( errorCode < SCE_OK )
	{
		HandleSaveDataErrorCode( errorCode, dirName, SCE_SAVE_DATA_DIALOG_TYPE_DELETE );
	}
}

String CUserProfileManagerOrbis::BuildFullDisplayNameForSave( const SSavegameInfo& info ) const 
{
	AnsiChar str[ 256 ];
	BuildFullDisplayNameForSave( str, 256, info );
	return ANSI_TO_UNICODE( str );
}

Bool CUserProfileManagerOrbis::AreSavesInitialized() const 
{
	return m_saveSystemInitialized && HasActiveUser();
}

Bool CUserProfileManagerOrbis::InitializeSaveSystem()
{
	ORBIS_SYS_CALL_RET( sceSaveDataInitialize( nullptr ) );	   // nullptr here means default initialization parameters, like default thread priority for the library, etc.
	ORBIS_SYS_CALL_RET( sceSysmoduleLoadModule( SCE_SYSMODULE_SAVE_DATA_DIALOG ) ); 
	ORBIS_SYS_CALL_RET( sceSaveDataDialogInitialize() );
	
	m_saveSystemInitialized = true;
	m_currentLoadOpProgress = LOAD_NotInitialized;
	m_currentSaveOpProgress = SAVE_NotInitialized;
	m_screenshotReadingRequestStatus.SetValue( SCR_NotRequested );
	m_saveLock = CGameSessionManager::GAMESAVELOCK_INVALID;

	m_mountTask = nullptr;
	m_unmountTask = nullptr;
	m_updateSaveInfosTask = nullptr;
	m_saveSettingsTask = nullptr;
	m_screenshotReadTask = nullptr;
	m_deleteSaveDataTask = nullptr;

	Red::MemoryZero( &m_saveDataMountResult, sizeof( m_saveDataMountResult ) );
	Red::MemoryZero( &m_userSettingsMountResult, sizeof( m_userSettingsMountResult ) );
	Red::MemoryZero( m_screenshotBuffer.GetData(), CGameSaver::SCREENSHOT_BUFFER_SIZE );

	m_saveBuffer.Reserve( c_saveGameCompressedDataBufferSize );

	#ifdef SAVE_SERVER_ENABLED
		m_debugSaveServer.Open();
	#endif

	return true;
}

Bool CUserProfileManagerOrbis::ShutdownSaveSystem()
{
	ASSERT( m_currentLoadOpProgress == LOAD_NotInitialized );
	ASSERT( SAVE_NotInitialized == m_currentSaveOpProgress );
	ASSERT( !IsDoingAsyncOp() );

	#ifdef SAVE_SERVER_ENABLED
		m_debugSaveServer.Close();
	#endif

	m_saveSystemInitialized = false;
	m_currentLoadOpProgress = LOAD_NotInitialized;
	m_currentSaveOpProgress = SAVE_NotInitialized;
	m_screenshotReadingRequestStatus.SetValue( SCR_NotRequested );
	if ( m_saveLock != CGameSessionManager::GAMESAVELOCK_INVALID )
	{
		SGameSessionManager::GetInstance().ReleaseNoSaveLock( m_saveLock );
		m_saveLock = CGameSessionManager::GAMESAVELOCK_INVALID;
	}

	m_saveBuffer.Clear();

	ORBIS_SYS_CALL_RET( sceSaveDataDialogTerminate() );
	ORBIS_SYS_CALL_RET( sceSysmoduleUnloadModule( SCE_SYSMODULE_SAVE_DATA_DIALOG ) ); 
	ORBIS_SYS_CALL_RET( sceSaveDataTerminate() );

	return true;
}

void CUserProfileManagerOrbis::UpdateSaveSystem()
{
	if ( m_displayingCorruptedSaveOverwriteMessage )
	{
		// do nothing, wait for the user to act
		return;
	}

	Int32 saveInfosState = m_saveInfosUpdateState.GetValue();
	if ( !IsDoingAsyncOp() && m_initializedLoadSaveInfoIndex < 0 )
	{
		switch ( saveInfosState )
		{
		case LIST_NeedsUpdate:
			if ( m_saveLock == CGameSessionManager::GAMESAVELOCK_INVALID )
			{
				SGameSessionManager::GetInstance().CreateNoSaveLock( TXT("save list update"), m_saveLock, true, false );
			}

			m_saveInfosUpdateState.SetValue( LIST_Updating );
			CreateUpdateSaveInfosTask();
			RunUpdateSaveInfosTask();
			break;
		case LIST_Updating:
			break;
		case LIST_Updated_EventPending:
			saveInfosState = LIST_Updated;
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

	if ( !IsDoingAsyncOp() && HasActiveUser() && saveInfosState == LIST_Updated && !IsSaveDataMounted() )
	{
		if ( !m_deleteQueue.Empty() )
		{
			SSavegameInfo info = m_deleteQueue.Front();
			
			// check if this operation is still valid
			Bool valid = false;
			for ( const auto& i : m_saveInfos )
			{
				if ( i.m_filename == info.m_filename )
				{
					valid = true;
					break;
				}
			}

			if ( valid )
			{
				CreateDeleteSaveDataTask();
				{
					m_deleteSaveDataTask->SetDirName( info.m_filename.AsChar() );
				}
				RunDeleteSaveDataTask();
			}

			m_deleteQueue.Pop();
		}
	}

	SceCommonDialogStatus status = sceSaveDataDialogUpdateStatus();
	if ( SCE_COMMON_DIALOG_STATUS_FINISHED == status )
	{
		m_displayingOutOfDiskSpaceError = false;
		m_displayingBrokenSaveDataError = false;
		m_displayingSaveDataErrorCode = false;
	}

	if ( m_currentLoadOpProgress == LOAD_Initializing )
	{
		UpdateGameLoadingInit();
	}

	if ( m_currentSaveOpProgress == SAVE_Initializing )
	{
		UpdateGameSavingInit();
	}

	if ( m_screenshotReadingRequestStatus.GetValue() == SCR_Requested && !IsDoingAsyncOp() )
	{
		Int32 listIndex = m_screenshotReadingRequestInfoIndex.GetValue();
		if ( listIndex >= 0 && listIndex < m_saveInfos.SizeInt() )
		{
			CreateScreenshotReadTask();
			{
				m_screenshotReadTask->SetSaveInfo( m_saveInfos[ listIndex ] );
			}
			RunScreenshotReadTask();
		}
	}

	if ( m_mountTask && m_mountTask->IsFinished() )						{ OnMountTaskFinished();			}
	if ( m_unmountTask && m_unmountTask->IsFinished() )					{ OnUnMountTaskFinished();			}
	if ( m_updateSaveInfosTask && m_updateSaveInfosTask->IsFinished() )	{ OnUpdateSaveInfosTaskFinished();	}
	if ( m_saveSettingsTask && m_saveSettingsTask->IsFinished() )		{ OnSaveSettingsTaskFinished();		}
	if ( m_screenshotReadTask && m_screenshotReadTask->IsFinished() )	{ OnScreenshotReadTaskFinished();	}
	if ( m_deleteSaveDataTask && m_deleteSaveDataTask->IsFinished() )	{ OnDeleteSaveDataTaskFinished();	}

	if ( !IsDoingAsyncOp() && HasActiveUser() )
	{
		if ( false == m_settingsToSave.Empty() )
		{
			UpdateSettingsSaving();

		}
		else // no setting to save
		{
			UpdateSettingsLoading();
		}
	}

	#ifdef SAVE_SERVER_ENABLED
		if ( !IsDoingAsyncOp() && ( SAVE_NotInitialized == m_currentSaveOpProgress ) && !m_displayingOutOfDiskSpaceError && !m_displayingBrokenSaveDataError
				&& !m_displayingSaveDataErrorCode )
		{
			m_debugSaveServer.Update();
		}
	#endif // SAVE_SERVER_ENABLED
}

void CUserProfileManagerOrbis::UpdateUserActionsLock( Bool lock )
{
	if ( lock )
	{
		m_lockedUserActions = true;
		ToggleInputProcessing( false, eAPDR_User );
		GGame->GetInputManager()->SuppressSendingEvents( true );
	}
	else if ( m_lockedUserActions )
	{
		m_lockedUserActions = false;
		ToggleInputProcessing( true, eAPDR_User );
		GGame->GetInputManager()->SuppressSendingEvents( false );
	}
}

void CUserProfileManagerOrbis::UpdateSaveInfos()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );

	SceSaveDataDirNameSearchCond cond;
	{
		Red::MemoryZero( &cond, sizeof( cond ) );
		cond.userId = m_userId;
		cond.key = SCE_SAVE_DATA_SORT_KEY_DIRNAME;
		cond.order = SCE_SAVE_DATA_SORT_ORDER_ASCENT;
	}

	const Uint32 NUM_DIR_NAMES = NUM_AUTOSAVE_SLOTS + NUM_QUICKMANUAL_SLOTS + NUM_USERCONFIG_SLOTS + NUM_CHECKPOINT_SLOTS;
	SceSaveDataDirName dirNames[ NUM_DIR_NAMES ];
	{
		Red::MemoryZero( &dirNames, sizeof( SceSaveDataDirName ) * NUM_DIR_NAMES );
	}

	SceSaveDataParam params[ NUM_DIR_NAMES ];
	{
		Red::MemoryZero( &params, sizeof( SceSaveDataParam ) * NUM_DIR_NAMES );
	}

	SceSaveDataDirNameSearchResult result;
	{
		Red::MemoryZero( &result, sizeof( result ) );
		result.dirNames = dirNames;
		result.dirNamesNum = NUM_DIR_NAMES;
		result.params = params;
	}

	Int32 errorCode;
	TIMER_BLOCK( dirSearch )
		errorCode = sceSaveDataDirNameSearch( &cond, &result );
	END_TIMER_BLOCK( dirSearch )

	if ( errorCode != SCE_OK ) 
	{
		m_saveInfosUpdateState.SetValue( LIST_Updated );
		RED_LOG( Save, TXT("SaveData dir search error: %08lx"), errorCode );
		HALT("TODO: add proper error handling in here!");
		return;
	}

	m_userSettingsExists = false;
	m_saveInfos.ClearFast();
	for ( Uint32 i = 0; i < result.hitNum; ++i )
	{
		if ( 0 == Red::StringCompare( result.dirNames[ i ].data, "user.settings" ) )
		{
			m_userSettingsExists = true;
			continue; // user setting file, just skip this
		}

		SSavegameInfo& info = m_saveInfos[ m_saveInfos.Grow( 1 ) ];
		info.Clear();
		info.m_filename = ANSI_TO_UNICODE( result.dirNames[ i ].data );	
		info.m_displayNameIndex = result.params[ i ].userParam;
		
		SceRtcDateTime orbisTime;
		ORBIS_SYS_CALL( sceRtcSetTime_t( &orbisTime, result.params[ i ].mtime ) );

		SceRtcTick orbisTimeTick;
		ORBIS_SYS_CALL( sceRtcGetTick( &orbisTime, &orbisTimeTick ) );

		SceRtcTick localTimeTick;
		ORBIS_SYS_CALL( sceRtcConvertUtcToLocalTime( &orbisTimeTick, &localTimeTick ) );

		SceRtcDateTime localTime;
		ORBIS_SYS_CALL( sceRtcSetTick( &localTime, &localTimeTick ) );

		info.m_timeStamp.Clear();
		info.m_timeStamp.SetYear			( Uint32( localTime.year ) );
		info.m_timeStamp.SetMonth			( Uint32( localTime.month ) - 1 );
		info.m_timeStamp.SetDay				( Uint32( localTime.day ) - 1 );
		info.m_timeStamp.SetHour			( Uint32( localTime.hour ) );
		info.m_timeStamp.SetMinute			( Uint32( localTime.minute ) );
		info.m_timeStamp.SetSecond			( Uint32( localTime.second ) );

		ParseFilename( info );	
	}

	m_saveInfosUpdateState.SetValue( LIST_Updated_EventPending );
}

Int16 CUserProfileManagerOrbis::GetNumSaveSlots( ESaveGameType type ) const
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

Bool CUserProfileManagerOrbis::GetSaveInSlot( ESaveGameType type, Int16 slot, SSavegameInfo& info ) const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_saveInfosMutex );

	Int32 index = FindSaveInfoIndex( type, slot );
	if ( index >= 0 )
	{
		info = m_saveInfos[ index ];
		return true;
	}

	return false;
}

Int32 CUserProfileManagerOrbis::FindSaveInfoIndex( ESaveGameType type, Int16 slotIndex ) const
{
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

void CUserProfileManagerOrbis::GenerateSaveFileName( SSavegameInfo& info ) 
{
	info.m_customFilename = false; // no custom filenames

	const Char* prefix = TXT("");
	switch ( info.m_slotType )
	{
	case SGT_AutoSave:
		prefix = TXT("AutoSave.");
		break;
	case SGT_QuickSave:
	case SGT_Manual:
		prefix = TXT("ManualSave.");
		break;
	case SGT_CheckPoint:
	case SGT_ForcedCheckPoint:
		prefix = TXT("CheckPoint.");
		break;
	}

	info.m_filename = String::Printf( TXT("%ls%d"),	prefix, info.m_slotIndex );
}

void CUserProfileManagerOrbis::ParseFilename( SSavegameInfo& info )
{
	ASSERT( false == info.m_filename.Empty() );

	// Filename format:
	// Type_Slot
	// Where: 
	// Type : AutoSave, QuickSave, or ManualSave
	// Slot : int
	
	CTokenizer tokenizer( info.m_filename, TXT(".") );
	if ( tokenizer.GetNumTokens() != 2 /*( GUseSaveHack ? 5 : 2 )*/ )
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
}


void CUserProfileManagerOrbis::BuildFullDisplayNameForSave( AnsiChar* out, Uint32 size, const SSavegameInfo& info ) const
{
	struct Hack
	{
		static void Arabic( String& str )
		{
			if ( SLocalizationManager::GetInstance().GetTextLocale().EqualsNC( TXT("AR") ) )
			{
				str.ReplaceAll( 0x06BE, 0xFEEB ); // unicode character 0x06BE is not supported by the font on ps4, while 0xFEEB is and looks as 0x06BE should 
			}
		}
	};

	String questName = info.GetDisplayName();
	Hack::Arabic( questName );

	Red::StringCopy( out, UNICODE_TO_ANSI( questName.AsChar() ), size );
	Uint32 len = Red::StringLength( out );

	out += len;
	size -= len;

	Red::StringCopy( out, " - ", size );
	len = Red::StringLength( out );

	out += len;
	size -= len;

	const size_t buf_szie = 16;
	AnsiChar dateTimeBuffer[ buf_szie ];
	{
		int32_t dateFormat;
		ORBIS_SYS_CALL( sceSystemServiceParamGetInt( SCE_SYSTEM_SERVICE_PARAM_ID_DATE_FORMAT, &dateFormat ) );

		const Uint32 year = info.m_timeStamp.GetYear();
		const Uint32 month = info.m_timeStamp.GetMonth() + 1;
		const Uint32 day = info.m_timeStamp.GetDay() + 1;

		switch ( dateFormat )
		{
		case SCE_SYSTEM_PARAM_DATE_FORMAT_YYYYMMDD: Red::SNPrintF( dateTimeBuffer, buf_szie, "%ld/%ld/%ld " , year, month, day ); break;
		case SCE_SYSTEM_PARAM_DATE_FORMAT_MMDDYYYY: Red::SNPrintF( dateTimeBuffer, buf_szie, "%ld/%ld/%ld " , month, day, year ); break;
		case SCE_SYSTEM_PARAM_DATE_FORMAT_DDMMYYYY: Red::SNPrintF( dateTimeBuffer, buf_szie, "%ld/%ld/%ld " , day, month, year ); break;

		default:
			break;
		}

		Red::StringCopy( out, dateTimeBuffer, size );
		len = Red::StringLength( out );

		out += len;
		size -= len;
	}

	{
		int32_t timeFormat;
		ORBIS_SYS_CALL( sceSystemServiceParamGetInt( SCE_SYSTEM_SERVICE_PARAM_ID_TIME_FORMAT, &timeFormat ) );

		Uint32 hour = info.m_timeStamp.GetHour();
		const Uint32 minute = info.m_timeStamp.GetMinute();
		const Uint32 second = info.m_timeStamp.GetSecond();
		const AnsiChar* suffix = "";

		if ( timeFormat == SCE_SYSTEM_PARAM_TIME_FORMAT_12HOUR ) 
		{											   
			suffix = ( hour < 12 ) ? " AM" : " PM";
			hour = hour % 12;
			hour = ( 0 == hour ) ? 12 : hour;  // noon or midnight, it's always "12" in 12-hour clock
		}
		
		Red::SNPrintF( dateTimeBuffer, buf_szie, "%ld:%02ld:%02ld%s", hour, minute, second, suffix ); 

		Red::StringCopy( out, dateTimeBuffer, size );
		len = Red::StringLength( out );

		out += len;
		size -= len;
	}
}

Bool CUserProfileManagerOrbis::IsDisplayingSystemMessage() const 
{
	return m_displayingBrokenSaveDataError || m_displayingOutOfDiskSpaceError || m_displayingSaveDataErrorCode;
}

Bool CUserProfileManagerOrbis::HaveLockedUserActions() const 
{
	return m_lockedUserActions;
}

Bool CUserProfileManagerOrbis::LoadUserSettings( StringAnsi& settingsString )
{
	if ( false == m_shouldMountSettingsForReading || false == AreSettingsMounted() )
	{
		RED_LOG( Profile, TXT("Settings are not available for reading yet. Wait for the event.") );
		return false;
	}

	if ( m_currentLoadOpProgress != LOAD_NotInitialized )
	{
		RED_LOG( Profile, TXT("Trying to load user settings while game is being loaded.") );
		return false;
	}

	if ( SAVE_NotInitialized != m_currentSaveOpProgress )
	{
		RED_LOG( Profile, TXT("Trying to load user settings while game is being saved.") );
		return false;
	}

	if ( !HasActiveUser() )
	{
		RED_LOG( Profile, TXT("Trying to load user settings while no user is signed in.") );
		return false;
	}

	if ( IsDoingAsyncOp() )
	{
		RED_LOG( Profile, TXT("Trying to load user settings while doing async op.") );
		return false;
	}

	TIMER_BLOCK( loadSettings )
		// ok, settings are already mounted at this point
		String filePath = ANSI_TO_UNICODE( m_userSettingsMountResult.mountPoint.data );
		filePath += TXT("/user.settings");

		IFile* file = GFileManager->CreateFileReader( filePath, FOF_AbsolutePath );
		if ( file == nullptr )
		{
			RED_LOG( Profile, TXT("Failed to read %ls"), filePath.AsChar() );
			CreateUnMountTask();
			{
				m_unmountTask->SetIsUserSettings( true );
			}
			RunUnMountTask();
			m_shouldMountSettingsForReading = false;
			return false;
		}

		settingsString.Resize( file->GetSize() + 1 );
		file->Serialize( ( void* ) settingsString.Data(), settingsString.GetLength() );
		settingsString[ settingsString.GetLength() ] = '\0';
		delete file;
	END_TIMER_BLOCK( loadSettings )

	// don't unmount
	return true;
}		 

Bool CUserProfileManagerOrbis::SaveUserSettings( const StringAnsi& settingsString )
{
	if( m_ignoreSaveUserSettingsRequest.GetValue() == true )
	{
		return false;
	}

	m_settingsToSave = settingsString;

	// actual saving will happen later ( see UpdateSettingsSaving() )	
	return true;
}


void CUserProfileManagerOrbis::DoSaveSettings()
{
	SceSaveDataDirName dirName;
	{
		Red::StringCopy( dirName.data, "user.settings", SCE_SAVE_DATA_DIRNAME_DATA_MAXSIZE );
	}

	Int32 errorCode = SCE_OK;
	SceSaveDataIcon icon;
	{
		Red::MemoryZero( &icon, sizeof( icon ) );
		icon.buf = ( void* ) CGameSaver::DEFAULT_SCREENSHOT_DATA;			// we use default screenshot here, as taking real screenshot here isn't the best idea
		icon.bufSize = CGameSaver::DEFAULT_SCREENSHOT_SIZE;
		icon.dataSize = CGameSaver::DEFAULT_SCREENSHOT_SIZE; 
	}

	errorCode = sceSaveDataSaveIcon( &m_userSettingsMountResult.mountPoint, &icon );
	if ( errorCode < SCE_OK ) 
	{
		HALT("couldn't write icon data");
	}

	String filePath = ANSI_TO_UNICODE( m_userSettingsMountResult.mountPoint.data );
	filePath += TXT("/user.settings");

	IFile* file = GFileManager->CreateFileWriter( filePath, FOF_AbsolutePath );
	file->Serialize( ( void* ) m_settingsToSave.Data(), m_settingsToSave.GetLength() );
	delete file;

	const String locKey = TXT("panel_mainmenu_usersettings");
	SceSaveDataParam param;
	{
		Red::MemoryZero( &param, sizeof( param ) );

		String settingsDisplayName = SLocalizationManager::GetInstance().GetStringByStringKey( locKey );
		Red::StringCopy( param.title, UNICODE_TO_ANSI( settingsDisplayName.AsChar() ), SCE_SAVE_DATA_TITLE_MAXSIZE );
	}

	if ( param.title[ 0 ] != '\0' )
	{
		errorCode = sceSaveDataSetParam( &m_userSettingsMountResult.mountPoint, SCE_SAVE_DATA_PARAM_TYPE_ALL, &param, sizeof( param ) );
		if ( errorCode < SCE_OK )
		{
			HandleSaveDataErrorCode( errorCode, &dirName, SCE_SAVE_DATA_DIALOG_TYPE_SAVE );
		}
	}
	else
	{
		RED_LOG( Profile, TXT("Missing localization key %ls, title for user.settings savedata not set."), locKey.AsChar() );
	}

	m_settingsToSave.Clear();
}


void CUserProfileManagerOrbis::UpdateSettingsSaving()
{
	TIMER_BLOCK( time )
		if ( Orbis::GStreamingInstaller )
		{
			Orbis::GStreamingInstaller->SuspendInstallerAsync(); // suspend now so I/O bandwidth available ASAP. It'll autoresume itself RequiresPriorityIO() false.
		}		

		if ( AreSettingsMounted() )
		{
			if ( m_shouldMountSettingsForReading )
			{
				m_shouldMountSettingsForReading = false;
				CreateUnMountTask();
				{
					m_unmountTask->SetIsUserSettings( true );
				}
				RunUnMountTask();
			}
			else // settings are mounted for writing
			{
				if ( false == IsSavingSettings() )
				{
					CreateSaveSettingsTask();
					RunSaveSettingsTask();
				}
				/* else do nothing, just wait for saving to finish */				
			}
		}
		else // settings are not mounted now
		{
			// don't start saving settings while we load a level
			if ( GGame->IsLoadingScreenShown() )
			{
				return;
			}

			CreateMountTask();
			{
				m_mountTask->SetIsUserSettings( true );
				m_mountTask->SetCanFail( true );
				m_mountTask->SetDirName( TXT("user.settings") );
				m_mountTask->SetBlocks( SCE_SAVE_DATA_BLOCKS_MIN );
				m_mountTask->SetMountMode( ( m_userSettingsExists ? 0 : SCE_SAVE_DATA_MOUNT_MODE_CREATE ) | SCE_SAVE_DATA_MOUNT_MODE_RDWR );
			}
			RunMountTask();
		}
	END_TIMER_BLOCK( time )
}

void CUserProfileManagerOrbis::UpdateSettingsLoading()
{
	if ( !AreSettingsMounted() && m_shouldMountSettingsForReading )
	{
		CreateMountTask();
		{
			m_mountTask->SetIsUserSettings( true );
			m_mountTask->SetCanFail( true );
			m_mountTask->SetDirName( TXT("user.settings") );
			m_mountTask->SetMountMode( SCE_SAVE_DATA_MOUNT_MODE_RDONLY );
		}
		RunMountTask();
	}

	if ( AreSettingsMounted() && !m_shouldMountSettingsForReading )
	{
		CreateUnMountTask();
		{
			m_unmountTask->SetIsUserSettings( true );
		}
		RunUnMountTask();
	}
}

Bool CUserProfileManagerOrbis::MountSaveData( SceSaveDataMount* mount, SceSaveDataMountResult* result, Bool canFail, Bool isUserSettings )
{
	Int32 errorCode;
	const Bool isCreate = ( 0 != ( mount->mountMode & SCE_SAVE_DATA_MOUNT_MODE_CREATE ) );
	TIMER_BLOCK( mountTime )
		errorCode = sceSaveDataMount( mount, result );
	END_TIMER_BLOCK( mountTime )
	RED_LOG( Save, TXT("SaveData mount for %ls, user: %x, mode: %ls, result: %08x, mountPoint: %s"), 
		isUserSettings ? TXT("user settings") : TXT("save"),
		m_userId,
		isCreate ? TXT("create") : ( ( mount->mountMode & SCE_SAVE_DATA_MOUNT_MODE_RDONLY ) ? TXT("read") : TXT("write") ),
		errorCode,
		result->mountPoint.data );
	if ( errorCode < SCE_OK )
	{
		if ( canFail ) // don't handle errors, failing is not an error in this case...
		{
			// ...except for this little HACK in here... :(
			// so... if there is no space for creating user.setting we're required to present the error message either way...
			if ( isUserSettings && isCreate )
			{
				HandleOutOfDiskSpaceError( errorCode, mount->dirName, m_userSettingsMountResult.requiredBlocks );
			}

			Red::MemoryZero( result, sizeof( SceSaveDataMountResult ) );
			return false; 
		}

		// ok, normal error handling with ( canFail == false )
		if (		HandleOutOfDiskSpaceError( errorCode, mount->dirName, m_saveDataMountResult.requiredBlocks )
				||	HandleBrokenSaveDataError( errorCode, mount->dirName )
				||	HandleSaveDataErrorCode( errorCode, mount->dirName, ( mount->mountMode & SCE_SAVE_DATA_MOUNT_MODE_RDONLY ) ? SCE_SAVE_DATA_DIALOG_TYPE_LOAD : SCE_SAVE_DATA_DIALOG_TYPE_SAVE )
			)
		{
			Red::MemoryZero( result, sizeof( SceSaveDataMountResult ) );
			return false;
		}

		Red::MemoryZero( result, sizeof( SceSaveDataMountResult ) );
		HALT("TODO: add proper error handling in here!"); // unhadled error code - shouldn't happen
		return false;															   
	}

	return true;
}

void CUserProfileManagerOrbis::UnmountSaveData( SceSaveDataMountResult* result, Bool isUserSettings )
{
	RED_LOG( Save, TXT("SaveData unmount for %ls, mountPoint: %s"), isUserSettings ? TXT("user settings") : TXT("save"), result->mountPoint.data );
	TIMER_BLOCK( umount )
		ORBIS_SYS_CALL( sceSaveDataUmount( &result->mountPoint ) );
	END_TIMER_BLOCK( umount )
	Red::MemoryZero( result, sizeof( SceSaveDataMountResult ) );
}

void CUserProfileManagerOrbis::CreateMountTask()
{
	ASSERT( !IsDoingAsyncOp() );
	m_mountTask = new ( CTask::Root ) CMountTask( this ); 
}

void CUserProfileManagerOrbis::CreateUnMountTask()
{
	ASSERT( !IsDoingAsyncOp() );
	m_unmountTask = new ( CTask::Root ) CUnMountTask( this ); 
}

void CUserProfileManagerOrbis::CreateUpdateSaveInfosTask()
{
	ASSERT( !IsDoingAsyncOp() );
	m_updateSaveInfosTask = new ( CTask::Root ) CUpdateSaveInfosTask( this ); 
}

void CUserProfileManagerOrbis::CreateSaveSettingsTask()
{
	ASSERT( !IsDoingAsyncOp() );
	m_saveSettingsTask = new ( CTask::Root ) CSaveSettingsTask( this ); 
}

void CUserProfileManagerOrbis::CreateScreenshotReadTask()
{
	ASSERT( !IsDoingAsyncOp() );
	m_screenshotReadTask = new ( CTask::Root ) CScreenshotReadTask( this ); 
}

void CUserProfileManagerOrbis::CreateDeleteSaveDataTask()
{
	ASSERT( !IsDoingAsyncOp() );
	m_deleteSaveDataTask = new ( CTask::Root ) CDeleteSaveDataTask( this ); 
}

void CUserProfileManagerOrbis::RunMountTask()
{
	GTaskManager->Issue( *m_mountTask, TSP_Critical, TSG_Service );
}

void CUserProfileManagerOrbis::RunUnMountTask()
{
	GTaskManager->Issue( *m_unmountTask, TSP_Critical, TSG_Service );
}

void CUserProfileManagerOrbis::RunUpdateSaveInfosTask()
{
	GTaskManager->Issue( *m_updateSaveInfosTask, TSP_Critical, TSG_Service );
}

void CUserProfileManagerOrbis::RunSaveSettingsTask()
{
	GTaskManager->Issue( *m_saveSettingsTask, TSP_Critical, TSG_Service );
}

void CUserProfileManagerOrbis::RunScreenshotReadTask()
{
	GTaskManager->Issue( *m_screenshotReadTask, TSP_Critical, TSG_Service );
}

void CUserProfileManagerOrbis::RunDeleteSaveDataTask()
{
	GTaskManager->Issue( *m_deleteSaveDataTask, TSP_Critical, TSG_Service );
}

void CUserProfileManagerOrbis::OnUserDialogCallback( Int32 messageId, Int32 actionId )
{
	// please see enum EUniqueMessageIDs in scripts
	// the values are hardcoded here because the enum stays in script for now...
	// IMO having an ENUM as ids in this case is a bad idea (should be a cname or string)
	// but for now i just hardcode the numbers here...
	enum 
	{ 
		UMID_UserSettingsCorrupted = 777, 
		UMID_CorruptedSaveDataOverwrite = 778, 
	};

	if ( messageId == UMID_UserSettingsCorrupted )
	{
		ASSERT( false == m_loadSaveReadyEventSent );
		DoDeleteSettings();
		m_loadSaveReadyEventSent = true;
		QueueEvent( EUserEvent::UE_LoadSaveReady );	
	}
	else if ( messageId == UMID_CorruptedSaveDataOverwrite )
	{
        RED_LOG( Save, TXT("Deleting corrupted save data in order to create new - user message dismissed") );
		m_displayingCorruptedSaveOverwriteMessage = false;

		if ( 0 == actionId ) // UMA_Ok == 0
		{
			const SSavegameInfo& info = m_saveInfos[ m_initializedLoadSaveInfoIndex ];
			
			if ( info.m_slotType == SGT_Manual || info.m_slotType == SGT_QuickSave ) // types triggered by user (no silent fail)
			{
				UpdateUserActionsLock( true );
			}

			CreateDeleteSaveDataTask();
			{
				m_deleteSaveDataTask->SetDirName( info.m_filename.AsChar() ); 
			}
			RunDeleteSaveDataTask();
		}
		else
		{
			m_currentSaveOpProgress = SAVE_Error;
		}
	}
}

void CUserProfileManagerOrbis::OnMountTaskFinished()
{
	if ( m_mountTask->IsUserSettings() )
	{
		if ( m_mountTask->GetMountMode() == SCE_SAVE_DATA_MOUNT_MODE_RDONLY && m_mountTask->GetResult() && false == m_loadSaveReadyEventSent )	 // succesful mount for reading for the first time
		{
			m_loadSaveReadyEventSent = true;

			// this is to let the game know it can load user settings now without paying cost of mounting the savedata
			QueueEvent( EUserEvent::UE_LoadSaveReady );	
		}
		else if ( m_mountTask->GetMountMode() == SCE_SAVE_DATA_MOUNT_MODE_RDONLY && false == m_mountTask->GetResult() ) // unsuccesful mount for reading
		{
			// don't try to mount again
			m_shouldMountSettingsForReading = false;

			// so we tried to mount user.settings for reading, but failed for some reason (corrupted data, fingerprint mismatch or sth)
			// this means we're unable to access settings storage, but we HAVE TO be able to do that!
			// so we handle this by deleting invalid user.settings to let the user create new one
			// but first we need to inform the user. Actual delete happens in OnUserDialogCallback()
			CallFunction( GCommonGame->GetGuiManager(), CNAME( OnUserSettingsCorrupted ) );
		}
		else if ( false == m_mountTask->GetResult() ) // mount for write failed
		{
			m_settingsToSave.Clear();
		}
		else if ( m_mountTask->GetMountMode() & SCE_SAVE_DATA_MOUNT_MODE_CREATE && m_mountTask->GetResult() )
		{
			m_userSettingsExists = true;
		}
	}

	m_mountTask->Release();
	m_mountTask = nullptr;
}

void CUserProfileManagerOrbis::OnUnMountTaskFinished()
{
	m_unmountTask->Release();
	m_unmountTask = nullptr;
}

void CUserProfileManagerOrbis::OnUpdateSaveInfosTaskFinished()
{
	m_updateSaveInfosTask->Release();
	m_updateSaveInfosTask = nullptr;
}

void CUserProfileManagerOrbis::OnSaveSettingsTaskFinished()
{
	m_saveSettingsTask->Release();
	m_saveSettingsTask = nullptr;

	CreateUnMountTask();
	{
		m_unmountTask->SetIsUserSettings( true );
	}
	RunUnMountTask();
	m_shouldMountSettingsForReading = true;
}

void CUserProfileManagerOrbis::OnScreenshotReadTaskFinished()
{
	m_screenshotReadTask->Release();
	m_screenshotReadTask = nullptr;
	// m_screenshotRequestStatus is set by the task itself (failed or ready)
}

void CUserProfileManagerOrbis::OnDeleteSaveDataTaskFinished()
{
	m_deleteSaveDataTask->Release();
	m_deleteSaveDataTask = nullptr;

	m_saveInfosUpdateState.SetValue( LIST_NeedsUpdate );
}

Bool CUserProfileManagerOrbis::HandleOutOfDiskSpaceError( Int32 errorCode, const SceSaveDataDirName* dirName, Uint64 requiredBlocks )
{
	if ( errorCode != SCE_SAVE_DATA_ERROR_NO_SPACE && errorCode != SCE_SAVE_DATA_ERROR_NO_SPACE_FS )
	{
		return false; // not handled
	}  

	if ( m_displayingOutOfDiskSpaceError )
	{
		return true; // already handled 
	}

	RED_LOG( Save, TXT("Out of disk space!") );

	SceSaveDataDialogParam param;
	{
		InitCommonDialogParams( param, dirName );
		param.mode = SCE_SAVE_DATA_DIALOG_MODE_SYSTEM_MSG;
		param.dispType = SCE_SAVE_DATA_DIALOG_TYPE_SAVE;
	}

	SceSaveDataDialogSystemMessageParam sysMessageParam;
	{
		Red::MemoryZero( &sysMessageParam, sizeof( sysMessageParam ) );
		sysMessageParam.sysMsgType = SCE_SAVE_DATA_DIALOG_SYSMSG_TYPE_NOSPACE_CONTINUABLE;
		sysMessageParam.value = requiredBlocks;
	}

	param.sysMsgParam = &sysMessageParam;
	Int32 ret = sceSaveDataDialogOpen( &param );
	if ( ret < SCE_OK )
	{
		HALT("sceSaveDataDialogOpen() error");
		return false;
	}

	m_displayingOutOfDiskSpaceError = true;
	return true;
}

Bool CUserProfileManagerOrbis::HandleBrokenSaveDataError( Int32 errorCode, const SceSaveDataDirName* dirName )
{
	if ( errorCode != SCE_SAVE_DATA_ERROR_BROKEN )
	{
		return false; // not handled
	}  

	if ( m_displayingBrokenSaveDataError )
	{
		return true; // already handled 
	}

	RED_LOG( Save, TXT("Broken save data!") );

	SceSaveDataDialogParam param;
	{
		InitCommonDialogParams( param, dirName );
		param.mode = SCE_SAVE_DATA_DIALOG_MODE_SYSTEM_MSG;
		param.dispType = SCE_SAVE_DATA_DIALOG_TYPE_LOAD;
	}


	SceSaveDataDialogSystemMessageParam sysMessageParam;
	{
		Red::MemoryZero( &sysMessageParam, sizeof( sysMessageParam ) );
		sysMessageParam.sysMsgType = SCE_SAVE_DATA_DIALOG_SYSMSG_TYPE_FILE_CORRUPTED;
	}

	param.sysMsgParam = &sysMessageParam;
	Int32 ret = sceSaveDataDialogOpen( &param );
	if ( ret < SCE_OK )
	{
		HALT("sceSaveDataDialogOpen() error");
		return false;
	}

	m_displayingBrokenSaveDataError = true;
	return true;
}

Bool CUserProfileManagerOrbis::HandleSaveDataErrorCode( Int32 errorCode, const SceSaveDataDirName* dirName, SceSaveDataDialogType type )
{
	if ( errorCode == SCE_SAVE_DATA_ERROR_NOT_FOUND )
	{
		// this is allowed when saving or deleting
		if ( type == SCE_SAVE_DATA_DIALOG_TYPE_SAVE || type == SCE_SAVE_DATA_DIALOG_TYPE_DELETE )
		{
			return true; // consider it handled in this case
		}
	}

	if ( m_displayingSaveDataErrorCode )
	{
		return true; // already handled 
	}

	SceSaveDataDialogParam param;
	{
		InitCommonDialogParams( param, dirName );
		param.mode = SCE_SAVE_DATA_DIALOG_MODE_ERROR_CODE;
		param.dispType = type;
	}

	SceSaveDataDialogErrorCodeParam eparam;
	{
		Red::MemoryZero( &eparam, sizeof( eparam ) );
		eparam.errorCode = errorCode;
	}

	param.errorCodeParam = &eparam;
	Int32 ret = sceSaveDataDialogOpen( &param );
	if ( ret < SCE_OK )
	{
		HALT("sceSaveDataDialogOpen() error");
		return false;
	}

	m_displayingSaveDataErrorCode = true;
	return true;
}

void CUserProfileManagerOrbis::InitCommonDialogParams( SceSaveDataDialogParam &param, const SceSaveDataDirName* dirName )
{
	sceSaveDataDialogParamInitialize( &param );

	static SceSaveDataDialogAnimationParam animParam;
	{
		Red::MemoryZero( &animParam, sizeof( animParam ) );
		animParam.userOK = SCE_SAVE_DATA_DIALOG_ANIMATION_ON;
		animParam.userCancel = SCE_SAVE_DATA_DIALOG_ANIMATION_ON;
		param.animParam = &animParam;
	}

	static SceSaveDataDialogItems items;
	{
		Red::MemoryZero( &items, sizeof( items ) );
		items.userId = m_userId;
		items.titleId = nullptr;
		items.dirName = dirName;
		items.dirNameNum = 1;
		items.newItem = nullptr;
		param.items = &items;
	}
}

void CUserProfileManagerOrbis::DoDeleteSettings()
{
	SceSaveDataDirName dirName;
	{
		Red::StringCopy( dirName.data, "user.settings", SCE_SAVE_DATA_DIRNAME_DATA_MAXSIZE );
	}
	
	SceSaveDataDelete del;
	{
		Red::MemoryZero( &del, sizeof( del ) );
		del.userId = m_userId;
		del.dirName = &dirName;
	}
	
	const Int32 errCode = sceSaveDataDelete( &del );
	if ( errCode == SCE_OK )
	{
		m_userSettingsExists = false;
	}
}






