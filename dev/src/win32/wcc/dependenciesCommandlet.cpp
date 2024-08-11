/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/core/filePath.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/depot.h"
#include "../../common/core/dependencyCache.h"
#include "../../common/core/objectGC.h"
#include "../../common/core/resourceid.h"

#include "../../common/engine/world.h"
#include "../../common/engine/layer.h"
#include "../../common/engine/entityTemplate.h"
#include "../../games/r4/r4GameResource.h"

#include "cookDataBase.h"
#include "reportWriter.h"

/// Dependency cache builder
class CDependencyCacheCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CDependencyCacheCommandlet, ICommandlet, 0 );

public:
	CDependencyCacheCommandlet();
	~CDependencyCacheCommandlet();

	// ICommandlet interface
	virtual const Char* GetOneLiner() const { return TXT("Build dependency cache file"); }
	virtual bool Execute( const CommandletOptions& options );
	virtual void PrintHelp() const;

private:
	Bool ProcessFile( CDiskFile* file, const Bool verbose, Uint32& outVersion );

	struct DirectoryInfo;

	struct FileInfo
	{
		String				m_depotPath;	// depot path (always set)
		Bool				m_visited;		// was file visited
		Bool				m_marker;		// recursive dependency marker

		DirectoryInfo*		m_directory;	// directory this file is in
		Uint32				m_reportID;		// ID of the file in the report system

		Bool				m_isProcessed;	// file was already processed
		Bool				m_isValid;		// file is valid
		Bool				m_isResource;	// resource file
		Bool				m_isRoot;		// root resource (world, game definition)

		Uint32				m_numHardRefs;	// number of times this file was referenced as a hard dependency
		Uint32				m_numSoftRefs;	// number of times this file was referenced as a soft dependency
		Uint32				m_numInplaceRefs;	// number of times this file was referenced as a inplace dependency

		Int32				m_fileIndex;	// index in the final dependency map (only valid files)
		Int32				m_dlcIndex;		// assigned DLC index (0 for original game, 1-16 for normal DLC, 17 for EP1, 18 for EP2, -1 for errors)

		Int32				m_totalUseCount;

		typedef TDynArray< FileInfo* >	TDependencies;
		TDependencies		m_hardDependencies;
		TDependencies		m_softDependencies;
		TDependencies		m_inplaceDependencies;

		TDependencies		m_hardUsers;
		TDependencies		m_softUsers;
		TDependencies		m_inplaceUsers;

		FileInfo( const String& filePath );
		~FileInfo();
	};

	struct DependencyInfo
	{
		StringAnsi		m_name;
		const FileInfo*	m_file;
		Bool			m_hard;
	};

	struct Histogram
	{
		String		m_ext;
		Uint32		m_count;
		Uint64		m_size;

		Histogram()
			: m_count(0)
			, m_size(0)
		{}

		Histogram( const String& ext )
			: m_ext( ext.ToLower() )
			, m_count( 1 )
			, m_size( 0 )
		{};
	};

	struct DirectoryInfo
	{
		String				m_name;
		DirectoryInfo*		m_parent;

		Uint32				m_numFiles;

		typedef TDynArray< DirectoryInfo* > 	TChildren;
		TChildren			m_children;

		typedef TDynArray< FileInfo* > TFiles;
		TFiles				m_files;

		DirectoryInfo(); // root
		DirectoryInfo( DirectoryInfo* parent, const String& name );
		~DirectoryInfo();

		void CountFile();

		StringAnsi GetPath() const;

		DirectoryInfo* MapChild( const String& childName );

		void WriteReport( const String& outputPath );
	};

	class IErrorReporter
	{
	public:
		virtual ~IErrorReporter() {};
		virtual void OnError( const Char* txt, ... ) = 0;
	};

	class CLogReporter : public IErrorReporter
	{
	public:
		CLogReporter( const Char* optionalDumpFilePath );
		virtual ~CLogReporter();

		virtual void OnError( const Char* txt, ... ) override;

	private:
		FILE*		m_file;
	};

	typedef TDynArray< FileInfo* >					TFileEntries;
	typedef THashMap< const CDiskFile*, FileInfo* >	TFileMap;  // valid files
	typedef THashMap< String, const FileInfo* >		TFileStringMap;  // valid files
	typedef THashMap< String, FileInfo* >			TDBEntryMap;  // valid db entries
	typedef THashMap< String, FileInfo* >			TFileInvalidMap; // invalid files (in general)

	typedef THashMap< String, String >			TFileMapping;

	typedef TDynArray< Histogram >				THistogram; // no point in using hash map here
	

	TFileEntries		m_files;
	TFileMap			m_fileMap;
	TFileStringMap		m_filePathMap;
	TDBEntryMap			m_dbEntryMap;
	TFileInvalidMap		m_invalidFiles;
	TFileMapping		m_fileMapping;
	THistogram			m_histogram;
	Bool				m_saveReport;

	Uint32				m_numValidDependencies;
	Uint32				m_numMovedDependencies;
	Uint32				m_numInvalidDependencies;
	Uint32				m_numResourceFiles;

	Uint64				m_totalDataSize;
	Uint64				m_totalResourceSize;

	String				m_worldFileExtension;
	String				m_layerFileExtension;
	String				m_gameFileExtension;

	DirectoryInfo*		m_root;

	DirectoryInfo* MapDirectory( const String& depotPath );

	FileInfo* MapInvalidFile( const String& depotPath );
	FileInfo* MapValidFile( const CDiskFile* diskFile );
	FileInfo* MapValidDBEntry( const CCookerResourceEntry* dbEntry );

	void CountFileType( const String& ext );
	void CountFileSize( const String& ext, const Uint64 size );

	void MapLink( const String& sourcePath, const String& targetPath );

	Bool ProcessFile( const CDiskFile* file, const String& ext, FileInfo* info );
	void ProcessDBEntry( const CCookerDataBase& dataBase, CCookerDataBase::TCookerDataBaseID entryId, const String& ext, FileInfo* info );

	void GetFileDependenciesInfo( const FileInfo* file, TDynArray< DependencyInfo >& infos);
	void SaveFileDepList( const String& outMainPath, const FileInfo* file, const StringAnsi& rootPath );
	void SaveFileList( const String& outMainPath, const StringAnsi& fileNameBase, const Uint32 colIndex, const StringAnsi& title, const TDynArray< FileInfo* >& files );
	void SaveFilesWithMissingReferences( const String& outMainPath, const String& filename, const StringAnsi& title, TDynArray< FileInfo* >& filesReferencingMissingFiles, THashMap < String, TDynArray< String > >& missingFilesLists );

	Bool ResaveFilesUsingDepot();
	Bool ResaveFilesUsingCookDB( const TList<String>& cookDBFilePaths, const Uint32 groupIndex );

	Bool CheckDLCDependencies( IErrorReporter& reporter );
	Bool CheckRecursiveDependencies( IErrorReporter& reporter );
	Bool CheckMissingEntityTemplates( IErrorReporter& reporter );
	Bool CheckTemplateStreamingData( IErrorReporter& reporter );
	Bool CheckLayerStreamingData( IErrorReporter& reporter );

	void CheckEntityForStaledStreamingData( IErrorReporter& reporter, const FileInfo* baseFile, const CResource* parentResource, const CEntity* entity ) const;

	void WalkFileTree( FileInfo* file, TDynArray< FileInfo* >& fileStack, Uint32& numRecursiveFiles, IErrorReporter& reporter ) const;

	Bool SaveReport( const String& outAbsolutePath );
	Bool SaveDepCache( const String& outAbsolutePath );
};

BEGIN_CLASS_RTTI( CDependencyCacheCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CDependencyCacheCommandlet );

//---

CDependencyCacheCommandlet::CLogReporter::CLogReporter( const Char* optionalDumpFilePath )
	: m_file( nullptr )
{
	if ( optionalDumpFilePath && *optionalDumpFilePath )
	{
		m_file = _wfopen( optionalDumpFilePath, L"w" );
	}
}

CDependencyCacheCommandlet::CLogReporter::~CLogReporter()
{
	if ( m_file )
	{
		fclose( m_file );
		m_file = nullptr;
	}
}

void CDependencyCacheCommandlet::CLogReporter::OnError( const Char* txt, ... )
{
	Char buf[ 8192 ];
	va_list args;

	va_start( args, txt );
	vswprintf_s( buf, ARRAY_COUNT(buf), txt, args );
	va_end( args );

	ERR_WCC( TXT("%ls"), buf );

	if ( m_file )
	{
		fwprintf( m_file, L"%ls\n", buf );
		fflush( m_file );
	}
}


//---

