#include "build.h"
#include "pathlibNavmeshArea.h"

#include "pathlibAreaProcessingJob.h"
#include "pathlibNavgraph.h"
#include "pathlibNavmesh.h"
#include "pathlibNavmeshComponent.h"
#include "pathlibObstaclesMap.h"
#include "pathlibSimpleBuffers.h"
#include "pathlibTerrain.h"
#include "pathlibVisualizer.h"

#include "../core/depot.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// CNavmeshAreaDescription
////////////////////////////////////////////////////////////////////////////
#ifndef NO_EDITOR_PATHLIB_SUPPORT
void CNavmeshAreaDescription::GenerateAsync( CAreaGenerationJob* job ) 
{
	CNavmesh* navmesh = m_navmesh.Get();
	if ( !navmesh )
	{
		Clear();
		return;
	}

	CAreaNavgraphsRes* graphs = m_graphs.Get();
	if ( !graphs )
	{
		m_graphs.Construct();
		graphs = m_graphs.Get();
		graphs->OnPreLoad( this );
		graphs->OnPostLoad( this );
	}

	for ( Uint16 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		Uint32 version = 0;
		CNavGraph* g = graphs->GetGraph( i );
		if ( g )
		{
			version = g->GetVersion();
			graphs->ClearDetachedGraph( i );
		}

		if ( i < m_pathlib->GetGlobalSettings().GetCategoriesCount() && m_usedCategories & (1 << i))
		{
			g = graphs->NewGraph( i, this );
			g->SetVersion( version );
			if ( !g->Generate( m_pathlib, navmesh, job ) )
			{
				graphs->ClearDetachedGraph( i );
				continue;
			}
			g->CompactData();
		}
	}

	CNavModyficationMap* metalinks = GetMetalinks();
	if ( metalinks )
	{
		metalinks->RemoveNotAppliedModyfications();
	}
}
void CNavmeshAreaDescription::GenerateSync()
{
	GenerateAsync( NULL );
}

Bool CNavmeshAreaDescription::PreGenerateSync()
{
	DetermineNeighbourAreas();
	// force all terrain neighbours to recalculate themselves
	for ( Uint32 i = 0, n = m_neighbourAreas.Size(); i != n; ++i )
	{
		if( m_neighbourAreas[ i ] == INVALID_AREA_ID )
		{
			continue;
		}
		CAreaDescription* area = m_pathlib->GetAreaDescription( m_neighbourAreas[ i ] );
		if ( area )
		{
			area->MarkDirty( DIRTY_TASK_CALCULATE_NEIGHBOURS );
		}
	}
	CNavmesh* navmesh = m_navmesh.Get();
	// important so we don't start job if it will fail in moment
	if ( !navmesh )
	{
		return false;
	}
	return true;
}
void CNavmeshAreaDescription::Describe( String& description )
{
	description = String::Printf( TXT( "NavmeshInstance %04x" ), m_id & ID_MASK_INDEX );
}

Bool CNavmeshAreaDescription::CorrectNeighbourList()
{
	CNavmesh* navmesh = m_navmesh.Get();
	if ( !navmesh )
	{
		return false;
	}

	CNavmesh::TriangleIndex phantomEdgesCount = navmesh->GetPhantomEdgesCount();
	if ( m_neighbourAreas.Size() ==  phantomEdgesCount )
	{
		Bool hasChanged = false;
		for ( CNavmesh::TriangleIndex tri = 0, triCount = navmesh->GetTrianglesCount(); tri < triCount; ++tri )
		{
			CNavmesh::TriangleIndex neighbours[ 3 ];
			navmesh->GetTriangleNeighbours( tri, neighbours );

			// enumerate all triangles to find phantom edges
			for ( Uint32 i = 0; i < 3; ++i )
			{
				if ( CNavmesh::IsPhantomEdge( neighbours[ i ] ) )
				{
					// determine phantom edge position
					Vector3 verts[2];
					navmesh->GetTriangleEdge( tri, i, verts );
					Vector3 centralPoint = (verts[0] + verts[1]) * 0.5f;

					VLocalToWorld( centralPoint );

					CNavmesh::TriangleIndex phantomEdgeIndex = CNavmesh::PhantomEdgeNeighbourIndex( neighbours[ i ] );

					if ( m_neighbourAreas[ phantomEdgeIndex ] != INVALID_AREA_ID )
					{
						// check if we still collide with given area
						CAreaDescription* area = m_pathlib->GetAreaDescription( m_neighbourAreas[ phantomEdgeIndex ] );
						if ( area && area->IsLoaded() ) // TODO: remove 2nd condition
						{
							// TODO: SUPPORT THIS FUCKIN CASE CAreaHandler loader( area, CAreaHandler::SYNCHRONOUS );
							if ( !area->VContainsPoint( centralPoint ) )
							{
								hasChanged = true;
								break;
							}
						}
					}
					else
					{
						// find area that connects with given edge
						CAreaDescription* area = m_pathlib->GetInstanceMap()->GetInstanceAt( centralPoint, m_id );
						if ( !area )
						{
							AreaId terrainAreaId = m_pathlib->GetTerrainInfo().GetTileIdAtPosition( centralPoint.AsVector2() );
							CTerrainAreaDescription* terrainArea = m_pathlib->GetTerrainAreaDescription( terrainAreaId );
							if ( terrainArea && terrainArea->IsLoaded() ) // TODO: remove 2nd condition
							{
								// TODO: SUPPORT THIS FUCKIN CASE CTerrainAreaHandler loader( terrainArea, CTerrainAreaHandler::SYNCHRONOUS );
								if ( terrainArea->ContainsPoint( centralPoint ) )
								{
									area = terrainArea;
								}
							}
						}
						if ( area )
						{
							hasChanged = true;
							break;
						}
					}
				}
			}

			if ( hasChanged )
			{
				break;
			}
		}
		if ( !hasChanged )
		{
			return false;
		}
	}
	MarkDirty( DIRTY_GENERATE );

#ifndef NO_EDITOR_PATHLIB_SUPPORT
	CVisualizer* visualizer = m_pathlib->GetVisualizer();
	if ( visualizer )
	{
		visualizer->InstanceUpdated( m_id );
	}
#endif

	return false;
	//ClearNeighboursConnection();
	//DetermineNeighbourAreas();
	//ConnectWithNeighbours();
	//return true;
}


