/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/renderFence.h"

/// Fence
class CRenderFence : public IRenderFence
{
protected:
	Red::Threads::CSemaphore	m_semaphore;		//!< Handle to internal event
	Red::Threads::CMutex		m_internalMutex;	//!< Internal mutex
	volatile Bool				m_isSignaled;		//!< Fence was signaled

public:
	CRenderFence();
	~CRenderFence();

	//! Wait for fence to be flushed
	virtual void FlushFence();
	
	//! Commit the fence, will release all waiting threads
	virtual void SignalWaitingThreads();
};