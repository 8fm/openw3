/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibObstaclesMap.h"

#include "pathlibNavmeshArea.h"
#include "pathlibObstacleDetour.h"
#include "pathlibObstacleMapper.h"
#include "pathlibObstacleShape.h"
#include "pathlibSimpleBuffers.h"
#include "pathlibWorld.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// CObstaclesMap::SmartReaderLock
////////////////////////////////////////////////////////////////////////////
RED_INLINE CObstaclesMap::SmartReaderLock::SmartReaderLock( const CObstaclesMap* obstaclesMap )
{
	m_map = obstaclesMap;
	m_map->m_readersCount.Increment();
	if ( m_map->m_isModified.GetValue() == false )
	{
		// common, lockless read
		m_type = SmartReaderLock::LOCKLESS_READ;
	}
	else
	{
		// possibly rare mutex-based read
		m_type = SmartReaderLock::FULL_LOCK;
		// immediately drop readers count (as we will wait on mutex)
		m_map->m_readersCount.Decrement();
		m_map->m_modificationMutex.Acquire();

	}

	// NOTICE: We ignore area->IsReady() case as that one is: 1) rare 2) actually checking 'IsReady' is touching other atomics so its better to run lightweight lockless read mechanism
}

RED_INLINE CObstaclesMap::SmartReaderLock::~SmartReaderLock()
{
	if ( m_type == SmartReaderLock::LOCKLESS_READ )
	{
		// release readers counter
		m_map->m_readersCount.Decrement();
	}
	else
	{
		m_map->m_modificationMutex.Release();
	}
}


////////////////////////////////////////////////////////////////////////////
// CObstaclesMap::SmartWriterLock
////////////////////////////////////////////////////////////////////////////
RED_INLINE CObstaclesMap::SmartWriterLock::SmartWriterLock( CObstaclesMap* obstaclesMap )
{
	if ( obstaclesMap->m_area->IsReady() )
	{
		m_type = FULL_LOCK;
		m_map = obstaclesMap;
		obstaclesMap->m_isModified.SetValue( true );
		while ( obstaclesMap->m_readersCount.GetValue() > 0 )
		{
			Red::Threads::YieldCurrentThread();
		}
		obstaclesMap->m_modificationMutex.Acquire();
	}
	else
	{
		m_type = NO_LOCK;
	}
}

RED_INLINE CObstaclesMap::SmartWriterLock::~SmartWriterLock()
{
	if ( m_type == FULL_LOCK )
	{
		m_map->m_isModified.SetValue( false );
		m_map->m_modificationMutex.Release();
	}
}

////////////////////////////////////////////////////////////////////////////
// CObstaclesMap::ObstaclesIterator
////////////////////////////////////////////////////////////////////////////
void CObstaclesMap::ObstaclesIterator::CelEntered()
{
	Uint32 celIndex = m_map->GetCelIndex( m_x, m_y );
	const auto& cel = m_map->m_obstaclesMap[ celIndex ];
	m_itCel = cel.Begin();
	m_endCel = cel.End();
}
void CObstaclesMap::ObstaclesIterator::Progress()
{
	for ( ;; )
	{
		while ( m_itCel == m_endCel )
		{
			if ( ++m_x > m_maxX )
			{
				m_x = m_minX;
				if ( ++m_y > m_maxY )
				{
					// iterator is finished
					m_inProgress = false;
					return;
				}
			}
			CelEntered();
		}
		CObstacle* obstacle = *m_itCel;
		// check if obstacle was tested b4
		if ( 
			(m_x != m_minX && obstacle->m_xmin != m_x) ||
			(m_y != m_minY && obstacle->m_ymin != m_y)
			)
		{
			++m_itCel;
			continue;
		}

		break;
	}
}
CObstaclesMap::ObstaclesIterator::ObstaclesIterator( CObstaclesMap* map, const Vector2& bboxMin, const Vector2& bboxMax )
	: m_map( map )
	, m_inProgress( true )
{
	map->TranslateWorld2Cel( bboxMin, m_minX, m_minY );
	map->TranslateWorld2Cel( bboxMax, m_maxX, m_maxY );
	m_x = m_minX;
	m_y = m_minY;
	CelEntered();
	Progress();
}

////////////////////////////////////////////////////////////////////////////
// CObstaclesMap
////////////////////////////////////////////////////////////////////////////

CObstaclesMap::CObstaclesMap()
	: CNavModyficationMap()
	, m_cellsX( 0 )
	, m_cellsY( 0 )
	, m_celSizeInverted( 1.f )
	, m_nextObstacleId( 1 )
	, m_isModified( false )
	, m_readersCount( 0 )
{
	m_areaLocalBBox[ 0 ].Set( 0, 0, 0 ); m_areaLocalBBox[ 1 ].Set( 0, 0, 0 );
}

