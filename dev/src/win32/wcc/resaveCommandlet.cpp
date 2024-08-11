#include "build.h"

#include "wccVersionControl.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/dependencyLinker.h"
#include "../../common/core/dependencyFileTables.h"
#include "../../common/core/depot.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/redSystem/crc.h"
#include "../../common/game/hudResource.h"
#include "../../common/game/menuResource.h"
#include "../../common/game/popupResource.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/boundedComponent.h"
#include "../../common/engine/meshTypeComponent.h"
#include "../../common/engine/meshTypeResource.h"
#include "../../common/engine/decalComponent.h"
#include "../../common/engine/dimmerComponent.h"
#include "../../common/engine/effectDummyComponent.h"
#include "../../common/engine/animDangleComponent.h"
#include "../../common/game/spawnPointComponent.h"
#include "../../common/engine/phantomComponent.h"
#include "../../common/engine/appearanceComponent.h"
#include "../../common/engine/dynamicFoliageComponent.h"
#include "../../common/engine/switchableFoliageComponent.h"
#include "../../common/engine/swarmRenderComponent.h"

class CResaveCommandlet: public ICommandlet
{
	DECLARE_ENGINE_CLASS( CResaveCommandlet, ICommandlet, 0 );

public:

	CResaveCommandlet( );

	virtual const Char* GetOneLiner( ) const;
	virtual void PrintHelp( ) const;
	virtual bool Execute( const CommandletOptions& options );

private:

	struct Settings
	{
		String			m_tmpDir;
		String			m_basePath;
		TList< String >	m_extensions;
		Bool			m_useSourceControl;
		Bool			m_ignoreFileVersion;
		Bool			m_forceStreamingSetup;
		Bool			m_forceIncludeResave;
		Uint32			m_changeListNumber;
		Bool			m_submitWhenFinished;
		Bool			m_discardUnchangedData;
		String			m_customResaveFilename;

		Settings( )
			: m_tmpDir( String::EMPTY )
			, m_basePath( String::EMPTY )
			, m_useSourceControl( false )
			, m_ignoreFileVersion( true )
			, m_forceStreamingSetup( true )
			, m_forceIncludeResave( true )
			, m_changeListNumber( 0 )
			, m_submitWhenFinished( false )
			, m_discardUnchangedData( false )
			, m_customResaveFilename( String::EMPTY )
		{ }

		Bool Parse( const ICommandlet::CommandletOptions& options );
	};

	struct Results
	{
		Uint32	succeed;
		Uint32	failed;
		Uint32	ignored;
		Uint32	notLoaded;

		Results( )
			: succeed( 0 )
			, failed( 0 )
			, ignored( 0 )
			, notLoaded( 0 )
		{ }
	};

private:

	Bool InitializeVersionControl( const ICommandlet::CommandletOptions& options );
	void BuildValidExtensionList( );
	Bool ScanDepotFileList( );
	void ScanDirectoryForResources( CDirectory* dir );
	void GetResourcesBasedOnList( TDynArray< String > fileNameList );
	void ReportFinalSetup( );
	void ResaveGatheredFiles( );
	void DiscardUnchangedData( );
	void SubmitResavedFiles( );

private:

	void PrintScanResults( ) const;
	void PrintResaveResults( ) const;
	void PrintDiscardResults( ) const;
	void PrintSubmitResults( ) const;
	void PrintAllResults( ) const;

private: // Helper functions. Better here inside a scoped namespace.

	int SuggestLODFromDistance( Float autoHideDistance ) const;
	void ForceStreamingSetupToEntity( CEntity* entity ) const;
	Bool ProcessEntityTemplateForResave( CEntityTemplate* tpl ) const;
	Bool ProcessLayerForResave( CLayer* layer ) const;
	Bool ProcessHudForResave( CHudResource* hud ) const;
	Bool ProcessMenuForResave( CMenuResource* menu ) const;
	Bool ProcessPopupForResave( CPopupResource* popup ) const;
	Bool ProcessResourceForResave( CResource* res ) const;
	Bool IsUpToDate( CDiskFile* file ) const;
	Bool ShouldIgnoreFile( CDiskFile* file ) const;
	Bool LoadResourceFile( CDiskFile* file ) const;
	Bool AfterFileSaved( CDiskFile* file, const String& absoluteFilePath ) const;
	void ResaveLogToFile( String someEntry, String someReason ) const;
	Uint32 GetFileDataCRC( const String& fileAbsolutePath ) const;

private:

	Settings					m_settings;
	THashSet< String >			m_validExtensions;
	String						m_validExtensionsString;
	TDynArray< CDiskFile* >		m_filesToResave;

private:

	Results						m_scanResults;
	Results						m_resaveResults;
	Results						m_discardResults;
	Results						m_submitResults;
};

DEFINE_SIMPLE_RTTI_CLASS( CResaveCommandlet, ICommandlet );
IMPLEMENT_ENGINE_CLASS( CResaveCommandlet );

CResaveCommandlet::CResaveCommandlet()
{
	m_commandletName = CNAME( resave );
}

const Char* CResaveCommandlet::GetOneLiner( ) const
{
	return TXT( "Resaves resources and optionally submits the changes into P4." );
}

