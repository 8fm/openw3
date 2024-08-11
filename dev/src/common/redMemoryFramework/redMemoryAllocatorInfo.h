/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_ALLOCATOR_INFO_H
#define _RED_MEMORY_ALLOCATOR_INFO_H
#pragma once

#include "redMemoryFrameworkTypes.h"

namespace Red { namespace MemoryFramework { 

/////////////////////////////////////////////////////////////////////
// AllocatorInfo provides high-level allocator data to the metrics system
// This will be filled out by the allocators when requested
class AllocatorInfo
{
public:
	AllocatorInfo();
	~AllocatorInfo();

	void SetAllocatorTypeName( const Red::System::Char* typeName );
	void SetAllocatorBudget( Red::System::MemSize budget );
	void SetPerAllocationOverhead( Red::System::Uint32 overhead );
	Red::System::MemSize GetBudget();
	const Red::System::Char* GetTypeName();
	Red::System::Uint32 GetPerAllocationOverhead();

private:
	static const Red::System::Uint32 c_allocatorTypeLength = 32;

	Red::System::MemSize	m_totalBudget;
	Red::System::Uint32		m_perAllocationOverhead;
	Red::System::Char		m_allocatorType[ c_allocatorTypeLength ];
};

/////////////////////////////////////////////////////////////////////
// AllocatorWalker
//	Used to get detailed information about the various areas of memory
//	that a allocator manages
class AllocatorWalker
{
public:
	virtual void OnMemoryArea( Red::System::MemUint address, Red::System::MemSize size ) = 0;
};

/////////////////////////////////////////////////////////////////////
// PoolAreaWalker
//	Used to get detailed information about the state of an area in a pool
class PoolAreaWalker
{
public:
	virtual void OnUsedArea( Red::System::MemUint address, Red::System::MemSize size, Red::System::Uint16 memoryClass ) = 0;
	virtual void OnFreeArea( Red::System::MemUint address, Red::System::MemSize size ) = 0;
	virtual void OnLockedArea( Red::System::MemUint address, Red::System::MemSize size ) = 0;		// only valid for certain allocator types
};

/////////////////////////////////////////////////////////////////////
// AllocatorAreaCallback
//	Used to catch times when areas are added / removed from a pool
class AllocatorAreaCallback
{
public:
	virtual void OnAreaAdded( Red::System::MemUint lowAddress, Red::System::MemUint highAddress ) = 0;
	virtual void OnAreaRemoved( Red::System::MemUint lowAddress, Red::System::MemUint highAddress ) = 0;
};

} }

#include "redMemoryAllocatorInfo.inl"

#endif