CObstaclesMap::~CObstaclesMap()
{
	for( auto it = m_obstacles.Begin(), end = m_obstacles.End(); it != end; ++it )
	{
		delete it->m_second;
	}
	m_obstacles.ClearFast();
}

Bool CObstaclesMap::Add2CollisionMap( CObstacle* obstacle )
{
	CObstacleShape* shape = obstacle->GetShape();
	if ( !shape )
	{
		return false;
	}
	TranslateWorld2Cel( shape->GetBBoxMin().AsVector2(), obstacle->m_xmin, obstacle->m_ymin );
	TranslateWorld2Cel( shape->GetBBoxMax().AsVector2(), obstacle->m_xmax, obstacle->m_ymax );

	SmartWriterLock lock( this );

	for ( Uint16 x = obstacle->m_xmin; x <= obstacle->m_xmax; ++x )
	{
		for ( Uint16 y = obstacle->m_ymin; y <= obstacle->m_ymax; ++y )
		{
			Uint32 cel = GetCelIndex( x,y );
			ASSERT( ::Find( m_obstaclesMap[ cel ].Begin(), m_obstaclesMap[ cel ].End(), obstacle ) == m_obstaclesMap[ cel ].End() );
			m_obstaclesMap[ cel ].PushBack( obstacle );
		}
	}
	return true;
}
void CObstaclesMap::RemoveFromCollisionMap( CObstacle* obstacle )
{
	CObstacleShape* shape = obstacle->GetShape();
	if ( !shape )
	{
		return;
	}

	SmartWriterLock lock( this );

	for ( Uint16 x = obstacle->m_xmin; x <= obstacle->m_xmax; ++x )
	{
		for ( Uint16 y = obstacle->m_ymin; y <= obstacle->m_ymax; ++y )
		{
			tCollisionCell& cel = m_obstaclesMap[ GetCelIndex( x,y ) ];
			cel.RemoveFast( obstacle );
		}
	}
}


CObstacle::Id CObstaclesMap::AddObstacle( CObstacle* obstacle, const CObstacleSpawnData& spawnData )
{
	Bool isPersistant = obstacle->IsPersistant();
	CObstacle::Id id = m_nextObstacleId++;
	if ( isPersistant )
	{
		id |= CObstacle::MASK_IS_PERSISTANT;
	}
	obstacle->m_id = id;
	m_obstacles.Insert( id, obstacle );

	obstacle->UpdateAreasInside( m_area );
	obstacle->Initialize( this, spawnData );
	obstacle->OnAddition( this );

	MarkVersionDirty();

	return id;
}
CObstacle* CObstaclesMap::GetObstacle( CObstacle::Id id ) const
{
	auto itFind = m_obstacles.Find( id );
	if ( itFind == m_obstacles.End() )
		return NULL;
	return itFind->m_second;
}
Bool CObstaclesMap::RemoveObstacle( CObstacle::Id id )
{
	auto itFind = m_obstacles.Find( id );
	ASSERT( itFind != m_obstacles.End() );
	if ( itFind == m_obstacles.End() )
		return false;

	CObstacle* obstacle = itFind->m_second;
	obstacle->OnRemoval( this );
	
	m_obstacles.Erase( itFind );
	delete obstacle;

	MarkVersionDirty();

	return true;
}

Bool CObstaclesMap::ShowObstacle( CObstacle::Id id, CComponentRuntimeProcessingContext& context )
{
	auto itFind = m_obstacles.Find( id );
	if ( itFind == m_obstacles.End() )
		return false;

	CObstacle* obstacle = itFind->m_second;
	obstacle->OnShow( m_area, context );

	MarkVersionDirty();

	return true;
}
Bool CObstaclesMap::HideObstacle( CObstacle::Id id, CComponentRuntimeProcessingContext& context )
{
	auto itFind = m_obstacles.Find( id );
	if ( itFind == m_obstacles.End() )
		return false;

	CObstacle* obstacle = itFind->m_second;
	obstacle->OnHide( m_area, context );

	MarkVersionDirty();

	return true;
}
void CObstaclesMap::OnGraphClearance( CNavGraph* navgraph )
{
	for ( auto it = m_obstacles.Begin(), end = m_obstacles.End(); it != end; ++it )
	{
		CObstacle* obstacle = it->m_second;
		obstacle->OnGraphClearance( navgraph );
	}
}
void CObstaclesMap::PostGraphGeneration( CNavGraph* navgraph )
{
	// first attach obstacles to base pathfinding data
	for ( auto it = m_obstacles.Begin(), end = m_obstacles.End(); it != end; ++it )
	{
		CObstacle* obstacle = it->m_second;
		obstacle->PostGraphGeneration( this, navgraph );
	}

	m_groups.PostGraphGeneration( navgraph );
}

