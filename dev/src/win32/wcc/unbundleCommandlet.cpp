/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/bundlePreamble.h"
#include "../../common/core/bundlePreambleParser.h"
#include "../../common/core/bundleFileReaderDecompression.h"
#include "../../common/core/asyncLoadToken.h"
#include "../../common/core/debugBundleHeaderParser.h"
#include "../../common/core/asyncIO.h"
#include "../bundlebuilder/creationParams.h"
#include "../../common/core/bundledefinition.h"

/// Unbundler - extracts all of the files from bundles
class CUnbundlerCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CUnbundlerCommandlet, ICommandlet, 0 );

public:
	CUnbundlerCommandlet();
	~CUnbundlerCommandlet();

	// ICommandlet interface
	virtual const Char* GetOneLiner() const { return TXT("Unbundle the files"); }
	virtual bool Execute( const CommandletOptions& options );
	virtual void PrintHelp() const;

private:
	String	m_outPath;

	Uint64	m_numTotalFilesFound;
	Uint64	m_numTotalFilesWritten;
	Uint64	m_numTotalBytes;

	Red::Core::Bundle::CBundlePreambleParser m_preambleParser;
	TDynArray< Uint8 > m_compressedData;
	TDynArray< Uint8 > m_uncompressedData;

	struct FileInfo
	{
		StringAnsi m_depotPath;		
		Uint8 m_compression;
	};

	struct BundleInfo
	{
		StringAnsi m_bundlePath;
		TDynArray< FileInfo > m_files;
	};

	TDynArray< BundleInfo > m_bundlesInfo;

	bool ProcessBundle( const String& bundleAbsolutePath, const StringAnsi& bundleRelativePath );
	CAsyncLoadToken* ReadPreamble( const String& absoluteBundleFilePath, Red::Core::Bundle::SBundleHeaderPreamble* preamble );
	Red::Core::Bundle::CDebugBundleHeaderParser::HeaderCollection ReadDebugBundleHeader( const String& absoluteFilePath, const Red::Core::Bundle::SBundleHeaderPreamble& preamble, void* dstBuffer );
};

BEGIN_CLASS_RTTI( CUnbundlerCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CUnbundlerCommandlet );

CUnbundlerCommandlet::CUnbundlerCommandlet()
	: m_preambleParser( *GDeprecatedIO )
{
	m_commandletName = CName( TXT("unbundle") );
}

CUnbundlerCommandlet::~CUnbundlerCommandlet()
{
}

