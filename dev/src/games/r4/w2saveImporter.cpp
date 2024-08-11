/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/2darray.h"
#include "../../common/core/gatheredResource.h"

#include "../../common/game/factsdb.h"

#include "w2saveImporter.h"

#ifndef NO_SAVE_IMPORT

CGatheredResource resW2import( TXT("gameplay\\globals\\w2import.csv"), RGF_Startup );

class CFactImportProcessor
{
private:
	struct ImportedFact
	{
		ImportedFact() 
			: m_expectedValue( 0 )
			, m_importedValue( 0 )
			, m_factFound( false )
		{}

		String m_oldName;
		Int32 m_expectedValue;

		String m_newName;
		Int32 m_importedValue;
		Bool m_factFound;
	};

	TDynArray< ImportedFact > m_facts;
	TDynArray< String > m_uniqueNewFacts;

	ImportedFact* FindByOldName( const String& oldName )
	{
		for ( Uint32 i = 0; i < m_facts.Size(); ++i )
		{
			if ( m_facts[ i ].m_oldName.EqualsNC( oldName ) )
			{
				return &m_facts[ i ];
			}
		}

		return nullptr;
	}

public:
	CFactImportProcessor()
	{
		C2dArray *importArray = resW2import.LoadAndGet< C2dArray > ();
		if ( importArray )
		{
			for ( Uint32 i = 0; i < importArray->GetNumberOfRows(); ++i )
			{
				ImportedFact& fact = m_facts[ ( Int32 ) m_facts.Grow() ];
				fact.m_oldName = importArray->GetValueRef( 0, i );
				Red::System::StringToInt( fact.m_expectedValue, importArray->GetValueRef( 1, i ).AsChar(), nullptr, Red::System::BaseTen );
				fact.m_newName = importArray->GetValueRef( 2, i ).ToLower();

				m_uniqueNewFacts.PushBackUnique( fact.m_newName );
			}
		}
	};

	void ProcessW2Fact( const String& name, Int32 value )
	{
		RED_LOG( SaveImport, TXT("Old fact: %s = %ld."), name.AsChar(), value );
		ImportedFact* foundFact = FindByOldName( name );
		if ( foundFact )
		{
			foundFact->m_importedValue += value;
			foundFact->m_factFound = true;
		}
	}

	void ApplyToFactsDB()
	{
		CFactsDB* factsDB = GCommonGame->GetSystem< CFactsDB > ();
		for ( Uint32 i = 0; i < m_uniqueNewFacts.Size(); ++i )
		{
			factsDB->RemoveFact( m_uniqueNewFacts[ i ] );
		}

		EngineTime time;
		for ( Uint32 i = 0; i < m_facts.Size(); ++i )
		{
			ImportedFact& fact = m_facts[ i ];
			const Bool shouldBeSet = fact.m_factFound;
			const Bool toOne = fact.m_expectedValue == fact.m_importedValue;

			if ( shouldBeSet )
			{
				RED_LOG( SaveImport, TXT("Fact %s will be set to %s."), fact.m_newName.AsChar(), toOne ? TXT("one") : TXT("zero") );
				factsDB->AddFact( fact.m_newName, toOne ? 1 : 0, time );
				GCommonGame->AddInitialFact( fact.m_newName, toOne ? 1 : 0 );
			}
		}
	}
};

IObsoleteSaveImporter* createSaveImporter()
{
	return new CW2SaveImporter();
}

CW2SaveImporter::CW2SaveImporter()
{
	Uint32 cloudPaths = GUserProfileManager->GetNumberOfWitcher2CloudSavePaths();
	m_searchPaths.Reserve( cloudPaths + 1 );

	String stdpath = FindStandardSavesPath();
	if ( false == stdpath.Empty() )
	{
		m_searchPaths.PushBack( stdpath );
		RED_LOG( SaveImport, TXT("adding search path '%ls'"), stdpath.AsChar() );
	}
	
	for ( Uint32 i = 0; i < cloudPaths; ++i )
	{
		m_searchPaths.PushBack( GUserProfileManager->GetWitcher2CloudSavePath( i ) + TXT("\\") );
		RED_LOG( SaveImport, TXT("adding search path '%ls'"), GUserProfileManager->GetWitcher2CloudSavePath( i ).AsChar() );
	}
}

String CW2SaveImporter::FindStandardSavesPath()
{
	const String& userDir = GFileManager->GetUserDirectory();
	size_t idx( 0 ), len( userDir.GetLength() );
	if ( userDir.FindCharacter( L'\\', idx, 0, len - 3, true ) )
	{
		return userDir.LeftString( idx ) + TXT("\\Witcher 2\\gamesaves\\");
	}
	return String::EMPTY;
}

