#include "build.h"
#include "fileDecompression.h"
#include "fileDecompressionAsyncFileWrapper.h"
#include "memoryFileReader.h"

///--------------

CFile_DecompressionTaskWrapper::CFile_DecompressionTaskWrapper( class IFileDecompressionTask* task, void* data, const Uint32 size, const String& debugFileName )
	: IFile( FF_Reader | FF_FileBased ) // NOTE: we set the file based even though we are in the memory because the original source of the data was in the file
	, m_localReadOffset( 0 )
	, m_localFileSize( size )
	, m_data( data )
	, m_debugFileName( debugFileName )
	, m_task( task )
{
	// keep the uncompressed data alive for the lifetime of this reader
	m_task->AddRef();
}

CFile_DecompressionTaskWrapper::~CFile_DecompressionTaskWrapper()
{
	// release memory 
	if ( m_task )
	{
		m_task->Release();
		m_task = nullptr;
	}
}

void CFile_DecompressionTaskWrapper::Serialize( void* buffer, size_t size )
{
	RED_ASSERT( m_localReadOffset + size <= m_localFileSize, TXT( "Reading off the end of the cached file!" ) );
	MemUint readPtrAddress = reinterpret_cast< MemUint >( m_data ) + m_localReadOffset;
	void* readPtr = reinterpret_cast< void* >( readPtrAddress );
	Red::System::MemoryCopy( buffer, readPtr, size );
	m_localReadOffset += static_cast< Uint32 >( size );
}

Uint64 CFile_DecompressionTaskWrapper::GetOffset() const
{
	return m_localReadOffset;
}

Uint64 CFile_DecompressionTaskWrapper::GetSize() const
{
	return m_localFileSize;
}

void CFile_DecompressionTaskWrapper::Seek( Int64 offset )
{
	RED_ASSERT( offset >= 0 && offset <= (Int64)m_localFileSize, TXT("Invalid file offset: %d"), offset );
	m_localReadOffset = (Uint32) offset;
}

class IFileDirectMemoryAccess* CFile_DecompressionTaskWrapper::QueryDirectMemoryAccess()
{
	return static_cast< IFileDirectMemoryAccess* >(this);
}

///--------------

CAsyncFile_DecompressionTaskWrapper::CAsyncFile_DecompressionTaskWrapper( class IFileDecompressionTask* task, const String& debugFileName )
	: m_decompression( task )
	, m_debugFileName( debugFileName )
{
	// keep reference
	m_decompression->AddRef();
}

CAsyncFile_DecompressionTaskWrapper::~CAsyncFile_DecompressionTaskWrapper()
{
	// release internal reference to the decompression task
	// in a way we don't care any more what happens to it
	if ( m_decompression )
	{
		m_decompression->Release();
		m_decompression = nullptr;
	}
}

const Char* CAsyncFile_DecompressionTaskWrapper::GetFileNameForDebug() const
{
	return m_debugFileName.AsChar();
}

const CAsyncFile_DecompressionTaskWrapper::EResult CAsyncFile_DecompressionTaskWrapper::GetReader( IFile*& outReader ) const
{
	// request decompressed data from the decompression task
	void* loadedData = nullptr;
	const auto ret = m_decompression->GetData( loadedData );
	if ( ret == IFileDecompressionTask::eResult_NotReady )
	{
		// decompression has not finished yet, neither are we
		return eResult_NotReady;
	}
	else if ( ret == IFileDecompressionTask::eResult_OK )
	{
		RED_FATAL_ASSERT( loadedData != nullptr, "Data got decompressed but there's no data" );

		// OK, we have the valid data, create memory based reader for it
		const Uint32 loadedDataSize = m_decompression->GetSize();
		outReader = new CFile_DecompressionTaskWrapper( m_decompression, loadedData, loadedDataSize, m_debugFileName );
		return eResult_OK;
	}

	// we failed to decompress the data
	return eResult_Failed;
}