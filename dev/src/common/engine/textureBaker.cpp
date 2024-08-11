#include "build.h"
#include "textureCache.h"
#include "../core/objectGC.h"
#include "../core/compression/chainedzlib.h"

#ifndef NO_TEXTURECACHE_COOKER

//////////////////////////////////////////////////////////////////////////
// NOTE : The following may no longer apply, now that the texture cache is built separately from the cooked resources.
//////////////////////////////////////////////////////////////////////////
//
// Async baking not currently implemented. Current plan as I understand it, is to end up with a process that would cook just textures, so we
// wouldn't gain much by doing the baking on a separate thread (wouldn't be baking a texture while some other resource might be cooked). Doing
// it async brought up other complications with data lifetimes -- since the mip chain is cleared after cooking a texture, so by the time the
// baking happens, that data might be gone -- or, baking a texture array after one of its source textures could fail, because we can't unload
// the source texture until it is baked, and the array needs full uncooked data. After overcoming these problems, the resulting cache differed
// in size.
//
// So, leaving async off for now, with simpler handling of the textures. Didn't remove the existing async logic, but it won't work as-is.
//#define ASYNC_BAKING



// When using chained zlib compression, don't split the lowest 6 mips into separate chunks. These would correspond to the resident
// portion, and there's no reason to want to load those separately.
static const Uint32 MIPS_TO_COMBINE = 6;


//////////////////////////////////////////////////////////////////////////


CTextureBakerOutput::CTextureBakerOutput()
	: m_dataAlignment( -1 )
{
}
CTextureBakerOutput::~CTextureBakerOutput()
{
}

void CTextureBakerOutput::WriteData( const void* data, Uint32 dataSize )
{
	if ( dataSize == 0 )
	{
		return;
	}

	Uint32 startOffset = m_data.Size();
	m_data.Resize( m_data.Size() + dataSize );

	if ( data != nullptr )
	{
		Red::System::MemoryCopy( m_data.TypedData() + startOffset, data, dataSize );
	}
}


//////////////////////////////////////////////////////////////////////////


struct STextureBakerTask
{
	String								m_debugName;

	Uint32								m_entry;				// If !m_isNonResource, the ID of the entry in the texture cache.
	String								m_path;					// If m_isNonResource, the depot path of the non-resource texture.

	CAsyncTextureBaker::CookFunctionPtr	m_cookFunction;

	Uint16								m_baseWidth;
	Uint16								m_baseHeight;
	GpuApi::eTextureFormat				m_format;
	GpuApi::eTextureType				m_textureType;

	Uint16								m_mipCount;
	Uint16								m_sliceCount;

	TDynArray< DataBuffer >				m_mipArray;				// Data for each mip/slice, copied from the original source.

	TDynArray< Uint32 >					m_mipPitch;				// Pitch of each mip/slice.

	Bool								m_isNonResource;		// Whether this task is for a resource or non-resource texture.

	// textureSource will not be kept by CBakeTask, so can be destroyed after task creation.
	STextureBakerTask( const String& debugName, Uint32 entry, const ITextureBakerSource& textureSource, CAsyncTextureBaker::CookFunctionPtr cookPtr )
		: m_debugName( debugName )
		, m_entry( entry )
		, m_cookFunction( cookPtr )
		, m_isNonResource( false )
	{
		Init( textureSource );
	}

	// textureSource will not be kept by CBakeTask, so can be destroyed after task creation.
	STextureBakerTask( const String& debugName, const String& path, const ITextureBakerSource& textureSource, CAsyncTextureBaker::CookFunctionPtr cookPtr )
		: m_debugName( debugName )
		, m_path( path )
		, m_cookFunction( cookPtr )
		, m_isNonResource( true )
	{
		Init( textureSource );
	}

