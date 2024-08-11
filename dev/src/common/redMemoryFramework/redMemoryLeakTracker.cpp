/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryLeakTracker.h"
#include "redMemoryAtomics.h"

namespace Red { namespace MemoryFramework {

///////////////////////////////////////////////////////////////////
// CTor
//
MemoryLeakTracker::MemoryLeakTracker()
	: m_deadNodes(nullptr)
	, m_isTrackingAllocations(true)
{
	Red::System::MemorySet( m_nodeBuffer, 0, sizeof( m_nodeBuffer ) );

	// Create the list of dead nodes
	for( Red::System::Uint32 i=0; i<c_maximumAllocationsToTrack-1; ++i )
	{
		m_nodeBuffer[i].m_nextAllocation = &m_nodeBuffer[i+1];
	}
	m_nodeBuffer[ c_maximumAllocationsToTrack-1 ].m_nextAllocation = nullptr;
	m_deadNodes = &m_nodeBuffer[0];

	for( Red::System::Uint32 bucketIndex = 0; bucketIndex < c_BucketCount; ++bucketIndex )
	{
		m_activeNodes[ bucketIndex ] = nullptr;
	}
}

///////////////////////////////////////////////////////////////////
// DTor
//
MemoryLeakTracker::~MemoryLeakTracker()
{

}

///////////////////////////////////////////////////////////////////
// ResetLeaksList
//	Clear the leaks list
void MemoryLeakTracker::ResetLeaksList()
{
	m_mutex.Acquire();

	// Push the head of each active bucket to the dead list, and stitch up the back pointers
	for( Red::System::Uint32 bucketIndex = 0; bucketIndex < c_BucketCount; ++bucketIndex )
	{
		TrackedAllocation* lastActive = m_activeNodes[ bucketIndex ];
		if( lastActive == nullptr )
		{
			continue;
		}

		while( lastActive->m_nextAllocation != nullptr )
		{
			lastActive = lastActive->m_nextAllocation;
		}

		lastActive->m_nextAllocation = m_deadNodes;
		m_deadNodes = lastActive->m_nextAllocation;
		m_activeNodes[ bucketIndex ] = nullptr;
	}
	
	m_mutex.Release();
}

///////////////////////////////////////////////////////////////////
// OnAllocation
//	Adds a new allocation to the leak list
void MemoryLeakTracker::OnAllocation( void* ptr, Red::System::MemSize size, MemoryClass memoryClass, const MetricsCallstack& callstack )
{
	m_mutex.Acquire();
	if( m_deadNodes != nullptr && m_isTrackingAllocations )
	{
		// Pop from dead nodes list
		TrackedAllocation* newAllocation = m_deadNodes;
		m_deadNodes = newAllocation->m_nextAllocation;

		// Add to active nodes list
		Red::System::Int32 bucketIndex = CalculateBucket( size );
		RED_MEMORY_ASSERT( bucketIndex < c_BucketCount,  "Bad bucket index!" );
		newAllocation->m_allocID = GetNewAllocationID();
		newAllocation->m_allocSize = size;
		newAllocation->m_memoryClass = memoryClass;
		newAllocation->m_ptr = ptr;
		newAllocation->m_callStack = callstack;
		newAllocation->m_nextAllocation = m_activeNodes[ bucketIndex ];		// Push to head of active list
		m_activeNodes[ bucketIndex ] = newAllocation;
	}
	m_mutex.Release();
}

///////////////////////////////////////////////////////////////////
// OnFree
//	Remove a allocation from the leak list
//	Note, this is slow!
void MemoryLeakTracker::OnFree( const void* ptr, Red::System::MemSize size )
{
	m_mutex.Acquire();
	
	Red::System::Int32 bucketIndex = CalculateBucket( size );
	RED_MEMORY_ASSERT( bucketIndex < c_BucketCount, "Bad bucket index!" );
	TrackedAllocation* activeNode = m_activeNodes[ bucketIndex ];
	TrackedAllocation* previousActive = nullptr;
	while( activeNode != nullptr )
	{
		if( activeNode->m_ptr == ptr )
		{
			// Remove from active nodes list
			if( previousActive != nullptr )
			{
				previousActive->m_nextAllocation = activeNode->m_nextAllocation;
			}
			else
			{
				// It must be the head of the node list, push the head along
				m_activeNodes[ bucketIndex ] = activeNode->m_nextAllocation;
			}

			// Push the node to the dead list
			activeNode->m_nextAllocation = m_deadNodes;
			m_deadNodes = activeNode;			

			m_mutex.Release();
			return ;
		}

		previousActive = activeNode;
		activeNode = activeNode->m_nextAllocation;
	}

	m_mutex.Release();
}

///////////////////////////////////////////////////////////////////
// AquireUnfreedAllocationsList
//	Get the head of the leaks list (and locks the mutex!)
TrackedAllocation* MemoryLeakTracker::AquireUnfreedAllocationsList( Red::System::Int32 bucketIndex )
{
	m_mutex.Acquire();
	RED_MEMORY_ASSERT( bucketIndex < c_BucketCount, "Invalid bucket index!" );
	return m_activeNodes[ bucketIndex ];
}

///////////////////////////////////////////////////////////////////
// UnacquireUnfreedAllocationsList
//	Unlock the list mutex
void MemoryLeakTracker::UnacquireUnfreedAllocationsList()
{
	m_mutex.Release();
}

///////////////////////////////////////////////////////////////////
// EnableAllocationTracking
//	Turn on allocation tracking (frees are always tracked!)
void MemoryLeakTracker::EnableAllocationTracking()
{
	m_mutex.Acquire();
	m_isTrackingAllocations = true;
	m_mutex.Release();
}

///////////////////////////////////////////////////////////////////
// DisableAllocationTracking
//	Turn off allocation tracking (frees are always tracked!)
void MemoryLeakTracker::DisableAllocationTracking()
{
	m_mutex.Acquire();
	m_isTrackingAllocations = false;
	m_mutex.Release();
}


} }