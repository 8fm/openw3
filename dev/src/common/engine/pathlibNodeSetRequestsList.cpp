/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibNodeSetRequestsList.h"

#include "pathlibAreaDescription.h"
#include "pathlibNavgraph.h"
#include "pathlibNodeSet.h"
#include "pathlibObstaclesMap.h"
#include "pathlibWorld.h"

namespace PathLib
{


////////////////////////////////////////////////////////////////////////////
// CNodeSetProcessingContext
////////////////////////////////////////////////////////////////////////////
void CNodeSetProcessingContext::BeginRequestCollection( CPathLibWorld& pathlib, CNavGraph* navgraph )
{
	m_marking = CSearchNode::Marking( pathlib );
}
void CNodeSetProcessingContext::EndRequestCollection( CPathLibWorld& pathlib, CNavGraph* navgraph )
{
	struct Order
	{
		Bool operator()( CNavNode* n0, CNavNode* n1 ) const
		{
			return n0->GetAreaRegionId() == n1->GetAreaRegionId()
				? n0 < n1 :
			n0->GetAreaRegionId() < n1->GetAreaRegionId();
		}
	};

	if ( m_nodesToUpdateConsistency.Empty() )
	{
		return;
	}

	ASSERT( m_marking.Debug_IsMarkerValid( pathlib ) );
	// sort consistency update input, so we can process it region after region
	::Sort( m_nodesToUpdateConsistency.Begin(), m_nodesToUpdateConsistency.End(), Order() );

	// remove duplicates
	CNavNode::Id lastId = CNavNode::Id::INVALID;
	for ( Int32 i = m_nodesToUpdateConsistency.Size()-1; i >= 0; --i )
	{
		CNavNode* node = m_nodesToUpdateConsistency[ i ];
		if ( node->GetFullId() == lastId )
		{
			m_nodesToUpdateConsistency.RemoveAt( i );
		}
		else
		{
			lastId = node->GetFullId();
		}
	}

	CRegionUpdateData updateRequest( m_nodesToUpdateConsistency, *navgraph );

	// update consistency
	PATHLIB_ASSERT( navgraph->GetHLGraph().Debug_HLCheckAllLinksTwoSided() );
	pathlib.GetSearchEngine().RegionsUpdate( updateRequest );				// update node marking
	navgraph->GetHLGraph().CleanLegacyNodes();								// cleanup obsolate hl nodes
	navgraph->GetNodeFinder().Compute();									// update node finders
	navgraph->GetHLGraph().UpdateLinkage();									// update hl nodes connections
	navgraph->MarkVersionDirty();											// force update debug visuals

	m_nodesToUpdateConsistency.ClearFast();
}


////////////////////////////////////////////////////////////////////////////
// CComponentRuntimeProcessingContext
////////////////////////////////////////////////////////////////////////////
CComponentRuntimeProcessingContext::CComponentRuntimeProcessingContext( CObstaclesMapper* mapper )
	: m_mapper( mapper )
#ifndef RED_FINAL_BUILD
	, m_isCollecting( false )
#endif
{
}
CComponentRuntimeProcessingContext::~CComponentRuntimeProcessingContext()
{

}

void CComponentRuntimeProcessingContext::BeginRequestsCollection( CPathLibWorld& pathlib )
{
	PATHLIB_ASSERT( m_requests.Empty(), TXT("Some process injected node set requests when request collection wasn't running!") );
#ifndef RED_FINAL_BUILD
	m_isCollecting = true;
#endif
}

void CComponentRuntimeProcessingContext::AddRequest( AreaId areaId, Uint16 category, Uint32 id, ERequestType requestType )
{
#ifndef RED_FINAL_BUILD
	ASSERT( m_isCollecting );
#endif
	
	AreaProcessingRequests& areaRequests = m_requestsCollection.GetRef( areaId );
	
	ProcessingRequest r;
	r.m_nodeSetId = id;
	r.m_requestType = requestType;
	r.m_next = areaRequests.m_firstRequestIndex[ category ];
	areaRequests.m_firstRequestIndex[ category ] = m_requestsBuffer.Size();

	m_requestsBuffer.PushBack( r );
}

void CComponentRuntimeProcessingContext::AddNodeSetAttachRequest( AreaId areaId, Uint16 category, Uint32 nodeSetId )
{
	AddRequest( areaId, category, nodeSetId, REQUEST_NODESET_ATTACH );
}

void CComponentRuntimeProcessingContext::AddNodeSetDetachRequest( AreaId areaId, Uint16 category, Uint32 nodeSetId )
{
	AddRequest( areaId, category, nodeSetId, REQUEST_NODESET_DETACH );
}

void CComponentRuntimeProcessingContext::AddRecomputationRequest( AreaId areaId, const Box& bbox )
{
	AreaProcessingRequests& areaRequests = m_requestsCollection.GetRef( areaId );
	RecomputationBox r;
	r.m_box = bbox;
	Int32 reuseBBoxIndex = -1;
	// try to merge new box with existing ones
	{
		Bool performedMerge;
		do 
		{
			performedMerge = false;
			for( Int32* boxIndex = &areaRequests.m_firstRecomputationBoxIndex; (*boxIndex) >= 0; boxIndex = &m_boxesBuffer[ *boxIndex ].m_next )
			{
				if ( r.m_box.Touches( m_boxesBuffer[ *boxIndex ].m_box ) )
				{
					// combine boxes
					r.m_box.AddBox( m_boxesBuffer[ *boxIndex ].m_box );
					// tag performMerge flag to reprocess boxes
					performedMerge = true;
					// mark that bbox request for re-use
					reuseBBoxIndex = *boxIndex;
					// remove bbox from queue
					(*boxIndex) = m_boxesBuffer[ *boxIndex ].m_next;
					break;
				}
			}
		}
		while ( performedMerge );
	}

	r.m_next = areaRequests.m_firstRecomputationBoxIndex;

	if ( reuseBBoxIndex < 0 )
	{
		reuseBBoxIndex = m_boxesBuffer.Size();
		m_boxesBuffer.PushBack( r );
	}
	else
	{
		m_boxesBuffer[ reuseBBoxIndex ] = r;
	}

	areaRequests.m_firstRecomputationBoxIndex = reuseBBoxIndex;
}

void CComponentRuntimeProcessingContext::EndRequestsCollection( CPathLibWorld& pathlib )
{
#ifndef RED_FINAL_BUILD
	m_isCollecting = false;
#endif

	for ( auto it = m_requestsCollection.Begin(), end = m_requestsCollection.End(); it != end; ++it )
	{
		AreaId areaId = it->m_first;
		const AreaProcessingRequests& reqList = it->m_second;

		CAreaDescription* area = pathlib.GetAreaDescription( areaId );
		CAreaNavgraphs* navList = area->GetNavgraphs();
		if ( !navList )
		{
			continue;
		}

		for ( Uint32 category = 0; category < MAX_ACTOR_CATEGORIES; ++category )
		{
			CNavGraph* navgraph = navList->GetGraph( category );
			if ( navgraph )
			{
				m_nodeSetProcessingContext.BeginRequestCollection( pathlib, navgraph );

				// remove requests from collision processing
				{
					Int32 reqIndex = reqList.m_firstRequestIndex[ category ];
					while( reqIndex >= 0 )
					{
						const ProcessingRequest& req = m_requestsBuffer[ reqIndex ];
						CNavgraphNodeSet* nodeSet = navgraph->GetNodeSet( req.m_nodeSetId );

						if ( req.m_requestType == REQUEST_NODESET_ATTACH )
						{
							nodeSet->PreAttach( navgraph, m_nodeSetProcessingContext );
						}
						else
						{
							nodeSet->PreDetach( navgraph, m_nodeSetProcessingContext );
						}

						reqIndex = req.m_next;
					}
				}
				

				// recompute all existing nodes
				{
					Int32 boxIndex = reqList.m_firstRecomputationBoxIndex;
					while ( boxIndex >= 0 )
					{
						const RecomputationBox& req = m_boxesBuffer[ boxIndex ];
						navgraph->RuntimeUpdateCollisionFlags( req.m_box, m_nodeSetProcessingContext );
						boxIndex = req.m_next;
					}
				}
			

				// process requests
				{
					Int32 reqIndex = reqList.m_firstRequestIndex[ category ];
					while( reqIndex >= 0 )
					{
						const ProcessingRequest& req = m_requestsBuffer[ reqIndex ];
						CNavgraphNodeSet* nodeSet = navgraph->GetNodeSet( req.m_nodeSetId );

						if ( req.m_requestType == REQUEST_NODESET_ATTACH )
						{
							nodeSet->Attach( navgraph, m_nodeSetProcessingContext );
						}
						else
						{
							nodeSet->Detach( navgraph, m_nodeSetProcessingContext );
						}

						reqIndex = req.m_next;
					}
				}


				// recompute all existing nodes
				Int32 boxIndex = reqList.m_firstRecomputationBoxIndex;
				while ( boxIndex >= 0 )
				{
					const RecomputationBox& req = m_boxesBuffer[ boxIndex ];
					Box bbox = req.m_box;
					// box in RecomputationBox is in area local coordinates
					area->VLocalToWorld( bbox );
					navgraph->RuntimeUpdateCollisionFlags( bbox, m_nodeSetProcessingContext );
					boxIndex = req.m_next;
				}

				// recompute navgraph consistency
				m_nodeSetProcessingContext.EndRequestCollection( pathlib, navgraph );
			}
		}
	}


	m_requestsCollection.ClearFast();
	m_requestsBuffer.ClearFast();
	m_boxesBuffer.ClearFast();

	
}

};			// namespace PathLib