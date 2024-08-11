/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "userProfileManagerWindowsSaving.h"

#include "../../common/core/tokenizer.h"
#include "../../common/core/fileProxy.h"
#include "../../common/core/chunkedLZ4File.h"
#include "../../common/core/diskInstrumentationPC.h"

#include "../../common/engine/compressedFile.h"
#include "../../common/engine/gameSaveManager.h"
#include "../../common/engine/localizationManager.h"

#include "../../common/game/gameSaver.h"
#include "../../common/game/commonGame.h"
#include "../../common/game/guiManager.h"

namespace
{
	Bool MakeSureSaveDirectoryExists_AndGetTheAbsolutePath( const Char* dir, String& outDirPath )
	{
		String userDir = GFileManager->GetUserDirectory();
		outDirPath = userDir + String( dir ) + TXT( "\\" );
		return GSystemIO.CreateDirectory( outDirPath.AsChar() );
	}

	const Uint64 MINIMUM_REQUIRED_SPACE_FOR_SAVEGAME = 0x1000000ull;		// 16 MB is minimum amount of free space on disk to have new game save
	const Uint64 MINIMUM_REQUIRED_SPACE_FOR_SAVESETTINGS = 0x100000ull;		// 1 MB is minimum amount of free space on disk to have new settings save
}

const Uint32 c_saveGameCompressedChunkSize = 1024 * 1024;
const Uint32 c_saveGameMaxCompressedChunks = 256;
const Char c_temporaryFileExtension[] = TXT( ".tmp" );		// We write the file with temp extension, then rename it once we have finished

CUserProfileManagerWindowsSaving::CUserProfileManagerWindowsSaving()
	: m_searchPattern( TXT("*.sav") )
	, m_saveListUpdated_EventPending( false )
	, m_screenshotBuffer( RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_BufferBitmap, CGameSaver::SCREENSHOT_BUFFER_SIZE, 16 ) )
	, m_screenshotBufferSize( CGameSaver::SCREENSHOT_BUFFER_SIZE )
	, m_lastTakenSceenshotSize( 0 )
	, m_writerCreated( false )
	, m_isSaving( false )
{
	// Get the save game directory
	VERIFY( MakeSureSaveDirectoryExists_AndGetTheAbsolutePath( TXT("gamesaves"), m_savesDir ) );
	LOG_ENGINE( TXT("Save directory set to '%ls'"), m_savesDir.AsChar() );
	m_currentSaveLoadOp.Clear();
}

CUserProfileManagerWindowsSaving::~CUserProfileManagerWindowsSaving()
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_BufferBitmap, m_screenshotBuffer );
}

IFile* CUserProfileManagerWindowsSaving::CreateSaveFileReader(const Bool rawFile)
{
	IFile* saveFile = NULL;

	// Open local file
	String absoluteSaveFilePath = BuildFullSaveFilePathForExistingFile( m_currentSaveLoadOp );
	IFile* fileReaderRaw = GFileManager->CreateFileReader( absoluteSaveFilePath, FOF_AbsolutePath | FOF_Buffered );
	if( !fileReaderRaw )
	{
		RED_FATAL_ASSERT( false, "Failed to open save file for reading" );
		return nullptr;
	}

	// Use the raw file
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
		return new CCompressedFileReader( absoluteSaveFilePath );
	}
}

IFile* CUserProfileManagerWindowsSaving::CreateScreenshotDataReader()
{
	ASSERT( m_requestedScreenshot.IsValid() );
	IFile* retVal = m_requestedScreenshot.IsValid() ? GFileManager->CreateFileReader( m_savesDir + m_requestedScreenshot.GetFileName() + m_requestedScreenshot.GetScreenshotExtension(), FOF_AbsolutePath | FOF_Buffered ) : nullptr;
	if ( nullptr == retVal )
	{
		// in case of CreateFileReader() failing for any reason or in case of creating reader without requesting, just read the default icon
		retVal = new CMemoryFileReaderExternalBuffer( CGameSaver::DEFAULT_SCREENSHOT_DATA, CGameSaver::DEFAULT_SCREENSHOT_SIZE );
	}

	return retVal;
}