#endif // NO_EDITOR_PATHLIB_SUPPORT
void CNavmeshAreaDescription::ConnectWithNeighbours()
{
	CAreaDescription::ConnectWithNeighbours();
}

Float CNavmeshAreaDescription::GetMaxNodesDistance() const
{
	return 30.f;
}


void CNavmeshAreaDescription::OnPostLoad()
{
	Super::OnPostLoad();
}

void CNavmeshAreaDescription::ClearNeighboursConnection()
{
	CAreaDescription::ClearNeighboursConnection();
}
void CNavmeshAreaDescription::DetermineNeighbourAreas()
{
	CNavmesh* navmesh = m_navmesh.Get();
	if ( !navmesh )
	{
		return;
	}
	CNavmesh::TriangleIndex phantomEdgesCount = navmesh->GetPhantomEdgesCount();
	m_neighbourAreas.Resize( phantomEdgesCount );
	for ( CNavmesh::TriangleIndex i = 0; i < phantomEdgesCount; ++i )
	{
		m_neighbourAreas[ i ] = INVALID_AREA_ID;
	}

	for ( CNavmesh::TriangleIndex tri = 0, triCount = navmesh->GetTrianglesCount(); tri < triCount; ++tri )
	{
		CNavmesh::TriangleIndex neighbours[ 3 ];
		navmesh->GetTriangleNeighbours( tri, neighbours );
		
		// enumerate all triangles to find phantom edges
		for ( Uint32 i = 0; i < 3; ++i )
		{
			if ( CNavmesh::IsPhantomEdge( neighbours[ i ] ) )
			{
				// determine phantom edge position
				Vector3 verts[2];
				navmesh->GetTriangleEdge( tri, i, verts );
				Vector3 centralPoint = (verts[0] + verts[1]) * 0.5f;

				VLocalToWorld( centralPoint );

				// find area that connects with given edge
				CAreaDescription* area = m_pathlib->GetInstanceMap()->GetInstanceAt( centralPoint, m_id );
				if ( !area )
				{
					AreaId terrainAreaId = m_pathlib->GetTerrainInfo().GetTileIdAtPosition( centralPoint.AsVector2() );
					CTerrainAreaDescription* terrainArea = m_pathlib->GetTerrainAreaDescription( terrainAreaId );
					if ( terrainArea && terrainArea->IsLoaded() ) // TODO: Remove 2nd condition
					{
						// TODO: Bring it back!!! CTerrainAreaHandler areaHandler( terrainArea, CTerrainAreaHandler::SYNCHRONOUS );
						if ( terrainArea->ContainsPoint( centralPoint ) )
						{
							area = terrainArea;
						}
					}
				}

				if ( area )
				{
					CNavmesh::TriangleIndex phantomEdgeIndex = CNavmesh::PhantomEdgeNeighbourIndex( neighbours[ i ] );
					if ( phantomEdgeIndex < phantomEdgesCount )
					{
						m_neighbourAreas[ phantomEdgeIndex ] = area->GetId();
					}
				}
			}
		}
	}
#ifndef NO_EDITOR_PATHLIB_SUPPORT
	CVisualizer* visualizer = m_pathlib->GetVisualizer();
	if ( visualizer )
	{
		visualizer->InstanceUpdated( m_id );
	}
#endif
}

