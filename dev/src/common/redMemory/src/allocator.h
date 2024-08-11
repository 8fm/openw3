/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_ALLOCATOR_H_
#define _RED_MEMORY_ALLOCATOR_H_

#include "proxyTypeId.h"
#include "proxy.h"

#define RED_MEMORY_DECLARE_ALLOCATOR( name, id, defaultAlignment )	\
			RED_MEMORY_DECLARE_PROXY( name, defaultAlignment );		\
			RED_MEMORY_PROXY_TYPE_ID( id )
			
#endif
