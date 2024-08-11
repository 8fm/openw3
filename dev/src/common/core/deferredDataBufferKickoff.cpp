/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "depot.h"
#include "depotBundles.h"
#include "diskBundle.h"
#include "deferredDataBuffer.h"
#include "deferredDataBufferLoaders.h"
#include "deferredDataBufferAsync.h"
#include "deferredDataBufferKickoff.h"

//#undef RED_FATAL_ASSERT
//#define RED_FATAL_ASSERT(x, expr) if ( !(x) ) { fwprintf( stderr, L"CRASH: %hs\n", #expr ); __debugbreak(); }

// TODO: optimize... 

CDeferredDataBufferKickOffList::CDeferredDataBufferKickOffList()
	: m_isProcessing( false )
	, m_isKickingNewJobs( false )
{
	m_callbacks.Reserve( 1024 );
}

void CDeferredDataBufferKickOffList::RegisterCallback( BufferAsyncData* callback )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
	RED_FATAL_ASSERT( m_isProcessing == false, "New callback added while processing is in place" );

	m_callbacks.Insert( callback );
	callback->AddRef();
}

Uint32 CDeferredDataBufferKickOffList::ReleaseCallbackRef( Red::Threads::CAtomic< Int32 >* refCount, class BufferAsyncData* data )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// release reference
	Uint32 ret = refCount->Decrement();

	// that was the last reference
	if ( ret == 0 )
	{
		// make sure we are NOT ON ANY LIST
		for ( auto callback : m_callbacks )
		{
			RED_FATAL_ASSERT( callback != data, "Removed callback that is on the list" );
		}

		for ( auto callback : m_toRemove)
		{
			RED_FATAL_ASSERT( callback != data, "Removed callback that is on the list" );
		}
	}

	// return new reference count
	return ret;
}

void CDeferredDataBufferKickOffList::UnregisterCallback( BufferAsyncData* callback )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
	
	// we need to remove this from the array outside the processing loop
	// when inside the processing loop we capture all the cases when the object is being removed
	if ( !m_isProcessing )
	{
		m_callbacks.Erase( callback );
		if ( 0 == callback->Release() )
			delete callback;
	}
	else
	{
		m_toRemove.PushBack( callback );
	}
}

void CDeferredDataBufferKickOffList::KickNewJobs()
{
	PC_SCOPE_PIX( KickNewJobs );

	if ( m_isKickingNewJobs.CompareExchange( true, false ) == true )
	{
		// Already doing this elsewhere
		return;
	}

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// lock direct update of the callback list
	m_toRemove.ClearFast();
	m_isProcessing = true;

	// process the callbacks
	for ( auto callback : m_callbacks )
	{
		// for some reason the terrain streaming can cancel ANOTHER callbacks, especially during world transition
		// this check is necessary to ensure no crashes, the array size is typically VERY small (0-2 elements)
		if ( !m_toRemove.Exist( callback ) )
			callback->Kick();
	}

	// unlock direct update of the callback list
	m_isProcessing = false;

	// delete object that got release DURING kickoff iteration	
	if ( !m_toRemove.Empty() )
	{
		auto toRemove = std::move(m_toRemove);

		for ( auto callback : toRemove )
		{
			m_callbacks.Erase( callback );

			if ( 0 == callback->Release() )
				delete callback;
		}

		m_toRemove = std::move(toRemove);
		m_toRemove.ClearFast();
	}

	m_isKickingNewJobs.SetValue( false );
}


