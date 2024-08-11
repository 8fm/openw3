/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "volumePathManager.h"

#include "../../common/core/loadingJobManager.h"

#include "../../common/redSystem/hash.h"

#include "../../common/engine/globalWater.h"
#include "../../common/physics/physicsWorldUtils.h"
#include "../../common/physics/physicsWorld.h"
#include "../../common/engine/renderFrame.h"

//#define VOLUMEPATH_DEBUG

IMPLEMENT_ENGINE_CLASS( CVolumePathManager );

#ifdef VOLUMEPATH_DEBUG
#pragma optimize("", off)
#endif
////////////////////////////////////////////////////////////////////////////////////////////
Uint32 CVolumePathManager::Coordinate::GetHash() const
{
	return Red::System::CalculateHash32( this, sizeof( Coordinate ) );
}

CVolumePathManager::Coordinate CVolumePathManager::Coordinate::GetNeighborCoordinate( Uint32 direction ) const
{
	// all this shit should be automatically optimized to address table and jump instruction
	switch( direction )
	{
	case Node::NEIGHBOR_X_POS:
		return Coordinate( x + 1, y, z );
	case Node::NEIGHBOR_X_NEG:
		return Coordinate( x - 1, y, z );
	case Node::NEIGHBOR_Y_POS:
		return Coordinate( x, y + 1, z );
	case Node::NEIGHBOR_Y_NEG:
		return Coordinate( x, y - 1, z );
	case Node::NEIGHBOR_Z_POS:
		return Coordinate( x, y, z + 1 );
	case Node::NEIGHBOR_Z_NEG:
		return Coordinate( x, y, z - 1 );
	default:
		ASSUME( false );
	}

	// should never happen
	ASSERT( false );
	return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////
// CNodeRecycleJob
////////////////////////////////////////////////////////////////////////////////////////////
CVolumePathManager::CNodeRecycleJob::CNodeRecycleJob( CVolumePathManager* volumeManager, int numNodesToRecycle, const Vector& referenceSpot ) 
	: ILoadJob( JP_Default )
	, m_volumeManager( volumeManager )
	, m_numNodesToRecycle( numNodesToRecycle )
	, m_referenceSpot( referenceSpot )
{
}

EJobResult CVolumePathManager::CNodeRecycleJob::Process()
{
	// create priority queue based on distance to player
	struct SNodeComperator
	{
		static RED_INLINE Bool Less( const CVolumePathManager::Node* a, const CVolumePathManager::Node* b )	{ return b->m_distToPlayerSq > a->m_distToPlayerSq; }
	};

	TPriQueue<CVolumePathManager::Node*, SNodeComperator> distanceQueue;
	
	// push nodes into queue
	for( THashMap<Uint32, Node*>::iterator it=m_volumeManager->m_usedNodes.Begin(); it != m_volumeManager->m_usedNodes.End(); ++it )
	{
		Node& node = *(*it).m_second;
		node.m_distToPlayerSq = ( m_referenceSpot - node.m_worldPosition ).SquareMag3();
		distanceQueue.Push( &node );
	}

	// pop out numNodes worth of nodes into list
	m_nodesToRecycle.Reserve( m_numNodesToRecycle );
	for( Uint32 i=0; i<m_numNodesToRecycle && !distanceQueue.Empty(); ++i )
	{
		m_nodesToRecycle.PushBack( distanceQueue.Top() );
		distanceQueue.Pop();
	}

	return JR_Finished;
}

////////////////////////////////////////////////////////////////////////////////////////////
// CVolumePathManager::IRequest
////////////////////////////////////////////////////////////////////////////////////////////
void CVolumePathManager::IRequest::GenerateResultPath( PathNode* endNode )
{
	// Generate path from start to end
	PathNode* pn = endNode;

	while ( pn != NULL && pn->m_node != m_startingNode )
	{
		m_result.PushBack( pn->m_node->m_worldPosition );
		pn = pn->m_parent;
	}
	// add starting node as a result
	m_result.PushBack( m_start );
	
}
Bool CVolumePathManager::IRequest::BeginSearch()
{
	return true;
}
Bool CVolumePathManager::CPreciseRequest::BeginSearch()
{
	m_endingNode = m_pathManager->RequestNodeAt( m_pathManager->CoordinateFromPosition( m_end ) );
	m_resultNode = nullptr;

	if ( !m_startingNode->m_available )
	{
		return false;
	}

	if ( m_startingNode == m_endingNode )
	{
		m_result.PushBack( m_end );
		m_result.PushBack( m_start );
		return false;
	}
	if ( !m_endingNode )
	{
		return false;
	}
	
	return true;
}

Bool CVolumePathManager::CPreciseRequest::EndSearch()
{
	if ( m_resultNode )
	{
		GenerateResultPath( m_resultNode );
		return true;
	}
	return false;
}
Bool CVolumePathManager::CPreciseRequest::ProcessNode( PathNode* node )
{
	if ( node->m_node == m_endingNode )
	{
		// we can get to target precisely
		m_result.PushBack( m_end );
		m_resultNode = node;
		return true;
	}

	return false;
}
Bool CVolumePathManager::CPreciseRequest::ProcessNeighbour( Node* neighbor )
{
	// neighbor must be available in order to search
	return neighbor->m_available || neighbor == m_endingNode;
}
Bool CVolumePathManager::CClosestAcceptablePointRequest::ProcessNode( PathNode* node )
{
	if ( node->m_node == m_endingNode )
	{
		m_acceptableDistance = 0.f;
		m_resultNode = node;
		// we can get to target precisely
		m_result.PushBack( m_end );
		return true;
	}

	if ( node->m_distToGoal < m_acceptableDistance )
	{
		m_acceptableDistance = node->m_distToGoal;
		m_resultNode = node;
	}

	return false;
}
Bool CVolumePathManager::CClosestAcceptablePointRequest::ProcessNeighbour( Node* neighbor )
{
	return neighbor->m_available;
}

Bool CVolumePathManager::CClosestAcceptablePointInBoundingsRequest::ProcessNeighbour( Node* neighbor )
{
	if ( neighbor->m_available )
	{
		if ( m_boundings.Contains( neighbor->m_worldPosition ) )
		{
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
// CVolumePathManager
////////////////////////////////////////////////////////////////////////////////////////////
CVolumePathManager::CVolumePathManager()
	: m_nodeSize( 3.0f )
	, m_recycleJob( NULL )
	, m_numNodes( 4000 )
	, m_nodes( NULL )
	, m_pathNodes( NULL )
	, m_enablePathfinding( true )
{}

CVolumePathManager::CVolumePathManager( Uint32 numNodes, Bool enablePathfinding )
	: m_nodeSize( 3.0f )
	, m_recycleJob( NULL )
	, m_numNodes( numNodes )
	, m_nodes( NULL )
	, m_pathNodes( NULL )
	, m_enablePathfinding( enablePathfinding )
{}

void CVolumePathManager::Initialize()
{
	m_nodes = (Node*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_VolumePathManager, m_numNodes * sizeof( Node ) );
	if( m_enablePathfinding )
	{
		m_pathNodes = (PathNode*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_VolumePathManager, m_numNodes * sizeof( PathNode ) );
	}
}

void CVolumePathManager::Shutdown()
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_VolumePathManager, m_nodes );

	if( m_pathNodes )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_VolumePathManager, m_pathNodes );
	}
}

