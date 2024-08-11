/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/core/depot.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/dependencyLinkerFactory.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/dependencyFileTables.h"
#include "../../common/core/dependencyFileGluer.h"
#include "../../common/core/memoryFileReader.h"

#include "cookSeedFile.h"
#include "cookDataBase.h"

#pragma optimize("",off)

/// File gluer - creates a file for inplace loading
class CGlueFilesDLCCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CGlueFilesDLCCommandlet, ICommandlet, 0 );

public:
	CGlueFilesDLCCommandlet();
	~CGlueFilesDLCCommandlet();

	// ICommandlet interface
	virtual const Char* GetOneLiner() const { return TXT("Glues files into optimal files"); }
	virtual bool Execute( const CommandletOptions& options );
	virtual void PrintHelp() const {};

private:
	/// helper class to remove any inplace data from file
	class FileSourceWithInplaceDataRemoved : public CDependencyFileUnpacker
	{
	public:
		FileSourceWithInplaceDataRemoved()
		{}

		// this callback prevents inplace data from being re-included
		virtual void OnModifyTables( CDependencyFileData& fileTables, const Uint64 headerOffset ) const override
		{
			// remove the inplace flag from any of the import
			for ( auto& imp : fileTables.m_imports )
			{
				imp.m_flags &= ~CDependencyFileData::eImportFlags_Inplace;
			}

			// remove the inplace data buffers
			fileTables.m_inplace.Clear();
		}
	};

	class FileInfo
	{
	public:
		String							m_absolutePath;		// path on disk
		String							m_depotPath;		// path in depot
		Uint64							m_pathHash;			// file path hash
		Uint32							m_fullDiskSize;		// initial size on disk
		Uint32							m_baseDataSize;		// size of the base data (not merged)
		Uint32							m_finalSize;		// final size (of the merged files only)
		TDynArray< FileInfo* >			m_hardDeps;			// hard dependencies (possible glue candidates)

		Bool							m_invalid;			// invalid file (not current version)
		Bool							m_depsExtracted;	// dependencies were extracted for this file

		mutable FileSourceWithInplaceDataRemoved*	m_sourceData;

	public:
		FileInfo( const String& depotPath, const String& absolutePath, const Uint64 hash, const Uint32 size )
			: m_depotPath( depotPath )
			, m_absolutePath( absolutePath )
			, m_pathHash( hash )
			, m_fullDiskSize( size )
			, m_baseDataSize( 0 )
			, m_finalSize( 0 )
			, m_invalid( false )
			, m_depsExtracted( false )
			, m_sourceData( nullptr )
		{}

		~FileInfo()
		{
			if ( m_sourceData )
			{
				delete m_sourceData;
				m_sourceData = nullptr;
			}
		}

		// load source data
		const FileSourceWithInplaceDataRemoved* GetSourceData() const
		{
			if ( m_sourceData )
				return m_sourceData;

			if ( m_invalid )
				return nullptr;

			Red::TScopedPtr< IFile > reader( GFileManager->CreateFileReader( m_absolutePath, FOF_AbsolutePath | FOF_Buffered ) );
			if ( !reader )
			{
				return nullptr;
			}

			m_sourceData = new FileSourceWithInplaceDataRemoved();
			if ( !m_sourceData->Load( *reader ) )
			{
				delete m_sourceData;
				return nullptr;
			}

			return m_sourceData;
		}

		// calculate size of the file + all of the dependencies
		const Uint64 CalcFileDataWithDependencies() const
		{
			Uint64 ret = 0;
			THashSet< const FileInfo* > visitedFiles;
			CalcFileDataWithDependencies( ret, visitedFiles );
			return ret;
		}

	private:
		void CalcFileDataWithDependencies( Uint64& outSize, THashSet< const FileInfo* >& filesVisited ) const
		{
			if ( filesVisited.Exist( this ) )
				return;

			filesVisited.Insert( this );
			outSize += m_baseDataSize;

			for ( const FileInfo* dep : m_hardDeps )
			{
				dep->CalcFileDataWithDependencies( outSize, filesVisited );
			}
		}
	};

	class FileMapping
	{
	public:
		FileMapping( const String& baseDir, const String& masterBaseDir )
			: m_baseDir( baseDir )
			, m_masterBaseDir( masterBaseDir )
		{
		}

		Uint32 GetNumFiles() const
		{
			return m_files.Size();
		}

		FileInfo* GetFileInfo( const String& depotPath )
		{
			const Uint64 hash = Red::System::CalculatePathHash64( depotPath.AsChar() );

			// check the mapping
			FileInfo* fileInfo = nullptr;
			if ( m_fileMap.Find( hash, fileInfo ) )
			{
				return fileInfo;
			}

			// does the file exist ?
			String absolutePath = m_baseDir + depotPath;
			Uint32 fileSize = (Uint32) GFileManager->GetFileSize( absolutePath );
			if ( 0 == fileSize )
			{
				// try the master depot
				if ( !m_masterBaseDir.Empty() )
				{
					absolutePath = m_masterBaseDir + depotPath;
					fileSize = (Uint32) GFileManager->GetFileSize( absolutePath );
					if ( 0 == fileSize )
					{
						// map to null - we don't create entries for files that do not exist
						m_fileMap.Insert( hash, nullptr );
						return nullptr;
					}
					else
					{
						LOG_WCC( TXT("Redirecting '%ls' to master depot"), depotPath.AsChar() );
					}
				}
				else
				{
					// map to null - we don't create entries for files that do not exist
					m_fileMap.Insert( hash, nullptr );
					return nullptr;
				}
			}

			// Create file entry
			fileInfo = new FileInfo( depotPath, absolutePath, hash, fileSize );
			m_fileMap.Insert( hash, fileInfo );
			m_files.PushBack( fileInfo );
			return fileInfo;
		}

		const TDynArray< FileInfo* >& GetFiles() const
		{
			return m_files;
		}

	protected:
		String								m_baseDir;
		String								m_masterBaseDir;

		THashMap< Uint64, FileInfo* >		m_fileMap;
		TDynArray< FileInfo* >				m_files;
	};

	struct GlueSettings
	{
		TDynArray< String >		m_glueExtensions;			// which file types to glue
		TDynArray< String >		m_followExtensions;			// which file types to follow during dependency walking
		Bool					m_dumpTree;					// dump the initial dependencies
		Bool					m_dumpGluedTree;			// dump the glued files
		String					m_outputPath;				// where to write glued files

		GlueSettings()
			: m_dumpTree( false )
			, m_dumpGluedTree( false )
		{}

		const Bool Parse( const CommandletOptions& options )
		{
			if ( options.HasOption( TXT("glue") ) )
			{
				const auto list = options.GetOptionValues( TXT("glue") );
				for ( const String& txt : list )
				{
					m_glueExtensions.PushBack( txt );
					LOG_WCC( TXT("Glue extension: '%ls'"), txt.AsChar() );
				}
			}

			if ( options.HasOption( TXT("follow") ) )
			{
				const auto list = options.GetOptionValues( TXT("follow") );
				for ( const String& txt : list )
				{
					m_followExtensions.PushBack( txt );
					LOG_WCC( TXT("Follow extension: '%ls'"), txt.AsChar() );
				}
			}

			if ( options.HasOption( TXT("dumpsrc") ) )
			{
				m_dumpTree = true;
			}

			if ( options.HasOption( TXT("dumpglued") ) )
			{
				m_dumpGluedTree = true;
			}

			if ( !options.GetSingleOptionValue( TXT("outdir"), m_outputPath ) )
			{
				ERR_WCC( TXT("Expecting output path to be specified") );
				return false;
			}

			if ( !m_outputPath.EndsWith( TXT("\\") ) )
				m_outputPath += TXT("\\");

			return true;
		}

		// can we follow this file ?
		const Bool CanWalk( const String& filePath ) const
		{
			const String ext = StringHelpers::GetFileExtension( filePath );
			if ( m_followExtensions.Exist( ext ) )
				return true;

			return false;
		}

		// can we glue this file ?
		const Bool CanGlue( const String& filePath ) const
		{
			const String ext = StringHelpers::GetFileExtension( filePath );
			if ( m_glueExtensions.Exist( ext ) )
				return true;

			return false;
		}
	};

	void ExtractDependencies( FileMapping& map, FileInfo* file );
	Bool BuildGlueFlist( const FileInfo* rootFile, const GlueSettings& settings, TDynArray< const FileInfo* >& filesToGlue );
	Bool ProcessFile( const FileInfo* rootFile, const GlueSettings& settings, CCookerDataBase& db );

	void PrintDependencyTree( const FileInfo* file, const Uint32 level, THashSet< const FileInfo* >& visitedFiles ) const;

	static void BuildGlueFileList( const FileInfo* file, const Uint32 level, const GlueSettings& settings, THashMap< const FileInfo*, Uint32 >& visitedFiles, THashMap< const FileInfo*, Uint32 >& gluedFiles );
	static Uint32 CalculateFileSizeWithNoGluedData( const CDependencyFileData& data );
};

