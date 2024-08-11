/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef BUFFER_ALLOCATOR_NULL_H
#define BUFFER_ALLOCATOR_NULL_H
#pragma once

#include "containersCommon.h"

///////////////////////////////////////////////////////////////////////
// BufferAllocatorPolicy is used to manage a single, contiguous buffer of
// homogenous data.
// BufferAllocatorPolicyNull is an empty buffer policy class, just to document the interface
class BufferAllocatorPolicyNull
{
public:
	// Default CTor / DTor must be implemented
	BufferAllocatorPolicyNull();
	~BufferAllocatorPolicyNull();

	// Element Buffer accessors
	void* Data();
	const void* Data() const;

	// Buffer size properties
	Red::System::Uint32 Size() const;			// Size in elements
	Red::System::Uint32 Capacity() const;		// Capacity in elements

protected:
	// Move() must be implemented and act like a move constructor 
	void MoveBuffer( BufferAllocatorPolicyNull& other, Red::MemoryFramework::MemoryClass otherMemClass );

	// Resize functions should generally stick to the elementCount provided
	void ResizeBuffer( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass );
	void ResizeFast( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass );
	void Reserve( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass );		// Increase capacity only

	// Grow functions return the *old* size. Grow can grow by any amount, as long as it is big enough to include elementCount
	Red::System::Uint32 GrowBuffer( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass );
};

#endif