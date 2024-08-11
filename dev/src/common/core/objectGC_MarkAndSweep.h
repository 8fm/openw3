/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "objectGC.h"
#include "objectReachability.h"

/// Old school garbage collector
class CObjectsGC_MarkAndSweep : public IObjectGCStategy
{
public:
	CObjectsGC_MarkAndSweep();
	~CObjectsGC_MarkAndSweep();

	// IObjectGCStategy implementation
	virtual void CollectGarbage( const Bool reportLeaks ) override;

private:
	CObjectReachability		m_reachability;			// Reachability analyzer
	CFastObjectList			m_unreachables;			// Unreachable objects
};
