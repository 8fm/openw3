/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "bitmapDataBuffer.h"

// DirectX only! (For now, at least, we need an equivalent for Orbis)
#include "../gpuApiUtils/gpuApiMemory.h"

BitmapMipLatentDataBuffer::BitmapMipLatentDataBuffer()
	: LatentDataBuffer( GpuApi::MC_BitmapDataBuffer )
{
	// Default constructor is safe to call into base class
}

BitmapMipLatentDataBuffer::BitmapMipLatentDataBuffer( const void* data, TSize size )
	: LatentDataBuffer( GpuApi::MC_BitmapDataBuffer )
{
	// Since the vtable will not be created yet, we need to avoid calling resize in the base class constructor
	// Do it manually here instead
	ReallocateMemory( size, GpuApi::MC_BitmapDataBuffer );
	m_dataSize = size;

	if ( data && m_dataHandle )
	{
		Red::System::MemoryCopy( m_dataHandle, data, size );
	}
}

BitmapMipLatentDataBuffer::BitmapMipLatentDataBuffer( TSize size )
	: LatentDataBuffer( GpuApi::MC_BitmapDataBuffer )
{
	// Since the vtable will not be created yet, we need to avoid calling resize in the base class constructor
	ReallocateMemory( size, GpuApi::MC_BitmapDataBuffer );
	m_dataSize = size;
}

BitmapMipLatentDataBuffer::BitmapMipLatentDataBuffer( const BitmapMipLatentDataBuffer& other )
	: LatentDataBuffer( GpuApi::MC_BitmapDataBuffer )
{
	// Allocate data buffer
	if ( ReallocateMemory( other.m_dataSize, other.m_memoryClass, other.m_memoryAlignment ) )
	{
		// Set data size, it's not set in the allocate buffer
		m_dataSize = other.m_dataSize;

		// Copy source data
		if( other.m_dataHandle )
		{
			Red::System::MemoryCopy( m_dataHandle, other.m_dataHandle, m_dataSize );
		}
	}
}

BitmapMipLatentDataBuffer::~BitmapMipLatentDataBuffer()
{
	// Release the data buffer here while the vtable still contains the correct ReallocateMemory entry
	ReallocateMemory( 0, m_memoryClass, m_memoryAlignment );
	m_dataSize = 0;
}

BitmapMipLatentDataBuffer& BitmapMipLatentDataBuffer::operator=( const BitmapMipLatentDataBuffer& other )
{
	LatentDataBuffer::operator=( other );
	return *this;
}

//! BitmapDataBuffer objects should always allocate their memory from the GpuApi texture pool
Bool BitmapMipLatentDataBuffer::ReallocateMemory( TSize size, Red::MemoryFramework::MemoryClass newMemoryClass, TSize alignment )
{
	// Release the current data buffer
	if ( m_dataHandle )
	{		
		if ( !m_lock || m_lock->Decrement() == 0 )
		{
			// Free the lock
			delete m_lock;
			m_lock = NULL;

			// Free the data buffer
			GpuApi::FreeTextureData( m_dataHandle );
		}
	}

	// Reset handle
	m_dataHandle = NULL;

	// Allocate new buffer
	if ( size )
	{
		// Change memory class
		m_memoryClass = newMemoryClass;

		// Allocate the memory in the texture pool
		m_dataHandle = GpuApi::AllocateTextureData( size, alignment );
		if ( !m_dataHandle )
		{
			// Invalid allocation
			LOG_ENGINE( TXT("Unable to allocate %i bytes for data buffer"), size );
			return false;
		}

		// Create the lock handle
		m_lock = new Red::Threads::CAtomic< Int32 >( 1 );
	}

	// Buffer created or already valid
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

class BitmapDataBufferAllocator : public IDataBufferAllocator
{
public:
	virtual void* Allocate( const Uint32 size ) const override
	{
		return GpuApi::AllocateTextureData( size, 128 );

	}

	virtual void Free( void* ptr, const Uint32 size ) const override
	{
		GpuApi::FreeTextureData( ptr );
	}

	static BitmapDataBufferAllocator& GetInstance()
	{
		static BitmapDataBufferAllocator theInstance;
		return theInstance;
	}
};

BitmapDataBuffer::BitmapDataBuffer( const Uint32 size /*= 0*/, const void* data /*= nullptr*/ )
	: DataBuffer( BitmapDataBufferAllocator::GetInstance(), size, data )
{
}

BitmapDataBuffer::BitmapDataBuffer( const BitmapDataBuffer& other )
	: DataBuffer( other )
{
}

BitmapDataBuffer& BitmapDataBuffer::operator=( const BitmapDataBuffer& other )
{
	DataBuffer::operator=( other );
	return *this;
}

BitmapDataBuffer& BitmapDataBuffer::operator=( const DataBuffer& other )
{
	DataBuffer::operator=( other );
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
