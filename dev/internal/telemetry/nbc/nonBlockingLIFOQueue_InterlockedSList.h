#pragma once

#include <windows.h>

#include "typeDefs.h"

//! This class implements lock-less LIFO Queue
class CNonBlockingLIFOQueue
{

public:
	CNonBlockingLIFOQueue();
	~CNonBlockingLIFOQueue();

	//! Enqueue object. Thread-safe
	void Equeue( void* value );
	//! Dequeue object. Thread safe
	void* Dequeue();
	

private:
	SLIST_HEADER	m_Head;

public:

#ifdef PROFILE_NON_BLOCKING_QUEUE
	// At this point of library compilation we cannot use RED engine's atomics
	volatile __declspec( align( 4 ) ) long m_objectCounter;
#endif

};