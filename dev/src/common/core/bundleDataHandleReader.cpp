/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "bundleDataCache.h"
#include "bundleDataHandleReader.h"

CBundleDataHandleReader::CBundleDataHandleReader( const BundleDataHandle& dataHandle, const Uint32 size )
	: IFile( FF_Reader | FF_FileBased ) // NOTE: we set the file based even though we are in the memory because the original source of the data was in the file
	, m_localReadOffset( 0 )
	, m_localFileSize( size )
	, m_bundleDataBuffer( dataHandle )
{
}

CBundleDataHandleReader::~CBundleDataHandleReader()
{
	// m_dataHandle is destroyed
}

void CBundleDataHandleReader::Serialize( void* buffer, size_t size )
{
	RED_ASSERT( m_localReadOffset + size <= m_localFileSize, TXT( "Reading off the end of the cached file!" ) );
	MemUint readPtrAddress = reinterpret_cast< MemUint >( m_bundleDataBuffer->GetBuffer() ) + m_localReadOffset;
	void* readPtr = reinterpret_cast< void* >( readPtrAddress );
	Red::System::MemoryCopy( buffer, readPtr, size );
	m_localReadOffset += static_cast< Uint32 >( size );
}

Uint64 CBundleDataHandleReader::GetOffset() const
{
	return m_localReadOffset;
}

Uint64 CBundleDataHandleReader::GetSize() const
{
	return m_localFileSize;
}

void CBundleDataHandleReader::Seek( Int64 offset )
{
	RED_ASSERT( offset >= 0 && offset <= (Int64)m_localFileSize, TXT("Invalid file offset: %d"), offset );
	m_localReadOffset = (Uint32) offset;
}
