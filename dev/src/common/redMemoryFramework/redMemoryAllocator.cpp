/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redMemoryAllocator.h"
#include "redMemoryCrt.h"

namespace Red { namespace MemoryFramework {

	//////////////////////////////////////////////////////////////////
	// CTor
	//
	IAllocator::IAllocator()
		: m_flags( 0 )
	{

	}

	//////////////////////////////////////////////////////////////////
	// DTor
	//
	IAllocator::~IAllocator()
	{

	}

} }
