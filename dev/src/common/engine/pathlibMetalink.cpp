/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibMetalink.h"

#include "component.h"
#include "pathlibAreaDescription.h"
#include "pathlibComponent.h"
#include "pathlibConnectorsBin.h"
#include "pathlibCookerData.h"
#include "pathlibMetalinkComponent.h"
#include "pathlibObstacle.h"
#include "pathlibObstacleMapper.h"
#include "pathlibObstaclesMap.h"
#include "pathlibSimpleBuffers.h"
#include "pathlibWorld.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// CMetalink
////////////////////////////////////////////////////////////////////////////
CNavModyfication::CNavModyfication( Id id, CComponent* component )
 	: m_isAttached( false )
	, m_id( id )
	, m_mapping( component )
{
	m_metalink = component->AsPathLibComponent()->AsMetalinkComponent()->CreateMetalinkSetup();
}

CNavModyfication::~CNavModyfication()
{
}


Bool CNavModyfication::SimplifyGraphInput( CMetalinkConfigurationCommon& data, CAreaDescription* area, Float personalSpace, Bool initialSimplification ) const
{
	CPathLibWorld& pathlib = area->GetPathLib();

	auto& nodesList = data.m_nodes;
	auto& connectionList = data.m_connections;

	if ( nodesList.Empty() )
	{
		return false;
	}

	// initialize processing flags
	for ( Uint32 i = 0, n = nodesList.Size(); i != n; ++i )
	{
		nodesList[ i ].m_processingFlags = 0;
		nodesList[ i ].m_targetArea = INVALID_AREA_ID;
	}

	AreaId otherAreaId = INVALID_AREA_ID;
	// check nodes availability
	for ( Uint32 i = 0, n = nodesList.Size(); i != n; ++i )
	{
		auto& node = nodesList[ i ];

		if ( !area->VContainsPoint( node.m_pos ) )
		{
			node.m_processingFlags |= NODEPROCESSING_IS_OUT_OF_AREA;
			if ( !pathlib.TestLocation( otherAreaId, node.m_pos, personalSpace, CT_NO_DYNAMIC_OBSTACLES ) )
			{
				node.m_processingFlags |= NODEPROCESSING_IS_UNACCESSIBLE;
			}
		}
		else
		{
			CCircleQueryData::MultiArea query( CCircleQueryData( CT_DEFAULT | CT_NO_DYNAMIC_OBSTACLES, node.m_pos, personalSpace ) );
			if ( !area->TMultiAreaQuery( query ) )
			{
				node.m_processingFlags |= NODEPROCESSING_IS_UNACCESSIBLE;
			}
		}
		
		
	}

	// throw away disabled connections
	for ( Int32 i = connectionList.Size() - 1; i >= 0; --i )
	{
		auto& connection = connectionList[ i ];
		auto& node0 = data.m_nodes[ connection.m_ind[ 0 ] ];
		auto& node1 = data.m_nodes[ connection.m_ind[ 1 ] ];
		Bool isOk = true;
		// check if one node is unavailable or both are out of area
		if ( (node0.m_processingFlags & NODEPROCESSING_IS_UNACCESSIBLE) || (node1.m_processingFlags & NODEPROCESSING_IS_UNACCESSIBLE)
			|| ( (node0.m_processingFlags & NODEPROCESSING_IS_OUT_OF_AREA) && (node1.m_processingFlags & NODEPROCESSING_IS_OUT_OF_AREA) ) )
		{
			isOk = false;
		}
		// for non-ghost links do navigation test
		else if ( ( connection.m_linkFlags & NF_IS_GHOST_LINK ) == 0 )
		{
			PathLib::AreaId myArea = area->GetId();
			// we might start test from different area (rare case)
			if ( !pathlib.TestLine( myArea, node0.m_pos, node1.m_pos, personalSpace, CT_DEFAULT | CT_NO_DYNAMIC_OBSTACLES ) )
			{
				isOk = false;
			}
		}

		// check if one node is out of area
		if ( !initialSimplification && isOk && ( (node0.m_processingFlags & NODEPROCESSING_IS_OUT_OF_AREA) || (node1.m_processingFlags & NODEPROCESSING_IS_OUT_OF_AREA) ) )
		{
			// as we checked it b4 one area is cool.
			Bool node0IsCool = (node0.m_processingFlags & NODEPROCESSING_IS_OUT_OF_AREA) == 0;
			ASSERT( ( (node1.m_processingFlags & NODEPROCESSING_IS_OUT_OF_AREA) == 0 ) != node0IsCool );
			auto& coolNode = node0IsCool ? node0 : node1;
			auto& failNode = node0IsCool ? node1 : node0;

			CAreaDescription* targetArea = pathlib.GetAreaAtPosition( failNode.m_pos, otherAreaId );
			ASSERT( targetArea && targetArea != area, TXT("We already marked this node as reachable, it can't have no area!") );
			if ( !targetArea || targetArea == area )
			{
				isOk = false;
			}
			// compare area ids
			else if ( targetArea->GetId() > area->GetId() )
			{
				// don't create connection, but do create connector, as connection will be possibly created by other area
				coolNode.m_processingFlags |= NODEPROCESSING_OK | NODEPROCESSING_POSSIBLE_CONNECTOR;
				coolNode.m_targetArea = targetArea->GetId();
				isOk = false;
			}
			else
			{
				// create connector
				failNode.m_processingFlags |= NODEPROCESSING_OK | NODEPROCESSING_POSSIBLE_CONNECTOR;
				failNode.m_targetArea = targetArea->GetId();
			}
		}

		if ( isOk )
		{
			node0.m_processingFlags |= NODEPROCESSING_OK;
			node1.m_processingFlags |= NODEPROCESSING_OK;
		}
		else
		{
			data.m_connections.RemoveAtFast( i );
		}
	}

	// mark unused nodes for deletion
	Bool isDeletingNodes = false;

	for ( Uint32 i = 0, n = nodesList.Size(); i != n; ++i )
	{
		auto& node = nodesList[ i ];
		if ( (node.m_processingFlags & NODEPROCESSING_OK) == 0 )
		{
			isDeletingNodes = true;
			node.m_nodeFlags |= NODEPROCESSING_MARKED_FOR_DELETION;
		}
	}

	if ( isDeletingNodes )
	{
		// throw out unused nodes
		Int16 nextNodeIdx = 0;
		Bool hasShift = false;
		TDynArray< Int16 > nodeShiftedIndexes( nodesList.Size() );

		for ( Uint32 i = 0, n = nodesList.Size(); i != n; ++i )
		{
			if ( ( nodesList[ i ].m_nodeFlags & NODEPROCESSING_MARKED_FOR_DELETION ) == 0 )
			{
				if ( hasShift )
				{
					nodesList[ nextNodeIdx ] = nodesList[ i ];
				}
				nodeShiftedIndexes[ i ] = nextNodeIdx++;
			}
			else
			{
				nodeShiftedIndexes[ i ] = -1;
				hasShift = true;
			}
		}

		nodesList.ResizeFast( nextNodeIdx );

		if ( nodesList.Empty() )
		{
			return false;
		}

		// reindex connections
		for ( Uint32 i = 0, n = connectionList.Size(); i != n; ++i )
		{
			auto& con = connectionList[ i ];
			con.m_ind[ 0 ] = nodeShiftedIndexes[ con.m_ind[ 0 ] ];
			con.m_ind[ 1 ] = nodeShiftedIndexes[ con.m_ind[ 1 ] ];
		}
	}
	

	return true;
}