void CObstaclesMap::GenerateDetourInfo( CObstaclesDetourInfo& detourInfo, Float personalSpace, Bool computeZ )
{
	for ( auto it = m_obstacles.Begin(), end = m_obstacles.End(); it != end; ++it )
	{
		CObstacle* obstacle = it->m_second;
		if ( !obstacle->IsPersistant() )
		{
			continue;
		}
		CObstacleShape* shape = obstacle->GetShape();
		if ( !shape )
		{
			continue;
		}
		ObstacleDetour detour;
		shape->VComputeDetour( m_area, personalSpace, detour );

		if ( !detour.Empty() )
		{
			detourInfo.Push( detour );
		}
	}
}

void CObstaclesMap::DepopulateArea( Bool runNotyfications )
{
	DepopulateArea( runNotyfications, false );
	DepopulateArea( runNotyfications, true );
}
void CObstaclesMap::DepopulateArea( Bool runNotyfications, Bool persistant )
{
	for ( auto it = m_obstacles.Begin(), end = m_obstacles.End(); it != end; ++it )
	{
		CObstacle* obstacle = it->m_second;
		if ( obstacle->IsPersistant() != persistant )
			continue;
		obstacle->OnRemoval( this );
	}
}
void CObstaclesMap::RepopulateArea( Bool runNotyfications )
{
	// firstly add persistant occluders
	RepopulateArea( runNotyfications, true );						
	RepopulateArea( runNotyfications, false );
}
void CObstaclesMap::RepopulateArea( Bool runNotyfications, Bool persistant )
{
	for ( auto it = m_obstacles.Begin(), end = m_obstacles.End(); it != end; ++it )
	{
		CObstacle* obstacle = it->m_second;
		if ( obstacle->IsPersistant() != persistant )
			continue;
		obstacle->OnAddition( this );
	}
}

void CObstaclesMap::Initialize()
{
	m_areaLocalBBox[ 0 ] = m_area->GetBBox().Min.AsVector3();
	m_areaLocalBBox[ 1 ] = m_area->GetBBox().Max.AsVector3();
	m_area->VWorldToLocal( m_areaLocalBBox[ 0 ] );
	m_area->VWorldToLocal( m_areaLocalBBox[ 1 ] );

	const Float optimalCelSize = 8.f;
	Vector2 areaSize = m_areaLocalBBox[ 1 ].AsVector2() - m_areaLocalBBox[ 0 ].AsVector2();
	if ( areaSize.X < NumericLimits< Float >::Epsilon() || areaSize.Y < NumericLimits< Float >::Epsilon() )
		return;
	Uint32 optimalCellsX = Uint32( 0.5f + areaSize.X / optimalCelSize );
	Uint32 optimalCellsY = Uint32( 0.5f + areaSize.Y / optimalCelSize );
	Uint32 desiredCellsX = Clamp( optimalCellsX, Uint32(1), Uint32(64) );
	Uint32 desiredCellsY = Clamp( optimalCellsY, Uint32(1), Uint32(64) );

	Float desiredCelSizeX = areaSize.X / Float( desiredCellsX );
	Float desiredCelSizeY = areaSize.Y / Float( desiredCellsY );

	Float celSize = Max( desiredCelSizeX, desiredCelSizeY );
	m_celSizeInverted = 1.f / celSize;

	m_cellsX = Uint16( ceil(areaSize.X * m_celSizeInverted - 0.1f) );
	m_cellsY = Uint16( ceil(areaSize.Y * m_celSizeInverted - 0.1f) );

	m_obstaclesMap.Resize( m_cellsX*m_cellsY );
}

void CObstaclesMap::Shutdown()
{
	for( auto it = m_obstacles.Begin(), end = m_obstacles.End(); it != end; ++it )
	{
		delete it->m_second;
	}
	m_obstacles.ClearFast();
	m_obstaclesMap.ClearFast();
}

Bool CObstaclesMap::MarkInstance( CNavmeshAreaDescription* area )
{
	Bool dirty = false;

	Box localBoundingBox = area->GetBBox();

	m_area->VWorldToLocal( localBoundingBox );

	ObstaclesIterator it( this, localBoundingBox.Min.AsVector2(), localBoundingBox.Max.AsVector2() );

	while ( it )
	{
		CObstacle* obstacle = *it;
		if ( obstacle->MarkArea( localBoundingBox, area, m_area ) )
		{
			dirty = true;
		}

		++it;
	}

	if ( dirty )
	{
		MarkVersionDirty();
		m_area->MarkDirty( CAreaDescription::DIRTY_SAVE );
		return true;
	}
	return false;
}
Bool CObstaclesMap::UpdateObstaclesAreaMarking( const Vector2& localMin, const Vector2& localMax )
{
	ObstaclesIterator it( this, localMin, localMax );

	while ( it )
	{
		CObstacle* obstacle = *it;
		CObstacleShape* shape = obstacle->GetShape();

		++it;

		if ( !shape->TestBoundings( localMin, localMax ) )
			continue;

		obstacle->UpdateAreasInside( m_area );


	}

	MarkVersionDirty();
	m_area->MarkDirty( CAreaDescription::DIRTY_SAVE );
	return true;
}


