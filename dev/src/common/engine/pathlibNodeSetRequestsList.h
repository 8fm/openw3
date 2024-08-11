/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlib.h"
#include "pathlibGraph.h"
#include "pathlibObstacle.h"

class CPathLibWorld;

namespace PathLib
{

class CObstaclesMapper;

////////////////////////////////////////////////////////////////////////////
// Context for runtime navgraph processing functions.
// As they touch and modify connection and node flags, post processing
// will have to update region consistency afterwards.
////////////////////////////////////////////////////////////////////////////
class CNodeSetProcessingContext
{
protected:
	CSearchNode::Marking					m_marking;
	TDynArray< CNavNode* >					m_nodesToUpdateConsistency;		// NOTICE: Holding nodes by pointer might turn out to be unsafe - need to be cautious here.

public:
	void				BeginRequestCollection( CPathLibWorld& pathlib, CNavGraph* navgraph );
	void				UpdateNodeConsistency( CNavNode* node )				{ m_nodesToUpdateConsistency.PushBack( node ); }
	void				EndRequestCollection( CPathLibWorld& pathlib, CNavGraph* navgraph );

	const CSearchNode::Marking&		GetMarking() const						{ return m_marking; }
};

////////////////////////////////////////////////////////////////////////////
// Context for runtime functions that process the pathlib components.
// Could be created dynamically per processing, bug to safe dynamic
// allocations we will keep this thing on CObstaclesMapper.
////////////////////////////////////////////////////////////////////////////
class CComponentRuntimeProcessingContext
{
protected:
	enum ERequestType
	{
		REQUEST_NODESET_ATTACH,
		REQUEST_NODESET_DETACH,
	};

	struct AreaProcessingRequests
	{
		AreaProcessingRequests()
			: m_firstRecomputationBoxIndex( -1 )														{ for ( Uint32 i = 0; i < MAX_ACTOR_CATEGORIES; ++i ) { m_firstRequestIndex[ i ] = -1; } }

		Int32									m_firstRecomputationBoxIndex;
		Int32									m_firstRequestIndex[ MAX_ACTOR_CATEGORIES ];
	};

	struct ProcessingNavgraph
	{
		AreaId									m_areaId;
		Uint16									m_category;

		Uint32									CalcHash() const										{ return Uint32( m_areaId ) | ( Uint32( m_category ) << 16 ); }
	};

	struct RecomputationBox
	{
		Box										m_box;
		Int32									m_next;													// index to next box in da list
	};

	struct ProcessingRequest
	{
		Uint32									m_nodeSetId;
		ERequestType							m_requestType;
		Int32									m_next;
	};

	typedef THashMap< AreaId, AreaProcessingRequests >		RequestsCollection;
	typedef TDynArray< ProcessingRequest >					Requests;
	typedef TDynArray< RecomputationBox >					RecomputationBoxes;			

	RequestsCollection						m_requestsCollection;
	Requests								m_requestsBuffer;
	RecomputationBoxes						m_boxesBuffer;
	CObstaclesMapper*						m_mapper;
	CNodeSetProcessingContext				m_nodeSetProcessingContext;
#ifndef RED_FINAL_BUILD
	Bool									m_isCollecting;
#endif

	void					AddRequest( AreaId areaId, Uint16 category, Uint32 id, ERequestType requestType );
public:
	CComponentRuntimeProcessingContext( CObstaclesMapper* mapper );
	~CComponentRuntimeProcessingContext();

	void					BeginRequestsCollection( CPathLibWorld& pathlib );

	void					AddNodeSetAttachRequest( AreaId areaId, Uint16 category, Uint32 nodeSetId );
	void					AddNodeSetDetachRequest( AreaId areaId, Uint16 category, Uint32 nodeSetId );
	void					AddRecomputationRequest( AreaId areaId, const Box& localBBox );

	//void					AddObstacleRemovalRequest( AreaId areaId, CObstacle::Id obstacleId );
	void					EndRequestsCollection( CPathLibWorld& pathlib );

	CObstaclesMapper*		GetMapper() const										{ return m_mapper; }
};


};		// namespace PathLib
