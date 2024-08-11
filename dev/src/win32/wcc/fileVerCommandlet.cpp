/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/depot.h"

/// File version checker
class CFileVersionCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CFileVersionCommandlet, ICommandlet, 0 );

public:
	CFileVersionCommandlet();

	// ICommandlet interface
	virtual const Char* GetOneLiner() const;
	virtual bool Execute( const CommandletOptions& options );
	virtual void PrintHelp() const;

private:
	Bool ProcessFile( CDiskFile* file, const Bool verbose, Uint32& outVersion );
	
	struct HistogramEntry
	{
		Uint32		m_count;
		CDateTime	m_oldest;
		CDateTime	m_newest;

		HistogramEntry()
			: m_count(0)
		{}

		void Add( const CDateTime& time )
		{
			if ( time != CDateTime::INVALID )
			{
				if ( m_count == 0 )
				{
					m_oldest = time;
					m_newest = time;
				}
				else
				{
					if ( time < m_oldest )
						m_oldest = time;
					if ( !(time < m_newest) )
						m_newest = time;
				}

				m_count += 1;
			}
		}
	};

	typedef TDynArray< HistogramEntry >		THistogram;
	
	Uint32					m_minVersion;
	THistogram				m_versionHistogram;	
	Uint32					m_numResources;
	Uint32					m_numEmpty;
	Uint32					m_numUnableToOpen;
	Uint32					m_numNotResources;
	Uint32					m_numInvalidVersion;
};

BEGIN_CLASS_RTTI( CFileVersionCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CFileVersionCommandlet );

CFileVersionCommandlet::CFileVersionCommandlet()
{
	m_commandletName = CName( TXT("filever") );
}

bool CFileVersionCommandlet::Execute( const CommandletOptions& options )
{
	// min version
	m_minVersion = VER_MINIMAL;
	{
		String minVersionStr;
		if ( options.GetSingleOptionValue( TXT("min"), minVersionStr ) )
		{
			if ( !FromString<Uint32>( minVersionStr, m_minVersion ) )
			{
				ERR_WCC( TXT("Unable to parse version number"));
				return false;
			}
		}
	}

	// verbose mode
	const Bool verbose = options.HasOption( TXT("verbose") );

	// histogram mode
	const Bool histogram = options.HasOption( TXT("histogram") );

	// tested extensions
	TDynArray< String > allowedExtensions;
	if ( options.HasOption( TXT("ext") ) )
	{
		const auto list = options.GetOptionValues( TXT("ext") );
		for ( auto it = list.Begin(); it != list.End(); ++it )
		{
			allowedExtensions.PushBack( (*it).ToLower() );
		}
	}

	// tested files
	TDynArray< String > customFiles;
	if ( options.HasOption( TXT("file") ) )
	{
		const auto list = options.GetOptionValues( TXT("file") );
		for ( auto it = list.Begin(); it != list.End(); ++it )
		{
			customFiles.PushBack( (*it).ToLower() );
		}
	}

	// initialize version histogram
	m_versionHistogram.Resize( VER_CURRENT+1 );
	
	// reset
	m_numNotResources = 0;
	m_numInvalidVersion = 0;
	m_numResources = 0;
	m_numEmpty = 0;
	m_numUnableToOpen = 0;

	// enumerate depot
	if ( !customFiles.Empty() )
	{
		for ( Uint32 i=0; i<customFiles.Size(); ++i )
		{
			const String& path = customFiles[i];

			// get file
			CDiskFile* file = GDepot->FindFile( path );
			if ( !file )
			{
				ERR_WCC( TXT("Unable to find '%ls' in depot"), path.AsChar() );
				continue;
			}

			// Process files
			Uint32 version = 0;
			if ( ProcessFile( file, true, version ) )
			{
				LOG_WCC( TXT("Version of file '%ls' = %d"), path.AsChar(), version );
			}
		}

		return true;
	}

	// non selective mode - process all
	if ( customFiles.Empty() )
	{
		CTimeCounter timer;

		LOG_WCC( TXT("Enumerating depot...") );

		TDynArray< CDiskFile* > allFiles;
		GDepot->CollectFiles( allFiles, String::EMPTY, true, false ); // get everything
		LOG_WCC( TXT("Depot enumerated in %1.2fs"), timer.GetTimePeriod() );
		LOG_WCC( TXT("Found %d files in depot"), allFiles.Size() );

		for ( CDiskFile* file : allFiles )
		{
			Uint32 version = 0;
			ProcessFile( file, verbose, version );
		}
	}

	// display summary
	LOG_WCC( TXT("Summary stats:") );
	LOG_WCC( TXT("  %d: files not opened"), m_numUnableToOpen );
	LOG_WCC( TXT("  %d: empty files"), m_numEmpty );
	LOG_WCC( TXT("  %d: non resource files"), m_numNotResources );
	LOG_WCC( TXT("  %d: deprecated resources"), m_numInvalidVersion );
	LOG_WCC( TXT("  %d: valid resources"), m_numResources );

	// display histogram
	if ( histogram )
	{
		CDateTime totalOldest( CDateTime::INVALID );
		CDateTime totalNewest( CDateTime::INVALID );

		LOG_WCC( TXT("Version histogram:") );
		for ( Uint32 i=0; i<m_versionHistogram.Size(); ++i )
		{
			const HistogramEntry& ver = m_versionHistogram[i];

			if ( ver.m_count > 0 )
			{
				LOG_WCC( TXT("  Version %d: %d files"), i, ver.m_count );
				LOG_WCC( TXT("     Oldest: %ls"), ToString( ver.m_oldest ).AsChar() );
				LOG_WCC( TXT("     Newest: %ls"), ToString( ver.m_newest ).AsChar() );

				if ( totalOldest == CDateTime::INVALID )
				{
					totalOldest = ver.m_oldest;
					totalNewest = ver.m_newest;
				}
				else
				{
					if ( ver.m_oldest < totalOldest )
						totalOldest = ver.m_oldest;

					if ( !(ver.m_newest < totalNewest) )
						totalNewest = ver.m_newest;
				}
			}
		}

		LOG_WCC( TXT("Total time span:" ) );
		LOG_WCC( TXT("     Oldest file: %ls"), ToString( totalOldest ).AsChar() );
		LOG_WCC( TXT("     Newest file: %ls"), ToString( totalNewest ).AsChar() );
	}

	return true;
}

