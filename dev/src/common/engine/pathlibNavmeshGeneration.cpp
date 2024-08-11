/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibNavmeshGeneration.h"

#include "../core/heap.h"
#include "../core/factory.h"
#include "../core/feedback.h"

#include "baseTree.h"
#include "collisionMesh.h"
#include "collisionShape.h"
#include "collisionShapeBuilder.h"
#include "component.h"
#include "clipMap.h"
#include "deniedAreaComponent.h"
#include "entity.h"
#include "foliageEditionController.h"
#include "game.h"
#include "layer.h"
#include "layerInfo.h"
#include "layerGroup.h"
#include "mesh.h"
#include "meshDataBuilder.h"
#include "pathlibNavmesh.h"
#include "pathlibNavmeshBorderAreaComponent.h"
#include "pathlibWorld.h"
#include "renderer.h"
#include "terrainTile.h"
#include "world.h"



#ifndef NO_NAVMESH_GENERATION

#ifdef _DEBUG
	#ifndef RECAST_LIB_PATH
		#define RECAST_LIB_PATH "..\\..\\..\\external\\recast\\lib\\Debug\\"
	#endif
#else
	#ifndef RECAST_LIB_PATH
		#define RECAST_LIB_PATH "..\\..\\..\\external\\recast\\lib\\Release\\"
	#endif
#endif

#if _MSC_VER == 1700
	#ifdef _WIN64
		#pragma comment(lib, RECAST_LIB_PATH "RecastVC110_x64.lib")
	#else
		#pragma comment(lib, RECAST_LIB_PATH "RecastVC110.lib")
	#endif
#elif _MSC_VER == 1600
	#ifdef _WIN64
		#pragma comment(lib, RECAST_LIB_PATH "Recast_x64.lib")
	#else
		#pragma comment(lib, RECAST_LIB_PATH "Recast.lib")
	#endif
#else
#error Unsupported compiler
#endif

#include "mesh.h"
#include "Recast.h"

