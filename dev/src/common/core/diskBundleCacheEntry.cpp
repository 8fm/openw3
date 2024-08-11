/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "diskBundleCacheEntry.h"
#include "bundleFileReaderDecompression.h"
#include "asyncFileAccess.h"
#include "fileDecompression.h"
#include "fileDecompressionAsyncFileWrapper.h"
#include "memoryFileReader.h"

//----------------------------------------------

CDiskBundleCacheEntryMemoryBlock::CDiskBundleCacheEntryMemoryBlock( const Bool ioPool, void* data, const Uint32 size )
	: m_ioPool( ioPool )
	, m_data( data )
	, m_size( size )
	, m_refCount( 1 )
{
}

CDiskBundleCacheEntryMemoryBlock::~CDiskBundleCacheEntryMemoryBlock()
{
	if ( m_ioPool )
	{
		RED_MEMORY_FREE( MemoryPool_IO, MC_BundlerAutoCache, m_data );
	}
	else
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_BundlerAutoCache, m_data );
	}

	m_data = nullptr;
}

void CDiskBundleCacheEntryMemoryBlock::AddRef()
{
	m_refCount.Increment();
}

void CDiskBundleCacheEntryMemoryBlock::Release()
{
	if ( 0 == m_refCount.Decrement() )
	{
		delete this;
	}
}

CDiskBundleCacheEntryMemoryBlock* CDiskBundleCacheEntryMemoryBlock::Create( const Bool ioPool, const Uint32 size )
{
	void* memory = nullptr;
	if ( ioPool )
	{
		memory = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_IO, MC_BundlerAutoCache, size, 16 );
	}
	else
	{
		memory = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_BundlerAutoCache, size, 16 );
	}

	if ( !memory )
		return nullptr;

	return new CDiskBundleCacheEntryMemoryBlock( ioPool, memory, size );
}


//----------------------------------------------

class CDiskBundleCacheEntryFileReader : public CMemoryFileReaderExternalBuffer
{
public:
	CDiskBundleCacheEntryFileReader( CDiskBundleCacheEntryMemoryBlock* memory )
		: CMemoryFileReaderExternalBuffer( memory->GetData(), memory->GetSize() )
		, m_memory( memory )
	{
		m_memory->AddRef();
	}

	virtual ~CDiskBundleCacheEntryFileReader()
	{
		SAFE_RELEASE( m_memory );
	}

	virtual const Char *GetFileNameForDebug() const override
	{
		return TXT("CDiskBundleCacheEntry");
	}

private:
	CDiskBundleCacheEntryMemoryBlock*		m_memory;
};

//----------------------------------------------

class CDiskBundleCacheEntryFileAsyncReader : public IAsyncFile
{
public:
	CDiskBundleCacheEntryFileAsyncReader( CDiskBundleCacheEntryMemoryBlock* memory )
		: m_memory( memory )
	{
	}

	virtual ~CDiskBundleCacheEntryFileAsyncReader()
	{
		SAFE_RELEASE( m_memory );
	}

	virtual const Char *GetFileNameForDebug() const override
	{
		return TXT("CDiskBundleCacheEntry");
	}

	virtual const EResult GetReader( IFile*& outReader) const override
	{
		outReader = new CDiskBundleCacheEntryFileReader( m_memory );
		return eResult_OK;
	}

private:
	CDiskBundleCacheEntryMemoryBlock*		m_memory;
};

//----------------------------------------------

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

//----------------------------------------------

CDiskBundleCacheEntry::CDiskBundleCacheEntry( Red::Core::Bundle::FileID fileID, Uint8 compressionType, const Uint32 uncompressedSize, CDiskBundleCacheEntryMemoryBlock* compressedData )
	: m_file( fileID )
	, m_compressionType( compressionType )
	, m_compressedData( compressedData )
	, m_uncompressedSize( uncompressedSize )
	, m_refCount( 1 )
{
	m_compressedData->AddRef();
}

CDiskBundleCacheEntry::~CDiskBundleCacheEntry()
{
	SAFE_RELEASE( m_compressedData );
}

void CDiskBundleCacheEntry::AddRef()
{
	m_refCount.Increment();
}

void CDiskBundleCacheEntry::Release()
{
	if ( 0 == m_refCount.Decrement() )
	{
		delete this;
	}
}

const Bool CDiskBundleCacheEntry::IsCompressed() const
{
	return (m_compressionType != Red::Core::Bundle::CT_Uncompressed);
}

IFile* CDiskBundleCacheEntry::CreateReader()
{
	// decompress the data
	auto* uncompressedData = DecompressData();
	if ( !uncompressedData )
		return nullptr;

	// create wrapper
	return new CDiskBundleCacheEntryFileReader( uncompressedData );
}

IAsyncFile* CDiskBundleCacheEntry::CreateAsyncReader()
{
	// decompress the data
	auto* uncompressedData = DecompressData();
	if ( !uncompressedData )
		return nullptr;

	// create wrapper
	return new CDiskBundleCacheEntryFileAsyncReader( uncompressedData );
}

CDiskBundleCacheEntryMemoryBlock* CDiskBundleCacheEntry::DecompressData()
{
	// not compressed, return original data
	if ( !IsCompressed() )
	{
		m_compressedData->AddRef();
		return m_compressedData;
	}
	
	// allocate output memory, can fail but it's handled
	CDiskBundleCacheEntryMemoryBlock* uncompressedData = CDiskBundleCacheEntryMemoryBlock::Create( true, m_uncompressedSize );
	if ( !uncompressedData )
		return nullptr;

	// decompress the memory
	Helper::CScopedDecompressDataProfiler profiler( m_uncompressedSize, m_compressionType );
	if ( !BundleFileReaderDecompression::DecompressFileBufferSynch( (Red::Core::Bundle::ECompressionType) m_compressionType, m_compressedData->GetData(), m_compressedData->GetSize(), uncompressedData->GetData(), m_uncompressedSize ) )
	{
		uncompressedData->Release();
		return nullptr;
	}

	// return memory with decompressed data
	return uncompressedData;
}

//----------------------------------------------