void CNavmeshAreaDescription::Clear()
{
	CAreaDescription::Clear();
}
void CNavmeshAreaDescription::SetNavmesh( CNavmesh* navi, const String& externalFilePath )
{
	Bool isDirty = false;

	if ( !externalFilePath.Empty() )
	{
		if ( ( m_areaFlags & FLAG_UseExternalNavmesh ) == 0 )
		{
			m_areaFlags |= FLAG_UseExternalNavmesh;
			isDirty = true;
		}

	}
	else
	{
		if ( m_areaFlags & FLAG_UseExternalNavmesh )
		{
			m_areaFlags &= ~FLAG_UseExternalNavmesh;
			isDirty = true;
		}
	}

	CNavmeshRes* navmesh = m_navmesh.Get();
	if ( !navmesh )
	{
		navmesh = m_navmesh.Construct();
	}
	if ( navmesh )
	{
		isDirty = true;
		navmesh->CopyFrom( navi );
		MarkDirty( DIRTY_GENERATE );
		// TODO: Disconnect it when we start serializing the navmesh
		OnNavmeshUpdated();
	}

	if ( isDirty )
	{
		MarkDirty( DIRTY_SAVE );
	}
}

void CNavmeshAreaDescription::OnRemoval()
{
	m_pathlib->UpdateInstancesMarking( m_bbox.Min.AsVector2(), m_bbox.Max.AsVector2() );

	Super::OnRemoval();
}
void CNavmeshAreaDescription::OnNavmeshUpdated()
{
	Vector2 testMin = m_bbox.Min.AsVector2();
	Vector2 testMax = m_bbox.Max.AsVector2();
	CNavmeshRes* navmesh = m_navmesh.Get();
	if ( navmesh )
	{
		m_bbox = navmesh->GetBoundingBox();
		//m_navmesh->ComputeConvexBoundings( m_bbox, m_convexBoundings );
		testMin.Set( Min( testMin.X, m_bbox.Min.X ), Min( testMin.Y, m_bbox.Min.Y ) );
		testMax.Set( Max( testMax.X, m_bbox.Max.X ), Max( testMax.Y, m_bbox.Max.Y ) );
	}

	m_pathlib->UpdateInstancesMarking( testMin, testMax );
}
Bool CNavmeshAreaDescription::IterateAreaResources( ResourceFunctor& functor )
{
	if ( !functor.Handle( m_navmesh, m_navmesh.GetResType() ) )
	{
		return false;
	}

	return Super::IterateAreaResources( functor );
}

void CNavmeshAreaDescription::GatherNeighbourAreas( TSortedArray< AreaId >& outAreas ) const
{
	CAreaDescription::GatherNeighbourAreas( outAreas );

	for ( Uint32 i = 0, n = m_neighbourAreas.Size(); i != n; ++i )
	{
		AreaId id = m_neighbourAreas[ i ];
		if ( id != INVALID_AREA_ID && outAreas.Find( id ) == outAreas.End() )
		{
			outAreas.Insert( id );
		}
	}
}

void CNavmeshAreaDescription::GatherPossibleConnectors( AreaId neighbourId, TDynArray< Vector3 >& outLocations ) const
{
	CNavmesh* navmesh = m_navmesh.Get();
	if ( !navmesh )
	{
		return;
	}

	CNavmesh::TriangleIndex phantomEdgesCount = navmesh->GetPhantomEdgesCount();
	ASSERT( m_neighbourAreas.Size() ==  phantomEdgesCount );
	if ( m_neighbourAreas.Size() != phantomEdgesCount )
	{
		return;
	}

	Bool hasChanged = false;
	for ( CNavmesh::TriangleIndex tri = 0, triCount = navmesh->GetTrianglesCount(); tri < triCount; ++tri )
	{
		CNavmesh::TriangleIndex neighbours[ 3 ];
		navmesh->GetTriangleNeighbours( tri, neighbours );

		// enumerate all triangles to find phantom edges
		for ( Uint32 i = 0; i < 3; ++i )
		{
			if ( CNavmesh::IsPhantomEdge( neighbours[ i ] ) )
			{
				// determine phantom edge position
				CNavmesh::TriangleIndex phantomEdgeIndex = CNavmesh::PhantomEdgeNeighbourIndex( neighbours[ i ] );

				if ( m_neighbourAreas[ phantomEdgeIndex ] == neighbourId )
				{
					Vector3 verts[2];
					navmesh->GetTriangleEdge( tri, i, verts );
					Vector3 centralPoint = (verts[0] + verts[1]) * 0.5f;

					VLocalToWorld( centralPoint );
					outLocations.PushBack( centralPoint );
				}
			}
		}
	}
}

void CNavmeshAreaDescription::NoticeEngineComponent( CNavmeshComponent* component )
{
	component->SetPathLibAreaId( m_id );
	m_ownerComponent = component->GetGUID4PathLib();
	CNavmesh* navmesh = component->GetNavmesh();
	if ( navmesh )
	{
		SetNavmesh( navmesh, component->GetSharedFilePath() );
	}
}

void CNavmeshAreaDescription::WriteToBuffer( CSimpleBufferWriter& writer )
{
	Super::WriteToBuffer( writer );

	writer.Put( m_bbox.Min.AsVector3() );
	writer.Put( m_bbox.Max.AsVector3() );

	writer.SmartPut( m_navmesh );
	writer.SmartPut( m_neighbourAreas );
	writer.Put( m_ownerComponent );
}