void CUserProfileManagerWindowsSaving::RequestScreenshotDataForReading( const SSavegameInfo& info ) 
{
	// TODO: consider loading it to memory at this point on async job
	m_requestedScreenshot = info;
}

Bool CUserProfileManagerWindowsSaving::IsScreenshotDataReadyForReading()
{
	return m_requestedScreenshot.IsValid();
}

void CUserProfileManagerWindowsSaving::DoneReadingScreenshotData()
{
	m_requestedScreenshot.Clear();
}

void CUserProfileManagerWindowsSaving::Update()
{
	if ( m_saveListUpdated_EventPending )
	{
		m_saveListUpdated_EventPending = false;
		GGame->CallEvent( CNAME( OnGameSaveListUpdated ) );
	}

	__super::Update();
}

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
extern Bool isAnselTurningOn;
extern Bool isAnselTurningOff;
#endif // USE_ANSEL

void CUserProfileManagerWindowsSaving::FinalizeGameSaving( IGameSaver* gameSaver )
{
	// flush the save data
	gameSaver->Close();

#ifdef USE_ANSEL
	if ( !isAnselTurningOn && !isAnselTurningOff && !isAnselSessionActive )
#endif // USE_ANSEL
	{
		// ... and write the screenshot
		void* bufferToWrite = ( void* ) GGame->GetGameSaver()->GetDefaultScreenshotData();
		Uint32 sizeToWrite = GGame->GetGameSaver()->GetDefaultScreenshotDataSize();
		if ( m_lastTakenSceenshotSize >= 1024 && m_lastTakenSceenshotSize <= m_screenshotBufferSize )
		{
			bufferToWrite = m_screenshotBuffer;
			sizeToWrite = m_lastTakenSceenshotSize;
		}

		IFile* file = GFileManager->CreateFileWriter( m_savesDir + m_currentSaveLoadOp.GetFileName() + m_currentSaveLoadOp.GetScreenshotExtension(), FOF_AbsolutePath );
		if ( file )
		{
			file->Serialize( bufferToWrite, sizeToWrite );
			delete file;
		}
	}

	// At this point we are safe to rename the save-game with the correct extension
	Bool renameSuccess = GFileManager->MoveFile( m_savesDir + m_currentSaveLoadOp.GetFileName() + c_temporaryFileExtension, 
		m_savesDir + m_currentSaveLoadOp.GetFileName() + m_currentSaveLoadOp.GetSaveExtension() );

	QueueEvent( renameSuccess ? EUserEvent::UE_GameSaved : EUserEvent::UE_GameSaveFailed );
	m_currentSaveLoadOp.Clear();
	m_saveListUpdated_EventPending = true;
	m_isSaving.SetValue( false );
	m_writerCreated.SetValue( false );
}

void CUserProfileManagerWindowsSaving::DeleteSaveGame( const SSavegameInfo& info )
{
	// Format save name
	String saveFile( info.GetFileName() + info.GetSaveExtension() );
	String fullScreenshotFileName( info.GetFileName() + info.GetScreenshotExtension() );
	DeleteFile( saveFile, fullScreenshotFileName );
	m_saveListUpdated_EventPending = true;
}

void* CUserProfileManagerWindowsSaving::GetScreenshotBuffer()
{
	return m_screenshotBuffer;
}

void CUserProfileManagerWindowsSaving::OnScreenshotDone( Uint32 realSize )
{
	m_lastTakenSceenshotSize = realSize;
}

void CUserProfileManagerWindowsSaving::GetSaveFiles( TDynArray< SSavegameInfo >& files ) const
{
	TIMER_BLOCK( time )

	TDynArray< String > allSaveFiles;

	// List all .sav files 
	TIMER_BLOCK( dirListing )
		GFileManager->FindFiles( m_savesDir, m_searchPattern, allSaveFiles, false );
	END_TIMER_BLOCK( dirListing )

	TIMER_BLOCK( genInfos )
		// Generate saveinfos and count autosaves btw
		files.Resize( allSaveFiles.Size() );
		for ( Uint32 i=0; i<allSaveFiles.Size(); i++ )
		{
			CFilePath filePath( allSaveFiles[ i ] );
			files[ i ] = GetSavegameInfo( filePath.GetFileName() );

		}
	END_TIMER_BLOCK( genInfos )

	TIMER_BLOCK( sort )
	// sort by date ( newest first )
	::Sort( files.Begin(), files.End(), SSavegameInfo::ComparePredicate() );
	END_TIMER_BLOCK( sort )

	// assign slot indices
	AssignSlotIndices( files );

	END_TIMER_BLOCK( time )
}