CVolumePathManager::~CVolumePathManager()
{}
RED_FORCE_INLINE void CVolumePathManager::ResetPathNodes()
{
	m_nextPathNode = 0;
}
RED_FORCE_INLINE CVolumePathManager::PathNode* CVolumePathManager::AllocatePathNode( Node* node, Float distFromStart, Float distToGoal, PathNode* parentNode )
{
	if ( m_nextPathNode == m_numNodes )
	{
		return nullptr;
	}
	PathNode* ret = &m_pathNodes[ m_nextPathNode++ ];
	ret->m_node = node;
	ret->m_distFromStart = distFromStart;
	ret->m_distToGoal = distToGoal;
	ret->m_totalCost = distFromStart + distToGoal;
	ret->m_open = true;
	ret->m_closed = false;
	ret->m_parent = parentNode;
	return ret;
}

void CVolumePathManager::OnGameStart( const CGameInfo& gameInfo )
{
	Clear();
}

void CVolumePathManager::OnGameEnd( const CGameInfo& gameInfo )
{
	Clear();
}

void CVolumePathManager::Tick( Float timeDelta )
{
	PC_SCOPE_PIX( VolumePathManager_Tick );

	// are we recycling?
	if( m_recycleJob != NULL && m_recycleJob->HasEnded() )
	{
		if( m_recycleJob->HasFinishedWithoutErrors() )
		{
			// perform re-cycling!
			const TDynArray<Node*>& nodesToRecycle = m_recycleJob->GetResult();
			for( Uint32 i=0; i<nodesToRecycle.Size(); ++i )
			{
				Node* nodeToRecycle = nodesToRecycle[ i ];
				m_usedNodes.Erase( nodeToRecycle->m_position.GetHash() );
				m_availableNodes.PushBack( nodeToRecycle );
			}
		}
		else
		{
			// shouldn't happen
			ASSERT( false );
		}

		// clean up
		delete m_recycleJob;
		m_recycleJob = NULL;
	}
	else if( m_recycleJob == NULL && m_availableNodes.Size() < m_numNodes * 0.25f )
	{
		CWorld* world = GGame->GetActiveWorld();
		Vector referencePosition =
			world
			? world->GetCameraPosition()
			: Vector::ZEROS;
		// spawn new recycle job
		m_recycleJob = new CNodeRecycleJob( this, (Uint32)( m_numNodes * 0.25f ), referencePosition  );
		SJobManager::GetInstance().Issue( m_recycleJob );
	}

#ifdef VOLUMEPATH_DEBUG 
	static float UPDATE_TIME = 0.0f;

	// create a new random path every X secs
	UPDATE_TIME += timeDelta;
	if( UPDATE_TIME > 5.0f )
	{
		UPDATE_TIME = 0.0f;

#if 0
		// TEST: find repulsion vectors around the player
		Vector3 vPos = GGame->GetPlayerEntity()->GetWorldPosition();
		
		for( float z=vPos.Z; z<=vPos.Z + m_nodeSize * 4; z += m_nodeSize )
		{
			for( float y=vPos.Y - m_nodeSize * 3; y<=vPos.Y + m_nodeSize * 3; y += m_nodeSize )
			{
				for( float x=vPos.X - m_nodeSize * 3; x<=vPos.X + m_nodeSize * 3; x += m_nodeSize )
				{
					Vector3 repulse;
					Uint32 availableNeighbors;
					GetCollisionRepulsion( Vector3( x, y, z ), repulse, availableNeighbors );
				}
			}
		}
#endif

#if 0
		// TEST: find some random paths
		Bool gotPath = false;
		do 
		{
			Clear();

			// pick random start & end
			Vector3 vPos = GGame->GetPlayerEntity()->GetWorldPosition();
			Vector3 vMin = Vector3( -20.0f, -20.0f, 2.0f );
			Vector3 vMax = Vector3( 20.0f, 20.0f, 15.0f );
			Vector3 start = vPos + GRandomVector( vMin, vMax );
			Vector3 end = vPos + GRandomVector( vMin, vMax );

			// get a random path and simplify it
			gotPath = GetPath( start, end, m_debugPath );
			gotPath = gotPath && SimpifyPath( m_debugPath, m_debugSimplePath );				
		} 
		while ( !gotPath );
#endif
	}
#endif
}