namespace Recast
{

static RED_INLINE void EngineVertexToRecast( const Vector3& vec, Float* dst )
{
	*dst++ = vec.X;
	*dst++ = vec.Z;
	*dst++ = vec.Y;
}

static RED_INLINE void RecastVertexToEngine( const Float* dst, Vector3& outVec )
{
	outVec.X = dst[ 0 ];
	outVec.Z = dst[ 1 ];
	outVec.Y = dst[ 2 ];
}



const Float CGenerator::GENROOT_AND_NAVMESH_MAX_DISTANCE = 1.5f;

////////////////////////////////////////////////////////////////////////////

CGenerationInputData::CGenerationInputData( CNavmeshComponent::InputIterator* inputIterator, CNavmeshComponent* component )
	: m_params( component->m_navmeshParams )
	, m_bbox( component->GetBoundingBox() )
	, m_boundingPoly( component->GetWorldPoints() )
	, m_defaultGenerationRoots( component->m_generationRootPoints )
	, m_localToWorld( component->GetLocalToWorld() )
	, m_inputIterator( inputIterator )
	, m_navmeshComponent( component )
{
}

CGenerationInputData::~CGenerationInputData()
{
	if ( m_inputIterator )
	{
		delete m_inputIterator;
	}
}

////////////////////////////////////////////////////////////////////////////
// CGenerationProcessingData
CGenerationProcessingData::CGenerationProcessingData( CGenerationInputData& input )
	: m_params( input.m_params )
{
	if ( m_params.m_useGenerationRootPoints )
	{
		m_generationRoots.Resize( input.m_defaultGenerationRoots.Size() );
		const Vector& translation = input.m_localToWorld.GetTranslationRef();
		for ( Uint32 i = 0, n = m_generationRoots.Size(); i != n; ++i )
		{
			Vector redPos = input.m_defaultGenerationRoots[ i ] + translation;
			EngineVertexToRecast( redPos.AsVector3(), m_generationRoots[ i ].A );
		}
	}
}

////////////////////////////////////////////////////////////////////////////
// CGenerator

CGenerator::CGenerator( CGenerationInputData& input )
	: CGenerationProcessingData( input )
	, m_currentGenerationTask( ENavGen_Initialize )
{

}

Bool CGenerator::CollectDeniedArea( CDeniedAreaComponent* da )
{
	return CollectComponent( da->GetCollisionType() );
}
Bool CGenerator::DeniedAreaMeshIsWalkable( CDeniedAreaComponent* da )
{
	return ComponentIsWalkable( da->GetCollisionType() );
}

Bool CGenerator::CollectStaticMesh( CStaticMeshComponent* staticMesh )
{
	EPathLibCollision navCollision = staticMesh->GetPathLibCollisionType();
	if ( navCollision == PLC_Dynamic )
	{
		CLayer* layer = staticMesh->GetLayer();
		if ( !layer->GetLayerInfo()->GetLayerGroup()->CanBeHidden() )
		{
			return true;
		}
		return false;
	}
	return CollectComponent( navCollision );
}

Bool CGenerator::VerifyInput( CGenerationInputData& input )
{
	if ( input.m_params.m_vertsPerPoly < 3 || input.m_params.m_vertsPerPoly > 32 )
	{
		NoticeError( TXT("Invalid vertsPerPoly generation param!\n") );
		return false;
	}
	
	if ( input.m_params.m_cellWidth < NumericLimits< Float >::Epsilon() || input.m_params.m_cellHeight < NumericLimits< Float >::Epsilon() )
	{
		NoticeError( TXT("Invalid cellWidth and(or) cellHeight generation params!\n") );
		return false;
	}

	return true;
}

Bool CGenerator::CollectMeshes( CGenerationInputData& input, TDynArray< CStaticMeshComponent* >& outMeshes, TDynArray< CDeniedAreaComponent* >& outDeniedAreas,  TDynArray< CNavmeshBorderAreaComponent* >& outBorderAreas )
{
	// Collect components
	for ( CNode* component; (component = input.m_inputIterator->Get()) != NULL; input.m_inputIterator->Next() )
	{
		// A mesh with collision
		if ( component->IsA< CStaticMeshComponent >() )
		{
			CStaticMeshComponent* smc = static_cast< CStaticMeshComponent* >( component );
			smc->OnUpdateBounds();

			// Mesh should not have disabled PE collision
			if ( CollectStaticMesh( smc ) )
			{
				outMeshes.PushBack( smc );
			}
		}
		else if ( component->IsA< CDeniedAreaComponent >() )
		{
			CDeniedAreaComponent* deniedAreaComponent = static_cast< CDeniedAreaComponent* >( component );

			if ( CollectDeniedArea( deniedAreaComponent ) )
			{
				outDeniedAreas.PushBack( deniedAreaComponent );
			}
		}
		else if ( component->IsA< CNavmeshBorderAreaComponent >() )
		{
			CNavmeshBorderAreaComponent* borderAreaComponent = static_cast< CNavmeshBorderAreaComponent* >( component );
			if ( borderAreaComponent->IsEffectingNavmesh( input.m_navmeshComponent ) )
			{
				outBorderAreas.PushBack( borderAreaComponent );
			}
		}
		else if ( m_params.m_useGenerationRootPoints && component->IsA< CNavmeshGenerationRootComponent >() )
		{
			m_generationRoots.PushBack( ToRecastPosition( component->GetWorldPosition().AsVector3() ) );
		}	
	}
	return !outMeshes.Empty() || !outDeniedAreas.Empty();
}

Bool CGenerator::CollectShapeGeometry( CGenerationInputData& input, const TDynArray< Vector >& vertices, const TDynArray< Uint32 >& indices, Bool walkable, Bool detectExtension )
{
	Bool collectedStuff = false;
	if ( !walkable )
	{
		Uint32 baseVer = m_unwalkableVertsInput.Size();
		m_unwalkableVertsInput.Grow( vertices.Size()*3 );

		for ( Uint32 i = 0, n = vertices.Size(); i < n; ++i )
		{
			Uint32 verticeBase = i*3;
			EngineVertexToRecast( vertices[ i ].AsVector3(), &m_unwalkableVertsInput[ baseVer+i*3 ] );
		}

		Uint32 baseTri = m_unwalkableTrisInput.Size();
		m_unwalkableTrisInput.Grow( indices.Size() );

		Uint32 baseVerIdx = baseVer/3;

		for ( Uint32 i = 0, n = indices.Size(); i < n; i += 3 )
		{
			m_unwalkableTrisInput[ baseTri+i+0 ] = indices[ i+0 ] + baseVerIdx;
			m_unwalkableTrisInput[ baseTri+i+1 ] = indices[ i+2 ] + baseVerIdx;
			m_unwalkableTrisInput[ baseTri+i+2 ] = indices[ i+1 ] + baseVerIdx;
		}

		collectedStuff = vertices.Size() > 0 && indices.Size() > 0;
	}
	else
	{
		if ( detectExtension )
		{
			// TODO: Could be optimized a lot! It produces a lot more vertices (don't use indices).
			for ( Uint32 i = 0, n = indices.Size(); i < n; i += 3 )
			{
				const Vector& v0 = vertices[ indices[ i+0 ] ];
				const Vector& v1 = vertices[ indices[ i+1 ] ];
				const Vector& v2 = vertices[ indices[ i+2 ] ];

				Bool b0 = MathUtils::GeometryUtils::IsPointInPolygon2D( input.m_boundingPoly.TypedData(), input.m_boundingPoly.Size(), v0.AsVector2() );
				Bool b1 = MathUtils::GeometryUtils::IsPointInPolygon2D( input.m_boundingPoly.TypedData(), input.m_boundingPoly.Size(), v1.AsVector2() );
				Bool b2 = MathUtils::GeometryUtils::IsPointInPolygon2D( input.m_boundingPoly.TypedData(), input.m_boundingPoly.Size(), v2.AsVector2() );

				TDynArray< Float >* vertsInput = NULL;
				TDynArray< Int32 >* trisInput = NULL;

				if ( b0 && b1 && b2 )
				{
					// everything inside - add usual walkable triangle
					vertsInput = &m_vertsInput;
					trisInput = &m_trisInput;
				}
				else
				{
					Bool collectTri = false;
					if (  b0 || b1 || b2 )
					{
						collectTri = true;
					}
					else
					{
						Vector2 tmpshit;
						Vector2 tmpshit2;

						Float maxRange =  input.m_params.m_extensionLength;
						Float maxRangeSq = maxRange*maxRange;

						if (
							MathUtils::GeometryUtils::ClosestPointPolygonLine2D( input.m_boundingPoly.TypedData(), input.m_boundingPoly.Size(), v0.AsVector2(), v1.AsVector2(), maxRange, tmpshit, tmpshit2 ) < maxRangeSq ||
							MathUtils::GeometryUtils::ClosestPointPolygonLine2D( input.m_boundingPoly.TypedData(), input.m_boundingPoly.Size(), v1.AsVector2(), v2.AsVector2(), maxRange, tmpshit, tmpshit2 ) < maxRangeSq ||
							MathUtils::GeometryUtils::ClosestPointPolygonLine2D( input.m_boundingPoly.TypedData(), input.m_boundingPoly.Size(), v2.AsVector2(), v0.AsVector2(), maxRange, tmpshit, tmpshit2 ) < maxRangeSq
							)
						{
							collectTri = true;
						}
					}

					if ( collectTri )
					{
						vertsInput = &m_vertsExtensionInput;
						trisInput = &m_trisExtensionInput;
					}
				}

				if ( vertsInput )
				{
					collectedStuff = true;

					Uint32 baseVer = (*vertsInput).Size();
					(*vertsInput).Grow( 9 );
					EngineVertexToRecast( v0.AsVector3(), &(*vertsInput)[ baseVer+0 ] );
					EngineVertexToRecast( v1.AsVector3(), &(*vertsInput)[ baseVer+3 ] );
					EngineVertexToRecast( v2.AsVector3(), &(*vertsInput)[ baseVer+6 ] );

					Uint32 baseVerIdx = baseVer/3;
					Uint32 baseTri = (*trisInput).Size();
					(*trisInput).Grow( 3 );
					(*trisInput)[ baseTri+0 ] = baseVerIdx + 0;
					(*trisInput)[ baseTri+1 ] = baseVerIdx + 2;
					(*trisInput)[ baseTri+2 ] = baseVerIdx + 1;
				}
			}
		}
		else
		{
			Uint32 baseVer = m_vertsInput.Size();
			m_vertsInput.Grow( vertices.Size()*3 );

			for ( Uint32 i = 0, n = vertices.Size(); i < n; ++i )
			{
				Uint32 verticeBase = i*3;
				EngineVertexToRecast( vertices[ i ].AsVector3(), &m_vertsInput[ baseVer+i*3 ] );
			}

			Uint32 baseTri = m_trisInput.Size();
			m_trisInput.Grow( indices.Size() );

			Uint32 baseVerIdx = baseVer/3;

			for ( Uint32 i = 0, n = indices.Size(); i < n; i += 3 )
			{
				m_trisInput[ baseTri+i+0 ] = indices[ i+0 ] + baseVerIdx;
				m_trisInput[ baseTri+i+1 ] = indices[ i+2 ] + baseVerIdx;
				m_trisInput[ baseTri+i+2 ] = indices[ i+1 ] + baseVerIdx;
			}

			collectedStuff = vertices.Size() > 0 && indices.Size() > 0;
		}
	}
	return collectedStuff;
}


Bool CGenerator::CollectMeshGeometry( CGenerationInputData& input, const TDynArray< CStaticMeshComponent* >& meshes, const TDynArray< CDeniedAreaComponent* >& deniedAreas )
{

	TDynArray< Vector > vertices;
	TDynArray< Uint32 > indices;

	if( m_params.m_useCollisionMeshes )
	{
		for( Uint32 i = 0; i < meshes.Size(); ++i )
		{
			CStaticMeshComponent* staticMesh = meshes[ i ];
			const CMesh* mesh = staticMesh->GetMeshNow();
			const CCollisionMesh* cm = mesh ? staticMesh->GetMeshNow()->GetCollisionMesh() : NULL;
			if( cm )
			{
				Bool walkable = StaticMeshIsWalkable( staticMesh );

				Bool detectExtension = walkable && input.m_params.m_cutMeshesWithBoundings;
				if ( detectExtension )
				{
					const Box& bbox = staticMesh->GetBoundingBox();
					detectExtension = !MathUtils::GeometryUtils::TestPolygonContainsRectangle2D( input.m_boundingPoly.TypedData(), input.m_boundingPoly.Size(), bbox.Min.AsVector2(), bbox.Max.AsVector2() );
				}

				const auto& shapeList = cm->GetShapes();
				for ( auto it = shapeList.Begin(), end = shapeList.End(); it != end; ++it )
				{
					const ICollisionShape* shape = *it;
					vertices.ClearFast();
					indices.ClearFast();
					shape->GetShapeForNavigation( staticMesh->GetLocalToWorld(), vertices, indices );
					CollectShapeGeometry( input, vertices, indices, walkable, detectExtension );
				}
			}
		}
	}
	else
	{
		for( Uint32 i = 0; i < meshes.Size(); ++i )
		{
			CStaticMeshComponent* staticMesh = meshes[ i ];
			const CMesh* mesh = staticMesh->GetMeshNow();
			if ( mesh )
			{
				Bool walkable = StaticMeshIsWalkable( staticMesh );
				Bool detectExtension = walkable && input.m_params.m_cutMeshesWithBoundings;
				if ( detectExtension )
				{
					const Box& bbox = staticMesh->GetBoundingBox();
					detectExtension = !MathUtils::GeometryUtils::TestPolygonContainsRectangle2D( input.m_boundingPoly.TypedData(), input.m_boundingPoly.Size(), bbox.Min.AsVector2(), bbox.Max.AsVector2() );
				}
				const Matrix& localToWorld = staticMesh->GetLocalToWorld();

				const CMeshData data( mesh );
				const auto& chunks = data.GetChunks();
				for( auto itr = chunks.Begin(), end = chunks.End(); itr != end; ++itr )
				{
					// collection doesn't have to be optimal because its very seldomly used branch, provided as 'fallback mechanism' and for debugging reasons
					const SMeshChunk& chunk = *itr;
					vertices.ResizeFast( chunk.m_vertices.Size() );
					indices.ResizeFast( chunk.m_indices.Size() );
					for ( Uint32 i = 0, n = vertices.Size(); i < n; ++i )
					{
						const SMeshVertex& vex = chunk.m_vertices[ i ];
						Vector worldPosition = localToWorld.TransformPoint( Vector( vex.m_position[0], vex.m_position[1], vex.m_position[2]) );
						vertices[ i ] = worldPosition;
					}
					for ( Uint32 i = 0, n = indices.Size(); i < n; i += 3 )
					{
						indices[ i+0 ] = chunk.m_indices[ i+0 ];
						indices[ i+1 ] = chunk.m_indices[ i+2 ];
						indices[ i+2 ] = chunk.m_indices[ i+1 ];
					}
					CollectShapeGeometry( input, vertices, indices, walkable, detectExtension );
				}
			}
		}
	}
	if ( !deniedAreas.Empty() )
	{
		for ( auto it = deniedAreas.Begin(), end = deniedAreas.End(); it != end; ++it )
		{
			CDeniedAreaComponent* deniedArea = *it;
			Bool walkable = DeniedAreaMeshIsWalkable( deniedArea );
			Bool detectExtension = walkable && input.m_params.m_cutMeshesWithBoundings;
			if ( detectExtension )
			{
				const Box& bbox = deniedArea->GetBoundingBox();
				detectExtension = !MathUtils::GeometryUtils::TestPolygonContainsRectangle2D( input.m_boundingPoly.TypedData(), input.m_boundingPoly.Size(), bbox.Min.AsVector2(), bbox.Max.AsVector2() );
			}
			Bool isAreaConvex = MathUtils::GeometryUtils::IsPolygonConvex2D( deniedArea->GetWorldPoints() );
			vertices.ClearFast();
			indices.ClearFast();
			deniedArea->GenerateTriMesh( vertices, indices, !isAreaConvex );
			::Reverse( indices.Begin(), indices.End() );
			CollectShapeGeometry( input, vertices, indices, walkable, detectExtension );
		}
	}
	return !m_vertsInput.Empty() && !m_trisInput.Empty();
}

Bool CGenerator::CollectTerrainGeometry( CGenerationInputData& input, const Box& bbox, const Vector* boundingPoly, Uint32 boundingPolySize, TDynArray< Float >& vertsInput, TDynArray< Int32 >& trisInput )
{
	CWorld* world = input.m_navmeshComponent->GetLayer()->GetWorld();
	if ( !world )
	{
		return false;
	}
	CClipMap* terrain = world->GetTerrain();
	if ( !terrain )
	{
		NoticeError( TXT("No terrain, cannot collect its geometry!\n") );
		return false;
	}
	Int32 minTileX, minTileY, maxTileX, maxTileY;
	terrain->GetTileFromPosition( bbox.Min, minTileX, minTileY, true );
	terrain->GetTileFromPosition( bbox.Max, maxTileX, maxTileY, true );

	TDynArray< Vector > tileVerts;
	TDynArray< Int32 > tileVertsIndexes;
	LongBitField vertsInRange;

	Uint32 prevTrisSize = trisInput.Size();

	for ( Int32 tileY = minTileY; tileY <= maxTileY; ++tileY )
	{
		for ( Int32 tileX = minTileX; tileX <= maxTileX; ++tileX )
		{
			// prepeare data
			CTerrainTile* tile = terrain->GetTile( tileX, tileY );
			if ( !tile )
			{
				continue;
			}
			/*Uint16* hightMapData =*/ tile->GetLevelSyncHM( 0 );
			Box tileBoundings = terrain->GetBoxForTile( tileX, tileY, 0.f );

			// compute terrain vertexes
			Int32 minX, minY, maxX, maxY;
			tile->GetTerrainGeometryVertexes( bbox, terrain, tileBoundings.Min, minX, minY, maxX, maxY, tileVerts );

			Uint32 vertsCount = tileVerts.Size();
			
			tileVertsIndexes.ResizeFast( vertsCount );
			vertsInRange.ResizeFast( vertsCount );
			vertsInput.Reserve( vertsInput.Size() + vertsCount * 3 );

			Uint32 spanX = maxX - minX + 1;
			Uint32 spanY = maxY - minY + 1;

			// check if vertexes are in generation boundings
			for ( Uint32 i = 0; i != vertsCount; ++i )
			{
				Bool inRange = false;
				if ( tileVerts[ i ].Z >= input.m_bbox.Min.Z && tileVerts[ i ].Z <= input.m_bbox.Max.Z )
				{
					inRange = MathUtils::GeometryUtils::IsPointInPolygon2D( boundingPoly, boundingPolySize, tileVerts[ i ].AsVector2() );
				}
				vertsInRange.SetBit( i, inRange );
			}

			// push vertexes and mark disabled (by control map) ones
			for ( Uint32 i = 0; i != vertsCount; ++i )
			{
				Bool isVertUsed;
				// additional test if vertex is not disabled by control map
				if ( tileVerts[ i ].Z >= (FLT_MAX - 1.f) )
				{
					isVertUsed = false;
				}
				else
				{
					isVertUsed = true;

					if ( !vertsInRange.IsBitSet( i ) )						// if current vertex is out of boundings
					{
						Uint32 x = i % spanX;
						Bool upAvailable = (i >= spanX);
						Bool downAvailable = (i < vertsCount - spanX);
						Bool leftAvailable = (x > 0);
						Bool rightAvailable = (x < spanX - 1);
						if (
							( !leftAvailable || !vertsInRange.IsBitSet(i-1) ) &&
							( !rightAvailable || !vertsInRange.IsBitSet(i+1) ) &&
							( !upAvailable || !leftAvailable || !vertsInRange.IsBitSet(i-spanX-1) ) &&
							( !upAvailable || !vertsInRange.IsBitSet(i-spanX) ) &&
							( !upAvailable || !rightAvailable || !vertsInRange.IsBitSet(i-spanX+1) ) &&
							( !downAvailable || !leftAvailable || !vertsInRange.IsBitSet(i+spanX-1) ) &&
							( !downAvailable || !vertsInRange.IsBitSet(i+spanX) ) &&
							( !downAvailable || !rightAvailable || !vertsInRange.IsBitSet(i+spanX+1) )
							)
						{
							isVertUsed = false;
						}
					}
				}
				if ( isVertUsed )
				{
					Uint32 vertStartIndex = vertsInput.Size();
					vertsInput.Grow( 3 );
					EngineVertexToRecast( tileVerts[ i ].AsVector3(), &vertsInput[ vertStartIndex ] );
					tileVertsIndexes[ i ] = vertStartIndex / 3;
				}
				else
				{
					tileVertsIndexes[ i ] = -1;
				}
			}

			

			auto funTriangleIndex =
				[ minX, minY, spanX ] ( Int32 x, Int32 y ) -> Uint32
			{
				return spanX * (y - minY) + (x - minX);
			};

			Uint32 trianglesCount = spanX * spanY * 2;
			trisInput.Reserve( trisInput.Size() + trianglesCount );

			for ( Int32 y = minY; y < maxY; ++y )
			{
				for ( Int32 x = minX; x < maxX; ++x )
				{
					Uint32 indexes1[3];
					Uint32 indexes2[3];

					if ( tile->IsQuadTriangulatedNW2SE( x, y ) )
					{
						indexes1[ 0 ] = funTriangleIndex( x+0, y+1 );
						indexes1[ 1 ] = funTriangleIndex( x+1, y+1 );
						indexes1[ 2 ] = funTriangleIndex( x+0, y+0 );

						indexes2[ 0 ] = funTriangleIndex( x+1, y+0 );
						indexes2[ 1 ] = funTriangleIndex( x+0, y+0 );
						indexes2[ 2 ] = funTriangleIndex( x+1, y+1 );
					}
					else
					{
						indexes1[ 0 ] = funTriangleIndex( x+1, y+1 );
						indexes1[ 1 ] = funTriangleIndex( x+1, y+0 );
						indexes1[ 2 ] = funTriangleIndex( x+0, y+1 );

						indexes2[ 0 ] = funTriangleIndex( x+0, y+0 );
						indexes2[ 1 ] = funTriangleIndex( x+0, y+1 );
						indexes2[ 2 ] = funTriangleIndex( x+1, y+0 );
					}

					if ( vertsInRange.IsBitSet( indexes1[ 0 ] ) || vertsInRange.IsBitSet( indexes1[ 1 ] ) || vertsInRange.IsBitSet( indexes1[ 2 ] ) )
					{
						Uint32 ind1 = tileVertsIndexes[ indexes1[ 0 ] ];
						Uint32 ind2 = tileVertsIndexes[ indexes1[ 1 ] ];
						Uint32 ind3 = tileVertsIndexes[ indexes1[ 2 ] ];
						if ( ind1 != -1 && ind2 != -1 && ind3 != -1 )
						{
							trisInput.PushBack( ind1 );
							trisInput.PushBack( ind2 );
							trisInput.PushBack( ind3 );
						}
					}

					if ( vertsInRange.IsBitSet( indexes2[ 0 ] ) || vertsInRange.IsBitSet( indexes2[ 1 ] ) || vertsInRange.IsBitSet( indexes2[ 2 ] ) )
					{
						Uint32 ind1 = tileVertsIndexes[ indexes2[ 0 ] ];
						Uint32 ind2 = tileVertsIndexes[ indexes2[ 1 ] ];
						Uint32 ind3 = tileVertsIndexes[ indexes2[ 2 ] ];
						if ( ind1 != -1 && ind2 != -1 && ind3 != -1 )
						{
							trisInput.PushBack( ind1 );
							trisInput.PushBack( ind2 );
							trisInput.PushBack( ind3 );
						}
					}
				}
			}

			// process triangles
			tileVertsIndexes.ClearFast();
			vertsInRange.ResizeFast( 0 );
			tileVerts.ClearFast();
		}
	}

	return prevTrisSize != trisInput.Size();
}

Bool CGenerator::CollectTerrainGeometry( CGenerationInputData& input, Bool collectForExtension )
{
	CWorld* world = GGame->GetActiveWorld();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
	if ( !pathlib )
	{
		return false;
	}
	// determine collection input data
	Box							boundingBox;
	const Vector*				boundingPoly;
	Uint32						boundingPolySize;
	TDynArray< Float >*			vertsInput;
	TDynArray< Int32 >*			trisInput;
	
	TDynArray< Vector >			extendedPoly;

	if ( collectForExtension )
	{
		Float extensionLength = input.m_params.m_extensionLength;
		boundingBox = Box( Box::RESET_STATE );

		boundingBox.Min.Z = input.m_bbox.Min.Z - 1.f;
		boundingBox.Max.Z = input.m_bbox.Max.Z + 1.f;

		const auto& baseBoundings = input.m_boundingPoly;

		// check if bounding verts are orientated clockwise
		Float polyArea = MathUtils::GeometryUtils::GetClockwisePolygonArea2D( baseBoundings.TypedData(), baseBoundings.Size() );
		Bool isCounterClockwise = polyArea < 0.f;

		// compute extended boundings
		const Vector* lastPos = &baseBoundings.Back();
		for ( Uint32 i = 0, n = baseBoundings.Size(); i < n; lastPos = &baseBoundings[ i++ ] )
		{
			const Vector& currPos = baseBoundings[ i ];
			const Vector& nextPos = baseBoundings[ i + 1 == n ? 0 : i + 1 ];

			// compute new node position
			Vector2 dirVec1 = currPos - *lastPos;
			Vector2 dirVec2 = currPos - nextPos;
			Vector2 dirVecOver = nextPos - *lastPos;
			dirVec1.Normalize();
			dirVec2.Normalize();

			if ( isCounterClockwise )
			{
				Swap( dirVec1, dirVec2 );
				dirVecOver = -dirVecOver;
			}

			Bool isConcave = dirVecOver.CrossZ( dirVec1 ) < 0.f;

			// Calculate node distance based on angle between walls
			Float cos2A = dirVec1.Dot( -dirVec2 );
			const Float r = extensionLength;
 			if ( cos2A < -0.001f && !isConcave )
 			{
 				// special case when angle is greater then 90 degrees. In this case we put 2 nodes insteady of one
 				Float cosA = sqrt( (cos2A + 1.f) / 2.f );
 				Float sinA = sqrt( 1.f - cosA*cosA );
 
 				Float x = (r * ( 1.f - cosA )) / sinA;
 				// Now compute two locations
 				Vector2 delta1 = MathUtils::GeometryUtils::PerpendicularL(dirVec1) * r + dirVec1 * x;
 				Vector2 delta2 = MathUtils::GeometryUtils::PerpendicularR(dirVec2) * r + dirVec2 * x;
 
 				Vector2 detourPoint1 = currPos + delta1;
 				Vector2 detourPoint2 = currPos + delta2;
 
				if ( isCounterClockwise )
				{
					extendedPoly.PushBack( detourPoint2 );
					extendedPoly.PushBack( detourPoint1 );
				}
				else
				{
					extendedPoly.PushBack( detourPoint1 );
					extendedPoly.PushBack( detourPoint2 );
				}
 				
 			}
 			else
			{
				// spawn single node
				Float cosA = sqrt( (cos2A + 1.f) / 2.f );
				Float dist = r / cosA ;			// at most: 2^(1/2) * r
				//
				Vector2 dirVec = dirVec1+dirVec2;
				if ( !dirVec.IsAlmostZero() )
				{
					dirVec.Normalize();
					dirVec *= dist;
					if ( isConcave )
					{
						dirVec = -dirVec;
					}
					Vector2 detourPoint = currPos + dirVec;
					extendedPoly.PushBack( detourPoint );
				}
			}

			lastPos = &currPos;
		}

		// compute extension bbox
		// extend bbox
		for ( auto it = extendedPoly.Begin(), end = extendedPoly.End(); it != end; ++it )
		{
			const Vector2& v = *it;

			boundingBox.Min.X = Min( boundingBox.Min.X, v.X );
			boundingBox.Min.Y = Min( boundingBox.Min.Y, v.Y );
			boundingBox.Max.X = Max( boundingBox.Max.X, v.X );
			boundingBox.Max.Y = Max( boundingBox.Max.Y, v.Y );
		}

		boundingPoly = extendedPoly.TypedData();
		boundingPolySize = extendedPoly.Size();
		vertsInput = &m_vertsExtensionInput;
		trisInput = &m_trisExtensionInput;
	}
	else
	{
		boundingBox = input.m_bbox;
		boundingPoly = input.m_boundingPoly.TypedData();
		boundingPolySize = input.m_boundingPoly.Size();
		vertsInput = &m_vertsInput;
		trisInput = &m_trisInput;
	}

	// sea level check
	const Float seaLevel = pathlib->GetGlobalSettings().GetSeaLevel();
	if ( boundingBox.Max.Z < seaLevel )
	{
		return false;
	}
	boundingBox.Min.Z = Max( boundingBox.Min.Z, seaLevel );

	return CollectTerrainGeometry( input, boundingBox, boundingPoly, boundingPolySize, *vertsInput, *trisInput );
	
}
Bool CGenerator::CollectFoliageGeometry( CGenerationInputData& input, const SFoliageInstance & foliageInstance, const TDynArray< Sphere >& foliageShape )
{
	Float scale = foliageInstance.GetScale();

	// WARNING: ctrl+c ctrl+v from CPhysicsTileWrapper::AddFoliageBody

	// Don't ask why. I have no clue. It works...
	Matrix baseTransform;
	Vector instanceQuat = foliageInstance.GetQuaterion();
	RedQuaternion q;
	q.SetMul( RedQuaternion( 0.f, 0.f, -instanceQuat.Z, instanceQuat.W ), RedQuaternion( RedVector4::EZ, -M_PI_HALF ) );

	baseTransform.BuildFromQuaternion( reinterpret_cast< Vector& >( q ) );
	baseTransform.SetTranslation( foliageInstance.GetPosition() );

	TDynArray< Vector > verts;
	TDynArray< Uint32 > indexes;
	for ( Uint32 i = 0, n = foliageShape.Size(); i+1 < n; i += 2 )
	{
		Vector shapePosition1 = foliageShape[ i ].GetCenter() * scale;
		Vector shapePosition2 = foliageShape[ i + 1 ].GetCenter() * scale;
		Float radius = foliageShape[ i ].GetRadius() * scale;
		shapePosition1.W = 1.0f;
		shapePosition2.W = 1.0f;

		Vector direction = shapePosition1 - shapePosition2;
		Float height = direction.Normalize3();
		Float halfHeight = height / 2.f;
		
		Matrix localPose = Matrix::IDENTITY;

		localPose.V[0] = direction;

		localPose.V[2] = Vector::Cross( Vector::EY, localPose.V[0] ).Normalized3();
		localPose.V[1] = Vector::Cross( localPose.V[2], localPose.V[0] ).Normalized3();

		localPose.V[0].W = 0.0f;
		localPose.V[1].W = 0.0f;
		localPose.V[2].W = 0.0f;
		localPose.V[3] = Vector::EW;

		Vector pos = Vector( ( shapePosition1 + shapePosition2 ) / 2 );
		localPose.SetTranslation( pos );

		Matrix shapeTransform = localPose * baseTransform;

		if ( halfHeight > 0.f )
		{
			CollisionShapeBuilder::BuildCapsule( radius, halfHeight, verts, indexes );
		}
		else
		{
			CollisionShapeBuilder::BuildSphere( radius, verts, indexes );
		}

		for( Uint32 v = 0, n = verts.Size(); v < n; ++v )
		{
			verts[ v ] = shapeTransform.TransformPoint( verts[ v ] );
		}

		CollectShapeGeometry( input, verts, indexes, false, false );
	}

	return true;
}
Bool CGenerator::CollectFoliageGeometry( CGenerationInputData& input )
{
	CLayer* componentLayer = input.m_navmeshComponent->GetEntity()->GetLayer();
	CWorld* world = componentLayer ? componentLayer->GetWorld() : NULL;
	CFoliageEditionController & foliageController = world->GetFoliageEditionController();
	
	TDynArray< Sphere > collisionShapes;
	
	TDynArray< SFoliageInstanceCollection > collection;
	foliageController.GetInstancesFromArea( input.m_bbox, collection );

	for( auto iter = collection.Begin(), end = collection.End(); iter != end; ++iter )
	{
		CSRTBaseTree* baseTree = iter->m_baseTree.Get();
		if ( !baseTree )
		{
			continue;
		}

		GRender->GetSpeedTreeResourceCollision( baseTree->AcquireRenderObject().Get(), collisionShapes );

		if( collisionShapes.Empty() )
		{
			continue;
		}

		// Get instances in the search radius
		const FoliageInstanceContainer & container = iter->m_instances;
		for ( auto instanceIter = container.Begin(), instanceEnd = container.End(); instanceIter != instanceEnd; ++instanceIter )
		{
			const SFoliageInstance & instance = *instanceIter;

			if ( !MathUtils::GeometryUtils::TestIntersectionPolygonCircle2D( input.m_boundingPoly, Vector2( instance.GetPosition().X, instance.GetPosition().Y ), 2.f ) )
			{
				continue;
			}

			CollectFoliageGeometry( input, instance, collisionShapes );
		}

		collisionShapes.ClearFast();
	}

	return true;
}

Bool CGenerator::ApplyBorderAreas( CGenerationInputData& input, const TDynArray< CNavmeshBorderAreaComponent* >& borderAreas )
{
	if ( borderAreas.Empty() )
	{
		return false;
	}

	const auto& verts = m_vertsInput;
	auto& tris = m_trisInput;

	auto& extVerts = m_vertsExtensionInput;
	auto& extTris = m_trisExtensionInput;

	// NOTICE: We iterate over all triangles for every border area, so they might turn out to be pretty heavy if overused
	for ( auto it = borderAreas.Begin(), end = borderAreas.End(); it != end; ++it )
	{
		CNavmeshBorderAreaComponent* area = *it;

		const Box& areaBox = area->GetBoundingBox();
		const CAreaComponent::TAreaPoints& areaPoints = area->GetWorldPoints();

		for ( Uint32 triInd = 0, triCount = tris.Size(); triInd < triCount; )
		{
			Vector3 v[ 3 ];

			// convert vertexes from recast-ready buffers back to engine triangle
			RecastVertexToEngine( &verts[ tris[ triInd+0 ]*3 ], v[ 0 ] );
			RecastVertexToEngine( &verts[ tris[ triInd+1 ]*3 ], v[ 1 ] );
			RecastVertexToEngine( &verts[ tris[ triInd+2 ]*3 ], v[ 2 ] );

			// compute triangle boundings
			Box triBox( Box::RESET_STATE );
			triBox.AddPoint( v[ 0 ] );
			triBox.AddPoint( v[ 1 ] );
			triBox.AddPoint( v[ 2 ] );

			// boundings test for fast-out
			if ( !triBox.Touches( areaBox ) )
			{
				triInd += 3;
				continue;
			}

			// We assume 'Z' boundings are handled by box test, so in this case just test area poly agains triangle in 2d. NOTICE: Its simplification.
			// TODO: We have no direct function for custom (possibly non-convex) polygon vs triangle test so I will use a bunch of costly but well defined math functions for a test
			if (
				!MathUtils::GeometryUtils::IsPointInPolygon2D( areaPoints, v[ 0 ].AsVector2() ) &&
				!MathUtils::GeometryUtils::TestIntersectionPolygonLine2D( areaPoints, v[ 0 ].AsVector2(), v[ 1 ].AsVector2() ) &&
				!MathUtils::GeometryUtils::TestIntersectionPolygonLine2D( areaPoints, v[ 1 ].AsVector2(), v[ 2 ].AsVector2() ) &&
				!MathUtils::GeometryUtils::TestIntersectionPolygonLine2D( areaPoints, v[ 2 ].AsVector2(), v[ 0 ].AsVector2() )
				)
			{
				triInd += 3;
				continue;
			}

			// add new triangle to extension geometry
			Uint32 vertsBaseInd = extVerts.Size();
			extVerts.Grow( 9 );
			Uint32 trisBaseInd = extTris.Size();
			extTris.Grow( 3 );

			// add vertexes
			EngineVertexToRecast( v[ 0 ], &extVerts[ vertsBaseInd+0 ] );
			EngineVertexToRecast( v[ 1 ], &extVerts[ vertsBaseInd+3 ] );
			EngineVertexToRecast( v[ 2 ], &extVerts[ vertsBaseInd+6 ] );

			// add indices
			extTris[ trisBaseInd+0 ] = (vertsBaseInd/3)+0;
			extTris[ trisBaseInd+1 ] = (vertsBaseInd/3)+1;
			extTris[ trisBaseInd+2 ] = (vertsBaseInd/3)+2;

			// remove original indices (move last triangle in that place
			tris[ triInd+0 ] = tris[ triCount-3 ];
			tris[ triInd+1 ] = tris[ triCount-2 ];
			tris[ triInd+2 ] = tris[ triCount-1 ];

			triCount -= 3;
			tris.ResizeFast( triCount );
		}
	}

	return true;
}

Bool CGenerator::CollectGeometry( CGenerationInputData& input )
{
	TDynArray< CStaticMeshComponent* > meshes;
	TDynArray< CDeniedAreaComponent* > deniedAreas;
	TDynArray< CNavmeshBorderAreaComponent* > borderAreas;

	CollectMeshes( input, meshes, deniedAreas, borderAreas );

	Bool hasData = m_params.m_useStaticMeshesInGeneration && CollectMeshGeometry( input, meshes, deniedAreas );
	if ( m_params.m_useTerrainInGeneration && CollectTerrainGeometry( input, false ) )
	{
		hasData = true;
	}
	if ( m_params.m_detectTerrainConnection )
	{
		if ( !CollectTerrainGeometry( input, true ) )
		{
			PATHLIB_LOG( TXT("No terrain collected for terrain connection algorithm!\n") );
		}
	}
	if ( !hasData )
	{
		return false;
	}
	if ( m_params.m_collectFoliage )
	{
		CollectFoliageGeometry( input );
	}
	ApplyBorderAreas( input, borderAreas );

	return true;
}

void CGenerator::NoticeErrorInternal( PathLib::SNavmeshProblem&& p )
{
	PATHLIB_ERROR( p.m_text.AsChar() );

	m_problems.PushBack( Move( p ) );
}

void CGenerator::ReportErrors()
{
	if ( !m_problems.Empty() )
	{
		String msg;
		for ( Uint32 i = 0, n = m_problems.Size(); i < n; msg += TXT("\n"), ++i )
		{
			msg += m_problems[ i ].m_text;
		}
		GFeedback->ShowMsg( TXT("Navmesh generation failed!"), msg.AsChar() );
	}
}


// synchronous processing
Bool CGenerator::Initialize( CGenerationInputData& input )
{
	if ( !VerifyInput( input ) )
	{
		return false;
	}

	{
		Box bbox = input.m_bbox;
		Float extraRange = input.m_params.m_extensionLength + input.m_params.m_extraStreamingRange;
		bbox.Min -= extraRange;
		bbox.Max += extraRange;

		// make sure everything in the given area is loaded fully
		GGame->GetActiveWorld()->ForceStreamingForArea( bbox );

		// fix for so called "streaming not working" bug...
		input.m_inputIterator->Reset();
	}

	if ( !CollectGeometry( input ) )
	{
		NoticeError( TXT("Empty navmesh generation input. No data in boundings!\n") );
		return false;
	}
	return true;
}


#define GENERATE_DATA_MEMBER_DEF( rcName )								\
	rc##rcName##*					m_##rcName##;

#define GENERATE_DATA_MEMBER_ALLOC( rcName )							\
	Bool Alloc##rcName##()												\
	{																	\
		m_##rcName = rcAlloc##rcName##();								\
		return m_##rcName != NULL;										\
	}

#define GENERATE_DATA_MEMBER_FREE( rcName, rcFun )						\
	void Free##rcName##()												\
	{																	\
		if ( m_##rcName )												\
		{																\
			rcFun##( m_##rcName );										\
			m_##rcName = NULL;											\
		}																\
	}

#define GENERATION_DATA_MEMBER( rcName )								\
	GENERATE_DATA_MEMBER_DEF( rcName )									\
	GENERATE_DATA_MEMBER_ALLOC( rcName )								\
	GENERATE_DATA_MEMBER_FREE( rcName, rcFree##rcName )

// possibly asynchronous processing
Bool CGenerator::Generate()
{
	struct GenerationData
	{
		GENERATE_DATA_MEMBER_DEF( Heightfield );
		GENERATE_DATA_MEMBER_ALLOC( Heightfield );
		GENERATE_DATA_MEMBER_FREE( Heightfield, rcFreeHeightField );
		GENERATION_DATA_MEMBER( CompactHeightfield );
		GENERATION_DATA_MEMBER( ContourSet );
		GENERATION_DATA_MEMBER( PolyMesh );
		GENERATION_DATA_MEMBER( PolyMeshDetail );

		GenerationData()
			: m_Heightfield( NULL )
			, m_CompactHeightfield( NULL )
			, m_ContourSet( NULL )
			, m_PolyMesh( NULL )
			, m_PolyMeshDetail( NULL )
		{}
		~GenerationData()
		{
			FreeHeightfield();
			FreeCompactHeightfield();
			FreeContourSet();
			FreePolyMesh();
			FreePolyMeshDetail();
		}
	};

	struct Local
	{
		enum
		{
			POLY_REACHABLE			= 1,
			POLY_GENERATE			= 2,
		};
		enum
		{
			AREA_BASE				= 4,
			AREA_EXTENSION			= 2
		};

		static void ComputePolyEdge( rcPolyMesh* polyMesh, Uint32 polyId, Int32 edge, Vector3& worldV1, Vector3& worldV2 )
		{
			Uint32 nvp = polyMesh->nvp;
			const Uint16* p = &polyMesh->polys[ polyId*nvp*2 ];
			const Uint16* verts = polyMesh->verts;

			Float* lowBoundings = polyMesh->bmin;

			Vector3 worldBoundings( lowBoundings[ 0 ], lowBoundings[ 2 ], lowBoundings[ 1 ] );

			const float cs = polyMesh->cs;
			const float ch = polyMesh->ch;

			Int32 nextEdge = ( edge+1 == polyMesh->nvp || p[ edge+1 ] == RC_MESH_NULL_IDX ) ? 0 : edge+1;

			const Uint16* v1 = &verts[ p[ edge ] * 3 ];
			const Uint16* v2 = &verts[ p[ nextEdge ] * 3 ];

			worldV1.Set( v1[0]*cs, v1[2]*cs, v1[1]*ch );
			worldV2.Set( v2[0]*cs, v2[2]*cs, v2[1]*ch );

			worldV1 += worldBoundings;
			worldV2 += worldBoundings;
		}
	};
	struct GenerationRootMarker
	{
		void MarkGenerationRoot( rcPolyMesh* polyMesh, Uint32 markPoly )
		{
			const int nvp = polyMesh->nvp;

			polyMesh->flags[ markPoly ] |= Local::POLY_REACHABLE;
			m_activePolys.PushBack( markPoly );
			while ( !m_activePolys.Empty() )
			{
				Uint32 polyId = m_activePolys.Back();
				m_activePolys.PopBackFast();

				const Uint16* p = &polyMesh->polys[ polyId*nvp*2 ];

				for (int j = 0; j < nvp; ++j)
				{
					if (p[j] == RC_MESH_NULL_IDX) 
						break; // End of vertices.

					if (p[j + nvp] != RC_MESH_NULL_IDX)
					{
						// The edge beginning with this vertex connects to 
						// polygon p[j + nvp].
						Uint16 neighbour = p[j + nvp];
						if ( (polyMesh->flags[ neighbour ] & Local::POLY_REACHABLE) == 0 && polyMesh->areas[ neighbour ] == Local::AREA_BASE )
						{
							polyMesh->flags[ neighbour ] |= Local::POLY_REACHABLE;
							m_activePolys.PushBack( neighbour );
						}
					}
				}
				
			}
		}
		TDynArray< Uint32 > m_activePolys;
	};
	struct ExtendableAreaDistanceMarkingAlgorithm : public Red::System::NonCopyable
	{
		struct AreaDistance
		{
			AreaDistance()
				: m_calculated( false ) {}
			Bool			m_calculated;
			Bool			m_generates;
			Uint16			m_testHash;
		};
		rcPolyMesh*						m_polyMesh;
		TDynArray< AreaDistance >		m_areas;
		const Uint32						m_nvp;
		const Float						m_extendableSearchDistSq;
		TDynArray< Uint32 >				m_activeReachablePolys;
		TDynArray< Uint32 >				m_activeExtendablePolys;
		Uint16							m_lastTestHash;

	private:
		Bool IsPolyReachable( Uint32 polyId )
		{
			if ( m_polyMesh->areas[ polyId ] == Local::AREA_BASE && (m_polyMesh->flags[ polyId ] & Local::POLY_REACHABLE) )
			{
				return true;
			}
			return false;
		}
		
		void WideExtendableSearch( const Vector2& baseEdgeV0, const Vector2& baseEdgeV1, Uint16 testHash )
		{
			while( !m_activeExtendablePolys.Empty() )
			{
				Uint32 polyId = m_activeExtendablePolys.Back();
				m_activeExtendablePolys.PopBackFast();

				//AreaDistance& currDistance = m_areas[ polyId ];

				const Uint16* p = &m_polyMesh->polys[ polyId*m_nvp*2 ];
				for ( Uint32 j = 0; j < m_nvp; ++j )
				{
					if (p[j] == RC_MESH_NULL_IDX) 
						break; // End of vertices.

					Uint32 nextPoly = p[j + m_nvp];
					if ( nextPoly != RC_MESH_NULL_IDX )
					{
						if ( IsPolyReachable( nextPoly ) )
						{
							// poly handled by WideReachableSearch
							continue;
						}
						AreaDistance& nextDistance = m_areas[ nextPoly ];
						if ( nextDistance.m_calculated && nextDistance.m_testHash == testHash )
						{
							// already visited during this test
							continue;
						}
						Vector3 edgeV0, edgeV1;
						Local::ComputePolyEdge( m_polyMesh, polyId, j, edgeV0, edgeV1 );
						Float distanceSq = MathUtils::GeometryUtils::TestDistanceSqrLineLine2D( edgeV0.AsVector2(), edgeV1.AsVector2(), baseEdgeV0, baseEdgeV1 );
						if ( distanceSq > m_extendableSearchDistSq )
						{
							// poly out-of-range (at least from given base edge)
							continue;
						}
						
						// notice: its possible that we could already have visited this node in previous tests - but there is no need to test it
						nextDistance.m_calculated = true;
						nextDistance.m_testHash = testHash;
						m_polyMesh->flags[ nextPoly ] |= Local::POLY_GENERATE;
						m_activeExtendablePolys.PushBack( nextPoly );
					}
				}
			}
		}

		void WideReachableSearch( )
		{
			while( !m_activeReachablePolys.Empty() )
			{
				Uint32 polyId = m_activeReachablePolys.Back();
				m_activeReachablePolys.PopBackFast();

				//AreaDistance& currDistance = m_areas[ polyId ];

				const Uint16* p = &m_polyMesh->polys[ polyId*m_nvp*2 ];
				for ( Uint32 j = 0; j < m_nvp; ++j )
				{
					if (p[j] == RC_MESH_NULL_IDX) 
						break; // End of vertices.

					Uint32 nextPoly = p[j + m_nvp];
					if ( nextPoly != RC_MESH_NULL_IDX )
					{
						AreaDistance& nextDistance = m_areas[ nextPoly ];
						if ( IsPolyReachable( nextPoly ) )
						{
							if( !nextDistance.m_calculated )
							{
								nextDistance.m_calculated = true;
								nextDistance.m_generates = true;
								m_polyMesh->flags[ nextPoly ] |= Local::POLY_GENERATE;
								m_activeReachablePolys.PushBack( nextPoly );
							}
						}
						else
						{
							Uint16 testHash = ++m_lastTestHash;
							nextDistance.m_calculated = true;
							nextDistance.m_generates = true;
							nextDistance.m_testHash = testHash;
							m_polyMesh->flags[ nextPoly ] |= Local::POLY_GENERATE;
							m_activeExtendablePolys.PushBack( nextPoly );
							Vector3 v0, v1;
							Local::ComputePolyEdge( m_polyMesh, polyId, j, v0, v1 );
							// start extendable search
							WideExtendableSearch( v0.AsVector2(), v1.AsVector2(), testHash );
						}
					}
				}
			}
		}
	public:
		ExtendableAreaDistanceMarkingAlgorithm( rcPolyMesh* polyMesh, Float extendableRange )
			: m_polyMesh( polyMesh )
			, m_areas( polyMesh->npolys )
			, m_nvp( polyMesh->nvp )
			, m_extendableSearchDistSq( extendableRange*extendableRange )
			, m_lastTestHash( 1 ) {}

		void MarkReachable()
		{
			// find starting points for 
			Uint32 polysCount = m_polyMesh->npolys;
			for ( Uint32 polyId = 0; polyId < polysCount; ++polyId )
			{
				if ( m_areas[ polyId ].m_calculated )
				{
					continue;
				}

				if ( IsPolyReachable( polyId ) )
				{
					m_polyMesh->flags[ polyId ] |= Local::POLY_GENERATE;
					m_areas[ polyId ].m_calculated = true;
					m_areas[ polyId ].m_generates = true;
					m_areas[ polyId ].m_testHash = 0;
					m_activeReachablePolys.PushBack( polyId );
					WideReachableSearch();
				}
			}
		}
	};

	EngineTime startTime = EngineTime::GetNow();

	Uint32 nverts = m_vertsInput.Size() / 3;		// 3 floats per vector
	Uint32 ntris = m_trisInput.Size() / 3;			// 3 indexes per triangle

	Uint32 extVerts = m_vertsExtensionInput.Size() / 3;
	Uint32 extTris = m_trisExtensionInput.Size() / 3;

	Uint32 blockVerts = m_unwalkableVertsInput.Size() / 3;
	Uint32 blockTris = m_unwalkableTrisInput.Size() / 3;

	m_currentGenerationTask = ENavGen_MarkTriangles;

	if( m_params.m_previewOriginalGeometry )
	{
		if ( nverts >= 0xffff || ntris >= 0xffff )
		{
			NoticeError( TXT("Too large geometry to preview!\n") );
			return false;
		}
		// Pass values back
		{
			m_vertsOutput.Resize( nverts );
			const Float* dst = m_vertsInput.TypedData();
			for( Uint32 i = 0; i < nverts; ++i )
			{
				Vector3 vec;

				vec.X = *dst++;
				vec.Z = *dst++;
				vec.Y = *dst++;

				m_vertsOutput[ i ] = vec;
			}
		}

		{
			m_trisOutput.Resize( ntris*3 );

			for ( Uint32 j = 0; j < ntris; ++j )
			{
				Uint32 base = j*3;
				m_trisOutput[ base+2 ] = Uint16(m_trisInput[ base+0 ]);
				m_trisOutput[ base+1 ] = Uint16(m_trisInput[ base+1 ]);
				m_trisOutput[ base+0 ] = Uint16(m_trisInput[ base+2 ]);
			}
		}
		return true;
	}

	GenerationData data;

	Bool useExtendedInput = !m_trisExtensionInput.Empty();

	Float bmin[3];
	Float bmax[3];
	rcCalcBounds( m_vertsInput.TypedData(), nverts, bmin, bmax );
	
	if ( useExtendedInput )
	{
		Float extmin[3];
		Float extmax[3];

		rcCalcBounds( m_vertsExtensionInput.TypedData(), extVerts, extmin, extmax );

		for ( Uint32 i = 0; i < 3; ++i )
		{
			bmin[ i ] = Min( bmin[ i ], extmin[ i ] );
			bmax[ i ] = Max( bmax[ i ], extmax[ i ] );
		}
	}

	// rare but somehow important case. We need to sligtly enlarge area or else there might be a problems with obstacles that are exactly on its edges, as they can become unnoticable (unmarkable)
	for ( Uint32 i = 0; i < 3; ++i )
	{
		--bmin[ i ];
		++bmax[ i ];
	}

	// collect static geometry that collide with standing character
	bmax[ 1 ] += PathLib::DEFAULT_AGENT_HEIGHT;

	//
	// Step 1. Initialize build config.
	//

	// Init build configuration
	rcConfig config;
	config.cs = m_params.m_cellWidth;
	config.ch = m_params.m_cellHeight;
	config.tileSize = 0;
	config.borderSize = 0;
	config.walkableSlopeAngle = m_params.m_walkableSlopeAngle;
	config.walkableHeight = (int)ceilf(m_params.m_agentHeight / config.ch);
	config.walkableClimb = (int)floorf(m_params.m_agentClimb / config.ch);
	config.walkableRadius = (int)ceilf(m_params.m_margin / config.cs);
	config.maxEdgeLen = (int)(m_params.m_maxEdgeLen / m_params.m_cellWidth);
	config.maxSimplificationError = m_params.m_maxEdgeError;
	config.minRegionArea = (int)rcSqr(m_params.m_regionMinSize);
	config.mergeRegionArea = (int)rcSqr(m_params.m_regionMergeSize);
	config.maxVertsPerPoly = (int)m_params.m_vertsPerPoly;
	config.detailSampleDist = m_params.m_detailSampleDist < 0.9f ? 0 : m_params.m_cellWidth * m_params.m_detailSampleDist;
	config.detailSampleMaxError = m_params.m_cellHeight * m_params.m_detailSampleMaxError;

	// Set the area where the navigation will be build.
	// Here the bounds of the input mesh are used, but the
	// area could be specified by an user defined box, etc.
	rcVcopy(config.bmin, bmin);
	rcVcopy(config.bmax, bmax);
	rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

	//// Start the build process.
	struct Context : public rcContext
	{
		Context()
			: rcContext()
		{
			m_logEnabled = true;
		}
		virtual void doLog( const rcLogCategory category, const char* msg, const int len )
		{
#if defined( UNICODE )
			CAnsiToUnicode str( msg );
			PATHLIB_LOG( (UniChar*)str );
#else
			PATHLIB_LOG( msg );
#endif
		}
	} context;
	PATHLIB_LOG( TXT("Building navigation:") );
	PATHLIB_LOG( TXT(" - %d x %d cells"), config.width, config.height );
	PATHLIB_LOG( TXT(" - %.1fK verts, %.1fK tris"), (nverts+extVerts+blockVerts)/1000.0f, (ntris+extTris+blockTris)/1000.0f );

	//
	// Step 2. Rasterize input polygon soup.
	//

	// Allocate voxel heightfield where we rasterize our input data to.
	if ( !data.AllocHeightfield() )
	{
		NoticeError( TXT("buildNavigation: Out of memory 'heightfield'.") );
		return false;
	}
	if ( !rcCreateHeightfield( &context, *data.m_Heightfield, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch) )
	{
		NoticeError( TXT("buildNavigation: Out of memory 'heightfield'.") );
		return false;
	}

	{
		// Allocate array that can hold triangle area types.
		// If you have multiple meshes you need to process, allocate
		// and array which can hold the max number of triangles you need to process.

		TDynArray< unsigned char > triAreas;
		triAreas.Reserve( Max( ntris, extTris, blockTris ) );

		// Find triangles which are walkable based on their slope and rasterize them.
		// If your input data is multiple meshes, you can transform them here, calculate
		// the are type for each of the meshes and rasterize them.
		if ( blockTris > 0 )
		{
			triAreas.ResizeFast( blockTris );
			Red::System::MemorySet( triAreas.Data(), 0, triAreas.DataSize() );
			rcRasterizeTriangles( &context, m_unwalkableVertsInput.TypedData(), blockVerts, m_unwalkableTrisInput.TypedData(), triAreas.TypedData(), blockTris, *data.m_Heightfield, 0 );
		}

		triAreas.ResizeFast( ntris );
		Red::System::MemorySet( triAreas.Data(), Local::AREA_BASE | (Local::AREA_BASE << 8) | (Local::AREA_BASE << 16) | (Local::AREA_BASE << 24), triAreas.DataSize() );
		rcClearUnwalkableTriangles( &context, config.walkableSlopeAngle, m_vertsInput.TypedData(), nverts, m_trisInput.TypedData(), ntris, triAreas.TypedData() );
		rcRasterizeTriangles( &context, m_vertsInput.TypedData(), nverts, m_trisInput.TypedData(), triAreas.TypedData(), ntris, *data.m_Heightfield, config.walkableClimb );

		if ( useExtendedInput )
		{
			triAreas.ResizeFast( extTris );
			Red::System::MemorySet( triAreas.Data(), Local::AREA_EXTENSION | (Local::AREA_EXTENSION << 8) | (Local::AREA_EXTENSION << 16) | (Local::AREA_EXTENSION << 24), triAreas.DataSize() );
			rcClearUnwalkableTriangles( &context, config.walkableSlopeAngle, m_vertsExtensionInput.TypedData(), extVerts, m_trisExtensionInput.TypedData(), extTris, triAreas.TypedData() );
			rcRasterizeTriangles( &context, m_vertsExtensionInput.TypedData(), extVerts, m_trisExtensionInput.TypedData(), triAreas.TypedData(), extTris, *data.m_Heightfield, config.walkableClimb );
		}
	}

	m_currentGenerationTask = ENavGen_ProcessHeightfield;

	//
	// Step 3. Filter walkables surfaces.
	//

	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	if ( m_params.m_stepOnNonWalkableMeshes )
	{
		rcFilterLowHangingWalkableObstacles( &context, config.walkableClimb, *data.m_Heightfield );
	}
	rcFilterLedgeSpans( &context, config.walkableHeight, config.walkableClimb, *data.m_Heightfield );
	rcFilterWalkableLowHeightSpans( &context, config.walkableHeight, *data.m_Heightfield );

	//
	// Step 4. Partition walkable surface to simple regions.
	//

	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	if ( !data.AllocCompactHeightfield() )
	{
		NoticeError( TXT("buildNavigation: Out of memory 'compactHeightfield'.") );
		return false;
	}
	if (!rcBuildCompactHeightfield( &context , config.walkableHeight, config.walkableClimb, *data.m_Heightfield, *data.m_CompactHeightfield))
	{
		NoticeError( TXT("buildNavigation: Could not build compact data.") );
		return false;
	}

	data.FreeHeightfield();

	// Erode the walkable area by agent radius.
	if ( config.walkableRadius > 0.f)
	{
		if (!rcErodeWalkableArea( &context, config.walkableRadius, *data.m_CompactHeightfield) )
		{
			NoticeError( TXT("buildNavigation: Could not erode.") );
			return false;
		}
	}

	if ( m_params.m_smoothWalkableAreas )
	{
		if ( !rcMedianFilterWalkableArea( &context, *data.m_CompactHeightfield ) )
		{
			NoticeError( TXT("buildNavigation: Problems with walkable areas smoothing.") );
			return false;
		}
	}

	m_currentGenerationTask = ENavGen_BuildRegions;

	if ( m_params.m_monotonePartitioning )
	{
		// Partition the walkable surface into simple regions without holes.
		// Monotone partitioning does not need distancefield.
		if ( !rcBuildRegionsMonotone( &context, *data.m_CompactHeightfield, 0, config.minRegionArea, config.mergeRegionArea ) )
		{
			NoticeError( TXT("buildNavigation: Could not build regions.") );
			return false;
		}
	}
	else
	{
		// Prepare for region partitioning, by calculating distance field along the walkable surface.
		if (!rcBuildDistanceField( &context, *data.m_CompactHeightfield ))
		{
			NoticeError( TXT("buildNavigation: Could not build distance field.") );
			return false;
		}

		// Partition the walkable surface into simple regions without holes.
		if (!rcBuildRegions( &context, *data.m_CompactHeightfield, 0, config.minRegionArea, config.mergeRegionArea ))
		{
			NoticeError( TXT("buildNavigation: Could not build regions.") );
			return false;
		}
	}

	m_currentGenerationTask = ENavGen_BuildContourSet;

	//
	// Step 5. Trace and simplify region contours.
	//

	// Create contours.
	if ( !data.AllocContourSet() )
	{
		NoticeError( TXT("buildNavigation: Out of memory 'cset'.") );
		return false;
	}
	if ( !rcBuildContours( &context, *data.m_CompactHeightfield, config.maxSimplificationError, config.maxEdgeLen, *data.m_ContourSet ) )
	{
		NoticeError( TXT("buildNavigation: Could not create contours.") );
		return false;
	}

	m_currentGenerationTask = ENavGen_BuildPolyMesh;

	//
	// Step 6. Build polygons mesh from contours.
	//

	// Build polygon navmesh from the contours.
	if ( !data.AllocPolyMesh() )
	{
		NoticeError( TXT("buildNavigation: Out of memory 'pmesh'.") );
		return false;
	}
	if (!rcBuildPolyMesh( &context, *data.m_ContourSet, config.maxVertsPerPoly, *data.m_PolyMesh ))
	{
		NoticeError( TXT("buildNavigation: Could not triangulate contours.") );
		return false;
	}

	data.FreeContourSet();

	m_currentGenerationTask = ENavGen_BuildPolyMeshDetail;

	// Fix poly mesh overlapping polygons
	//FixOverlappingPolygons( *data.m_PolyMesh );

	//
	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	//

	if ( !data.AllocPolyMeshDetail() )
	{
		NoticeError( TXT("buildNavigation: Out of memory 'pmdtl'.") );
		return false;
	}

	if ( !rcBuildPolyMeshDetail( &context, *data.m_PolyMesh, *data.m_CompactHeightfield, config.detailSampleDist, config.detailSampleMaxError, *data.m_PolyMeshDetail ) )
	{
		NoticeError( TXT("buildNavigation: Could not build detail mesh.") );
		return false;
	}

	data.FreeCompactHeightfield();

	m_currentGenerationTask = ENavGen_ConsistencyProcessing;

	// we will use this two a lot!
	rcPolyMesh* polyMesh = data.m_PolyMesh;
	rcPolyMeshDetail* polyMeshDetail = data.m_PolyMeshDetail;
	Uint32 polysCount = polyMesh->npolys;

	// mark generation roots
	Bool useGenerationRoots = false;
	if ( !m_generationRoots.Empty() )
	{
		GenerationRootMarker localImpl;

		const int nvp = polyMesh->nvp;
		const float cs = polyMesh->cs;
		const float ch = polyMesh->ch;
		Float* lowBoundings = polyMesh->bmin;
		for ( Uint32 polyInd = 0; polyInd < polysCount; ++polyInd )
		{
			if ( polyMesh->flags[ polyInd ] & Local::POLY_REACHABLE || polyMesh->areas[ polyInd ] == Local::AREA_EXTENSION )
			{
				continue;
			}

			const unsigned short* p = &polyMesh->polys[ polyInd*nvp*2 ];
			
			// compute poly boundings
			Float polyMin[3];
			Float polyMax[3];
			for ( Uint32 i = 0; i < 3; ++i )
			{
				polyMin[ i ] = FLT_MAX;
				polyMax[ i ] = -FLT_MAX;
			}
			for ( Int32 vertIdx = 0; vertIdx < nvp; ++vertIdx )
			{
				if ( p[ vertIdx ] == RC_MESH_NULL_IDX )
				{
					break;
				}
				const unsigned short* v = &polyMesh->verts[ p[ vertIdx ] * 3 ];
				Float worldPos[3];
				worldPos[0] = lowBoundings[0] + v[0]*cs;
				worldPos[1] = lowBoundings[1] + v[1]*ch;
				worldPos[2] = lowBoundings[2] + v[2]*cs;
				for ( Uint32 i = 0; i < 3; ++i )
				{
					polyMin[ i ] = Min( polyMin[ i ], worldPos[ i ] );
					polyMax[ i ] = Max( polyMax[ i ], worldPos[ i ] );
				}
			}

			polyMax[ 1 ] += 3.f;
			polyMin[ 1 ] -= 1.f;
			// test poly boundings against generation roots
			for ( Uint32 i = 0, n = m_generationRoots.Size(); i != n; ++i )
			{
				const Vector3& rcGenRoot = m_generationRoots[ i ];
				
				if( rcGenRoot.A[ 0 ] < polyMin[ 0 ] || rcGenRoot.A[ 0 ] > polyMax[ 0 ] ||
					rcGenRoot.A[ 2 ] < polyMin[ 2 ] || rcGenRoot.A[ 2 ] > polyMax[ 2 ] )
				{
					continue;
				}

				// base generation root <-> polygon test
				TStaticArray< Vector2, 32 > polyVerts;
				for ( Int32 vertIdx = 0; vertIdx < nvp; ++vertIdx )
				{
					if ( p[ vertIdx ] == RC_MESH_NULL_IDX )
					{
						break;
					}
					const unsigned short* v = &polyMesh->verts[ p[ vertIdx ] * 3 ];
					polyVerts.PushBack(
						Vector2(
							lowBoundings[0] + v[0]*cs,
							lowBoundings[2] + v[2]*cs
					) );
				}
				if ( !MathUtils::GeometryUtils::IsPointInPolygon2D( polyVerts.TypedData(), polyVerts.Size(), Vector2( rcGenRoot.X, rcGenRoot.Z ) ) )
				{
					continue;
				}

				// detailed generation root <-> poly detail z-test
				const Uint32* meshDef = &polyMeshDetail->meshes[polyInd*4];

				const Uint32 baseVerts = meshDef[0];
				//const Uint32 vertsCount = meshDef[ 1 ];
				const Uint32 baseMeshTri = meshDef[2];
				const Uint32 meshNtris = meshDef[3];

				const Float* verts = &polyMeshDetail->verts[baseVerts*3];
				const Uint8* meshTris = &polyMeshDetail->tris[baseMeshTri*4];
				Vector3 genRoot( rcGenRoot.X, rcGenRoot.Z, rcGenRoot.Y );

				Bool foundTriangle = false;

				for (Uint32 k = 0; k < meshNtris; ++k)
				{
					const Float* rcV0 = &verts[ meshTris[k*4+0]*3 ];
					const Float* rcV1 = &verts[ meshTris[k*4+2]*3 ];
					const Float* rcV2 = &verts[ meshTris[k*4+1]*3 ];

					Vector3 v0( rcV0[ 0 ], rcV0[ 2 ], rcV0[ 1 ] );
					Vector3 v1( rcV1[ 0 ], rcV1[ 2 ], rcV1[ 1 ] );
					Vector3 v2( rcV2[ 0 ], rcV2[ 2 ], rcV2[ 1 ] );

					if ( !MathUtils::GeometryUtils::IsPointInsideTriangle2D( v0.AsVector2(), v1.AsVector2(), v2.AsVector2(), genRoot.AsVector2() ) )
						continue;

					Vector3 triangleNormal = ( v2 - v0 ).Cross( v1 - v0 );
					triangleNormal.Normalize();
					// Calculate distance
					Float fZ = - ( Vector3( genRoot.X, genRoot.Y, 0.f ) - v0 ).Dot( triangleNormal );
					fZ /= triangleNormal.Z;		// == Dot( Vec3(0,0,1), vNormal ) where this first one is ray direction
					// basically thats just it
					if ( fZ < genRoot.Z - GENROOT_AND_NAVMESH_MAX_DISTANCE || fZ > genRoot.Z + GENROOT_AND_NAVMESH_MAX_DISTANCE )
					{
						continue;
					}
					foundTriangle = true;
					break;
				}

				if ( !foundTriangle )
				{
					continue;
				}
				
				localImpl.MarkGenerationRoot( polyMesh, polyInd );
				useGenerationRoots = true;
			}
		}
		if ( !useGenerationRoots )
		{
			PATHLIB_LOG( TXT("No generation roots starting points! Skipping simplification.\n") );
		}
	}

	if ( !useGenerationRoots )
	{
		if ( m_params.m_useGenerationRootPoints )
		{
			NoticeError( TXT("No valid generation roots!") );
		}
		// No generation roots. As a fallback we mark all polys as "reachable"
		for ( Uint32 i = 0; i < polysCount; ++i )
		{
			polyMesh->flags[ i ] |= Local::POLY_REACHABLE;
		}
	}

	// mark areas as "reachable or not"
	if ( useExtendedInput )
	{
		ExtendableAreaDistanceMarkingAlgorithm markingAlgorithm( polyMesh, m_params.m_extensionLength );
		markingAlgorithm.MarkReachable();
	}
	else
	{
		for ( Uint32 i = 0; i < polysCount; ++i )
		{
			if ( polyMesh->flags[ i ] & Local::POLY_REACHABLE )
			{
				polyMesh->flags[ i ] |= Local::POLY_GENERATE;
			}
		}
	}


	m_currentGenerationTask = ENavGen_PrepareExport;
	// Determine geometry to export from poly mesh detail

	// iterate over submeshes and get triangles & vertices
	typedef Uint32 VertexIndex;
	TDynArray< Vector3 > verticesWithDupes;
	TDynArray< Uint32 > triangleIndices;

	PATHLIB_ASSERT( polyMeshDetail->nmeshes == polyMesh->npolys );
	for (Int32 j = 0; j < polyMeshDetail->nmeshes; ++j)
	{
		if ( (polyMesh->flags[ j ] & Local::POLY_GENERATE) == 0 )
		{
			continue;
		}

		const Uint32* meshDef = &polyMeshDetail->meshes[j*4];
		const Uint32 baseVerts = meshDef[0];
		const Uint32 vertsCount = meshDef[ 1 ];
		const Uint32 baseMeshTri = meshDef[2];
		const Uint32 meshNtris = meshDef[3];

		PATHLIB_ASSERT( meshNtris != 0 );

		Uint32 baseTriIndex = triangleIndices.Size();
		Uint32 currIndex = baseTriIndex;
		VertexIndex baseVertIndex = verticesWithDupes.Size();
		
		//triangleIndices.Grow( meshNtris*3 );

#ifdef DEBUG_NAVMESH_COLORS
		{
			Uint32 prevSize = m_triangleColours.Size();
			m_triangleColours.Grow( meshNtris );
			//Color rgb( IRand(0,255), IRand(0,255), IRand(0,255) );
			for ( Uint32 i = prevSize; i < m_triangleColours.Size(); ++i )
			{
				m_triangleColours[ i ] = (polyMesh->areas[ j ] == Local::AREA_BASE) ?  j : 1000 + j;
			}
		}
#endif

		const Float* verts = &polyMeshDetail->verts[baseVerts*3];
		const Uint8* meshTris = &polyMeshDetail->tris[baseMeshTri*4];

		// Push vertices
		verticesWithDupes.Grow( vertsCount );
		for ( Uint32 vertId = 0; vertId < vertsCount; ++vertId )
		{
			const Float* recastVert = &verts[ vertId * 3 ];
			Vector3 v(
				recastVert[ 0 ],
				recastVert[ 2 ],
				recastVert[ 1 ]
			);

			verticesWithDupes[ baseVertIndex + vertId ] = v;
		}

		// Iterate the sub-mesh's triangles.
		for (Uint32 k = 0; k < meshNtris; ++k)
		{
			Uint32 ind1 = meshTris[k*4+0];
			Uint32 ind2 = meshTris[k*4+2];
			Uint32 ind3 = meshTris[k*4+1];

			// filter fuckedup triangles
			const Vector3& v1 = verticesWithDupes[ baseVertIndex + ind1 ];
			const Vector3& v2 = verticesWithDupes[ baseVertIndex + ind2 ];
			const Vector3& v3 = verticesWithDupes[ baseVertIndex + ind3 ];

			Vector3 triNormal = ( v2 - v1 ).Cross( v3 - v1 );						// calculate triangle normal (not normalized)
			if ( Abs( triNormal.Z ) < NumericLimits< Float >::Epsilon() )
			{
				// Triangle normal is horizontal - skip this triangle
				continue;
			}
			if ( triNormal.Z < 0.f)
			{
				// triangle normal is pointing down
				// we could either skip the triangle, or correct the normal
				// as usually its some fucked up situation, for now we decide just to skip them
				continue;
			}

			triangleIndices.Grow( 3 );

			triangleIndices[currIndex++] = ind1+baseVertIndex;
			triangleIndices[currIndex++] = ind2+baseVertIndex;
			triangleIndices[currIndex++] = ind3+baseVertIndex;
		}

		// connection detection algorithm
		if ( (polyMesh->flags[ j ] & Local::POLY_REACHABLE) == 0 && m_params.m_detectTerrainConnection )
		{
			m_connectorEdges.PushBack( ConnectorEdge( Uint16(baseTriIndex/3), Uint16(currIndex/3) ) );
		}
	}

	if ( verticesWithDupes.Empty() || triangleIndices.Empty() )
	{
		NoticeError( TXT("Empty navmesh geometry computed!\n") );
		return false;
	}

	if ( triangleIndices.Size() / 3 > PathLib::CNavmesh::TRIANGLE_LIMIT )
	{
		NoticeError( String::Printf( TXT("Too many triangles in generated navmesh (%d / %d)! Please modify generation parameters."), triangleIndices.Size() / 3, PathLib::CNavmesh::TRIANGLE_LIMIT ) );
		return false;
	}

	m_currentGenerationTask = ENavGen_ExportGeometry;

	// Establish vertex duplicates and store them for reference
	typedef TArrayMap<Uint32, Uint32> tIndexChangeSet;
	tIndexChangeSet indexChanges;
	// Determine double vertexes. This means to fill up indexChanges data.
	{
		struct UsedVertex
		{
			Vector3			m_v;
			Uint32			m_ind;
			Uint16			m_useCount;
			void Init( const Vector3& v, Uint32 i )
			{
				m_v = v;
				m_ind = i;
				m_useCount = 0;
			}
		};
		struct SVectorComparator
		{
			static RED_INLINE Bool Less( const UsedVertex& val1, const UsedVertex& val2 ) 
			{
				const Vector3& key1 = val1.m_v;
				const Vector3& key2 = val2.m_v;

				return key1.X < key2.X ? true :
					key1.X > key2.X ? false :
					key1.Y < key2.Y ? true :
					key1.Y > key2.Y ? false :
					key1.Z < key2.Z ? true :
					key1.Z > key2.Z ? false :
					val1.m_ind < val2.m_ind;
			}	
		};

		typedef TSortedArray< UsedVertex, SVectorComparator > tVertexSet;
		tVertexSet usedVertices;

		usedVertices.Resize( verticesWithDupes.Size() );
		for ( Uint32 nVert = 0; nVert < verticesWithDupes.Size(); ++nVert )
		{
			usedVertices[ nVert ].Init( verticesWithDupes[ nVert ], nVert );
		}

		for ( Uint32 i = 0, n = triangleIndices.Size(); i != n; ++i )
		{
			++usedVertices[ triangleIndices[ i ] ].m_useCount;
		}
		
		// sort all vertices to quickly get duplicates
		usedVertices.Sort();
		// get duplicates by iterating over sorted vertex array
		Uint32 lastIndex = usedVertices[ 0 ].m_ind;
		for ( Uint32 i = 1, n = usedVertices.Size(); i != n; ++i )
		{
			// check if vertice is duplicate
			if ( usedVertices[ i ].m_useCount == 0 )
			{
				indexChanges.PushBack( TPair<Uint32, Uint32>( usedVertices[ i ].m_ind, 0xffffffff ) );
			}
			else if( (usedVertices[ i ].m_v - usedVertices[ i - 1 ].m_v).IsZero() )
			{
				// store new duplicate
				indexChanges.PushBack( TPair<Uint32, Uint32>( usedVertices[ i ].m_ind, lastIndex ) );
			}
			else
			{
				// mark our index for possibly duplicate
				lastIndex = usedVertices[ i ].m_ind;
			}
		}
	}
	Uint32 simplifiedVertexCount = verticesWithDupes.Size() - indexChanges.Size();
	if ( simplifiedVertexCount > 0xffff )
	{
		NoticeError( String::Printf( TXT("Too many vertexes in generated navmesh (%d / %d)! Please modify generation parameters."), simplifiedVertexCount, PathLib::CNavmesh::VERTEX_LIMIT ) );
		return false;
	}

	indexChanges.Sort();

	// Get rid of dupes and shift vertices
	VertexIndex shift = 0;
	m_vertsOutput.Resize( simplifiedVertexCount );
	TDynArray< Uint16 > indexShifts;
	indexShifts.Resize( verticesWithDupes.Size() );
	auto changedEnd = indexChanges.End();
	for (Uint32 nVert = 0; nVert < verticesWithDupes.Size(); ++nVert)
	{
		auto changedIter = indexChanges.Find( nVert );
		if( changedIter != changedEnd)
		{
			++shift;

			indexShifts[ nVert ] =
				(changedIter->m_second != 0xffffffff )
				? indexShifts[ changedIter->m_second ]
				: 0xffff;
			continue;
		}

		Uint32 shiftedInd = nVert-shift;
		PATHLIB_ASSERT( shiftedInd < 0xffff );
		indexShifts[ nVert ] = Uint16(shiftedInd);
		// store vertex
		m_vertsOutput[ shiftedInd ] = verticesWithDupes[ nVert ];
	}		

	m_trisOutput.Resize( triangleIndices.Size() );
	// Change triangle indices to point to non-duplicate vertices
	for ( Uint32 i = 0, n = triangleIndices.Size(); i < n; ++i )
	{
		m_trisOutput[i] = indexShifts[ triangleIndices[i] ];
	}

	EngineTime endTime = EngineTime::GetNow();

	PATHLIB_LOG( TXT("Navmesh built in %f, with %d vertices and %d indexes\n"), Float(endTime - startTime), m_vertsOutput.Size(), m_trisOutput.Size() );

	return true;
}

////////////////////////////////////////////////////////////////////////////
// CNavmeshGenerationJob
////////////////////////////////////////////////////////////////////////////

CNavmeshGenerationJob::CNavmeshGenerationJob( CGenerationInputData& inputData )
	: m_isSuccess( false )
	, m_navmesh( nullptr )
{
	m_generator = new CGenerator( inputData );
}
CNavmeshGenerationJob::~CNavmeshGenerationJob()
{
	delete m_generator;
	if ( m_navmesh )
	{
		delete m_navmesh;
	}
}

Bool CNavmeshGenerationJob::Init()
{
	m_navmesh = new PathLib::CNavmesh();

	return true;
}

CNavmeshGenerationJob* CNavmeshGenerationJob::CreateGenerationJob( CGenerationInputData& inputData )
{
	CNavmeshGenerationJob* job = new ( CTask::Root ) CNavmeshGenerationJob( inputData );

	if ( !job->m_generator->Initialize( inputData ) )
	{
		job->m_generator->ReportErrors();
		job->Release();
		return NULL;
	}

	if ( !job->Init() ) 
	{
		job->Release();
		return NULL;
	}

	return job;
}

void CNavmeshGenerationJob::Run()
{
	PC_SCOPE_PIX( CNavmeshGenerationJob );

	EngineTime startTime = EngineTime::GetNow();
	if ( m_generator->Generate() )
	{
		PATHLIB_LOG( TXT("Post processing navmesh\n") );
		PATHLIB_ASSERT( m_navmesh );

		if ( m_generator->HadProblems() )
		{
			m_navmesh->NoticeProblems( m_generator->GetProblems() );
		}

		m_navmesh->InitializeMesh( m_generator->m_vertsOutput, m_generator->m_trisOutput );

		const auto& connectorEdges = m_generator->m_connectorEdges;

		for ( auto it = connectorEdges.Begin(), end = connectorEdges.End(); it != end; ++it )
		{
			for ( PathLib::CNavmesh::TriangleIndex triIndex = it->m_indBegin; triIndex < it->m_indEnd; ++triIndex )
			{
				m_navmesh->MarkPhantomEdge( triIndex, 0 );
				m_navmesh->MarkPhantomEdge( triIndex, 1 );
				m_navmesh->MarkPhantomEdge( triIndex, 2 );
			}
		}

#ifdef DEBUG_NAVMESH_COLORS
		{
			m_navmesh->m_triangleColours = m_generator->m_triangleColours;
		}
#endif

		m_isSuccess = true;
		EngineTime endTime = EngineTime::GetNow();
		PATHLIB_LOG( TXT("Navmesh processing completed. Overal duration: %f\n"), Float(endTime - startTime) );
	}
	else
	{
		m_generator->ReportErrors();
	}
}

void CNavmeshGenerationJob::StopRecursiveGeneration() volatile
{}
Bool CNavmeshGenerationJob::IsRunningRecursiveGeneration() const volatile
{
	return false;
}

#ifndef NO_DEBUG_PAGES
const Char* CNavmeshGenerationJob::GetDebugName() const
{
	return TXT("Navmesh generation");
}
Uint32 CNavmeshGenerationJob::GetDebugColor() const
{
	return COLOR_UINT32( 0x50, 0xff, 0xa0 );
}

#endif


////////////////////////////////////////////////////////////////////////////
// CNavmeshRecursiveGenerationJob
////////////////////////////////////////////////////////////////////////////
CNavmeshRecursiveGenerationJob::CNavmeshRecursiveGenerationJob( CGenerationInputData& inputData )
	: CNavmeshGenerationJob( inputData )
	, m_isRunningRecursiveGeneration( true )
	, m_inputIterator( NULL )
{
	m_generator->m_params.m_useTerrainInGeneration = false;
	m_generator->m_params.m_detectTerrainConnection = false;
}

CNavmeshRecursiveGenerationJob::~CNavmeshRecursiveGenerationJob()
{
	if ( m_inputIterator )
	{
		delete m_inputIterator;
	}
}

CNavmeshRecursiveGenerationJob* CNavmeshRecursiveGenerationJob::CreateGenerationJob( CGenerationInputData& inputData )
{
	CNavmeshRecursiveGenerationJob* job = new ( CTask::Root ) CNavmeshRecursiveGenerationJob( inputData );

	if ( !job->m_generator->Initialize( inputData ) )
	{
		job->m_generator->ReportErrors();
		job->Release();
		return NULL;
	}

	if ( ! job->Init() )
	{
		job->Release();
		return NULL;
	}

	job->StoreInputIterator( inputData );

	return job;
}

void CNavmeshRecursiveGenerationJob::StopRecursiveGeneration() volatile
{
	m_isRunningRecursiveGeneration = false;
}
Bool CNavmeshRecursiveGenerationJob::IsRunningRecursiveGeneration() const volatile
{
	return m_isRunningRecursiveGeneration;
};



};				// namepsace Recast

#endif