void CObstaclesMap::SerializeToDataBuffer( TDynArray< Int8 >& buffer ) const
{
	CSimpleBufferWriter writer( buffer, RES_VERSION );
	WriteToBuffer( writer );
}
CObstaclesMap* CObstaclesMap::SertializeFromDataBuffer( const TDynArray< Int8 >& buffer, CAreaDescription* area )
{
	CObstaclesMap* obstaclesMap = new CObstaclesMap();
	CSimpleBufferReader reader( buffer );
	if ( !obstaclesMap->ReadFromBuffer( reader ) )
	{
		delete obstaclesMap;
		return NULL;
	}
	obstaclesMap->OnPostLoad( area );
	return obstaclesMap;
}

Bool CObstaclesMap::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( reader.GetVersion() != RES_VERSION )
	{
		return false;
	}

	if ( !Super::ReadFromBuffer( reader ) )
	{
		return false;
	}

	Uint16 obstaclesCount;
	if ( !reader.Get( obstaclesCount ) )
	{
		return false;
	}

	for( Uint16 i = 0; i < obstaclesCount; ++i )
	{
		CObstacle::Id id;
		if ( !reader.Get( id ) )
		{
			return false;
		}
		m_nextObstacleId = Max( m_nextObstacleId, ( id & CObstacle::MASK_INDEX ) + 1 );
		CObstacle* obstacle = CObstacle::NewFromBuffer( reader );
		if ( !obstacle )
		{
			return false;
		}
		obstacle->m_id = id;
		m_obstacles.Insert( id, obstacle );
	}

	if ( !m_groups.ReadFromBuffer( reader ) )
	{
		return false;
	}

	return true;
}
void CObstaclesMap::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	Super::WriteToBuffer( writer );

	Uint16 obstaclesCount = Uint16(m_obstacles.Size());
	writer.Put( obstaclesCount );

	for( auto it = m_obstacles.Begin(), end = m_obstacles.End(); it != end; ++it )
	{
		writer.Put( it->m_first );
		it->m_second->WriteToBuffer( writer );
	}

	m_groups.WriteToBuffer( writer );
}

