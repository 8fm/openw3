#include "build.h"
#include "pathlibObstacle.h"

#include "pathlibNodeFinder.h"
#include "pathlibObstacleMapper.h"
#include "pathlibNavmeshArea.h"
#include "pathlibTerrain.h"
#include "pathlibObstacleShape.h"
#include "pathlibObstaclesMap.h"
#include "pathlibSimpleBuffers.h"


namespace PathLib
{

const Float CObstaclesMap::IGNORE_SHORT_MESHES_Z = 0.25f;


////////////////////////////////////////////////////////////////////////////
// CObstacle
CObstacle::~CObstacle()
{
	if ( m_shape )
	{
		delete m_shape;
	}
}
CObstacle* CObstacle::NewFromBuffer( CSimpleBufferReader& reader )
{
	CObstacle* ret;
	Uint32 flags;
	if ( !reader.Get( flags ) )
	{
		return NULL;
	}

	ret =
		(flags & IS_PERSISTANT)
		? static_cast< CObstacle* >( new CPersistantObstacle( flags ) )
		: (flags & IS_GROUPED)
		? static_cast< CObstacle* >( new CDynamicGroupedObstacle( flags ) )
		: static_cast< CObstacle* >( new CDynamicPregeneratedObstacle( flags ) );
	if ( !ret->ReadFromBuffer( reader ) )
	{
		delete ret;
		return NULL;
	}
	
	return ret;
}
void CObstacle::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	Uint32 saveableFlags = m_flags & FLAGS_SAVEABLE;
	writer.Put( saveableFlags );
	Bool hasShape = m_shape != NULL;
	writer.Put( hasShape );
	if ( hasShape )
	{
		m_shape->WriteToBuffer( writer );
	}

}
Bool CObstacle::ReadFromBuffer( CSimpleBufferReader& reader )
{
	Bool hasShape;
	if ( !reader.Get( hasShape ) )
	{
		return false;
	}
	if ( hasShape )
	{
		m_shape = CObstacleShape::NewFromBuffer( reader );
		if ( !m_shape )
		{
			return false;
		}
	}
	
	return true;
}

void CObstacle::ClearShape()
{
	PATHLIB_ASSERT( ( m_flags & IS_MARKED ) == 0, TXT("Obstacle that is already marked is clearing its shape! Crash imminent.") );
	if ( m_shape )
	{
		delete m_shape;
		m_shape = NULL;
	}
}

Bool CObstacle::CollideArea( const Box& localNaviAreaBBox, CNavmeshAreaDescription* naviArea, CAreaDescription* myArea )
{
	if ( !m_shape )
	{
		return false;
	}
	if ( !m_shape->TestBoundings( localNaviAreaBBox.Min.AsVector3(), localNaviAreaBBox.Max.AsVector3() ) )
	{
		return false;
	}

	if ( !naviArea->IsLoaded() )
	{
		return false;
	}
	CNavmesh* navmesh = naviArea->GetNavmesh();
	if ( !navmesh )
	{
		return false;
	}

	Box shapeBBox( m_shape->GetBBoxMin(), m_shape->GetBBoxMax() );
	myArea->VLocalToWorld( shapeBBox );
	naviArea->VWorldToLocal( shapeBBox );
	return navmesh->GetClosestTriangle( shapeBBox ) != CNavmesh::INVALID_INDEX;
}


