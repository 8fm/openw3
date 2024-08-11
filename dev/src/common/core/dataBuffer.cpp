/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "dataBuffer.h"
#include "file.h"

//----

CName DataBuffer::st_typeName( TXT("DataBuffer") );

TRTTITypeRegistrator< CRTTIDataBufferType > g_simpleTypeCRTTIDataBufferTypeRegistrator;

//----

DataBuffer::DataBuffer( const IDataBufferAllocator& allocator /*= DefaultDataBufferAllocator()*/, const TSize size /*=0*/, const void* data /*=nullptr*/ )
	: m_dataHandle( NULL )
	, m_dataSize( 0 )
	, m_allocator( &allocator )  // assumes that we used a some kind of singleton
{
	// Allocate data buffer
	if ( size )
	{
		m_dataHandle = ReallocateMemory( *m_allocator, nullptr, size, 0 );
		m_dataSize = size;

		// Copy source data, if not specified - memset the whole thing to zeros
		if ( data )
		{
			Red::System::MemoryCopy( m_dataHandle, data, size );
		}
		else
		{
			// TODO: is this required ?
			Red::System::MemoryZero( m_dataHandle, size );
		}
	}
}

DataBuffer::DataBuffer( const DataBuffer& other )
	: m_dataHandle( NULL )
	, m_dataSize( 0 )
	, m_allocator( other.m_allocator )
{
	// Allocate data buffer
	if ( other.m_dataSize )
	{
		RED_FATAL_ASSERT( m_allocator != nullptr, "Trying to use DataBuffer that was not initialized properly" );
		RED_FATAL_ASSERT( other.m_dataHandle != nullptr, "DataBuffer has non zero data size but no data" );

		m_dataHandle = ReallocateMemory( *m_allocator, m_dataHandle, other.GetSize(), 0 );
		m_dataSize = other.GetSize();

		// Copy source data
		Red::System::MemoryCopy( m_dataHandle, other.m_dataHandle, m_dataSize );
	}
}

DataBuffer::DataBuffer( DataBuffer&& other )
	: m_dataHandle( other.m_dataHandle )
	, m_dataSize( other.m_dataSize )
	, m_allocator( other.m_allocator )
{
	// steal only the data, do not touch the allocator
	other.m_dataHandle = nullptr;
	other.m_dataSize = 0;
}

DataBuffer::~DataBuffer()
{
	Clear();
}

void DataBuffer::Clear()
{
	RED_FATAL_ASSERT( m_allocator != nullptr, "Trying to use DataBuffer that was not initialized properly" );

	// Release the data buffer
	ReallocateMemory( *m_allocator, m_dataHandle, 0, m_dataSize );

	// Reset internal state (but not the allocator)
	m_dataHandle = nullptr;
	m_dataSize = 0;
}

void DataBuffer::Allocate( const TSize size )
{
	RED_FATAL_ASSERT( m_allocator != nullptr, "Trying to use DataBuffer that was not initialized properly" );

	// Allocate new buffer size
	m_dataHandle = ReallocateMemory( *m_allocator, m_dataHandle, size, m_dataSize );
	m_dataSize = size;
}

void* DataBuffer::ReallocateMemory( const IDataBufferAllocator& allocator, void* dataPtr, const TSize newSize, const TSize oldSize )
{
	// free existing data
	if ( dataPtr )
	{
		RED_FATAL_ASSERT( oldSize != 0, "Trying to free data buffer with zero size" );
		allocator.Free( dataPtr, oldSize );
		dataPtr	= nullptr;
	}

	// allocate new data
	if ( newSize )
	{
		dataPtr = allocator.Allocate( newSize );
		RED_FATAL_ASSERT( dataPtr != nullptr, "Out of memory when allocating %d bytes for data buffer", newSize );
	}

	return dataPtr;
}

void DataBuffer::CopyHandle( const DataBuffer& other )
{
	DataBuffer copy( other );
	MoveHandle( copy );
}

void DataBuffer::MoveHandle( DataBuffer& other )
{
	// free current data
	Clear();

	// Copy crap
	m_dataHandle = other.m_dataHandle;
	m_dataSize = other.m_dataSize;
	m_allocator = other.m_allocator;

	// Reset crap in the source buffer - NOTE: the allocator is NOT reset
	other.m_dataHandle = nullptr;
	other.m_dataSize = 0;
}