void CResaveCommandlet::PrintHelp( ) const
{
	LOG_WCC( TXT( "Usage:" ) );
	LOG_WCC( TXT( "  resave -tmpdir=<dirpath> [options]" ) );
	LOG_WCC( TXT( "    -tmpdir                - Absolute path to temporary folder." ) );
	LOG_WCC( TXT( "" ) );
	LOG_WCC( TXT( "Options:" ) );
	LOG_WCC( TXT( "  -path=<dirpath>        - Base directory path to scan (full depot resave if not specified)." ) );
	LOG_WCC( TXT( "  -ext=<ext1,ext2,...>   - Comma-separated list of extensions to scan for (full resave if not specified)." ) );
	LOG_WCC( TXT( "  -nosourcecontrol       - Disables version control (local resave)." ) );
	LOG_WCC( TXT( "  -ignorefileversion     - Resave even if files are latest version." ) );
	LOG_WCC( TXT( "  -forcestreamingsetup   - Force reset and setup of streaming in world and layers (dangerous!)." ) );
	LOG_WCC( TXT( "  -cl=<number>           - Add resaved files into the given changelist (when enabled)." ) );
	LOG_WCC( TXT( "  -submitwhenfinished    - Enables CL submit when finished (disabled by default)." ) );
	LOG_WCC( TXT( "  -discardunchangeddata  - Discards files for which data has not changed (disabled by default, potentially slow)." ) );
	LOG_WCC( TXT( "  -customresave          - Resaves only the given list of files." ) );
	LOG_WCC( TXT( "" ) );
	LOG_WCC( TXT( "Version control options:" ) );
	LogWCCVersionControlOptions( );
}

bool CResaveCommandlet::Execute( const CommandletOptions& options )
{
	if( !m_settings.Parse( options ) )
		return false;

	if( !InitializeVersionControl( options ) )
		return false;

	BuildValidExtensionList( );

	if( !ScanDepotFileList( ) )
		return false;

	ReportFinalSetup( );
	ResaveGatheredFiles( );
	DiscardUnchangedData( );
	SubmitResavedFiles( );

	PrintAllResults( );

	return true;
}

Bool CResaveCommandlet::Settings::Parse( const ICommandlet::CommandletOptions& options )
{
	// Get and fix temporary directory path.
	if( !options.GetSingleOptionValue( TXT( "tmpdir" ), m_tmpDir ) || m_tmpDir.Empty( ) )
	{
		ERR_WCC( TXT( "No temporary directory path has been specified!" ) );
		return false;
	}
	if( !m_tmpDir.EndsWith( TXT( "\\" ) ) )
	{
		m_tmpDir += TXT( "\\" );
	}

	// Get and fix input base path.
	if( options.GetSingleOptionValue( TXT( "path" ), m_basePath ) )
	{
		if( !m_basePath.Empty( ) && !m_basePath.EndsWith( TXT( "\\" ) ) )
		{
			m_basePath += TXT( "\\" );
		}
	}

	// Check for input extensions.
	if( options.HasOption( TXT( "ext" ) ) )
	{
		m_extensions = options.GetOptionValues( TXT( "ext" ) );
	}

	// Get some flags.
	m_useSourceControl = !options.HasOption( TXT( "nosourcecontrol" ) );
	m_ignoreFileVersion = options.HasOption( TXT( "ignorefileversion" ) );
	m_forceStreamingSetup = options.HasOption( TXT( "forcestreamingsetup" ) );
	m_forceIncludeResave = options.HasOption( TXT( "forceincluderesave" ) );
	m_submitWhenFinished = options.HasOption( TXT( "submitwhenfinished" ) );
	m_discardUnchangedData = options.HasOption( TXT( "discardunchangeddata" ) );

	// Check for change list number.
	String clString;
	if( !options.GetSingleOptionValue( TXT( "cl" ), clString ) || !::FromString( clString, m_changeListNumber ) )
	{
		m_changeListNumber = 0;
	}

	// Use custom filename to specify resave resources.
	options.GetSingleOptionValue( TXT( "customresave" ), m_customResaveFilename );

	return true;
}

Bool CResaveCommandlet::InitializeVersionControl( const ICommandlet::CommandletOptions& options )
{
	if( m_settings.m_useSourceControl )
	{
		if( !InitializeWCCVersionControl( options ) )
		{
			ERR_WCC( TXT( "Failed to initialize source version control!" ) );
			return false;
		}
	}

	return true;
}

