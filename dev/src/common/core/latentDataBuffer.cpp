/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "latentDataBuffer.h"
#include "jobGenericJobs.h"
#include "fileLatentLoadingToken.h"
#include "loadingJobManager.h"
#include "object.h"
#include "memoryFileReader.h"
#include "dependencyLoader.h"
#include "profiler.h"

//---

LegacyDataBuffer::LegacyDataBuffer()
	: m_dataHandle( NULL )
	, m_memoryClass( MC_LegacyDataBuffer )
	, m_dataSize( 0 )
	, m_memoryAlignment( 0 )
	, m_lock( NULL )
{
}

LegacyDataBuffer::LegacyDataBuffer( Red::MemoryFramework::MemoryClass memoryClass, const void* data, TSize size, TSize alignment )
	: m_dataHandle( NULL )
	, m_memoryClass( memoryClass )
	, m_dataSize( 0 )
	, m_memoryAlignment( alignment )
	, m_lock( NULL )
{
	// Allocate data buffer
	if ( ReallocateMemory( size, memoryClass, m_memoryAlignment ) )
	{
		// Set data size, it's not set in the allocate buffer
		m_dataSize = size;

		// Copy source data
		if( data )
		{
			Red::System::MemoryCopy( m_dataHandle, data, size );
		}
	}
}

LegacyDataBuffer::LegacyDataBuffer( Red::MemoryFramework::MemoryClass memoryClass, TSize size, TSize alignment )
	: m_dataHandle( NULL )
	, m_dataSize( 0 )
	, m_memoryClass( memoryClass )
	, m_memoryAlignment( alignment )
	, m_lock( NULL )
{
	// Allocate data buffer
	if ( size > 0 && ReallocateMemory( size, memoryClass, alignment ) )
	{
		// Set data size, it's not set in the allocate buffer
		m_dataSize = size;

		// Initialize data buffer
		Red::System::MemoryZero( m_dataHandle, size );
	}
}

LegacyDataBuffer::LegacyDataBuffer( const LegacyDataBuffer& other )
	: m_dataHandle( NULL )
	, m_dataSize( 0 )
	, m_memoryClass( other.m_memoryClass )
	, m_memoryAlignment( other.m_memoryAlignment )
	, m_lock( NULL )
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

LegacyDataBuffer::~LegacyDataBuffer()
{
	// Release the data buffer
	ReallocateMemory( 0, m_memoryClass, m_memoryAlignment );
	m_dataSize = 0;
}

void LegacyDataBuffer::Clear()
{
	// Unlink buffers
	Unlink();

	// Free allocated memory
	ReallocateMemory( 0, m_memoryClass, m_memoryAlignment );
	m_dataSize = 0;
}

Bool LegacyDataBuffer::Load()
{
	// Static buffers are already loaded
	return true;
}

Bool LegacyDataBuffer::Unload()
{
	// Data cannot be unloaded from static buffer
	return false;
}

void LegacyDataBuffer::Unlink()
{
	// Data cannot be unlinked from static buffer
}

void LegacyDataBuffer::Allocate( TSize size )
{
	// Unlink from current file
	Unlink();

	// Allocate new buffer size
	if ( ReallocateMemory( size, m_memoryClass, m_memoryAlignment ) )
	{
		// Set new data size
		m_dataSize = size;
	}
	else
	{
		// Buffer not allocated
		m_dataSize = 0;
	}
}

Bool LegacyDataBuffer::ReallocateMemory( TSize size, Red::MemoryFramework::MemoryClass newMemoryClass, TSize alignment )
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
			RED_MEMORY_FREE( MemoryPool_Default, m_memoryClass, m_dataHandle );
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
		m_dataHandle = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, m_memoryClass, size, alignment );
		if ( !m_dataHandle )
		{
			// Invalid allocation
			LOG_CORE( TXT("Unable to allocate %i bytes for data buffer"), size );
			return false;
		}

		// Create the lock handle
		m_lock = new Red::Threads::CAtomic< Int32 >( 1 );
	}

	// Buffer created or already valid
	return true;
}

void LegacyDataBuffer::CopyHandle( LegacyDataBuffer& other )
{
	// Create the lock
	if ( !other.m_lock )
	{
		other.m_lock = new Red::Threads::CAtomic< Int32 >(1);
	}

	// Keep reference
	other.m_lock->Increment();

	// Copy crap
	m_lock = other.m_lock;
	m_memoryClass = other.m_memoryClass;
	m_memoryAlignment = other.m_memoryAlignment;
	m_dataHandle = other.m_dataHandle;
	m_dataSize = other.m_dataSize;	
}