bool DataBuffer::CompareContent( const DataBuffer& other ) const
{
	if ( other.m_dataSize != m_dataSize )
		return false;

	if ( !m_dataSize && !other.m_dataSize )
		return true;

	if ( m_dataHandle && other.m_dataHandle )
		return (0 == Red::MemoryCompare( m_dataHandle, other.m_dataHandle, m_dataSize ));

	return false;
}

void DataBuffer::ClearWithoutFree()
{
	// Reset internal state (but not the allocator)
	m_dataHandle = nullptr;
	m_dataSize = 0;
}

void DataBuffer::Serialize( IFile& file )
{
	// No job done for the non-file stuff
	if ( file.IsGarbageCollector() || file.IsMapper() )
	{
		return;
	}

	// Loading
	if ( file.IsReader() )
	{
		// If we are serializing a buffer twice, this is probably bad news
		if( m_dataHandle )
		{
			ReallocateMemory( *m_allocator, m_dataHandle, 0, m_dataSize );
			m_dataHandle = nullptr;
			m_dataSize = 0;
		}

		// Load the data size
		TSize dataSize = 0;
		file << dataSize;

		// Restore default allocator - rare, but may happen
		if ( !m_allocator ) m_allocator = &DefaultDataBufferAllocator();

		// Create the data buffer
		m_dataSize = dataSize;
		m_dataHandle = ReallocateMemory( *m_allocator, nullptr, m_dataSize, 0 );

		// Load the data from file
		file.Serialize( m_dataHandle, m_dataSize );
	}
	else
	{
		// Save data size
		file << m_dataSize;

		// Save data
		file.Serialize( m_dataHandle, m_dataSize );
	}
}

DataBuffer& DataBuffer::operator=( const DataBuffer& other )
{
	DataBuffer copy( other );
	MoveHandle( copy );
	return *this;
}

DataBuffer& DataBuffer::operator=( DataBuffer&& other )
{
	MoveHandle( other );
	return *this;
}

//----

void CRTTIDataBufferType::Clean( void* data ) const
{		
	DataBuffer* dataBuffer = (DataBuffer*) data;
	dataBuffer->Clear();
}

Bool CRTTIDataBufferType::Serialize( IFile& file, void* data ) const
{
	DataBuffer* dataBuffer = (DataBuffer*) data;
	dataBuffer->Serialize( file );
	return true;
}

Bool CRTTIDataBufferType::ToString( const void* data, String& valueString ) const
{
	const DataBuffer* dataBuffer = (const DataBuffer*) data;
	if ( dataBuffer->GetSize() > 0 )
	{
		valueString = String::Printf( TXT("DataBuffer, %1.2fKB"), dataBuffer->GetSize() / 1024.0f );
	}
	else
	{
		valueString = String::Printf( TXT("DataBuffer, empty") );
	}
	return true;
}

Bool CRTTIDataBufferType::FromString( void* data, const String& valueString ) const
{
	return false;
}

Bool CRTTIDataBufferType::DebugValidate( const void* /*data*/ ) const
{
	return true;
}

void CRTTIDataBufferType::Construct( void* object ) const
{
	new (object) DataBuffer();		
}

void CRTTIDataBufferType::Destruct( void* object ) const
{
	( (DataBuffer*) object )->~DataBuffer();
}

Bool CRTTIDataBufferType::Compare( const void* data1, const void* data2, Uint32 /*flags*/ ) const
{
	const DataBuffer* dataBuffer1 = (const DataBuffer*) data1;
	const DataBuffer* dataBuffer2 = (const DataBuffer*) data2;
	return dataBuffer1->CompareContent( *dataBuffer2 );
}

void CRTTIDataBufferType::Copy( void* dest, const void* src ) const
{
	const DataBuffer* dataBufferSrc = (const DataBuffer*) src;
	DataBuffer* dataBufferDest = (DataBuffer*) dest;
	dataBufferDest->CopyHandle( *dataBufferSrc );
}

