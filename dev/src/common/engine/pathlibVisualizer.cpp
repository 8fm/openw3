#include "build.h"
#include "pathlibVisualizer.h"

#include "../core/engineTime.h"
#include "../core/heap.h"

#include "baseEngine.h"
#include "clipMap.h"
#include "game.h"
#include "pathlibNavgraphHelper.inl"
#include "pathlibWorld.h"
#include "pathlibAreaDescription.h"
#include "pathlibTerrain.h"
#include "pathlibNavgraph.h"
#include "terrainTile.h"
#include "pathlibObstacleShape.h"
#include "pathlibObstaclesMap.h"
#include "pathlibNavmesh.h"
#include "pathlibNavmeshArea.h"
#include "renderer.h"
#include "renderFragment.h"
#include "renderFrame.h"
#include "renderVertices.h"
#include "world.h"


namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// CNavmeshRenderer
////////////////////////////////////////////////////////////////////////////

CNavmeshRenderer::CNavmeshRenderer( PathLib::AreaId id )
	: m_id( id )
	, m_isPrepeared( false )
	, m_navmeshPreview( NULL )
	, m_transformation( Matrix::IDENTITY )
{

}
CNavmeshRenderer::~CNavmeshRenderer()
{
	InvalidatePreview();
}

CNavmesh* CNavmeshRenderer::GetNavmesh() const
{
	CWorld* world = GGame->GetActiveWorld();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : nullptr;
	if ( !pathlib )
	{
		return nullptr;
	}
	PathLib::CNavmeshAreaDescription* area = pathlib->GetInstanceAreaDescription( m_id );
	if ( !area )
	{
		return nullptr;
	}

	return area->GetNavmesh();
}

Bool CNavmeshRenderer::PrepeareRender()
{
	static const Int32 MAX_COLORS = 19;
	static const Color COLORS[ MAX_COLORS ] =
	{
		Color( 0, 0, 255 ),
		Color( 0, 64, 190 ),
		Color( 64, 0, 190 ),
		Color( 64, 64, 190 ),
		Color( 128, 0, 128 ),
		Color( 128, 128, 0 ),
		Color( 190, 128, 0 ),
		Color( 190, 128, 90 ),
		Color( 0, 128, 90 ),
		Color( 64, 128, 160 ),
		Color( 160, 128, 160 ),
		Color( 180, 128, 0 ),
		Color( 15, 128, 192 ),
		Color( 15, 192, 192 ),
		Color( 15, 190, 46 ),
		Color( 90, 160, 15 ),
		Color( 120, 180, 0 ),
		Color( 120, 180, 90 ),
		Color( 54, 216, 135 )
	};

	if ( !m_isPrepeared )
	{
		// sanity check
		CNavmesh* navi = GetNavmesh();
		if ( !navi )
		{
			return false;
		}

		// Get pathlib world and area if possible
		CPathLibWorld* pathlib = GGame->GetActiveWorld()->GetPathLibWorld();
		CNavmeshAreaDescription* area = pathlib ? pathlib->GetInstanceAreaDescription( m_id ) : NULL;

		// determine navmesh colors
		Color TRIANGLE_COLOR = (m_id == INVALID_AREA_ID) ? Color( 0, 0, 255 ) : COLORS[ m_id % MAX_COLORS ];
#ifndef NO_NAVMESH_GENERATION
		if ( navi->HadProblems() )
		{
			TRIANGLE_COLOR = Color::RED;
		}
#endif

		const Vector3 VERTS_SHIFT( 0.f, 0.f, 0.1f );

		TRIANGLE_COLOR.A = 160;
		Color WIREFRAME_COLOR(
			TRIANGLE_COLOR.R + (255-TRIANGLE_COLOR.R)/2,
			TRIANGLE_COLOR.G + (255-TRIANGLE_COLOR.G)/2,
			TRIANGLE_COLOR.B + (255-TRIANGLE_COLOR.B)/2,
			200
			);


		CNavmesh::TriangleIndex trianglesCount = navi->GetTrianglesCount();
		CNavmesh::VertexIndex vertexCount = navi->GetVertexesCount();
		if ( trianglesCount )
		{
			m_borders.ClearFast();
			//m_bordersColors.ClearFast();

			// Create data
			TDynArray< DebugVertex > vertices( vertexCount );
			TDynArray< Uint32 > indices;
			indices.Reserve( trianglesCount * 6 );
			indices.ResizeFast( trianglesCount * 3 );

			Uint32* writeIndex = indices.TypedData();

			// Indices + phantom edges
			for ( CNavmesh::TriangleIndex i = 0; i < trianglesCount; ++i )
			{
				CNavmesh::VertexIndex vertsIndexes[3];
				navi->GetTriangleVertsIndex( i, vertsIndexes );

				PATHLIB_ASSERT( vertsIndexes[2] < vertexCount && vertsIndexes[ 1 ] < vertexCount && vertsIndexes[ 0 ] < vertexCount );

				*writeIndex++ = vertsIndexes[2];
				*writeIndex++ = vertsIndexes[1];
				*writeIndex++ = vertsIndexes[0];

				// phantom edges search
				CNavmesh::TriangleIndex neighbours[3];
				navi->GetTriangleNeighbours( i, neighbours );
				for ( Uint32 j = 0; j < 3; ++j )
				{
					if ( CNavmesh::IsEdge( neighbours[ j ] ) )
					{
						Vector3 verts[2];
						navi->GetTriangleEdge( i, j, verts );
						Color edgeColor;
						if ( CNavmesh::IsPhantomEdge( neighbours[ j ] ) )
						{
							edgeColor = Color::GREEN;
							if ( area )
							{
								CNavmesh::TriangleIndex neighbourId = CNavmesh::PhantomEdgeNeighbourIndex( neighbours[ j ] );
								AreaId neighbourAreaId = area->GetNeighbourAreaId( neighbourId );
								CAreaDescription* neighbourArea = pathlib->GetAreaDescription( neighbourAreaId );
								if ( !neighbourArea )
								{
									edgeColor = Color( 255, 127, 39 );
								}
							}
						}
						else
						{
							edgeColor = Color::BLACK;
						}
						Uint32 baseIdx = m_borders.Size();
						m_borders.Grow( 2 );
						m_borders[ baseIdx+0 ] = DebugVertex( Vector( verts[ 0 ] + VERTS_SHIFT ), edgeColor );
						m_borders[ baseIdx+1 ] = DebugVertex( Vector( verts[ 1 ] + VERTS_SHIFT ), edgeColor );
					}
				}
			}

			PATHLIB_ASSERT( writeIndex == &(*indices.End()) );
			// Vertexes
			for ( CNavmesh::VertexIndex i = 0; i < vertexCount; ++i )
			{
				vertices[ i ].Set( Vector( navi->GetVertex( i ) + VERTS_SHIFT ), TRIANGLE_COLOR );
				PATHLIB_ASSERT( vertices[ i ].x >= navi->GetBoundingBox().Min.X && vertices[ i ].x <= navi->GetBoundingBox().Max.X
					&& vertices[ i ].y >= navi->GetBoundingBox().Min.Y && vertices[ i ].y <= navi->GetBoundingBox().Max.Y );
			}

			// Generate rendering mesh
			if ( !m_navmeshPreview )
			{
				m_navmeshPreview = GRender->UploadDebugMesh( vertices, indices );
				//m_navmeshPreview = new CTmpShitRenderResource();
				//m_navmeshPreview->m_m = m_transformation;
				//m_navmeshPreview->m_indices = indices;
				//m_navmeshPreview->m_vertices = vertices;
				//m_navmeshPreview->m_color = TRIANGLE_COLOR;
			}
			if ( !m_navmeshPreviewWireframe )
			{
				// rebuild data
				indices.ResizeFast( trianglesCount * 6 );
				Uint32 indicesIndex = 0;
				Uint32* writeIndex = indices.TypedData();
				for ( CNavmesh::VertexIndex i = 0; i < vertexCount; ++i )
				{
					vertices[ i ].color = WIREFRAME_COLOR.ToUint32();
				}
				for ( CNavmesh::TriangleIndex i = 0; i < trianglesCount; ++i )
				{
					CNavmesh::VertexIndex vertsIndexes[3];
					navi->GetTriangleVertsIndex( i, vertsIndexes );
					indices[ indicesIndex++ ] = vertsIndexes[0];
					indices[ indicesIndex++ ] = vertsIndexes[1];
					indices[ indicesIndex++ ] = vertsIndexes[1];
					indices[ indicesIndex++ ] = vertsIndexes[2];
					indices[ indicesIndex++ ] = vertsIndexes[2];
					indices[ indicesIndex++ ] = vertsIndexes[0];

					PATHLIB_ASSERT( vertsIndexes[2] < vertexCount && vertsIndexes[ 1 ] < vertexCount && vertsIndexes[ 0 ] < vertexCount );
				}
				PATHLIB_ASSERT( indicesIndex == indices.Size() );
				m_navmeshPreviewWireframe = GRender->UploadDebugMesh( vertices, indices );
			}

			m_isPrepeared = true;
		}
	}
	return true;
}
void CNavmeshRenderer::GenerateEditorFragments( CRenderFrame* frame )
{
	PrepeareRender();

	// USEFULL: navi index debug code
	//String text;
	//for ( CNavmesh::TriangleIndex i = 0; i < m_navmesh->GetTrianglesCount(); ++i )
	//{
	//	Vector3 v[2];
	//	m_navmesh->GetTriangleEdge( i, 0, v );
	//	Char buff[16];
	//	Red::System::SNPrintF( buff, ARRAY_COUNT( buff ), TXT("%d"), i );
	//	frame->AddDebugText( Vector((v[0] + v[1])/2.f), String::Printf( TXT("%d"), i ), 0, 0 );
	//}

	//String text;
	//CNavmesh* navi = m_navmesh.Get();
	//if ( navi )
	//{
	//	for ( CNavmesh::VertexIndex i = 0; i < navi->GetVertexesCount(); ++i )
	//	{
	//		const Vector3& v = navi->GetVertex( i );
	//		Char buff[16];
	//		Red::System::SNPrintF( buff, ARRAY_COUNT( buff ), TXT("%d"), i );
	//		frame->AddDebugText( v, String::Printf( TXT("%d"), i ), 0, 0 );
	//	}
	//}
	

	// Generate rendering fragment
	Bool overlay = false;
	if ( m_navmeshPreview )
	{
		if ( !frame->GetFrameInfo().IsShowFlagOn( SHOW_NavMeshOverlay ) )
		{
			new ( frame ) CRenderFragmentDebugMesh( frame, m_transformation, m_navmeshPreview, true );

		}
		else
		{
			new ( frame ) CRenderFragmentDebugMesh( frame, m_transformation, m_navmeshPreview, RSG_DebugOverlay, true );
			overlay = true;
		}
		
	}

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_NavMeshTriangles ) )
	{
		if ( m_navmeshPreviewWireframe )
		{
			new ( frame ) CRenderFragmentDebugWireSingleColorMesh( frame, m_transformation, m_navmeshPreviewWireframe );
		}
		frame->AddDebugLines( m_borders.TypedData(), m_borders.Size(), overlay );
	}
	else
	{
		frame->AddDebugFatLines( m_borders.TypedData(), m_borders.Size(), 0.1f, overlay, false );
	}

	
