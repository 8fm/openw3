/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/core/filePath.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/depot.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/objectDiscardList.h"
#include "../../common/core/dataError.h"

#include "../../common/gpuApiUtils/gpuApiMemory.h"

#include "cookDataBase.h"
#include "reportWriter.h"

/// Data validator - checks if resources are not fucked up
class CDataValidatorCommandlet : public ICommandlet, public IDataErrorSystem
{
	DECLARE_ENGINE_CLASS( CDataValidatorCommandlet, ICommandlet, 0 );

public:
	CDataValidatorCommandlet();
	~CDataValidatorCommandlet();

	// ICommandlet interface
	virtual const Char* GetOneLiner() const { return TXT("Validate resources"); }
	virtual bool Execute( const CommandletOptions& options );
	virtual void PrintHelp() const;

private:
	struct ErrorInfo
	{
		EDataErrorSeverity		m_severity;
		String					m_category;
		String					m_text;

		ErrorInfo( const EDataErrorSeverity severity, const Char* category, const Char* txt )
			: m_severity( severity )
			, m_category( category )
			, m_text( txt )
		{}
	};

	struct FileInfo
	{
		const CDiskFile*		m_diskFile;
		TDynArray< ErrorInfo >	m_errors;
		Uint32					m_numErrors[ DES_Count ];

		FileInfo( const CDiskFile* file )
			: m_diskFile( file )
		{
			for ( Uint32 i=0; i<DES_Count; ++i )
			{
				m_numErrors[ i ] = 0;
			}
		}
	};

	CCookerDataBase		m_db;
	Bool				m_useDB;
	String				m_outReportPath;

	typedef THashMap< const CDiskFile*, FileInfo* >		TFileMap;
	TFileMap			m_files;

	Uint32				m_numInputFiles;
	Uint32				m_numErrors[ DES_Count ];

	CDiskFile*			m_currentFile;

	FileInfo* MapFile( const CDiskFile* file );

	void ProcessFile( CDiskFile* file );
	void ReportError( CDiskFile* file, EDataErrorSeverity severity, const String& category, const String& txt );
	void WriteReport();
	void WriteErrors( EDataErrorSeverity severity, const Char* fileName, const AnsiChar* name );

	Uint32 CountResources( EDataErrorSeverity level ) const;

	// IDataErrorSystem
	virtual void Flush() {}
	virtual void GetCurrentCatchedFilteredForResource( TSortedArray< SDataError >& , const TDynArray< String >& ) {}
	virtual void ClearCatchedForResource( const TDynArray< String >& ) {}
	virtual void FillErrorInfoFromPerforce( SDataError& error ) {}
	virtual void ReportError( SAssertInfo& info, const Char* message, ... );
	virtual void RegisterListener( IDataErrorListener* listener ) {}
	virtual void UnregisterListener( IDataErrorListener* listener ) {}
};

BEGIN_CLASS_RTTI( CDataValidatorCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CDataValidatorCommandlet );

CDataValidatorCommandlet::CDataValidatorCommandlet()
{
	m_commandletName = CName( TXT("validate") );
}

CDataValidatorCommandlet::~CDataValidatorCommandlet()
{
}

#pragma optimize ("",off)

