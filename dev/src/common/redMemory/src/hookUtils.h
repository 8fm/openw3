/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_HOOK_UTILS_H_
#define _RED_MEMORY_HOOK_UTILS_H_

namespace red
{
namespace memory
{
	struct Block;
	class HookHandler;

	//////////////////////////////////////////////////////////////////////////

	void InitializePermanentHooks( HookHandler & handler );

	//////////////////////////////////////////////////////////////////////////

	template< typename PoolType >
	void ProcessPreAllocateHooks( u32 & size );

	template< typename ProxyType >
	void ProcessPreAllocateHooks( const ProxyType & proxy, u32 & size );

	//////////////////////////////////////////////////////////////////////////

	template< typename PoolType >
	void ProcessPreFreeHooks( Block & block );

	template< typename ProxyType >
	void ProcessPreFreeHooks( const ProxyType & proxy, Block & block );

	//////////////////////////////////////////////////////////////////////////

	template< typename PoolType >
	void ProcessPreReallocateHooks( Block & block, u32 & size );

	template< typename ProxyType >
	void ProcessPreReallocateHooks( const ProxyType & proxy, Block & block, u32 & size );

	//////////////////////////////////////////////////////////////////////////

	template< typename PoolType >
	void ProcessPostAllocateHooks( Block & block );

	template< typename ProxyType >
	void ProcessPostAllocateHooks( const ProxyType & proxy, Block & block );

	template< typename PoolType >
	void ProcessPostReallocateHooks( Block & inputBlock, Block & outputBlock );

	template< typename ProxyType >
	void ProcessPostReallocateHooks( const ProxyType & proxy, Block & inputBlock, Block & outputBlock );
}
}

#include "hookUtils.hpp"

#endif