CObstacleShape* CObstaclesMap::NewShape( TDynArray< Vector2 >&& convexHull, const Vector3& bbmin, const Vector3& bbmax ) const
{
	ASSERT( !convexHull.Empty() );

	CObstacleShape* shape = NULL;

	if ( convexHull.Size() == 1 )
	{
		// create tiny box shape
		CObstacleShapeBox* box = new CObstacleShapeBox();
		box->InitializeBBox( bbmin - Vector3( 0.1f, 0.1f, 0.f ), bbmax + Vector3( 0.1f, 0.1f, 0.f ) );
		shape = box;
	}
	else 
	{
		if ( convexHull.Size() == 2 )
		{
			CObstacleShapeLineSegment* line = new CObstacleShapeLineSegment();
			line->m_v1 = convexHull[ 0 ];
			line->m_v2 = convexHull[ 1 ];
			shape = line;
		}
		else
		{
			CObstacleShapePoly* poly = new CObstacleShapePoly();
			poly->m_verts = Move( convexHull );
			shape = poly;
		}
		shape->InitializeBBox( bbmin, bbmax );
	}

	return shape;
}
CObstacleShape* CObstaclesMap::NewCylinderShape( const Vector2& cylinderPos, Float radius, const Vector3& bbmin, const Vector3& bbmax ) const
{
	CObstacleShapeCircle* circle = new CObstacleShapeCircle();
	circle->InitializeBBox( bbmin, bbmax );
	circle->m_center = cylinderPos;
	circle->m_radius = radius;
	return circle;
}
CObstacleShape* CObstaclesMap::ComputeShape( CObstacleSpawnContext& context )
{
	Float minZ = FLT_MAX;
	Float maxZ = -FLT_MAX;

	m_area->VPrecomputeObstacleSpawnData( context );

	const CObstacleSpawnData& data = context.Data();

	Float maxNaviZ = -FLT_MAX;
	TDynArray< CObstacleShape* > shapes;


	TDynArray< Vector2 > input;
	for ( auto it = data.m_shapes.Begin(), end = data.m_shapes.End(); it != end; ++it )
	{
		const CObstacleShapeGeometryData* shapeData = *it;

		switch ( shapeData->m_type )
		{
		case CObstacleShapeGeometryData::T_INDVERTS:
			{
				const CObstacleIndicedVertsGeometryData& shape = static_cast< const CObstacleIndicedVertsGeometryData& >( *shapeData );

				Box obstacleBBox( Box::RESET_STATE );
				for ( Uint32 i = 0, n = shape.m_verts.Size(); i < n; ++i )
				{
					obstacleBBox.AddPoint( shape.m_verts[ i ] );
				}
				obstacleBBox.Min.Z -= DEFAULT_AGENT_HEIGHT;

				Float minShapeZ;
				Float maxShapeZ;
				Float defaultZ;
				if ( !m_area->VComputeAverageHeight( obstacleBBox, defaultZ, minShapeZ, maxShapeZ ) )
				{
					// shape doesn't touch walkable surfaces so we don't care
					continue;
				}

				maxNaviZ = Max( maxNaviZ, maxShapeZ );

				for ( Uint32 i = 0, n = shape.m_indices.Size(); i < n; i += 3 )
				{
					Vector3 verts[3];
					verts[ 0 ] = shape.m_verts[ shape.m_indices[ i + 0 ] ];
					verts[ 1 ] = shape.m_verts[ shape.m_indices[ i + 1 ] ];
					verts[ 2 ] = shape.m_verts[ shape.m_indices[ i + 2 ] ];
					// z check
					Float triangleZ = defaultZ;
					Vector3 triCenter = (verts[ 0 ] + verts[ 1 ] + verts[ 2 ]) / 3.f;
					m_area->VComputeHeight( triCenter.AsVector2(), minShapeZ - GEOMETRY_AND_NAVMESH_MAX_DISTANCE, maxShapeZ + GEOMETRY_AND_NAVMESH_MAX_DISTANCE, triangleZ );
					// TODO: triangleZ computation
					Float minTriZ = Min( Min( verts[ 0 ].Z, verts[ 1 ].Z ), verts[ 2 ].Z );
					Float maxTriZ = Max( Max( verts[ 0 ].Z, verts[ 1 ].Z ), verts[ 2 ].Z );
					if ( triangleZ < minTriZ - DEFAULT_AGENT_HEIGHT || triangleZ > maxTriZ + IGNORE_SHORT_MESHES_Z )		// ignore short meshes TODO: Its discussable functionality and solution.
					{
						// triangle is out of z-bounds
						continue;
					}
					input.PushBack( verts[ 0 ].AsVector2() );
					input.PushBack( verts[ 1 ].AsVector2() );
					input.PushBack( verts[ 2 ].AsVector2() );

					minZ = Min( minTriZ, minZ );
					maxZ = Max( maxTriZ, maxZ );
				}

				maxZ = Min( maxZ + DEFAULT_AGENT_HEIGHT, maxNaviZ + DEFAULT_AGENT_HEIGHT );

				if ( input.Empty() )
				{
					break;
				}

				Vector3 bboxMin( FLT_MAX, FLT_MAX, m_area->VWorldToLocalZ( minZ ) );
				Vector3 bboxMax( -FLT_MAX, -FLT_MAX, m_area->VWorldToLocalZ( maxZ ) );
				TDynArray< Vector2 > convexHull;
				MathUtils::GeometryUtils::ComputeConvexHull2D( input, convexHull );
				input.ClearFast();

				// transfer poly shape to local coordinates system
				for ( auto it = convexHull.Begin(), end = convexHull.End(); it != end; ++it )
				{
					Vector2& v = *it;
					m_area->VWorldToLocal( v );
					bboxMin.AsVector2().Set(
						Min( bboxMin.X, v.X ),
						Min( bboxMin.Y, v.Y )
						);
					bboxMax.AsVector2().Set(
						Max( bboxMax.X, v.X ),
						Max( bboxMax.Y, v.Y )
						);
				}

				CObstacleShape* newShape = NewShape( Move( convexHull ), bboxMin, bboxMax );

				shapes.PushBack( newShape );
			}
			break;
		case CObstacleShapeGeometryData::T_PURE_CONVEX:
			{
				const CObstacleConvexOccluderData& shape = static_cast< const CObstacleConvexOccluderData& >( *shapeData );

				// TODO: Support recomputation of bbox for unprecise bbox transformations
				Box bbox = shape.m_shapeBBox;
				m_area->VWorldToLocal( bbox );

				Vector3 bboxMin( bbox.Min.AsVector3() );
				Vector3 bboxMax( bbox.Max.AsVector3() );
				TDynArray< Vector2 > convexHull = shape.m_occluder;
				for ( Uint32 i = 0, n = convexHull.Size(); i != n; ++i )
				{
					m_area->VWorldToLocal( convexHull[ i ] );
				}

				CObstacleShape* newShape = NewShape( Move( convexHull ), bboxMin, bboxMax );

				shapes.PushBack( newShape );
			}
			break;
		case CObstacleShapeGeometryData::T_CYLINDER:
			{
				const CObstacleCylinderData& cylinder = static_cast< const CObstacleCylinderData& >( *shapeData );

				// TODO: Support recomputation of bbox for unprecise bbox transformations
				Box bbox = cylinder.m_shapeBBox;

				Vector2 worldCylinderPos = cylinder.m_pos;

				m_area->VWorldToLocal( bbox );
				m_area->VWorldToLocal( worldCylinderPos );

				Vector3 bboxMin( bbox.Min.AsVector3() );
				Vector3 bboxMax( bbox.Max.AsVector3() );

				CObstacleShape* newShape = NewCylinderShape( worldCylinderPos, cylinder.m_radius, bboxMin, bboxMax );

				shapes.PushBack( newShape );
			}
			break;
		default:
			ASSERT( false );
			ASSUME( false );
		}


	}

	if ( shapes.Empty() )
	{
		return NULL;
	}
	if ( shapes.Size() == 1 )
	{
		return shapes[ 0 ];
	}
	else
	{
		CObstacleShapeComposite* compositeShape = new CObstacleShapeComposite();
		compositeShape->m_subShapes = Move( shapes );
		compositeShape->ComputeBBox();
		return compositeShape;
	}
}