#ifdef DEBUG_NAVMESH_COLORS
	CNavmesh* navi = m_navmesh.Get();
	if ( navi && !navi->m_triangleColours.Empty() )
	{
		const auto& colours = navi->m_triangleColours;
		for ( Uint32 i = 0, n = Min( colours.Size(), Uint32(navi->GetTrianglesCount()) ); i < n; ++i )
		{
			Vector3 v[3];
			navi->GetTriangleVerts( CNavmesh::TriangleIndex(i), v );
			Vector pos;
			pos.AsVector3() = (v[ 0 ] + v[ 1 ] + v[ 2 ]) * 0.333333333333f;
			String str = String::Printf( TXT("%d"), colours[ i ] );
			frame->AddDebugText( pos, str );
		}
	}
#endif

}

void CNavmeshRenderer::InvalidatePreview()
{
	if ( m_navmeshPreview )
	{
		m_navmeshPreview->Release();
		m_navmeshPreview = NULL;
	}
	if ( m_navmeshPreviewWireframe )
	{
		m_navmeshPreviewWireframe->Release();
		m_navmeshPreviewWireframe = NULL;
	}
	m_borders.ClearFast();
	//m_bordersColors.ClearFast();

	m_isPrepeared = false;

}

////////////////////////////////////////////////////////////////////////////
// CVisualizer
////////////////////////////////////////////////////////////////////////////
void CVisualizer::CNavgraphInfo::Clear()
{
	m_areaId = INVALID_AREA_ID;
	m_lastGraphVersion = 0xffffffff;
	m_lastObstaclesVersion = 0xbaada550;
	m_lastUsage = EngineTime::ZERO;
	if ( m_obstacles )
	{
		m_obstacles->Release();
		m_obstacles = nullptr;
	}
}
CVisualizer::CVisualizer( CPathLibWorld* pathlib )
	: m_pathlib( pathlib )
	, m_debugNavgraph( 0 )
	, m_colorNavgraphsByRegions( false )
	, m_terrainHeightDebugArea( INVALID_AREA_ID )
	, m_terrainHeightMesh( nullptr )
{
	m_navgraphs.Resize( m_navgraphs.Capacity() );
}
CVisualizer::~CVisualizer()
{
	if ( m_terrainHeightMesh )
	{
		m_terrainHeightMesh->Release();
	}
	for( auto it = m_navmeshRenderers.Begin(), end = m_navmeshRenderers.End(); it != end; ++it )
	{
		delete it->m_second;
	}
	m_navmeshRenderers.Clear();
	
}

