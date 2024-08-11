/**
* Copyright (c) 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/bundlePreamble.h"
#include "../../common/core/bundlePreambleParser.h"
#include "../../common/core/bundleFileReaderDecompression.h"
#include "../../common/core/asyncLoadToken.h"
#include "../../common/core/debugBundleHeaderParser.h"
#include "../../common/core/asyncIO.h"

class CDumpBundleInfoCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CDumpBundleInfoCommandlet, ICommandlet, 0 );

public:

	CDumpBundleInfoCommandlet( )
		: m_preambleParser( *GDeprecatedIO )
		, m_outputFile( nullptr )
	{
		m_commandletName = CName( TXT( "dumpbundleinfo" ) );
	}
	~CDumpBundleInfoCommandlet( ) { }

	virtual bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner( ) const;
	virtual void PrintHelp( ) const;

private:

	struct Settings
	{
		String				m_inDir;
		String				m_outFile;
		TDynArray< String >	m_inFiles;

		Settings( )
			: m_inDir( String::EMPTY )
			, m_outFile( String::EMPTY )
		{ }

		Bool Parse( const ICommandlet::CommandletOptions& options );
	};

private:

	Settings m_settings;

	Red::Core::Bundle::CBundlePreambleParser m_preambleParser;

	IFile* m_outputFile;

private:

	bool ProcessBundle( const String& bundleAbsolutePath );
	CAsyncLoadToken* ReadPreamble( const String& absoluteBundleFilePath, Red::Core::Bundle::SBundleHeaderPreamble* preamble );
	Red::Core::Bundle::CDebugBundleHeaderParser::HeaderCollection ReadDebugBundleHeader( const String& absoluteFilePath, const Red::Core::Bundle::SBundleHeaderPreamble& preamble, void* dstBuffer );
};

BEGIN_CLASS_RTTI( CDumpBundleInfoCommandlet )
PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CDumpBundleInfoCommandlet );

bool CDumpBundleInfoCommandlet::Execute( const CommandletOptions& options )
{
	if( !m_settings.Parse( options ) )
		return false;

	// Find all bundles in the input directory.
	TDynArray< String > bundles;
	GFileManager->FindFiles( m_settings.m_inDir, TXT( "*.bundle" ), bundles, true );
	if( bundles.Empty( ) )
	{
		ERR_WCC( TXT( "No bundles found in specified directory '%ls'!" ), m_settings.m_inDir.AsChar( ) );
		return false;
	}

	// Create output file.
	m_outputFile = GFileManager->CreateFileWriter( m_settings.m_outFile, FOF_AbsolutePath | FOF_Buffered );
	if( m_outputFile == nullptr )
	{
		ERR_WCC( TXT( "Error creating file '%ls'!" ), m_settings.m_outFile.AsChar( ) );
		return false;
	}

	// Process bundles from directories.
	for( const String& path : bundles )
	{
		if( !ProcessBundle( path ) )
		{
			ERR_WCC( TXT( "Error processing bundle '%ls'!" ), path.AsChar( ) );
			delete m_outputFile;
			return false;
		}
	}

	// Process single bundles.
	for( const String& path : m_settings.m_inFiles )
	{
		if( !ProcessBundle( path ) )
		{
			ERR_WCC( TXT( "Error processing bundle '%ls'!" ), path.AsChar( ) );
			delete m_outputFile;
			return false;
		}
	}

	delete m_outputFile;

	return true;
}

const Char* CDumpBundleInfoCommandlet::GetOneLiner( ) const
{
	return TXT( "Dumps info for bundled files." );
}

void CDumpBundleInfoCommandlet::PrintHelp( ) const
{
	LOG_WCC( TXT( "Usage:" ) );
	LOG_WCC( TXT( "  dumpbundleinfo -indir=<path> -outpath=<path>" ) );
	LOG_WCC( TXT( "    -indir=<path>    - Directory with the bundles (recursive)." ) );
	LOG_WCC( TXT( "    -infile=<path>   - Absolute path to input bundle (multiple can be specified)." ) );
	LOG_WCC( TXT( "    -outfile=<path>  - Absolute path to output text file." ) );
}

Bool CDumpBundleInfoCommandlet::Settings::Parse( const ICommandlet::CommandletOptions& options )
{
	options.GetSingleOptionValue( TXT( "indir" ), m_inDir );

	if( !options.GetSingleOptionValue( TXT( "outfile" ), m_outFile ) )
	{
		ERR_WCC( TXT( "No out file path has been specified!" ) );
		return false;
	}

	if( options.HasOption( TXT( "infile" ) ) )
	{
		TList< String > files = options.GetOptionValues( TXT( "infile" ) );
		for( const String& file : files )
		{
			CFilePath path( file );
			if( path.IsRelative( ) )
			{
				ERR_WCC( TXT( "File path '%ls' is not absolute! Ignoring..." ), file.AsChar( ) );
				continue;
			}
			if( !GFileManager->FileExist( file ) )
			{
				ERR_WCC( TXT( "File '%ls' does not exist! Ignoring..." ), file.AsChar( ) );
				continue;
			}
			m_inFiles.PushBackUnique( file );
		}
	}

	if( m_inDir == String::EMPTY && m_inFiles.Empty( ) )
	{
		ERR_WCC( TXT( "No bundle files have been specified!" ) );
		return false;
	}

	if( !m_inDir.EndsWith( TXT( "\\" ) ) )
		m_inDir += TXT( "\\" );

	return true;
}

bool CDumpBundleInfoCommandlet::ProcessBundle( const String& bundleAbsolutePath )
{
	Red::Core::Bundle::SBundleHeaderPreamble preamble;

	Red::TScopedPtr< IFile > bundleFile( GFileManager->CreateFileReader( bundleAbsolutePath, FOF_AbsolutePath | FOF_Buffered ) );
	if( !bundleFile.Get( ) )
	{
		ERR_WCC( TXT( "Unable to open bundle file %ls for reading" ), bundleAbsolutePath.AsChar( ) );
		return false;
	}

	CAsyncLoadToken* preambleToken = ReadPreamble( bundleAbsolutePath, &preamble );
	if( preamble.m_configuration != Red::Core::Bundle::EBundleHeaderConfiguration::BHC_Debug )
	{
		delete preambleToken;
		return false;
	}

	void* headerBuffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_BundleMetadata, preamble.m_headerSize );
	const auto headerItemCollection = ReadDebugBundleHeader( bundleAbsolutePath, preamble, headerBuffer );
	const Uint32 headerItemCount = headerItemCollection.Size( );

	for( Uint32 itemIndex = 0; itemIndex < headerItemCount; ++itemIndex )
	{
		const auto& fileHeaderInfo = *headerItemCollection[ itemIndex ];

		StringAnsi filePath( fileHeaderInfo.m_rawResourcePath );
		filePath.MakeLower( );

		const String entry = String::Printf( TXT( "%ls;%ls;%d;%x;\n" ),
			bundleAbsolutePath.AsChar( ), ANSI_TO_UNICODE( filePath.AsChar( ) ), fileHeaderInfo.m_dataSize, fileHeaderInfo.m_cookedResourceCRC );
		m_outputFile->Serialize( UNICODE_TO_ANSI( entry.AsChar( ) ), entry.GetLength( ) );
	}

	RED_MEMORY_FREE( MemoryPool_Default, MC_BundleMetadata, headerBuffer );

	return true;
}

CAsyncLoadToken* CDumpBundleInfoCommandlet::ReadPreamble( const String& absoluteFilePath, Red::Core::Bundle::SBundleHeaderPreamble* preamble )
{
	CAsyncLoadToken* preambleToken = m_preambleParser.CreateLoadToken( absoluteFilePath, preamble );
	m_preambleParser.Parse( );
	return preambleToken;
}

Red::Core::Bundle::CDebugBundleHeaderParser::HeaderCollection CDumpBundleInfoCommandlet::ReadDebugBundleHeader( const String& absoluteFilePath, const Red::Core::Bundle::SBundleHeaderPreamble& preamble, void* dstBuffer )
{
	Red::Core::Bundle::CDebugBundleHeaderParser headerParser( *GDeprecatedIO, preamble );
	headerParser.CreateLoadToken( absoluteFilePath, dstBuffer );
	headerParser.Parse( );
	return headerParser.GetHeaderItems( );
}