CObstacle* CObstaclesMap::CreateObstacle( const CObstacleSpawnData& data )
{
	Bool isPersistant = true;
	Uint32 specialObstacleFlags = 0;
	switch ( data.m_collisionType )
	{
	default:
	case PLC_Disabled:
	case PLC_Walkable:
		return nullptr;
	case PLC_Immediate:
		if ( m_area->GetPathLib().IsCooking() )
		{
			return nullptr;
		}
		isPersistant = false;
		break;
	case PLC_Dynamic:
		if ( data.m_isOnHiddeableLayer )
		{
			isPersistant = false;
		}
		break;
	case PLC_StaticWalkable:
	case PLC_Static:
		// this obstacles blocks only terrain areas
		if ( m_area->IsNavmeshArea() )
		{
			return NULL;
		}
		break;
	case PLC_StaticMetaobstacle:
		specialObstacleFlags |= CObstacle::IS_METAOBSTACLE;
		break;
	}

	CObstacleSpawnContext context( data );
	CObstacleShape* obstacleShape = ComputeShape( context );
	if ( isPersistant && obstacleShape )
	{
		SimplifyShape( obstacleShape );		// NOTICE: it can possibly nullify the obstacleShape pointer
	}
	if ( !obstacleShape )
	{
		return nullptr;
	}
	CObstacle* obstacle = 
		isPersistant
		? static_cast< CObstacle* >( new CPersistantObstacle( obstacleShape ) )
		: data.m_collisionType == PLC_Immediate
		? static_cast< CObstacle* >( new CDynamicImmediateObstacle( obstacleShape ) )
		: data.m_isLayerBasedGrouping
		? static_cast< CObstacle* >( new CDynamicGroupedObstacle( obstacleShape ) )
		: static_cast< CObstacle* >( new CDynamicPregeneratedObstacle( obstacleShape ) );

	obstacle->AddFlags( specialObstacleFlags );
	obstacle->SetMapping( data.m_mapping, data.m_layerMapping );

	return obstacle;
}

Bool CObstaclesMap::UpdateObstacleShape( const CObstacleSpawnData& data, CObstacle* obstacle )
{
	CObstacleSpawnContext context( data );

	CObstacleShape* currentShape = obstacle->GetShape();
	CObstacleShape* newShape = ComputeShape( context );

	if ( !currentShape || !newShape )
	{
		if ( !currentShape && !newShape )
		{
			return false;
		}

		if ( newShape )
		{
			// detached obstacle has just got new shape
			obstacle->ChangeShape( newShape );
			obstacle->OnAddition( this );
			obstacle->UpdateAreasInside( m_area );
		}
		else // currentShape
		{
			RemoveFromCollisionMap( obstacle );
			obstacle->ClearShape();
		}
		MarkVersionDirty();

		return true;
	}

	if ( !newShape->CompareShape( currentShape ) )
	{
		obstacle->OnRemoval( this );
		obstacle->ChangeShape( newShape );
		obstacle->OnAddition( this );
		obstacle->UpdateAreasInside( m_area );
		MarkVersionDirty();
		return true;
	}

	return false;
}

Bool CObstaclesMap::ObstacleTypeChanged( EPathLibCollision obstacleType, CObstacle* obstacle )
{
	switch ( obstacleType )
	{
	default:
	case PLC_Disabled:
	case PLC_Walkable:
	case PLC_Immediate:
		return obstacle != NULL;
	case PLC_Dynamic:
		return !obstacle || obstacle->IsPersistant();
	case PLC_StaticWalkable:
		// this obstacles blocks only terrain areas
		if ( m_area->IsNavmeshArea() )
		{
			return obstacle != NULL;
		}
	case PLC_Static:
	case PLC_StaticMetaobstacle:
		return !obstacle || !obstacle->IsPersistant();
	}
}