bool CDataValidatorCommandlet::Execute( const CommandletOptions& options )
{
	// cleanup lists
	m_useDB = false;
	m_files.ClearPtr();
	Red::MemoryZero( &m_numErrors, sizeof(m_numErrors) );

	// gather the list of files to process
	TDynArray< CDiskFile* > filesToProcess;
	if ( options.HasOption( TXT("all") ) )
	{
		// get all files
		GDepot->CollectFiles( filesToProcess, String::EMPTY, true, false );
	}
	else if ( options.HasOption( TXT("db") ) )
	{
		String cookDBPath;
		options.GetSingleOptionValue( TXT("db"), cookDBPath );

		// load the cook db
		if ( !m_db.LoadFromFile( cookDBPath ) )
		{
			ERR_WCC( TXT("Failed to load cooked DB from file '%ls'"), cookDBPath.AsChar() );
			return false;
		}

		// get the files using the entries of cook.db
		m_db.QueryResourcesUnfiltered( [&]( const CCookerDataBase& db, const CCookerResourceEntry& entry ) 
			{
				const String depotPath = ANSI_TO_UNICODE( entry.GetFilePath().AsChar() );
				CDiskFile* file = GDepot->FindFile( depotPath );
				if ( file != nullptr )
				{
					filesToProcess.PushBack( file );
				}
				else
				{
					ERR_WCC( TXT("Cooked file '%ls' not found in depot - no validation will be done"), depotPath.AsChar() );
				}

				return true;
			}
		);

		// db is valid, use it
		m_useDB = true;
	}
	else if ( options.HasOption( TXT("file") ) )
	{
		auto fileList = options.GetOptionValues( TXT("file") );
		for ( auto it = fileList.Begin(); it != fileList.End(); ++it )
		{
			CDiskFile* file = GDepot->FindFile( *it );
			if ( file != nullptr )
			{
				filesToProcess.PushBack( file );
			}
			else
			{
				ERR_WCC( TXT("File '%ls' not found in depot - no validation will be done"), file->GetDepotPath().AsChar() );
			}
		}
	}

	// output path
	if ( !options.GetSingleOptionValue( TXT("outdir"), m_outReportPath ) )
	{
		ERR_WCC( TXT("Expected output path to be specified") );
		return false;
	}

	// any files to validate?
	LOG_WCC( TXT("Found %d files to validate"), filesToProcess.Size() );
	if ( filesToProcess.Empty() )
	{
		WARN_WCC( TXT("No files to validate, existing") );
		return true;
	}

	// Process the file validation
	m_numInputFiles = filesToProcess.Size();
	for ( Uint32 i=0; i<filesToProcess.Size(); ++i )
	{
		CDiskFile* file = filesToProcess[i];

		LOG_WCC( TXT("Status: [%d/%d] Processing '%ls'..."), i, filesToProcess.Size(), file->GetDepotPath().AsChar() );
		ProcessFile( file );

		// Check memory consumption
		{
			const Uint64 limit = 3000;
			const Uint64 total = Memory::GetTotalBytesAllocated() >> 20;
			if ( total > limit )
			{
				LOG_WCC( TXT("Running emergency GC...") );
				SGarbageCollector::GetInstance().CollectNow();
				GObjectsDiscardList->ProcessList(true);

				const Uint64 rest = Memory::GetTotalBytesAllocated() >> 20;
				LOG_WCC( TXT("%d MB saved"), total - rest );
			}
		}

		if ( i > 10000 )
			break;
	}

	// Stats
	LOG_WCC( TXT("Errors found in %d resources:"), m_files.Size() );
	LOG_WCC( TXT("  %d uber errors"), m_numErrors[ DES_Uber ] );
	LOG_WCC( TXT("  %d major errors"), m_numErrors[ DES_Major ] );
	LOG_WCC( TXT("  %d minor errors"), m_numErrors[ DES_Minor ] );
	LOG_WCC( TXT("  %d tiny errors"), m_numErrors[ DES_Tiny ] );

	// Generate report
	WriteReport();
	return true;
}

#pragma optimize ("",on)


void CDataValidatorCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Usage:") );
	LOG_WCC( TXT("  validate [-db=<cook.db file> -file=<filename> -all] -outdir=<dir>") );
	LOG_WCC( TXT("Exclusive options:") );
	LOG_WCC( TXT("  -db=<cook.db>      - Validate files from given cook") );
	LOG_WCC( TXT("  -file=<filename>   - Validate single file") );
	LOG_WCC( TXT("  -all               - Validate all files in depot (not recommended)") );
}

CDataValidatorCommandlet::FileInfo* CDataValidatorCommandlet::MapFile( const CDiskFile* file )
{
	FileInfo* info = nullptr;

	m_files.Find( file, info );
	if ( !info )
	{
		info = new FileInfo( file );
		m_files.Insert( file, info );
	}

	return info;	
}

void CDataValidatorCommandlet::ReportError( CDiskFile* file, EDataErrorSeverity severity, const String& category, const String& txt )
{
	CDataValidatorCommandlet::FileInfo* info = MapFile( file );
	if ( info )
	{
		info->m_numErrors[ severity ] += 1;
		m_numErrors[ severity ] += 1;
		new ( info->m_errors ) ErrorInfo( severity, category.AsChar(), txt.AsChar() );
	}
}

