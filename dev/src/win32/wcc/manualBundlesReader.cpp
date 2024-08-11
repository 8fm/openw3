/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "manualBundlesReader.h"
#include "patchUtils.h"

#include "../../common/core/bundlePreamble.h"
#include "../../common/core/bundleFileReaderDecompression.h"

#pragma optimize ("",off)

CManualBundleReader::CManualBundleReader()
{
}

CManualBundleReader::~CManualBundleReader()
{
	Clear();
}

IFile* CManualBundleReader::FileInfo::CreateReader() const
{
	if ( m_placement.m_compression == Red::Core::Bundle::CT_Uncompressed )
	{
		// direct access
		RED_FATAL_ASSERT( m_placement.m_sizeInBundle == m_placement.m_sizeInMemory, "Invalid bundle file placement" );
		return new PatchUtils::COffsetFileReader( m_bundle->m_file, m_placement.m_offsetInBundle, m_placement.m_sizeInBundle );
	}
	else
	{
		Uint8* diskMemory = (Uint8*) PatchUtils::AllocateTempMemory();

		// load compressed file data
		m_bundle->m_file->Seek( m_placement.m_offsetInBundle );
		m_bundle->m_file->Serialize( diskMemory, m_placement.m_sizeInBundle );

		// allocate decompressed memory
		Uint8* fileMemory = diskMemory + m_placement.m_sizeInBundle;
		if ( !BundleFileReaderDecompression::DecompressFileBufferSynch( 
			(Red::Core::Bundle::ECompressionType) m_placement.m_compression, 
			diskMemory, m_placement.m_sizeInBundle,
			fileMemory, m_placement.m_sizeInMemory ) )
		{
			return nullptr;
		}

		// create memory based reader
		return new PatchUtils::CSimpleMemoryFileReader( fileMemory, m_placement.m_sizeInMemory );
	}
}

IFile* CManualBundleReader::FileInfo::CreateRawReader() const
{
	return new PatchUtils::COffsetFileReader( m_bundle->m_file, m_placement.m_offsetInBundle, m_placement.m_sizeInBundle );
}

void CManualBundleReader::Clear()
{
	m_files.Clear();
	m_bundles.ClearPtr();
}

