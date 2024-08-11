/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "profiler.h"
#include "depot.h"
#include "depotBundles.h"
#include "diskBundlePreloader.h"
#include "diskBundleContent.h"
#include "diskBundle.h"
#include "ringAllocator.h"
#include "bundleDataCache.h"
#include "bundleDataHandleReader.h"
#include "bundleFileReaderDecompression.h"
#include "fileHandleCache.h"
#include "fileHandleReader.h"
#include "fileSystemProfilerWrapper.h"
#include "diskBundleIOCache.h"
#include "asyncFileAccess.h"
#include "fileDecompression.h"
#include "fileDecompressionAsyncFileWrapper.h"

CDiskBundle::CDiskBundle( const StringAnsi& shortName, const StringAnsi& fullPath, const String& absoluteFilePath, const Red::Core::Bundle::BundleID bundleId )
	: m_syncFileHandle( absoluteFilePath )
	, m_asyncFileHandle( Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE )
	, m_bundleId( bundleId )
	, m_shortName( shortName )
	, m_fullPath( fullPath )
{
}

CDiskBundle::~CDiskBundle()
{
	if ( m_asyncFileHandle != Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE )
	{
		Red::IO::GAsyncIO.ReleaseFile( m_asyncFileHandle );
	}
}

void CDiskBundle::OnAttached()
{
	const String& absoluteFilePath = m_syncFileHandle.GetAbsolutePath(); // already stored here, so don't store in CDiskBundle redundantly

	RED_FATAL_ASSERT( m_asyncFileHandle == Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE, "File %ls already initialized", absoluteFilePath.AsChar() );
	m_asyncFileHandle = Red::IO::GAsyncIO.OpenFile( absoluteFilePath.AsChar() );
	RED_FATAL_ASSERT( m_asyncFileHandle != Red::IO::CAsyncIO::INVALID_FILE_HANDLE, "Failed to create async handle to bundle file %ls", absoluteFilePath.AsChar() );
}

Bool CDiskBundle::ReadRawData( void* ptr, const Uint32 offset, const Uint32 size, Uint32& outNumBytesRead )
{
	PC_SCOPE( BundleReadRawData );

	// direct read of data, note: the file handles are cached, it's ultra fast to create new reader here
	// TODO: it would be fabulous not to allocate the reader here
	CNativeFileReader* reader = m_syncFileHandle.CreateReader();
	if ( reader )
	{
		reader->Seek( offset, Red::IO::eSeekOrigin_Set );
		reader->Read( ptr, size, outNumBytesRead );
		delete reader;

		return true;
	}

	// invalid file access
	return false;
}

namespace Helper
{
	class CScopedLoadDataProfiler
	{
	public:
		CScopedLoadDataProfiler( const Uint32 size )
		{
#ifdef RED_PROFILE_FILE_SYSTEM
			// profiling
			RedIOProfiler::ProfileDiskFileSyncLoadDataStart( size );
#endif
		}

		~CScopedLoadDataProfiler()
		{
#ifdef RED_PROFILE_FILE_SYSTEM
			// profiling
			RedIOProfiler::ProfileDiskFileSyncLoadDataEnd();
#endif
		}
	};

	class CScopedDecompressDataProfiler
	{
	public:
		CScopedDecompressDataProfiler( const Uint32 size, const Uint8 type )
		{
#ifdef RED_PROFILE_FILE_SYSTEM
			// profiling
			RedIOProfiler::ProfileDiskFileSyncDecompressStart( size, type );
#endif
		}

		~CScopedDecompressDataProfiler()
		{
#ifdef RED_PROFILE_FILE_SYSTEM
			// profiling
			RedIOProfiler::ProfileDiskFileSyncDecompressEnd();
#endif
		}
	};

} // Helper

