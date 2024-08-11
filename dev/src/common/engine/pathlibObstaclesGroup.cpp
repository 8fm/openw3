/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibObstaclesGroup.h"

#include "pathlibAreaDescription.h"
#include "pathlibNodeSetRequestsList.h"
#include "pathlibObstacle.h"
#include "pathlibObstacleDetour.h"
#include "pathlibObstacleShape.h"
#include "pathlibObstaclesMap.h"
#include "pathlibWorld.h"
#include "pathlibWorldLayersMapping.h"


namespace PathLib
{



///////////////////////////////////////////////////////////////////////////////
// CObstacleGroup
///////////////////////////////////////////////////////////////////////////////
CObstacleGroup::CObstacleGroup()
	: m_isAttached( false )
	, m_obstaclesBBox( Box::RESET_STATE )
{

}

CObstacleGroup::~CObstacleGroup()
{

}

void CObstacleGroup::Attach( CObstaclesMap* obstaclesMap, CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	if ( m_isAttached )
	{
		return;
	}
	m_isAttached = true;

	for ( ExternalDepenentId id : m_obstaclesList )
	{
		CObstacle* obstacle = obstaclesMap->GetObstacle( id );
		if ( obstacle )
		{
			CDynamicGroupedObstacle* dynamicObstacle = static_cast< CDynamicGroupedObstacle* >( obstacle );
			dynamicObstacle->Show( area, context );
		}
	}

	context.AddRecomputationRequest( area->GetId(), m_obstaclesBBox );

	INodeSetPack::Attach( area, context );
}

void CObstacleGroup::Detach( CObstaclesMap* obstaclesMap, CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	if ( !m_isAttached )
	{
		return;
	}
	m_isAttached = false;

	for ( ExternalDepenentId id : m_obstaclesList )
	{
		CObstacle* obstacle = obstaclesMap->GetObstacle( id );
		if ( obstacle )
		{
			CDynamicGroupedObstacle* dynamicObstacle = static_cast< CDynamicGroupedObstacle* >( obstacle );
			dynamicObstacle->Hide( area, context );
		}
	}

	context.AddRecomputationRequest( area->GetId(), m_obstaclesBBox );

	INodeSetPack::Detach( area, context );
}

void CObstacleGroup::OnPostLoad( CAreaDescription* area )
{
	INodeSetPack::OnPostLoad( area );
}

void CObstacleGroup::AddObstacle( CObstacle* obstacle )
{
	ASSERT( Find( m_obstaclesList.Begin(), m_obstaclesList.End(), obstacle->GetId() ) == m_obstaclesList.End() );
	m_obstaclesList.PushBack( obstacle->GetId() );
	const CObstacleShape* shape = obstacle->GetShape();
	Box bbox( Vector( shape->GetBBoxMin() ), Vector( shape->GetBBoxMax() ) );
	m_obstaclesBBox.AddBox( bbox );
}

void CObstacleGroup::SimplifyShapes( CObstaclesMap* obstaclesMap )
{
	TDynArray< ExternalDepenentId > obstaclesToRemove;

	// collect obstacles for removal
	for ( Uint32 i = 0, n = m_obstaclesList.Size(); i < n; ++i )
	{
		CObstacle* obstacle1 = obstaclesMap->GetObstacle( m_obstaclesList[ i ] );
		CObstacleShape* shape1 = obstacle1->GetShape();

		for ( Uint32 j = i+1; j < n; ++j )
		{
			CObstacle* obstacle2 = obstaclesMap->GetObstacle( m_obstaclesList[ j ] );
			CObstacleShape* shape2 = obstacle2->GetShape();

			if ( shape2->Contains( shape1 ) )
			{
				obstaclesToRemove.PushBack( m_obstaclesList[ i ] );
				break;
			}
			else if ( shape1->Contains( shape2 ) )
			{
				obstaclesToRemove.PushBack( m_obstaclesList[ j ] );
				continue;
			}
		}
	}

	// sort obstacles - as there can be duplicates on the list
	Sort( obstaclesToRemove.Begin(), obstaclesToRemove.End() );
	ExternalDepenentId lastId = EXTERNAL_INVALID_ID;

	// remove obstructed obstacles
	for ( ExternalDepenentId obstacle2Remove : obstaclesToRemove )
	{
		// check if obstacle is already removed
		if ( obstacle2Remove == lastId )
		{
			continue;
		}

		obstaclesMap->RemoveObstacle( obstacle2Remove );
		m_obstaclesList.RemoveFast( obstacle2Remove );			// n^2
		lastId = obstacle2Remove;
	}
}

void CObstacleGroup::PostGraphGeneration( CNavGraph* navgraph )
{
	CAreaDescription* area = navgraph->GetArea();
	CObstaclesMap* obstaclesMap = area->GetObstaclesMap();
	Float personalSpace = navgraph->GetPersonalSpace();
	Float maxNodesDistance = area->GetMaxNodesDistance();

	// mark all obstacles in group
	for ( ExternalDepenentId id : m_obstaclesList )
	{
		CObstacle* obstacle = obstaclesMap->GetObstacle( id );
		CDynamicGroupedObstacle* dynamicObstacle = static_cast< CDynamicGroupedObstacle* >( obstacle );
		dynamicObstacle->RemoveFlags( CObstacle::IS_COLLISION_DISABLED );
		dynamicObstacle->MarkCollision( obstaclesMap );
	}

	// compute shapes detour info
	CObstaclesDetourInfo detourInfo;
	for ( ExternalDepenentId id : m_obstaclesList )
	{
		CObstacle* obstacle = obstaclesMap->GetObstacle( id );
		CObstacleShape* shape = obstacle->GetShape();

		ObstacleDetour detour;
		shape->VComputeDetour( area, personalSpace, detour );
		if ( !detour.Empty() )
		{
			detourInfo.Push( detour );
		}
	}

	// create detour nodeset graph
	CNavgraphNodeSet* nodeSet = RegisterNodeSet( navgraph );
	// connect graph with main graph
	DetourComputationAlgorithm::GenerateDetour( area, navgraph, nodeSet, navgraph->GetCategory(), detourInfo );
	// block generated nodes
	nodeSet->GenerationBlockAllNodes();

	// unmark all obstacles from group
	for ( ExternalDepenentId id : m_obstaclesList )
	{
		CObstacle* obstacle = obstaclesMap->GetObstacle( id );
		CDynamicGroupedObstacle* dynamicObstacle = static_cast< CDynamicGroupedObstacle* >( obstacle );
		dynamicObstacle->UnmarkCollision( obstaclesMap );
		dynamicObstacle->AddFlags( CObstacle::IS_COLLISION_DISABLED );
		
	}
}

Bool CObstacleGroup::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !reader.SmartGet( m_obstaclesList ) )
	{
		return false;
	}

	if ( !reader.Get( m_obstaclesBBox ) )
	{
		return false;
	}

	if ( !INodeSetPack::ReadFromBuffer( reader ) )
	{
		return false;
	}

	return true;
}