Bool CUserProfileManagerWindowsSaving::GetLatestSaveFile( SSavegameInfo& info ) const
{
	// quick impl - can be optimized
	TDynArray< SSavegameInfo > allSaveFiles;
	GetSaveFiles( allSaveFiles );

	Red::System::DateTime thisTime, bestTime; // zero-time
	Int32 bestFile = -1;

	for ( Int32 i = 0; i < allSaveFiles.SizeInt(); ++i )
	{
		thisTime = allSaveFiles[ i ].m_timeStamp;
		if ( thisTime > bestTime )
		{
			bestTime = thisTime;
			bestFile = i;
		}
	}

	const Bool succ = bestFile >= 0;
	if ( succ )
	{
		info = allSaveFiles[ bestFile ];
	}

	return succ;
}


Bool CUserProfileManagerWindowsSaving::FileExist( const String& fileName )
{
	return GFileManager->FileExist( m_savesDir + fileName );
}

Red::System::DateTime CUserProfileManagerWindowsSaving::GetFileTime( const String& fileName, const String& extension ) const
{
	return GFileManager->GetFileTime( m_savesDir + fileName + extension );
}

#ifndef NO_SAVE_IMPORT
Bool CUserProfileManagerWindowsSaving::ImportSave( const SSavegameInfo& info )
{
	Bool retVal = false;
	if ( m_saveImporterFunc )
	{
		IObsoleteSaveImporter* importer = ( *m_saveImporterFunc )();
		retVal = importer->ImportSave( info );
		delete importer;
	}
	return retVal;
}

void CUserProfileManagerWindowsSaving::Import_GetSaveFiles( TDynArray< SSavegameInfo >& files )
{
	if ( m_saveImporterFunc )
	{
		IObsoleteSaveImporter* importer = ( *m_saveImporterFunc )();
		importer->GetSaveFiles( files );
		delete importer;
	}
}

void CUserProfileManagerWindowsSaving::Import_GetSearchPaths( TDynArray< String >& outArray )
{
	if ( m_saveImporterFunc )
	{
		IObsoleteSaveImporter* importer = ( *m_saveImporterFunc )();
		outArray = importer->GetSearchPaths();
		delete importer;
	}
}
#endif // ifndef NO_SAVE_IMPORT

void CUserProfileManagerWindowsSaving::DeleteFile( const String& gameSaveFilePath, const String& screenshotFilePath )
{
	// Delete local file														  
	if ( false == GFileManager->DeleteFile( m_savesDir + gameSaveFilePath ) )
	{
		RED_LOG( Save, TXT("Can't delete save file: %ls"), ( m_savesDir + gameSaveFilePath ).AsChar() );
	}

	if ( false == GFileManager->DeleteFile( m_savesDir + screenshotFilePath ) )
	{
		RED_LOG( Save, TXT("Can't delete screenshot file: %ls"), ( m_savesDir + gameSaveFilePath ).AsChar() );
	}
}

SSavegameInfo CUserProfileManagerWindowsSaving::GetSavegameInfo( const String& fileName ) const
{
	SSavegameInfo info;
	info.m_filename = fileName;
	info.m_timeStamp = GetFileTime( fileName, SSavegameInfo::GetSaveExtension() );
	ParseFilename( info );

	return info;
}

String CUserProfileManagerWindowsSaving::BuildFullSaveFilePathForExistingFile( const SSavegameInfo& info )
{
#ifndef NO_SAVE_IMPORT
	if ( info.IsW2Import() )
	{
		TDynArray< String > searchPaths;
		Import_GetSearchPaths( searchPaths );
		
		for ( const auto& path : searchPaths )
		{
			String abs = path + info.GetFileName() + info.GetSaveExtension();
			if ( GFileManager->FileExist( abs ) )
			{
				return abs;
			}
		}

		return String::EMPTY;
	}
	else
	{
		return GetSavesDir() + info.GetFileName() + info.GetSaveExtension();
	}
#else
	return GetSavesDir() + info.GetFileName() + info.GetSaveExtension();
#endif
}

