/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "../core/idAllocator.h"
#include "../core/bitset.h"

//////////////////////////////////////////////////////////////////////////////////////
/// Helper class that handles "invalid area" stuff for physical world
class CPhysicsWorldInvalidAreaCache
{
public:
	CPhysicsWorldInvalidAreaCache();

	// add remove invalid area to the list of "blockers" in the physics scene
	Uint32 AddBox( const Box& meshBox, const Matrix& localToWorld );	
	void RemoveBox( Uint32 areaIndex );

	// positional query, the _Cached version is much faster but works with data that is delayed by one frame
	const Bool TestPoint_Cached( const Vector& worldPoint ) const;
	const Bool TestBox_Cached( const Box& worldBox ) const;

	// positional query, the _Current version is slower but always checks agains the current data
	const Bool TestPoint_Current( const Vector& worldPoint ) const;
	const Bool TestBox_Current( const Box& worldBox ) const;

	// get the lists (for debug mostly)
	void GetBoxes_Current( TDynArray< Box >& outCurrentAreas ) const;
	void GetBoxes_Cached( TDynArray< Box >& outCurrentAreas ) const;

	// cache the current list
	void CacheCurrentData();

private:
	static const Uint32 MAX_BOXES = 4096;

	mutable Red::Threads::CLightMutex	m_lock;

	Box									m_boxes[ MAX_BOXES ];
	IDAllocator< MAX_BOXES >			m_boxesID;
	TBitSet64< MAX_BOXES >				m_boxesMask;

	Box 								m_cachedBoxes[ 2 ][ MAX_BOXES ];
	Red::Threads::CAtomic< Int32 >		m_cachedBoxesBuffer;
};