Bool CObstaclesMap::SimplifyObstacles()
{
	TDynArray< CObstacle* > obstaclesToRemove;

	for ( Uint16 y = 0; y < m_cellsY; ++y )
	{
		for ( Uint16 x = 0; x < m_cellsX; ++x )
		{
			tCollisionCell& collisionCell = m_obstaclesMap[ GetCelIndex( x, y ) ];
			for ( auto itCel = collisionCell.Begin(), endCel = collisionCell.End(); itCel != endCel; ++itCel )
			{
				CObstacle* obstacle1 = *itCel;
				CObstacleShape* shape1 = obstacle1->GetShape();
				for ( auto itCel2 = itCel+1; itCel2 != endCel; ++itCel2 )
				{
					CObstacle* obstacle2 = *itCel2;
					// check if this shapes was tested be4
					if ( (obstacle1->m_xmin < x && obstacle2->m_xmin < x) || (obstacle1->m_ymin < y && obstacle2->m_ymin < y) )
					{
						continue;
					}

					CObstacleShape* shape2 = obstacle2->GetShape();

					// mutual containment tests
					if ( shape1->Contains( shape2 ) )
					{
						obstaclesToRemove.PushBack( obstacle2 );
					}
					else if ( shape2->Contains( shape1 ) )
					{
						obstaclesToRemove.PushBack( obstacle1 );
					}
				}
			}
		}
	}
	if ( !obstaclesToRemove.Empty() )
	{
		for ( auto it = obstaclesToRemove.Begin(); it != obstaclesToRemove.End(); ++it )
		{
			CObstacle* obstacle = *it;
			obstacle->OnRemoval( this );
			obstacle->ClearShape();
		}
		return true;
	}
	return false;
}

Bool CObstaclesMap::SimplifyShape( CObstacleShape*& shape )
{
	TDynArray< CObstacle* > obstaclesToRemove;

	ObstaclesIterator it( this, shape->GetBBoxMin().AsVector2(), shape->GetBBoxMax().AsVector2() );

	while ( it )
	{
		CObstacle* obstacle2 = *it;

		++it;

		if ( !obstacle2->IsPersistant() )
		{
			continue;
		}

		CObstacleShape* shape2 = obstacle2->GetShape();

		// mutual containment tests
		if ( shape->Contains( shape2 ) )
		{
			// destroy shape
			obstaclesToRemove.PushBack( obstacle2 );
		}
		else if ( shape2->Contains( shape ) )
		{
			delete shape;
			shape = NULL;
			return true;
		}
	}

	if ( !obstaclesToRemove.Empty() )
	{
		for ( auto it = obstaclesToRemove.Begin(); it != obstaclesToRemove.End(); ++it )
		{
			CObstacle* obstacle = *it;
			obstacle->UnmarkCollision( this );
			obstacle->ClearShape();
		}
		return true;
	}
	return false;
}

Bool CObstaclesMap::TestLocation( const Vector3& v ) const
{
	Uint16 x,y;
	TranslateWorld2Cel( v.AsVector2(), x, y );
	const tCollisionCell& cel = m_obstaclesMap[ GetCelIndex( x, y ) ];

	SmartReaderLock lock( this );

	for ( auto it = cel.Begin(), end = cel.End(); it != end; ++it )
	{
		CObstacle* obstacle = (*it);
		CObstacleShape* shape = obstacle->GetShape();

		// include bbox check
		if ( shape->TestBoundings( v ) && !shape->VTestLocation( v ) )
		{
			return false;
		}
	}
	return true;
}

template < class TQuery >
Bool CObstaclesMap::TSpatialQuery( TQuery& query ) const
{
	Vector3 bbox[2];
	query.ComputeBBox( bbox );

	Uint32 forbiddedObstacleFlags = CObstacle::EMPTY_FLAGS;
	if ( query.m_flags & CT_NO_DYNAMIC_OBSTACLES )
	{
		forbiddedObstacleFlags |= CObstacle::IS_DYNAMIC;
	}
	if ( query.m_flags & CT_NO_PERSISTANT_OBSTACLES )
	{
		forbiddedObstacleFlags |= CObstacle::IS_PERSISTANT;
	}
	if ( query.m_flags & CT_IGNORE_METAOBSTACLE )
	{
		forbiddedObstacleFlags |= CObstacle::IS_METAOBSTACLE;
	}

	Uint16 xmin, ymin, xmax, ymax;
	TranslateWorld2Cel( bbox[ 0 ].AsVector2(), xmin, ymin );
	TranslateWorld2Cel( bbox[ 1 ].AsVector2(), xmax, ymax );

	SmartReaderLock lock( this );

	for ( Uint16 x = xmin; x <= xmax; ++x )
	{
		for ( Uint16 y = ymin; y <= ymax; ++y )
		{
			const tCollisionCell& cel = m_obstaclesMap[ GetCelIndex( x, y ) ];
			for ( auto it = cel.Begin(), end = cel.End(); it != end; ++it )
			{
				CObstacle* obstacle = *it;

				// check if obstacle was tested b4
				if ( ( x > xmin && obstacle->m_xmin < x ) || ( y > ymin && obstacle->m_ymin < y ) )
				{
					continue;
				}

				if ( obstacle->GetFlags() & forbiddedObstacleFlags )
				{
					continue;
				}

				// TODO: check if obstacle is in ignored group.
				if ( !obstacle->TestFlags( query ) )
				{
					continue;
				}

				CObstacleShape* shape = obstacle->GetShape();

				// include bbox check
				if ( shape->TestBoundings( bbox[ 0 ], bbox[ 1 ] ) && !shape->VSpatialQuery( query, bbox ) )
				{
					return false;
				}
			}
		}
	}
	return true;
}