Bool CNavmeshAreaDescription::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !Super::ReadFromBuffer( reader ) )
	{
		return false;
	}

	if ( !reader.Get( m_bbox.Min.AsVector3() ) )
	{
		return false;
	}
	if ( !reader.Get( m_bbox.Max.AsVector3() ) )
	{
		return false;
	}

	if ( !reader.SmartGet( m_navmesh ) )
	{
		return false;
	}

	if ( !reader.SmartGet( m_neighbourAreas ) )
	{
		return false;
	}

	if ( !reader.Get( m_ownerComponent ) )
	{
		return false;
	}

	return true;
}

template < class TQuery >
Bool CNavmeshAreaDescription::LocalSpaceSpatialQuery( TQuery& query ) const
{
	CNavmeshRes* navmesh = m_navmesh.Get();
	if ( navmesh && navmesh->SpatialQuery( query, this ) )
	{
		if ( (query.m_flags & CT_NO_OBSTACLES) != CT_NO_OBSTACLES )
		{
			CObstaclesMap* obstaclesMap = GetObstaclesMap();
			if ( obstaclesMap && !obstaclesMap->TSpatialQuery( query ) )
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

// These instantiations are here as the Clang compiler will strip them out in optimised builds otherwise
template Bool CNavmeshAreaDescription::LocalSpaceSpatialQuery( CCircleQueryData& query ) const;
template Bool CNavmeshAreaDescription::LocalSpaceSpatialQuery( CClosestObstacleCircleQueryData& query ) const;
template Bool CNavmeshAreaDescription::LocalSpaceSpatialQuery( CCollectCollisionPointsInCircleQueryData& query ) const;
template Bool CNavmeshAreaDescription::LocalSpaceSpatialQuery( CLineQueryData& query ) const;
template Bool CNavmeshAreaDescription::LocalSpaceSpatialQuery( CWideLineQueryData& query ) const;
template Bool CNavmeshAreaDescription::LocalSpaceSpatialQuery( CClosestObstacleWideLineQueryData& query ) const;
template Bool CNavmeshAreaDescription::LocalSpaceSpatialQuery( CClearWideLineInDirectionQueryData& query ) const;
template Bool CNavmeshAreaDescription::LocalSpaceSpatialQuery( CCollectGeometryInCirceQueryData& query ) const;
template Bool CNavmeshAreaDescription::LocalSpaceSpatialQuery( CRectangleQueryData& query ) const;
template Bool CNavmeshAreaDescription::LocalSpaceSpatialQuery( CCustomTestQueryData& query ) const;

template < class TQuery >
RED_FORCE_INLINE Bool CNavmeshAreaDescription::SpatialQuery( TQuery& query ) const
{
	return LocalSpaceSpatialQuery( query );
}

// These instantiations are here as the Clang compiler will strip them out in optimised builds otherwise
template Bool CNavmeshAreaDescription::SpatialQuery( CCircleQueryData& query ) const;
template Bool CNavmeshAreaDescription::SpatialQuery( CClosestObstacleCircleQueryData& query ) const;
template Bool CNavmeshAreaDescription::SpatialQuery( CCollectCollisionPointsInCircleQueryData& query ) const;
template Bool CNavmeshAreaDescription::SpatialQuery( CLineQueryData& query ) const;
template Bool CNavmeshAreaDescription::SpatialQuery( CWideLineQueryData& query ) const;
template Bool CNavmeshAreaDescription::SpatialQuery( CClosestObstacleWideLineQueryData& query ) const;
template Bool CNavmeshAreaDescription::SpatialQuery( CClearWideLineInDirectionQueryData& query ) const;
template Bool CNavmeshAreaDescription::SpatialQuery( CCollectGeometryInCirceQueryData& query ) const;
template Bool CNavmeshAreaDescription::SpatialQuery( CRectangleQueryData& query ) const;
template Bool CNavmeshAreaDescription::SpatialQuery( CCustomTestQueryData& query ) const;

Bool CNavmeshAreaDescription::VSpatialQuery( CCircleQueryData& query ) const										{ return SpatialQuery( query ); }
Bool CNavmeshAreaDescription::VSpatialQuery( CClosestObstacleCircleQueryData& query ) const							{ return SpatialQuery( query ); }
Bool CNavmeshAreaDescription::VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query ) const					{ return SpatialQuery( query ); }
Bool CNavmeshAreaDescription::VSpatialQuery( CLineQueryData& query ) const											{ return SpatialQuery( query ); }
Bool CNavmeshAreaDescription::VSpatialQuery( CWideLineQueryData& query ) const										{ return SpatialQuery( query ); }
Bool CNavmeshAreaDescription::VSpatialQuery( CClosestObstacleWideLineQueryData& query ) const						{ return SpatialQuery( query ); }
Bool CNavmeshAreaDescription::VSpatialQuery( CClearWideLineInDirectionQueryData& query ) const						{ return SpatialQuery( query ); }
Bool CNavmeshAreaDescription::VSpatialQuery( CCollectGeometryInCirceQueryData& query ) const						{ return SpatialQuery( query ); }
Bool CNavmeshAreaDescription::VSpatialQuery( CRectangleQueryData& query ) const										{ return SpatialQuery( query ); }
Bool CNavmeshAreaDescription::VSpatialQuery( CCustomTestQueryData& query ) const									{ return SpatialQuery( query ); }

Bool CNavmeshAreaDescription::ComputeHeight( const Vector3& v, Float& z )
{
	CNavmeshRes* navmesh = m_navmesh.Get();
	return navmesh ? m_navmesh->ComputeHeight( v, z ) : false;
}
Bool CNavmeshAreaDescription::ComputeHeight( const Vector2& v, Float minZ, Float maxZ, Float& z )
{
	CNavmeshRes* navmesh = m_navmesh.Get();
	return navmesh ? m_navmesh->ComputeHeight( v, minZ, maxZ, z ) : false;
}
Bool CNavmeshAreaDescription::ComputeHeightFrom(const Vector2& pos, const Vector3& posFrom, Float& outHeight)
{
	CNavmeshRes* navmesh = m_navmesh.Get();
	return navmesh ? m_navmesh->ComputeHeightFrom( pos, posFrom, outHeight ) : false;
}

Bool CNavmeshAreaDescription::VContainsPoint( const Vector3& v ) const
{
	return ContainsPoint( v );
}
Bool CNavmeshAreaDescription::VTestLocation( const Vector3& v1, Uint32 collisionFlags )
{
	return TestLocation( v1, collisionFlags );
}
Bool CNavmeshAreaDescription::VComputeHeight( const Vector3& v, Float& z, Bool smooth )
{
	CNavmeshRes* navmesh = m_navmesh.Get();
	return navmesh ? m_navmesh->ComputeHeight( v, z ) : false;
}
Bool CNavmeshAreaDescription::VComputeHeight( const Vector2& v, Float minZ, Float maxZ, Float& z, Bool smooth )
{
	CNavmeshRes* navmesh = m_navmesh.Get();
	return navmesh ? m_navmesh->ComputeHeight( v, minZ, maxZ, z ) : false;
}
Bool CNavmeshAreaDescription::VComputeHeightFrom( const Vector2& pos, const Vector3& posFrom, Float& outHeight, Bool smooth )
{
	CNavmeshRes* navmesh = m_navmesh.Get();
	return navmesh ? m_navmesh->ComputeHeightFrom( pos, posFrom, outHeight ) : false;
}
Bool CNavmeshAreaDescription::VComputeAverageHeight( const Box& bbox, Float& zAverage, Float& zMin, Float& zMax )
{
	CNavmeshRes* navmesh = m_navmesh.Get();
	return navmesh ? m_navmesh->ComputeAverageHeight( bbox, zAverage, zMin, zMax ) : false;
}
Bool CNavmeshAreaDescription::ContainsPoint( const Vector3& v ) const
{
	// boundings test
	if ( !m_bbox.Contains( v ) )
	{
		return false;
	}
	CNavmeshRes* navmesh = m_navmesh.Get();
	return navmesh ? navmesh->GetTriangle( v ) != CNavmesh::INVALID_INDEX : false;
}
Bool CNavmeshAreaDescription::TestLocation(const Vector3& v1, Uint32 collisionFlags )
{
	ASSERT( !m_useTransformation );
	CNavmeshRes* navmesh = m_navmesh.Get();
	return navmesh ? m_navmesh->GetTriangle( v1 ) != CNavmesh::INVALID_INDEX : false;
}
////////////////////////////////////////////////////////////////////////////
// CNavmeshTransformedAreaDescription
////////////////////////////////////////////////////////////////////////////
void CNavmeshTransformedAreaDescription::OnNavmeshUpdated()
{
	Vector2 testMin = m_bbox.Min.AsVector2();
	Vector2 testMax = m_bbox.Max.AsVector2();
	// calculate exact navmesh
	CNavmeshRes* navmesh = m_navmesh.Get();
	if ( navmesh )
	{
		if ( fabs( m_yaw ) < NumericLimits< Float >::Epsilon() )
		{
			m_bbox = navmesh->GetBoundingBox() + Vector( m_position );
		}
		else
		{
			Box bbox( Box::RESET_STATE );
			for ( CNavmesh::VertexIndex i = 0, n = navmesh->GetVertexesCount(); i < n; ++i )
			{
				bbox.AddPoint( navmesh->GetVertex( i ) );
			}
			m_bbox = bbox;
		}
		testMin.Set( Min( testMin.X, m_bbox.Min.X ), Min( testMin.Y, m_bbox.Min.Y ) );
		testMax.Set( Max( testMax.X, m_bbox.Max.X ), Max( testMax.Y, m_bbox.Max.Y ) );
	}

	m_pathlib->UpdateInstancesMarking( testMin, testMax );
}
void CNavmeshTransformedAreaDescription::NoticeEngineComponent( CNavmeshComponent* component )
{
	ASSERT( component->IsNavmeshUsingTransformation() );

	Matrix mat;
	component->GetWorldToLocal( mat );

	m_position = mat.GetTranslation();
	m_yaw = DEG2RAD(mat.GetYaw());
	m_yawSin = sinf( m_yaw );
	m_yawCos = cosf( m_yaw );

	CNavmeshAreaDescription::NoticeEngineComponent( component );
}
void CNavmeshTransformedAreaDescription::WriteToBuffer( CSimpleBufferWriter& writer )
{
	Super::WriteToBuffer( writer );

	writer.Put( m_position );
	writer.Put( m_yaw );
}
Bool CNavmeshTransformedAreaDescription::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !Super::ReadFromBuffer( reader ) )
		return false;

	if ( !reader.Get( m_position ) )
		return false;

	if ( !reader.Get( m_yaw ) )
		return false;

	m_yawSin = sinf( m_yaw );
	m_yawCos = cosf( m_yaw );

	return true;
}
void CNavmeshTransformedAreaDescription::ComputeTransformation( Matrix& m )
{
	Matrix translation;
	Matrix rotation;
	translation.SetTranslation( m_position.X, m_position.Y, m_position.Z );
	rotation.SetRotZ33( m_yaw );
	m = translation * rotation;
}
void CNavmeshTransformedAreaDescription::VLocalToWorld( Box& v ) const			{ LocalToWorld( v ); }
void CNavmeshTransformedAreaDescription::VWorldToLocal( Box& v ) const			{ WorldToLocal( v ); }
void CNavmeshTransformedAreaDescription::VLocalToWorld( Vector3& v ) const		{ LocalToWorld( v ); }
void CNavmeshTransformedAreaDescription::VWorldToLocal( Vector3& v ) const		{ WorldToLocal( v ); }
void CNavmeshTransformedAreaDescription::VLocalToWorld( Vector2& v ) const		{ LocalToWorld( v ); }
void CNavmeshTransformedAreaDescription::VWorldToLocal( Vector2& v ) const		{ WorldToLocal( v ); }
Float CNavmeshTransformedAreaDescription::VLocalToWorldZ( Float z ) const		{ return LocalToWorldZ( z ); }
Float CNavmeshTransformedAreaDescription::VWorldToLocalZ( Float z ) const		{ return WorldToLocalZ( z ); }