template < class Functor >
RED_INLINE Bool CObstacle::GenerateDetourNodes( Functor& func )
{
	struct CollectNodes : public CNodeFinder::Handler
	{
		CAreaDescription*								m_area;
		CNavGraph*										m_navgraph;
		Float											m_personalSpace;
		CObstacleShape*									m_shape;
		TDynArray< TPair< CNavNode::Id, Uint32 > >		m_detourConnectors;
		const ObstacleDetour&							m_detour;
		Functor&										m_functor;

		CollectNodes( CNavGraph* navgraph, CObstacleShape* shape, const ObstacleDetour&	detour, Functor& functor )
			: m_area( navgraph->GetArea() )
			, m_navgraph( navgraph )
			, m_personalSpace( navgraph->GetPersonalSpace() )
			, m_shape( shape )
			, m_detour( detour )
			, m_functor( functor ) {}

		void Handle( CNavNode& node ) override
		{
			// check if node collides with our shape - if so mark it for deletion
			const Vector3& nodePosition = node.GetPosition();

			// check if node can connect to any detour point
			Uint32 bestDetourPoint = 0xffff;
			Float closestDetourPointDistSq = FLT_MAX;
			for ( Uint32 i = 0, n = m_detour.Size(); i != n; ++i )
			{
				Float distSq = (nodePosition - m_detour[ i ]).SquareMag();
				if ( distSq < closestDetourPointDistSq )
				{
					CWideLineQueryData query( CT_IGNORE_OTHER_AREAS, nodePosition, m_detour[ i ], m_personalSpace );
					if ( m_area->VSpatialQuery( query ) )
					{
						closestDetourPointDistSq = distSq;
						bestDetourPoint = i;
					}
				}
			}

			if ( bestDetourPoint != 0xffff )
			{
				m_detourConnectors.PushBack( TPair< CNavNode::Id, Uint32 >( node.GetFullId(), bestDetourPoint ) );
			}
		}
	};

	if ( !m_shape )
	{
		return false;
	}
	CNavGraph* navgraph = func.GetNavgraph();
	CAreaDescription* area = navgraph->GetArea();
	const CCentralNodeFinder& nodeFinder = navgraph->GetNodeFinder();
	Float personalSpace = navgraph->GetPersonalSpace();
	CObstacleShape* shape = m_shape;

	// compute detour nodes locations
	ObstacleDetour detour;
	m_shape->VComputeDetour( area, personalSpace, detour );

	if ( detour.Empty() )
	{
		return false;
	}

	Box detourBBox( Box::RESET_STATE );
	for ( Uint32 i = 0; i < detour.Size(); ++i )
	{
		detourBBox.AddPoint( detour[ i ] );
	}

	Float maxNodesDistance = navgraph->GetMaxNodesDistance();

	detourBBox.Min.AsVector3() -= Vector3( maxNodesDistance, maxNodesDistance, maxNodesDistance );
	detourBBox.Max.AsVector3() += Vector3( maxNodesDistance, maxNodesDistance, maxNodesDistance );

	CollectNodes collectNodes( navgraph, shape, detour, func );

	nodeFinder.IterateNodes( detourBBox, collectNodes, CCentralNodeFinder::AnyRegion( true, false ), NF_MARKED_FOR_DELETION );

	TStaticArray< CNavNode::Id, MAX_OBSTACLE_DETOUR_LEN > detourNodes;
	// create detour nodes
	detourNodes.Resize( detour.Size() );

	for ( Uint32 i = 0; i < detour.Size(); ++i )
	{
		CNavNode& node = func.CreateNode( detour[ i ] );
		detourNodes[ i ] = node.GetFullId();
	}

	// try to connect detour nodes

	// at first connect detour nodes to each other "in round"
	if ( detourNodes.Size() > 1 )
	{
		Uint32 prevNodeIdx = detourNodes.Size() - 1;
		for ( Uint32 i = 0, n = ((detourNodes.Size() > 2) ? detourNodes.Size() : 1); i < n; ++i )
		{
			CNavNode* node = func.GetDetourNode( detourNodes[ i ] );
			CNavNode::Id prevNodeId = detourNodes[ prevNodeIdx ];
			// try to connect with previous detour point
			CNavNode* prevNode = func.GetDetourNode( prevNodeId );
			CWideLineQueryData spatialQuery( CT_DEFAULT, node->GetPosition(), prevNode->GetPosition(), personalSpace );
			if ( area->VSpatialQuery( spatialQuery ) )
			{
				func.CreateLink( detourNodes[ i ], prevNodeId );		// navgraph->GenerationConnectNode( detourNodes[ i ], prevNodeId );
			}
			prevNodeIdx = i;
		}
	}

	// now compute closest connector nodes
	// initialize data
	TStaticArray< TPair< CNavNode::Id, Float >, MAX_OBSTACLE_DETOUR_LEN > detourClosestConnectors;
	detourClosestConnectors.Resize( detour.Size() );
	for ( auto it = detourClosestConnectors.Begin(), end = detourClosestConnectors.End(); it != end; ++it )
	{
		(*it) = TPair< CNavNode::Id, Float >( CNavNode::Id::INVALID, FLT_MAX );
	}

	// compute closest connectors
	for ( auto it = collectNodes.m_detourConnectors.Begin(), end = collectNodes.m_detourConnectors.End(); it != end; ++it )
	{
		CNavNode* connectorNode = navgraph->GetNode( it->m_first );
		CNavNode* detourNode = func.GetDetourNode( detourNodes[ it->m_second ] );
		Float distSq = (connectorNode->GetPosition() - detourNode->GetPosition()).SquareMag();
		if ( distSq < detourClosestConnectors[ it->m_second ].m_second )
		{
			detourClosestConnectors[ it->m_second ].m_second = distSq;
			detourClosestConnectors[ it->m_second ].m_first = it->m_first;
		}
	}

	// connect detour with connectors
	for ( Uint32 i = 0, n = detourClosestConnectors.Size(); i != n; ++i )
	{
		if ( detourClosestConnectors[ i ].m_first != CNavNode::Id::INVALID )
		{
			func.CreateLink( detourClosestConnectors[ i ].m_first, detourNodes[ i ] );			// navgraph->GenerationConnectNode( detourClosestConnectors[ i ].m_first, detourNodes[ i ] );
		}

	}
	return true;
}

