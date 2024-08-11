/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "gpuDataBuffer.h"
#include "../core/fileLatentLoadingToken.h"
#include "renderObject.h"
#include "../redMemoryFramework/redMemoryRegionAllocator.h"
#include "renderer.h"

class CGpuDataBufferObject : public IRenderObject
{
	DECLARE_RENDER_OBJECT_MEMORYPOOL( MemoryPool_SmallObjects, MC_RenderObjects );

private:
	Red::Threads::CAtomic< const Red::MemoryFramework::MemoryRegion* >	m_data;
	GpuApi::EInPlaceResourceType	m_type;
	Red::System::Uint32				m_dataAlignment;

public:
	CGpuDataBufferObject( GpuApi::EInPlaceResourceType type, Uint32 size, Uint32 alignment=-1 )
		: m_data( nullptr )
		, m_type( type )
		, m_dataAlignment( alignment )
	{
		if ( size > 0 )
		{
			m_data.SetValue( GpuApi::AllocateInPlaceMemoryRegion( m_type, size, GpuApi::MC_InPlaceTexture, m_dataAlignment, Red::MemoryFramework::Region_Shortlived ).GetRegionInternal() );
		}
	}

	virtual ~CGpuDataBufferObject()
	{
		Red::MemoryFramework::MemoryRegionHandle ptr = m_data.Exchange( nullptr );
		if ( ptr.IsValid() )
		{
			GpuApi::ReleaseInPlaceMemoryRegion( m_type, ptr );
		}
	}

	Red::MemoryFramework::MemoryRegionHandle UnlinkDataHandle()
	{
		return m_data.Exchange( nullptr );
	}

	Red::MemoryFramework::MemoryRegionHandle GetDataHandle()
	{
		return m_data.GetValue();
	}

	void UnlockRegion()
	{
		GpuApi::UnlockInPlaceMemoryRegion( m_type, m_data.GetValue() );
	}
};

static CGpuDataBufferObject* CreateDataBufferObject( GpuApi::EInPlaceResourceType type, Uint32 size, Uint32 alignment )
{
	if ( size == 0 )
	{
		return nullptr;
	}

	if ( type < 0 || type >= GpuApi::INPLACE_MAX )
	{
		RED_HALT( "Attempting to create a CGpuDataBufferObject with invalid resource type %u!", type );
		return nullptr;
	}

	CGpuDataBufferObject* obj = new CGpuDataBufferObject( type, size, alignment );
	// If we couldn't allocate the buffer, release the buffer object -- it's no good anyways.
	if ( !obj->GetDataHandle().IsValid() )
	{
		SAFE_RELEASE( obj );
	}
	return obj;
}

GpuDataBuffer::GpuDataBuffer()
	: m_loadingToken( nullptr )
	, m_data( nullptr )
	, m_dataSize( 0 )
	, m_type( GpuApi::INPLACE_MAX )
	, m_dataAlignment(-1)
{
}

GpuDataBuffer::GpuDataBuffer( GpuApi::EInPlaceResourceType type )
	: m_loadingToken( nullptr )
	, m_data( nullptr )
	, m_dataSize( 0 )
	, m_type( type )
	, m_dataAlignment( -1 )
{
}

GpuDataBuffer::GpuDataBuffer( GpuApi::EInPlaceResourceType type, TSize size, const void* data, Uint32 alignment )
	: m_loadingToken( nullptr )
	, m_data( nullptr )
	, m_dataSize( 0 )
	, m_type( type )
	, m_dataAlignment( alignment )
{
	m_data = CreateDataBufferObject( m_type, size, alignment );
	if ( data != nullptr && m_data != nullptr )
	{
		m_dataSize = size;

		// TODO : Pass to GpuApi, so it can maybe use DMA or something?
		// !NOTE! We are assuming the handle internal address does not change here. If we want to defrag, this needs to be synchronised
		Red::System::MemoryCopy( m_data->GetDataHandle().GetRawPtr(), data, size );
	}
}