void CVisualizer::GenerateEditorFragments( CRenderFrame* frame )
{
#ifdef TERRAIN_TILE_DEBUG_RENDERING
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_NavTerrain ) )
	{
		const PathLib::CTerrainInfo& terrainInfo = m_pathlib->GetTerrainInfo();

		if ( terrainInfo.IsInitialized() )
		{
			Vector cameraPosition = frame->GetFrameInfo().m_camera.GetPosition();

			Int32 tileX, tileY;
			terrainInfo.GetTileCoordsAtPosition( cameraPosition.AsVector2(), tileX, tileY );
			AreaId areaId = terrainInfo.GetTileIdFromCoords( tileX, tileY );
			CTerrainAreaDescription* terrainArea = m_pathlib->GetTerrainAreaDescription( areaId );
			const CClipMap* clipMap = m_pathlib->GetWorld()->GetTerrain();

			CTerrainTile* tile = clipMap ? clipMap->GetTile( tileX, tileY ) : NULL;
			if ( terrainArea && terrainArea->IsReady() && terrainArea->GetTerrainMap() && tile )
			{
				if ( tile->m_debugRenderingColorMap.Empty() || tile->m_debugRenderingVersion != terrainArea->GetTerrainMap()->GetVersion() )
				{
					ProcessTerrainTile( *terrainArea->GetTerrainMap(), tile );
				}
				if ( !tile->m_performDebugRendering )
				{
					tile->m_performDebugRendering = true;
					Uint32 tileHash = tileX | (tileY << 16);
					m_debuggedTiles.Insert( tileHash );
				}
			}
		}
	
	}
	else if ( !m_debuggedTiles.Empty() )
	{
		const CClipMap* clipMap = m_pathlib->GetWorld()->GetTerrain();
		if ( clipMap )
		{
			for ( auto it = m_debuggedTiles.Begin(), end = m_debuggedTiles.End(); it != end; ++it )
			{
				Uint32 tileHash = *it;
				Int32 tileX = tileHash & ((1 << 15)-1);
				Int32 tileY = tileHash >> 16;
				CTerrainTile* tile = clipMap->GetTile( tileX, tileY );
				if ( tile )
				{
					tile->m_performDebugRendering = false;
				}
			}
		}
		m_debuggedTiles.Clear();
	}
#endif // TERRAIN_TILE_DEBUG_RENDERING

	Bool showNavgraphs = frame->GetFrameInfo().IsShowFlagOn( SHOW_NavGraph );
	Bool showObstacles = frame->GetFrameInfo().IsShowFlagOn( SHOW_NavObstacles );

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_NavTerrainHeight ) )
	{
		DrawTerrainHeight( frame );
	}

	if ( showNavgraphs && ( m_colorNavgraphsByRegions != frame->GetFrameInfo().IsShowFlagOn( SHOW_NavGraphRegions ) ) )
	{
		m_colorNavgraphsByRegions = !m_colorNavgraphsByRegions;
		ClearNavgraphs();
	}

	if ( showNavgraphs || showObstacles )
	{
		CollectAndDrawNavgraphs( frame, showNavgraphs, showObstacles );
	}

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_NavMesh ) )
	{
		CollectAndDrawNavmeshes( frame );
	}
}

void CVisualizer::CollectAndDrawNavmeshes( CRenderFrame* frame )
{
	const CInstanceMap* instanceMap = m_pathlib->GetInstanceMap();

	if ( !instanceMap->IsInitialized() )
	{
		return;
	}

	m_renderedNavmeshes.ClearFast();

	Vector cameraPosition = frame->GetFrameInfo().m_camera.GetPosition();

	struct Functor : public CInstanceMap::CInstanceFunctor
	{
		Functor( CVisualizer* me, CRenderFrame* frame )
			: m_this( me )
			, m_frame( frame )
			, m_handled( false ) {}

		Bool Handle( CNavmeshAreaDescription* naviArea ) override
		{
			CNavmeshRenderer* naviRenderer = m_this->GetNavmeshRender( naviArea->GetId() );
			if ( naviRenderer )
			{
				naviRenderer->GenerateEditorFragments( m_frame );
				String areaName = String::Printf( TXT("instance_%04x"), naviArea->GetId() );
				m_frame->AddDebugText( naviArea->GetBBox().GetMassCenter(), areaName, false, Color::YELLOW );
				m_this->m_renderedNavmeshes.PushBack( naviArea->GetId() );

#ifndef NO_NAVMESH_GENERATION
				{
					CNavmesh* navmesh = naviArea->GetNavmesh();
					if ( navmesh && navmesh->HadProblems() )
					{
						const auto& generationProblems = navmesh->GetProblems();
						Vector centralPoint = naviArea->GetBBox().GetMassCenter();
						Uint32 centralProblemsCount = 0;
						for ( Uint32 i = 0, problemsCount = generationProblems.Size(); i < problemsCount; ++i )
						{
							const auto& problem = generationProblems[ i ];
							if ( problem.IsLocationUnspecified() )
							{
								m_frame->AddDebugText( centralPoint, problem.m_text, -50, centralProblemsCount++, true, Color::RED );
								if ( centralProblemsCount == 1 )
								{
									m_frame->AddDebugText( centralPoint, areaName, -50, -2, true, Color::RED );
								}
							}
							else
							{
								m_frame->AddDebugText( problem.m_location, areaName, -50, -2, true,Color::RED );
								m_frame->AddDebugText( problem.m_location, problem.m_text, -50, 0, true,Color::RED );
							}
						}

					}
				}
#endif
			}

			m_handled = true;

			return true;
		}

		CVisualizer*	m_this;
		CRenderFrame*	m_frame;
		Bool			m_handled;
	} functor( this, frame );

	instanceMap->IterateAreasAt( Box( cameraPosition - Vector( 30.f, 30.f, 100.f ), cameraPosition + Vector( 30.f, 30.f, 25.f ) ), &functor );

	if( !functor.m_handled )
	{
		AreaId instanceAreaId = instanceMap->GetClosestIntance( cameraPosition, 150.f );
		CNavmeshAreaDescription* naviArea = m_pathlib->GetInstanceAreaDescription( instanceAreaId );
		if ( naviArea )
		{
			functor.Handle( naviArea );
		}
	}
}