Bool CObstacle::MarkArea( const Box& localNaviBBox, CNavmeshAreaDescription* naviArea, CAreaDescription* myArea )
{
	if ( CollideArea( localNaviBBox, naviArea, myArea ) )
	{
		return m_areasInside.PushBackUnique( naviArea->GetId() );
	}
	return false;
}
void CObstacle::UpdateAreasInside( CAreaDescription* area )
{
	m_areasInside.ClearFast();

	if ( area->IsTerrainArea() && m_shape )
	{
		CTerrainAreaDescription* terrainArea = area->AsTerrainArea();

		struct CInstanceFunctor : public CInstanceMap::CInstanceFunctor
		{
		public:
			CInstanceFunctor( CObstacle* me, CTerrainAreaDescription* area )
				: m_this( me )
				, m_area( area ) {}

			Bool Handle( CNavmeshAreaDescription* naviArea ) override
			{
				Box areaBBox = naviArea->GetBBox();
				m_area->WorldToLocal( areaBBox );
				if ( m_this->CollideArea( areaBBox, naviArea, m_area ) )
				{
					m_this->m_areasInside.PushBack( naviArea->GetId() );
				}

				return false;
			}

			CObstacle*					m_this;
			CTerrainAreaDescription*	m_area;
			
		} functor( this, terrainArea );

		Box shapeBBox( m_shape->GetBBoxMin(), m_shape->GetBBoxMax() );
		Int32 x, y;
		terrainArea->GetTileCoords( x, y );
		terrainArea->GetPathLib().GetInstanceMap()->IterateAreasAt( x, y, shapeBBox, &functor );
	}
}

CDynamicPregeneratedObstacle* CObstacle::AsDynamicPregeneratedObstacle()
{
	return NULL;
}

void CObstacle::MarkCollision( CObstaclesMap* obstacles )
{
	if ( (m_flags & (IS_COLLISION_DISABLED | IS_MARKED)) == 0 )
	{
		if ( obstacles->Add2CollisionMap( this ) )
		{
			m_flags |= IS_MARKED;
		}
	}
}
void CObstacle::UnmarkCollision( CObstaclesMap* obstacles )
{
	if ( m_flags & IS_MARKED )
	{
		obstacles->RemoveFromCollisionMap( this );
		m_flags &= ~IS_MARKED;
	}
}

void CObstacle::OnPostLoad( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	MarkCollision( area->GetObstaclesMap() );
}
void CObstacle::OnPreUnload( CAreaDescription* area )
{

}

void CObstacle::SetMapping( const SComponentMapping& mapping, const SLayerMapping& layerMapping )
{
	
}

void CObstacle::Initialize( CObstaclesMap* obstaclesMap, const CObstacleSpawnData& spawnData )
{

}

void CObstacle::OnAddition( CObstaclesMap* obstaclesMap )
{
	MarkCollision( obstaclesMap );
}
void CObstacle::OnRemoval( CObstaclesMap* obstaclesMap )
{
	UnmarkCollision( obstaclesMap );
}
void CObstacle::PostGraphGeneration( CObstaclesMap* map, CNavGraph* navgraph )
{
}
void CObstacle::OnGraphClearance( CNavGraph* navgraph )
{
	UnmarkCollision( navgraph->GetArea()->GetObstaclesMap() );
}


void CObstacle::OnShow( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
}
void CObstacle::OnHide( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
}

void CObstacle::UpdateCollisionFlags( CNavGraph* navgraph, CNodeSetProcessingContext& context )
{
	Box bbox( Vector( m_shape->GetBBoxMin() ), Vector( m_shape->GetBBoxMax() ) );
	navgraph->RuntimeUpdateCollisionFlags( bbox, context );
}

////////////////////////////////////////////////////////////////////////////
// CDynamicGroupedObstacle