void CDataValidatorCommandlet::ProcessFile( CDiskFile* file )
{
	// try to open the file and validate the version number & header
	Red::TScopedPtr< IFile > reader( file->CreateReader() );
	if ( !reader )
	{
		ReportError( file, DES_Uber, TXT("System"), TXT("Unable to create file reader") );
		return;
	}

	// empty file - what the fuck ?
	if ( !reader->GetSize() )
	{
		// the "txt" and "csv" files CAN be empty - the rest - not		
		if ( file->GetFileName().EndsWith( TXT("csv") ) || 
			file->GetFileName().EndsWith( TXT("txt") ) )
			return;

		ReportError( file, DES_Uber, TXT("System"), TXT("File has zero size on disk") );
		return;
	}

	// try to load magic number to see if it's a resource
	Uint32 magic = 0;
	*reader << magic;
	
	// not a resource - exit
	if ( magic != CDependencyLoader::FILE_MAGIC )
	{
		return;
	}

	// rewind the file for LoadTables
	reader->Seek( 0 );

	// try to load the tables
	CDependencyLoader loader( *reader, file );
	if ( !loader.LoadTables() )
	{
		ReportError( file, DES_Uber, TXT("System"), TXT("File has invalid header") );
		return;
	}

	/*// validate tables
	const String tableError = loader.ValidateTables();
	if ( !tableError.Empty() )
	{
		ReportError( file, DES_Uber, TXT("System"), String::Printf( TXT("File has corrupted data: '%ls'"), tableError.AsChar() ) );
		return;
	}*/

	// rewind the file for FileDependency
	reader->Seek( 0 );

	// make sure the hard dependencies exist
	TDynArray< FileDependency > hardDependencies;
	loader.LoadDependencies( hardDependencies, true );
	for ( const FileDependency& dep : hardDependencies )
	{
		// when checking cooked builds use the cook DB to check for the file
		if ( m_useDB )
		{
			// is file in the cook DB?
			CCookerDataBase::TCookerDataBaseID id = m_db.GetFileEntry( dep.m_depotPath );
			if ( id == CCookerDataBase::NO_ENTRY )
			{
				if ( dep.m_isSoftDepdencency )
				{
					ReportError( file, DES_Major, TXT("Dependencies"), String::Printf( TXT("Cooked file has missing soft import '%ls', class '%ls'"), 
						dep.m_depotPath.AsChar(), dep.m_resourceClass ? dep.m_resourceClass.AsChar() : TXT("null") ) );
				}
				else
				{
					ReportError( file, DES_Uber, TXT("Dependencies"), String::Printf( TXT("Cooked file has missing hard import '%ls', class '%ls'"), 
						dep.m_depotPath.AsChar(), dep.m_resourceClass ? dep.m_resourceClass.AsChar() : TXT("null") ) );
				}
			}
		}
		else
		{
			// check only hard dependencies
			if ( !dep.m_isSoftDepdencency )
			{
				CDiskFile* depFile = GDepot->FindFileUseLinks( dep.m_depotPath.AsChar(), 0 );
				if ( !depFile )
				{
					ReportError( file, DES_Uber, TXT("Dependencies"), String::Printf( TXT("File has missing import '%ls', class '%ls'"), 
						dep.m_depotPath.AsChar(), dep.m_resourceClass ? dep.m_resourceClass.AsChar() : TXT("null") ) );
				}
			}
		}
	}

	// load the resource
	THandle< CResource > resource = file->Load();
	if ( !resource )
	{
		if ( file->GetFileName().EndsWith( TXT("veglib") ) )
			return;

		ReportError( file, DES_Uber, TXT("System"), TXT("File loading has failed") );
		return;
	}

	// capture resource errors
	{
		IDataErrorSystem* oldDataErrors = GDataError;
		GDataError = this;
		m_currentFile = file;

		// protect resource
		resource->AddToRootSet();

		// normal errors
		resource->OnCheckDataErrors();

		// full validation mode
		resource->OnFullValidation( String::EMPTY );

		// release protection
		resource->RemoveFromRootSet();

		m_currentFile = nullptr;
		GDataError = oldDataErrors;
	}

	// unload the resource
	file->Unload();	
}

Uint32 CDataValidatorCommandlet::CountResources( EDataErrorSeverity level ) const
{
	Uint32 numResources = 0;
	for ( auto it = m_files.Begin(); it != m_files.End(); ++it )
	{
		if ( (*it).m_second->m_numErrors[ level ] )
			numResources += 1;
	}

	return numResources;
}