void CVisualizer::CollectAndDrawNavgraphs( CRenderFrame* frame, Bool showNavgraphs, Bool showObstacles )
{
	const CInstanceMap* instanceMap = m_pathlib->GetInstanceMap();

	if ( !instanceMap->IsInitialized() )
	{
		return;
	}

	Vector cameraPosition = frame->GetFrameInfo().m_camera.GetPosition();

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_NavMesh ) )
	{
		// instance area
		AreaId instanceAreaId = instanceMap->GetClosestIntance( cameraPosition, 15.f );
		if ( instanceAreaId != INVALID_AREA_ID )
		{
			Bool procceed = true;

			CNavgraphInfo* instanceInfo = GetNavgraph( instanceAreaId, showNavgraphs, showObstacles );
			if ( instanceInfo )
			{
				if ( showNavgraphs )
				{
					DrawNavgraph( frame, instanceInfo );
					showNavgraphs = false;
				}
				if ( showObstacles && instanceInfo->m_obstacles )
				{
					DrawObstacles( frame, instanceInfo );
					showObstacles = false;
				}
			}
		}

	}
	else
	{
		// terrain area
		const PathLib::CTerrainInfo& terrainInfo = m_pathlib->GetTerrainInfo();
		Int32 tileX, tileY;
		terrainInfo.GetTileCoordsAtPosition( cameraPosition.AsVector2(), tileX, tileY );
		AreaId areaId = terrainInfo.GetTileIdFromCoords( tileX, tileY );
		CNavgraphInfo* info = GetNavgraph( areaId, showNavgraphs, showObstacles );
		if ( info )
		{
			if ( showNavgraphs )
			{
				DrawNavgraph( frame, info );
				showNavgraphs = false;
			}
			if ( showObstacles && info->m_obstacles )
			{
				DrawObstacles( frame, info );
				showObstacles = false;
			}
		}
	}
}

void CVisualizer::DrawNavgraph( CRenderFrame* frame, CNavgraphInfo* data )
{
	frame->AddDebugLines( data->m_links.TypedData(), data->m_links.Size(), frame->GetFrameInfo().IsShowFlagOn( SHOW_NavGraphNoOcclusion ) );

	// Debug code that draws link costs
	//if ( s_drawLinksCost )
	//{
	//	CAreaDescription* area = m_pathlib->GetAreaDescription( data->m_areaId );
	//	if ( area )
	//	{
	//		CNavGraph* navgraph = area->GetNavigationGraph( m_debugNavgraph );
	//		if ( navgraph )
	//		{
	//			auto funDraw =
	//				[ frame ] ( CNavNode& node )
	//			{
	//				Vector3 pos = node.GetPosition();
	//				const auto& links = node.GetLinksArray();
	//				for ( auto it = links.Begin(), end = links.End(); it != end; ++it )
	//				{
	//					CPathNode* destNode = it->GetDestination();
	//					if ( destNode->GetIndex() < node.GetIndex() )
	//					{
	//						continue;
	//					}

	//					Vector3 midPos = (destNode->GetPosition() + pos) * 0.5f;

	//					frame->AddDebugText( Vector(midPos), String::Printf( TXT("%d"), it->GetCost() ), 0, 0 );
	//				}
	//			};

	//			NavgraphHelper::ForAllNodes( *navgraph, funDraw, true );
	//		}
	//	}
	//}
	
	//// Debug code that draw labels over nodes
	//CAreaDescription* area = m_pathlib->GetAreaDescription( data->m_areaId );
	//if ( area )
	//{
	//	CNavGraph* navgraph = area->GetNavigationGraph( 0 );
	//	if ( navgraph )
	//	{
	//		const auto& nodes = navgraph->GetNodesArray();
	//		for ( auto it = nodes.Begin(), end = nodes.End(); it != end; ++it )
	//		{
	//			frame->AddDebugText( Vector( (*it).GetPosition() + Vector3( 0.f, 0.f, 1.f ) ), String::Printf( TXT("%d"), (*it).GetIndex() ), 0, 0 );
	//		}
	//	}
	//}
}
void CVisualizer::DrawObstacles( CRenderFrame* frame, CNavgraphInfo* data )
{
	if ( data->m_obstacles )
	{
		new ( frame ) CRenderFragmentDebugMesh( frame, Matrix::IDENTITY, data->m_obstacles, true );

		//CAreaDescription* area = m_pathlib->GetAreaDescription( data->m_areaId );
		//if ( area )
		//{
		//	CObstaclesMap* obstacles = area->GetObstaclesMap();
		//	if ( obstacles )
		//	{
		//		Vector cameraPosition = frame->GetFrameInfo().m_camera.GetPosition();
		//		area->VWorldToLocal( cameraPosition.AsVector3() );
		//		Box bbox( cameraPosition, 30.f );
		//		
		//		CObstaclesMap::ObstaclesIterator it( obstacles, bbox.Min.AsVector2(), bbox.Max.AsVector2() );
		//		while ( it )
		//		{
		//			CObstacle* o = *it;
		//			CObstacleShape* s = o->GetShape();
		//			Vector3 pos = (s->GetBBoxMin() + s->GetBBoxMax()) * 0.5f;
		//			area->VLocalToWorld( pos );

		//			frame->AddDebugText( pos, String::Printf( TXT("%08x"), o->GetId() ), 0, 0 );

		//			++it;
		//		}
		//	}
		//}
	}
}

void CVisualizer::DrawTerrainHeight( CRenderFrame* frame )
{
	const PathLib::CTerrainInfo& terrainInfo = m_pathlib->GetTerrainInfo();

	if ( !terrainInfo.IsInitialized() )
	{
		return;
	}

	Vector cameraPosition = frame->GetFrameInfo().m_camera.GetPosition();

	Int32 tileX, tileY;
	terrainInfo.GetTileCoordsAtPosition( cameraPosition.AsVector2(), tileX, tileY );
	AreaId areaId = terrainInfo.GetTileIdFromCoords( tileX, tileY );

	if ( areaId != m_terrainHeightDebugArea )
	{
		if ( m_terrainHeightMesh )
		{
			m_terrainHeightMesh->Release();
			m_terrainHeightMesh = nullptr;
		}
		m_terrainHeightDebugArea = INVALID_AREA_ID;

		CTerrainAreaDescription* terrain = m_pathlib->GetTerrainAreaDescription( areaId );
		if ( terrain && terrain->IsReady() )
		{
			m_terrainHeightDebugArea = areaId;
			const CTerrainMap* terrainMap = terrain->GetTerrainMap();
			const CTerrainHeight& terrainHeight = terrainMap->GetHeightContext();
			m_terrainHeightMesh = terrainHeight.GenerateDebugMesh( terrain->GetCorner(), terrainInfo.GetTileSize() );
		}
	}

	if ( m_terrainHeightMesh )
	{
		new ( frame ) CRenderFragmentDebugMesh( frame, Matrix::IDENTITY, m_terrainHeightMesh, true );
	}
}

