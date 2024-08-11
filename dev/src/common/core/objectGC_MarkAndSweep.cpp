/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "objectGC_MarkAndSweep.h"
#include "objectMap.h"
#include "objectRootSet.h"
#include "objectReachability.h"

CObjectsGC_MarkAndSweep::CObjectsGC_MarkAndSweep()
{
}

CObjectsGC_MarkAndSweep::~CObjectsGC_MarkAndSweep()
{
}

void CObjectsGC_MarkAndSweep::CollectGarbage(const Bool /*reportLeaks*/)
{
	CTimeCounter totalTime;

	// GC stages:
	//  1. prepare (reset unreachables table, take initial objects form root set)
	//  2. mark reachable object (recursive)
	//  3. discard unreachable object (objects that were not visited)
	// GC assumptions:
	//  - objects are not being created on some other thread (no async (de)serialization)
	//  - there are no GC blocking jobs running
	//  - job manager will not issue new jobs

	// prepare the unreachable list - zero it then set 1 for each VALID object
	const Uint32 maxObjectIndex = GObjectsMap->GetMaxObjectIndex(); // may be not 100% thread safe
	const Uint32 initialLiveObjects = GObjectsMap->GetNumLiveObjects();
	m_unreachables.ResetToZeros( maxObjectIndex );

	// mark all valid objects as initially unreachable
	GObjectsMap->VisitAllObjectsNoFilter( [&](const CObject*, const Uint32 index)
	{
		m_unreachables.Set(index);
	} );

	// create the initial seed list of object by copying the root set
	m_reachability.InitializeFromRootSet( maxObjectIndex );

	// propagate reachability flag, clearing the unreachable flags on the objects that were visited
	const Bool useMultiThreadedImplementation = false; // TODO
	m_reachability.CollectUnreachables( &m_unreachables, useMultiThreadedImplementation );

	// discard unreachable objects
	{
		CObjectsMap::ObjectDiscarder objectDiscarded( GObjectsMap );
		m_unreachables.VisitSet( maxObjectIndex, objectDiscarded );
	}

	// Stats
	const Uint32 finalLiveObjects = GObjectsMap->GetNumLiveObjects();
	RED_UNUSED( finalLiveObjects );
	RED_UNUSED( initialLiveObjects );
	LOG_CORE( TXT("GC %d->%d (%1.2fms)"), 
		initialLiveObjects, finalLiveObjects,
		totalTime.GetTimePeriodMS() );

}