void CVolumePathManager::OnGenerateDebugFragments( CRenderFrame* frame )
{
	if ( !( frame->GetFrameInfo().IsShowFlagOn( SHOW_VolumetricPaths ) ) )
		return;

	// draw used nodes
	for( THashMap<Uint32, Node*>::iterator it=m_usedNodes.Begin(); it != m_usedNodes.End(); ++it )
	{
		Node& node = *(*it).m_second;
		Box drawBox( PositionFromCoordinate( node.m_position ), ( node.m_available ? 0.05f : m_nodeSize * 0.45f ) );
		frame->AddDebugBox( drawBox, Matrix::IDENTITY, node.m_available ? Color::GREEN : Color::RED );

		// repulsion vector
		if( node.m_hasRepultionVector && !node.m_repulsionVector.IsAlmostZero( 0.01f ) )
		{
			Matrix matrix = Matrix::IDENTITY;
			matrix.SetTranslation( node.m_worldPosition.X, node.m_worldPosition.Y, node.m_worldPosition.Z );
			frame->AddDebugArrow( matrix, node.m_repulsionVector, 1.5f, Color( 255, 128, 0 ) );
		}
	}

	// debug path
	for( Uint32 i=1; i<m_debugPath.Size(); ++i )
	{
		frame->AddDebugLine( m_debugPath[ i - 1 ], m_debugPath[ i ], Color::YELLOW, true );
	}

	// debug simple path
	for( Uint32 i=1; i<m_debugSimplePath.Size(); ++i )
	{
		frame->AddDebugFatLine( m_debugSimplePath[ i - 1 ], m_debugSimplePath[ i ], Color( 255, 128, 0 ), 0.1f, true );
	}
}

