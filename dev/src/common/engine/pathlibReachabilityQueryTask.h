/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/refCountPointer.h"

#include "pathlibSearchData.h"
#include "pathlibTaskManager.h"


namespace PathLib
{


// Task meant for external
class IReachabilityTaskBase : public CTaskManager::CAsyncTask
{
	typedef CTaskManager::CAsyncTask Super;
protected:
	struct ComputationContext
	{
		AreaId 					m_areaId;
		CNavNode*				m_startingNode;
		CNavGraph*				m_startingNavgraph;

		CNavNode*				m_outputStartingNode;
		CNavNode*				m_outputDestinationNode;
		Vector3					m_outputDestinationPosition;
		Bool					m_outputIsPrecise;
		Bool					m_bailOutOnSuccess;
		Bool					m_isDestinationTested;
	};
	Bool						QueryPathEndsPrecise( ComputationContext& context, const Vector3& destination );
	Bool						QueryPathEndsWithTolerance( ComputationContext& context, const Vector3& destination );

	IReachabilityDataBase::Ptr	m_searchData;

#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
	CSearchData::EPathfindFailureReason		m_failureReason;

	void						SetFailureReason( CSearchData::EPathfindFailureReason reason )			{ m_failureReason = reason; }
#else
	void						SetFailureReason( CSearchData::EPathfindFailureReason reason )			{}
#endif

	EPathfindResult				QueryPathEnds( const Vector3& destination, CNavNode*& outStartingNode, CNavNode*& outDestinationNode, Vector3& outDestination, Bool& useTolerance, Bool getAnyOutput = false );

public:
	IReachabilityTaskBase( CTaskManager& taskManager, IReachabilityDataBase* searchData, EPriority priority );
};


class CReachabilityTask : public IReachabilityTaskBase
{
	typedef IReachabilityTaskBase Super;
protected:
	virtual void				Process() override;
public:
	typedef TRefCountPointer< CReachabilityTask > Ptr;

	CReachabilityTask( CTaskManager& taskManager, CReachabilityData* searchData )
		: Super( taskManager, searchData, EPriority::ReachabilityQuery )															{}
	// external, simplified interface for spawnin & running this stuff
	static Ptr					Request( CPathLibWorld& pathlib, CReachabilityData* searchData );

	virtual void				DescribeTask( String& outName ) const override;
};

class CMultiReachabilityTask : public IReachabilityTaskBase
{
	typedef IReachabilityTaskBase Super;
protected:
	static const Float REACHABILITY_TEST_DISTANCE_LIMIT_MAGIC_MULTIPLIER;

	Bool						QueryReachability();
	Bool						QueryReachabilitySimplified();

	virtual void				Process() override;

public:
	typedef TRefCountPointer< CMultiReachabilityTask > Ptr;

	CMultiReachabilityTask( CTaskManager& taskManager, CMultiReachabilityData* searchData )
		: Super( taskManager, searchData, EPriority::ReachabilityQuery )															{}
	// external, simplified interface for spawnin & running this stuff
	static Ptr					Request( CPathLibWorld& pathlib, CMultiReachabilityData* searchData );

	virtual void				DescribeTask( String& outName ) const override;
};

};