BEGIN_CLASS_RTTI( CGlueFilesDLCCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CGlueFilesDLCCommandlet );

CGlueFilesDLCCommandlet::CGlueFilesDLCCommandlet()
{
	m_commandletName = CName( TXT("gluefilesdlc") );
}

CGlueFilesDLCCommandlet::~CGlueFilesDLCCommandlet()
{
}

bool CGlueFilesDLCCommandlet::Execute( const CommandletOptions& options )
{
	// load file list
	THashSet< String > files;
	if ( options.HasOption( TXT("file") ) )
	{
		auto list = options.GetOptionValues( TXT("file") );
		for ( auto it = list.Begin(); it != list.End(); ++it )
		{
			files.Insert( *it );
		}
	}
	if ( options.HasOption( TXT("files") ) )
	{
		auto list = options.GetOptionValues( TXT("files") );
		for ( auto it = list.Begin(); it != list.End(); ++it )
		{
			// load file list
			CCookerSeedFile seedFile;
			if ( !seedFile.LoadFromFile( *it ) )
			{
				ERR_WCC( TXT("Failed to load file list from '%ls'"), (*it).AsChar() );
				return false;
			}

			// process files
			const Uint32 count = seedFile.GetNumEntries();
			for ( Uint32 i=0; i<count; ++i )
			{
				const auto* entry = seedFile.GetEntry( i );
				files.Insert( ANSI_TO_UNICODE( entry->GetFilePath().AsChar() ) );
			}
		}
	}

	// do we have files ?
	if ( files.Empty() )
	{
		ERR_WCC( TXT("No files specified for commandlet. Use -file of -files option.") );
		return false;
	}

	// list file
	LOG_WCC( TXT("Found %d files in the input set"), files.Size() );

	// base directory
	String baseDirectory;
	if ( !options.GetSingleOptionValue( TXT("path"), baseDirectory ))
	{
		ERR_WCC( TXT("No base directory specified for commandline. Use -path to specifiy the root directory for content.") );
		return false;
	}

	// additional base directory
	String masterBaseDirectory;
	if ( options.GetSingleOptionValue( TXT("pathmaster"), masterBaseDirectory ) )
	{
		LOG_WCC( TXT("Using master data path: '%ls'"), masterBaseDirectory.AsChar() );
	}

	// end with "\"
	if ( !baseDirectory.EndsWith( TXT("\\") ) )
		baseDirectory += TXT("\\");
	if ( !masterBaseDirectory.EndsWith( TXT("\\") ) )
		masterBaseDirectory += TXT("\\");

	// data base (that will be resaved)
	String dataBasePath;
	if ( !options.GetSingleOptionValue( TXT("db"), dataBasePath ) )
	{
		ERR_WCC( TXT("No cooker data base specified for commandline. Use -db to specifiy the cooker data base to use and modify.") );
		return false;
	}

	// master data base (that will be resaved)
	String masterDataBasePath;
	if ( options.GetSingleOptionValue( TXT("dbmaster"), masterDataBasePath ) )
	{
		LOG_WCC( TXT("Using master db from '%ls'"), masterDataBasePath.AsChar() );
	}

	// load the MERGED DBs
	Uint32 dbGroupToModify = 0;
	TList< String > dbFilesToLoad;
	if ( !masterDataBasePath.Empty() )
	{
		dbFilesToLoad.PushBack( masterDataBasePath );
		dbGroupToModify = 1; // resave the cook DB only for the DLC
	}
	dbFilesToLoad.PushBack( dataBasePath );

	// load the cook.db
	CCookerDataBase db;
	if ( !db.LoadFromFile( dbFilesToLoad, true ) )
	{
		ERR_WCC( TXT("Failed to load cooker data base from '%ls'. Invalid file?"), dataBasePath.AsChar() );
		return false;
	}

	// load glue settings
	GlueSettings settings;
	if ( !settings.Parse( options ) )
		return false;

	// process files and create dependency map
	//Sort( files.Begin(), files.End() );
	FileMapping fileMapping( baseDirectory, masterBaseDirectory );
	TDynArray< FileInfo* > rootFiles;
	for ( const String& path : files )
	{
		// create file info
		FileInfo* info = fileMapping.GetFileInfo( path );
		if ( !info )
		{
			ERR_WCC( TXT("Failed to extract file information from '%ls'. File will not be processed."), path.AsChar() );
			continue;
		}

		// extract file dependencies
		ExtractDependencies( fileMapping, info );
		rootFiles.PushBack( info );
	}

	// stats
	LOG_WCC( TXT("Mapped %d files from %d initial files"), 
		fileMapping.GetNumFiles(), files.Size() );

	// stats: count total size of visible files (generic info)
	{
		Uint64 totalBaseSize = 0;
		for ( const FileInfo* file : fileMapping.GetFiles() )
		{
			totalBaseSize += file->m_baseDataSize;
		}

		Uint64 rootBaseSize = 0;
		for ( const FileInfo* file : rootFiles )
		{
			rootBaseSize += file->m_baseDataSize;
		}

		LOG_WCC( TXT("Size of root data: %1.3f MB"), (Double)rootBaseSize / (1024.0*1024.0) );
		LOG_WCC( TXT("Size of all data visible: %1.3f MB"), (Double)totalBaseSize / (1024.0*1024.0) );
	}

	// glue files
	for ( FileInfo* file : rootFiles )
	{
		if ( !ProcessFile( file, settings, db ) )
		{
			ERR_WCC( TXT("Failed gluing file '%ls'. Process stopped."), file->m_depotPath.AsChar() );
			return false;
		}
	}

	// store new cook db
	{
		const String newDBPath = settings.m_outputPath + TXT("cook.db");		
		if ( !db.SaveToFile( newDBPath, dbGroupToModify ) )
		{
			ERR_WCC( TXT("Failed to save modified cook.db to '%ls'. Process stopped."), newDBPath.AsChar() );
			return false;
		}
	}

	// done
	return true;
}