IFileEx* CUserProfileManagerWindowsSaving::CreateSaveFileWriter()
{
	// Platform-specific file
	IFile* platformFile = GFileManager->CreateFileWriter( m_savesDir + m_currentSaveLoadOp.GetFileName() + c_temporaryFileExtension, FOF_AbsolutePath );
	RED_FATAL_ASSERT( platformFile, "Failed to open save game file for writing" );
	if( !platformFile )
	{
		return nullptr;
	}

	Uint32 magicHeader = SAVE_NEW_FORMAT_HEADER;
	*platformFile << magicHeader;

	// Wrap it in compression
	if( platformFile )
	{
		m_writerCreated.SetValue( true );
		return new CChunkedLZ4FileWriter( c_saveGameCompressedChunkSize, c_saveGameMaxCompressedChunks, platformFile );
	}

	return nullptr;
}

ELoadGameResult CUserProfileManagerWindowsSaving::InitGameLoading( const SSavegameInfo& info )
{
	m_requestedScreenshot = m_currentSaveLoadOp = info;
	return LOAD_ReadyToLoad; // TODO: check if the file exists, check content availablility
}

ESaveGameResult CUserProfileManagerWindowsSaving::InitGameSaving( SSavegameInfo& info )
{
	// Check if there is enough space on disk
	HardwareInstrumentation::CDiskInstrumentationPC diskInstrumentation;
	diskInstrumentation.GatherInformation( m_savesDir );

	if( diskInstrumentation.GetAvailableFreeSpace() < MINIMUM_REQUIRED_SPACE_FOR_SAVEGAME )
	{
		WARN_ENGINE( TXT("Can't save game - disk is out of space.") );
		CallFunction( GCommonGame->GetGuiManager(), CNAME( DisplayGameSaveErrorOutOfDiskSpace ) );
		return SAVE_Error;
	}

	Int32 overwriteIndex = -1;

	TDynArray< SSavegameInfo > files;
	GetSaveFiles( files );

	// auto-assign
	if ( info.m_slotIndex < 0 )
	{
		if ( info.IsAutoSave() )
		{
			Int32 lastAutoSlot = -1, lastAutoIndex = -1;
			for ( Int32 i = 0; i < files.SizeInt(); ++i )
			{
				const SSavegameInfo& file = files[ i ];
				if ( file.IsAutoSave() && lastAutoSlot < file.m_slotIndex )
				{
					lastAutoSlot = file.m_slotIndex;
					lastAutoIndex = i;
				}
			}

			// new slot if there are free slots left, overwrite last one otherwise
			if ( lastAutoSlot >= ( NUM_AUTOSAVE_SLOTS - 1 ) )
			{
				overwriteIndex = lastAutoIndex;
			}
		}
		else if ( info.IsCheckPoint() )
		{
			Int32 lastCPSlot = -1, lastCPIndex = -1;
			for ( Int32 i = 0; i < files.SizeInt(); ++i )
			{
				const SSavegameInfo& file = files[ i ];
				if ( file.IsCheckPoint() && lastCPSlot < file.m_slotIndex )
				{
					lastCPSlot = file.m_slotIndex;
					lastCPIndex = i;
				}
			}

			// new slot if there are free slots left, overwrite last one otherwise
			if ( lastCPSlot >= ( NUM_CHECKPOINT_SLOTS - 1 ) )
			{
				overwriteIndex = lastCPIndex;
			}
		}
		else if( info.IsQuickSave() )
		{
			Int32 lastQuickIndex = -1;
			for ( Int32 i = 0; i < files.SizeInt(); ++i )
			{
				const SSavegameInfo& file = files[ i ];
				if ( file.IsQuickSave() )
				{
					lastQuickIndex = i;
					break;
				}
			}

			if( lastQuickIndex >= ( NUM_QUICKSAVE_SLOTS - 1 ) )
			{
				overwriteIndex = lastQuickIndex;
			}
		}
	}
	else if ( info.IsManualSave() ) // overwrite manual save
	{
		for ( Int32 i = 0; i < files.SizeInt(); ++i )
		{
			if ( !files[ i ].IsManualSave() )
			{
				continue;
			}

			if ( files[ i ].m_slotIndex == info.m_slotIndex )
			{
				overwriteIndex = i;
				break;
			}
		}
	}
	else
	{
		RED_FATAL_ASSERT( false, "m_slotIndex different then -1 may be specified only for Manual Saves." );
	}

	if ( overwriteIndex >= 0 )
	{		
		String nameOfTheFileToOverwrite = files[ overwriteIndex ].GetFileName();
		DeleteFile( nameOfTheFileToOverwrite + info.GetSaveExtension(), nameOfTheFileToOverwrite + info.GetScreenshotExtension() );
	}

	GenerateSaveFileName( info );
	m_currentSaveLoadOp = info;
	m_isSaving.SetValue( true );

	return SAVE_ReadyToSave;
}