CVolumePathManager::Coordinate CVolumePathManager::CoordinateFromPosition( const Vector3& position ) const
{
	Int32 x = (Int32)( ( position.X - m_nodeSize * 0.5f ) / m_nodeSize );
	Int32 y = (Int32)( ( position.Y - m_nodeSize * 0.5f ) / m_nodeSize );
	Int32 z = (Int32)( ( position.Z - m_nodeSize * 0.5f ) / m_nodeSize );
	return Coordinate( x, y, z );
}

Vector3 CVolumePathManager::PositionFromCoordinate( const Coordinate& coordinate ) const
{
	return Vector3( coordinate.x * m_nodeSize + m_nodeSize * 0.5f, 
					coordinate.y * m_nodeSize + m_nodeSize * 0.5f, 
					coordinate.z * m_nodeSize + m_nodeSize * 0.5f);
}


Bool CVolumePathManager::PlotPath( IRequest& request )
{
	TDynArray<Vector3>& result = request.m_result;
	result.ClearFast();

	const Vector3& start = request.m_start;
	const Vector3& end = request.m_end;

	// find start & end points
	Node* startNode = RequestNodeAt( CoordinateFromPosition( start ) );

	if ( !startNode )
	{
		return false;
	}

	request.m_pathManager = this;
	request.m_startingNode = startNode;

	if ( !request.BeginSearch() )
	{
		return true;
	}


	Int32 waterLevel = NumericLimits< Int32 >::Min();
	IRequest::EWaterSetup waterSetup = request.GetWaterSetup();
	if ( waterSetup != IRequest::WATER_ABOVE_AND_BELOW )
	{
		CGlobalWater* globalWater = GGame->GetActiveWorld()->GetGlobalWater();
		if ( globalWater )
		{
			Float waterZ = globalWater->GetWaterLevelBasic( start.X, start.Y );
			waterLevel = (Int32)( ( waterZ - m_nodeSize * 0.5f ) / m_nodeSize );
		}
		else
		{
			waterSetup = IRequest::WATER_ABOVE_AND_BELOW;
		}
	}
	
	// clear path finding data
	ResetPathNodes();

	// clear open queue (priority queue of nodes to search next)
	m_openQueue.ClearFast();

	// init our starting point
	Uint32 nextPathNodeIndex = 0;
	m_pathNodeLookupMap.ClearFast();

	PathNode* startPathNode = AllocatePathNode( startNode, 0.f, ( end - startNode->m_worldPosition ).Mag() );

	m_pathNodeLookupMap.Insert( startNode, startPathNode );
	m_openQueue.Push( startPathNode );

	// search as long as a path hasn't been found
	while( !m_openQueue.Empty() )			
	{
		// search next node
		PathNode* nextPathNode = m_openQueue.Top();
		m_openQueue.Pop();
		nextPathNode->m_open = false;

		// found goal (i.e. end)?
		if ( request.ProcessNode( nextPathNode ) )
		{
			break;
		}

		Coordinate prevNodeCoordinate = nextPathNode->m_parent ? nextPathNode->m_parent->m_node->m_position : Coordinate( NumericLimits< Int32 >::Max(), NumericLimits< Int32 >::Max(), NumericLimits< Int32 >::Max() );

		// otherwise, check the links of the best tile
		for( Uint32 i=0; i<Node::NEIGHBOR_COUNT; ++i )
		{
			const Coordinate& coord = nextPathNode->m_node->m_position;
			// get neighbor
			Coordinate neighborCoordinate = coord.GetNeighborCoordinate( i );
			if ( neighborCoordinate == prevNodeCoordinate )
			{
				continue;
			}

			if ( i != Node::NEIGHBOR_Z_POS && waterSetup == IRequest::WATER_ABOVE )
			{
				if ( neighborCoordinate.z < waterLevel )
				{
					continue;
				}
			}
			else if ( i != Node::NEIGHBOR_Z_NEG && waterSetup == IRequest::WATER_BELOW )
			{
				if ( neighborCoordinate.z > waterLevel )
				{
					continue;
				}
			}


			Node* neighbor = RequestNodeAt( neighborCoordinate );
			if( neighbor == nullptr )
			{
				// if we couldn't get a neighbor, 
				// it means we ran out of available nodes. Lets just process all remaining nodes
				continue;
			}

			if ( !request.ProcessNeighbour( neighbor ) )
			{
				continue;
			}

			// calculate new F & G
			Float newDistFromStart = nextPathNode->m_distFromStart + m_nodeSize;

			// does neighbor path node exist?
			PathNode** neighborPathNodePtr = m_pathNodeLookupMap.FindPtr( neighbor );
			if( !neighborPathNodePtr )
			{
				Float distToGoal = ( end - neighbor->m_worldPosition ).Mag();

				// create new path node for this neighbor
				PathNode* neighborPathNode  = AllocatePathNode( neighbor, newDistFromStart, distToGoal, nextPathNode );

				// out of path nodes
				if ( !neighborPathNode )
				{
					continue;
				}

				// add new path node to queue
				m_pathNodeLookupMap.Insert( neighbor, neighborPathNode );

				m_openQueue.Push( neighborPathNode );
			}
			else
			{
				PathNode* neighborPathNode = *neighborPathNodePtr;
				if( newDistFromStart < neighborPathNode->m_distFromStart )
				{
					// better parent than before?
					neighborPathNode->m_distFromStart = newDistFromStart;
					neighborPathNode->m_totalCost = newDistFromStart + neighborPathNode->m_distToGoal;
					neighborPathNode->m_parent = nextPathNode;								
				}
			}
		}

		// the best tile has now been searched, add the closed flag
		nextPathNode->m_closed = true;
	}

	return request.EndSearch();
}

