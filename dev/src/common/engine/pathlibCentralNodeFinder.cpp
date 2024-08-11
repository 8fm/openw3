/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibCentralNodeFinder.h"

#include "pathlibAreaDescription.h"
#include "pathlibHLGraph.h"
#include "pathlibHLNode.h"
#include "pathlibHLSubGraph.h"
#include "pathlibNavgraph.h"
#include "pathlibSearchEngine.h"
#include "pathlibWalkableSpotQueryRequest.h"
#include "pathlibWorld.h"

#include "pathlibNavgraphHelper.inl"

namespace PathLib
{

///////////////////////////////////////////////////////////////////////////////
// CCentralNodeFinder::RegionAcceptor
///////////////////////////////////////////////////////////////////////////////
CCentralNodeFinder::RegionAcceptor CCentralNodeFinder::AnyRegion( Bool includeInvalid, Bool bailOutOnSuccess )
{
	RegionAcceptor r;
	r.m_type = includeInvalid ? RegionAcceptor::ACCEPT_ANY : RegionAcceptor::ACCEPT_ANY_VALID;
	r.m_areaRegion = INVALID_AREA_REGION;
	r.m_bailOutOnSuccess = bailOutOnSuccess;
	return r;
}
CCentralNodeFinder::RegionAcceptor CCentralNodeFinder::SpecyficRegion( AreaRegionId regionId )
{
	RegionAcceptor r;
	r.m_type = RegionAcceptor::ACCEPT_SPECYFIC;
	r.m_areaRegion = regionId;
	r.m_bailOutOnSuccess = true;
	return r;
}
CCentralNodeFinder::RegionAcceptor CCentralNodeFinder::ReacheableRegion( AreaRegionId regionId, Bool bailOutOnSuccess, NodeFlags pathfindingForbiddenFlags ) const
{
	RegionAcceptor r;
	
	CHLNode* hlNode = m_navgraph.GetHLGraph().FindHLNode( regionId );
	if ( !hlNode )
	{
		// mark as empty iterator
		r.m_type = RegionAcceptor::ACCEPT_INVALID;
		return r;
	}

	CBrowseData browse;
	browse.m_start = hlNode;
	browse.m_flagsForbidden = pathfindingForbiddenFlags;

	r.m_type = RegionAcceptor::ACCEPT_REACHABLE;
	r.m_areaRegion = 0xbaad;							// not meant to be used
	r.m_bailOutOnSuccess = bailOutOnSuccess;
	r.m_marker = m_navgraph.GetArea()->GetPathLib().GetSearchEngine().SpreadMarking( browse );

	return r;
}

CCentralNodeFinder::RegionAcceptor CCentralNodeFinder::ReacheableRegion( CoherentRegion regionId, Bool bailOutOnSuccess, NodeFlags pathfindingForbiddenFlags ) const
{
	RegionAcceptor r;

	CPathLibWorld& pathlib = m_navgraph.GetArea()->GetPathLib();

	CHLNode* hlNode = HLGraph::FindNode( pathlib, m_navgraph.GetCategory(), regionId );
	if ( !hlNode )
	{
		// mark as empty iterator
		r.m_type = RegionAcceptor::ACCEPT_INVALID;
		return r;
	}

	CBrowseData browse;
	browse.m_start = hlNode;
	browse.m_flagsForbidden = pathfindingForbiddenFlags;

	r.m_type = RegionAcceptor::ACCEPT_REACHABLE;
	r.m_areaRegion = 0xbaad;							// not meant to be used
	r.m_bailOutOnSuccess = bailOutOnSuccess;
	r.m_marker = pathlib.GetSearchEngine().SpreadMarking( browse );

	return r;
}

CCentralNodeFinder::RegionAcceptor CCentralNodeFinder::SpecyficAt( const Vector3& pos ) const
{
	Float maxDist = m_navgraph.GetMaxNodesDistance();
	Float personalSpace = m_navgraph.GetPersonalSpace();
	CNavNode* navNode = FindClosestNodeWithLinetest( pos, maxDist, personalSpace, AnyRegion( false, true ) );
	if ( !navNode )
	{
		RegionAcceptor r;
		r.m_type = RegionAcceptor::ACCEPT_INVALID;
		return r;
	}
	return SpecyficRegion( navNode->GetAreaRegionId() );
}
CCentralNodeFinder::RegionAcceptor CCentralNodeFinder::ReachableFrom( const Vector3& pos, Float personalSpace, Bool bailOutOnSuccess, Uint32 collisionFlags, NodeFlags pathfindingForbiddenFlags ) const
{
	Float maxDist = m_navgraph.GetMaxNodesDistance();
	CNavNode* navNode = FindClosestNodeWithLinetest( pos, maxDist, personalSpace, AnyRegion( false, true ), collisionFlags, pathfindingForbiddenFlags );
	if ( !navNode )
	{
		RegionAcceptor r;
		r.m_type = RegionAcceptor::ACCEPT_INVALID;
		return r;
	}
	return ReacheableRegion( navNode->GetAreaRegionId(), bailOutOnSuccess, pathfindingForbiddenFlags );
}

CCentralNodeFinder::RegionAcceptor CCentralNodeFinder::ReachableFrom( CPathLibWorld& pathlib, const Vector3& pos, Float personalSpace, Bool bailOutOnSuccess, Uint32 collisionFlags, NodeFlags pathfindingForbiddenFlags )
{
	CAreaDescription* area = pathlib.GetAreaAtPosition( pos );
	if ( area )
	{
		Uint32 category = pathlib.GetGlobalSettings().ComputePSCategory( personalSpace );
		CNavGraph* navgraph = area->GetNavigationGraph( category );
		if ( navgraph )
		{
			return navgraph->GetNodeFinder().ReachableFrom( pos, personalSpace, bailOutOnSuccess, collisionFlags, pathfindingForbiddenFlags );
		}
	}
	RegionAcceptor r;
	r.m_type = RegionAcceptor::ACCEPT_INVALID;
	return r;
}

///////////////////////////////////////////////////////////////////////////////
// CCentralNodeFinder::RegionIterator
///////////////////////////////////////////////////////////////////////////////
//struct RegionIterator : public Red::System::NonCopyable
//{
//protected:
//	CNodeFinder*		m_nodeFinder;
//	Uint32				m_currentRegion;
//	Uint32				m_regionsCount;
//	Uint16				m_marker;
//	void				(*m_next)( RegionIterator& it );
//
//public:
CCentralNodeFinder::RegionIterator::RegionIterator( const CCentralNodeFinder& centralNF, const RegionAcceptor& regionAcceptor )
{
	struct Local
	{
		static void AcceeptAny( RegionIterator& it )
		{
			// iterate over all nodes
			it.m_nodeFinder =
				(it.m_index) < it.m_indexCount
				? &it.m_topGraph->GetHLNode( it.m_index++ )->GetNodeFinder()
				: nullptr;
		}
		static void AcceeptSpecyfic( RegionIterator& it )
		{
			// after processing given node - turn off
			it.m_nodeFinder = nullptr;
		}
		static void AcceeptReachable( RegionIterator& it )
		{
			// iterate over all nodes
			while( it.m_index < it.m_indexCount )
			{
				CHLNode* hlNode = it.m_topGraph->GetHLNode( it.m_index++ );
				// check were they visited (check marking)
				if ( it.m_marker.CheckMarking( *hlNode ) )
				{
					// accept visited ( reachable ) node
					it.m_nodeFinder = &hlNode->GetNodeFinder();
					return;
				}
				
			}
			// run out of nodes
			it.m_nodeFinder = nullptr;
		}
	};
	CNavGraph& navgraph = centralNF.GetNavGraph();
	CHLSubGraph& hlGraph = navgraph.GetHLGraph();
	
	switch( regionAcceptor.m_type )
	{
	case RegionAcceptor::ACCEPT_ANY:
		{
			m_next = Local::AcceeptAny;
			m_topGraph = &hlGraph;
			m_index = 0;
			m_indexCount = hlGraph.GetHLNodesCount();
			m_nodeFinder = &centralNF.m_detachedNodeMap;
		}
		break;
	case RegionAcceptor::ACCEPT_ANY_VALID:
		{
			m_next = Local::AcceeptAny;
			m_topGraph = &hlGraph;
			m_index = 1;
			m_indexCount = hlGraph.GetHLNodesCount();
			m_nodeFinder =
				m_indexCount > 0
				? &hlGraph.GetHLNode( 0 )->GetNodeFinder()
				: nullptr;
		}
		
		break;
	case RegionAcceptor::ACCEPT_SPECYFIC:
		{
			CHLNode* hlNode = hlGraph.FindHLNode( regionAcceptor.m_areaRegion );
			if ( !hlNode )
			{
				// mark as empty iterator
				m_nodeFinder = nullptr;
				return;
			}
			m_nodeFinder = &hlNode->GetNodeFinder();
			m_next = Local::AcceeptSpecyfic;
		}
		
		break;
	case RegionAcceptor::ACCEPT_REACHABLE:
		{
			m_next = Local::AcceeptReachable;
			m_topGraph = &hlGraph;
			m_index = 0;
			m_indexCount = hlGraph.GetHLNodesCount();
			m_marker = regionAcceptor.m_marker;
			ASSERT( m_marker.Debug_IsMarkerValid( navgraph.GetArea()->GetPathLib() ) );
			Local::AcceeptReachable( *this );
		}
		break;
	case RegionAcceptor::ACCEPT_INVALID:
		{
			m_nodeFinder = nullptr;
		}
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////
// CCentralNodeFinder
///////////////////////////////////////////////////////////////////////////////
CCentralNodeFinder::CCentralNodeFinder( CNavGraph& navgraph )
	: m_navgraph( navgraph )
	, m_detachedNodeMap( navgraph, INVALID_AREA_REGION )
	, m_isInitialized( false )
{

}
CCentralNodeFinder::~CCentralNodeFinder()
{

}
///////////////////////////////////////////////////////////////////////////////
// Life-cycle
void CCentralNodeFinder::Compute() 
{
	m_isInitialized = true;

	CHLSubGraph& hlGraph = m_navgraph.GetHLGraph();
	auto& nodesArray = hlGraph.GetHLNodesArray();

	// functors
	auto funBBox =
		[ &hlGraph, this ] ( CNavNode& node )
	{
		AreaRegionId areaRegionId = node.GetAreaRegionId();
		CHLNode* hlNode =
			areaRegionId != INVALID_AREA_REGION
			? hlGraph.FindHLNode( areaRegionId )
			: nullptr;
		if ( hlNode )
		{
			hlNode->CentralInitializationBBoxUpdate( node );
		}
		else
		{
			this->m_detachedNodeMap.CentralInitializationBBoxUpdate( node );
		}
	};
	auto funNotice =
		[ &hlGraph, this ] ( CNavNode& node )
	{
		AreaRegionId areaRegionId = node.GetAreaRegionId();
		CHLNode* hlNode =
			areaRegionId != INVALID_AREA_REGION
			? hlGraph.FindHLNode( areaRegionId )
			: nullptr;
		if ( hlNode )
		{
			hlNode->CentralInitializationCollectNode( node );
		}
		else
		{
			this->m_detachedNodeMap.CentralInitializationCollectNode( node );
		}
	};



	// Centralized initialization process
	Bool isDirty = false;

	for ( auto it = nodesArray.Begin(), end = nodesArray.End(); it != end; ++it )
	{
		if ( it->GetNodeFinder().IsValid() )
		{
			continue;
		}
		it->PreCenteralInitialization( m_navgraph );
		isDirty = true;
	}

	if ( !m_detachedNodeMap.IsValid() )
	{
		m_detachedNodeMap.PreCenteralInitialization( m_navgraph, INVALID_AREA_REGION );
		isDirty = true;
	}
	
	if ( !isDirty )
	{
		return;
	}

	NavgraphHelper::ForAllNodes( m_navgraph, funBBox, true );

	for ( auto it = nodesArray.Begin(), end = nodesArray.End(); it != end; ++it )
	{
		it->CentralInitilzationComputeCelMap();
	}
	m_detachedNodeMap.CentralInitilzationComputeCelMap();

	NavgraphHelper::ForAllNodes( m_navgraph, funNotice, true );

	for ( auto it = nodesArray.Begin(), end = nodesArray.End(); it != end; ++it )
	{
		it->PostCentralInitialization();
	}
	m_detachedNodeMap.PostCentralInitialization();
}
void CCentralNodeFinder::Clear()
{
	if ( m_isInitialized )
	{
		const CHLSubGraph& hlGraph = m_navgraph.GetHLGraph();
		const auto& nodesArray = hlGraph.GetHLNodesArray();
		for ( auto it = nodesArray.Begin(), end = nodesArray.End(); it != end; ++it )
		{
			it->GetNodeFinder().Clear();
		}
		m_detachedNodeMap.Clear();
		m_isInitialized = false;
	}
	
}
Bool CCentralNodeFinder::IsInitialized() const
{
	return m_isInitialized;
}

void CCentralNodeFinder::Invalidate()
{
	if ( m_isInitialized )
	{
		const CHLSubGraph& hlGraph = m_navgraph.GetHLGraph();
		const auto& nodesArray = hlGraph.GetHLNodesArray();
		for ( auto it = nodesArray.Begin(), end = nodesArray.End(); it != end; ++it )
		{
			it->GetNodeFinder().Invalidate();
		}
		m_detachedNodeMap.Invalidate();

		m_isInitialized = false;
	}
	
}
void CCentralNodeFinder::CompactData() const
{
	const CHLSubGraph& hlGraph = m_navgraph.GetHLGraph();
	const auto& nodesArray = hlGraph.GetHLNodesArray();
	for ( auto it = nodesArray.Begin(), end = nodesArray.End(); it != end; ++it )
	{
		it->GetNodeFinder().CompactData();
	}
	m_detachedNodeMap.CompactData();
}

void CCentralNodeFinder::AddDynamicElement( CNavNode* pNode ) const
{
	AreaRegionId regionId = pNode->GetAreaRegionId();
	if ( regionId == INVALID_AREA_REGION )
	{
		m_detachedNodeMap.AddDynamicElement( pNode );
		return;
	}

	CHLSubGraph& hlGraph = m_navgraph.GetHLGraph();
	CHLNode* hlNode = hlGraph.FindHLNode( regionId );
	if ( !hlNode )
	{
		m_detachedNodeMap.AddDynamicElement( pNode );
		return;
	}
	hlNode->GetNodeFinder().AddDynamicElement( pNode );
}
void CCentralNodeFinder::RemoveDynamicElement( CNavNode* pNode ) const
{
	AreaRegionId regionId = pNode->GetAreaRegionId();
	if ( regionId == INVALID_AREA_REGION )
	{
		m_detachedNodeMap.RemoveDynamicElement( pNode );
		return;
	}

	CHLSubGraph& hlGraph = m_navgraph.GetHLGraph();
	CHLNode* hlNode = hlGraph.FindHLNode( regionId );
	if ( !hlNode )
	{
		m_detachedNodeMap.RemoveDynamicElement( pNode );
		return;
	}
	hlNode->GetNodeFinder().RemoveDynamicElement( pNode );
	pNode->ClearRegionId();
}


///////////////////////////////////////////////////////////////////////////////
// Queries
template < class Functor >
RED_INLINE CNavNode* CCentralNodeFinder::TQuery( Functor& fun, const RegionAcceptor& region ) const
{
	RegionIterator it( *this, region );

	CNavNode* bestNode = nullptr;

	while( it )
	{
		if ( CNavNode* navNode = fun( *it ) )
		{
			bestNode = navNode;
			if ( region.m_bailOutOnSuccess )
			{
				break;
			}
		}

		++it;
	}

	return bestNode;
}
template < class Functor >
RED_INLINE Bool CCentralNodeFinder::TQueryN( Functor& fun, const RegionAcceptor& region ) const
{
	RegionIterator it( *this, region );

	Bool ret = false;

	while( it )
	{
		Bool b = fun( *it );
		if ( b )
		{
			ret = true;
			if ( region.m_bailOutOnSuccess )
			{
				break;
			}
		}

		++it;
	}

	return ret;
}


CNavNode* CCentralNodeFinder::FindClosestNode( const Vector3& pos, Float maxDist, const RegionAcceptor& region, NodeFlags forbiddenNodeFlags ) const
{
	auto fun =
		[ pos, &maxDist, forbiddenNodeFlags ] ( const CNodeFinder& nf ) -> CNavNode*
	{
		return nf.FindClosestNode( pos, maxDist, forbiddenNodeFlags );
	};

	return TQuery( fun, region );
}
CNavNode* CCentralNodeFinder::FindClosestNodeWithLinetest( const Vector3& pos, Float maxDist, Float lineWidth, const RegionAcceptor& region, Uint32 lineTestFlags, NodeFlags forbiddenNodeFlags ) const
{
	auto fun =
		[ pos, &maxDist, lineWidth, lineTestFlags, forbiddenNodeFlags ] ( const CNodeFinder& nf ) -> CNavNode*
	{
		return nf.FindClosestNodeWithLinetest( pos, maxDist, lineWidth, lineTestFlags, forbiddenNodeFlags );
	};

	return TQuery( fun, region );
}
CNavNode* CCentralNodeFinder::FindClosestNodeWithLinetestAndTolerance( const Vector3& pos, Float& toleranceRadius, Float lineWidth, Vector3& outAccessiblePosition, const RegionAcceptor& region, Uint32 lineTestFlags, NodeFlags forbiddenNodeFlags, Bool allowVerticalDiversity ) const
{
	auto fun =
		[ pos, &toleranceRadius, lineWidth, &outAccessiblePosition, lineTestFlags, forbiddenNodeFlags, allowVerticalDiversity ] ( const CNodeFinder& nf ) -> CNavNode*
	{
		return nf.FindClosestNodeWithLinetestAndTolerance( pos, toleranceRadius, lineWidth, outAccessiblePosition, lineTestFlags, forbiddenNodeFlags, allowVerticalDiversity );
	};

	return TQuery( fun, region );
}

Bool CCentralNodeFinder::FindNClosestNodes( const Vector3& pos, Float maxDist, Uint32 n, OutputVector& output, const RegionAcceptor& region, NodeFlags forbiddenNodeFlags ) const
{
	auto fun =
		[ pos, &maxDist, n, &output, forbiddenNodeFlags ] ( const CNodeFinder& nf ) -> Bool
	{
		return nf.FindNClosestNodes( pos, maxDist, n, output, forbiddenNodeFlags );
	};
	
	return TQueryN( fun, region );
}
Bool CCentralNodeFinder::FindNClosestNodesWithLinetest( const Vector3& pos, Float maxDist, Float lineWidth, Uint32 n, OutputVector& output, const RegionAcceptor& region, Uint32 lineTestFlags, NodeFlags forbiddenNodeFlags ) const
{
	auto fun =
		[ pos, &maxDist, lineWidth, n, &output, lineTestFlags, forbiddenNodeFlags ] ( const CNodeFinder& nf ) -> Bool
	{
		return nf.FindNClosestNodesWithLinetest( pos, maxDist, lineWidth, n, output, lineTestFlags, forbiddenNodeFlags );
	};

	return TQueryN( fun, region );
}

CNavNode* CCentralNodeFinder::FindClosestNode( CWalkableSpotQueryRequest& request, const RegionAcceptor& region ) const
{
	auto fun =
		[ &request ] ( const CNodeFinder& nf ) -> CNavNode*
	{
		return nf.FindClosestNode( request );
	};

	CNavNode* navNode = TQuery( fun, region );

	if ( !navNode && !request.IsQuerySuccess() && !request.IsComputingClosestPositionWithClearLinetest() )
	{
		auto funForce =
			[ &request ] ( const CNodeFinder& nf ) -> CNavNode*
		{
			return nf.FindClosestNode( request, true );
		};

		navNode = TQuery( funForce, region );
	}

	return navNode;
}

//
//CNavNode* CCentralNodeFinder::FindClosestNode( const Vector3& pos, Float maxDist, Acceptor& acceptor, const RegionAcceptor& region, NodeFlags forbiddenNodeFlags ) const
//{
//	auto fun =
//		[ pos, &maxDist, &acceptor, forbiddenNodeFlags ] ( const CNodeFinder& nf ) -> CNavNode*
//	{
//		return nf.FindClosestNode( pos, maxDist, acceptor, forbiddenNodeFlags );
//	};
//
//	return TQuery( fun, region );
//}
//CNavNode* CCentralNodeFinder::FindClosestNode( const Vector3& pos, const Box& bbox, Float maxDist, Acceptor& acceptor, const RegionAcceptor& region, NodeFlags forbiddenNodeFlags ) const
//{
//	auto fun =
//		[ pos, bbox, &maxDist, &acceptor, forbiddenNodeFlags ] ( const CNodeFinder& nf ) -> CNavNode*
//	{
//		return nf.FindClosestNode( pos, bbox, maxDist, acceptor, forbiddenNodeFlags );
//	};
//
//	return TQuery( fun, region );
//}
//Bool CCentralNodeFinder::FindNClosestNodes( const Vector3& pos, Float maxDist, Uint32 n, OutputVector& output, Acceptor& acceptor, const RegionAcceptor& region, NodeFlags forbiddenNodeFlags ) const
//{
//	auto fun =
//		[ pos, &maxDist, n, &output, &acceptor, forbiddenNodeFlags ] ( const CNodeFinder& nf ) -> Bool
//	{
//		return nf.FindNClosestNodes( pos, maxDist, n, output, acceptor, forbiddenNodeFlags );
//	};
//
//	return TQueryN( fun, region );
//}

void CCentralNodeFinder::IterateNodes( const Box& boundings, Handler& handler, const RegionAcceptor& region, NodeFlags forbiddenNodeFlags ) const
{
	RegionIterator it( *this, region );

	while( it )
	{
		it->IterateNodes( boundings, handler, forbiddenNodeFlags );
		++it;
	}
}


};			// namespace PathLib