void CObstacleGroup::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	writer.SmartPut( m_obstaclesList );
	writer.Put( m_obstaclesBBox );

	INodeSetPack::WriteToBuffer( writer );
}

///////////////////////////////////////////////////////////////////////////////
// CObstacleGroupsCollection
///////////////////////////////////////////////////////////////////////////////
CObstacleGroupsCollection::CObstacleGroupsCollection()
{

}
CObstacleGroupsCollection::~CObstacleGroupsCollection()
{
	for ( auto pair : m_groups )
	{
		delete pair.m_second;
	}
}

CObstacleGroup* CObstacleGroupsCollection::GetObstacleGroup( const SLayerMapping& layer ) const
{
	auto itFind = m_groups.Find( layer );
	if ( itFind == m_groups.End() )
	{
		return nullptr;
	}
	return itFind->m_second;
}

CObstacleGroup* CObstacleGroupsCollection::LazyCreateObstacleGroup( CObstaclesMap* obstaclesMap, const SLayerMapping& layer )
{
	auto itFind = m_groups.Find( layer );
	if ( itFind == m_groups.End() )
	{
		CAreaDescription* area = obstaclesMap->GetArea();
		area->GetPathLib().GetWorldLayers().AddAreaDependency( layer, area->GetId() );

		CObstacleGroup* group = new CObstacleGroup();
		m_groups.Insert( layer, group );
		return group;
	}
	return itFind->m_second;
}

void CObstacleGroupsCollection::OnPostLoad( CObstaclesMap* obstacles, CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	// run standard post-load code
	for ( auto pair : m_groups )
	{
		pair.m_second->OnPostLoad( area );
	}
	// 'attach' all layers groups that are already enabled
	const CWorldLayersMapping& worldLayers = area->GetPathLib().GetWorldLayers();
	for ( Uint32 id = 0, n = m_groups.Size(); id < n; ++id )
	{
		const auto& pair = m_groups[ id ];
		pair.m_second->m_id = id;
		if ( worldLayers.IsLayerEnabled( pair.m_first ) )
		{
			pair.m_second->Attach( obstacles, area, context );
		}
	}
}

void CObstacleGroupsCollection::SimplifyShapes( CObstaclesMap* obstaclesMap )
{
	for ( auto pair : m_groups )
	{
		pair.m_second->SimplifyShapes( obstaclesMap );
	}
}

void CObstacleGroupsCollection::PostGraphGeneration( CNavGraph* navgraph )
{
	for ( auto pair : m_groups )
	{
		pair.m_second->PostGraphGeneration( navgraph );
	}
}

Bool CObstacleGroupsCollection::ReadFromBuffer( CSimpleBufferReader& reader )
{
	Uint16 size;
	if ( !reader.Get( size ) )
	{
		return false;
	}

	m_groups.Reserve( size );

	for ( Uint16 i = 0; i < size; ++i )
	{
		SLayerMapping mapping;
		
		if ( !reader.Get( mapping ) )
		{
			return false;
		}

		// TODO: remove it with new binaries version!
		if ( !reader.Get( mapping ) )
		{
			return false;
		}

		CObstacleGroup* group = new CObstacleGroup();
		if ( !group->ReadFromBuffer( reader ) )
		{
			delete group;
			return false;
		}
		m_groups.PushBack( ObstaclesGroups::value_type( mapping, group ) );
	}


	return true;
}
void CObstacleGroupsCollection::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	Uint16 size = Uint16( m_groups.Size() );
	writer.Put( size );

	for ( auto pair : m_groups )
	{
		const SLayerMapping& mapping = pair.m_first;
		CObstacleGroup* group = pair.m_second;

		writer.Put( mapping );
		// TODO: remove it with new binaries version!
		writer.Put( mapping );
		group->WriteToBuffer( writer );
	}
}

};		// namespace PathLib