EAsyncReaderResult CDiskBundle::CreateAsyncReader( const Red::Core::Bundle::SMetadataFileInBundlePlacement& placement, const Uint8 ioTag, IAsyncFile*& outAsyncReader ) const
{
	// validate the placement
	if ( placement.m_bundleID == 0 || placement.m_fileID == 0 )
		return eAsyncReaderResult_Failed;

	// file is to big
	const Uint32 unbufferedFileCutoffSize = 1024*1024*32; // files larger than that will NOT be buffered in the memory
	if ( placement.m_sizeInMemory > unbufferedFileCutoffSize )
	{
		ERR_CORE( TXT("!!! FILE IS TO BIG TO BE LOADED ON COOK/CONSOLES SAFELY !!!") );
		ERR_CORE( TXT("File is to big to be loaded on consoles without risking out of memory. File size = %1.2fMB, limit = %1.2fMB. Check the log for more details."), 
			placement.m_sizeInMemory / (1024.0f*1024.0f), unbufferedFileCutoffSize / (1024.0f*1024.0f) );
		return eAsyncReaderResult_Failed;
	}

	// request decompression task
	CFileDecompression::TaskInfo taskInfo;
	taskInfo.m_asyncFile = m_asyncFileHandle;
	taskInfo.m_compressionType = placement.m_compression;
	taskInfo.m_compressedSize = placement.m_sizeInBundle;
	taskInfo.m_uncompressedSize = placement.m_sizeInMemory;
	taskInfo.m_uncompressedMemory = nullptr; // we ask the system to allocate the memory for us
	taskInfo.m_offset = placement.m_offsetInBundle;
	taskInfo.m_ioTag = ioTag;

	// create the decompression task
	IFileDecompressionTask* decompressionTask = nullptr;
	const auto ret = GFileManager->GetDecompressionEngine()->DecompressAsyncFile( taskInfo, decompressionTask );
	if ( ret == CFileDecompression::eResult_Failed )
	{
		ERR_CORE( TXT("Failed to create decompression task for file ID #%d, size %d, bundle %ls."), 
			placement.m_fileID, placement.m_sizeInBundle, m_syncFileHandle.GetAbsolutePath().AsChar() );
		return eAsyncReaderResult_Failed;
	}
	else if ( ret == CFileDecompression::eResult_NotReady )
	{
		// busy busy
		return eAsyncReaderResult_NotReady;
	}

	// wrap the decompression task into a async file
	outAsyncReader = new CAsyncFile_DecompressionTaskWrapper( decompressionTask, String::EMPTY );

	// release internal reference (it was transfered to the async file wrapper)
	decompressionTask->Release();
	return eAsyncReaderResult_OK;
}

THandle< CDiskBundleContent > CDiskBundle::Preload()
{
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileDiskFileLoadResourceStart( ANSI_TO_UNICODE( m_shortName.AsChar() ) );
#endif

	// preload bundle data
	CDiskBundlePreloader preloader;
	TDynArray< THandle< CResource > > loadedResources;
	preloader.Preload( m_asyncFileHandle, m_bundleId, loadedResources );

#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileDiskFileLoadResourceEnd( ANSI_TO_UNICODE( m_shortName.AsChar() ) );
#endif

	// create the wrapper
	CDiskBundleContent* content = new CDiskBundleContent();
	content->SetResources( loadedResources );
	return content;
}