void CDynamicGroupedObstacle::Initialize( CObstaclesMap* obstaclesMap, const CObstacleSpawnData& spawnData )
{
	// NOTICE: to add obstacle to group it mush have a valid obstacle id. Thats why its done at 'Initialize' call, after adding to CObstaclesMap.
	obstaclesMap->GetObstacleGroups().LazyCreateObstacleGroup( obstaclesMap, spawnData.m_layerMapping )->AddObstacle( this );
}
void CDynamicGroupedObstacle::OnShow( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	Show( area, context );

}
void CDynamicGroupedObstacle::OnHide( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	Hide( area, context );
}
void CDynamicGroupedObstacle::Show( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	m_flags &= ~IS_COLLISION_DISABLED;

	MarkCollision( area->GetObstaclesMap() );
}
void CDynamicGroupedObstacle::Hide( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	m_flags |= IS_COLLISION_DISABLED;

	UnmarkCollision( area->GetObstaclesMap() );
}

void CDynamicGroupedObstacle::SetMapping( const SComponentMapping& mapping, const SLayerMapping& layerMapping )
{
	
}

////////////////////////////////////////////////////////////////////////////
// CDynamicIndependentObstacle
Bool CDynamicIndependentObstacle::Generate( CNavGraph* navgraph )
{
	struct Functor
	{
		Functor( CDynamicIndependentObstacle* me, CNavGraph* navgraph )
			: m_this( me )
			, m_graph( navgraph )
		{
			m_set = me->RegisterNodeSet( navgraph );
		}

		CNavNode& CreateNode( const Vector3& position )
		{
			return m_set->AddNode( position, NF_IS_OBSTACLE_DETOUR_NODE | NF_IS_IN_NODESET | NF_BLOCKED );
		}
		void CreateLink( CNavNode::Id n1, CNavNode::Id n2 )
		{
			CNavNode* p1 = m_set->GetNode( n1 );
			CNavNode* p2 = m_set->GetNode( n2 );

			m_graph->ConnectNode( n1, n2, NF_IS_OBSTACLE_DETOUR_NODE | NF_IS_IN_NODESET | NF_BLOCKED, CalculateLinkCost( (p1->GetPosition() - p2->GetPosition()).Mag() ) );
		}

		CNavGraph* GetNavgraph() const
		{
			return m_graph;
		}
		CNavNode* GetDetourNode( CNavNode::Id id )
		{
			return m_set->GetNode( id.m_index );
		}

		CDynamicIndependentObstacle* m_this;
		CNavGraph* m_graph; 
		CNavgraphNodeSet* m_set;
	} func( this, navgraph );
	
	if ( !GenerateDetourNodes( func ) )
	{
		ClearNodeSet( navgraph );
		return false;
	}
	return true;
}

void CDynamicIndependentObstacle::Show( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	if ( !m_isShown )
	{
		m_isShown = true;
		m_flags &= ~IS_COLLISION_DISABLED;

		MarkCollision( area->GetObstaclesMap() );

		Attach( area, context );

		context.AddRecomputationRequest( area->GetId(), Box( m_shape->GetBBoxMin(), m_shape->GetBBoxMax() ) );
	}
}
void CDynamicIndependentObstacle::Hide( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	if ( m_isShown )
	{
		m_isShown = false;
		m_flags |= IS_COLLISION_DISABLED;

		UnmarkCollision( area->GetObstaclesMap() );

		Detach( area, context );

		context.AddRecomputationRequest( area->GetId(), Box( m_shape->GetBBoxMin(), m_shape->GetBBoxMax() ) );
	}
}

void CDynamicIndependentObstacle::OnShow( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	Show( area, context );
}

void CDynamicIndependentObstacle::OnHide( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	Hide( area, context );
}


void CDynamicIndependentObstacle::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	Super::WriteToBuffer( writer );

	writer.Put( m_mapping );

	INodeSetPack::WriteToBuffer( writer );
}

Bool CDynamicIndependentObstacle::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !Super::ReadFromBuffer( reader ) )
	{
		return false;
	}

	if ( !reader.Get( m_mapping ) )
	{
		return false;
	}

	if ( !INodeSetPack::ReadFromBuffer( reader ) )
	{
		return false;
	}

	return true;
}
Bool CDynamicIndependentObstacle::IsUpdatingNodeCollisionOnAttach()
{
	return true;
}

void CDynamicIndependentObstacle::OnPostLoad( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	Super::OnPostLoad( area, context );

	INodeSetPack::OnPostLoad( area );
}