void CManualBundleReader::AddBundles( const String& rootDirectory )
{
	// scan directory for bundles
	TDynArray< String > bundleFiles;
	GFileManager->FindFiles( rootDirectory, TXT("*.bundle"), bundleFiles, true );
	LOG_WCC( TXT("Found %d bundle files in '%ls'"), bundleFiles.Size(), rootDirectory.AsChar() );

	// process each bundle
	Uint32 numTotalFiles = 0;
	Uint32 numNewFiles = 0;
	Uint32 numNewBundles = 0;
	for ( const String& bundlePath : bundleFiles )
	{
		// open access to bundle absolutePath
		IFile* bundleFile = GFileManager->CreateFileReader( bundlePath, FOF_AbsolutePath | FOF_Buffered );
		if ( !bundleFile )
		{
			ERR_WCC( TXT("Unable to open bundle file %ls for reading"), bundlePath.AsChar() );
			continue;
		}

		// load the preamble
		Red::Core::Bundle::SBundleHeaderPreamble preamble;
		Red::MemoryZero( &preamble, sizeof(preamble) );
		bundleFile->Serialize( &preamble, sizeof(preamble) );

		// not a bundle
		if ( preamble.m_bundleStamp != Red::Core::Bundle::BUNDLE_STAMP )
		{
			delete bundleFile;
			ERR_WCC( TXT("Bundle header stamp mismatch in '%ls'"), bundlePath.AsChar() );
			continue;
		}

		// not a bundle
		if ( preamble.m_headerVersion != Red::Core::Bundle::BUNDLE_HEADER_VERSION )
		{
			delete bundleFile;
			ERR_WCC( TXT("Bundle header version mismatch in '%ls'"), bundlePath.AsChar() );
			continue;
		}

		// the output file
		BundleInfo* ret = new BundleInfo();
		ret->m_file = bundleFile;
		ret->m_absolutePath = bundlePath;
		numNewBundles += 1;

		// read the entries
		const Uint32 numItems = (preamble.m_headerSize / Red::Core::Bundle::ALIGNED_DEBUG_BUNDLE_HEADER_SIZE);
		ret->m_files.Reserve( numItems );
		for ( Uint32 i=0; i<numItems; ++i )
		{
			// move to the right place in the file
			const Uint32 fileOffset = sizeof(preamble) + (i * Red::Core::Bundle::ALIGNED_DEBUG_BUNDLE_HEADER_SIZE);
			ret->m_file->Seek( fileOffset );

			// keep reading
 			Red::Core::Bundle::SBundleHeaderItem fileInfo;
			Red::MemoryZero( &fileInfo, sizeof(fileInfo) );
			ret->m_file->Serialize( &fileInfo, sizeof(fileInfo) );

			// create placement entry
			Red::Core::Bundle::SMetadataFileInBundlePlacement placement;
			placement.m_bundleID = 0;
			placement.m_fileID = 0;
			placement.m_compression = fileInfo.m_compressionType;
			placement.m_offsetInBundle = fileInfo.m_dataOffset;
			placement.m_sizeInBundle = fileInfo.m_compressedDataSize;
			placement.m_sizeInMemory = fileInfo.m_dataSize;

			// setup file info
			FileInfo* file = new FileInfo();
			file->m_bundle = ret;
			file->m_fileIndex = i;
			file->m_depotPath = ANSI_TO_UNICODE( fileInfo.m_rawResourcePath );
 			file->m_placement = placement;
			ret->m_files.PushBack( file );
			numTotalFiles += 1;

			// is this a new file, if so, add it to the global list
			if ( m_files.Insert( file->m_depotPath, file ) )
			{
				const Uint64 pathHash = Red::CalculatePathHash64( file->m_depotPath.AsChar() );
				m_filesByHash.Insert( pathHash, file );
				numNewFiles += 1;
			}
		}
	}

	// stats
	LOG_WCC( TXT("Scanned %d bundles, found %d files, %d new"), 
		numNewBundles, numTotalFiles, numNewFiles );
}

IFile* CManualBundleReader::CreateReader( const String& depotPath ) const
{
	// find file
	FileInfo* fileInfo = nullptr;
	if ( !m_files.Find( depotPath, fileInfo ) )
		return nullptr;

	// create file reader
	return fileInfo->CreateReader();
}

IFile* CManualBundleReader::CreateRawReader( const String& depotPath ) const
{
	// find file
	FileInfo* fileInfo = nullptr;
	if ( !m_files.Find( depotPath, fileInfo ) )
		return nullptr;

	// create file reader
	return fileInfo->CreateRawReader();
}

Bool CManualBundleReader::ResolvePathHash( const Uint64 pathHash, String& outDepotPath ) const
{
	FileInfo* fileInfo = nullptr;
	if ( !m_filesByHash.Find( pathHash, fileInfo ) )
		return false;

	outDepotPath = fileInfo->m_depotPath;
	return true;
}

Uint32 CManualBundleReader::GetCompressedFileSize( const String& depotPath ) const
{
	FileInfo* fileInfo = nullptr;
	if ( !m_files.Find( depotPath, fileInfo ) )
		return 0;

	return fileInfo->m_placement.m_sizeInBundle;
}

Uint32 CManualBundleReader::GetMemoryFileSize( const String& depotPath ) const
{
	FileInfo* fileInfo = nullptr;
	if ( !m_files.Find( depotPath, fileInfo ) )
		return 0;

	return fileInfo->m_placement.m_sizeInMemory;
}

Uint32 CManualBundleReader::GetCompressionType( const String& depotPath ) const
{
	FileInfo* fileInfo = nullptr;
	if ( !m_files.Find( depotPath, fileInfo ) )
		return 0;

	return fileInfo->m_placement.m_compression;
}