void CNavmeshTransformedAreaDescription::LocalToWorld( Box& v ) const
{
	Vector2 ll = v.Min.AsVector2();
	Vector2 lh = Vector2( v.Min.X, v.Max.Y );
	Vector2 hl = Vector2( v.Max.X, v.Min.Y );
	Vector2 hh = v.Max.AsVector2();

	LocalToWorld( ll );
	LocalToWorld( lh );
	LocalToWorld( hl );
	LocalToWorld( hh );

	v.Min.AsVector2().Set(
		Min( ll.X, lh.X, Min( hl.X, hh.X ) ),
		Min( ll.Y, lh.Y, Min( hl.Y, hh.Y ) )
		);

	v.Max.AsVector2().Set(
		Max( ll.X, lh.X, Max( hl.X, hh.X ) ),
		Max( ll.Y, lh.Y, Max( hl.Y, hh.Y ) )
		);

	v.Min.Z = LocalToWorldZ( v.Min.Z );
	v.Max.Z = LocalToWorldZ( v.Max.Z );
}
void CNavmeshTransformedAreaDescription::WorldToLocal( Box& v ) const
{
	Vector2 ll = v.Min.AsVector2();
	Vector2 lh = Vector2( v.Min.X, v.Max.Y );
	Vector2 hl = Vector2( v.Max.X, v.Min.Y );
	Vector2 hh = v.Max.AsVector2();

	WorldToLocal( ll );
	WorldToLocal( lh );
	WorldToLocal( hl );
	WorldToLocal( hh );

	v.Min.AsVector2().Set(
		Min( ll.X, lh.X, Min( hl.X, hh.X ) ),
		Min( ll.Y, lh.Y, Min( hl.Y, hh.Y ) )
		);

	v.Max.AsVector2().Set(
		Max( ll.X, lh.X, Max( hl.X, hh.X ) ),
		Max( ll.Y, lh.Y, Max( hl.Y, hh.Y ) )
		);

	v.Min.Z = WorldToLocalZ( v.Min.Z );
	v.Max.Z = WorldToLocalZ( v.Max.Z );
}
void CNavmeshTransformedAreaDescription::LocalToWorld( Vector3& v ) const
{
	v.AsVector2() = MathUtils::GeometryUtils::Rotate2D( v.AsVector2(), -m_yawSin, m_yawCos );
	v += m_position;
}
void CNavmeshTransformedAreaDescription::WorldToLocal( Vector3& v ) const
{
	v -= m_position;
	v.AsVector2() = MathUtils::GeometryUtils::Rotate2D( v.AsVector2(), m_yawSin, m_yawCos );
}
void CNavmeshTransformedAreaDescription::LocalToWorld( Vector2& v ) const
{
	v = MathUtils::GeometryUtils::Rotate2D( v, -m_yawSin, m_yawCos ) + m_position.AsVector2();
}
void CNavmeshTransformedAreaDescription::WorldToLocal( Vector2& v ) const
{
	v = MathUtils::GeometryUtils::Rotate2D( v - m_position.AsVector2(), m_yawSin, m_yawCos );
}

