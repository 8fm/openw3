/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "engineTime.h"

// Generic "soft" fence - not to be used for any kind of hard ass synchronization
// You can have up to 32 internal fences. Once the fence is set it cannot be unset.
// The example usage case: quest waiting for the gameplay layers to finish loading.
// The object itself is thread safe but is NOT using any kind of proper signalling so active waiting is not recommended.
class CGenericFence
{
public:
	CGenericFence( const AnsiChar* debugName ); // set refcount=1

	//! Internal thread safe reference counting
	void AddRef();
	void Release();	

	//! Signal the fence
	void Signal( const Uint32 mask );

	//! Test if particular flag combination is set
	//! Returns the mask of current bits with the given bits
	const Uint32 Test( const Uint32 mask ) const;

	//! Debug functionality: get the time passed since the fence was created
	const Double GetDebugTimeSinceStart() const;

	//! Debug functionality: get internal fence name
	const AnsiChar* GetDebugName() const;

private:
	~CGenericFence();

	// internal refcount
	Red::Threads::AtomicOps::TAtomic32		m_refCount;

	// internal signal mask - 32-bits
	Red::Threads::AtomicOps::TAtomic32		m_flags;

	// engine time when the fence was created
	const AnsiChar*							m_debugName;
	EngineTime								m_debugCreationTime;
};

// Generic "soft" counted fence - will signal the internal fence only after given amount of events occured
// The example usage case: quest waiting for the gameplay layers to finish loading.
// Event reporting is thread safe
// Fence is thread safe
class CGenericCountedFence
{
public:
	CGenericCountedFence( const AnsiChar* debugName, const Uint32 expectedCount ); // set refcount=1

	//! Internal thread safe reference counting
	void AddRef();
	void Release();	

	//! Count the events, overflowing the fence is an offense :)
	void Signal();

	//! Test if the fence is completed
	//! Returns the mask of current bits with the given bits
	const Bool Test() const;

	//! Debug functionality: get the time passed since the fence was created
	const Double GetDebugTimeSinceStart() const;

	//! Debug functionality: get the time elapsed between fence creation and final unlock
	const Double GetDebugTimeWaiting() const;

	//! Debug functionality: get internal fence name
	const AnsiChar* GetDebugName() const;

private:
	~CGenericCountedFence();

	// internal refcount
	Red::Threads::AtomicOps::TAtomic32		m_refCount;

	// internal signal counter
	Red::Threads::AtomicOps::TAtomic32		m_counter;
	volatile bool							m_flag;

	// engine time when the fence was created
	const AnsiChar*							m_debugName;
	EngineTime								m_debugCreationTime;
	EngineTime								m_debugSignalTime;
};