void CUserProfileManagerWindowsSaving::FinalizeGameLoading()
{
	m_currentSaveLoadOp.Clear();
}


ESaveGameResult CUserProfileManagerWindowsSaving::GetSaveGameProgress() const 
{
	if ( m_isSaving.GetValue() )
	{
		return m_writerCreated.GetValue() ? SAVE_Saving : SAVE_ReadyToSave;
	}

	return SAVE_NotInitialized;
}

ELoadGameResult CUserProfileManagerWindowsSaving::GetLoadGameProgress() const 
{
	return m_currentSaveLoadOp.IsValid() ? LOAD_ReadyToLoad : LOAD_NotInitialized;
}

void CUserProfileManagerWindowsSaving::CancelGameLoading()
{
	m_currentSaveLoadOp.Clear();
}

void CUserProfileManagerWindowsSaving::CancelGameSaving()
{
	m_currentSaveLoadOp.Clear();
	m_writerCreated.SetValue( false );
	m_isSaving.SetValue( false );
}

void CUserProfileManagerWindowsSaving::GenerateSaveFileName( SSavegameInfo& info ) 
{
	if ( info.m_customFilename )
	{
		return;
	}

	const Char* prefix = TXT("");
	switch ( info.m_slotType )
	{
	case SGT_AutoSave:
		prefix = TXT("AutoSave");
		break;
	case SGT_QuickSave:
		prefix = TXT("QuickSave");
		break;
	case SGT_Manual:
		prefix = TXT("ManualSave");
		break;
	case SGT_CheckPoint:
	case SGT_ForcedCheckPoint:
		prefix = TXT("CheckPoint");
		break;
	}

	info.m_filename = String::Printf( TXT("%ls_%lx_%lx_%lx"),
		prefix, info.m_displayNameIndex, info.m_timeStamp.GetDateRaw(), info.m_timeStamp.GetTimeRaw() );
}

void CUserProfileManagerWindowsSaving::ParseFilename( SSavegameInfo& info )
{
	ASSERT( false == info.m_filename.Empty() );

	if ( info.m_customFilename )
	{
		info.m_slotIndex = 0;
		info.m_slotType = SGT_Manual;
		return;
	}

	// Filename format:
	// Type_DisplayNameIndex_Date_Time
	// Where: 
	// Type : AutoSave, QuickSave, or ManualSave
	// DisplayNameIndex: string
	// Date: raw hex uint
	// Time: raw hex uint

	CTokenizer tokenizer( info.m_filename, TXT("_") );
	if ( tokenizer.GetNumTokens() != 4 )
	{
		//RED_LOG( Save, TXT("Cannot parse save filename: %s"), info.m_filename.AsChar() );
		info.m_customFilename = true;
		info.m_slotType = SGT_Manual;
		return;
	}

	String firstToken = tokenizer.GetToken( 0 );
	if ( firstToken.EqualsNC( TXT("AutoSave") ) )
	{
		info.m_slotType = SGT_AutoSave;
	}
	else if ( firstToken.EqualsNC( TXT("QuickSave") ) )
	{
		info.m_slotType = SGT_QuickSave;
	}
	else if ( firstToken.EqualsNC(  TXT("ManualSave") ) )
	{
		info.m_slotType = SGT_Manual;
	}
	else if ( firstToken.EqualsNC( TXT("CheckPoint") ) )
	{
		info.m_slotType = SGT_CheckPoint;
	}
	else
	{
		info.m_slotType = SGT_Manual;
		info.m_customFilename = true;
		//RED_LOG( Save, TXT("Cannot parse save filename: %s"), info.m_filename.AsChar() );
		return;
	}

#define SSGI_VERIFY_PARSE_STEP( x ) if ( false == x ) { info.m_customFilename = true; info.m_slotType = SGT_Manual; RED_LOG( Save, TXT("Cannot parse save filename: %s"), info.m_filename.AsChar() ); return; }

	String secondToken = tokenizer.GetToken( 1 );
	Uint32 unumber;
	Char *end;
	SSGI_VERIFY_PARSE_STEP( Red::StringToInt( unumber, secondToken.AsChar(), &end, Red::System::Base::BaseSixteen ) )
		info.m_displayNameIndex = unumber;

	String thirdToken = tokenizer.GetToken( 2 );
	SSGI_VERIFY_PARSE_STEP( Red::StringToInt( unumber, thirdToken.AsChar(), &end, Red::System::Base::BaseSixteen ) )
		info.m_timeStamp.SetDateRaw( unumber );

	String fourthToken = tokenizer.GetToken( 3 );
	SSGI_VERIFY_PARSE_STEP( Red::StringToInt( unumber, fourthToken.AsChar(), &end, Red::System::Base::BaseSixteen ) )
		info.m_timeStamp.SetTimeRaw( unumber );
}

