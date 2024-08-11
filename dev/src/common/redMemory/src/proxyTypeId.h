/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_PROXY_TYPE_ID_H_
#define _RED_MEMORY_PROXY_TYPE_ID_H_

namespace red
{
namespace memory
{
	typedef u32 ProxyTypeId;

	template< typename Allocator, typename >
	struct ProxyHasTypeId;

	// With constexpr it would be possible to develop a nice compile time unique id with allocator name.
	// However it requires MSVC 2015. Until then, I trust myself to use uniqueId.
#define RED_MEMORY_PROXY_TYPE_ID( uniqueId ) enum TypeIdEnum : ProxyTypeId { TypeId = uniqueId }

}
}

#include "proxyTypeId.hpp"

#endif