void CResaveCommandlet::BuildValidExtensionList( )
{
	// Get all known resource extensions.
	TDynArray< CClass* > resourceClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< CResource >( ), resourceClasses );
	for( Uint32 i = 0; i < resourceClasses.Size( ); ++i )
	{
		m_validExtensions.Insert( resourceClasses[ i ]->GetDefaultObject< CResource >( )->GetExtension( ) );
	}

	if( m_settings.m_customResaveFilename.Empty( ) )
	{
		// Filter extensions
		TDynArray< String > customExtensions;		// Build a list of extensions we care about (if its empty, we build everything)
		for( auto extIt = m_settings.m_extensions.Begin( ); extIt != m_settings.m_extensions.End( ); ++extIt )
		{
			String& ext = *extIt;

			// Check it's a valid extension.
			if ( !m_validExtensions.Exist( ext ) )
			{
				WARN_WCC( TXT( "Extension %s is not recognized! It will be ignored!" ), ext.AsChar( ) );
				continue;
			}
			customExtensions.PushBackUnique( ext );
		}

		// If there are valid extensions in the command line, use them.
		if( customExtensions.Size( ) > 0 )
		{
			m_validExtensions.Clear( );
			for( auto extIt : customExtensions )
			{
				m_validExtensions.Insert( extIt );
			}
		}

		// Build a user friendly list of the extensions that will be processed (just for debug).
		for( auto fnlExtIt = m_validExtensions.Begin( ); fnlExtIt != m_validExtensions.End( ); ++fnlExtIt )
		{
			if( m_validExtensionsString.GetLength( ) > 0 )
			{
				m_validExtensionsString += TXT( ", " );
			}
			m_validExtensionsString += *fnlExtIt;
		}
	}
}

Bool CResaveCommandlet::ScanDepotFileList( )
{
	if( !m_settings.m_customResaveFilename.Empty( ) )
	{
		TDynArray<String> fileNameList;

		String bigNastyString = String::EMPTY;
		if( GFileManager->LoadFileToString( m_settings.m_customResaveFilename, bigNastyString, true ) )
		{
			fileNameList = bigNastyString.Split(TXT("\n") );
		}
		else
		{
			// Something went wrong here
			ERR_WCC( TXT("Failed to load custom resave filename") );
			return false;
		}

		GetResourcesBasedOnList( fileNameList );
	}
	else
	{
		// Find base directory.
		CDirectory* baseDirectory = GDepot->FindPath( m_settings.m_basePath.AsChar( ) );
		if( baseDirectory == nullptr )
		{
			ERR_WCC( TXT( "Failed to locate base directory '%s' in the depot!" ), m_settings.m_basePath.AsChar( ) );
			return false;
		}

		// Scan directory contents.
		LOG_WCC( TXT( "Scanning for files..." ) );
		ScanDirectoryForResources( baseDirectory );
	}

	return true;
}

void CResaveCommandlet::ScanDirectoryForResources( CDirectory* dir )
{
	// Scan files in current directory.
	const TFiles& files = dir->GetFiles( );
	for( auto it = files.Begin( ); it != files.End( ); ++it )
	{
		CDiskFile* file = *it;
		CFilePath filePath( file->GetDepotPath( ) );
		if ( m_validExtensions.Exist( filePath.GetExtension( ) ) )
		{
			m_filesToResave.PushBack( file );
			++m_scanResults.succeed;
		}
		else
			++m_scanResults.ignored;
	}

	// Scan subdirectories.
	for( CDirectory* child : dir->GetDirectories( ) )
	{
		ScanDirectoryForResources( child );
	}
}

void CResaveCommandlet::GetResourcesBasedOnList( TDynArray<String> fileNameList )
{
	// Get files
	for ( Uint32 i=0; i<fileNameList.Size(); ++i )
	{
		if( !fileNameList[ i ].Empty() )
		{
			// local R4 data hack since we don't have proper game/folder variable
			String localR4FileHack = fileNameList[ i ].StringAfter( TXT("r4data\\") );

			CDiskFile* file = GDepot->FindFile( localR4FileHack );
			if( file )
			{
				CFilePath filePath( file->GetDepotPath() );
				if ( m_validExtensions.Exist( filePath.GetExtension() ) )
				{
					m_filesToResave.PushBack( file );
					LOG_WCC( TXT("Adding %s..."), fileNameList[ i ].AsChar() );
					++m_scanResults.succeed;
				}
				else
					++m_scanResults.ignored;
			}
			else
			{
				LOG_WCC( TXT("Checking %s [FAILURE - Cannot find file!]"), fileNameList[ i ].AsChar() );
				++m_scanResults.notLoaded;
			}
		}
		else
			LOG_WCC( TXT("Checking EMPTY [FAILURE - Cannot open an EMPTY file!]") );
	}
}

void CResaveCommandlet::ReportFinalSetup( )
{
	LOG_WCC( TXT( "Resaving base path: %s" ), m_settings.m_basePath.AsChar( ) );
	LOG_WCC( TXT( "Using tempdir path: %s" ), m_settings.m_tmpDir.AsChar( ) );
	LOG_WCC( TXT( "Resaved extensions: %s" ), m_validExtensionsString.AsChar( ) );
	if( m_settings.m_changeListNumber != 0 )
	{
		LOG_WCC( TXT( "Checked out change list: %u" ), m_settings.m_changeListNumber );
	}
	if( m_settings.m_forceStreamingSetup )
	{
		LOG_WCC( TXT( "IMPORTANT: Forcing streaming setup!" ) );
	}
	if( m_settings.m_forceIncludeResave )
	{
		LOG_WCC( TXT( "IMPORTANT: Forcing entity template include resave!" ) );
	}
}