GpuDataBuffer::GpuDataBuffer( const GpuDataBuffer& other )
	: m_loadingToken( nullptr )
	, m_data( nullptr )
	, m_dataSize( 0 )
	, m_type( other.m_type )
	, m_dataAlignment( other.m_dataAlignment )
{
	operator =( other );
}

GpuDataBuffer::~GpuDataBuffer()
{
	Clear();
}


GpuDataBuffer& GpuDataBuffer::operator =( const GpuDataBuffer& other )
{
	if ( this == &other )
	{
		return *this;
	}

	// If we were loaded from a file, forget about that info.
	Clear();

	if ( other.m_loadingToken != nullptr )
	{
		m_loadingToken = other.m_loadingToken->Clone();
	}

	m_type = other.m_type;
	m_dataSize = other.GetSize();
	m_dataAlignment = other.m_dataAlignment;

	Red::MemoryFramework::MemoryRegionHandle otherData = other.GetDataHandle();
	if ( m_dataSize > 0 && otherData.IsValid() )
	{
		m_data = CreateDataBufferObject( m_type, m_dataSize, m_dataAlignment );

		if ( m_data != nullptr )
		{
			// TODO : Pass to GpuApi, so it can maybe use DMA or something?
			// !NOTE! We are assuming the handle internal address does not change here. If we want to defrag, this needs to be synchronised
			Red::System::MemoryCopy( m_data->GetDataHandle().GetRawPtr(), otherData.GetRawPtr(), m_dataSize );
		}
	}

	return *this;
}

void GpuDataBuffer::Clear()
{
	UnlinkFromFile();

	SAFE_RELEASE( m_data );
	m_dataSize = 0;
}


Red::MemoryFramework::MemoryRegionHandle GpuDataBuffer::GetDataHandle()
{
	return m_data != nullptr ? m_data->GetDataHandle() : nullptr;
}

Red::MemoryFramework::MemoryRegionHandle GpuDataBuffer::GetDataHandle() const
{
	return m_data != nullptr ? m_data->GetDataHandle() : nullptr;
}


void GpuDataBuffer::Allocate( TSize size )
{
	// Clear existing data
	Clear();

	// Allocate new buffer size
	m_data = CreateDataBufferObject( m_type, size, m_dataAlignment );
	if ( m_data != nullptr )
	{
		m_dataSize = size;
	}
}

void GpuDataBuffer::Serialize( IFile& file )
{
	// TODO : Reading/writing directly to/from GPU memory could cause performance problems. We might actually want to use a temporary buffer
	// in system memory and then DMA it over?


	// Nothing to do for the GC or mapper
	if ( file.IsGarbageCollector() || file.IsMapper() )
	{
		return;
	}

	// Loading
	if ( file.IsReader() )
	{
		// Release current file binding
		UnlinkFromFile();

		// Load the data size
		file << m_dataSize;

		SAFE_RELEASE( m_data );
		if ( m_dataSize > 0 )
		{
			if( file.GetVersion() < VER_GPU_DATA_BUFFERS_ALIGNMENT )
			{
				m_dataAlignment = -1;
			}
			else
			{
				file << m_dataAlignment;
			}


			m_loadingToken = file.CreateLatentLoadingToken( static_cast< TSize >( file.GetOffset() ) );

			m_data = CreateDataBufferObject( m_type, m_dataSize, m_dataAlignment );
			if ( m_data != nullptr )
			{
				// !NOTE! We are assuming the handle internal address does not change here. If we want to defrag, this needs to be synchronised
				file.Serialize( m_data->GetDataHandle().GetRawPtr(), m_dataSize );
				GpuApi::FlushCpuCache( m_data->GetDataHandle().GetRawPtr(), m_dataSize );

				// At this point we won't write anything else to the buffer, so it can be defragged if required
				m_data->UnlockRegion();
			}
			else
			{
				// Couldn't allocate the buffer just now, so skip the data... can try loading again later
				file.Seek( file.GetOffset() + m_dataSize );
			}
		}
	}
	else
	{
		Bool wasLoaded = m_data != nullptr;

		// Preload data
		Load();

		// Save only valid data
		if ( m_data != nullptr )
		{
			// Save data size
			file << m_dataSize;
			file << m_dataAlignment;

			// Try to create loading token for the target file
			if ( file.IsResourceResave() )
			{
				// Delete current token
				if ( m_loadingToken )
				{
					delete m_loadingToken;
					m_loadingToken = nullptr;
				}

				// Create new token
				m_loadingToken = file.CreateLatentLoadingToken( static_cast< TSize >( file.GetOffset() ) );
			}

			// Save data
			// !NOTE! We are assuming the handle internal address does not change here. If we want to defrag, this needs to be synchronised
			file.Serialize( m_data->GetDataHandle().GetRawPtr(), m_dataSize );
		}
		else
		{
			// Save empty buffer
			TSize zeroSize = 0;
			file << zeroSize;
		}

		// If we have a way to restore data, and we weren't already loaded to begin with, unload the data.
		if ( !wasLoaded && m_loadingToken != nullptr )
		{
			Unload();
		}
	}
}