void CVisualizer::ClearNavgraphs()
{
	for ( Uint32 i = 0; i < m_navgraphs.Size(); ++i )
	{
		m_navgraphs[ i ].Clear();
	}
}
CNavmeshRenderer* CVisualizer::GetNavmeshRender( AreaId areaId )
{
	auto it = m_navmeshRenderers.Find( areaId );
	if ( it != m_navmeshRenderers.End() )
	{
		return it->m_second;
	}
	CNavmeshAreaDescription* area = m_pathlib->GetInstanceAreaDescription( areaId );
	if ( !area || !area->GetNavmesh() || !area->IsLoaded() )
	{
		return NULL;
	}
	CNavmeshRenderer* naviRenderer = new CNavmeshRenderer( area->GetId() );
	m_navmeshRenderers.Insert( areaId, naviRenderer );
	return naviRenderer;
}

void CVisualizer::InstanceUpdated( AreaId areaId )
{
	auto itFind = m_navmeshRenderers.Find( areaId );
	if ( itFind != m_navmeshRenderers.End() )
	{
		CNavmeshRenderer* naviRend = itFind->m_second;
		//naviRend->InvalidatePreview();
		delete naviRend;
		m_navmeshRenderers.Erase( itFind );
		
	}
}
void CVisualizer::InstanceRemoved( AreaId areaId )
{
	auto itFind = m_navmeshRenderers.Find( areaId );
	if ( itFind != m_navmeshRenderers.End() )
	{
		CNavmeshRenderer* naviRend = itFind->m_second;
		m_navmeshRenderers.Erase( itFind );
		delete naviRend;
	}
}

void CVisualizer::DebugNavgraph( Uint32 i )
{
	if ( m_debugNavgraph != i )
	{
		m_debugNavgraph = i;
		ClearNavgraphs();
	}
}