void CResaveCommandlet::ResaveGatheredFiles( )
{
	LOG_WCC( TXT( "Resaving %d resource files..." ), m_filesToResave.Size( ) );

	// Re-save.
	for( Uint32 i = 0; i < m_filesToResave.Size( ); ++i )
	{
		CDiskFile* file = m_filesToResave[i];

		if( m_settings.m_useSourceControl && !m_settings.m_ignoreFileVersion )
		{
			if( IsUpToDate( file ) )
			{
				LOG_WCC( TXT( "%6i. %s [SUCCESS - Up to date]" ), i, file->GetDepotPath( ).AsChar( ) );
				++m_resaveResults.succeed;
				continue;
			}
		}

		if( ShouldIgnoreFile( file ) )
		{
			WARN_WCC( TXT(  "%6i. %s [IGNORED - Skipped]" ), i, file->GetDepotPath( ).AsChar( ) );
			++m_resaveResults.ignored;
			continue;
		}

		// Load resource
		if( !LoadResourceFile( file ) )
		{
			WARN_WCC( TXT(  "%6i. %s [FAILURE - Cannot load resource]" ), i, file->GetDepotPath( ).AsChar( ) );
			if( !m_settings.m_customResaveFilename.Empty( ) ) ResaveLogToFile( file->GetDepotPath( ), TXT( "[FAILURE - Cannot load resource]" ) );
			++m_resaveResults.notLoaded;
			continue;
		}

		LOG_WCC( TXT( "%6i. %s..." ), i, file->GetDepotPath( ).AsChar( ) );

		// Process resource
		if( file->GetResource( ) != nullptr )
		{
			if( !file->GetResource( )->MarkModified( ) )
			{
				WARN_WCC( TXT( "%6i. %s [FAILURE - Cannot mark as modified]" ), i, file->GetDepotPath( ).AsChar( ) );
				if( !m_settings.m_customResaveFilename.Empty( ) ) ResaveLogToFile( file->GetDepotPath( ), TXT( "[FAILURE - Cannot mark as modified]" ) );
				++m_resaveResults.failed;
				file->Unload( );
				continue;
			}
			if( !ProcessResourceForResave( file->GetResource( ) ) )
			{
				WARN_WCC( TXT( "%6i. %s [FAILURE - Resave failed]" ), i, file->GetDepotPath( ).AsChar( ) );
				if( !m_settings.m_customResaveFilename.Empty( ) ) ResaveLogToFile( file->GetDepotPath( ), TXT( "[FAILURE - Resave failed]" ) );
				++m_resaveResults.failed;
				file->Unload( );
				continue;
			}
		}

		// Save resource
		String outFilepath = m_settings.m_tmpDir + file->GetDepotPath( );
		if( !file->Save( outFilepath ) )
		{
			WARN_WCC( TXT( "%6i. %s [FAILURE - Cannot save temp file]" ), i, outFilepath.AsChar( ) );
			if( !m_settings.m_customResaveFilename.Empty( ) ) ResaveLogToFile( file->GetDepotPath( ), TXT( "[FAILURE - Cannot save temp file]" ) );
			++m_resaveResults.failed;
			file->Unload( );
			continue;
		}

		file->Unload();

		// Process the file after it was resaved (append layer info, thumbnails...)
		if ( !AfterFileSaved( file, outFilepath ) )
		{
			GFileManager->DeleteFile( outFilepath );
			LOG_WCC( TXT( "%6i. %s [FAILED - Cannot post-process resaved file]" ), i, outFilepath.AsChar( ) );
			++m_resaveResults.failed;
		}
		else
		{
			LOG_WCC( TXT( "%6i. %s [SUCCESS - Resaved]" ), i, outFilepath.AsChar( ) );
			++m_resaveResults.succeed;
		}

		// Process stuff every now and then so we don't run out of memory.
		if( i > 0 && ( ( i+1 ) % 10 ) == 0 )
		{
			SEvents::GetInstance( ).ProcessPendingEvens( );
			SGarbageCollector::GetInstance( ).CollectNow( );
		}
	}

	// One last cleanup pass.
	SEvents::GetInstance( ).ProcessPendingEvens( );
	SGarbageCollector::GetInstance( ).CollectNow( );
}

void CResaveCommandlet::DiscardUnchangedData( )
{
	if( !m_settings.m_discardUnchangedData )
		return;

	Uint32 discarded = 0, kept = 0;

	for( Uint32 i = 0; i < m_filesToResave.Size( ); ++i )
	{
		String resavedFilepath = m_settings.m_tmpDir + m_filesToResave[ i ]->GetDepotPath( );

		// Skip the files that were not processed.
		if( !GFileManager->FileExist( resavedFilepath ) )
			continue;

		Uint32 originalCRC = GetFileDataCRC( m_filesToResave[ i ]->GetAbsolutePath( ) );
		Uint32 resavedCRC = GetFileDataCRC( resavedFilepath );

		// Remove re-saved file if data hasn't changed, or for some reason file header is broken.
		if( originalCRC == resavedCRC || resavedCRC == 0 )
		{
			GFileManager->DeleteFile( resavedFilepath );
			LOG_WCC( TXT( "Resaved file '%s' data didn't change from previous version. Discarding.." ), resavedFilepath.AsChar( ) );
			++m_discardResults.ignored;
		}
		else
		{
			++m_discardResults.succeed;
		}
	}
}