namespace Helper
{
	static Int32 ExtractDLCIndex( const String& path ) 
	{
		// part of the game
		if ( !path.BeginsWith( TXT("dlc\\") ) )
			return 0;

		// get the DLC name
		CFilePath filePath( path );
		if ( filePath.GetNumberOfDirectories() < 2 )
			return -1; // invalid DLC

		Char dlcName[ 64 ];
		filePath.GetDirectory( 1, dlcName, ARRAY_COUNT(dlcName) );

		if ( 0 == Red::StringCompare( dlcName, TXT("dlc1") ) ) return 1;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc2") ) ) return 2;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc3") ) ) return 3;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc4") ) ) return 4;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc5") ) ) return 5;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc6") ) ) return 6;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc7") ) ) return 7;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc8") ) ) return 8;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc9") ) ) return 9;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc10") ) ) return 10;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc11") ) ) return 11;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc12") ) ) return 12;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc13") ) ) return 13;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc14") ) ) return 14;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc15") ) ) return 15;
		if ( 0 == Red::StringCompare( dlcName, TXT("dlc16") ) ) return 16;

		if ( 0 == Red::StringCompare( dlcName, TXT("ep1") ) ) return 20;
		if ( 0 == Red::StringCompare( dlcName, TXT("bob") ) ) return 21;

		return 0; // not an interesting DLC directory name (ignore the file)
	}
}

CDependencyCacheCommandlet::FileInfo::FileInfo( const String& filePath )
	: m_depotPath( filePath )
	, m_isProcessed( false )
	, m_isValid( false )
	, m_isResource( false )
	, m_isRoot( false )
	, m_fileIndex( -1 )
	, m_numHardRefs( 0 )
	, m_numSoftRefs( 0 )
	, m_reportID( 0 )
	, m_directory( nullptr )
	, m_totalUseCount( 0 )
	, m_visited( false )
{
	m_dlcIndex = Helper::ExtractDLCIndex( filePath );
}

CDependencyCacheCommandlet::FileInfo::~FileInfo()
{

}

//---

CDependencyCacheCommandlet::DirectoryInfo::DirectoryInfo()
	: m_parent( nullptr )
	, m_numFiles( 0 )
	, m_name( TXT(".") )
{
}

CDependencyCacheCommandlet::DirectoryInfo::DirectoryInfo( DirectoryInfo* parent, const String& name )
	: m_parent( parent )
	, m_name( name )
	, m_numFiles( 0 )
{
	m_parent->m_children.PushBack( this );
}

CDependencyCacheCommandlet::DirectoryInfo::~DirectoryInfo()
{
	m_parent = nullptr;
	m_children.ClearPtr();
}

void CDependencyCacheCommandlet::DirectoryInfo::CountFile()
{
	m_numFiles += 1;

	if ( m_parent )
		m_parent->CountFile();
}

StringAnsi CDependencyCacheCommandlet::DirectoryInfo::GetPath() const
{
	StringAnsi ret;
	if ( m_parent )
		ret = m_parent->GetPath();

	ret += UNICODE_TO_ANSI( m_name.AsChar() );
	ret += "\\";
	return ret;
}

CDependencyCacheCommandlet::DirectoryInfo* CDependencyCacheCommandlet::DirectoryInfo::MapChild( const String& childName )
{
	for ( DirectoryInfo* child : m_children )
	{
		if ( child->m_name == childName )
		{
			return child;
		}
	}

	return new DirectoryInfo( this, childName );
}

void CDependencyCacheCommandlet::DirectoryInfo::WriteReport( const String& outputPath )
{
	CHTMLReportDocument page( outputPath + TXT("index.html"), "File list" );
	page << "<h1>Content of \"";
	page << GetPath().AsChar();
	page << "\"</h1>";

	// sub directories
	page << "<p><ul>";
	for ( DirectoryInfo* child : m_children )
	{
		page << "<li>";

		page << StringAnsi::Printf( "<a href=\"%s\\index.html\">", UNICODE_TO_ANSI( child->m_name.AsChar() ) ).AsChar();
		page << StringAnsi::Printf( "%s (%d files)", UNICODE_TO_ANSI( child->m_name.AsChar() ), child->m_numFiles ).AsChar();
		page << "</a>";

		page << "</li>";
	}
	page << "</ul></p>";

	if ( m_files.Empty() )
	{
		page << "No files in directory";
	}

	// files
	for ( FileInfo* info : m_files )
	{
		const CFilePath filePath( info->m_depotPath );

		page << "<hr/>";

		// compute depot depth and construct prefix path
		const Uint32 depth = filePath.GetNumberOfDirectories();
		StringAnsi pathToRoot;
		for ( Uint32 i=0; i<depth; ++i )
		{
			pathToRoot += "..\\";
		}

		page << StringAnsi::Printf( "<a id=\"F%d\">", info->m_reportID ).AsChar();
		page << StringAnsi::Printf( "<h2>%s</h2>", UNICODE_TO_ANSI( filePath.GetFileNameWithExt().AsChar() ) ).AsChar();
		page << "</a><br>";

		// per resource info
		{
			CHTMLNode p(page, "p");

			// info
			if ( info->m_numHardRefs || info->m_numSoftRefs )
			{
				p << StringAnsi::Printf( "File is referenced %d times (%d soft, %d hard)<br>", 
					info->m_numHardRefs + info->m_numSoftRefs, info->m_numSoftRefs, info->m_numHardRefs ).AsChar();
			}
			else
			{
				p << "File is not referenced by any other known resource<br>";
			}

			// gather file dependencies
			struct DepInfo
			{
				StringAnsi		m_path;
				Uint32			m_id;
				DirectoryInfo*	m_dir;
				Bool			m_hard;
				Bool			m_valid;
			};

			TDynArray< DepInfo >	deps;
			for ( const FileInfo* dep : info->m_hardDependencies )
			{
				DepInfo newDep;
				newDep.m_hard = true;
				newDep.m_id = dep->m_reportID;
				newDep.m_dir = dep->m_directory;
				newDep.m_valid = dep->m_isValid;
				newDep.m_path = UNICODE_TO_ANSI( dep->m_depotPath.AsChar() );
				deps.PushBack(newDep);
			}
			for ( const FileInfo* dep : info->m_softDependencies )
			{
				DepInfo newDep;
				newDep.m_hard = false;
				newDep.m_id = dep->m_reportID;
				newDep.m_dir = dep->m_directory;
				newDep.m_valid = dep->m_isValid;
				newDep.m_path = UNICODE_TO_ANSI( dep->m_depotPath.AsChar() );
				deps.PushBack(newDep);
			}

			// deps
			if ( !info->m_isValid )
			{
				p << "<font color=red>File is missing</font><br>";
			}
			else if ( deps.Empty() )
			{
				p << "File has no known dependencies<br>";
			}
			else
			{
				CHTMLTable table(p);
				table.Column( "Path", 600 );
				table.Column( "Type", 70 );
				table.Column( "Status", 70 );

				Sort( deps.Begin(), deps.End(), []( const DepInfo& a, const DepInfo& b ) { return a.m_path < b.m_path; } );

				for ( const DepInfo& info : deps )
				{
					table.Row();
					table.Cellf( "<a href=\"%s%sindex.html#F%d\">%s</a>", 
						pathToRoot.AsChar(), info.m_dir->GetPath().AsChar(), info.m_id, info.m_path.AsChar() );
					table.Cell( info.m_hard ? "Hard" : "Soft" );
					table.Cell( info.m_valid ? "OK" : "<font color=red><b>MISSING</b></font>" );
				}
			}
		}
	}

	// create the reports for the directories
	for ( DirectoryInfo* child : m_children )
	{
		child->WriteReport( outputPath + child->m_name + TXT("\\") );
	}
}
//----

CDependencyCacheCommandlet::CDependencyCacheCommandlet()
{
	m_commandletName = CName( TXT("dependencies") );

	m_worldFileExtension = ResourceExtension< CWorld >();
	m_layerFileExtension = ResourceExtension< CLayer >();
	m_gameFileExtension = ResourceExtension< CWitcherGameResource >();
}

CDependencyCacheCommandlet::~CDependencyCacheCommandlet()
{
	m_files.ClearPtr();
	m_fileMap.Clear();
	m_dbEntryMap.Clear();
}

CDependencyCacheCommandlet::DirectoryInfo* CDependencyCacheCommandlet::MapDirectory( const String& depotPath )
{
	const CFilePath filePath( depotPath );

	DirectoryInfo* cur = m_root;
	for ( Uint32 i=0; i<filePath.GetNumberOfDirectories(); ++i )
	{
		Char directoryName[512];
		if ( !filePath.GetDirectory( i, directoryName, ARRAY_COUNT(directoryName) ) )
			continue;

		cur = cur->MapChild( directoryName );
	}

	return cur;
}

