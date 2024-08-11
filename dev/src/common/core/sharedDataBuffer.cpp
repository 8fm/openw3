/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "sharedDataBuffer.h"
#include "sharedDataBufferImpl.h"

//-------------------------------------

CName SharedDataBuffer::st_typeName( TXT("SharedDataBuffer") );

TRTTITypeRegistrator< CRTTISharedDataBufferType > g_simpleTypeCRTTISharedDataBufferTypeRegistrator;

//-------------------------------------

SharedDataBuffer::SharedDataBuffer( const SharedDataBuffer& other )
{
	m_data = SSharedDataBufferCache::GetInstance().Copy( other.m_data );
}

SharedDataBuffer::SharedDataBuffer( const void* data, const Uint32 size )
{
	m_data = SSharedDataBufferCache::GetInstance().Request( data, size );
}

SharedDataBuffer::~SharedDataBuffer()
{
	if ( m_data != nullptr )
	{
		SSharedDataBufferCache::GetInstance().Release( m_data );
		m_data  = nullptr;
	}
}

SharedDataBuffer& SharedDataBuffer::operator=( const SharedDataBuffer& other )
{
	if ( m_data != other.m_data )
	{
		if ( m_data )
		{
			SSharedDataBufferCache::GetInstance().Release( m_data );
		}
		m_data = SSharedDataBufferCache::GetInstance().Copy( other.m_data );
	}

	return *this;
}

SharedDataBuffer& SharedDataBuffer::operator=( SharedDataBuffer&& other )
{
	auto oldData = m_data;

	m_data = other.m_data;
	other.m_data = nullptr;

	if ( oldData )
	{
		SSharedDataBufferCache::GetInstance().Release( oldData );
	}

	return *this;
}

const Uint32 SharedDataBuffer::GetSize() const
{
	return m_data ? m_data->GetSize() : 0;
}

const void* SharedDataBuffer::GetData() const
{
	return m_data ? m_data->GetData() : 0;
}

const Uint64 SharedDataBuffer::GetHash() const
{
	return m_data ? m_data->GetHash() : 0;
}

void SharedDataBuffer::Clear()
{
	if ( m_data )
	{
		SSharedDataBufferCache::GetInstance().Release( m_data );
		m_data = nullptr;
	}
}

void SharedDataBuffer::SetData( const void* data, const Uint32 size )
{
	auto newData = SSharedDataBufferCache::GetInstance().Request( data, size );

	if ( m_data )
	{
		SSharedDataBufferCache::GetInstance().Release( m_data );
	}

	m_data = newData;
}

void SharedDataBuffer::Serialize( class IFile& file )
{
	// nothing to do for garbage collector
	if ( file.IsGarbageCollector() || file.IsMapper() )
		return;

	// NOTE: This MUST be compatible with DataBuffer serialization

	// Loading
	if ( file.IsReader() )
	{
		// Load the data size
		Uint32 dataSize = 0;
		file << dataSize;

		// Load buffer content
		if ( dataSize > 0 )
		{
			// Load the data into temporary memory
			void* tempMem = nullptr;
			const Bool tempMemDynamic = (dataSize > 65535); // temporary buffer needs to be big
			if ( tempMemDynamic )
			{
				tempMem = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, dataSize );
			}
			else
			{
				tempMem = RED_ALLOCA( dataSize );
			}

			// Load the data
			file.Serialize( tempMem, dataSize );

			// Assign the data
			SetData( tempMem, dataSize );

			// Free temporary buffer if it was allocated from Heap
			if ( tempMemDynamic )
			{
				RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, tempMem );
				tempMem = nullptr;
			}
		}
		else
		{
			// No data
			Clear();
		}
	}
	else
	{
		// Save data size
		Uint32 dataSize = GetSize();
		file << dataSize;

		// Save data
		if ( dataSize > 0 )
		{
			const void* data = GetData();
			file.Serialize( (void*) data, dataSize );
		}
	}
}

///-----

void CRTTISharedDataBufferType::Clean( void* data ) const
{		
	SharedDataBuffer* dataBuffer = (SharedDataBuffer*) data;
	dataBuffer->Clear();
}

Bool CRTTISharedDataBufferType::Serialize( IFile& file, void* data ) const
{
	SharedDataBuffer* dataBuffer = (SharedDataBuffer*) data;
	dataBuffer->Serialize( file );
	return true;
}

Bool CRTTISharedDataBufferType::ToString( const void* data, String& valueString ) const
{
	const SharedDataBuffer* dataBuffer = (const SharedDataBuffer*) data;
	if ( dataBuffer->GetSize() > 0 )
	{
		valueString = String::Printf( TXT("SharedDataBuffer, %1.2fKB"), dataBuffer->GetSize() / 1024.0f );
	}
	else
	{
		valueString = String::Printf( TXT("SharedDataBuffer, empty") );
	}
	return true;
}

Bool CRTTISharedDataBufferType::FromString( void* data, const String& valueString ) const
{
	return false;
}

Bool CRTTISharedDataBufferType::DebugValidate( const void* /*data*/ ) const
{
	return true;
}

void CRTTISharedDataBufferType::Construct( void* object ) const
{
	new (object) SharedDataBuffer();		
}

void CRTTISharedDataBufferType::Destruct( void* object ) const
{
	( (SharedDataBuffer*) object )->~SharedDataBuffer();
}

Bool CRTTISharedDataBufferType::Compare( const void* data1, const void* data2, Uint32 /*flags*/ ) const
{
	const SharedDataBuffer* dataBuffer1 = (const SharedDataBuffer*) data1;
	const SharedDataBuffer* dataBuffer2 = (const SharedDataBuffer*) data2;
	return *dataBuffer1 == *dataBuffer2;
}

void CRTTISharedDataBufferType::Copy( void* dest, const void* src ) const
{
	const SharedDataBuffer* dataBufferSrc = (const SharedDataBuffer*) src;
	SharedDataBuffer* dataBufferDest = (SharedDataBuffer*) dest;
	*dataBufferDest = *dataBufferSrc;
}