	~STextureBakerTask()
	{
	}


private:
	void Init( const ITextureBakerSource& textureSource )
	{
		m_baseWidth		= textureSource.GetBaseWidth();
		m_baseHeight	= textureSource.GetBaseHeight();
		m_format		= textureSource.GetTextureFormat();
		m_textureType	= textureSource.GetTextureType();
		m_mipCount		= textureSource.GetMipCount();
		m_sliceCount	= textureSource.GetSliceCount();

		m_mipArray.Reserve( m_sliceCount * m_mipCount );
		m_mipPitch.Reserve( m_sliceCount * m_mipCount );

		for ( Uint16 tex_i = 0; tex_i < m_sliceCount; ++tex_i )
		{
			for ( Uint16 mip_i = 0; mip_i < m_mipCount; ++mip_i )
			{
				const Uint32 dataSize = textureSource.GetMipDataSize( mip_i, tex_i );
				const void* data = textureSource.GetMipData( mip_i, tex_i );

				m_mipArray.PushBack( DataBuffer( TDataBufferAllocator< MC_BufferBitmap >::GetInstance(), dataSize, data ) );
				m_mipPitch.PushBack( textureSource.GetMipPitch( mip_i, tex_i ) );
			}
		}
	}
};


//////////////////////////////////////////////////////////////////////////


class BakeTextureSource : public ITextureBakerSource
{
private:
	STextureBakerTask* m_task;

public:
	BakeTextureSource( STextureBakerTask* task )
		: m_task( task )
	{}


	virtual const void* GetMipData( Uint16 mip, Uint16 slice ) const override
	{
		return m_task->m_mipArray[ mip + slice * m_task->m_mipCount ].GetData();
	}

	virtual Uint32 GetMipDataSize( Uint16 mip, Uint16 slice ) const override
	{
		return m_task->m_mipArray[ mip + slice * m_task->m_mipCount ].GetSize();
	}

	virtual Uint16 GetSliceCount() const override
	{
		return m_task->m_sliceCount;
	}

	virtual Uint16 GetMipCount() const override
	{
		return m_task->m_mipCount;
	}

	virtual Uint32 GetMipPitch( Uint16 mip, Uint16 slice ) const override
	{
		return m_task->m_mipPitch[ mip + slice * m_task->m_mipCount ];
	}

	virtual Uint16 GetBaseWidth() const override
	{
		return m_task->m_baseWidth;
	}

	virtual Uint16 GetBaseHeight() const override
	{
		return m_task->m_baseHeight;
	}

	virtual GpuApi::eTextureFormat GetTextureFormat() const override
	{
		return m_task->m_format;
	}

	virtual GpuApi::eTextureType GetTextureType() const override
	{
		return m_task->m_textureType;
	}

	virtual Bool IsLooseFileTexture() const { return false; }
};


//////////////////////////////////////////////////////////////////////////


void CAsyncTextureBaker::Bake( const String& debugName, Uint32 entry, const ITextureBakerSource& textureSource, CAsyncTextureBaker::CookFunctionPtr cookerPtr )
{
	// Spawn task
	STextureBakerTask* task = new STextureBakerTask( debugName, entry, textureSource, cookerPtr );

#ifdef ASYNC_BAKING
	{
		// Locked access
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_listMutex );
		// Add to task list
		m_tasks.PushBack( task );
	}

	// Wake up the processing thread
	m_wakeUpSemaphore.Release( 1 );
#else
	//single thread option
	BakeTask( task );
	delete task;
#endif
}


void CAsyncTextureBaker::Bake( const String& debugName, const String& path, const ITextureBakerSource& textureSource, CookFunctionPtr cookerPtr )
{
	// Spawn task
	STextureBakerTask* task = new STextureBakerTask( debugName, path, textureSource, cookerPtr );

#ifdef ASYNC_BAKING
	{
		// Locked access
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_listMutex );
		// Add to task list
		m_tasks.PushBack( task );
	}

	// Wake up the processing thread
	m_wakeUpSemaphore.Release( 1 );
#else
	//single thread option
	BakeTask( task );
	delete task;