Bool CVolumePathManager::GetPath( const Vector3& start, const Vector3& end, TDynArray<Vector3>& result, Float maxHeight )
{
	// clear old path (if any)
	result.ClearFast();

	// check if we need volume pathfinding
	if( m_pathNodes == NULL || !IsPathfindingNeeded( start, end )  )
	{
		// ... if not, then return straight path
		result.PushBack( start );
		result.PushBack( end );
		return true;
	}

	// find start & end points
	Node* startNode = RequestNodeAt( CoordinateFromPosition( start ) );
	Node* endNode = RequestNodeAt( CoordinateFromPosition( end ) );

	// no existing path?
	if( startNode == NULL || endNode == NULL )
		return false;

	// already there?
	if( startNode == endNode )
		return true;

	// clear path finding data
	ResetPathNodes();

	// clear open queue (priority queue of nodes to search next)
	m_openQueue.ClearFast();

	// init our starting point ( which is the end position )
	// optimization to skip reversing path at the end
	m_pathNodeLookupMap.ClearFast();

	PathNode* endPathNode = AllocatePathNode( endNode, 0.f, ( endNode->m_worldPosition - startNode->m_worldPosition ).Mag() );

	m_pathNodeLookupMap.Insert( endNode, endPathNode );
	m_openQueue.Push( endPathNode );

	// search as long as a path hasn't been found
	while( !m_openQueue.Empty() )			
	{	
		// search next node
		PathNode* nextPathNode = m_openQueue.Top();
		m_openQueue.Pop();
		nextPathNode->m_open = false;

		// found goal (i.e. start)?
		if( nextPathNode->m_node == startNode )
		{
			// Generate path from start to end
			PathNode* pn = nextPathNode;
			result.PushBack( start );

			while( pn != NULL && pn->m_node != endNode )
			{
				if( pn != nextPathNode )
					result.PushBack( pn->m_node->m_worldPosition );

				pn = pn->m_parent;
			}
			
			result.PushBack( endNode->m_worldPosition );
			result.PushBack( end );
			return true;
		}

		Node* prevNode = nextPathNode->m_parent ? nextPathNode->m_parent->m_node : nullptr;
		
		// otherwise, check the links of the best tile
		for( Uint32 i=0; i<Node::NEIGHBOR_COUNT; ++i )
		{
			// get neighbor
			Node* neighbor = RequestNodeAt( nextPathNode->m_node->m_position.GetNeighborCoordinate( i ) );
			if( neighbor == NULL )
			{
				// if we couldn't get a neighbor, 
				// it means we ran out of available nodes.
				return false;
			}

			// if the neighbor is the target, always allow pathfinding to it
			if( neighbor != startNode )
			{
				// neighbor must be available in order to search
				if( !neighbor->m_available )
				{
					continue;
				}

				if ( neighbor == prevNode )
				{
					continue;
				}

				// is neighbor node above the max height?
				// (used for water pathfinding etc.)
				if( neighbor->m_worldPosition.Z > maxHeight )
				{
					continue;
				}
			}

			Float newDistFromStart = nextPathNode->m_distFromStart + m_nodeSize;

			// does neighbor path node exist?
			PathNode** neighborPathNodePtr = m_pathNodeLookupMap.FindPtr( neighbor );
			if( !neighborPathNodePtr )
			{
				// calculate new F & G
				Float newDistToGoal = newDistFromStart + ( startNode->m_worldPosition - neighbor->m_worldPosition ).Mag();
				// create new path node for this neighbor
				PathNode* neighborPathNode = AllocatePathNode( neighbor, newDistFromStart, newDistToGoal, nextPathNode );
				if ( !neighborPathNode )
				{
					continue;
				}

				m_pathNodeLookupMap.Insert( neighbor, neighborPathNode );
				m_openQueue.Push( neighborPathNode );
			}
			else
			{
				PathNode* neighborPathNode = *neighborPathNodePtr;
				if( newDistFromStart < neighborPathNode->m_distFromStart )
				{
					neighborPathNode->m_distFromStart = newDistFromStart;
					neighborPathNode->m_totalCost = newDistFromStart + neighborPathNode->m_distToGoal;
					neighborPathNode->m_parent = nextPathNode;								
				}
			}
		}

		// the best tile has now been searched, add the closed flag
		nextPathNode->m_closed = true;
	}

	return false;
}