template < class TQuery >
Bool CNavmeshTransformedAreaDescription::SpatialQuery( TQuery& query ) const
{
	query.Transform( *this );
	Bool ret = true;
	CNavmeshRes* navmesh = m_navmesh.Get();
	if ( !navmesh || !m_navmesh->SpatialQuery( query, this ) )
	{
		ret = false;
	}
	else
	{
		if ( (query.m_flags & CT_NO_OBSTACLES) != CT_NO_OBSTACLES )
		{
			CObstaclesMap* obstaclesMap = GetObstaclesMap();
			if ( obstaclesMap && !obstaclesMap->TSpatialQuery( query ) )
			{
				ret = false;
			}
		}
	}
	query.CancelTransformOutput( *this );
	return ret;
}

template Bool CNavmeshTransformedAreaDescription::SpatialQuery( CCircleQueryData& query ) const;
template Bool CNavmeshTransformedAreaDescription::SpatialQuery( CClosestObstacleCircleQueryData& query ) const;
template Bool CNavmeshTransformedAreaDescription::SpatialQuery( CCollectCollisionPointsInCircleQueryData& query ) const;
template Bool CNavmeshTransformedAreaDescription::SpatialQuery( CLineQueryData& query ) const;
template Bool CNavmeshTransformedAreaDescription::SpatialQuery( CWideLineQueryData& query ) const;
template Bool CNavmeshTransformedAreaDescription::SpatialQuery( CClosestObstacleWideLineQueryData& query ) const;
template Bool CNavmeshTransformedAreaDescription::SpatialQuery( CClearWideLineInDirectionQueryData& query ) const;
template Bool CNavmeshTransformedAreaDescription::SpatialQuery( CCollectGeometryInCirceQueryData& query ) const;
template Bool CNavmeshTransformedAreaDescription::SpatialQuery( CRectangleQueryData& query ) const;
template Bool CNavmeshTransformedAreaDescription::SpatialQuery( CCustomTestQueryData& query ) const;