CNavModyfication* CNavModyfication::AsNavModyfication()
{
	return this;
}

Bool CNavModyfication::Compute( IMetalinkComponent* component, CMetalinkConfiguration& configuration, CAreaDescription* area )
{
	CMetalinkComputedData* metalinkData = new CMetalinkComputedData();

	metalinkData->CopyGraphFrom( configuration );
	metalinkData->m_injectGraph = configuration.m_injectGraph;

	// precompute data
	Float lowestPersonalSpace = area->GetPathLib().GetGlobalSettings().GetCategoryPersonalSpace( 0 );

	if ( !SimplifyGraphInput( *metalinkData, area, lowestPersonalSpace, true ) )
	{
		delete metalinkData;
		return false;
	}

	if ( configuration.m_internalObstacle )
	{
		// compute internal shape
		CObstacleSpawnContext context( *configuration.m_internalObstacle );
		area->VPrecomputeObstacleSpawnData( context );
		metalinkData->m_internalShape = area->GetObstaclesMap()->ComputeShape( context );
	}

	metalinkData->ComputeBBox();

	CNavigationCookingContext* cookingContext = area->GetPathLib().GetCookingContext();
	ASSERT( cookingContext );
	CGlobalConnectorsBin::MetalinkId m;
	m.m_area = area->GetId();
	m.m_navModId = m_id;
	cookingContext->GetPathlibCookerData()->GetGlobalConnections()->StoreMetalinkData( m, metalinkData );

	return true;
}

