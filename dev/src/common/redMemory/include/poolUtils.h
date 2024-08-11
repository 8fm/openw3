/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_INCLUDE_POOL_UTILS_H_
#define _RED_MEMORY_INCLUDE_POOL_UTILS_H_

#include "redMemoryInternal.h"
#include "poolTypes.h"

namespace red
{
namespace memory
{
	//////////////////////////////////////////////////////////////////////////
	// Initialize all the root pool. 
	// This function do need to be called, but it will set default budget and will add them to memory report. 
	RED_MEMORY_API void InitializeRootPools();

	// Initialize and Register PoolType. Optional only if allocator is DefaultAllocator.
	// For convenience, use RED_INITIALIZE_MEMORY_POOL macro.
	template< typename PoolType >
	void InitializePool( const PoolParameter & param, typename PoolType::AllocatorType & allocator );

	// Register Pool in metrics system. Called by InitializePool. No need to do both
	RED_MEMORY_API void RegisterPool( PoolHandle handle, const PoolParameter & param );

	//////////////////////////////////////////////////////////////////////////

	template< typename Pool >
	u64 GetPoolBudget();

	RED_MEMORY_API u64 GetPoolBudget( PoolHandle handle );

	template< typename Pool >
	const char* GetPoolName();
	
	RED_MEMORY_API const char * GetPoolName( PoolHandle handle );

	bool IsPoolRegistered( PoolHandle handle );

	u32 GetPoolCount();

	template< typename Pool >
	u64 GetPoolTotalBytesAllocated();

	//////////////////////////////////////////////////////////////////////////

	class OOMHandler;

	template< typename PoolType >
	void SetPoolOOMHandler( OOMHandler * oomHandler );

	//////////////////////////////////////////////////////////////////////////
	// Pool Resolver
	// Resolve at Compile Time the pool bond to a given Type
	// How-to: 
	// PoolResolver< MyObject >::PoolType is the correct Pool type
	// See poolUtils.hpp for implementation details.
	template< typename Type >
	struct PoolResolver;
}
}

#define _INTERNAL_RED_POOL_CONTEXT __PoolContext

//////////////////////////////////////////////////////////////////////////
// Add this helper in a public section of your Object to make operators use the specified pool.
#define RED_USE_MEMORY_POOL( poolName ) \
	typedef poolName _INTERNAL_RED_POOL_CONTEXT

#define RED_INITIALIZE_MEMORY_POOL( poolType, poolParentType, allocator, budget ) \
	do{  red::memory::PoolParameter param = { #poolType, &red::memory::StaticPoolStorage< poolType >::storage, budget, poolParentType::GetHandle() }; red::memory::InitializePool< poolType >( param, allocator ); } while(0,0)

#include "poolUtils.hpp"

#endif