void GpuDataBuffer::CopyHandle( const GpuDataBuffer& other )
{
	if ( this == &other )
	{
		return;
	}

	SAFE_COPY( m_data, other.m_data );

	m_dataSize	= other.m_dataSize;
	m_type		= other.m_type;

	UnlinkFromFile();
	if ( other.m_loadingToken != nullptr )
	{
		m_loadingToken = other.m_loadingToken->Clone();
	}
}


void GpuDataBuffer::MoveHandle( GpuDataBuffer& other )
{
	if ( this == &other )
	{
		return;
	}

	m_data		= other.m_data;
	m_dataSize	= other.m_dataSize;
	m_type		= other.m_type;

	// Make a copy of the other's loading token. We don't just steal theirs, so that they can still reload later. We're just taking
	// the currently allocated buffer.
	UnlinkFromFile();
	if ( other.m_loadingToken != nullptr )
	{
		m_loadingToken = other.m_loadingToken->Clone();
	}

	// Clear out other buffer.
	other.m_data = nullptr;

	// If the other doesn't have a loading token, then we've effectively reallocated it to 0. If it does have a token, it's still
	// possible to reload, so data size should still be valid.
	if ( other.m_loadingToken == nullptr )
	{
		other.m_dataSize = 0;
	}
}


Red::MemoryFramework::MemoryRegionHandle GpuDataBuffer::UnlinkDataHandle()
{
	Red::MemoryFramework::MemoryRegionHandle theHandle = nullptr;
	if ( m_data != nullptr )
	{
		theHandle = m_data->UnlinkDataHandle();
	}

	SAFE_RELEASE( m_data );
	if ( m_loadingToken == nullptr )
	{
		m_dataSize = 0;
	}

	return theHandle;
}


Bool GpuDataBuffer::Load()
{
	// Already loaded
	if ( ( m_data != nullptr && m_data->GetDataHandle().IsValid() ) || m_dataSize == 0 )
	{
		return true;
	}

	// We must have valid loading token
	if ( m_loadingToken == nullptr )
	{
		return false;
	}

	// Resume loading
	IFile* file = m_loadingToken->Resume( 0 );
	if ( file == nullptr )
	{
		return false;
	}

	// Release old data (can happen if it was previously unlinked)
	SAFE_RELEASE( m_data );

	// Allocate data buffer
	m_data = CreateDataBufferObject( m_type, m_dataSize, m_dataAlignment );
	if ( m_data != nullptr )
	{
		// Load the data from file
		// !NOTE! We are assuming the handle internal address does not change here. If we want to defrag, this needs to be synchronised
		file->Serialize( m_data->GetDataHandle().GetRawPtr(), m_dataSize );
	}

	// Done loading, close the handle
	delete file;

	return m_data != nullptr;
}

Bool GpuDataBuffer::Unload()
{
	if ( m_data == nullptr )
	{
		return true;
	}
	if ( m_loadingToken == nullptr )
	{
		return false;
	}

	SAFE_RELEASE( m_data );

	return true;
}


void GpuDataBuffer::UnlinkFromFile()
{
	// Unlink from latent loading token
	if ( m_loadingToken != nullptr )
	{
		delete m_loadingToken;
		m_loadingToken = nullptr;
	}
}