Uint32 CGlueFilesDLCCommandlet::CalculateFileSizeWithNoGluedData( const CDependencyFileData& data )
{
	Uint32 endOfFile = sizeof(CDependencyFileData::Header);

	// measure data needed for buffers
	for ( const auto& buffer : data.m_buffers )
	{
		if ( buffer.m_dataOffset != 0 ) // only embedded buffers
		{
			endOfFile = Max< Uint32 >( endOfFile, buffer.m_dataSizeOnDisk + buffer.m_dataOffset );
		}
	}

	// measure data needed for objects
	for ( const auto& object : data.m_exports )
	{
		endOfFile = Max< Uint32 >( endOfFile, object.m_dataSize + object.m_dataOffset );
	}

	return endOfFile;
}

void CGlueFilesDLCCommandlet::ExtractDependencies( FileMapping& map, FileInfo* file )
{
	// file already has dependencies extracted
	if ( file->m_depsExtracted )
		return;

	// mark as extracted
	file->m_depsExtracted = true;

	// create file reader
	Red::TScopedPtr< IFile > reader( GFileManager->CreateFileReader( file->m_absolutePath, FOF_Buffered | FOF_AbsolutePath ) );
	if ( !reader )
	{
		ERR_WCC( TXT("Failed to open file '%ls'"), file->m_depotPath.AsChar() );
		file->m_invalid = true;
		return;
	}

	// load tables from the file
	CDependencyLoader loader( *reader, nullptr );
	if ( !loader.LoadTables() )
	{
		ERR_WCC( TXT("Failed to load tables from file '%ls'"), file->m_depotPath.AsChar() );
		file->m_invalid = true;
		return;
	}

	// get file tables
	const CDependencyFileData* tables = loader.GetFileTables();

	// process the imports
	TDynArray< FileInfo* > filesToFollow;
	for ( const CDependencyFileData::Import& import : tables->m_imports )
	{
		// ignore soft imports
		if ( import.m_flags & CDependencyFileData::eImportFlags_Soft )
			continue;		
		
		// get file path
		const AnsiChar* filePathAnsi = &tables->m_strings[ import.m_path ];
		const String filePath( ANSI_TO_UNICODE( filePathAnsi ) );

		// W3 Hack: don't follow files that are censored!
		{
			CFilePath filePathCensored( filePath );
			filePathCensored.SetFileName( filePathCensored.GetFileName() + TXT("_censored") );

			// do we also have the censored version of the file ?
			extern Bool GAllowFindFileFallback;
			GAllowFindFileFallback = false;
			auto* diskFile = GDepot->FindFileUseLinks( filePathCensored.ToString(), 0 );
			GAllowFindFileFallback = true;
			if ( diskFile != nullptr )
			{
				static THashSet< String > censoredFiles;
				if ( !censoredFiles.Exist( filePath ) )
				{
					censoredFiles.Insert( filePath );
					LOG_WCC( TXT("File '%ls' will not be glued because it can be censored."), filePath.AsChar() );
				}
				continue;
			}
		}

		// register dependency and recurse
		FileInfo* depFile = map.GetFileInfo( filePath );
		if ( depFile != nullptr )
		{
			//LOG_WCC( TXT("Extracted '%ls' as dep of '%ls'"), depFile->m_depotPath.AsChar(), file->m_depotPath.AsChar() );
			file->m_hardDeps.PushBack( depFile );
			filesToFollow.PushBack( depFile );
		}
		else
		{
			WARN_WCC( TXT("Unknown '%ls' as dep of '%ls'"), filePath.AsChar(), file->m_depotPath.AsChar() );
		}
	}

	// calculate actual data size that we will have to glue
	// this may be smaller than the file on disk because the file on disk may be already glued
	file->m_baseDataSize = CalculateFileSizeWithNoGluedData( *tables );

	// extract dependencies from children
	for ( auto* depFile : filesToFollow )
		ExtractDependencies( map, depFile );
}