CDependencyCacheCommandlet::FileInfo* CDependencyCacheCommandlet::MapValidFile( const CDiskFile* diskFile )
{
	FileInfo* ret = nullptr;
	m_fileMap.Find( diskFile, ret );
	if ( !ret )
	{
		const String depotPath = diskFile->GetDepotPath().ToLower();

		ret = new FileInfo( depotPath );
		ret->m_isProcessed = false;
		ret->m_isValid = true;
		ret->m_reportID = m_files.Size() + 1;

		if ( m_saveReport )
		{
			ret->m_directory = MapDirectory( depotPath );
			ret->m_directory->m_files.PushBack( ret );
			ret->m_directory->CountFile();
		}

		m_fileMap.Set( diskFile, ret );
		m_files.PushBack( ret );
	}

	return ret;
}

CDependencyCacheCommandlet::FileInfo* CDependencyCacheCommandlet::MapValidDBEntry( const CCookerResourceEntry* dbEntry )
{
	FileInfo* ret = nullptr;
	const String depotPath = String::Printf( TXT( "%hs" ), dbEntry->GetFilePath().AsChar( ) );
	m_dbEntryMap.Find( depotPath, ret );
	if ( !ret )
	{
		ret = new FileInfo( depotPath );
		ret->m_isProcessed = false;
		ret->m_isValid = true;
		ret->m_reportID = m_files.Size() + 1;

		if ( m_saveReport )
		{
			ret->m_directory = MapDirectory( depotPath );
			ret->m_directory->m_files.PushBack( ret );
			ret->m_directory->CountFile();
		}

		m_dbEntryMap.Set( depotPath, ret );
		m_files.PushBack( ret );
	}

	return ret;
}

CDependencyCacheCommandlet::FileInfo* CDependencyCacheCommandlet::MapInvalidFile( const String& depotPath )
{
	String temp;
	const String& safeDepotPath = CFilePath::ConformPath( depotPath, temp );

	FileInfo* ret = nullptr;
	m_invalidFiles.Find( safeDepotPath, ret );
	if ( !ret )
	{
		ret = new FileInfo( depotPath );
		ret->m_isProcessed = true;
		ret->m_isValid = false;
		ret->m_reportID = m_files.Size() + 1;

		if ( m_saveReport )
		{
			ret->m_directory = MapDirectory( depotPath );
			ret->m_directory->m_files.PushBack( ret );
			ret->m_directory->CountFile();
		}

		m_invalidFiles.Set( safeDepotPath, ret );
		m_files.PushBack( ret );
	}

	return ret;
}

void CDependencyCacheCommandlet::CountFileType( const String& ext )
{
	for ( Histogram& h : m_histogram )
	{
		if ( h.m_ext == ext )
		{
			h.m_count += 1;
			return;
		}
	}

	m_histogram.PushBack( Histogram( ext ) );
}

void CDependencyCacheCommandlet::CountFileSize( const String& ext, const Uint64 size )
{
	for ( Histogram& h : m_histogram )
	{
		if ( h.m_ext == ext )
		{
			h.m_size += size;
			return;
		}
	}
}

void CDependencyCacheCommandlet::MapLink( const String& sourcePath, const String& targetPath )
{
	String temp1, temp2;
	const String& safeSourcePath = CFilePath::ConformPath( sourcePath, temp1 );
	const String& safeTargetPath = CFilePath::ConformPath( targetPath, temp2 );

	m_fileMapping.Set( safeSourcePath, safeTargetPath );
}

bool CDependencyCacheCommandlet::Execute( const CommandletOptions& options )
{
	// reset
	m_numValidDependencies = 0;
	m_numMovedDependencies = 0;
	m_numInvalidDependencies = 0;
	m_numResourceFiles = 0;
	m_totalDataSize = 0;
	m_totalResourceSize = 0;
	m_histogram.Clear();
	m_fileMap.Clear();
	m_dbEntryMap.Clear();
	m_fileMapping.Clear();
	m_files.ClearPtr();
	m_saveReport = false;

	// create the root directory
	m_root = new DirectoryInfo();

	// will we be saving report data ?
	String reportOutputPath;
	if ( options.GetSingleOptionValue( TXT("report"), reportOutputPath ) )
	{
		m_saveReport = true;
	}

	// Do we have fatal errors
	Bool validData = true;
	Bool cookDBused = false;
	// Resave using entries in cook.db.
	Bool allowFatalErrors = false;
	if( options.HasOption( TXT( "db" ),  TXT( "cookdb" ) ) )
	{
		const TList<String>& cookDBFilePaths = options.GetOptionValues(TXT( "db" ),  TXT( "cookdb" ) );
		const Uint32 groupIndexToProcess = cookDBFilePaths.Size() - 1; // always process LAST DB
		validData &= ResaveFilesUsingCookDB( cookDBFilePaths, groupIndexToProcess );
		cookDBused = true;
	}
	// Resave using depot files directly.
	else
	{
		validData &= ResaveFilesUsingDepot( );
		allowFatalErrors = true;
	}

	// Extra report file
	String extraReportFilePath;
	options.GetSingleOptionValue( TXT("dump"), extraReportFilePath );

	// create file map
	m_filePathMap.Reserve( m_files.Size() );
	for ( const auto* file : m_files )
		m_filePathMap.Insert( file->m_depotPath, file );

	CLogReporter reporter( extraReportFilePath.AsChar() );

	// check DLC dependencies
	{
		CTimeCounter timer;

		LOG_WCC( TXT("Checking for invalid DLC dependencies...") );

		if ( !CheckDLCDependencies( reporter ) )
		{
			ERR_WCC( TXT("!!! Depot contains recursive dependencies !!!") );
			validData = false;
		}

		LOG_WCC( TXT("DLC dependencies checked in %1.2fs"), timer.GetTimePeriod() );
	}

	// check recursive dependencies
	{
		CTimeCounter timer;

		LOG_WCC( TXT("Checking for recursive dependencies...") );

		if ( !CheckRecursiveDependencies( reporter ) )
		{
			ERR_WCC( TXT("!!! Depot contains recursive dependencies !!!") );
			validData = false;
		}

		LOG_WCC( TXT("Recursive dependencies checked in %1.2fs"), timer.GetTimePeriod() );
	}

	// check for missing entity templates on layers
	{
		CTimeCounter timer;

		LOG_WCC( TXT("Checking for missing entity templates...") );

		if ( !CheckMissingEntityTemplates( reporter ) )
		{
			ERR_WCC( TXT("!!! Depot contains missing entity template dependencies !!!") );
			validData = false;
		}

		LOG_WCC( TXT("Layers dependencies checked in %1.2fs"), timer.GetTimePeriod() );
	}

	// check layer streaming buffers
	if ( cookDBused == false )
	{
		CTimeCounter timer;

		LOG_WCC( TXT("Checking layer streaming problems...") );

		if ( !CheckLayerStreamingData( reporter ) )
		{
			ERR_WCC( TXT("!!! Depot contains fatal problems within layers !!!") );
			validData = false;
		}

		LOG_WCC( TXT("Layer templates checked in %1.2fs"), timer.GetTimePeriod() );		
	}

	// check entity template streaming buffers
	if ( cookDBused == false )
	{
		CTimeCounter timer;

		LOG_WCC( TXT("Checking entity template streaming problems...") );

		if ( !CheckTemplateStreamingData( reporter ) )
		{
			ERR_WCC( TXT("!!! Depot contains fatal problems within entity templates !!!") );
			validData = false;
		}

		LOG_WCC( TXT("Entity templates checked in %1.2fs"), timer.GetTimePeriod() );		
	}

	// save cache
	String cacheOutputPath;
	if ( options.GetSingleOptionValue( TXT("out"), cacheOutputPath ) )
	{
		if ( !validData && allowFatalErrors )
		{
			ERR_WCC( TXT("Files contain fatal errors. Dependency data base will not be generated. ") );
			return false;
		}

		if ( !SaveDepCache( cacheOutputPath ) )
			return false;
	}

	// save report
	if ( !reportOutputPath.Empty() )
	{
		if ( !SaveReport( reportOutputPath ) )
			return false;
	}

	return true;
}