bool CUnbundlerCommandlet::Execute( const CommandletOptions& options )
{
	// get out path
	if ( !options.GetSingleOptionValue( TXT("outdir"), m_outPath ) )
	{
		ERR_WCC( TXT("Missing output directory") );
		return false;
	}

	// conform directory name
	if ( !m_outPath.EndsWith( TXT("\\") ) )
		m_outPath += TXT("\\");

	// get input path
	String bundleBasePath;
	if ( !options.GetSingleOptionValue( TXT("dir"), bundleBasePath ) )
	{
		ERR_WCC( TXT("Missing bundle directrory") );
		return false;
	}

	// repacking file
	String repackingFilePath;
	options.GetSingleOptionValue( TXT("json"), repackingFilePath );
	if ( !repackingFilePath.Empty() )
	{
		LOG_WCC( TXT("Unpacking will dump bundles.json at '%ls'"), repackingFilePath.AsChar() );
	}

	// conform directory name
	if ( !bundleBasePath.EndsWith( TXT("\\") ) )
		bundleBasePath += TXT("\\");

	// find bundles
	TDynArray< String > bundles;
	GFileManager->FindFiles(bundleBasePath, TXT("*.bundle"), bundles, true );
	if ( bundles.Empty() )
	{
		ERR_WCC( TXT("No bundles found in specified directory '%ls'"), bundleBasePath.AsChar() );
		return false;
	}

	// stats reset
	m_numTotalFilesFound = 0;
	m_numTotalFilesWritten = 0;
	m_numTotalBytes = 0;

	// prepare buffers
	m_compressedData.Resize( 64 * 1024 * 1024 );
	m_uncompressedData.Resize( 64 * 1024 * 1024 );

	// process bundles
	for ( const String& path : bundles )
	{
		const StringAnsi bundleRelativePath = UNICODE_TO_ANSI( path.StringAfter(bundleBasePath).AsChar() );
		if ( !ProcessBundle( path, bundleRelativePath ) )
		{
			ERR_WCC( TXT("Error processing bundle '%ls'"), path.AsChar() );
			return false;
		}
	}

	// dump the json
	if ( !repackingFilePath.Empty() )
	{
		Red::Core::BundleDefinition::CBundleDefinitionWriter writer( UNICODE_TO_ANSI(repackingFilePath.AsChar()) );

		for ( const auto& bundleInfo : m_bundlesInfo )
		{
			writer.AddBundle( bundleInfo.m_bundlePath );

			for ( const auto& fileInfo : bundleInfo.m_files )
			{
				Red::Core::BundleDefinition::SBundleFileDesc fileDesc;
				fileDesc.Populate( fileInfo.m_depotPath );
				fileDesc.m_compressionType = (Red::Core::Bundle::ECompressionType) fileInfo.m_compression;
				writer.AddBundleFileDesc( bundleInfo.m_bundlePath, fileDesc );
			}
		}

		// flush output to file
		if ( !writer.Write() )
			ERR_WCC( TXT("Failed to save the generated JSON file to '%ls'"), repackingFilePath.AsChar() );
	}

	// print stats
	LOG_WCC( TXT("-------------------------------------------------") );
	LOG_WCC( TXT("-- Extracted %d files (ouf of %d) from %d bundles"), m_numTotalFilesWritten, m_numTotalFilesFound, bundles.Size() ); 
	LOG_WCC( TXT("-- Total data size: %1.3f MB"), m_numTotalBytes / (1024.0*1024.0) );
	LOG_WCC( TXT("-------------------------------------------------") );
	return true;
}

CAsyncLoadToken* CUnbundlerCommandlet::ReadPreamble( const String& absoluteFilePath, Red::Core::Bundle::SBundleHeaderPreamble* preamble )
{
	CAsyncLoadToken* preambleToken = m_preambleParser.CreateLoadToken( absoluteFilePath, preamble );
	m_preambleParser.Parse();
	return preambleToken;
}

Red::Core::Bundle::CDebugBundleHeaderParser::HeaderCollection CUnbundlerCommandlet::ReadDebugBundleHeader( const String& absoluteFilePath, const Red::Core::Bundle::SBundleHeaderPreamble& preamble, void* dstBuffer )
{
	Red::Core::Bundle::CDebugBundleHeaderParser headerParser( *GDeprecatedIO, preamble );
	CAsyncLoadToken* headerToken = headerParser.CreateLoadToken( absoluteFilePath, dstBuffer );
	// For later use.
	RED_UNUSED( headerToken );
	headerParser.Parse();
	return headerParser.GetHeaderItems();
}


