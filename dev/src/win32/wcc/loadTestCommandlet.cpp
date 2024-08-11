#include "build.h"

#include "wccVersionControl.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/dependencyLinker.h"
#include "../../common/core/dependencyFileTables.h"
#include "../../common/core/depot.h"
#include "../../common/core/garbageCollector.h"

class CLoadTestCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CLoadTestCommandlet, ICommandlet, 0 );

public:
	CLoadTestCommandlet();

	virtual const Char* GetOneLiner( ) const;
	virtual void PrintHelp( ) const;
	virtual bool Execute( const CommandletOptions& options );

private:

	void ScanDirectory( CDirectory* dir  );
	void BuildExtensionList();
	void CheckFiles();
	void CheckFile( CDiskFile* file );

	TDynArray< CDiskFile* >		m_filesToCheck;
	THashSet< String >			m_validExtensions;
	Uint32						m_failedFiles;
};

DEFINE_SIMPLE_RTTI_CLASS( CLoadTestCommandlet, ICommandlet );
IMPLEMENT_ENGINE_CLASS( CLoadTestCommandlet );

CLoadTestCommandlet::CLoadTestCommandlet()
{
	m_commandletName = CName( TXT("loadtest") );
}

const Char* CLoadTestCommandlet::GetOneLiner( ) const
{
	return TXT( "Attempts to load all resources to check for errors." );
}

void CLoadTestCommandlet::PrintHelp( ) const
{
	LOG_WCC( TXT( "Usage:" ) );
	LOG_WCC( TXT( "  loadtest") );
}

bool CLoadTestCommandlet::Execute( const CommandletOptions& options )
{
	// Do not skip CRC verification
	extern Bool GSkipCRCVerification;
	GSkipCRCVerification = false;

	// Build set with resource extensions
	BuildExtensionList();

	// Find files
	LOG_WCC( TXT("Scanning depot...") );
	ScanDirectory( GDepot );
	LOG_WCC( TXT("Found %i resource files"), (int)m_filesToCheck.Size() );

	// Check the files
	m_failedFiles = 0;
	CheckFiles();

	// Report failed files
	if ( m_failedFiles > 0 )
	{
		ERR_WCC( TXT("Failed to load %i of %i resources"), (int)m_failedFiles, (int)m_filesToCheck.Size() );
		return false;
	}

	return true;
}

void CLoadTestCommandlet::ScanDirectory( CDirectory* dir  )
{
	// Scan files in current directory.
	const TFiles& files = dir->GetFiles( );
	for( auto it = files.Begin( ); it != files.End( ); ++it )
	{
		CDiskFile* file = *it;
		CFilePath filePath( file->GetDepotPath( ) );
		if ( m_validExtensions.Exist( filePath.GetExtension( ) ) )
		{
			m_filesToCheck.PushBack( file );
		}
	}

	// Scan subdirectories.
	for( CDirectory* child : dir->GetDirectories( ) )
	{
		ScanDirectory( child );
	}
}

void CLoadTestCommandlet::BuildExtensionList( )
{
	// Get all known resource extensions.
	TDynArray< CClass* > resourceClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< CResource >( ), resourceClasses );
	for( Uint32 i = 0; i < resourceClasses.Size( ); ++i )
	{
		m_validExtensions.Insert( resourceClasses[ i ]->GetDefaultObject< CResource >( )->GetExtension( ) );
	}

	// Remove extensions we don't care about
	m_validExtensions.Erase( TXT("navmesh") );
	m_validExtensions.Erase( TXT("w2comm") );
}

void CLoadTestCommandlet::CheckFiles()
{
	for ( auto it = m_filesToCheck.Begin(); it != m_filesToCheck.End(); ++it )
	{
		CheckFile( *it );
	}
}

void CLoadTestCommandlet::CheckFile( CDiskFile* file )
{
	// Try to load the resource
	LOG_WCC( TXT("Attempting to load '%ls'..."), file->GetDepotPath().AsChar() );
	CResource* resource = file->Load();
	if ( resource == nullptr )
	{
		ERR_WCC( TXT("[resource load failed] '%ls'"), file->GetDepotPath().AsChar() );
		++m_failedFiles;
	}

	// Resource loaded, unload it
	file->Unload();

	// Process stuff so we don't run out of memory.
	SEvents::GetInstance( ).ProcessPendingEvens( );
	SGarbageCollector::GetInstance( ).CollectNow( );
}