Bool CDependencyCacheCommandlet::ResaveFilesUsingDepot( )
{
	Bool ret = true;

	// scan depot
	TDynArray< CDiskFile* > allFiles;
	{
		CTimeCounter timer;

		LOG_WCC( TXT("Enumerating depot...") );

		GDepot->CollectFiles( allFiles, String::EMPTY, true, false ); // get everything
		LOG_WCC( TXT("Depot enumerated in %1.2fs"), timer.GetTimePeriod() );
		LOG_WCC( TXT("Found %d files in depot"), allFiles.Size() );
	}

	// process files
	{
		CTimeCounter timer;

		LOG_WCC( TXT("Analyzing dependencies...") );

		Int32 lastPrc = -1;
		for ( Uint32 i=0; i<allFiles.Size(); ++i )
		{
			CDiskFile* file = allFiles[i];
			const String ext = StringHelpers::GetFileExtension( file->GetDepotPath() ).ToLower();

			// update status - don't update every file because it's SLOOOOW
			const Int32 prc = i * 100 / allFiles.Size();
			if ( prc != lastPrc )
			{
				LOG_WCC( TXT("Status: Analyzing files %d%%..."), prc );
				lastPrc = prc;
			}

			// count file types
			CountFileType( ext );

			// link file ? if so, skip it
			if ( ext == TXT("link") )
				continue;

			// all the files in the depot are considered valid so create a valid mapping for the file
			FileInfo* info = MapValidFile( file );
			RED_FATAL_ASSERT( info != nullptr, "File mapping not created" );

			// process the file
			ret &= ProcessFile( file, ext, info );
		}

		LOG_WCC( TXT("Dependencies analyzed in %1.2fs"), timer.GetTimePeriod() );
	}

	// return merged status
	return ret;
}

Bool CDependencyCacheCommandlet::ResaveFilesUsingCookDB( const TList<String>& cookDBFilePaths, const Uint32 groupIndex )
{
	CCookerDataBase dataBase;
	dataBase.LoadFromFile( cookDBFilePaths, true /*includeAdditionalFiles*/ );

	TDynArray< CCookerResourceEntry > dataBaseFiles;
	dataBase.GetFileEntries( dataBaseFiles );

	CTimeCounter timer;

	LOG_WCC( TXT("Analyzing cook DB dependencies...") );

	Int32 lastPrc = -1;
	for( Uint32 i = 0; i < dataBaseFiles.Size(); ++i )
	{
		const CCookerResourceEntry& entry = dataBaseFiles[ i ];
		if ( entry.GetGroupIndex() == groupIndex )
		{
			const String ext = StringHelpers::GetFileExtension( String::Printf( TXT( "%hs" ), entry.GetFilePath().AsChar( ) ) ).ToLower( );

			// update status - don't update every file because it's SLOOOOW
			const Int32 prc = i * 100 / dataBaseFiles.Size();
			if ( prc != lastPrc )
			{
				LOG_WCC( TXT("Status: Analyzing files %d%%..."), prc );
				lastPrc = prc;
			}

			// count file types
			CountFileType( ext );

			// all the files in the depot are considered valid so create a valid mapping for the file
			FileInfo* info = MapValidDBEntry( &entry );
			RED_FATAL_ASSERT( info != nullptr, "File mapping not created" );

			// process the file (I don't like passing in the database, but meh..).
			ProcessDBEntry( dataBase, dataBase.GetFileEntry( entry.GetFilePath() ), ext, info );
		}
	}

	LOG_WCC( TXT("Cook DB dependencies analyzed in %1.2fs"), timer.GetTimePeriod() );
	return true;
}

Bool CDependencyCacheCommandlet::CheckDLCDependencies( IErrorReporter& reporter )
{
	// process unvisited files
	Uint32 numInvalidDeps = 0;
	for ( FileInfo* info : m_files )
	{
		if ( info->m_dlcIndex == -1 )
		{
			reporter.OnError( TXT("INVALID DLC;%ls;;;"), info->m_depotPath.AsChar() );
			continue;
		}

		// scan for hard deps
		for ( const FileInfo* depInfo : info->m_hardDependencies )
		{
			// file from main game using a DLC file
			if ( info->m_dlcIndex == 0 && depInfo->m_dlcIndex > 0 )
			{
				reporter.OnError( TXT("MAIN USING DLC;%ls;%ls;;"), 
					info->m_depotPath.AsChar(), depInfo->m_depotPath.AsChar() );
				numInvalidDeps += 1;
			}

			// file from DLC using another DLC
			else if ( info->m_dlcIndex > 0 && depInfo->m_dlcIndex > 0 && depInfo->m_dlcIndex != info->m_dlcIndex )
			{
				reporter.OnError( TXT("DLC USING DLC;%ls;%ls;;"), 
					info->m_depotPath.AsChar(), depInfo->m_depotPath.AsChar() );
				numInvalidDeps += 1;
			}

		}

		// scan for soft deps
		for ( const FileInfo* depInfo : info->m_softDependencies )
		{
			// file from DLC using another DLC
			if ( info->m_dlcIndex > 0 && depInfo->m_dlcIndex > 0 && depInfo->m_dlcIndex != info->m_dlcIndex )
			{
				reporter.OnError( TXT("DLC USING DLC;%ls;%ls;;"), 
					info->m_depotPath.AsChar(), depInfo->m_depotPath.AsChar() );
				numInvalidDeps += 1;
			}

		}
	}

	// return success only if we don't have recursive files
	return (numInvalidDeps == 0);
}

Bool CDependencyCacheCommandlet::CheckRecursiveDependencies( IErrorReporter& reporter )
{
	// process unvisited files
	Uint32 numRecursiveFiles = 0;
	for ( FileInfo* info : m_files )
	{
		if ( info->m_visited )
			continue;

		TDynArray< FileInfo* > stack;
		WalkFileTree( info, stack, numRecursiveFiles, reporter );
	}

	// return success only if we don't have recursive files
	return (numRecursiveFiles == 0);
}

void CDependencyCacheCommandlet::CheckEntityForStaledStreamingData( IErrorReporter& reporter, const FileInfo* baseFile, const CResource* parentResource, const CEntity* entity ) const
{
	const void* streamingData = nullptr;
	Uint32 streamingDataSize = 0;

	if ( entity->GetLocalStreamedComponentDataBuffer().GetSize() == 0 )
	{
		if ( entity->HasFlag( ESF_Streamed ) )
		{
/*			reporter.OnError( TXT("!!! ENTITY STREAMING PROBLEM !!! Entity '%ls' in '%ls' is streamable but has no stremaing buffer. Resave required."), 
				entity->GetName().AsChar(), parentResource->GetDepotPath().AsChar() );*/
			reporter.OnError( TXT("INVALID FLAGS;%ls;%ls;;"), 
				parentResource->GetDepotPath().AsChar(), entity->GetName().AsChar() );
		}

		if ( !entity->GetOldStreamingBuffer(0).Empty() || !entity->GetOldStreamingBuffer(1).Empty() || !entity->GetOldStreamingBuffer(2).Empty() )
		{
			/*reporter.OnError( TXT("!!! ENTITY STREAMING PROBLEM !!! Entity '%ls' in '%ls' has streaming data in the old format. Resave required."), 
				entity->GetName().AsChar(), parentResource->GetDepotPath().AsChar() );*/
			reporter.OnError( TXT("OLD FORMAT;%ls;%ls;;"), 
				parentResource->GetDepotPath().AsChar(), entity->GetName().AsChar() );
		}

		streamingData = entity->GetOldStreamingBuffer(0).Data();
		streamingDataSize = (Uint32) entity->GetOldStreamingBuffer(0).DataSize();
	}
	else
	{
		streamingData = entity->GetLocalStreamedComponentDataBuffer().GetData();
		streamingDataSize = entity->GetLocalStreamedComponentDataBuffer().GetSize();

		if ( entity->HasFlag( ESF_Streamed ) )
		{
			WARN_WCC( TXT("Entity '%ls' in '%ls' has unnecessary streaming data"), 
				entity->GetName().AsChar(), parentResource->GetDepotPath().AsChar() );
		}
	}

	// check dependencies of the streamed data
	if ( streamingData )
	{
		CMemoryFileReader reader( (const Uint8*)streamingData, streamingDataSize, 0 );
		CDependencyLoader loader( reader, nullptr );
		if ( !loader.LoadTables() )
		{
			/*reporter.OnError( TXT("!!! ENTITY STREAMING PROBLEM !!! Entity '%ls' in '%ls' has corrupted streaming data buffer. Resave required."), 
				entity->GetName().AsChar(), parentResource->GetDepotPath().AsChar() );*/
			reporter.OnError( TXT("CORRUPTED DATA;%ls;%ls;;"), 
				parentResource->GetDepotPath().AsChar(), entity->GetName().AsChar() );
		}
		else
		{
			reader.Seek(0);

			TDynArray< FileDependency > deps;
			loader.LoadDependencies( deps, true );

			for ( Uint32 i=0; i<deps.Size(); ++i )
			{
				String depPath = deps[i].m_depotPath;

				if ( nullptr == GDepot->FindFile( depPath ) )
				{
					CDiskFile* otherFile = GDepot->FindFileUseLinks( depPath, 0 );
					if ( otherFile )
					{
						/*reporter.OnError( TXT("!!! ENTITY STREAMING PROBLEM !!! Entity '%ls' in '%ls' streaming references file '%ls' that was moved to '%ls'. Resave required."), 
							entity->GetName().AsChar(), parentResource->GetDepotPath().AsChar(),
							depPath.AsChar(), otherFile->GetDepotPath().AsChar() );*/
						reporter.OnError( TXT("MOVED ASSET;%ls;%ls;%ls;%ls"), 
							parentResource->GetDepotPath().AsChar(), entity->GetName().AsChar(),
							depPath.AsChar(), otherFile->GetDepotPath().AsChar() );

						depPath = otherFile->GetDepotPath();
					}
					else
					{
						/*reporter.OnError( TXT("!!! ENTITY STREAMING PROBLEM !!! Entity '%ls' in '%ls' streaming references file '%ls' that no longer exists in the depot'. Fix the entity."), 
							entity->GetName().AsChar(), parentResource->GetDepotPath().AsChar(), depPath.AsChar() );*/
						reporter.OnError( TXT("MISSING ASSET;%ls;%ls;%ls;"), 
							parentResource->GetDepotPath().AsChar(), entity->GetName().AsChar(),
							depPath.AsChar() );
					}
				}

				// DLC data check
				if ( baseFile->m_dlcIndex == 0 ) // main
				{
					const FileInfo* depFile = nullptr;
					m_filePathMap.Find( depPath, depFile );

					if ( depFile && depFile->m_dlcIndex != 0 )
					{
						reporter.OnError( TXT("MAIN ENTITY USES DLC;%ls;%ls;%ls;"), 
							parentResource->GetDepotPath().AsChar(), entity->GetName().AsChar(),
							depPath.AsChar() );
					}
				}
			}
		}
	}
}