Bool CNavModyfication::NavgraphInjection( CNavGraph* navgraph )
{
	CAreaDescription* area = navgraph->GetArea();
	CNavigationCookingContext* cookingContext = area->GetPathLib().GetCookingContext();
	if ( !cookingContext )
	{
		// we need to support that case for navmesh generation
		return false;
	}
	CGlobalConnectorsBin* connectionsData = cookingContext->GetPathlibCookerData()->GetGlobalConnections();


	CGlobalConnectorsBin::MetalinkId m;
	m.m_area = area->GetId();
	m.m_navModId = m_id;
	CMetalinkComputedData* metalinkData = connectionsData->GetMetalinkData( m );
	if ( !metalinkData || !metalinkData->m_injectGraph )
	{
		return false;
	}

	CMetalinkGraph graph;
	graph.CopyGraphFrom( *metalinkData );

	Float personalSpace = navgraph->GetPersonalSpace();
	SimplifyGraphInput( graph, area, personalSpace, false );

	if ( graph.IsEmpty() )
	{
		return false;
	}

	auto& nodesDef = graph.m_nodes;
	auto& connsDef = graph.m_connections;

	TStaticArray< CNavNode::Index, CMetalinkGraph::MAX_NODES > createdNodes;
	createdNodes.Resize( nodesDef.Size() );

	// create nodes
	for ( Uint32 i = 0, n = nodesDef.Size(); i != n; ++i )
	{
		CMetalinkGraph::Node& nodeDef = nodesDef[ i ];
		CNavNode& node = navgraph->AddNode( nodeDef.m_pos, nodeDef.m_nodeFlags );
		createdNodes[ i ] = node.GetIndex();
	}

	// interconnect nodes
	for ( Uint32 i = 0, n = connsDef.Size(); i != n; ++i )
	{
		const auto& con = connsDef[ i ];
		CNavNode* n0 = navgraph->GetNode( createdNodes[ con.m_ind[ 0 ] ] );
		CNavNode* n1 = navgraph->GetNode( createdNodes[ con.m_ind[ 1 ] ] );

		Bool isCustomLink = (con.m_linkFlags & NF_IS_CUSTOM_LINK) != 0;

		navgraph->VGenerationConnectNodes( *n0, *n1, con.m_linkFlags | NF_IS_IN_NODESET, CalculateLinkCost( (n0->GetPosition() - n1->GetPosition()).Mag() ) );
	}

	// create external connections (connect to graph)
	CNavNode* firstNode = navgraph->GetNode( createdNodes[ 0 ] );
	if ( !navgraph->ConnectNodeList( firstNode, createdNodes.Size(), NF_DEFAULT, true ) )
	{
		for ( auto it = createdNodes.Begin(), end = createdNodes.End(); it != end; ++it )
		{
			CNavNode::Index idx = *it;
			navgraph->MarkNodeForDeletion( idx );
		}
		return true;
	}

	// query for external connectors
	for ( Uint32 i = 0, n = nodesDef.Size(); i != n; ++i )
	{
		CMetalinkGraph::Node& nodeDef = nodesDef[ i ];
		if ( nodeDef.m_processingFlags & NODEPROCESSING_POSSIBLE_CONNECTOR )
		{
			CNavNode& node = *navgraph->GetNode( createdNodes[ i ] );

			CGlobalConnectorsBin::Connection con;
			con.m_category = navgraph->GetCategory();
			con.m_areaFrom = area->GetId();
			con.m_areaTo = nodeDef.m_targetArea;
			con.m_idFrom = node.GetFullId();
			connectionsData->StoreConnection( con );
		}

	}

	return true;
}