void LegacyDataBuffer::MoveHandle( LegacyDataBuffer& other )
{
	// free current memory
	if ( m_dataHandle )
	{		
		if ( !m_lock || m_lock->Decrement() == 0 )
		{
			// Free the lock
			delete m_lock;
			m_lock = NULL;

			// Free the data buffer
			RED_MEMORY_FREE( MemoryPool_Default, m_memoryClass, m_dataHandle );
		}
	}

	// Copy crap
	m_memoryClass = other.m_memoryClass;
	m_memoryAlignment = other.m_memoryAlignment;
	m_dataHandle = other.m_dataHandle;
	m_dataSize = other.m_dataSize;
	m_lock = other.m_lock;

	// Reset crap here
	other.m_dataHandle = NULL;
	//other.m_dataSize = 0;
	other.m_lock = NULL;
}

void LegacyDataBuffer::Serialize( IFile& file )
{
	// No job done for the GC
	if ( file.IsGarbageCollector() )
	{
		return;
	}

	// No job for mapper
	if ( file.IsMapper() )
	{
		return;
	}

	// Loading
	if ( file.IsReader() )
	{
		// Release current file binding
		Unlink();

		// Load the data size
		TSize dataSize = 0;
		file << dataSize;

		// Create the data buffer
		m_dataSize = dataSize;
		ReallocateMemory( dataSize, m_memoryClass, m_memoryAlignment );

		// Load the data from file
		void* realData = m_dataHandle;
		if ( realData )
		{
			ASSERT( m_dataSize );
			file.Serialize( realData, dataSize );
		}
		else
		{
			// Skip the data
			const TSize skipOffset = static_cast< TSize >( file.GetOffset() ) + dataSize;
			file.Seek( skipOffset );
		}
	}
	else
	{
		// Preload data
		Load();

		// Save only valid data
		void* realData = m_dataHandle;
		if ( realData )
		{
			// Save data size
			file << m_dataSize;

			// Save data
			file.Serialize( realData, m_dataSize );
		}
		else
		{
			// Save empty buffer
			TSize zeroSize = 0;
			file << zeroSize;
		}

		// After saving the latent loading information is broken, unlink data buffer
		Unlink();
	}
}

void LegacyDataBuffer::Serialize( const void* data )
{
	// Load the data size
	TSize dataSize = *(TSize*)data;
	data = (TSize*)data + 1;

	// Create the data buffer
	m_dataSize = dataSize;
	ReallocateMemory( dataSize, m_memoryClass, m_memoryAlignment );

	// Load the data
	void* realData = m_dataHandle;
	if ( realData )
	{
		ASSERT( m_dataSize );
		Red::System::MemoryCopy(realData, data, m_dataSize);
	}
	data = (TSize*)data + m_dataSize;
}

LegacyDataBuffer& LegacyDataBuffer::operator=( const LegacyDataBuffer& other )
{
	// Unlink from latent loading token
	Unlink();

	// Allocate new memory
	if ( ReallocateMemory( other.m_dataSize, other.m_memoryClass, other.m_memoryAlignment ) )
	{
		// Set new size
		m_dataSize = other.GetSize();

		// Load the data from source		
		if ( const_cast< LegacyDataBuffer* >( &other )->Load() )
		{
			// Copy data
			if( other.m_dataHandle )
			{
				Red::System::MemoryCopy( m_dataHandle, other.m_dataHandle, m_dataSize );
			}
		}
		else
		{
			// Clear memory
			Red::System::MemoryZero( m_dataHandle, m_dataSize );
		}

		m_memoryAlignment = other.m_memoryAlignment;
	}

	// Done
	return *this;
}


//---

LatentDataBuffer::LatentDataBuffer()
	: LegacyDataBuffer( MC_DataBlob )
	, m_loadingToken( NULL )
{
}

LatentDataBuffer::LatentDataBuffer( Red::MemoryFramework::MemoryClass memoryClass )
	: LegacyDataBuffer( memoryClass )
	, m_loadingToken( NULL )
{
}

LatentDataBuffer::LatentDataBuffer( Red::MemoryFramework::MemoryClass memoryClass, const void* data, TSize size )
	: LegacyDataBuffer( memoryClass, data, size )
	, m_loadingToken( NULL )
{
}

LatentDataBuffer::LatentDataBuffer( Red::MemoryFramework::MemoryClass memoryClass, TSize size )
	: LegacyDataBuffer( memoryClass, size )
	, m_loadingToken( NULL )
{
}