Bool CDependencyCacheCommandlet::CheckLayerStreamingData( IErrorReporter& reporter )
{
	// process unvisited files
	Uint32 numProcessed = 0;
	Int32 lastPrc = -1;
	for ( Uint32 fileIndex=0; fileIndex<m_files.Size(); ++fileIndex )
	{
		FileInfo* file = m_files[fileIndex];
		if ( !file->m_isValid )
			continue;
		// update status - don't update every file because it's SLOOOOW
		const Int32 prc = fileIndex * 100 / m_files.Size();
		if ( prc != lastPrc )
		{
			LOG_WCC( TXT("Status: Checking streaming data in layers %d%%..."), prc );
			lastPrc = prc;
		}

		// we are interested only in layers
		const CFilePath filePath( file->m_depotPath );
		if ( filePath.GetExtension() == ResourceExtension< CLayer >() )
		{
			// W3 hack: only check production levels for now
			Bool isValidLevel = false;
			isValidLevel |= file->m_depotPath.BeginsWith( TXT("levels\\island_of_mist\\") );
			isValidLevel |= file->m_depotPath.BeginsWith( TXT("levels\\kaer_morhen\\") );
			isValidLevel |= file->m_depotPath.BeginsWith( TXT("levels\\novigrad\\") );
			isValidLevel |= file->m_depotPath.BeginsWith( TXT("levels\\prolog_village\\") );
			isValidLevel |= file->m_depotPath.BeginsWith( TXT("levels\\prolog_village_winter\\") );
			isValidLevel |= file->m_depotPath.BeginsWith( TXT("levels\\skellige\\") );
			isValidLevel |= file->m_depotPath.BeginsWith( TXT("levels\\the_spiral\\") );
			isValidLevel |= file->m_depotPath.BeginsWith( TXT("levels\\wyzima_castle\\") );
			if ( !isValidLevel )
				continue;

			// load the template
			THandle< CLayer > layer = LoadResource< CLayer >( file->m_depotPath );
			if ( !layer )
			{
				reporter.OnError( TXT("!!! CORRUPTED LAYER !!! Failed to load layer from '%ls'"), 
					file->m_depotPath.AsChar() );
				continue;
			}

			// count progress
			numProcessed += 1;

			// process the entities
			for ( CEntity* entity : layer->GetEntities( ))
			{
				if ( !entity->GetEntityTemplate() )
				{
					CheckEntityForStaledStreamingData( reporter, file, layer.Get(), entity );
				}
			}
		}

		// cleanup
		if ( numProcessed == 1000 )
		{
			GObjectGC->CollectNow();
			numProcessed = 0;
		}
	}

	return true;
}

Bool CDependencyCacheCommandlet::CheckTemplateStreamingData( IErrorReporter& reporter )
{
	// process unvisited files
	Uint32 numProcessed = 0;
	Int32 lastPrc = -1;
	for ( Uint32 fileIndex=0; fileIndex<m_files.Size(); ++fileIndex )
	{
		FileInfo* file = m_files[fileIndex];
		if ( !file->m_isValid )
			continue;

		// update status - don't update every file because it's SLOOOOW
		const Int32 prc = fileIndex * 100 / m_files.Size();
		if ( prc != lastPrc )
		{
			LOG_WCC( TXT("Status: Checking streaming data in templates %d%%..."), prc );
			lastPrc = prc;
		}

		// we are interested only in layers
		const CFilePath filePath( file->m_depotPath );
		if ( filePath.GetExtension() == ResourceExtension< CEntityTemplate >() )
		{
			// W3 hack: skip templates from QA folder
			if ( file->m_depotPath.BeginsWith( TXT("qa\\") ) )
				continue;

			// load the template
			THandle< CEntityTemplate > et = LoadResource< CEntityTemplate >( file->m_depotPath );
			if ( !et || !et->GetEntityObject() )
			{
				reporter.OnError( TXT("!!! CORRUPTED ENTITY TEMPLATE !!! Failed to load entity from '%ls'"), 
					file->m_depotPath.AsChar() );
				continue;
			}
			
			// count progress
			numProcessed += 1;

			// process the template
			CheckEntityForStaledStreamingData( reporter, file, et.Get(), et->GetEntityObject() );
		}

		// cleanup
		if ( numProcessed == 1000 )
		{
			GObjectGC->CollectNow();
			numProcessed = 0;
		}
	}

	// no fatal errors
	return true;
}

Bool CDependencyCacheCommandlet::CheckMissingEntityTemplates( IErrorReporter& reporter )
{
	Bool status = true;

	// process unvisited files
	for ( FileInfo* file : m_files )
	{
		if ( !file->m_isValid )
			continue;

		// we are interested only in layers
		const CFilePath filePath( file->m_depotPath );
		const Bool isLayer = (filePath.GetExtension() == ResourceExtension< CLayer >());
		const Bool isEntityTemplate = (filePath.GetExtension() == ResourceExtension< CEntityTemplate >());
		if ( !isLayer && !isEntityTemplate )
			continue;

		// we are not interested in the QA crap
		const auto dirs = filePath.GetDirectories();
		if ( dirs.Empty() || dirs[0] == TXT("qa") )
			continue;

		// resource name (layer or entity template)
		const Char* resourceName = TXT("RESOURCE");
		if ( isEntityTemplate ) resourceName = TXT("ENTITY TEMPLATE");
		else if ( isLayer ) resourceName = TXT("LAYER");

		// make sure a layer has no dependencies to a entity template that is not there
		for ( auto dep : file->m_hardDependencies )
		{
			// valid dependencies are ok :)
			if ( dep->m_isValid )
				continue;

			// get the extension of the dependency
			const CFilePath filePathDep( dep->m_depotPath );
			if ( filePathDep.GetExtension() == ResourceExtension< CEntityTemplate >() )
			{
				// it's a missing entity template
				reporter.OnError( TXT("!!! FATAL MISSING %ls DEPENDENCY !!! Missing entity template '%ls' referenced from '%ls'"), resourceName,
					dep->m_depotPath.AsChar(), file->m_depotPath.AsChar() );

				status = false;
			}
			else
			{
				// it's a missing entity template
				reporter.OnError( TXT("!!! MISSING %ls DEPENDENCY !!! Missing resource '%ls' referenced from '%ls'"), resourceName, 
					dep->m_depotPath.AsChar(), file->m_depotPath.AsChar() );
			}
		}
	}

	// return status (false if there are missing entity templates)
	return status;
}

void CDependencyCacheCommandlet::WalkFileTree( FileInfo* file, TDynArray< FileInfo* >& fileStack, Uint32& numRecursiveFiles, IErrorReporter& reporter ) const
{
	// file was already visited in the past
	if ( file->m_visited )
		return;

	// WOW ! A Recursive dependency!!
	if ( file->m_marker )
	{
		reporter.OnError( TXT("Recursive dependency to file '%ls' via files: "), file->m_depotPath.AsChar() );
		for ( Int32 i=(fileStack.Size()-1); i>=0; --i )
		{
			reporter.OnError( TXT("  [%d]: %ls"), i, fileStack[i]->m_depotPath.AsChar() );
		}
		numRecursiveFiles += 1;
		return;
	}

	// mark file in the current dependency chain
	file->m_marker = true;
	fileStack.PushBack( file );

	// visit hard dependencies
	for ( Uint32 i=0; i<file->m_hardDependencies.Size(); ++i )
	{
		WalkFileTree( file->m_hardDependencies[i], fileStack, numRecursiveFiles, reporter );
	}

	// pop from list
	file->m_marker = false;
	fileStack.PopBack();

	// mark as visited
	file->m_visited = true;
}

