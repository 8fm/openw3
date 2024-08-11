/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redMemoryFramework/redMemoryCallbackList.h"

namespace Helper
{
	/// We use the callstack from memory system
	struct ObjectCallstack
	{
		Red::MemoryFramework::MetricsCallstack		m_callstack;
		Bool										m_mainThread;

		RED_FORCE_INLINE ObjectCallstack()
			: m_callstack(4)
			, m_mainThread( ::SIsMainThread() )
		{}
	};

	/// Tracker for object allocations
	class CObjectAllocationTracker
	{
	public:
		CObjectAllocationTracker();

		// Register allocation of the object - this will capture the callstack this object was created with
		void RegisterObject( const Uint32 objectIndex );

		// Register deallocation of the object - this will discard callstack information for the object
		void UnregisterObject( const Uint32 objectIndex );

		// Dump list of callstacks to file
		bool DumpCallstackInfo( const TDynArray<CObject*, MC_Engine>& objects ) const;

	private:
		TDynArray< ObjectCallstack* >		m_callstacks;
		mutable Red::Threads::CMutex		m_lock;

		// assemble file path for output dump file
		static void AssembleFilePath( String& outFilePath );
	};

	// tracker singleton
	typedef TSingleton< CObjectAllocationTracker > SObjectAllocationTracker;

} // Helpers