bool CUnbundlerCommandlet::ProcessBundle( const String& bundleAbsolutePath, const StringAnsi& bundleRelativePath )
{
	Red::Core::Bundle::SBundleHeaderPreamble preamble;

	CAsyncLoadToken* preambleToken = ReadPreamble( bundleAbsolutePath, &preamble );
	if ( preamble.m_configuration != Red::Core::Bundle::EBundleHeaderConfiguration::BHC_Debug )
	{
		delete preambleToken;
		return false;
	}

	// open access to bundle file
	Red::TScopedPtr< IFile > bundleFile( GFileManager->CreateFileReader( bundleAbsolutePath, FOF_AbsolutePath | FOF_Buffered ) );
	if ( !bundleFile.Get() )
	{
		ERR_WCC( TXT("Unable to open bundle file %ls for reading"), bundleAbsolutePath.AsChar() );
		return false;
	}

	// create bundle info (for repacking)
	BundleInfo bundleInfo;
	bundleInfo.m_bundlePath = bundleRelativePath;
	m_bundlesInfo.PushBack( bundleInfo );

	// copy entries
	void* headerBuffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_BundleMetadata, preamble.m_headerSize );
	const auto headerItemCollection = ReadDebugBundleHeader( bundleAbsolutePath, preamble, headerBuffer );
	const Uint32 headerItemCount = headerItemCollection.Size();
	for( Uint32 itemIndex = 0; itemIndex < headerItemCount; ++itemIndex )
	{
		const auto& fileHeaderInfo = *headerItemCollection[ itemIndex ];

		// count files
		m_numTotalFilesFound += 1;

		// convert path for safety
		StringAnsi filePath( fileHeaderInfo.m_rawResourcePath );
		filePath.MakeLower(); // safety :(

		// add file info to the bundle dump
		FileInfo fileInfo;
		fileInfo.m_compression = (Uint8)fileHeaderInfo.m_compressionType;
		fileInfo.m_depotPath = filePath;
		m_bundlesInfo.Back().m_files.PushBack( fileInfo );

		// format full path
		const String fullPath = m_outPath + ANSI_TO_UNICODE( filePath.AsChar() );

		// file already exists ?
		const Uint64 existingSize = GFileManager->GetFileSize( fullPath );
		if ( existingSize == fileHeaderInfo.m_dataSize )
			continue;

		// create output file
		Red::TScopedPtr< IFile > outputFile( GFileManager->CreateFileWriter( fullPath, FOF_AbsolutePath | FOF_Buffered ) );
		if ( !outputFile.Get() )
		{
			ERR_WCC( TXT("Unable to create output file %ls for writing"), fullPath.AsChar() );
			return false;
		}

		// count stuff
		m_numTotalFilesWritten += 1;
		m_numTotalBytes += fileHeaderInfo.m_dataSize;

		// stats
		LOG_WCC( TXT("Extracting '%ls' (%1.2f KB)..."), ANSI_TO_UNICODE( filePath.AsChar() ), fileHeaderInfo.m_dataSize / 1024.0f );

		// uncompressed file - write directly
		if ( fileHeaderInfo.m_compressionType == Red::Core::Bundle::CT_Uncompressed )
		{
			// move to file start
			bundleFile->Seek( fileHeaderInfo.m_dataOffset );

			// copy data
			Uint8 buffer[ 64*1024 ];
			Uint64 sizeLeft = fileHeaderInfo.m_dataSize;
			while ( sizeLeft > 0 )
			{
				const Uint64 readBlock = Min< Uint64 >( sizeof(buffer), sizeLeft );
				bundleFile->Serialize( buffer, readBlock );
				outputFile->Serialize( buffer, readBlock );
				sizeLeft -= readBlock;
			}
			continue;
		}

		// prepare source buffer
		if ( fileHeaderInfo.m_compressedDataSize > m_compressedData.Size() )
			m_compressedData.Resize( fileHeaderInfo.m_compressedDataSize  );

		// prepare target buffer
		if ( fileHeaderInfo.m_dataSize > m_uncompressedData.Size() )
			m_uncompressedData.Resize( fileHeaderInfo.m_dataSize );

		// read compressed data
		bundleFile->Seek( fileHeaderInfo.m_dataOffset );
		bundleFile->Serialize( m_compressedData.Data(), fileHeaderInfo.m_compressedDataSize );

		// decompress file data
		if ( !BundleFileReaderDecompression::DecompressFileBufferSynch( (Red::Core::Bundle::ECompressionType) fileHeaderInfo.m_compressionType, m_compressedData.Data(), fileHeaderInfo.m_compressedDataSize, m_uncompressedData.Data(), fileHeaderInfo.m_dataSize ) )
		{
			ERR_WCC( TXT("Failed to decompress '%hs'"), filePath.AsChar() );
			return false;
		}

		// write output data
		outputFile->Serialize( m_uncompressedData.Data(), fileHeaderInfo.m_dataSize );
	}

	// cleanup
	RED_MEMORY_FREE( MemoryPool_Default, MC_BundleMetadata, headerBuffer );
	return true;
}


void CUnbundlerCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Usage:") );
	LOG_WCC( TXT("  unbundle -dir=<path> -outdir=<path>") );
	LOG_WCC( TXT("Parameters:") );
	LOG_WCC( TXT("  -dir=<dir>     - directory with the bundles (recursive)") );
	LOG_WCC( TXT("  -outdir=<dir>  - absolute path to output directory") );
	LOG_WCC( TXT("  -json=<path>   - optional bundles.json dump (for repacking)") );
}