Bool CVolumePathManager::GetCollisionRepulsion( const Vector3& position, Vector3& repulsionVector, Uint32& availableNeighbors )
{
	// get node at coordinate
	Coordinate c = CoordinateFromPosition( position );
	Node* node = RequestNodeAt( c );
	if( !node )
	{
		// no node? then no repulsion!
		return false;
	}

	// do the coordinate already have a cached repulsion vector?
	if( !node->m_hasRepultionVector )
	{
		// ... if not, calculate repulsion vector by checking all surrounding nodes
		node->m_repulsionVector = Vector3( 0.0f, 0.0f, 0.0f );
		node->m_availableNeighbors = 0;

		for( Int32 z=c.z - 1; z<=c.z + 1; ++z )
		{
			for( Int32 y=c.y - 1; y<=c.y + 1; ++y )
			{
				for( Int32 x=c.x - 1; x<=c.x + 1; ++x )
				{
					if( c.z == z && c.y == y && c.x == x )
					{
						continue;
					}

					// get neighbor
					Coordinate neighborCoordinate( x, y, z );
					Node* neighbor = RequestNodeAt( neighborCoordinate );
					if( !neighbor )
					{
						// no neighbor? no repulsion!
						return false;
					}

					// is neighbor available?
					if( neighbor->m_available )
					{
						// store neighbor availability in bit field
						Uint32 flag = CVolumePathManager::GetDirectionFlag( neighborCoordinate - c );
						node->m_availableNeighbors |= flag;
						continue;
					}

					// ... otherwise add some repulsion
					node->m_repulsionVector += ( node->m_worldPosition - neighbor->m_worldPosition ).Normalized();
				}
			}
		}

		if ( node->m_available )
		{
			node->m_availableNeighbors |= NODE_AVAILABLE;
		}

		// normalize final repulsion
		node->m_hasRepultionVector = true;
		if( !node->m_repulsionVector.IsAlmostZero( 0.01f ) )
		{
			node->m_repulsionVector.Normalize();
		}
	}

	
	// return stuffs
	repulsionVector = node->m_repulsionVector;
	availableNeighbors = node->m_availableNeighbors;
	return true;
}