Bool CFileVersionCommandlet::ProcessFile( CDiskFile* file, const Bool verbose, Uint32& outVersion )
{
	// create reader
	IFile* reader = file->CreateReader();
	if ( !reader )
	{
		if ( verbose )
		{
			ERR_WCC( TXT("Unable to open '%ls'"), file->GetDepotPath().AsChar() );
		}

		m_numUnableToOpen += 1;
		return false;
	}

	// empty file
	if ( reader->GetSize() == 0 )
	{
		if ( verbose )
		{
			ERR_WCC( TXT("Empty file '%ls'"), file->GetDepotPath().AsChar() );
		}

		m_numEmpty += 1;
		delete reader;
		return false;
	}

	// read file header
	struct RawFileHeader
	{
		Uint32		m_magic;
		Uint32		m_version;
	};
	RawFileHeader header;
	Red::MemoryZero( &header, sizeof(header) );
	reader->Serialize( &header, sizeof(header) );
	delete reader;

	// resource ?
	if ( header.m_magic != CDependencyLoader::FILE_MAGIC )
	{
		m_numNotResources += 1;
		return false;
	}

	// check version
	if ( header.m_version > VER_CURRENT )
	{
		// that's highly unusual - print it regardless of the settings
		ERR_WCC( TXT("File '%ls' has version %d that is outside current range"), 
			file->GetDepotPath().AsChar(), header.m_version );
		return false;
	}

	// get file time of the file
	const CDateTime fileTime( file->GetFileTime() );

	// to old
	if ( header.m_version < m_minVersion )
	{
		ERR_WCC( TXT("File '%ls' is in deprecated version %d"), 
				file->GetDepotPath().AsChar(), header.m_version );

		m_numInvalidVersion += 1;
		m_versionHistogram[ header.m_version ].Add( fileTime );
		return false;
	}

	// update version histogram
	m_versionHistogram[ header.m_version ].Add( fileTime );
	m_numResources += 1;
	return true;
}

const Char* CFileVersionCommandlet::GetOneLiner() const
{
	return TXT( "Check file versions" );
}

void CFileVersionCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Useage: " ) );
	LOG_WCC( TXT( "  filever [-ext=extensionlist] [-file=file] [-min=version] [-verbose] [-histogram]" ) );
	LOG_WCC( TXT( "" ) );
	LOG_WCC( TXT( "Optional parameters:" ) );
	LOG_WCC( TXT( "  -ext - Check only files with give extensions" ) );
	LOG_WCC( TXT( "  -file - Check only given files" ) );
	LOG_WCC( TXT( "  -min - Treat any file with version lower than given number as error" ) );
	LOG_WCC( TXT( "  -verbose - Print all invalid files" ) );
	LOG_WCC( TXT( "  -histogram - Print version histogram" ) );
}