Bool CUserProfileManagerWindowsSaving::LoadUserSettings( StringAnsi& settingsString )
{
	String settingsFile = GFileManager->GetUserDirectory() + TXT("\\user.settings");
	String settingsFileBackup = settingsFile + TXT(".bak");

	Bool loaded = TryLoadUserSettingsFromFile( settingsFile, settingsString );
	if ( false == loaded )
	{
		loaded = TryLoadUserSettingsFromFile( settingsFileBackup, settingsString );
		if ( loaded )
		{
			GFileManager->DeleteFileW( settingsFile );
			GFileManager->CopyFileW( settingsFileBackup, settingsFile, true );
		}
	}

	return loaded;
}

Bool CUserProfileManagerWindowsSaving::SaveUserSettings( const StringAnsi& settingsString )
{
	// Check if there is enough space on disk
	HardwareInstrumentation::CDiskInstrumentationPC diskInstrumentation;
	diskInstrumentation.GatherInformation( m_savesDir );

	if( diskInstrumentation.GetAvailableFreeSpace() < MINIMUM_REQUIRED_SPACE_FOR_SAVESETTINGS )
	{
		WARN_ENGINE( TXT("Can't save user settings - disk is out of space.") );
		return false;
	}

	String settingsFile = GFileManager->GetUserDirectory() + TXT("\\user.settings");
	String settingsFileBackup = settingsFile + TXT(".bak");

	if ( GFileManager->FileExist( settingsFile ) )
	{
		// backup before saving
		GFileManager->DeleteFileW( settingsFileBackup );
		GFileManager->CopyFileW( settingsFile, settingsFileBackup, true );
		GFileManager->DeleteFileW( settingsFile );
	}

	IFile* file = GFileManager->CreateFileWriter( settingsFile, FOF_AbsolutePath );
	if ( nullptr == file )
	{
		return false;
	}

	file->Serialize( ( void* ) settingsString.Data(), settingsString.GetLength() );
	delete file;

	return true;
}

Bool CUserProfileManagerWindowsSaving::TryLoadUserSettingsFromFile( const String& fileAbsolutePath, StringAnsi& settingsString )
{
	settingsString.ClearFast();
	TDynArray< Uint8 > temp;
	Bool succ = GFileManager->LoadFileToBuffer( fileAbsolutePath, temp, true );
	succ &= temp.Size() > 1;
	if ( succ )
	{
		settingsString.Set( ( AnsiChar* ) temp.Data(), temp.Size() - 1 );
	}
	return succ;
}