Bool CVolumePathManager::IsNodeBlocked( const Vector3& position )
{
	Node* node = RequestNodeAt( CoordinateFromPosition( position ) );
	if( node )
	{
		return !node->m_available;
	}

	// should never happen (if the node garbage collector has done it's job)
	ASSERT( false );
	return false;
}

Uint32 CVolumePathManager::GetDirectionFlag( const Coordinate& c )
{
	return GetDirectionFlag( c.x, c.y, c.z );
}

Uint32 CVolumePathManager::GetDirectionFlag( Int32 x, Int32 y, Int32 z )
{
	Uint32 _x = ( x < 0 ? 0 : x == 0 ? 1 : 2 );
	Uint32 _y = ( y < 0 ? 0 : y == 0 ? 1 : 2 );
	Uint32 _z = ( z < 0 ? 0 : z == 0 ? 1 : 2 );
	return FLAG( _x + _y * 3 + _z * 9 );
}

void CVolumePathManager::Clear()
{	
	// clear lists
	m_debugPath.Clear();
	m_debugSimplePath.Clear();
	m_availableNodes.Clear();
	m_usedNodes.Clear();

	// clear all nodes ...
	Red::MemorySet( m_nodes, 0, sizeof( Node ) * m_numNodes );

	// ...and make them available and assign ID
	for(Uint32 i=0; i<m_numNodes; ++i)
	{
		m_availableNodes.PushBack( &m_nodes[ i ] );
	}
}

CVolumePathManager::Node* CVolumePathManager::RequestNodeAt( const CVolumePathManager::Coordinate& coordinate )
{
	// check if we have an already used node for this coordinate?
	Uint32 nodeHash = coordinate.GetHash();
	if( m_usedNodes.KeyExist( nodeHash ) )
	{
		return *m_usedNodes.FindPtr( nodeHash );
	}

	// make sure we have an available node
	if( m_availableNodes.Empty() )
	{
		return nullptr;
	}

	// otherwise we create it from the available nodes and add it to the search space
	Node& newNode = *m_availableNodes[ m_availableNodes.Size() - 1 ];
	m_availableNodes.RemoveAt( m_availableNodes.Size() - 1 );

	// set node data
	newNode.m_position = coordinate;
	newNode.m_available = true;
	newNode.m_worldPosition = PositionFromCoordinate( coordinate );
	newNode.m_hasRepultionVector = false;
	newNode.m_repulsionVector = Vector3( 0.0f, 0.0f, 0.0f );
	newNode.m_availableNeighbors = 0;

	// physics world box overlay test
	CPhysicsWorld* physicsWorld = nullptr;
	if ( GGame->GetActiveWorld()->GetPhysicsWorld( physicsWorld ) )
	{
		static SPhysicsOverlapInfo contactInfo;
		static const CPhysicsEngine::CollisionMask includeMask =
			GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) |
			GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) |
			GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) |
			GPhysicEngine->GetCollisionTypeBit( CNAME( Foliage ) );

		static Vector3 halfExtent( m_nodeSize * 0.5f, m_nodeSize * 0.5f, m_nodeSize * 0.5f );

		if( physicsWorld->BoxOverlapWithAnyResult( newNode.m_worldPosition, halfExtent, Matrix::IDENTITY, includeMask, 0, contactInfo ) == TRV_Hit )
		{
			// can't fly here
			newNode.m_available = false;
		}
	}

	// add new node to used nodes map
	m_usedNodes.Insert( nodeHash, &newNode );

	return &newNode;
}

Bool CVolumePathManager::IsPathfindingNeeded( const Vector3& start, const Vector3& end ) const
{
	CPhysicsWorld* physicsWorld = nullptr;
	if ( !GGame->GetActiveWorld()->GetPhysicsWorld( physicsWorld ) )
		return false;

	static SPhysicsContactInfo contactInfo;
	static const CPhysicsEngine::CollisionMask includeMask =
		GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) |
		GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) |
		GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) |
		GPhysicEngine->GetCollisionTypeBit( CNAME( Foliage ) );

	// do a sweep test to see if target is obstructed
	return physicsWorld->SweepTestWithSingleResult( start, end, 0.5f, includeMask, 0, contactInfo ) == TRV_Hit;
}