Bool CNavModyfication::OnGraphGeneration( CNavGraph* navgraph )
{
	CAreaDescription* area = navgraph->GetArea();
	CNavigationCookingContext* cookingContext = area->GetPathLib().GetCookingContext();
	if ( !cookingContext )
	{
		// we need to support that case for navmesh generation
		return false;
	}
	CGlobalConnectorsBin* connectionsData = cookingContext->GetPathlibCookerData()->GetGlobalConnections();

	
	CGlobalConnectorsBin::MetalinkId m;
	m.m_area = area->GetId();
	m.m_navModId = m_id;
	CMetalinkComputedData* metalinkData = connectionsData->GetMetalinkData( m );
	if ( !metalinkData || metalinkData->m_injectGraph )
	{
		return false;
	}

	CMetalinkGraph graph;
	graph.CopyGraphFrom( *metalinkData );

	Float personalSpace = navgraph->GetPersonalSpace();
	SimplifyGraphInput( graph, area, personalSpace, false );

	if ( graph.IsEmpty() )
	{
		ClearNodeSet( navgraph );
		return false;
	}

	auto& nodesDef = graph.m_nodes;
	auto& connsDef = graph.m_connections;

	TStaticArray< CNavNode::Index, CMetalinkGraph::MAX_NODES > createdNodes;
	createdNodes.Resize( nodesDef.Size() );

	CNavgraphNodeSet* nodeSet = RegisterNodeSet( navgraph );

	// create nodes
	for ( Uint32 i = 0, n = nodesDef.Size(); i != n; ++i )
	{
		CMetalinkGraph::Node& nodeDef = nodesDef[ i ];
		CNavNode& node = nodeSet->AddNode( nodeDef.m_pos, nodeDef.m_nodeFlags | NF_IS_IN_NODESET );
		createdNodes[ i ] = node.GetIndex();
	}

	// interconnect nodes
	for ( Uint32 i = 0, n = connsDef.Size(); i != n; ++i )
	{
		const auto& con = connsDef[ i ];
		CNavNode* n0 = nodeSet->GetNode( createdNodes[ con.m_ind[ 0 ] ] );
		CNavNode* n1 = nodeSet->GetNode( createdNodes[ con.m_ind[ 1 ] ] );
		Bool isCustomLink = (con.m_linkFlags & NF_IS_CUSTOM_LINK) != 0;
		if ( isCustomLink )
		{
			n0->AddFlags( NF_IS_CUSTOM_LINK );
			n1->AddFlags( NF_IS_CUSTOM_LINK );
		}
		nodeSet->ConnectNodes( *n0, *n1, con.m_linkFlags | NF_IS_IN_NODESET, isCustomLink ? con.m_linkCost : CalculateLinkCost( (n0->GetPosition() - n1->GetPosition()).Mag() ) );
	}

	CObstacleShape* shape = metalinkData->m_internalShape;

	// remove base connections that overlap with shape
	if ( shape )
	{
		navgraph->CutOutWithShape( shape );
	}

	// create external connections (connect to graph)
	auto& nodesList = nodeSet->GetNodesArray();
	if ( !navgraph->ConnectNodeList( nodesList.TypedData(), nodesList.Size(), NF_IS_IN_NODESET, true ) )
	{
		ClearNodeSet( navgraph );
		return false;
	}

	// query for external connectors
	for ( Uint32 i = 0, n = nodesDef.Size(); i != n; ++i )
	{
		CMetalinkGraph::Node& nodeDef = nodesDef[ i ];

		if ( nodeDef.m_processingFlags & NODEPROCESSING_POSSIBLE_CONNECTOR )
		{
			CNavNode& node = *nodeSet->GetNode( createdNodes[ i ] );

			CGlobalConnectorsBin::Connection con;
			con.m_category = navgraph->GetCategory();
			con.m_areaFrom = area->GetId();
			con.m_areaTo = nodeDef.m_targetArea;
			con.m_idFrom = node.GetFullId();
			connectionsData->StoreConnection( con );
		}
	}

	m_wasApplied = true;

	return true;
}