IFile* CDiskBundle::CreateReader( CBundleDataCache& dataCache, CDiskBundleIOCache & ioCache, const Red::Core::Bundle::SMetadataFileInBundlePlacement& placement )
{
	PC_SCOPE( CreateBundleFileReader )

	// If file is not compressed and small enough - create a direct reader
	const Uint32 unbufferedFileCutoffSize = 1024*1024*4; // c_bundleBufferSize - files larger than that will NOT be buffered in the memory
	if ( placement.m_compression == Red::Core::Bundle::CT_Uncompressed && (placement.m_sizeInBundle > unbufferedFileCutoffSize) )
	{
		// Make sure it's truly uncompressed
		if ( placement.m_sizeInBundle != placement.m_sizeInMemory )
		{
			ERR_CORE( TXT("Uncompressed file has different size in memory than on disk (%d!=%d), file ID #%d, size %d, bundle %ls. File may be to big to be loaded from bundles."), 
				placement.m_sizeInMemory, placement.m_sizeInBundle, placement.m_fileID, placement.m_sizeInBundle, m_syncFileHandle.GetAbsolutePath().AsChar() );
			return nullptr;
		}

		// Open native file access
		CNativeFileReader* reader = m_syncFileHandle.CreateReader();
		if ( !reader )
		{
			ERR_CORE( TXT("Failed to access uncompressed bundle, file ID #%d, size %d, bundle %ls. File may be to big to be loaded from bundles."), 
				placement.m_fileID, placement.m_sizeInBundle, m_syncFileHandle.GetAbsolutePath().AsChar() );
			return nullptr;
		}

		// Create wrapper for native file access
		return new CFileHandleReaderEx( reader, placement.m_offsetInBundle, placement.m_sizeInBundle );
	}
	else if ( placement.m_sizeInBundle > unbufferedFileCutoffSize )
	{
		ERR_CORE( TXT("File is to big to be loaded from bundles using synchronus CreateReader(), file ID #%d, size %d, bundle %ls."), 
			placement.m_fileID, placement.m_sizeInBundle, m_syncFileHandle.GetAbsolutePath().AsChar() );
		return nullptr;
	}

	// First check if a cached buffer exists for the decompressed data. Note that the buffer caches work on 'compressed' data indexing
	BundleDataHandle bundleDataHandle = dataCache.AcquireBuffer( placement.m_sizeInMemory );

	// If no buffer existed in the cache, we need to load the data (and possibly decompress it)
	if ( bundleDataHandle )
	{
		if ( placement.m_compression != Red::Core::Bundle::CT_Uncompressed )
		{
			// allocate the memory for the temporary data
			const Uint32 fileAlignment = 16; // was 4K - why ?
			CRingBufferBlock* block = ioCache.AllocateBlock( placement.m_sizeInBundle, fileAlignment );
			if ( !block )
			{
				ERR_CORE( TXT("Failed to create IO buffer for file ID #%d, size %d, bundle %ls. File may be to big to be loaded from bundles."), 
					placement.m_fileID, placement.m_sizeInBundle, m_syncFileHandle.GetAbsolutePath().AsChar() );

				return nullptr;
			}

			// read data
			Uint32 readBytes = 0;
			{
				Helper::CScopedLoadDataProfiler profiler( placement.m_sizeInBundle );
				if ( !ReadRawData( block->GetData(), placement.m_offsetInBundle, block->GetSize(), readBytes ) || (readBytes != block->GetSize()) )
				{
					ERR_CORE( TXT("Failed to read compressed file into the IO buffer, file ID #%d, size %d, bundle %ls. File may be to big to be loaded from bundles."), 
						placement.m_fileID, placement.m_sizeInBundle, m_syncFileHandle.GetAbsolutePath().AsChar() );

					block->Release();
					return nullptr;
				}
			}

			// decompress the data
			{
				Helper::CScopedDecompressDataProfiler profiler( placement.m_sizeInMemory, placement.m_compression );
				if ( !BundleFileReaderDecompression::DecompressFileBufferSynch( placement.m_compression, block->GetData(), block->GetSize(), bundleDataHandle->GetBuffer(), placement.m_sizeInMemory ) )
				{
					ERR_CORE( TXT("Failed to decompress file from IO buffer, file ID #%d, size %d, bundle %ls. File may be to big to be loaded from bundles."), 
						placement.m_fileID, placement.m_sizeInBundle, m_syncFileHandle.GetAbsolutePath().AsChar() );

					block->Release();
					return nullptr;
				}
			}

			// release the temporary data
			block->Release();
		}
		else
		{
			// No compression, load directly into the buffer
			{
				Helper::CScopedLoadDataProfiler profiler( placement.m_sizeInBundle );
				Uint32 readBytes = 0;
				if ( !ReadRawData( bundleDataHandle->GetBuffer(), placement.m_offsetInBundle, placement.m_sizeInBundle, readBytes ) || (readBytes != placement.m_sizeInBundle) )
				{
					ERR_CORE( TXT("Failed to read uncompressed file into the IO buffer, file ID #%d, size %d, bundle %ls. File may be to big to be loaded from bundles."), 
						placement.m_fileID, placement.m_sizeInBundle, m_syncFileHandle.GetAbsolutePath().AsChar() );

					return nullptr;
				}
			}
		}
	}

	// create the reader for the data buffer
	return new CBundleDataHandleReader( bundleDataHandle, placement.m_sizeInMemory );
}