Bool CGlueFilesDLCCommandlet::ProcessFile( const FileInfo* rootFile, const GlueSettings& settings, CCookerDataBase& db )
{
	// build the glue list
	TDynArray< const FileInfo* > glueList;
	if ( !BuildGlueFlist( rootFile, settings, glueList ) )
		return false;

	// stats
	{
		Uint64 totalGluedSize = 0;
		THashSet< const FileInfo* > gluedFilesSet;
		for ( const FileInfo* glueFile : glueList )
		{
			totalGluedSize += glueFile->m_baseDataSize;
			gluedFilesSet.Insert( glueFile );
		}

		LOG_WCC( TXT("File '%ls' will be glued with %d files adding %1.3fMB to it"),
			rootFile->m_depotPath.AsChar(), glueList.Size(), (Double)totalGluedSize / (1024.0*1024.0) );

		if ( settings.m_dumpTree )
		{
			LOG_WCC( TXT("Initial dependency tree of '%ls':"), rootFile->m_depotPath.AsChar() );

			THashSet< const FileInfo* > visitedFiles;
			PrintDependencyTree( rootFile, 1, visitedFiles );
		}

		if ( settings.m_dumpGluedTree )
		{
			LOG_WCC( TXT("Glue list for '%ls' (%d files):"), 
				rootFile->m_depotPath.AsChar(), glueList.Size() );

			TDynArray< const FileInfo* > glueListWithRoot( glueList );
			glueListWithRoot.PushBack( rootFile );

			// another files
			for ( const FileInfo* glueFile : glueListWithRoot )
			{
				LOG_WCC( TXT("  [%ls]: %1.2f KB"),  
					glueFile->m_depotPath.AsChar(), glueFile->m_baseDataSize / 1024.0f );

				// print any dependencies that were not glued
				for ( const FileInfo* depFile : glueFile->m_hardDeps )
				{
					if ( !gluedFilesSet.Exist( depFile ) )
					{
						LOG_WCC( TXT("      [%ls]: %1.2f KB (NOT GLUED)"),  
							depFile->m_depotPath.AsChar(), depFile->m_baseDataSize / 1024.0f );
					}
				}
			}
		}
	}

	// load the file being glued
	// this is the file we will be appending stuff into
	// note: we remove any existing glued content 
	CDependencyFileGluer gluer;
	{
		Red::TScopedPtr< IFile > reader( GFileManager->CreateFileReader( rootFile->m_absolutePath, FOF_AbsolutePath | FOF_Buffered ) );
		if ( !reader )
		{
			ERR_WCC( TXT("Failed to read from source file '%ls'"), rootFile->m_depotPath.AsChar() );
			return false;
		}

		if ( !gluer.Load( *reader ) )
		{
			ERR_WCC( TXT("Failed to load source data from '%ls'"), rootFile->m_depotPath.AsChar() );
			return false;
		}
	}

	// load and add the files we want to glue
	for ( const FileInfo* glueFile : glueList )
	{
		const StringAnsi depotPath( UNICODE_TO_ANSI( glueFile->m_depotPath.AsChar() ) );
		gluer.AddFile( depotPath, glueFile->GetSourceData() );
	}

	// save the glued file
	{
		const String outputFilePath = settings.m_outputPath + rootFile->m_depotPath;
		Red::TScopedPtr< IFile > writer( GFileManager->CreateFileWriter( outputFilePath, FOF_AbsolutePath | FOF_Buffered ) );
		if ( !writer )
		{
			ERR_WCC( TXT("Failed to write to target file '%ls'"), outputFilePath.AsChar() );
			return false;
		}

		if ( !gluer.Save( *writer ) )
		{
			ERR_WCC( TXT("Failed to write glued file to '%ls'"), outputFilePath.AsChar() );
			return false;
		}
	}

	// add the glued files as the inplace dependencies in the cook.db
	{
		// resolve the Id of the root file in the DB, this must exist
		CCookerDataBase::TCookerDataBaseID rootFileID = db.GetFileEntry( rootFile->m_depotPath );
		if ( !rootFileID )
		{
			ERR_WCC( TXT("File '%ls' is not in the cook.db. Cook is corrupted."), rootFile->m_depotPath.AsChar() );
		}

		// remove existing inplace dependencies for given file
		db.RemoveInplaceDependencies( rootFileID );

		// add new inplace dependencies
		for ( const FileInfo* glueFile : glueList )
		{
			// resolve the ID of the glued file in the DB
			CCookerDataBase::TCookerDataBaseID gluedFileID = db.GetFileEntry( glueFile->m_depotPath );
			if ( !gluedFileID )
			{
				ERR_WCC( TXT("Glued file '%ls' does not exist in the cook.db"), glueFile->m_depotPath.AsChar() );
				continue;
			}

			// add dependency
			db.AddFileInplaceDependency( rootFileID, gluedFileID );
		}
	}

	// file was glued
	return true;
}