CVisualizer::CNavgraphInfo* CVisualizer::GetNavgraph( AreaId areaId,Bool precomputeNavgraph, Bool precomputeOccluders )
{
	CAreaDescription* description = m_pathlib->GetAreaDescription( areaId );
	if ( !description || !description->IsReady() )
		return NULL;
	CNavGraph* graph = description->GetNavigationGraph( Min( m_debugNavgraph, m_pathlib->GetGlobalSettings().GetCategoriesCount() ) );
	if ( !graph )
		return NULL;

	// check if we have this graph already precomputed
	EngineTime time = GEngine->GetRawEngineTime();
	for ( Uint32 i = 0; i < m_navgraphs.Size(); ++i )
	{
		auto& info = m_navgraphs[ i ];
		if ( info.m_areaId == areaId )
		{
			info.m_lastUsage = time;
			if ( precomputeNavgraph )
			{
				if ( graph->GetVersion() != info.m_lastGraphVersion )
				{
					ProcessNavgraph( &info, graph );
					info.m_lastGraphVersion = graph->GetVersion();
				}
			}
			if ( precomputeOccluders )
			{
				CObstaclesMap* obstaclesMap = description->GetObstaclesMap();
				if ( obstaclesMap && obstaclesMap->GetVersion() != info.m_lastObstaclesVersion )
				{
					ProcessObstacles( &info, description );
				}
			}
			return &info;
		}
	}
	// find unused navgraph info
	EngineTime oldestInfo = time - 1.f;
	Int32 bestNavgraph = -1;
	for( Uint32 i = 0; i < m_navgraphs.Size(); ++i )
	{
		auto& info = m_navgraphs[ i ];
		if ( info.m_lastUsage < oldestInfo )
		{
			oldestInfo = info.m_lastUsage;
			bestNavgraph = i;
		}
	}
	// reuse unused info for new graph
	if ( bestNavgraph >= 0 )
	{
		auto& info = m_navgraphs[ bestNavgraph ];
		info.Clear();
		info.m_lastUsage = time;

		
		info.m_areaId = areaId;
		if ( precomputeNavgraph )
		{
			ProcessNavgraph( &info, graph );
			info.m_lastGraphVersion = graph->GetVersion();
		}
		if ( precomputeOccluders )
		{
			CObstaclesMap* obstaclesMap = description->GetObstaclesMap();
			if ( obstaclesMap )
			{
				ProcessObstacles( &info, description );
			}
		}
		
		return &info; 
	}
	return NULL;
}
void CVisualizer::ProcessNavgraph( CNavgraphInfo* data,CNavGraph* graph )
{
	data->m_links.ClearFast();

	auto funDrawnNodes =
		[ this, data, graph ] ( const CNavGraph::NodesArray& nodes )
	{
		for ( Uint32 i = 0, n = nodes.Size(); i < n; ++i )
		{
			const CPathNode& node = nodes[ i ];

			for ( ConstLinksIterator it( node ); it; ++it )
			{
				const CPathLink& link = *it;
				const CPathNode* destinationNode = link.GetDestination();
				// each link should be processed only once
				if ( link.HaveAnyFlag( NF_CONNECTOR ) || destinationNode->GetIndex() < node.GetIndex() )
				{
					ProcessLink( data, graph, &node, destinationNode, &link );
				}
			}
		}
	};

	{
		const auto& nodes = graph->GetNodesArray();
		funDrawnNodes( nodes );
	}
	
	{
		for ( auto it = graph->GetNodeSets().Begin(), end = graph->GetNodeSets().End(); it != end; ++it  )
		{
			const CNavgraphNodeSet* nodeSet = it->m_second;
			const auto& nodes = nodeSet->GetNodesArray();
			funDrawnNodes(  nodes );
		}
	}
	
	//if ( !graph->m_colors.Empty() )
	//{
	//	data->m_useColors = true;
	//	data->m_colors = graph->m_colors;
	//}
}
void CVisualizer::ProcessObstacles( CNavgraphInfo* data, CAreaDescription* area )
{
	CObstaclesMap* obstaclesMap = area->GetObstaclesMap();
	if ( obstaclesMap )
	{
		if ( data->m_obstacles )
		{
			data->m_obstacles->Release();
			data->m_obstacles = NULL;
		}

		const Color PERSISTANT_COLOR = Color( 255, 240, 0, 120 );
		const Color GROUPED_DISABLED_COLOR = Color( 217, 255, 174, 75 );
		const Color GROUPED_ENABLED_COLOR = Color( 172, 254, 82, 200 );
		const Color DYNAMIC_DISABLED_COLOR = Color( 170, 170, 170, 75 );
		const Color DYNAMIC_ENABLED_COLOR = Color( 0, 60, 240, 200 );
		const Color IMMEDIATE_COLOR = Color( 255, 0, 240, 120 );

		TDynArray< Uint32 > indices;
		TDynArray< DebugVertex > vertices;

		for ( auto it = obstaclesMap->m_obstacles.Begin(), end = obstaclesMap->m_obstacles.End(); it != end; ++it )
		{
			CObstacle* obstacle = it->m_second;
			CObstacleShape* shape = obstacle->GetShape();
			if ( !shape )
			{
				continue;
			}
			
			CObstacleShapePoly* poly = shape->AsPoly();
			CObstacleShapeLineSegment* lineSegment = poly ? NULL : shape->AsLineSegment();

			Bool persistant = obstacle->IsPersistant();
			Color color = PERSISTANT_COLOR;
			if ( !persistant )
			{
				Bool isMarked = (obstacle->GetFlags() & CObstacle::IS_MARKED) != 0;
				if ( obstacle->GetFlags() & CObstacle::IS_GROUPED )
				{
					color = isMarked
						? GROUPED_ENABLED_COLOR
						: GROUPED_DISABLED_COLOR;
				}
				else if ( obstacle->AsDynamicPregeneratedObstacle() )
				{
					color = isMarked
						? DYNAMIC_ENABLED_COLOR
						: DYNAMIC_DISABLED_COLOR;
				}
				else
				{
					color =  IMMEDIATE_COLOR;
				}
			}

			auto funDrawBox = 
				[ this, &indices, &vertices, area, color ] ( CObstacleShapeBox* box )
			{
				Uint32 baseVertex = vertices.Size();
				Vector3 min = box->GetBBoxMin();
				Vector3 max = box->GetBBoxMax();

				area->VLocalToWorld( min );
				area->VLocalToWorld( max );

				vertices.Grow( 8 );
				vertices[baseVertex + 0] = DebugVertex( Vector( min.X, min.Y, max.Z ), color );
				vertices[baseVertex + 1] = DebugVertex( Vector( max.X, min.Y, max.Z ), color );
				vertices[baseVertex + 2] = DebugVertex( Vector( min.X, max.Y, max.Z ), color );
				vertices[baseVertex + 3] = DebugVertex( Vector( max.X, max.Y, max.Z ), color );
				vertices[baseVertex + 4] = DebugVertex( Vector( min.X, min.Y, min.Z ), color );
				vertices[baseVertex + 5] = DebugVertex( Vector( max.X, min.Y, min.Z ), color );
				vertices[baseVertex + 6] = DebugVertex( Vector( min.X, max.Y, min.Z ), color );
				vertices[baseVertex + 7] = DebugVertex( Vector( max.X, max.Y, min.Z ), color );

				const Uint32 numIdx = 36;
				const Uint16 idx[ numIdx ] = {
					0, 1, 2, 1, 3, 2,			// top
					5, 4, 6, 5, 6, 7,			// bottom
					0, 2, 4, 4, 2, 6,			// left
					1, 5, 3, 3, 5, 7,			// right
					2, 3, 6, 3, 7, 6,			// front
					0, 4, 1, 1, 4, 5			// back
				};

				Uint32 baseIndex = indices.Size();
				indices.Grow( numIdx );
				for ( Uint32 i = 0; i < numIdx; ++i )
				{
					indices[baseIndex + i] = idx[i] + baseVertex;
				}
			};

			auto funDrawCircle =
				[ this, &indices, &vertices, area, color ] ( CObstacleShapeCircle* circle )
			{
				const Uint32 CIRCLE_VERTEX_COUNT = 8;

				Uint32 baseVertice = vertices.Size();
				Uint32 baseIndice = indices.Size();

				vertices.Grow( CIRCLE_VERTEX_COUNT*2 );
				indices.Grow( 12 * CIRCLE_VERTEX_COUNT - 12 );

				Vector worldPos;
				worldPos.AsVector2() = circle->GetCenter();
				worldPos.Z = circle->GetBBoxMin().Z;
				area->VLocalToWorld( worldPos.AsVector3() );
				Float zMax = circle->GetBBoxMax().Z;
				area->VLocalToWorldZ( zMax );
				Float radius = circle->GetRadius();
				Float angleStep = ( M_PI*2.f ) / Float( CIRCLE_VERTEX_COUNT );

				// add vertices
				Uint32 currVert = baseVertice;
				for( Uint32 i = 0; i < CIRCLE_VERTEX_COUNT; ++i )
				{
					Vector pointPos = worldPos;
					pointPos.AsVector2() += MathUtils::GeometryUtils::Rotate2D( Vector2( radius, 0.f ), Float( i ) * angleStep );
					vertices[ currVert++ ] = DebugVertex( pointPos, color );
					vertices[ currVert++ ] = DebugVertex( Vector( pointPos.X, pointPos.Y, zMax ), color );
				}
				PATHLIB_ASSERT( currVert == vertices.Size() );

				// add wall indices
				Uint32 wallIndice = baseVertice;
				Uint32 ind = baseIndice;
				for( Uint32 i = 0; i < CIRCLE_VERTEX_COUNT; ++i )
				{
					Uint32 ind0 = wallIndice+0;
					Uint32 ind1 = wallIndice+1;
					Uint32 ind2 = ( i < CIRCLE_VERTEX_COUNT-1 ) ? wallIndice+2 : baseVertice;
					Uint32 ind3 = ( i < CIRCLE_VERTEX_COUNT-1 ) ? wallIndice+3 : baseVertice+1;

					// wall tri1
					indices[ ind++ ] = ind1;
					indices[ ind++ ] = ind3;
					indices[ ind++ ] = ind2;
					// wall tri2
					indices[ ind++ ] = ind0;
					indices[ ind++ ] = ind1;
					indices[ ind++ ] = ind2;

					wallIndice += 2;
				}

				// spawn roof & base
				for ( Uint32 i = 2; i < CIRCLE_VERTEX_COUNT; ++i )
				{
					// base
					indices[ ind++ ] = baseVertice;
					indices[ ind++ ] = baseVertice + (i-1)*2;
					indices[ ind++ ] = baseVertice + i*2;

					// roof
					indices[ ind++ ] = baseVertice + i*2 + 1;
					indices[ ind++ ] = baseVertice + (i-1)*2 + 1;
					indices[ ind++ ] = baseVertice + 1;
				}
				PATHLIB_ASSERT( ind == indices.Size() );
			};

			auto funDrawLine =
				[ this, &indices, &vertices, area, color ] ( CObstacleShapeLineSegment* line )
			{
				Uint32 baseVertice = vertices.Size();
				Uint32 baseIndice = indices.Size();
				vertices.Grow( 4 );
				indices.Grow( 4 * 3 );

				for( Uint32 i = 0; i < 4; ++i )
				{
					Vector3 v = line->GetVert( i );
					area->VLocalToWorld( v );
					vertices[ baseVertice + i ] = DebugVertex( v, color );
				}

				indices[ baseIndice+0  ] = baseVertice+0;
				indices[ baseIndice+1  ] = baseVertice+1;
				indices[ baseIndice+2  ] = baseVertice+2;
				indices[ baseIndice+3  ] = baseVertice+2;
				indices[ baseIndice+4  ] = baseVertice+1;
				indices[ baseIndice+5  ] = baseVertice+3;
				indices[ baseIndice+6  ] = baseVertice+2;
				indices[ baseIndice+7  ] = baseVertice+1;
				indices[ baseIndice+8  ] = baseVertice+0;
				indices[ baseIndice+9  ] = baseVertice+3;
				indices[ baseIndice+10 ] = baseVertice+1;
				indices[ baseIndice+11 ] = baseVertice+2;
			};

			auto funDrawPolyShape =
				[ this, &indices, &vertices, area, color ] ( CObstacleShapePoly* poly )
			{
				const auto& vertsList = poly->GetVerts();
				Uint32 vertsCount = vertsList.Size();
				Float minZ = poly->GetBBoxMin().Z;
				Float maxZ = poly->GetBBoxMax().Z;
				Uint32 indiceMin = vertices.Size();
				Uint32 prevIndicesCount = indices.Size();
				// spawn wall
				for ( Uint32 i = 0; i < vertsCount; ++i )
				{
					const Vector2& v = vertsList[ i ];

					Vector3 vMin( v.X, v.Y, minZ );
					Vector3 vMax( v.X, v.Y, maxZ );
					area->VLocalToWorld( vMin );
					area->VLocalToWorld( vMax );
					// add vertices
					vertices.PushBack( DebugVertex( vMin, color ) );
					vertices.PushBack( DebugVertex( vMax, color ) );

					// add indices
					if ( i > 0 )
					{
						Uint32 baseInd = vertices.Size();
						// wall tri1
						indices.PushBack( baseInd-1 );
						indices.PushBack( baseInd-2 );
						indices.PushBack( baseInd-3 );
						// wall tri2
						indices.PushBack( baseInd-4 );
						indices.PushBack( baseInd-3 );
						indices.PushBack( baseInd-2 );
					}
				};

				Uint32 indiceLimit = vertices.Size();

				// final quad wrapping poly end with beginning
				indices.PushBack( indiceMin );
				indices.PushBack( indiceLimit-2 );
				indices.PushBack( indiceLimit-1 );

				indices.PushBack( indiceLimit-1 );
				indices.PushBack( indiceMin+1 );
				indices.PushBack( indiceMin );

				auto isConvex = MathUtils::GeometryUtils::IsPolygonConvex2D( vertsList );
				if ( isConvex )
				{
					// spawn roof & base
					for ( Uint32 i = 2; i < vertsCount; ++i )
					{
						// base
						indices.PushBack( indiceMin );
						indices.PushBack( indiceMin + (i-1)*2 );
						indices.PushBack( indiceMin + i*2 );

						// roof
						indices.PushBack( indiceMin + i*2 + 1);
						indices.PushBack( indiceMin + (i-1)*2 + 1 );
						indices.PushBack( indiceMin + 1);
					}
				}
				else
				{
					// spawn wall 2nd side (cause roof visualization don't support non-convex polys)
					Uint32 createdIndices = indices.Size() - prevIndicesCount;
					indices.Grow( createdIndices );
					Red::System::MemoryCopy( &indices[ prevIndicesCount+createdIndices ], &indices[ prevIndicesCount ], createdIndices * sizeof( Uint32 ) );
					Reverse( indices.Begin()+prevIndicesCount, indices.Begin() + (prevIndicesCount+createdIndices) );
				}
			};
			auto funDrawShape = 
				[ funDrawPolyShape, funDrawCircle, funDrawLine, funDrawBox ] ( CObstacleShape* shape )
			{
				CObstacleShapePoly* poly = shape->AsPoly();
				if ( poly )
				{
					funDrawPolyShape( poly );
					return;
				}
				CObstacleShapeCircle* circle = shape->AsCircle();
				if ( circle )
				{
					funDrawCircle( circle );
					return;
				}
				CObstacleShapeLineSegment* line = shape->AsLineSegment();
				if ( line )
				{
					funDrawLine( line );
					return;
				}
				CObstacleShapeBox* box = shape->AsBox();
				if ( box )
				{
					funDrawBox( box );
					return;
				}
			};

			CObstacleShapeComposite* composite = shape->AsComposite();
			if ( composite )
			{
				const auto& subShapes = composite->GetSubShapes();
				for ( auto it = subShapes.Begin(), end = subShapes.End(); it != end; ++it )
				{
					funDrawShape( *it );
				}
			}
			else
			{
				funDrawShape( shape );
			}
		}

		data->m_lastObstaclesVersion = obstaclesMap->GetVersion();

		if ( !indices.Empty() )
		{
			data->m_obstacles = GRender->UploadDebugMesh( vertices, indices );
		}
	}
}