void CResaveCommandlet::SubmitResavedFiles( )
{
	// Create/get change list.
	SChangelist changeList = SChangelist::DEFAULT;
	if( m_settings.m_useSourceControl && m_settings.m_changeListNumber != 0 )
	{
		if( !static_cast< CSourceControlP4* >( GVersionControl )->CreateChangelistWithNumber( m_settings.m_changeListNumber, changeList ) )
		{
			changeList = SChangelist::DEFAULT;
		}
	}

	LOG_WCC( TXT( "Updating %d depot files:" ), m_filesToResave.Size( ) );

	for( Uint32 i = 0; i < m_filesToResave.Size( ); ++i )
	{
		String resavedFilepath = m_settings.m_tmpDir + m_filesToResave[ i ]->GetDepotPath( );

		// Skip the files that were not processed.
		if( !GFileManager->FileExist( resavedFilepath ) )
			continue;

		// Ask the source control to check out this file.
		m_filesToResave[ i ]->SetChangelist( changeList );
		if( m_settings.m_useSourceControl && !( m_filesToResave[ i ]->IsCheckedIn( ) || m_filesToResave[ i ]->IsCheckedOut( ) ) && !m_filesToResave[ i ]->CheckOut( ) )
		{
			WARN_WCC( TXT( "%6i. %s [FAILURE - Cannot check out]" ), i, m_filesToResave[ i ]->GetDepotPath( ).AsChar( ) );
			if( !m_settings.m_customResaveFilename.Empty( ) ) ResaveLogToFile( m_filesToResave[ i ]->GetDepotPath( ), TXT( "[FAILURE - Cannot check out]" ) );
			++m_submitResults.failed;
			continue;
		}

		// Copy the re-saved file from the temporary folder into the proper depot folder.
		GFileManager->SetFileReadOnly( m_filesToResave[ i ]->GetAbsolutePath( ).AsChar( ), false );
		if( !GFileManager->CopyFile( resavedFilepath.AsChar( ), m_filesToResave[ i ]->GetAbsolutePath( ).AsChar( ), true ) )
		{
			WARN_WCC( TXT( "%6i. %s [FAILURE - Cannot copy temp file into depot]" ), i, resavedFilepath.AsChar( ) );
			if( !m_settings.m_customResaveFilename.Empty( ) ) ResaveLogToFile( m_filesToResave[ i ]->GetDepotPath( ), TXT( "[FAILURE - Cannot copy temp file into depot]" ) );
			++m_submitResults.notLoaded;
			continue;
		}

		++m_submitResults.succeed;
	}

	// Submit if it's not in default change list.
	if( m_settings.m_useSourceControl && m_settings.m_submitWhenFinished && changeList != SChangelist::DEFAULT )
	{
		LOG_WCC( TXT( "Submitting files.." ) );
		if( !static_cast< CSourceControlP4* >( GVersionControl )->Submit( changeList ) )
		{
			WARN_WCC( TXT( "Changelist %d [FAILURE - Cannot submit change list]" ), m_settings.m_changeListNumber );
			m_submitResults.ignored = m_submitResults.succeed;
		}
		LOG_WCC( TXT( "All files submitted!" ) );
	}
	else
	{
		LOG_WCC( TXT( "Nothing has been submited." ) );
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------------------

void CResaveCommandlet::PrintScanResults( ) const
{
	LOG_WCC( TXT( " Scan result: %d added, %d ignored, %d couln't be loaded" ), m_scanResults.succeed, m_scanResults.ignored, m_scanResults.notLoaded );
}

void CResaveCommandlet::PrintResaveResults( ) const
{
	LOG_WCC( TXT( " Resave result: %d succeed, %d failed, %d ignored, %d couln't be loaded" ), m_resaveResults.succeed, m_resaveResults.failed, m_resaveResults.ignored, m_resaveResults.notLoaded );
}

void CResaveCommandlet::PrintDiscardResults( ) const
{
	if( m_settings.m_discardUnchangedData )
		LOG_WCC( TXT( " Unchanged data discard result: %d discarded, %d kept" ), m_discardResults.ignored, m_discardResults.succeed );
	else
		LOG_WCC( TXT( " Unchanged data discard result: Not performed." ) );
}

void CResaveCommandlet::PrintSubmitResults( ) const
{
	if( m_settings.m_useSourceControl )
	{
		LOG_WCC( TXT( " Data submit result: %d checked out and copied, %d failed to checkout, %d couldn't be copied to depot" ), m_submitResults.succeed, m_submitResults.failed, m_submitResults.notLoaded );
		if( m_settings.m_submitWhenFinished && m_submitResults.ignored > 0 )
			LOG_WCC( TXT( " CHANGE LIST SUBMIT FAILED!" ) );
	}
	else
	{
		LOG_WCC( TXT( " Data submit result: %d copied to depot, %d couldn't be copied, nothing submitted" ), m_submitResults.succeed, m_submitResults.notLoaded );
	}
}

void CResaveCommandlet::PrintAllResults( ) const
{
	LOG_WCC( TXT( "" ) );
	LOG_WCC( TXT( "---------------------------------------------------------------------------------------------" ) );
	LOG_WCC( TXT( " RESAVE COMMANDLET RESULTS" ) );
	LOG_WCC( TXT( "---------------------------------------------------------------------------------------------" ) );
	PrintScanResults( );
	LOG_WCC( TXT( "---------------------------------------------------------------------------------------------" ) );
	PrintResaveResults( );
	LOG_WCC( TXT( "---------------------------------------------------------------------------------------------" ) );
	PrintDiscardResults( );
	LOG_WCC( TXT( "---------------------------------------------------------------------------------------------" ) );
	PrintSubmitResults( );
	LOG_WCC( TXT( "---------------------------------------------------------------------------------------------" ) );
	LOG_WCC( TXT( "" ) );
}

// ------------------------------------------------------------------------------------------------------------------------------------------------

int CResaveCommandlet::SuggestLODFromDistance( Float autoHideDistance ) const
{ 
	if ( autoHideDistance <= 128.0f ) return 0;
	else if ( autoHideDistance <= 256.0f ) return 1;
	else if ( autoHideDistance <= 512.0f ) return 2;
	else return -1;
}

void CResaveCommandlet::ForceStreamingSetupToEntity( CEntity* entity ) const
{
	// Use the ForceFinishAsyncResourceLoads Luke
	entity->ForceFinishAsyncResourceLoads();

	const TDynArray< CComponent* >& components = entity->GetComponents();

	// Check if we should force the entity to be streamed
	
	Bool shouldForceStreaming = false;
	for ( auto it=components.Begin(); it != components.End(); ++it )
	{		
		if( entity->AllowStreamingForComponent( *it ) )
		{
			shouldForceStreaming = true;
			break;
		}		
	}

	entity->SetStreamed( shouldForceStreaming );
	
	// If the entity isn't streamed, don't bother
	if ( !entity->ShouldBeStreamed() )
	{
		return;
	}

	// Regenerate streaming LODs
	for ( auto it=components.Begin(); it != components.End(); ++it )
	{		
		CBoundedComponent* boundedComponent = Cast< CBoundedComponent >( *it );
		CMeshTypeComponent* meshTypeComponent = Cast< CMeshTypeComponent>( *it );
		CDecalComponent* decalComponent = Cast< CDecalComponent >( *it );
		CDimmerComponent* dimmerComponent = Cast< CDimmerComponent >( *it );

		CEffectDummyComponent* effectDummyComponent = Cast< CEffectDummyComponent >( *it );
		CPhantomComponent* phantomComponent = Cast< CPhantomComponent >( *it );
		CAnimatedComponent* animatedComponent = Cast< CAnimatedComponent >( *it );
		CAnimDangleComponent* animDangleComponent = Cast< CAnimDangleComponent >( *it );
		CSoundEmitterComponent* soundEmitterComponent = Cast< CSoundEmitterComponent >( *it );
		CInteractionComponent* interactionComponent = Cast< CInteractionComponent >( *it );	
		CDynamicFoliageComponent* dynamicFoliageComponent = Cast< CDynamicFoliageComponent >( *it );
		CSwitchableFoliageComponent* switchableFoliageComponent = Cast< CSwitchableFoliageComponent >( *it );
		CSwarmRenderComponent* swarmComponent = Cast< CSwarmRenderComponent >( *it );

		if ( meshTypeComponent != nullptr )
		{
			CMeshTypeResource* meshRes = meshTypeComponent->GetMeshTypeResource();
			if ( meshRes != nullptr )
			{
				if( meshRes->GetAutoHideDistance() < 460 ) meshTypeComponent->SetStreamed( true );
				else
					meshTypeComponent->SetStreamed( false );
			}
			else
			{
				boundedComponent->SetStreamed( true );
			}
		}
		else if ( decalComponent != nullptr )
		{
			decalComponent->SetStreamed( true );
		}
		else if ( dimmerComponent != nullptr )
		{
			dimmerComponent->SetStreamed( true );
		}
		else if ( boundedComponent != nullptr )
		{
			boundedComponent->SetStreamed( true );
		}
		else if ( effectDummyComponent != nullptr )
		{
			effectDummyComponent->SetStreamed( true );
		}
		else if ( phantomComponent != nullptr )
		{
			phantomComponent->SetStreamed( true );
		}
		else if ( animatedComponent != nullptr )
		{
			if ( animatedComponent == animatedComponent->GetEntity()->GetRootAnimatedComponent() )
			{
				animatedComponent->SetStreamed( false );
			}
			else
			{
				animatedComponent->SetStreamed( true );
			}
		}
		else if ( animDangleComponent != nullptr )
		{
			animDangleComponent->SetStreamed( true );
		}
		else if ( soundEmitterComponent != nullptr )
		{
			Float dist = soundEmitterComponent->GetMaxDistance();
			if( dist < 460 ) soundEmitterComponent->SetStreamed( true );
			else
				soundEmitterComponent->SetStreamed( false );
		}		
		else if ( interactionComponent != nullptr )
		{
			interactionComponent->SetStreamed( true );
		}
		else if ( dynamicFoliageComponent != nullptr )
		{
			dynamicFoliageComponent->SetStreamed( true );
		}
		else if ( switchableFoliageComponent != nullptr )
		{
			switchableFoliageComponent->SetStreamed( true );
		}
		else if ( swarmComponent != nullptr )
		{			
			if( CMeshTypeResource* m = swarmComponent->GetMesh() )
			{
				if( m->GetAutoHideDistance() < 460 ) swarmComponent->SetStreamed( true );
				else
					swarmComponent->SetStreamed( false );
			}
			else
				swarmComponent->SetStreamed( false );			
		}
		// in case some components were removed from the streaming allowed list (see entity.cpp)
		else
		{
			( *it )->SetStreamed( false );
		}		

	}
}

Bool CResaveCommandlet::ProcessEntityTemplateForResave( CEntityTemplate* tpl ) const
{
	static THashSet< CDiskFile* > processedTemplates;

	// Ignore already processed templates
	if ( processedTemplates.Exist( tpl->GetFile() ) )
	{
		return true;
	}
	processedTemplates.Insert( tpl->GetFile() );

	// First resave any includes (if we were asked to do so)
	if ( m_settings.m_forceIncludeResave )
	{
		// Direct includes
		const TDynArray< THandle< CEntityTemplate > >& includes = tpl->GetIncludes();
		for ( auto it=includes.Begin(); it != includes.End(); ++it )
		{
			CEntityTemplate* include = (*it).Get();
			if ( include == nullptr ) continue;

			// Try to mark that as modified
			if ( !include->MarkModified() )
			{
				LOG_WCC( TXT( "   == [Error] Failed to mark modify (checkout?) include: %s" ), include->GetFile()->GetAbsolutePath().AsChar() );
				continue;
			}

			LOG_WCC( TXT( "   == Resaving Include: %s" ), include->GetFile()->GetAbsolutePath().AsChar() );
			ProcessEntityTemplateForResave( include );

			// Load thumbnails before save - otherwise they'll be gone
			include->GetFile()->LoadThumbnail();

			// Save
			include->Save();
		}

		// Appearances
		const TDynArray< CEntityAppearance >& appearances = tpl->GetAppearances();
		for ( auto it=appearances.Begin(); it != appearances.End(); ++it )
		{
			const CEntityAppearance& appearance = *it;
			const TDynArray< THandle< CEntityTemplate > >& includes = appearance.GetIncludedTemplates();
			for ( auto it=includes.Begin(); it != includes.End(); ++it )
			{
				CEntityTemplate* include = (*it).Get();
				if ( include == nullptr ) continue;

				// Try to mark that as modified
				if ( !include->MarkModified() )
				{
					LOG_WCC( TXT( "   == [Error] Failed to mark modify (checkout?) appearance '%s' include: %s" ), appearance.GetName().AsChar(), include->GetFile()->GetAbsolutePath().AsChar() );
					continue;
				}

				LOG_WCC( TXT( "   == Resaving Appearance '%s' Include: %s" ), appearance.GetName().AsChar(), include->GetFile()->GetAbsolutePath().AsChar() );
				ProcessEntityTemplateForResave( include );
				
				// Load thumbnails before save - otherwise they'll be gone
				include->GetFile()->LoadThumbnail();

				// Save
				include->Save();
			}
		}
	}
	
	// Create entity
	CEntity* entity = tpl->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
	if ( entity == nullptr ) return false;

	entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
	if ( m_settings.m_forceStreamingSetup )
	{
		ForceStreamingSetupToEntity( entity );
	}
	entity->UpdateStreamedComponentDataBuffers();

	// Update the streaming distance in source
	entity->UpdateStreamingDistance( );

	entity->PrepareEntityForTemplateSaving();
	entity->DetachTemplate();
	tpl->CaptureData( entity );

	// Destroy instance
	entity->Discard();

	// Second instance to convert any new properties
	entity = tpl->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
	if ( entity != nullptr )
	{
		entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
		entity->UpdateStreamedComponentDataBuffers();
		// Update the streaming distance in source
		entity->UpdateStreamingDistance();
		entity->Discard();
	}

	return true;
}

Bool CResaveCommandlet::ProcessLayerForResave( CLayer* layer ) const
{
	// Stream in all entity components
	const LayerEntitiesArray& entities = layer->GetEntities( );
	for ( auto it=entities.Begin( ); it != entities.End( ); ++it )
	{
		CEntity* ent = *it;
		// Ignore templated objects, we only want to refresh entities stored in the layer.
		if ( ent->GetEntityTemplate( ) != nullptr )
		{
			continue;
		}

		ent->MarkModified( );
		ent->CreateStreamedComponents( SWN_DoNotNotifyWorld );
		if ( m_settings.m_forceStreamingSetup )
		{
			ForceStreamingSetupToEntity( ent );
		}
		ent->UpdateStreamedComponentDataBuffers( );
		// Update the streaming distance in source
		ent->UpdateStreamingDistance();
		ent->DestroyStreamedComponents( SWN_DoNotNotifyWorld );
	}


	// Inform the layer we're going to save it editor-style
	layer->OnResourceSavedInEditor( );
	return true;
}

Bool CResaveCommandlet::ProcessHudForResave( CHudResource* hud ) const
{
	String swfPath = hud->GetHudFlashSwf().GetPath().ToLower();
	swfPath.Replace( TXT(".swf"), TXT(".redswf"), true );
	CHudResource::ResaveHudFlashSwf( hud, swfPath );

	return true;
}

Bool CResaveCommandlet::ProcessMenuForResave( CMenuResource* menu ) const
{
	String swfPath = menu->GetMenuFlashSwf().GetPath().ToLower();
	swfPath.Replace( TXT(".swf"), TXT(".redswf"), true );
	CMenuResource::ResaveMenuFlashSwf( menu, swfPath );

	return true;
}

Bool CResaveCommandlet::ProcessPopupForResave( CPopupResource* popup ) const
{
	String swfPath = popup->GetPopupFlashSwf().GetPath().ToLower();
	swfPath.Replace( TXT(".swf"), TXT(".redswf"), true );
	CPopupResource::ResavePopupFlashSwf( popup, swfPath );

	return true;
}

Bool CResaveCommandlet::ProcessResourceForResave( CResource* res ) const
{
#define RESOURCE_PROCESS_PROC(t,p) \
	if ( res->IsA< t >() ) \
	{ \
		return p( static_cast< t* >( res ) ); \
	}

	// Add more processors below
	RESOURCE_PROCESS_PROC( CLayer, ProcessLayerForResave );
	RESOURCE_PROCESS_PROC( CEntityTemplate, ProcessEntityTemplateForResave );
	RESOURCE_PROCESS_PROC( CHudResource, ProcessHudForResave );
	RESOURCE_PROCESS_PROC( CMenuResource, ProcessMenuForResave );
	RESOURCE_PROCESS_PROC( CPopupResource, ProcessPopupForResave );

	return true;
}

Bool CResaveCommandlet::IsUpToDate( CDiskFile* file ) const
{
	// Create reader
	Red::TScopedPtr< IFile > reader ( file->CreateReader() );
	if ( !reader )
	{
		WARN_WCC( TXT( "Can't create reader for file: '%s'" ), file->GetAbsolutePath().AsChar() );
		return false;
	}

	// File is to small
	if ( reader->GetSize() < 8 )
		return false;

	// Read header
	Uint32 magic=0, version=0;
	*reader << magic;
	*reader << version;

	// Check version
	if ( magic != CDependencyLoader::FILE_MAGIC )
	{
		WARN_WCC( TXT( "Invalid header in file: '%s'" ), file->GetAbsolutePath().AsChar() );
		return false;
	}

	// Not current version
	if ( version != CDependencyLoader::FILE_VERSION )
	{
		LOG_WCC( TXT( "File: '%s' version: '%d' is != to current version: '%d'" ), file->GetAbsolutePath().AsChar(), version, VER_CURRENT  );
		return false;
	}

	return true;
}

Bool CResaveCommandlet::ShouldIgnoreFile( CDiskFile* file ) const
{
	return false;
}

Bool CResaveCommandlet::LoadResourceFile( CDiskFile* file ) const
{
	if ( !file->Load() )
	{
		return false;
	}
	if ( !file->LoadThumbnail() )
	{
		// if loading thumbnails failed, that means the thumbnail array should be empty
		return file->GetThumbnails().Empty();
	}

	// file loaded, thumbnails loaded, that means the thumbnail array should not be empty
	return !file->GetThumbnails().Empty();
}

Bool CResaveCommandlet::AfterFileSaved( CDiskFile* file, const String& absoluteFilePath ) const
{
	const String& fileName = file->GetFileName();

	// a layer ? :)
	/*if ( fileName.EndsWith( ResourceExtension< CLayer >() ) )
	{
		// Get layer contents and copy them into new instance.
		CLayerInfo* layerInfo = CLayerGroup::GrabRawDynamicLayerInfo( file->GetDepotPath() );
		if ( !layerInfo )
		{
			ERR_WCC( TXT( "Layer %ls does not contain layer info and is not valid." ), file->GetDepotPath( ).AsChar( ) );
			return false;
		}

		if ( !layerInfo->AppendLayerInfoObject( absoluteFilePath ) )
		{
			ERR_WCC( TXT( "Could not update layer info for resaved layer %ls." ), file->GetDepotPath( ).AsChar( ) );
			return false;
		}

		delete layerInfo;
	}*/

	return true;
}

void CResaveCommandlet::ResaveLogToFile( String someEntry, String someReason ) const
{	
	CDirectory* direct = GDepot->FindPath( TXT("engine\\") );
	String logFilename = String(TXT("customResave_ErrorLog.csv"));

	if( direct ) GFileManager->SaveStringToFile( direct->GetAbsolutePath() + logFilename, someEntry + TXT(";") + someReason + TXT("\n"), true );
}

Uint32 CResaveCommandlet::GetFileDataCRC( const String& fileAbsolutePath ) const
{
	CDependencyFileData::Header header;

	Red::TScopedPtr< IFile > srcFile( GFileManager->CreateFileReader( fileAbsolutePath, FOF_AbsolutePath ) );
	if( !srcFile || srcFile->GetSize( ) <= sizeof( header ) )
	{
		return 0;
	}

	Red::MemoryZero( &header, sizeof( header ) );
	srcFile->Serialize( &header, sizeof( header ) );

	if ( header.m_magic != CDependencyLoader::FILE_MAGIC )
	{
		return 0;
	}

	Uint64 dataSize = srcFile->GetSize( ) - sizeof( header );
	Uint8* buffer = new Uint8 [ dataSize ];
	srcFile->Serialize( buffer, dataSize );

	Red::System::CRC32 crcCalculator;
	Uint32 fileCRC = crcCalculator.Calculate( buffer, static_cast< Uint32 >( dataSize ) );

	delete [ ] buffer;

	return fileCRC;
}