Bool CNavmeshTransformedAreaDescription::VSpatialQuery( CCircleQueryData& query ) const										{ return SpatialQuery( query ); }
Bool CNavmeshTransformedAreaDescription::VSpatialQuery( CClosestObstacleCircleQueryData& query ) const						{ return SpatialQuery( query ); }
Bool CNavmeshTransformedAreaDescription::VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query ) const					{ return SpatialQuery( query ); }
Bool CNavmeshTransformedAreaDescription::VSpatialQuery( CLineQueryData& query ) const										{ return SpatialQuery( query ); }
Bool CNavmeshTransformedAreaDescription::VSpatialQuery( CWideLineQueryData& query ) const									{ return SpatialQuery( query ); }
Bool CNavmeshTransformedAreaDescription::VSpatialQuery( CClosestObstacleWideLineQueryData& query ) const					{ return SpatialQuery( query ); }
Bool CNavmeshTransformedAreaDescription::VSpatialQuery( CClearWideLineInDirectionQueryData& query ) const					{ return SpatialQuery( query ); }
Bool CNavmeshTransformedAreaDescription::VSpatialQuery( CCollectGeometryInCirceQueryData& query ) const						{ return SpatialQuery( query ); }
Bool CNavmeshTransformedAreaDescription::VSpatialQuery( CRectangleQueryData& query ) const									{ return SpatialQuery( query ); }
Bool CNavmeshTransformedAreaDescription::VSpatialQuery( CCustomTestQueryData& query ) const									{ return SpatialQuery( query ); }

Bool CNavmeshTransformedAreaDescription::ComputeHeight( const Vector3& v, Float& z )
{
	CNavmeshRes* navmesh = m_navmesh.Get();
	if ( navmesh )
	{
		Vector3 local = v;
		WorldToLocal( local );
		if ( navmesh->ComputeHeight( local, z ) )
		{
			z += m_position.Z;
			return true;
		}
	}
	
	return false;
}
Bool CNavmeshTransformedAreaDescription::ComputeHeight( const Vector2& v, Float minZ, Float maxZ, Float& z )
{
	CNavmeshRes* navmesh = m_navmesh.Get();
	if ( navmesh )
	{
		Vector2 local = v;
		minZ = WorldToLocalZ( m_position.Z );
		maxZ = WorldToLocalZ( m_position.Z );
		WorldToLocal( local );
		if ( navmesh->ComputeHeight( local, minZ, maxZ, z ) )
		{
			z = LocalToWorldZ( z );
			return true;
		}
	}
	
	return false;
}
Bool CNavmeshTransformedAreaDescription::ComputeHeightFrom( const Vector2& pos, const Vector3& posFrom, Float& outHeight )
{
	CNavmeshRes* navmesh = m_navmesh.Get();
	if ( navmesh )
	{
		Vector2 localPos = pos;
		Vector3 localPosFrom = posFrom;
		WorldToLocal( localPos );
		WorldToLocal( localPosFrom );
		if( navmesh->ComputeHeightFrom( localPos, localPosFrom, outHeight ) )
		{
			outHeight = LocalToWorldZ( outHeight );
			return true;
		}
	}
	return false;
}