void CVisualizer::ProcessLink(CNavgraphInfo* data, CNavGraph* graph, const CPathNode* node1, const CPathNode* node2, const CPathLink* link)
{
	static const Int32 MAX_COLORS = 16;
	static const Color COLORS[ MAX_COLORS ] =
	{
		Color( 100, 100, 255 ),
		Color( 100, 255, 100 ),
		Color( 255, 255, 0 ),
		Color( 255, 0, 255 ),
		Color( 119, 222, 202 ),
		Color( 0, 255, 255 ),
		Color( 185, 0, 255 ),
		Color( 236, 255, 0 ),
		Color( 166, 255, 90 ),
		Color( 165, 3, 252 ),
		Color( 0, 140, 255 ),
		Color( 179, 255, 0 ),
		Color( 3, 252, 158 ),
		Color( 3, 90, 252 ),
		Color( 64, 255, 0 ),
		Color( 11, 244, 198 ),
	};

	Color color;

	if ( m_colorNavgraphsByRegions )
	{
		if ( link->HaveAnyFlag( NFG_FORBIDDEN_ALWAYS ) )
		{
			if ( ( node1->HaveAnyFlag( NF_BLOCKED ) && node1->GetAreaRegionId() != INVALID_AREA_REGION )
				|| ( node2->HaveAnyFlag( NF_BLOCKED ) && node2->GetAreaRegionId() != INVALID_AREA_REGION ) )
			{
				color = Color::LIGHT_RED;
			}
			else
			{
				color = Color::BLACK;
			}
		}
		else
		{
			AreaRegionId region = node1->GetAreaRegionId();
			AreaRegionId region2 = node2->GetAreaRegionId();
			if ( region == INVALID_AREA_REGION || region2 == INVALID_AREA_REGION )
			{
				color = Color::RED;
			}
			else if ( region != region2 )
			{
				if ( link->HaveFlag( NF_PLAYER_ONLY_PORTAL ) )
				{
					color = Color::YELLOW;
				}
				else
				{
					color = Color::WHITE;
				}
			}
			else
			{
				color = COLORS[ region % MAX_COLORS ];
			}
		}
	}
	else
	{
		if ( link->HaveAnyFlag( NF_DETACHED ) )
		{
			color = Color( 50, 50, 50, 50 );
		}
		else if ( link->HaveAnyFlag( NF_CONNECTOR ) )
		{
			Vector pos0 = ( node1->GetPosition() + node2->GetPosition() ) * 0.5f + Vector( 0.f, 0.f, 1.f );
			Vector pos1 = pos0 + Vector( 0.5f, 0.0f, 0.0f );
			Vector pos2 = pos0 + Vector( 0.0f, 0.5f, 0.0f );
			Vector pos3 = pos0 + Vector(-0.5f, 0.0f, 0.0f );
			Vector pos4 = pos0 + Vector( 0.0f,-0.5f, 0.0f );
			Vector posH = pos0 + Vector( 0.0f, 0.0f, 0.5f );

			data->m_links.PushBack( DebugVertex( pos1, Color::LIGHT_BLUE ) );
			data->m_links.PushBack( DebugVertex( pos2, Color::LIGHT_BLUE ) );
			data->m_links.PushBack( DebugVertex( pos2, Color::LIGHT_BLUE ) );
			data->m_links.PushBack( DebugVertex( pos3, Color::LIGHT_BLUE ) );
			data->m_links.PushBack( DebugVertex( pos3, Color::LIGHT_BLUE ) );
			data->m_links.PushBack( DebugVertex( pos4, Color::LIGHT_BLUE ) );
			data->m_links.PushBack( DebugVertex( pos4, Color::LIGHT_BLUE ) );
			data->m_links.PushBack( DebugVertex( pos1, Color::LIGHT_BLUE ) );

			data->m_links.PushBack( DebugVertex( pos1, Color::LIGHT_BLUE ) );
			data->m_links.PushBack( DebugVertex( posH, Color::LIGHT_BLUE ) );
			data->m_links.PushBack( DebugVertex( pos2, Color::LIGHT_BLUE ) );
			data->m_links.PushBack( DebugVertex( posH, Color::LIGHT_BLUE ) );
			data->m_links.PushBack( DebugVertex( pos3, Color::LIGHT_BLUE ) );
			data->m_links.PushBack( DebugVertex( posH, Color::LIGHT_BLUE ) );
			data->m_links.PushBack( DebugVertex( pos4, Color::LIGHT_BLUE ) );
			data->m_links.PushBack( DebugVertex( posH, Color::LIGHT_BLUE ) );

			color = Color::LIGHT_BLUE;
		}
		else if ( link->HaveFlag( NF_PLAYER_ONLY_PORTAL ) )
		{
			color = Color::YELLOW;
		}
		else if ( link->HaveFlag( NF_IS_CUSTOM_LINK ) )
		{
			if ( link->HaveFlag( NF_IS_ONE_SIDED ) )
			{
				Vector diff = ( node1->GetPosition() - node2->GetPosition() );
				diff.Normalize3();
				diff *= 0.33f;
				Vector arrowHeadLDiff = diff;
				Vector arrowHeadRDiff = diff;
				arrowHeadLDiff.AsVector2() = MathUtils::GeometryUtils::Rotate2D( arrowHeadLDiff.AsVector2(), M_PI * 0.25f );
				arrowHeadRDiff.AsVector2() = MathUtils::GeometryUtils::Rotate2D( arrowHeadRDiff.AsVector2(), -M_PI * 0.25f );
				Vector tip = node2->GetPosition() + Vector3( 0.f,0.f,1.f );

				data->m_links.PushBack( DebugVertex( tip, Color::WHITE ) );
				data->m_links.PushBack( DebugVertex( tip + arrowHeadLDiff, Color::WHITE ) );
				data->m_links.PushBack( DebugVertex( tip, Color::WHITE ) );
				data->m_links.PushBack( DebugVertex( tip + arrowHeadRDiff, Color::WHITE ) );
			}
			color = Color::WHITE;
		}
		else if ( link->HaveAnyFlag( NFG_FORBIDDEN_ALWAYS ) )
		{
			color = Color::BLACK;
		}
		else if ( link->HaveAnyFlag( NF_ROUGH_TERRAIN ) )
		{
			color = Color::BROWN;
		}
		else if ( link->HaveAnyFlag( NF_INTERIOR ) )
		{
			color = Color::GREEN;
		}
		else if ( link->HaveFlag( NF_IS_IN_NODESET ))
		{
			color = Color::BLUE;
		}
		else
		{
			color = Color::CYAN;
		}
	}
	
	data->m_links.PushBack( DebugVertex( Vector( node1->GetPosition() + Vector3( 0.f,0.f,1.f ) ), color ) );
	data->m_links.PushBack( DebugVertex( Vector( node2->GetPosition() + Vector3( 0.f,0.f,1.f ) ), color ) );
}

void CVisualizer::ProcessTerrainTile( const CTerrainMap& terrainMap, CTerrainTile* terrainTile )
{
#ifdef TERRAIN_TILE_DEBUG_RENDERING
	if ( terrainMap.IsInitialized() )
	{
		auto& colorMap = terrainTile->m_debugRenderingColorMap;
		Uint32 resolution = terrainMap.GetTerrainInfo()->GetTilesResolution();
		Uint32 quadsCount = resolution * resolution;
		colorMap.Resize( quadsCount );
		TControlMapType free = ( 2 ) | ( 2 << 5 );
		TControlMapType occupied = ( 1 ) | ( 1 << 5 );
		for ( Uint32 i = 0; i < quadsCount; ++i )
		{
			colorMap[ i ] = terrainMap.GetQuadState( i ) == CTerrainMap::QUAD_FREE ? free : occupied;
		}

		terrainTile->m_debugRenderingVersion = terrainMap.GetVersion();
		terrainTile->m_debugForceRenderingUpdate = true;
	}
#endif // TERRAIN_TILE_DEBUG_RENDERING
}

};				// namespace PathLib