void CGlueFilesDLCCommandlet::BuildGlueFileList( const FileInfo* file, const Uint32 level, const GlueSettings& settings, THashMap< const FileInfo*, Uint32 >& visitedFiles, THashMap< const FileInfo*, Uint32 >& gluedFiles )
{
	// invalid file
	if ( file->m_invalid )
		return;

	// walk the allowed dependencies
	for ( const FileInfo* dep : file->m_hardDeps )
	{
		if ( settings.CanGlue( dep->m_depotPath ) )
		{
			Uint32 fileLevel = 0;
			gluedFiles.Find( dep, fileLevel );

			if ( level > fileLevel )
			{
				gluedFiles.Set( dep, level );
			}
		}

		if ( settings.CanWalk( dep->m_depotPath ) )
		{
			Uint32 fileLevel = 0;
			visitedFiles.Find( dep, fileLevel );

			if ( level > fileLevel )
			{
				visitedFiles.Set( dep, level );
				BuildGlueFileList( dep, level+1, settings, visitedFiles, gluedFiles );
			}
		}
	}
}

Bool CGlueFilesDLCCommandlet::BuildGlueFlist( const FileInfo* rootFile, const GlueSettings& settings, TDynArray< const FileInfo* >& filesToGlue )
{
	Bool state = true;

	// walk the glue map
	THashMap< const FileInfo*, Uint32 > filesVisitedMap, filesGluedMap;
	BuildGlueFileList( rootFile, 1, settings, filesVisitedMap, filesGluedMap );

	// collect files to glue
	{
		filesToGlue.Clear();

		struct SortInfo
		{
			const FileInfo*		m_file;
			Uint32				m_level;

			const Bool operator<( const SortInfo& other ) const
			{
				return m_level < other.m_level;
			}
		};

		TDynArray< SortInfo > sortedFiles;
		sortedFiles.Reserve( filesGluedMap.Size() );

		for ( auto it = filesGluedMap.Begin(); it != filesGluedMap.End(); ++it )
		{
			SortInfo info;
			info.m_file = it.Key();
			info.m_level = it.Value();
			sortedFiles.PushBack( info );
		}

		::Sort( sortedFiles.Begin(), sortedFiles.End() );

		for ( Int32 i=sortedFiles.SizeInt()-1; i >= 0; --i )
		{
			filesToGlue.PushBack( sortedFiles[i].m_file );
		}
	}

	// validate glue list
	{
		// build index map
		THashMap< const FileInfo*, Uint32 > fileIndex;
		for ( Uint32 i=0; i<filesToGlue.Size(); ++i )
		{
			const FileInfo* file = filesToGlue[i];
			RED_ASSERT( !fileIndex.KeyExist(file), TXT("File [%ls] already collected"), file->m_depotPath.AsChar() );
			fileIndex.Insert( file, i );
		}

		// make sure all of the child dependencies are included BEFORE the files
		for ( Uint32 i=0; i<filesToGlue.Size(); ++i )
		{
			const FileInfo* file = filesToGlue[i];

			for ( const FileInfo* dep : file->m_hardDeps )
			{
				Uint32 depFileIndex = 0;
				if ( fileIndex.Find( dep, depFileIndex ) )
				{
					if ( depFileIndex >= i ) // dependeny file is later in the list than the parent file - this is invalid as this will cause recursive loading on the client
					{
						ERR_WCC( TXT("Dependence file [%ls] is after [%ls] in the glue list (%d > %d)"),
							dep->m_depotPath.AsChar(), file->m_depotPath.AsChar(), depFileIndex, i  );
						//state = false;
					}
				}
			}
		}
	}

	return state;
}

void CGlueFilesDLCCommandlet::PrintDependencyTree( const FileInfo* file, const Uint32 level, THashSet< const FileInfo* >& visitedFiles ) const
{
	// indent buffer
	Char tabs[ 64 ];
	for ( Uint32 i=0; i<level*2; ++i )
	{
		tabs[i] = ' ' ;
	}
	tabs[level*2] = 0;

	// visit each file only once
	const Bool visited = visitedFiles.Exist( file );
	visitedFiles.Insert( file );

	// stats
	LOG_WCC( TXT("%ls[%ls], %1.2f KB, %d deps %ls"), 
		tabs, file->m_depotPath.AsChar(),
		file->m_baseDataSize / 1024.0f,
		file->m_hardDeps.Size(),
		(visited && !file->m_hardDeps.Empty()) ? TXT("(VISITED)") : TXT("") );

	// recurse (only once)
	if ( !visited )
	{
		for ( const FileInfo* dep : file->m_hardDeps )
		{
			PrintDependencyTree( dep, level+1, visitedFiles );
		}
	}
}
