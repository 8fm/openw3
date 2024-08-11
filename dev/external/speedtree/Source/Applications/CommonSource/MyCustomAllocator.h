///////////////////////////////////////////////////////////////////////  
//  MyCustomAllocator.h
//
//	All source files prefixed with "My" indicate an example implementation, 
//	meant to detail what an application might (and, in most cases, should)
//	do when interfacing with the SpeedTree SDK.  These files constitute 
//	the bulk of the SpeedTree Reference Application and are not part
//	of the official SDK, but merely example or "reference" implementations.
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#pragma once
#include <cstdlib>
#include <exception>
#include "Core/Memory.h"
#include "Utilities/Utility.h"
#ifdef __ORBIS__
	#include "Renderers/Orbis/OrbisRenderer.h"
#endif


///////////////////////////////////////////////////////////////////////  
//  Overloaded global new

#ifdef SPEEDTREE_OVERRIDE_GLOBAL_NEW_AND_DELETE

void* operator new(size_t siBytes)
{ 
	return malloc(siBytes);
}


///////////////////////////////////////////////////////////////////////  
//  Overloaded global new[]

void* operator new[](size_t siBytes)
{
	return malloc(siBytes);
}


///////////////////////////////////////////////////////////////////////  
//  Overloaded global delete

void operator delete(void* pBlock)
{
	if (pBlock)
	{
		return free(pBlock);
	}
}


///////////////////////////////////////////////////////////////////////  
//  Overloaded global delete[]

void operator delete[](void* pBlock)
{
	if (pBlock)
	{
		return free(pBlock);
	}
}

#endif // SPEEDTREE_OVERRIDE_GLOBAL_NEW_AND_DELETE


///////////////////////////////////////////////////////////////////////  
//  class CCustomAllocator

#ifdef SPEEDTREE_USE_ALLOCATOR_INTERFACE

class CMyCustomAllocator : public SpeedTree::CAllocator
{
public:
		void* Alloc(size_t siBlockSize, SpeedTree::EAllocationType eType)
		{
			ST_UNREF_PARAM(eType); // eType is a hint to indicate a long-term or temporary heap allocation
			
			#ifdef __ORBIS__
				return SpeedTree::Orbis::Allocate(siBlockSize, sce::Gnm::kAlignmentOfBufferInBytes, false);
			#else
				return malloc(siBlockSize);
			#endif
		}

		void Free(void* pBlock)
		{
			if (pBlock)
			{
				#ifdef __ORBIS__
					SpeedTree::Orbis::Release(pBlock);
				#else
					return free(pBlock);
				#endif
			}
		}
};

#endif // SPEEDTREE_USE_ALLOCATOR_INTERFACE