Bool CDependencyCacheCommandlet::SaveReport( const String& reportOutputPath )
{
	const Double oneGB = 1024.0f * 1024.0f * 1024.0f;
	const Double oneMB = 1024.0f * 1024.0f;

	/*
	LOG_WCC( TXT("Extension breakdown per count:") );
	Sort( m_histogram.Begin(), m_histogram.End(), [](const Histogram& a, const Histogram& b) { return a.m_count > b.m_count; } );
	for ( const Histogram& h : m_histogram )
	{
		LOG_WCC( TXT("   %7d: %ls"), h.m_count, h.m_ext.AsChar() );
	}

	LOG_WCC( TXT("Extension breakdown per size:") );
	Sort( m_histogram.Begin(), m_histogram.End(), [](const Histogram& a, const Histogram& b) { return a.m_size > b.m_size; } );
	for ( const Histogram& h : m_histogram )
	{
		LOG_WCC( TXT("   %1.3f MB: %ls"), (Double)h.m_size / oneMB, h.m_ext.AsChar() );
	}*/

	// general report
	{ CHTMLReportDocument page( reportOutputPath + TXT("index.html"), "Depot dump" );
		page << "<h1>Depot content dump</h1>";

		// common stats
		{ CHTMLNode p(page, "p");
			p << "Size information:<br>";
			p->Writef( "<b>%1.3f</b> GB of data<br>", (Double)m_totalDataSize / oneGB );
			p->Writef( "<b>%1.3f</b> GB of resources<br>", (Double)m_totalResourceSize / oneGB );
		}

		// common stats
		{ CHTMLNode p(page, "p");
			p << "Dependency breakdown:<br>";
			p->Writef( "<b>%d</b> valid<br>", m_numValidDependencies  );
			p->Writef( "<b>%d</b> invalid<br>", m_numInvalidDependencies  );
			p->Writef( "<b>%d</b> moved<br>", m_numMovedDependencies  );
		}

		// files
		{ CHTMLNode p(page, "p");
			p << "File breakdown:<br>";
			p->Writef( "<b>%d</b> files<br>", m_files.Size() );
			p->Writef( "<b>%d</b> resource files<br>", m_numResourceFiles );
			p->Writef( "<b>%d</b> missing files<br>", m_invalidFiles.Size() );
			p->Writef( "<b>%d</b> valid links<br>", m_fileMapping.Size() );
		}

		// tables
		{ CHTMLNode p(page, "p");
			p << "Additional tables:<br>";
			p << "<a href=\"all\\index.html\">All files</a><br>";
			p << "<a href=\"missing0.html\">Missing Files</a><br>";
			p << "<a href=\"filesReferencingMissingFiles.html\">Files referencing missing files</a><br>";
			p << "<a href=\"notused0.html\">Not Used Files</a><br>";
		}
	}

	// generate the resource tree
	{
		const String depotOutputPath = reportOutputPath + TXT("all\\");
		m_root->WriteReport( depotOutputPath );
	}

	// missing file reports
	{
		for ( auto it = m_invalidFiles.Begin(); it != m_invalidFiles.End(); ++it )
		{
			const FileInfo* info = (*it).m_second;
			SaveFileDepList( reportOutputPath + TXT("references\\"), info, "..\\all\\" );
		}
	}

	// missing files tables
	{
		// get raw list
		TDynArray< FileInfo* > files;
		files.Reserve( m_invalidFiles.Size() );
		for ( auto it = m_invalidFiles.Begin(); it != m_invalidFiles.End(); ++it )
		{
			files.PushBack( (*it).m_second );
		}

		// sort by name
		Sort( files.Begin(), files.End(), []( FileInfo* a, FileInfo* b ) { return a->m_depotPath < b->m_depotPath; } );
		SaveFileList( reportOutputPath, "missing", 0, "Missing depot files", files );

		// sort by hard refs
		Sort( files.Begin(), files.End(), []( FileInfo* a, FileInfo* b ) { return a->m_numHardRefs > b->m_numHardRefs; } );
		SaveFileList( reportOutputPath, "missing", 1, "Missing depot files", files );

		// sort by soft refs
		Sort( files.Begin(), files.End(), []( FileInfo* a, FileInfo* b ) { return a->m_numSoftRefs > b->m_numSoftRefs; } );
		SaveFileList( reportOutputPath, "missing", 2, "Missing depot files", files );

		// gather all files that references missing files
		TDynArray< FileInfo* > filesReferencingMissingFiles;
		THashMap< String, TDynArray< String > > missingFilesLists;
		for ( FileInfo* file : files )
		{
			for ( FileInfo* refFile : file->m_hardUsers )
			{
				filesReferencingMissingFiles.PushBackUnique( refFile );
				missingFilesLists.GetRef( refFile->m_depotPath ).PushBackUnique( file->m_depotPath );
			}

			for ( FileInfo* refFile : file->m_softUsers )
			{
				filesReferencingMissingFiles.PushBackUnique( refFile );
				missingFilesLists.GetRef( refFile->m_depotPath ).PushBackUnique( file->m_depotPath );
			}
		}
		// sort by name
		Sort( filesReferencingMissingFiles.Begin(), filesReferencingMissingFiles.End(), []( FileInfo* a, FileInfo* b ) { return a->m_depotPath < b->m_depotPath; } );
		SaveFilesWithMissingReferences( reportOutputPath, TXT( "filesReferencingMissingFiles" ), "Files referencing missing files", filesReferencingMissingFiles, missingFilesLists );
		
	}

	return true;
}

void CDependencyCacheCommandlet::SaveFileList( const String& outMainPath, const StringAnsi& fileNameBase, const Uint32 colIndex, const StringAnsi& title, const TDynArray< FileInfo* >& files )
{
	const String fullFileName = String::Printf( TXT("%ls%ls%d.html"), 
		outMainPath.AsChar(), 
		ANSI_TO_UNICODE( fileNameBase.AsChar() ),
		colIndex );

	{ CHTMLReportDocument page( fullFileName, title.AsChar() );
		page << "<h1>";
		page << title.AsChar();
		page << "</h1>";

		page << StringAnsi::Printf( "There are %d files in the list<br>", files.Size() ).AsChar();

		// column names with links
		const StringAnsi col0 = StringAnsi::Printf( "<a href=\"%s0.html\">Path</a>", fileNameBase.AsChar(), colIndex );
		const StringAnsi col1 = StringAnsi::Printf( "<a href=\"%s1.html\">Hard Refs</a>", fileNameBase.AsChar(), colIndex );
		const StringAnsi col2 = StringAnsi::Printf( "<a href=\"%s2.html\">Soft Refs</a>", fileNameBase.AsChar(), colIndex );

		// columns
		{ CHTMLTable table( page );
			table.Column( col0.AsChar(), 400 );
			table.Column( col1.AsChar(), 40 );
			table.Column( col2.AsChar(), 40 );
			table.Column( "References List", 400 );

			// show files
			for ( const FileInfo* info : files )
			{
				table.Row();
				table.Cellf( "<a href=\"references\\report%d.html\">%s</a>", info->m_reportID, UNICODE_TO_ANSI( info->m_depotPath.AsChar() ) );
				table.Cellf( "%d", info->m_numHardRefs );
				table.Cellf( "%d", info->m_numSoftRefs );

				TDynArray< DependencyInfo > refsInfos;
				GetFileDependenciesInfo( info, refsInfos );

				page << "<td><ul>";
				for ( const DependencyInfo& info : refsInfos )
				{
					page << StringAnsi::Printf( "<li>%s</li>", info.m_name.AsChar() ).AsChar();
				}
				page << "</ul></td>";
			}
		}
	}
}