Bool CNavmeshTransformedAreaDescription::VContainsPoint( const Vector3& v ) const
{
	return ContainsPoint( v );
}
Bool CNavmeshTransformedAreaDescription::VTestLocation( const Vector3& v1, Uint32 collisionFlags )
{
	return TestLocation( v1, collisionFlags );
}
Bool CNavmeshTransformedAreaDescription::VComputeHeight( const Vector3& v, Float& z, Bool smooth )
{
	return ComputeHeight( v, z );
}
Bool CNavmeshTransformedAreaDescription::VComputeHeight( const Vector2& v, Float minZ, Float maxZ, Float& z, Bool smooth )
{
	return ComputeHeight( v, minZ, maxZ, z );
}
Bool CNavmeshTransformedAreaDescription::VComputeHeightFrom( const Vector2& pos, const Vector3& posFrom, Float& outHeight, Bool smooth )
{
	return ComputeHeightFrom( pos, posFrom, outHeight );
}
Bool CNavmeshTransformedAreaDescription::VComputeAverageHeight( const Box& bbox, Float& zAverage, Float& zMin, Float& zMax )
{
	CNavmeshRes* navmesh = m_navmesh.Get();
	if ( navmesh )
	{
		Box transformedBox;

		Vector2 corners[ 4 ];
		corners[ 0 ] = bbox.Min.AsVector2();
		corners[ 1 ] = Vector2( bbox.Max.X, bbox.Min.Y );
		corners[ 2 ] = bbox.Max.AsVector2();
		corners[ 3 ] = Vector2( bbox.Min.X, bbox.Max.Y );

		WorldToLocal( corners[ 0 ] );
		WorldToLocal( corners[ 1 ] );
		WorldToLocal( corners[ 2 ] );
		WorldToLocal( corners[ 3 ] );

		transformedBox.Min.X = Min( corners[ 0 ].X, Min( corners[ 1 ].X, Min( corners[ 2 ].X, corners[ 3 ].X ) ) );
		transformedBox.Max.X = Max( corners[ 0 ].X, Max( corners[ 1 ].X, Max( corners[ 2 ].X, corners[ 3 ].X ) ) );
		transformedBox.Min.Y = Min( corners[ 0 ].Y, Min( corners[ 1 ].Y, Min( corners[ 2 ].Y, corners[ 3 ].Y ) ) );
		transformedBox.Max.Y = Max( corners[ 0 ].Y, Max( corners[ 1 ].Y, Max( corners[ 2 ].Y, corners[ 3 ].Y ) ) );
		transformedBox.Min.Z = bbox.Min.Z + m_position.Z;
		transformedBox.Max.Z = bbox.Max.Z + m_position.Z;

		if ( navmesh->ComputeAverageHeight( bbox, zAverage, zMin, zMax ) )
		{
			zAverage += m_position.Z;
			zMin += m_position.Z;
			zMax += m_position.Z;
			return true;
		}
	}
	return false;
}
Bool CNavmeshTransformedAreaDescription::ContainsPoint( const Vector3& v ) const
{
	CNavmesh* navmesh = m_navmesh.Get();
	if ( !navmesh )
	{
		return false;
	}
	Vector3 local = v;
	WorldToLocal( local );

	// boundings test
	if ( !navmesh->GetBoundingBox().Contains( local ) )
	{
		return false;
	}
	// detailed test
	return navmesh->GetTriangle( local ) != CNavmesh::INVALID_INDEX;
}

Bool CNavmeshTransformedAreaDescription::TestLocation(const Vector3& v1, Uint32 collisionFlags )
{
	CNavmesh* navmesh = m_navmesh.Get();
	if ( !navmesh )
	{
		return false;
	}
	Vector3 localV1 = v1;
	WorldToLocal( localV1 );
	if ( navmesh->GetTriangle( localV1 ) == CNavmesh::INVALID_INDEX )
	{
		return false;
	}
	if ( (collisionFlags & CT_NO_OBSTACLES) != CT_NO_OBSTACLES )
	{
		CObstaclesMap* obstaclesMap = GetObstaclesMap();
		if ( obstaclesMap && !obstaclesMap->TestLocation( localV1 ) )
		{
			return false;
		}
	}

	return true;
}


};				// namespace PathLib