void CNavModyfication::OnPostLoad( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	Super::OnPostLoad( area );

	CObstaclesMapper* mapper = area->GetPathLib().GetObstaclesMapper();

	CObstaclesMapper::ObstacleInfo& info = mapper->RequestMapping( m_mapping );

	info.m_areaInfo.PushBack( CObstaclesMapper::ObstacleAreaInfo( area->GetId(), m_id ) );

	if ( info.m_isRuntimeObstacleEnabled )
	{
		Enable( area, context );
	}
}

void CNavModyfication::OnPreUnload( CAreaDescription* area )
{
	CPathLibWorld& pathlib = area->GetPathLib();

	CObstaclesMapper* mapper = pathlib.GetObstaclesMapper();
	CObstaclesMapper::ObstacleInfo* info = mapper->GetMapping( m_mapping );
	if ( info )
	{
		CObstaclesMapper::ObstacleAreaInfo obsInfo( area->GetId(), m_id );
		info->m_areaInfo.RemoveFast( obsInfo );
		if ( info->m_isRuntimeObstacleEnabled == false && info->m_areaInfo.Empty() )
		{
			mapper->ForgetMapping( m_mapping );
		}
	}
}

void CNavModyfication::OnRemoval( CAreaDescription* area )
{
	INodeSetPack::Clear( area );
}

void CNavModyfication::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	Super::WriteToBuffer( writer );

	writer.Put( m_id );
	writer.Put( m_mapping );

	CMetalinkSetupFactory::SaveToBuffer( writer, m_metalink.Get() );
}
Bool CNavModyfication::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !Super::ReadFromBuffer( reader ) )
	{
		return false;
	}

	if ( !reader.Get( m_id ) )
	{
		return false;
	}
	if ( !reader.Get( m_mapping ) )
	{
		return false;
	}

	m_metalink = CMetalinkSetupFactory::GetInstance().NewFromBuffer( reader );
	if ( !m_metalink )
	{
		return false;
	}

	return true;
}
CNavModyfication* CNavModyfication::NewFromBuffer( CSimpleBufferReader& reader )
{
	CNavModyfication* mod = new CNavModyfication();
	if ( !mod->ReadFromBuffer( reader ) )
	{
		delete mod;
		return NULL;
	}
	return mod;
}

void CNavModyfication::Enable( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	if ( !m_isAttached )
	{
		m_isAttached = true;

		Attach( area, context );
	}
}
void CNavModyfication::Disable( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	if ( m_isAttached )
	{
		m_isAttached = false;
		Detach( area, context );
	}
}

////////////////////////////////////////////////////////////////////////////
// CMetalinkSet
////////////////////////////////////////////////////////////////////////////
CNavModyficationMap::CNavModyficationMap()
	: CVersionTracking()
	, m_area( NULL )
	, m_prevMetalinkId( 0 )
{}

CNavModyficationMap::~CNavModyficationMap()
{
	m_metalinks.ClearPtr();
}

Bool CNavModyficationMap::CreateMetalink( CNavModyfication::Id id, IMetalinkComponent* component, CMetalinkConfiguration& configuration )
{
	auto it = m_metalinks.Find( id );
	if ( it == m_metalinks.End() )
	{
		CComponent* engineComponent = component->AsEngineComponent();
		it = m_metalinks.Insert( new CNavModyfication( id, engineComponent ) );
	}
	CNavModyfication* metalink = *it;
	if ( !metalink->Compute( component, configuration, m_area ) )
	{
		m_metalinks.Erase( it );
		delete metalink;
		return false;
	}

	MarkVersionDirty();
	m_area->MarkDirty( CAreaDescription::DIRTY_GENERATE );

	return true;
}