void CW2SaveImporter::GetSaveFiles( TDynArray< SSavegameInfo >& files ) const
{
	TDynArray< String > allSaveFiles;

	for ( const auto& path : m_searchPaths )
	{
		// Process all files in directory
		GFileManager->FindFiles( path, TXT("*.sav"), allSaveFiles, false );

		// Output stripped file names
		files.Resize( allSaveFiles.Size() );
		for ( Uint32 i = 0; i < allSaveFiles.Size(); ++i )
		{
			RED_LOG( SaveImport, TXT("found file '%ls'"), allSaveFiles[ i ].AsChar() );

			CFilePath filePath( allSaveFiles[ i ] );
			files[ i ].m_filename = filePath.GetFileName();
			// TODO //files[ i ].m_displayName
			files[ i ].m_slotIndex = 0;
			files[ i ].m_slotType = SGT_Manual;
			files[ i ].m_timeStamp = GFileManager->GetFileTime( allSaveFiles[ i ] );
			files[ i ].m_w2import = true;
		}
	}
}

Bool CW2SaveImporter::ImportSave( const SSavegameInfo& info ) const
{
	GUserProfileManager->InitGameLoading( info );

	IGameLoader* loader = CreateLoader( info );
	if ( nullptr == loader )
	{
		GUserProfileManager->CancelGameLoading();
		return false;
	}

	CFactImportProcessor processor;

	CGameSaverBlock block( loader, CNAME( facts ) );

	Uint32 count = 0;
	loader->ReadValue( CNAME( count ), count );
	for ( Uint32 i = 0; i < count; ++i )
	{
		CGameSaverBlock factBlock( loader, CNAME( fact ) );

		String factName;
		loader->ReadValue( CNAME( id ), factName );

		Uint32 expiringCount( 0 );
		loader->ReadValue( CNAME( expiringCount ), expiringCount );

		Uint32 entryCount( 0 );
		loader->ReadValue( CNAME( entryCount ), entryCount );

		Int32 sum( 0 ); // last fact holds the real sum in W2, and that's the only value interesting for us
		for ( Uint32 k = 0; k < entryCount; ++k )
		{
			CGameSaverBlock entryBlock( loader, CNAME( entry ) );

			Int32 value( 0 );
			Int32 time( 0 );
			Int32 expiryTime( 0 );

			loader->ReadValue( CNAME( value ), value );
			loader->ReadValue( CNAME( sum ), sum );
			loader->ReadValue( CNAME( time ), time );
			loader->ReadValue( CNAME( expiryTime ), expiryTime );
		}

		processor.ProcessW2Fact( factName, sum );
	}

	processor.ApplyToFactsDB();

	GUserProfileManager->FinalizeGameLoading();

	return true;
}

IGameLoader* CW2SaveImporter::CreateLoader( const SSavegameInfo& info ) const
{
	ASSERT( info.IsW2Import() );
	IFile* saveFile = GUserProfileManager->CreateSaveFileReader();

	if ( !saveFile )
	{
		RED_LOG( Save, TXT("Unable to open file '%ls' for reading"), info.GetFileName().AsChar() );
		return NULL;
	}

	// Create wrapper
	CGameStorageReader* file = new CGameStorageReader( saveFile, SGameSessionManager::GetInstance().GetCNamesRemapper() );

	// Load magic
	Uint32 magic = 0;
	*file << magic;

	// Load save version
	Uint32 saveVersion = 0;
	*file << saveVersion;
	file->m_saveVersion = saveVersion;

	Uint32 endMagicOffset = 0;
	endMagicOffset = static_cast< Uint32 >( file->GetSize() ) - 4;

	// Load game version
	Uint32 gameVersion = 0;
	*file << gameVersion;

	// File serialization version from W2 PC
	Uint32 serialzationVersion = 115;
	file->m_version = serialzationVersion;

	// Check magic
	if ( magic != SAVE_FILE_MAGIC )
	{
		RED_LOG( Save, TXT("File '%ls' is not save file"), info.GetFileName().AsChar() );
		delete file;
		return NULL;
	}

	if ( saveVersion >= SAVE_VERSION_MAGIC_AT_END )
	{
		// Save offset
		Uint32 fileOffset = static_cast< Uint32 >( file->GetOffset() );

		// Read magic at the end of file
		file->Seek( endMagicOffset );
		Uint32 magic;
		*file << magic;

		if ( magic != SAVE_END_FILE_MAGIC )
		{
			RED_LOG( Save, TXT( "Invalid save file '%ls'" ), info.GetFileName().AsChar() );
			delete file;
			return NULL;
		}

		// Go back
		file->Seek( fileOffset );
	}

	return new CW2PCGameFileLoader( file, saveVersion );
}

CW2PCGameFileLoader::CW2PCGameFileLoader( CGameStorageReader* file, Uint32 saveVersion )
	: CGameStorageLoader( file, 1, saveVersion, 115, true ) // magic values meaning: actual last versions from the W2 PC "patch" branch.
{
}

CW2PCGameFileLoader::~CW2PCGameFileLoader()
{
}

#endif // NO_SAVE_IMPORT