// template instantiation
template Bool CObstaclesMap::TSpatialQuery< CCircleQueryData >( CCircleQueryData& query ) const;
template Bool CObstaclesMap::TSpatialQuery< CClosestObstacleCircleQueryData >( CClosestObstacleCircleQueryData& query ) const;
template Bool CObstaclesMap::TSpatialQuery< CCollectCollisionPointsInCircleQueryData >( CCollectCollisionPointsInCircleQueryData& query ) const;
template Bool CObstaclesMap::TSpatialQuery< CCollectGeometryInCirceQueryData >( CCollectGeometryInCirceQueryData& query ) const;
template Bool CObstaclesMap::TSpatialQuery< CLineQueryData >( CLineQueryData& query ) const;
template Bool CObstaclesMap::TSpatialQuery< CWideLineQueryData >( CWideLineQueryData& query ) const;
template Bool CObstaclesMap::TSpatialQuery< CClosestObstacleWideLineQueryData >( CClosestObstacleWideLineQueryData& query ) const;
template Bool CObstaclesMap::TSpatialQuery< CClearWideLineInDirectionQueryData >( CClearWideLineInDirectionQueryData& query ) const;
template Bool CObstaclesMap::TSpatialQuery< CRectangleQueryData >( CRectangleQueryData& query ) const;
template Bool CObstaclesMap::TSpatialQuery< CCustomTestQueryData >( CCustomTestQueryData& query ) const;


void CObstaclesMap::OnPreLoad( CAreaDescription* area )
{
	Super::OnPreLoad( area );
}
void CObstaclesMap::OnPostLoad( CAreaDescription* area )
{
	CPathLibWorld& pathlib = m_area->GetPathLib();
	CComponentRuntimeProcessingContext& context = pathlib.GetObstaclesMapper()->GetComponentProcessingContext();
	context.BeginRequestsCollection( pathlib );

	Super::OnPostLoad( context );

	Initialize();

	for ( auto it = m_obstacles.Begin(), end = m_obstacles.End(); it != end; ++it )
	{
		CObstacle* obstacle = it->m_second;

		obstacle->OnPostLoad( m_area, context );
	}

	m_groups.OnPostLoad( this, area, context );

	context.EndRequestsCollection( pathlib );
}

void CObstaclesMap::OnPreUnload( CAreaDescription* area )
{
	Super::OnPreUnload( area );

	for ( auto it = m_obstacles.Begin(), end = m_obstacles.End(); it != end; ++it )
	{
		CObstacle* obstacle = it->m_second;

		obstacle->OnPreUnload( area );
	}
}

Bool CObstaclesMap::Load( const String& depotPath )
{
	return CAreaRes::Load( this, depotPath );
}

Bool CObstaclesMap::Save( const String& depotPath ) const
{
	return CAreaRes::Save( this, depotPath );
}
Bool CObstaclesMap::VHasChanged() const
{
	return !IsInitialVersion();
}
Bool CObstaclesMap::VSave( const String& depotPath ) const
{
	return Save( depotPath );
}
void CObstaclesMap::VOnPreLoad( CAreaDescription* area )
{
	OnPreLoad( area );
}
Bool CObstaclesMap::VLoad( const String& depotPath, CAreaDescription* area )
{
	return Load( depotPath );
}
void CObstaclesMap::VOnPostLoad( CAreaDescription* area )
{
	return OnPostLoad( area );
}
void CObstaclesMap::VOnPreUnload( CAreaDescription* area )
{
	OnPreUnload( area );
}
const Char* CObstaclesMap::VGetFileExtension() const
{
	return GetFileExtension();
}
ENavResType CObstaclesMap::VGetResType() const
{
	return GetResType();
}

};			// namespace PathLib


