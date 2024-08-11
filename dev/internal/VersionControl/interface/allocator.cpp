/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "allocator.h"

namespace VersionControl
{
	Allocator::AllocatorFunc Allocator::m_alloc = nullptr;
	Allocator::FreeFunc Allocator::m_free = nullptr;

	void Allocator::SetCustom( AllocatorFunc afunc, FreeFunc ffunc )
	{
		m_alloc = afunc;
		m_free = ffunc;
	}
}