void CDynamicIndependentObstacle::OnPreUnload( CAreaDescription* area )
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
	
	Super::OnPreUnload( area );
}

void CDynamicIndependentObstacle::SetMapping( const SComponentMapping& mapping, const SLayerMapping& layerMapping )
{
	m_mapping = mapping;
}

////////////////////////////////////////////////////////////////////////////
// CDynamicPregeneratedObstacle

void CDynamicPregeneratedObstacle::OnAddition( CObstaclesMap* obstaclesMap )
{
	//CObstaclesMapper* globalObstacles = area->GetPathLib().GetObstaclesMapper();
	//if ( globalObstacles->IsDynamicObstacleEnabled( m_mapping ) )
	//{
	//	m_isShown = true;
	//	Show( area );
	//}
}
void CDynamicPregeneratedObstacle::OnRemoval( CObstaclesMap* obstaclesMap )
{
	//if ( m_isShown )
	//{
	//	Hide( area );	
	//}
}

void CDynamicPregeneratedObstacle::OnPostLoad( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	Super::OnPostLoad( area, context );

	CObstaclesMapper* mapper = area->GetPathLib().GetObstaclesMapper();

	CObstaclesMapper::ObstacleInfo& info = mapper->RequestMapping( m_mapping );

	info.m_areaInfo.PushBack( CObstaclesMapper::ObstacleAreaInfo( area->GetId(), m_id ) );

	if ( info.m_isRuntimeObstacleEnabled )
	{
		Show( area, context );
	}
}

void CDynamicPregeneratedObstacle::GenerationOnPreNodeSetConnection( CNavGraph* navgraph )
{
	CObstaclesMap* obstaclesMap = navgraph->GetArea()->GetObstaclesMap();
	m_flags &= ~IS_COLLISION_DISABLED;
	MarkCollision( obstaclesMap );
}
void CDynamicPregeneratedObstacle::GenerationOnPostNodeSetConnection( CNavGraph* navgraph )
{
	CObstaclesMap* obstaclesMap = navgraph->GetArea()->GetObstaclesMap();
	m_flags |= IS_COLLISION_DISABLED;
	UnmarkCollision( obstaclesMap );
}

void CDynamicPregeneratedObstacle::PostGraphGeneration( CObstaclesMap* obstaclesMap, CNavGraph* navgraph )
{
	if ( !m_shape )
	{
		return;
	}
	Bool isDisabled = (m_flags & IS_COLLISION_DISABLED) != 0;

	if ( isDisabled )
	{
		m_flags &= ~IS_COLLISION_DISABLED;
		MarkCollision( obstaclesMap );
	}
	
	Generate( navgraph );

	if ( isDisabled )
	{
		m_flags |= IS_COLLISION_DISABLED;
		UnmarkCollision( obstaclesMap );
	}
}

CDynamicPregeneratedObstacle* CDynamicPregeneratedObstacle::AsDynamicPregeneratedObstacle()
{
	return this;
}

////////////////////////////////////////////////////////////////////////////
// CDynamicImmediateObstacle
void CDynamicImmediateObstacle::OnAddition( CObstaclesMap* obstaclesMap )
{
	CAreaDescription* area = obstaclesMap->GetArea();
	if ( !m_isGenerated )
	{
		for ( Uint32 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
		{
			CNavGraph* navgraph = area->GetNavigationGraph( i );
			if ( navgraph )
			{
				Generate( navgraph );
			}
		}

		m_isGenerated = true;
	}

	CPathLibWorld& pathlib = area->GetPathLib();

	auto& processingContext = area->GetPathLib().GetObstaclesMapper()->GetComponentProcessingContext();
	processingContext.BeginRequestsCollection( pathlib );
	Show( area, processingContext );
	processingContext.EndRequestsCollection( pathlib );
}
void CDynamicImmediateObstacle::OnRemoval( CObstaclesMap* obstaclesMap )
{
	Super::OnRemoval( obstaclesMap );

	CAreaDescription* area = obstaclesMap->GetArea();

	if ( (m_flags & IS_COLLISION_DISABLED) == 0 )
	{
		CPathLibWorld& pathlib = area->GetPathLib();

		auto& processingContext = area->GetPathLib().GetObstaclesMapper()->GetComponentProcessingContext();
		processingContext.BeginRequestsCollection( pathlib );
		Hide( area, processingContext );
		processingContext.EndRequestsCollection( pathlib );
	}

	
	Clear( area );
	m_isGenerated = false;
}

};			// namespace PathLib