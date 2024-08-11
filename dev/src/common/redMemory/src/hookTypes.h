/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_HOOK_TYPES_H_
#define _RED_MEMORY_HOOK_TYPES_H_

#include "proxyTypeId.h"
#include "block.h"
#include "../include/poolTypes.h"

namespace red
{
namespace memory
{
	struct Block;
	struct HookPreParameter;
	struct HookPostParameter;

	typedef u64 HookHandle;

	typedef void (*HookPreCallback)( HookPreParameter &, void* );
	typedef void (*HookPostCallback)( HookPostParameter &, void* );

	struct HookCreationParameter
	{
		HookPreCallback preCallback;
		HookPostCallback postCallback;
		void * userData;
	};

	struct HookProxyParameter
	{
		PoolHandle poolHandle;
		ProxyTypeId id;
		u64 address;
	};

	struct HookPreParameter
	{
		Block * block;
		u32 * size;
		HookProxyParameter proxy;
	};

	struct HookPostParameter
	{
		Block * inputBlock;
		Block * outputBlock;
		HookProxyParameter proxy;
	};

	// FOR UNIT TEST
	RED_MEMORY_API bool operator==( const HookProxyParameter & left, const HookProxyParameter & right );
	RED_MEMORY_API bool operator==( const HookPreParameter & left, const HookPreParameter & right );
	RED_MEMORY_API bool operator==( const HookPostParameter & left, const HookPostParameter & right );
	
}
}

#endif