#endif
}


void CAsyncTextureBaker::BakeTask( STextureBakerTask* task )
{
	const Uint16 mipCount	= task->m_mipCount;
	const Uint16 sliceCount	= task->m_sliceCount;


	CTextureBakerOutput bakedData;

	if ( mipCount > 0 && sliceCount > 0 )
	{
		if ( !task->m_cookFunction( BakeTextureSource( task ), bakedData ) )
		{
			RED_LOG_ERROR( TextureCache, TXT("Failed to cook texture in cache '%ls'"), task->m_debugName.AsChar() );
		}
		else
		{
			RED_LOG_SPAM( TextureCache, TXT("Baked texture %u x %u with %u mips and %u slices..."), task->m_baseWidth, task->m_baseHeight, mipCount, sliceCount );
		}
	}


	const TDynArray< Uint32 >& cookedMipOffsets = bakedData.GetMipOffsets();
	const Uint32 numMipSegments = Max( cookedMipOffsets.Size(), MIPS_TO_COMBINE ) - MIPS_TO_COMBINE + 1;

	TDynArray< Uint32 > compressedMipOffsets( numMipSegments );
	
	// Compress the data.
	Red::Core::Compressor::CChainedZLib compressor;
	for ( Uint32 i = 0; i < numMipSegments; ++i )
	{
		const Uint32 mipOffset = cookedMipOffsets[ i ];
		const Uint32 mipEndOffset = ( i < numMipSegments - 1 ) ? cookedMipOffsets[ i + 1 ] : bakedData.GetTotalDataSize();
		const Uint32 mipSize = mipEndOffset - mipOffset;

		const Uint32 compressedMipOffset = compressor.GetResultSize();
		compressor.Compress( OffsetPtr( bakedData.GetData(), cookedMipOffsets[ i ] ), mipSize );

		compressedMipOffsets[ i ] = compressedMipOffset;
	}

	m_cooker->WriteEntryData( task->m_entry, compressor.GetResult(), compressor.GetResultSize(), bakedData.GetTotalDataSize(), bakedData.GetDataAlignment(), compressedMipOffsets );
}

STextureBakerTask* CAsyncTextureBaker::PopTask()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_listMutex );

	if (!m_tasks.Size())
		return nullptr;

	return m_tasks.PopBack();
}

void CAsyncTextureBaker::ThreadFunc()
{
	// Pop tasks
	for( ;; )
	{
		// Wait for signal
		m_wakeUpSemaphore.Acquire();

		if ( ! m_loop.GetValue() || GIsClosing )
		{
			break;
		}

		STextureBakerTask* newTask = PopTask();
		while ( newTask )
		{
			// Process only if not doing GC
			if ( GObjectGC->IsDoingGC() )
			{
				Sleep( 100 );
			}

			// Load the shit
			BakeTask( newTask );
			
			delete newTask;

			// Get the new task
			newTask = PopTask();

			// Async work has been done
			m_workDoneSemaphore.Release();
		}
	}
}

void CAsyncTextureBaker::Flush()
{
	// Info
	LOG_ENGINE( TXT("Flushing texture baker...") );

	// Get current time
	Double currentTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();

	// Wake up the baking thread
	m_wakeUpSemaphore.Release();

	// Keep waiting in a loop
	for ( ;; )
	{
		// Test condition
		Bool allTasksFinished = true;
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_listMutex );
			allTasksFinished = ( HasTasks() == false );
		}

		// All done
		if ( allTasksFinished )
		{
			break;
		}

		// Signal semaphore
		m_workDoneSemaphore.Acquire();
	}

	// Info
	Double endTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	RED_LOG_SPAM( TextureCache, TXT("Flushing async texture baker %1.2f s"), ( Float ) ( endTime - currentTime ) );
}

void CAsyncTextureBaker::Stop()
{
	m_loop.SetValue( false );
	m_wakeUpSemaphore.Release();
}

#endif