LatentDataBuffer::LatentDataBuffer( const LatentDataBuffer& other )
	: LegacyDataBuffer( other )
	, m_loadingToken( NULL )
{
}

LatentDataBuffer::~LatentDataBuffer()
{
	// Release the loading token
	if ( m_loadingToken )
	{
		delete m_loadingToken;
		m_loadingToken = NULL;
	}

}

void LatentDataBuffer::MoveHandle( LatentDataBuffer& other )
{
	if ( &other == this )
	{
		return;
	}

	LegacyDataBuffer::MoveHandle( other );
	if ( m_loadingToken )
	{
		delete m_loadingToken;
		m_loadingToken = NULL;
	}

	m_loadingToken = other.m_loadingToken;
	other.m_loadingToken = 0;
}

Bool LatentDataBuffer::Load()
{
	// Already loaded
	if ( m_dataHandle )
	{
		return true;
	}

	// We must have valid loading token
	if ( m_loadingToken )
	{
		// Resume loading
		IFile* file = m_loadingToken->Resume( 0 );
		if ( file )
		{
			// Allocate data buffer
			ReallocateMemory( m_dataSize, m_memoryClass );

			// Load the data from file
			void* realData = m_dataHandle;
			if ( realData )
			{
				ASSERT( m_dataSize );
				file->Serialize( realData, m_dataSize );
			}

			// Done loading, close the handle
			delete file;
		}

		// Return loading status
		return m_dataHandle != NULL;
	}

	// Unable to load
	return false;
}

Bool LatentDataBuffer::Unload()
{
	// We can unload data only if we have a valid latent loading token
	if ( m_loadingToken )
	{
		// Data should already exist
		if ( m_dataHandle )
		{
			// Release memory
			ASSERT( m_dataSize );
			ReallocateMemory( 0, m_memoryClass );
			return true;
		}
	}

	// Data cannot be unloaded
	return false;
}

void LatentDataBuffer::Unlink()
{
	// Unlink from latent loading token
	if ( m_loadingToken )
	{
		delete m_loadingToken;
		m_loadingToken = NULL;
	}
}

void LatentDataBuffer::Serialize( IFile& file, Bool allowStreaming, Bool preload )
{
	// No job done for the GC
	if ( file.IsGarbageCollector() )
	{
		return;
	}

	// No job for mapper
	if ( file.IsMapper() )
	{
		return;
	}

	// Loading
	if ( file.IsReader() )
	{
		// Release current file binding
		Unlink();

		// Load the data size
		TSize dataSize = 0;
		file << dataSize;

		// We can use this created token to resume loading
		if ( allowStreaming )
		{
			m_loadingToken = file.CreateLatentLoadingToken( file.GetOffset() );
		}

		// Load right now
		if ( preload || !m_loadingToken )
		{
			// Create the data buffer
			m_dataSize = dataSize;
			ReallocateMemory( dataSize, m_memoryClass );

			// Load the data from file
			void* realMemory = m_dataHandle;
			if ( realMemory )			
			{
				// Load the data
				ASSERT( m_dataSize );
				file.Serialize( realMemory, dataSize );
			}
			else
			{
				// Skip the data ( OOM occured )
				file.Seek( file.GetOffset() + m_dataSize );
			}
		}
		else
		{
			// Skip the data, it will be loaded later
			m_dataSize = dataSize;
			file.Seek( file.GetOffset() + m_dataSize );
		}
	}
	else
	{
		// Preload data
		Load();

		// Save only valid data
		void* realData = m_dataHandle;
		if ( realData )
		{
			// Save data size
			file << m_dataSize;

			// Try to create loading token for the target file
			if ( file.IsResourceResave() )
			{
				// Delete current token
				if ( m_loadingToken )
				{
					delete m_loadingToken;
					m_loadingToken = NULL;
				}

				// Create new token
				if ( allowStreaming )
				{
					m_loadingToken = file.CreateLatentLoadingToken( file.GetOffset() );
				}
			}

			// Save data
			file.Serialize( realData, m_dataSize );
		}
		else
		{
			// Save empty buffer
			TSize zeroSize = 0;
			file << zeroSize;
		}

		// If we have a way to restore data after saving unload the buffer
		if ( m_loadingToken )
		{
			Unload();
		}
	}
}

LatentDataBuffer& LatentDataBuffer::operator=( const LatentDataBuffer& other )
{
	LegacyDataBuffer::operator =( other );
	return *this;
}

void LatentDataBuffer::Serialize( IFile& file )
{
	LegacyDataBuffer::Serialize( file );
}

void LatentDataBuffer::Serialize( const void* data )
{
	LegacyDataBuffer::Serialize( data );
}