void CDependencyCacheCommandlet::SaveFilesWithMissingReferences( const String& outMainPath, const String& filename, const StringAnsi& title, 
																TDynArray< FileInfo* >& filesReferencingMissingFiles, THashMap < String, TDynArray< String > >& missingFilesLists )
{
	const String fullFileName = String::Printf( TXT("%ls%ls.html"), 
		outMainPath.AsChar(), filename.AsChar() );

	{ CHTMLReportDocument page( fullFileName, title.AsChar() );
		page << "<h1>";
		page << title.AsChar();
		page << "</h1>";

		page << StringAnsi::Printf( "There are %d files in the list<br>", filesReferencingMissingFiles.Size() ).AsChar();

		// column names
		const StringAnsi col0 = "Path";
		const StringAnsi col1 = "Count";
		const StringAnsi col2 = "Missing files referenced";

		// columns
		{ CHTMLTable table( page );
			table.Column( col0.AsChar(), 600 );
			table.Column( col1.AsChar(), 40 );
			table.Column( col2.AsChar(), 600 );

			// show files
			Sort( filesReferencingMissingFiles.Begin(), filesReferencingMissingFiles.End(), []( FileInfo* a, FileInfo* b ) { return a->m_depotPath < b->m_depotPath; } );

			for ( FileInfo* info : filesReferencingMissingFiles )
			{
				TDynArray< String >& missingFiles = missingFilesLists.GetRef(info->m_depotPath );

				table.Row();
				table.Cellf( "<a href=\"all\\%sindex.html#F%d\">%s</a>", 
					info->m_directory->GetPath().AsChar(),
					info->m_reportID,
					UNICODE_TO_ANSI( info->m_depotPath.AsChar() ) );
				table.Cellf( "%d", missingFiles.Size() );
				
				page << "<td><ul>";
				for ( const String& fileName : missingFiles )
				{
					page << StringAnsi::Printf( "<li>%s</li>", UNICODE_TO_ANSI( fileName.AsChar() ) ).AsChar();
				}
				page << "</ul></td>";

			}
		}
	}
}

void CDependencyCacheCommandlet::GetFileDependenciesInfo( const FileInfo* file, TDynArray< DependencyInfo >& infos )
{
	for ( const FileInfo* refFile : file->m_hardUsers )
	{	
		DependencyInfo newInfo;
		newInfo.m_file = refFile;
		newInfo.m_hard = true;
		newInfo.m_name = UNICODE_TO_ANSI( refFile->m_depotPath.AsChar() );
		infos.PushBack(newInfo);
	}
	for ( const FileInfo* refFile : file->m_softUsers )
	{
		DependencyInfo newInfo;
		newInfo.m_file = refFile;
		newInfo.m_hard = false;
		newInfo.m_name = UNICODE_TO_ANSI( refFile->m_depotPath.AsChar() );
		infos.PushBack(newInfo);
	}
}

void CDependencyCacheCommandlet::SaveFileDepList( const String& outMainPath, const FileInfo* file, const StringAnsi& rootPath )
{
	const String filePath = String::Printf( TXT("report%d.html"), file->m_reportID );

	CHTMLReportDocument page( outMainPath + filePath, "References" );
	page << "<h1>";
	page << "Files referencing the file \"";
	page << UNICODE_TO_ANSI( file->m_depotPath.AsChar() );
	page << "\"</h1>";

	page << StringAnsi::Printf( "This file has %d hard and %d soft references<br>", file->m_hardUsers.Size(), file->m_softUsers.Size() ).AsChar();


	TDynArray< DependencyInfo > infos;
	GetFileDependenciesInfo( file, infos );

	Sort( infos.Begin(), infos.End(), []( const DependencyInfo& a, const DependencyInfo& b ) { return a.m_name < b.m_name; } );

	// columns
	{ CHTMLTable table( page );
		table.Column( "Path", 700 );
		table.Column( "Type", 90 );

		// show files
		for ( const DependencyInfo& info : infos )
		{
			table.Row();
			table.Cellf( "<a href=\"%s%sindex.html#F%d\">%s</a>", 
				rootPath.AsChar(),
				info.m_file->m_directory->GetPath().AsChar(),
				info.m_file->m_reportID,
				info.m_name.AsChar() );
			table.Cell( info.m_hard ? "Hard" : "Soft" );
		}
	}
}

static const Char* CSV_EXT = TXT("csv");

Bool ProcessCSV( const CDiskFile* csvFile, TDynArray<const CDiskFile*>& outputList )
{
	// CVS files that we are adding here may contain links to other resources (recursive)
	Bool status = true;
	
	// assume it's a file from depot
	String absoluteFilePath = GFileManager->GetDataDirectory();
	absoluteFilePath += csvFile->GetDepotPath().AsChar();

	// load the CSV
	THandle<C2dArray> ar( C2dArray::CreateFromString(absoluteFilePath) );
	if (ar)
	{
		Uint32 numDeps = 0;

		// look for the resource paths (naive but works 100% of cases right now)
		const Uint32 numCols = ar->GetNumberOfColumns();
		const Uint32 numRows = ar->GetNumberOfRows();
		for (Uint32 i=0; i<numRows; ++i)
		{
			for (Uint32 j=0; j<numCols; ++j)
			{
				const String& cellText = ar->GetValueRef(j,i);
				if ( !cellText.Empty() && cellText.GetLength() < 1000 ) // TODO: magic number ?
				{
					// is a depot file path ?
					// we use the links here for safety
					CDiskFile* file = GDepot->FindFileUseLinks( cellText, 0 );
					if (file)
					{
						if ( file->GetDepotPath() != cellText )
						{
							ERR_WCC( TXT("!!! LINK FILE IN CSV '%ls'!!! - CSV references file '%ls' that was moved to '%ls'. Please update the CSV (we don't have links on cook and this will not work)."),
								csvFile->GetDepotPath().AsChar(), cellText.AsChar(), file->GetDepotPath().AsChar() );
							status = false;
						}

						outputList.PushBackUnique( file );
						numDeps += 1;
					}
				}
			}
		}

		// list dependencies of the CSV files
		if (numDeps)
		{
			LOG_WCC( TXT("Found %d dependencies in CSV '%ls'"), 
				numDeps, csvFile->GetDepotPath().AsChar());
		}
	}

	// added
	return status;
}

Bool CDependencyCacheCommandlet::ProcessFile( const CDiskFile* file, const String& ext, FileInfo* info )
{
	// skip if already processed
	if ( info->m_isProcessed )
		return true;
	info->m_isProcessed = true;

	// set the root resource flag for world files and game definitions
	if ( ext == m_worldFileExtension || ext == m_gameFileExtension )
		info->m_isRoot = true;

	if( ext == CSV_EXT)
	{
		TDynArray<const CDiskFile*> listOfCSVDeps;
		if( ProcessCSV( file, listOfCSVDeps ) )
		{
			for( const CDiskFile* file : listOfCSVDeps )
			{
				FileInfo* depInfo = MapValidFile( file );
				info->m_hardDependencies.PushBack( depInfo );

				// data needed only for report
				if ( m_saveReport )
				{
					depInfo->m_hardUsers.PushBack( info );
					depInfo->m_numHardRefs += 1;
				}

			}
			return true;
		}
		return false;
	}
	// create file reader - NOTE - this should be possible for EVERY file
	Red::TScopedPtr< IFile > reader( file->CreateReader() );
	if ( !reader )
	{
		info->m_isValid = false;
		return true;
	}

	// get file size
	const Uint64 fileSize = (Uint64) reader->GetSize();
	m_totalDataSize += fileSize;

	// update the histogram with file size
	CountFileSize( ext, fileSize );

	// try to load the dependency tables
	if ( fileSize >= sizeof( Uint32 ) )
	{
		// read the magic to confirm that it's the resource (saved the needless warning)
		Uint32 magic = 0;
		*reader << magic;
		if ( magic == CDependencyLoader::FILE_MAGIC )
		{
			// rewind file for reading
			reader->Seek(0);

			CDependencyLoader loader( *reader, file );
			TDynArray< FileDependency > dependencies;
			if ( loader.LoadDependencies( dependencies, true ) )
			{
				m_numResourceFiles += 1;
				m_totalResourceSize += fileSize;

				info->m_isResource = true;

				for ( const FileDependency& dep : dependencies )
				{
					FileInfo* depInfo = nullptr;

					// try to find matching depot file without links
					CDiskFile* file = GDepot->FindFile( dep.m_depotPath );
					if ( file )
					{
						depInfo = MapValidFile( file );
						m_numValidDependencies += 1;
					}
					else
					{
						// try to find the dependency using links
						CDiskFile* file = GDepot->FindFileUseLinks( dep.m_depotPath, 0 );
						if ( file )
						{
							depInfo = MapValidFile( file );
							m_numMovedDependencies += 1;

							MapLink( dep.m_depotPath, file->GetDepotPath() );
						}
						else
						{
							// just a plain invalid file
							depInfo = MapInvalidFile( dep.m_depotPath );
							m_numInvalidDependencies += 1;
						}
					}

					// save as dependency
					if ( depInfo )
					{
						if ( dep.m_isSoftDepdencency )
						{
							// data needed only for report
							if ( m_saveReport )
							{
								info->m_softDependencies.PushBack( depInfo );
								depInfo->m_softUsers.PushBack( info );
								depInfo->m_numSoftRefs += 1;
							}
						}
						else
						{
							info->m_hardDependencies.PushBack( depInfo );

							// data needed only for report
							if ( m_saveReport )
							{
								depInfo->m_hardUsers.PushBack( info );
								depInfo->m_numHardRefs += 1;
							}
						}
					}
				}
			}
			else
			{
				// load the tables - if we fail it means that the file is corrupted
				reader->Seek(0);
				if ( !loader.LoadTables() )
				{
					ERR_WCC( TXT("!!! CORRUPTED FILE DATA !!! File '%ls' contains corrupted data"), 
						file->GetDepotPath().AsChar() );
					return false;
				}
			}

			// hack for w2w - add all the w2l files as soft dependencies
			if ( ext == m_worldFileExtension && m_saveReport )
			{
				TDynArray< CDiskFile* > layerFiles;
				file->GetDirectory()->CollectFiles( layerFiles, String::EMPTY, true, false );

				for ( Uint32 i=0; i<layerFiles.Size(); ++i )
				{
					const CDiskFile* layerFile = layerFiles[i];
					const String layerFileExt = StringHelpers::GetFileExtension( layerFile->GetDepotPath() );

					if ( layerFileExt == m_layerFileExtension )
					{
						FileInfo* depInfo = MapValidFile( layerFile );
						if ( depInfo )
						{
							m_numValidDependencies += 1;
							info->m_softDependencies.PushBack( depInfo );
							depInfo->m_numSoftRefs += 1;
						}
					}
				}
			}
		}
	}

	// no fatal errors
	return true;
}

