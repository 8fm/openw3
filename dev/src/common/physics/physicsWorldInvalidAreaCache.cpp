#include "build.h"
#include "physicsWorldInvalidAreaCache.h"

CPhysicsWorldInvalidAreaCache::CPhysicsWorldInvalidAreaCache()
	: m_cachedBoxesBuffer( 0 )
{
}

const Bool CPhysicsWorldInvalidAreaCache::TestPoint_Cached( const Vector& worldPoint ) const
{
	// NOTE: this is not 100% thread safe but it's safe enough (aka. no crashes will occur from this ever)
	const Uint32 buf = m_cachedBoxesBuffer.GetValue() & 1;
	for ( Uint32 i=0; i<MAX_BOXES; ++i )
	{
		const Box& box = m_cachedBoxes[buf][i];
		if ( box.IsEmpty() )
			break; // end of the list

		if ( box.Contains( worldPoint ) )
			return true;
	}

	// not found
	return false;
}

const Bool CPhysicsWorldInvalidAreaCache::TestBox_Cached( const Box& worldBox ) const
{
	// NOTE: this is not 100% thread safe but it's safe enough (aka. no crashes will occur from this ever)
	const Uint32 buf = m_cachedBoxesBuffer.GetValue() & 1;
	for ( Uint32 i=0; i<MAX_BOXES; ++i )
	{
		const Box& box = m_cachedBoxes[buf][i];
		if ( box.IsEmpty() )
			break; // end of the list

		if ( box.Touches( worldBox ) )
			return true;
	}

	// not found
	return false;
}

const Bool CPhysicsWorldInvalidAreaCache::TestPoint_Current( const Vector& worldPoint ) const
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// extract valid boxes
	return m_boxesMask.VisitSetBitsEarlyExit(
		[this, worldPoint]( const Uint32 boxIndex )
		{
			const Box& box = m_boxes[boxIndex];
			return box.Contains( worldPoint );
		}
	);
}

const Bool CPhysicsWorldInvalidAreaCache::TestBox_Current( const Box& worldBox ) const
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// extract valid boxes
	return m_boxesMask.VisitSetBitsEarlyExit(
		[this, worldBox]( const Uint32 boxIndex )
		{
			const Box& box = m_boxes[boxIndex];
			return box.Touches( worldBox );
		}
	);
}

void CPhysicsWorldInvalidAreaCache::GetBoxes_Current( TDynArray< Box >& outCurrentAreas ) const
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	outCurrentAreas.Reserve( m_boxesID.GetNumAllocated() );

	// extract valid boxes
	m_boxesMask.VisitSetBits(
		[&outCurrentAreas, this]( const Uint32 boxIndex )
		{
			outCurrentAreas.PushBack( m_boxes[boxIndex] );
		}
	);
}

void CPhysicsWorldInvalidAreaCache::GetBoxes_Cached( TDynArray< Box >& outCurrentAreas ) const
{
	const Uint32 buf = m_cachedBoxesBuffer.GetValue() & 1;
	for ( Uint32 i=0; i<MAX_BOXES; ++i )
	{
		const Box& box = m_cachedBoxes[buf][i];
		if ( box.IsEmpty() )
			break; // end of the list

		outCurrentAreas.PushBack( box );
	}
}

Uint32 CPhysicsWorldInvalidAreaCache::AddBox( const Box& meshBox, const Matrix& localToWorld )
{
	const Box worldBox = localToWorld.TransformBox( meshBox );

	{
		Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

		// allocate box ID
		Uint32 id = m_boxesID.Alloc();
		if ( id == 0 )
			return 0;

		// place the box
		RED_FATAL_ASSERT( m_boxesMask.Get(id) == false, "Box ID is already allocated" );
		m_boxes[ id ] = worldBox;
		m_boxesMask.Set( id );

		return id;
	}
}

void CPhysicsWorldInvalidAreaCache::RemoveBox( Uint32 areaIndex )
{
	// invalid ID, handle silently
	if ( !areaIndex )
		return;

	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// should be allocated
	RED_FATAL_ASSERT( m_boxesMask.Get(areaIndex) == true, "Box ID is not allocated buy we try to free it" );

	// erase
	m_boxesID.Release( areaIndex );
	m_boxesMask.Clear( areaIndex );
	m_boxes[ areaIndex ].Clear();
}

void CPhysicsWorldInvalidAreaCache::CacheCurrentData()
{
	// copy data
	{
		Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

		const Uint32 nextBuf = (m_cachedBoxesBuffer.GetValue()+1) & 1;

		Uint32 writeIndex = 0;

		// extract valid boxes
		m_boxesMask.VisitSetBits(
			[this, nextBuf, &writeIndex]( const Uint32 boxIndex )
			{
				m_cachedBoxes[ nextBuf ][ writeIndex ] = m_boxes[ boxIndex ];
				writeIndex += 1;
			}
		);

		// EOF
		if ( writeIndex < MAX_BOXES )
			m_cachedBoxes[ nextBuf ][ writeIndex ].Clear();
	}

	// swap buffers
	m_cachedBoxesBuffer.Increment();
}