void CNavModyficationMap::RemoveMetalink( CNavModyfication::Id id )
{
	auto it = m_metalinks.Find( id );
	if ( it != m_metalinks.End() )
	{
		(*it)->OnRemoval( m_area );
		delete *it;
		m_metalinks.Erase( it );

		MarkVersionDirty();
		m_area->MarkDirty( CAreaDescription::DIRTY_GENERATE );
	}
}

void CNavModyficationMap::RemoveNotAppliedModyfications()
{
	for ( Int32 i = m_metalinks.Size() - 1; i >= 0; --i )
	{
		if ( !m_metalinks[ i ]->WasApplied() )
		{
			delete m_metalinks[ i ];
			m_metalinks.RemoveAt( i );
		}
	}
}

void CNavModyficationMap::NavgraphInjection( CNavGraph* navgraph )
{
	Bool modified = false;
	for ( CNavModyfication* mod : m_metalinks )
	{
		if ( mod->NavgraphInjection( navgraph ) )
		{
			modified = true;
		}

	}
	if ( modified )
	{
		navgraph->DeleteMarked();
	}
}

void CNavModyficationMap::OnGraphGeneration( CNavGraph* navgraph )
{
	Bool modified = false;
	for ( CNavModyfication* mod : m_metalinks )
	{
		if ( mod->OnGraphGeneration( navgraph ) )
		{
			modified = true;
		}

	}
	if ( modified )
	{
		navgraph->DeleteMarked();
	}
}
void CNavModyficationMap::OnPreLoad( CAreaDescription* area )
{
	m_area = area;
}
void CNavModyficationMap::OnPostLoad( CComponentRuntimeProcessingContext& context )
{
	for ( auto it = m_metalinks.Begin(), end = m_metalinks.End(); it != end; ++it )
	{
		CNavModyfication* mod = *it;

		mod->OnPostLoad( m_area, context );
	}
}

void CNavModyficationMap::OnPreUnload( CAreaDescription* area )
{
	for ( auto it = m_metalinks.Begin(), end = m_metalinks.End(); it != end; ++it )
	{
		CNavModyfication* mod = *it;

		mod->OnPreUnload( area );
	}
}

void CNavModyficationMap::RuntimeEnableMetalink( CNavModyfication::Id id, CComponentRuntimeProcessingContext& context )
{
	auto itFind = m_metalinks.Find( id );
	if ( itFind != m_metalinks.End() )
	{
		(*itFind)->Enable( m_area, context );
	}
}
void CNavModyficationMap::RuntimeDisableMetalink( CNavModyfication::Id id, CComponentRuntimeProcessingContext& context )
{
	auto itFind = m_metalinks.Find( id );
	if ( itFind != m_metalinks.End() )
	{
		(*itFind)->Disable( m_area, context );
	}
}

void CNavModyficationMap::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	Uint32 nodesetCount = m_metalinks.Size();
	writer.Put( nodesetCount );
	for ( Uint32 i = 0; i < nodesetCount; ++i )
	{
		m_metalinks[ i ]->WriteToBuffer( writer );
	}
}
Bool CNavModyficationMap::ReadFromBuffer( CSimpleBufferReader& reader )
{
	Uint32 nodesetCount;
	if ( !reader.Get( nodesetCount ) )
	{
		return false;
	}
	m_metalinks.Resize( nodesetCount );
	for ( Uint32 i = 0; i < nodesetCount; ++i )
	{
		CNavModyfication* mod = CNavModyfication::NewFromBuffer( reader );
		if ( mod == NULL )
		{
			m_metalinks.Resize( i );
			return false;
		}
		m_metalinks[ i ] = mod;
		m_prevMetalinkId = Max( mod->GetId(), m_prevMetalinkId );
	}

	return true;
}

};			// namespace PathLib

