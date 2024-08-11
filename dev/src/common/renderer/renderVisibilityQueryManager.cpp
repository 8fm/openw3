#include "build.h"
#include "renderVisibilityQueryManager.h"

CRenderVisibilityQueryManager::CRenderVisibilityQueryManager()
{
	m_previousFrame = (Uint8*) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_RenderObjects, MAX_QUERIES );
	m_currentFrame = (Uint8*) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_RenderObjects, MAX_QUERIES );
}

CRenderVisibilityQueryManager::~CRenderVisibilityQueryManager()
{
	// it's safe to delete - the NextFrame cannot be called while we are in here
	RED_MEMORY_FREE( MemoryPool_Default, MC_RenderObjects, m_previousFrame );
	RED_MEMORY_FREE( MemoryPool_Default, MC_RenderObjects, m_currentFrame );
}

void CRenderVisibilityQueryManager::FrameFinished()
{
	// let the engine use the generated data
	::Swap( m_currentFrame, m_previousFrame );
	
	// cleanup for new frame that will come
	Red::MemoryZero( m_currentFrame, MAX_QUERIES );
}

void CRenderVisibilityQueryManager::MarkQuery( const TRenderVisibilityQueryID id, const EFlags flags )
{
	RED_ASSERT( id > 0 && id < MAX_QUERIES, TXT("Invalid visibility query ID") );
	if ( id < MAX_QUERIES )
	{
		const Uint32 word = id / 4;
		const Uint32 shift = 8 * (id & 3); // LE only
		const Uint32 mask = flags << shift;  

		Red::Threads::AtomicOps::TAtomic32* table = (Red::Threads::AtomicOps::TAtomic32*) m_currentFrame;
		Red::Threads::AtomicOps::Or32( table + word, mask );
	}
}

ERenderVisibilityResult CRenderVisibilityQueryManager::TestQuery( const TRenderVisibilityQueryID id ) const
{
	RED_ASSERT( id > 0 && id < MAX_QUERIES, TXT("Invalid visibility query ID") );
	ERenderVisibilityResult ret = RVR_NotVisible;

	if ( id < MAX_QUERIES )
	{
		const Uint8 marker = m_previousFrame[ id ];
		if ( marker & VisibleScene )
		{
			// visible in scene
			ret = RVR_Visible;
		}
		else if ( marker & (VisibleAdditionalShadows | VisibleMainShadows) )
		{
			// visible in shadows for sure
			ret = RVR_PartialyVisible;
		}
	}

	return ret;
}

Bool CRenderVisibilityQueryManager::TestQuery( Uint32 elementsCount, Int16* indexes, void* inputPos, void* outputPos, size_t stride ) const
{
	for( Uint32 i = 0; i != elementsCount; ++i )
	{
		Uint16 index = indexes[ i ];
		TRenderVisibilityQueryID input = ( TRenderVisibilityQueryID )( ( *( ( Uint32* ) ( ( ( Uint8* ) inputPos ) + stride * index ) ) ) & 0x00FFFFFF );
		Uint8* output = ( ( ( Uint8* ) outputPos ) + stride * index );
		if( !input ) continue;
		if ( input >= MAX_QUERIES ) continue;

		const Uint8 marker = m_previousFrame[ input ];
		if ( marker & VisibleScene )
		{
			// visible in scene
			*output = ( *output & 0xFC ) | ( Uint8 ) RVR_Visible;
		}
		else if ( marker & (VisibleAdditionalShadows | VisibleMainShadows) )
		{
			// visible in shadows for sure
			*output = ( *output & 0xFC ) | ( Uint8 ) RVR_PartialyVisible;
		}
		else
		{
			*output = ( *output & 0xFC ) | ( Uint8 ) RVR_NotVisible;
		}
	}
	return true;
}

TRenderVisibilityQueryID CRenderVisibilityQueryManager::AllocQuerryId()
{
	Red::Threads::CScopedLock< TLock > lock( m_lock );
	return (TRenderVisibilityQueryID) m_allocator.Alloc();
}

void CRenderVisibilityQueryManager::ReleaseQueryId( TRenderVisibilityQueryID id )
{
	Red::Threads::CScopedLock< TLock > lock( m_lock );
	m_allocator.Release( id );
}