String CUserProfileManagerWindowsSaving::BuildFullDisplayNameForSave( const SSavegameInfo& info ) const
{
	SYSTEMTIME windowsTime;
	{
		windowsTime.wYear = WORD( info.m_timeStamp.GetYear() );
		windowsTime.wMonth = WORD( info.m_timeStamp.GetMonth() + 1 );
		windowsTime.wDay = WORD( info.m_timeStamp.GetDay() + 1 );
		windowsTime.wHour = WORD( info.m_timeStamp.GetHour() );
		windowsTime.wMinute = WORD( info.m_timeStamp.GetMinute() );
		windowsTime.wSecond = WORD( info.m_timeStamp.GetSecond() );
		windowsTime.wMilliseconds = WORD( 0 );
	}																																	

	FILETIME temp;
	{
		// we do this to resolve windowsTime.wDayOfWeek
		::SystemTimeToFileTime( &windowsTime, &temp );
		::FileTimeToSystemTime( &temp, &windowsTime );
	}

	// get the language and region codes set in the game
	String localeStringToUse;
	const Char* currentLanguage;
	const Char* defaultRegionForCurrentLanguage;
	SLocalizationManager::GetInstance().GetLanguageAndDefaultRegionCodes( &currentLanguage, &defaultRegionForCurrentLanguage );

	// get the system-set locale string for current user
	Char userLocaleName[ LOCALE_NAME_MAX_LENGTH ];
	userLocaleName[ 0 ] = 0;
	GetUserDefaultLocaleName( userLocaleName, LOCALE_NAME_MAX_LENGTH );

	// get the language and region codes from the locale string 
	CTokenizer tok( userLocaleName, TXT("-") );

	// check if the language match
	if ( tok.GetNumTokens() < 2 || false == tok.GetToken( 0 ).EqualsNC( currentLanguage ) )
	{
		// failed to check, or does not match, use our defaults
		localeStringToUse = String::Printf( TXT("%ls-%ls"), currentLanguage, defaultRegionForCurrentLanguage );
	}
	else
	{
		// use the system-set locale
		localeStringToUse = userLocaleName;
	}

	String out = info.GetDisplayName();
	const Bool arabicHack = ( currentLanguage[ 0 ] == L'a' && currentLanguage[ 1 ] == L'r' && currentLanguage[ 2 ] == 0 );
	if ( !arabicHack )
	{
		out = info.GetDisplayName() + TXT(" - ");
	}

	const Uint32 BUF_SIZE = 256;
	Char buffer[ BUF_SIZE ];
	{
		::GetDateFormatEx( localeStringToUse.AsChar(), arabicHack ? 0 : DATE_LONGDATE, &windowsTime, nullptr, buffer, BUF_SIZE, nullptr ); 
		out += buffer;
		out += L' ';

		::GetTimeFormatEx( localeStringToUse.AsChar(), 0, &windowsTime, nullptr, buffer, BUF_SIZE ); 

		if ( arabicHack )
		{
			Uint32 lastCharPos = Uint32( red::Strlen( buffer ) ) - 1;
			if ( lastCharPos < BUF_SIZE - 5 )
			{
				if ( 1589 == buffer[ lastCharPos ] ) // arabic equivalent of "AM"
				{
					buffer[ lastCharPos++ ] = L'A';
					buffer[ lastCharPos++ ] = L'M';
					
				}
				else if ( 1605 == buffer[ lastCharPos ] ) // arabic equivalent of "PM"
				{
					buffer[ lastCharPos++ ] = L'P';
					buffer[ lastCharPos++ ] = L'M';
				}
				else
				{
					lastCharPos++;
				}

				buffer[ lastCharPos++ ] = L' ';
				buffer[ lastCharPos++ ] = L'-';
				buffer[ lastCharPos++ ] = L' ';
				buffer[ lastCharPos ] = 0;
			}
		}

		out += buffer;
	}

	return out;
}

Bool CUserProfileManagerWindowsSaving::Initialize()
{
	m_saveListUpdated_EventPending = true;
	return true;
}

/* static */ void CUserProfileManagerWindowsSaving::AssignSlotIndices( TDynArray< SSavegameInfo >& files )
{
	Int32 currAuto = 0, currCP = 0, currM = 0, currQ = 0;
	for ( auto& file : files )
	{
		file.m_slotIndex = file.IsAutoSave() ? currAuto++ : ( file.IsCheckPoint() ? currCP++ : ( file.IsQuickSave() ? currQ++ : currM++ ) );
	}
}