void CDataValidatorCommandlet::WriteReport()
{
	// create output path
	GFileManager->CreatePath( m_outReportPath );

	// overview
	{
		CHTMLReportDocument page( m_outReportPath + TXT("index.html"), "Validation overview" );
		page << "<h1>";
		page << "Validation report";
		page << "</h1>";

		// count
		page << StringAnsi::Printf( "<p>Analyzed %d resource files</p>", m_numInputFiles ).AsChar();

		// status
		const Bool hasFatalErrors = (m_numErrors[ DES_Uber ] > 0);
		const Bool hasErrors = (m_numErrors[ DES_Major ] > 0) && (m_numErrors[ DES_Minor ] > 0);
		if ( hasFatalErrors )
		{
			page << "<h2><font color=red>";
			page << "Fatal errors found";
			page << "</font></h2>";
		}
		else if ( hasErrors )
		{
			page << "<h2><font color=yellow>";
			page << "Errors found";
			page << "</font></h2>";
		}
		else
		{
			page << "<h2><font color=green>";
			page << "No errors found";
			page << "</font></h2>";
		}

		// print the table only if we have anything
		if ( m_numErrors[ DES_Minor ] || m_numErrors[ DES_Major ] || m_numErrors[ DES_Tiny ] || m_numErrors[ DES_Uber ] )
		{
			// per-error type stats
			CHTMLTable table( page );
			table.Column( "Type", 100 );
			table.Column( "Count", 50 );
			table.Column( "Resources", 50 );
		
			if ( m_numErrors[ DES_Uber ] )
			{
				table.Row();
				table.Cell( "<a href=\"level0.html\">Uber</a>" );
				table.Cellf( "%d", m_numErrors[ DES_Uber ] );
				table.Cellf( "%d", CountResources( DES_Uber ) );
			}

			if ( m_numErrors[ DES_Major ] )
			{
				table.Row();
				table.Cell( "<a href=\"level1.html\">Major</a>" );
				table.Cellf( "%d", m_numErrors[ DES_Major ] );
				table.Cellf( "%d", CountResources( DES_Major ) );
			}

			if ( m_numErrors[ DES_Minor ] )
			{
				table.Row();
				table.Cell( "<a href=\"level2.html\">Minor</a>" );
				table.Cellf( "%d", m_numErrors[ DES_Minor ] );
				table.Cellf( "%d", CountResources( DES_Minor ) );
			}

			if ( m_numErrors[ DES_Tiny ] )
			{
				table.Row();
				table.Cell( "<a href=\"level3.html\">Warnings</a>" );
				table.Cellf( "%d", m_numErrors[ DES_Tiny ] );
				table.Cellf( "%d", CountResources( DES_Tiny ) );
			}

			// write error report - per class
			if ( m_numErrors[ DES_Uber ] )
				WriteErrors( DES_Uber, TXT("level0.html"), "Fatal errors");
			if ( m_numErrors[ DES_Major ] )
				WriteErrors( DES_Major, TXT("level1.html"), "Major errors");
			if ( m_numErrors[ DES_Minor ] )
				WriteErrors( DES_Minor, TXT("level2.html"), "Minor errors");
			if ( m_numErrors[ DES_Tiny ] )
				WriteErrors( DES_Tiny, TXT("level3.html"), "Warnings");
		}
	}
}

void CDataValidatorCommandlet::WriteErrors( EDataErrorSeverity severity, const Char* fileName, const AnsiChar* name )
{
	CHTMLReportDocument page( m_outReportPath + fileName, name );
	page << "<h1>";
	page << name;
	page << "</h1>";

	for ( auto it = m_files.Begin(); it != m_files.End(); ++it )
	{
		const FileInfo* info = (*it).m_second;

		if ( !info->m_numErrors[ severity ] )
			continue;

		page << "<h2>";
		page << UNICODE_TO_ANSI( info->m_diskFile->GetDepotPath().AsChar() );
		page << "</h2>";

		// find categories
		TDynArray< String > categories;
		for ( const ErrorInfo& errorInfo : info->m_errors )
		{
			if ( errorInfo.m_severity == severity )
			{
				categories.PushBackUnique( errorInfo.m_category );
			}
		}

		// show errors for each category
		for ( const String& catName : categories )
		{
			page << "<h3>";
			if ( catName.Empty() )
				page << "Generic";
			else
				page << UNICODE_TO_ANSI( catName.AsChar() );
			page << "</h3>";

			{
				page << "<ul>";

				for ( const ErrorInfo& errorInfo : info->m_errors )
				{
					if ( errorInfo.m_severity == severity && errorInfo.m_category == catName )
					{
						page << "<li>";
						page << UNICODE_TO_ANSI( errorInfo.m_text.AsChar() );
						page << "</li>";
					}
				}

				page << "</ul>";
			}
		}

		page << "<hr/>";

	}
}

void CDataValidatorCommandlet::ReportError( SAssertInfo& info, const Char* message, ... )
{
	Char infoBuffer[ 2048 ];
	va_list args;

	va_start( args, message );
	Red::VSNPrintF( infoBuffer, ARRAY_COUNT_U32( infoBuffer ), message, args );
	va_end( args );

	ReportError( m_currentFile, info.m_severity, info.m_category, infoBuffer );
}