void CDependencyCacheCommandlet::ProcessDBEntry( const CCookerDataBase& dataBase, CCookerDataBase::TCookerDataBaseID entryId, const String& ext, FileInfo* info )
{
	// skip if already processed
	if ( info->m_isProcessed )
		return;
	info->m_isProcessed = true;

	// set the root resource flag for world files and game definitions
	if ( ext == m_worldFileExtension || ext == m_gameFileExtension )
		info->m_isRoot = true;

	// get file size
	Uint32 dbFileSize = 0;
	dataBase.GetFileDiskSize( entryId, dbFileSize );
	const Uint64 fileSize = static_cast< Uint64 >( dbFileSize );
	m_totalDataSize += fileSize;

	// update the histogram with file size
	CountFileSize( ext, fileSize );

	// try to load the dependency tables
	if ( fileSize >= sizeof( Uint32 ) )
	{
		TDynArray< CCookerResourceEntry > hardDependencies, softDependencies, inplaceDependencies;
		dataBase.GetFileDependencies( entryId, hardDependencies, softDependencies, inplaceDependencies );

		// all entries in the cook.db are assumed to be ok
		m_numResourceFiles += 1;
		m_totalResourceSize += fileSize;
		info->m_isResource = true;

		if( !hardDependencies.Empty() )
		{
			for( const CCookerResourceEntry& dep : hardDependencies )
			{
				FileInfo* depInfo = MapValidDBEntry( &dep );
				m_numValidDependencies += 1;

				if( depInfo )
				{
					info->m_hardDependencies.PushBack( depInfo );

					// data needed only for report
					if ( m_saveReport )
					{
						depInfo->m_hardUsers.PushBack( info );
						depInfo->m_numHardRefs += 1;
					}
				}
			}
		}

		if( !softDependencies.Empty() )
		{
			for( const CCookerResourceEntry& dep : softDependencies )
			{
				FileInfo* depInfo = MapValidDBEntry( &dep );
				m_numValidDependencies += 1;

				if( depInfo )
				{
					info->m_softDependencies.PushBack( depInfo );

					// data needed only for report
					if ( m_saveReport )
					{
						depInfo->m_softUsers.PushBack( info );
						depInfo->m_numSoftRefs += 1;
					}
				}
			}
		}

		if( !inplaceDependencies.Empty() )
		{
			for( const CCookerResourceEntry& dep : inplaceDependencies )
			{
				FileInfo* depInfo = MapValidDBEntry( &dep );
				m_numValidDependencies += 1;

				if( depInfo )
				{
					info->m_inplaceDependencies.PushBack( depInfo );

					// data needed only for report
					if ( m_saveReport )
					{
						depInfo->m_inplaceUsers.PushBack( info );
						depInfo->m_numInplaceRefs += 1;
					}
				}
			}
		}		

		// here we really don't need to collect anything else from w2w,
		// everything should have been properly collected during the cook
	}
}

Bool CDependencyCacheCommandlet::SaveDepCache( const String& absolutePath )
{
	CTimeCounter timer;

	Red::TSharedPtr<IFile> file( GFileManager->CreateFileWriter( absolutePath, FOF_AbsolutePath | FOF_Buffered ) );
	if ( !file )
	{
		ERR_WCC( TXT("Failed to open file '%ls' for writing"), absolutePath.AsChar() );
		return false;
	}

	// write "header"
	{
		Uint32 magic = CDependencyCache::FILE_MAGIC;
		Uint32 version = CDependencyCache::FILE_VERSION;
		*file << magic;
		*file << version;
	}

	// resource IDs of the files
	{
		typedef Red::Core::ResourceManagement::CResourceId ResID;

		TDynArray< ResID > ids;
		ids.Reserve( 1 + m_files.Size() );

		// the zero value has special meaning
		ids.PushBack( ResID("") );
		
		// calculate and save the resource IDs
		for ( const FileInfo* fileInfo : m_files )
		{
			ResID id( fileInfo->m_depotPath.AsChar() );
			ids.PushBack( id );
		}

		// save the table
		ids.SerializeBulk( *file );
	}

	// dependencies
	{
		// prepare the "dependency buffer"
		TDynArray< Uint32 > depBuffer;
		TDynArray< Uint32 > depExclusion;
		TDynArray< Uint32 > depIndices;
		depBuffer.Reserve( m_numValidDependencies + m_numMovedDependencies + m_numInvalidDependencies );
		depExclusion.Reserve( 1 + m_files.Size() );
		depIndices.Reserve( 1 + m_files.Size() );

		// the zero values have special meaning and are not used
		depIndices.PushBack(0);
		depExclusion.PushBack(0);
		depBuffer.PushBack(0);

		// create the dependency mapping from hard dependencies
		for ( const FileInfo* file : m_files )
		{
			RED_FATAL_ASSERT( file->m_reportID == depIndices.Size(), "Mismatched file indices" );

			// do not emit hard dependencies that are also inplace dependencies, build a set for faster lookups
			THashSet< const FileInfo* > inplaceDependenciesSet;
			for ( const FileInfo* dep : file->m_inplaceDependencies )
			{
				inplaceDependenciesSet.Insert( dep );
			}

			// emit dependencies
			if ( file->m_hardDependencies.Empty() )
			{
				// no dependencies
				depIndices.PushBack( 0 );
			}
			else
			{
				// emit index to list head
				const Uint32 depListStart = depBuffer.Size();
				depIndices.PushBack( depListStart );

				// emit the dependencies
				for ( const FileInfo* dep : file->m_hardDependencies )
				{
					if ( !inplaceDependenciesSet.Exist( dep ) )
					{
						RED_FATAL_ASSERT( dep->m_reportID != 0, "Invalid file index" );
						depBuffer.PushBack( dep->m_reportID );
					}
				}

				// end with 0
				depBuffer.PushBack( 0 );
			}

			// emit inplace dependencies to separate array
			if ( file->m_inplaceDependencies.Empty() )
			{
				// no dependencies
				depExclusion.PushBack( 0 );
			}
			else
			{
				// emit index to list head
				const Uint32 depListStart = depBuffer.Size();
				depExclusion.PushBack( depListStart );

				// emit the dependencies
				for ( const FileInfo* dep : file->m_inplaceDependencies )
				{
					RED_FATAL_ASSERT( dep->m_reportID != 0, "Invalid file index" );
					LOG_CORE( TXT("File '%ls' excludes '%ls'"), file->m_depotPath.AsChar(), dep->m_depotPath.AsChar() );
					depBuffer.PushBack( dep->m_reportID );
				}

				// end with 0
				depBuffer.PushBack( 0 );
			}
		}

		// write tables
		depIndices.SerializeBulk( *file );
		depExclusion.SerializeBulk( *file );
		depBuffer.SerializeBulk( *file );
	}

	// done
	const Float oneMB = 1024.0f * 1024.0f;
	LOG_WCC( TXT("Dependency cache saved in %1.2fs (%1.2f MB)"), 
		timer.GetTimePeriod(), file->GetOffset() / oneMB );

	return true;
}

void CDependencyCacheCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Usage:") );
	LOG_WCC( TXT("  dependencies -out=<filepath> -report=<dir> [-db=<filepath>]") );
	LOG_WCC( TXT("") );
	LOG_WCC( TXT("Arguments:") );
	LOG_WCC( TXT("  out=<filepath>     - Save dependency cache to file") );
	LOG_WCC( TXT("  report=<dir>       - Generate dependency report in given directory") );
	LOG_WCC( TXT("  db=<filepath>      - Use dependencies in cook.db file instead of full depot") );
}
