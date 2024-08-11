/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "exports.h"

namespace DDI
{
	void* Realloc( void* ptr, size_t size );
	void Free( const void* ptr );

	void SetAllocators( ReallocatorFunc reallocFunc, FreeFunc freeFunc );
}