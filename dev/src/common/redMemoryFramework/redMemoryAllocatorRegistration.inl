/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_ALLOCATOR_REGISTRATION_INL
#define _RED_MEMORY_ALLOCATOR_REGISTRATION_INL

namespace Red { namespace MemoryFramework {

////////////////////////////////////////////////////////////////////
// GetLabelForIndex
//	For now, labels are mapped directly to indices
RED_INLINE PoolLabel AllocatorManager::GetLabelForIndex( Red::System::Uint16 poolIndex )
{
	return static_cast< PoolLabel >( poolIndex );
}

////////////////////////////////////////////////////////////////////
// GetAllocator
//	Fast lookup for allocator by label
RED_INLINE Red::System::Uint16 AllocatorManager::GetAllocatorCount()
{
	return m_allocatorCount;
}

////////////////////////////////////////////////////////////////////
// GetAllocator
//	Fast lookup for allocator by label
RED_INLINE IAllocator* AllocatorManager::GetAllocatorByLabel( PoolLabel label ) const
{
	if( label < k_MaximumPools )
	{
		return m_allocators[label];
	}

	return nullptr;
}

} }

#endif