Bool CVolumePathManager::SimpifyPath( TDynArray<Vector3>& path, TDynArray<Vector3>& simplePath ) const
{
	simplePath.ClearFast();

	// get physics world
	CPhysicsWorld* physicsWorld = nullptr;
	if ( !GGame->GetActiveWorld()->GetPhysicsWorld( physicsWorld ) )
		return false;

	// got a valid path?
	if( path.Size() < 2 )
		return false;

	static SPhysicsContactInfo contactInfo;
	static const CPhysicsEngine::CollisionMask includeMask =
		GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) |
		GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) |
		GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) |
		GPhysicEngine->GetCollisionTypeBit( CNAME( Foliage ) );

	Uint32 startIndex = 0;
	Uint32 endIndex = 2;

	// clear simple path, and add start point
	simplePath.PushBack( path[ 0 ] );

	while( endIndex < path.Size() )
	{	
		// ray cast from start to end point
		if( physicsWorld->RayCastWithSingleResult( path[ startIndex ], path[ endIndex ], includeMask, 0, contactInfo ) == TRV_Hit )
		{
			// make sure we're not spinning around the same start & end pair
			if( startIndex == endIndex - 1 )
			{
				// should in theory not happen...
				simplePath.PushBack( path[ endIndex ] );
				startIndex = endIndex;
				++endIndex;
			}
			else
			{
				// add last known "good" end point and move start point
				simplePath.PushBack( path[ endIndex - 1 ] );
				startIndex = endIndex - 1;
			}
		}
		else
		{
			// move end point
			++endIndex;
		}
	}

	// and end point
	simplePath.PushBack( path[ path.Size() - 1 ] );

	// ta-da
	return true;
}

void CVolumePathManager::funcGetPath( CScriptStackFrame& stack, void* result )
{
	PC_SCOPE( VolumeManagerGetPath );

	GET_PARAMETER( Vector, start, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Vector, end, Vector::ZERO_3D_POINT );
	GET_PARAMETER_REF( TDynArray< Vector >, resultPath, TDynArray< Vector >() );
	GET_PARAMETER_OPT( Float, maxHeight, FLT_MAX );
	FINISH_PARAMETERS;

	// get path and simplify it
	Bool pathOK = GetPath( start, end, m_debugPath, maxHeight ) && SimpifyPath( m_debugPath, m_debugSimplePath );
	for( Uint32 i=0; i<m_debugSimplePath.Size(); ++i )
	{
		resultPath.PushBack( m_debugSimplePath[ i ] );
	}

	RETURN_BOOL( pathOK );
}

void CVolumePathManager::funcGetPointAlongPath( CScriptStackFrame& stack, void* result )
{
	PC_SCOPE( VolumeManagerGetPath );

	GET_PARAMETER( Vector, start, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Vector, end, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Float, distAlongPath, m_nodeSize );
	GET_PARAMETER_OPT( Float, maxHeight, FLT_MAX );
	FINISH_PARAMETERS;

	// get path & simplify it
	Vector dest = end;
	Bool pathOK = GetPath( start, end, m_debugPath, maxHeight ) && SimpifyPath( m_debugPath, m_debugSimplePath );

	if( pathOK )
	{
		// find point along path with distance along path
		for( Uint32 i=1; i<m_debugSimplePath.Size(); ++i )
		{
			Vector3& startPoint = m_debugSimplePath[ i - 1 ];
			Vector3& endPoint = m_debugSimplePath[ i ];
			Float segmentLength = ( endPoint - startPoint ).Mag();

			if( segmentLength > distAlongPath )
			{
				// calculate dest point
				dest = startPoint + ( endPoint - startPoint ).Normalized() * distAlongPath;
				break;
			}
			else
			{
				// move to next segment
				distAlongPath -= segmentLength;
			}
		}
	}

	RETURN_STRUCT( Vector, dest );
}

void CVolumePathManager::funcIsPathfindingNeeded( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, start, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Vector, end, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
	RETURN_BOOL( IsPathfindingNeeded( start, end ) );
}



#ifdef VOLUMEPATH_DEBUG
#pragma optimize("", on)
#endif
