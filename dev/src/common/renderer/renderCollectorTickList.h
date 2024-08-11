/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
* Kamil Nowakowski
*/
#pragma once
#include "renderProxy.h"

class CRenderCollectorTickList
{
	// MUST BE POWER OF TWO
	static const Int32 ASYNC_COLLECTED_TICK_SIZE = 2048;

	IRenderProxyBase*						m_collectedTickAsyncArray[ASYNC_COLLECTED_TICK_SIZE];	//!< Collected tickable proxies that were collected on another thread

	Red::Threads::CAtomic< Uint32 >			m_collectedTickAsyncArrayCounter;	//!< Thread safe counter for the list
	Uint32									m_currentCollectionCounter;			//!< Thread safe counter for the list

	Bool									m_collectionEnabled;

public:

	CRenderCollectorTickList();

	~CRenderCollectorTickList();

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	RED_INLINE Bool IsCollectionEnabled() const { return m_collectionEnabled; } 

	RED_INLINE void SetCollectionEnabled( Bool collectionEnabled ) { m_collectionEnabled = collectionEnabled; } 

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	
	//! Pushes new proxy to the tick list
	void PushProxy( IRenderProxyBase* proxy );

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	//! Go through collected tick list, call CollectedTick() on each proxy. The collected tick list will be empty after this call.
	void TickCollectedProxies( CRenderSceneEx* scene );

};