#include "build.h"
#include "bitmapTexture.h"
#include "clipMap.h"
#include "materialGraph.h"
#include "materialInstance.h"
#include "mesh.h"
#include "meshChunk.h"
#include "meshComponent.h"
#include "meshDataBuilder.h"
#include "meshEnum.h"
#include "renderFrame.h"
#include "umbraIncludes.h"
#include "umbraTile.h"
#include "umbraScene.h"
#include "umbraStructures.h"
#include "world.h"
#include "decalComponent.h"
#include "materialRootBlock.h"
#include "../core/configVar.h"
#include "../core/configVarLegacyWrapper.h"
#include "../core/events.h"
#include "../core/feedback.h"
#include "../core/ioTags.h"
#include "../core/sortedmap.h"
#include "../core/taskManager.h"
#include "../core/versionControl.h"
#include "../redIO/redIO.h"

#ifdef USE_UMBRA

IMPLEMENT_ENGINE_CLASS( CUmbraTile );
IMPLEMENT_RTTI_ENUM( EUmbraTileDataStatus );

const AllocateFunction tileAlloc = []( Uint32 size, Uint16 alignment )
{
	return RED_MEMORY_ALLOCATE( MemoryPool_Umbra, MC_UmbraBuffer, size );
};
const DeallocateFunction tileDealloc = []( void * ptr )
{
	RED_MEMORY_FREE( MemoryPool_Umbra, MC_UmbraBuffer, ptr );
};

//////////////////////////////////////////////////////////////////////////
CUmbraTile::CUmbraTile()
	: m_parentScene( nullptr )
	, m_tileId( 0 )
#ifndef NO_UMBRA_DATA_GENERATION
	, m_minZ( FLT_MAX )
	, m_maxZ( -FLT_MAX )
	, m_bbMin( FLT_MAX, FLT_MAX, FLT_MAX )
	, m_bbMax( -FLT_MAX, -FLT_MAX, -FLT_MAX )
#if defined(USE_UMBRA_COOKING)
	, m_scene( nullptr )
	, m_overriddenVolumeId( 0 )
#endif //USE_UMBRA_COOKING
#endif //!NO_UMBRA_DATA_GENERATION
	, m_dataStatus( EUTDS_Unknown )
	, m_tome( nullptr )
	, m_tomeRefCount( 0 )
	, m_dataSize( 0 )
{
	m_data.SetAllocateFunction( tileAlloc );
	m_data.SetDeallocateFunction( tileDealloc );
}

CUmbraTile::~CUmbraTile()
{
	RED_ASSERT( m_tomeRefCount.GetValue() == 0 );

#if defined(USE_UMBRA_COOKING) && !defined(NO_UMBRA_DATA_GENERATION)
	if ( m_scene )
	{
		m_scene->release();
		m_scene = nullptr;
	}
#endif //USE_UMBRA_COOKING && !NO_UMBRA_DATA_GENERATION
	if ( m_tome )
	{
		m_handle.Reset();
		m_tome = nullptr;
	}
}

void CUmbraTile::Initialize( CUmbraScene* parentScene, const VectorI& coordinates, Uint16 tileId )
{
	m_parentScene = parentScene;
	m_tileId = tileId;
	m_coordinates = coordinates;
#if defined(USE_UMBRA_COOKING) && !defined(NO_UMBRA_DATA_GENERATION)
	m_overriddenVolumeId = 0;
	m_volumeParameterOverrides.Clear();
	if ( m_scene )
	{
		m_scene->release();
		m_scene = nullptr;
	}
	m_scene = Umbra::Scene::create();
#endif //USE_UMBRA_COOKING
	//LOG_ENGINE( TXT("OCache size [%d;%d]: %d"), m_coordinates.X, m_coordinates.Y, m_objectCache.Size() );
}

void CUmbraTile::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() || file.IsMapper() )
	{
		return;
	}

#if 0
	if ( file.IsWriter() )
	{
		Uint32 id = 0;
		LOG_ENGINE( TXT("[%d; %d] object cache: %d elements"), m_coordinates.X, m_coordinates.Y, m_objectCache.Size() );
		for ( auto it = m_objectCache.Begin(), itEnd = m_objectCache.End(); it != itEnd; ++it )
		{
			LOG_ENGINE( TXT("  %d: [%") RED_PRIWu64 TXT(" -> %u]"), id++, it.Key(), it.Value() );
		}
	}
#endif

	if ( file.GetVersion() < VER_UMBRA_USES_DEFERRED_DATA_BUFFERS )
	{
		SerializeDeferredDataBufferAsLatentDataBufferData( file, m_data );
	}

	if ( file.GetVersion() >= VER_UMBRA_SWITCHED_TO_HASHMAPS )
	{
		file << m_objectCache;
	}
	else
	{
		TSortedMap< TObjectCacheKeyType, TObjectIdType > oldMap;
		file << oldMap;

		m_objectCache.Reserve( oldMap.Size() );
		for ( auto it = oldMap.Begin(), end = oldMap.End(); it != end; ++it )
		{
			m_objectCache.Insert( it->m_first, it->m_second );
		}
	}

	// computation parameters
#ifndef NO_UMBRA_DATA_GENERATION
	file << m_computationParameters.m_smallestHole;
	file << m_computationParameters.m_smallestOccluder;
	file << m_computationParameters.m_tileSize;
	file << m_computationParameters.m_umbraTilesCountPerEdgeOfStreamingTile;
	file << m_computationParameters.m_umbraTileSize;
#endif
}

#ifndef NO_UMBRA_DATA_GENERATION
#ifdef USE_UMBRA_COOKING
enum EObjectType
{
	OT_Occluder,
	OT_Target,
	OT_Gate,
	OT_Volume,
	OT_Invalid,
};

static void ModifyDistancesWithFOVCorrection( Float& lodNear, Float& lodFar )
{
	// Take FOV into account
	// const Float defaultFOV = 70.0f;
	// const Float desiredFOV = 60.0f;
	// Float tanOfDefaultFOV = 0.700207651f; // Red::Math::MTan( 0.5f * DEG2RAD( defaultFOV ) );
	// Float tanOfDesiredFOV = 0.577350259f; // Red::Math::MTan( 0.5f * DEG2RAD( desiredFOV ) );
	const Float fovFactor = 0.824541509f; // tanOfDesiredFOV / tanOfDefaultFOV;
	//lodNear /= fovFactor;
	lodFar /= fovFactor;
}

EObjectType BuildMeshData( const CMesh* mesh, Uint8 targetLOD, Uint32 flags,
					TDynArray< Float >& vertices, TDynArray< Uint32 >& indices,
					Uint32& vertexCount, Uint32& triangleCount )
{
	const Bool isVolumeOnly = ( ( flags & CCF_VolumeOnly ) != 0 );

	TDynArray< Float > occluderVertices;
	TDynArray< Uint32 > occluderIndices;
	Uint32 occluderVertexOffset	= 0;
	Uint32 occluderVertexCount = 0;
	Uint32 occluderTriangleCount = 0;
	
	TDynArray< Float > targetVertices;
	TDynArray< Uint32 > targetIndices;
	Uint32 targetVertexOffset = 0;
	Uint32 targetVertexCount = 0;
	Uint32 targetTriangleCount = 0;

	TDynArray< Uint32 > volumeChunkIds;

	const CMeshData data( mesh );
	const auto& chunks = data.GetChunks();
	const CMesh::LODLevel& lodLevel = mesh->GetMeshLODLevels()[ targetLOD ];
	Uint32 numberOfChunks = lodLevel.m_chunks.Size();
	for ( Uint32 chunk_i = 0; chunk_i < numberOfChunks; ++chunk_i )
	{
		Uint16 chunkIndex = lodLevel.m_chunks[ chunk_i ];
		RED_ASSERT( chunkIndex < chunks.Size() );
		const auto& chunk = chunks[ chunkIndex ];

		if ( chunk.m_numVertices == 0 || chunk.m_numIndices == 0 )
		{
			continue;
		}

		if ( chunk.m_vertexType != MVT_StaticMesh )
		{
			// We don't let non-static meshes in here
			continue;
		}

		const CMeshTypeResource::TMaterials& materials = mesh->GetMaterials();
		if ( chunk.m_materialID >= materials.Size() )
		{
			// corrupted mesh - using non-existing materials
			continue;
		}

		IMaterial* chunkMaterial = materials[ chunk.m_materialID ].Get();
		if ( !chunkMaterial )
		{
			// don't take chunk into account if it has no material
			continue;
		}

		IMaterialDefinition* materialDefinition = chunkMaterial->GetMaterialDefinition();
		if ( !materialDefinition )
		{
			// don't take chunk into account if it has no material definition
			continue;
		}

		if ( materialDefinition->IsUsedByVolumeRendering() )
		{
			// do not use volume chunks for culling
			volumeChunkIds.PushBack( chunk_i );
			continue;
		}

		Bool masked = false;
		if ( CMaterialRootBlock* rootBlock = static_cast< CMaterialGraph* >( materialDefinition )->GetRootBaseBlock() )
		{
			// the deferred materials are always assumed to be non-masked, take the mask property into consideration
			// only when material is non-deferred
			if ( !rootBlock->IsDeferred() )
			{
				masked = chunkMaterial->IsMasked();
			}
		}

		Bool blendMode = materialDefinition->GetRenderingBlendMode() != RBM_None;
		Bool sortGroupBackgroundDepthWrite = materialDefinition->GetRenderingSortGroup() == RSG_RefractiveBackgroundDepthWrite;
		Bool sortGroupBackground = materialDefinition->GetRenderingSortGroup() == RSG_RefractiveBackground || materialDefinition->GetRenderingSortGroup() == RSG_TransparentBackground;

		Bool targetOnly = blendMode || masked || sortGroupBackgroundDepthWrite || sortGroupBackground;
		targetOnly |= targetVertexCount > 0; // TEMPHACK : If we've already started with a "target" mesh, only build that.
		if ( flags & CCF_TargetOnly )
		{
			targetOnly = true;
		}

		// alpha-tested and alpha-blended objects should not be used as occluders
		if ( targetOnly )
		{
			// TEMPHACK : If we already have occluder vertices, move them to the target data.
			if ( occluderVertexCount > 0 )
			{
				targetIndices.PushBack( occluderIndices );
				targetVertices.PushBack( occluderVertices );

				targetVertexOffset = occluderVertexOffset;
				targetVertexCount = occluderVertexCount;
				targetTriangleCount = occluderTriangleCount;

				occluderIndices.Clear();
				occluderVertices.Clear();
				occluderTriangleCount = 0;
				occluderVertexCount = 0;
			}

			for ( Uint32 i = 0; i < chunk.m_vertices.Size(); ++i )
			{
				targetVertices.PushBack( chunk.m_vertices[ i ].m_position[0] );
				targetVertices.PushBack( chunk.m_vertices[ i ].m_position[1] );
				targetVertices.PushBack( chunk.m_vertices[ i ].m_position[2] );
			}

			targetVertexCount += chunk.m_numVertices;

			RED_ASSERT( chunk.m_indices.Size() % 3 == 0 );
			Uint32 chunkTriangleCount = chunk.m_indices.Size() / 3;

			for ( Uint32 i = 0; i < chunkTriangleCount; ++i )
			{
				Uint32 t0 = (Uint32)chunk.m_indices[ i * 3 + 0 ];
				Uint32 t1 = (Uint32)chunk.m_indices[ i * 3 + 1 ];
				Uint32 t2 = (Uint32)chunk.m_indices[ i * 3 + 2 ];

				targetIndices.PushBack( t0 + targetVertexOffset );
				targetIndices.PushBack( t1 + targetVertexOffset );
				targetIndices.PushBack( t2 + targetVertexOffset );
			}

			targetVertexOffset += chunk.m_numVertices;
			targetTriangleCount += chunk.m_numIndices / 3;
		}
		else
		{
			for ( Uint32 i = 0; i < chunk.m_vertices.Size(); ++i )
			{
				occluderVertices.PushBack( chunk.m_vertices[ i ].m_position[0] );
				occluderVertices.PushBack( chunk.m_vertices[ i ].m_position[1] );
				occluderVertices.PushBack( chunk.m_vertices[ i ].m_position[2] );
			}

			occluderVertexCount += chunk.m_numVertices;

			RED_ASSERT( chunk.m_indices.Size() % 3 == 0 );
			Uint32 chunkTriangleCount = chunk.m_indices.Size() / 3;

			for ( Uint32 i = 0; i < chunkTriangleCount; ++i )
			{
				Uint32 t0 = (Uint32)chunk.m_indices[ i * 3 + 0 ];
				Uint32 t1 = (Uint32)chunk.m_indices[ i * 3 + 1 ];
				Uint32 t2 = (Uint32)chunk.m_indices[ i * 3 + 2 ];

				occluderIndices.PushBack( t0 + occluderVertexOffset );
				occluderIndices.PushBack( t1 + occluderVertexOffset );
				occluderIndices.PushBack( t2 + occluderVertexOffset );
			}

			occluderVertexOffset += chunk.m_numVertices;
			occluderTriangleCount += chunk.m_numIndices / 3;
		}
	}

	Bool targetToInsert = targetTriangleCount > 0;
	Bool occluderToInsert = occluderTriangleCount > 0;
	Bool volumeToInsert = volumeChunkIds.Size() > 0;

	// in case the mesh is explicitly marked as volume only or we have the situation that there are only volume chunks
	if ( isVolumeOnly || (!targetToInsert && !occluderToInsert && volumeToInsert) )
	{
		// only volume chunks
		TDynArray< Float > volumeVertices;
		TDynArray< Uint32 > volumeIndices;
		Uint32 volumeVertexOffset = 0;
		Uint32 volumeVertexCount = 0;
		Uint32 volumeTriangleCount = 0;

		for ( Uint32 volumeChunkId = 0; volumeChunkId < volumeChunkIds.Size(); ++volumeChunkId )
		{
			Uint16 chunkIndex = lodLevel.m_chunks[ volumeChunkId ];
			RED_ASSERT( chunkIndex < chunks.Size() );
			const auto& chunk = chunks[ chunkIndex ];
			for ( Uint32 i = 0; i < chunk.m_vertices.Size(); ++i )
			{
				volumeVertices.PushBack( chunk.m_vertices[ i ].m_position[0] );
				volumeVertices.PushBack( chunk.m_vertices[ i ].m_position[1] );
				volumeVertices.PushBack( chunk.m_vertices[ i ].m_position[2] );
			}
			volumeVertexCount += chunk.m_numVertices;

			ASSERT( chunk.m_indices.Size() % 3 == 0 );
			Uint32 chunkTriangleCount = chunk.m_indices.Size() / 3;

			for ( Uint32 i = 0; i < chunkTriangleCount; ++i )
			{
				Uint32 t0 = (Uint32)chunk.m_indices[ i * 3 + 0 ];
				Uint32 t1 = (Uint32)chunk.m_indices[ i * 3 + 1 ];
				Uint32 t2 = (Uint32)chunk.m_indices[ i * 3 + 2 ];

				volumeIndices.PushBack( t0 + volumeVertexOffset );
				volumeIndices.PushBack( t1 + volumeVertexOffset );
				volumeIndices.PushBack( t2 + volumeVertexOffset );
			}
			volumeVertexOffset += chunk.m_numVertices;
			volumeTriangleCount += chunk.m_numIndices / 3;
		}
		
		vertices = volumeVertices;
		indices = volumeIndices;
		vertexCount = volumeVertexCount;
		triangleCount = volumeTriangleCount;
		return OT_Volume;
	}

	if ( targetVertices.Size() )
	{
		vertices = targetVertices;
		indices = targetIndices;
		vertexCount = targetVertexCount;
		triangleCount = targetTriangleCount;
		return OT_Target;
	}
	
	if ( occluderVertices.Size() )
	{
		vertices = occluderVertices;
		indices = occluderIndices;
		vertexCount = occluderVertexCount;
		triangleCount = occluderTriangleCount;
		return OT_Occluder;
	}

	return OT_Invalid;
}

//////////////////////////////////////////////////////////////////////////
void BuildBoxGeometry( const Box& box, TDynArray< Float >& vertices, Uint32& numberOfVertices, TDynArray< Uint32 >& indices, Uint32& numberOfTriangles )
{
	// front wall (Front-Top-Left)
	Vector vFTL( box.Min.X, box.Min.Y, box.Max.Z );
	Vector vFTR( box.Max.X, box.Min.Y, box.Max.Z );
	Vector vFBL( box.Min.X, box.Min.Y, box.Min.Z );
	Vector vFBR( box.Max.X, box.Min.Y, box.Min.Z );

	vertices.PushBack( vFTL.X ); vertices.PushBack( vFTL.Y ); vertices.PushBack( vFTL.Z );
	vertices.PushBack( vFTR.X ); vertices.PushBack( vFTR.Y ); vertices.PushBack( vFTR.Z );
	vertices.PushBack( vFBL.X ); vertices.PushBack( vFBL.Y ); vertices.PushBack( vFBL.Z );
	vertices.PushBack( vFBR.X ); vertices.PushBack( vFBR.Y ); vertices.PushBack( vFBR.Z );

	// back wall (Back-Top-Left)
	Vector vBTL( box.Min.X, box.Max.Y, box.Max.Z );
	Vector vBTR( box.Max.X, box.Max.Y, box.Max.Z );
	Vector vBBL( box.Min.X, box.Max.Y, box.Min.Z );
	Vector vBBR( box.Max.X, box.Max.Y, box.Min.Z );

	vertices.PushBack( vBTL.X ); vertices.PushBack( vBTL.Y ); vertices.PushBack( vBTL.Z );
	vertices.PushBack( vBTR.X ); vertices.PushBack( vBTR.Y ); vertices.PushBack( vBTR.Z );
	vertices.PushBack( vBBL.X ); vertices.PushBack( vBBL.Y ); vertices.PushBack( vBBL.Z );
	vertices.PushBack( vBBR.X ); vertices.PushBack( vBBR.Y ); vertices.PushBack( vBBR.Z );

	// FTL: 0, FTR: 1, FBL: 2, FBR: 3
	// BTL: 4, BTR: 5, BBL: 6, BBR: 7

	// front wall
	indices.PushBack( 0 );	// FTL
	indices.PushBack( 2 );	// FBL
	indices.PushBack( 1 );	// FTR
	indices.PushBack( 1 );	// FTR
	indices.PushBack( 2 );	// FBL
	indices.PushBack( 3 );	// FBR

	// right wall
	indices.PushBack( 1 );	// FTR
	indices.PushBack( 3 );	// FBR
	indices.PushBack( 5 );	// BTR
	indices.PushBack( 5 );	// BTR
	indices.PushBack( 3 );	// FBR
	indices.PushBack( 7 );	// BBR

	// right wall
	indices.PushBack( 4 );	// BTL
	indices.PushBack( 5 );	// BTR
	indices.PushBack( 6 );	// BBL
	indices.PushBack( 6 );	// BBL
	indices.PushBack( 5 );	// BTR
	indices.PushBack( 7 );	// BBR

	// left wall
	indices.PushBack( 4 );	// BTL
	indices.PushBack( 6 );	// BBL
	indices.PushBack( 0 );	// FTL
	indices.PushBack( 0 );	// FTL
	indices.PushBack( 6 );	// BBL
	indices.PushBack( 2 );	// FBL

	// top wall
	indices.PushBack( 4 );	// BTL
	indices.PushBack( 0 );	// FTL
	indices.PushBack( 5 );	// BTR
	indices.PushBack( 5 );	// BTR
	indices.PushBack( 0 );	// FTL
	indices.PushBack( 1 );	// FTR

	// bottom wall
	indices.PushBack( 2 );	// FBL
	indices.PushBack( 6 );	// BBL
	indices.PushBack( 3 );	// FBR
	indices.PushBack( 3 );	// FBR
	indices.PushBack( 6 );	// BBL
	indices.PushBack( 7 );	// BBR

	numberOfVertices = vertices.Size();
	RED_ASSERT( indices.Size() % 3 == 0 );
	numberOfTriangles = indices.Size() / 3;
}

void BuildBoundingBoxGeometry( TDynArray< Float >& vertices, Uint32& numberOfVertices, TDynArray< Uint16 >& indices, Uint32& numberOfTriangles, Float extent = 1.0f )
{
	enum AXIS
	{
		AXIS_X = 0,
		AXIS_Y = 1,
		AXIS_Z = 2,
	};
	
	struct LocalBuilder
	{
		TDynArray< Float >&		m_vertices;
		TDynArray< Uint16 >&	m_indices;
		Float					m_extent;
		Uint32					m_currentNumberOfVertices;

		LocalBuilder( TDynArray< Float >& vertices, TDynArray< Uint16 >& indices, Float extent )
			: m_vertices( vertices )
			, m_indices( indices )
			, m_extent( extent )
			, m_currentNumberOfVertices( 0 )
		{}

		void AppendFaces( AXIS axis, const Vector& tl, const Vector& tr, const Vector& bl, const Vector& br )
		{
			enum CORNER
			{
				TL = 0,
				TR = 1,
				BL = 2,
				BR = 3,
			};

			Vector m_tl( tl );
			Vector m_tr( tr );
			Vector m_bl( bl );
			Vector m_br( br );

			m_tl.A[ axis ] = -m_extent;
			m_tr.A[ axis ] = -m_extent;
			m_bl.A[ axis ] = -m_extent;
			m_br.A[ axis ] = -m_extent;
			
			m_vertices.PushBack( m_tl.X );
			m_vertices.PushBack( m_tl.Y );
			m_vertices.PushBack( m_tl.Z );

			m_vertices.PushBack( m_tr.X );
			m_vertices.PushBack( m_tr.Y );
			m_vertices.PushBack( m_tr.Z );

			m_vertices.PushBack( m_bl.X );
			m_vertices.PushBack( m_bl.Y );
			m_vertices.PushBack( m_bl.Z );

			m_vertices.PushBack( m_br.X );
			m_vertices.PushBack( m_br.Y );
			m_vertices.PushBack( m_br.Z );

			m_indices.PushBack( m_currentNumberOfVertices + TR );
			m_indices.PushBack( m_currentNumberOfVertices + BL );
			m_indices.PushBack( m_currentNumberOfVertices + TL );

			m_indices.PushBack( m_currentNumberOfVertices + TR );
			m_indices.PushBack( m_currentNumberOfVertices + BR );
			m_indices.PushBack( m_currentNumberOfVertices + BL );

			m_currentNumberOfVertices += 4;

			m_tl.A[ axis ] = m_extent;
			m_tr.A[ axis ] = m_extent;
			m_bl.A[ axis ] = m_extent;
			m_br.A[ axis ] = m_extent;

			m_vertices.PushBack( m_tl.X );
			m_vertices.PushBack( m_tl.Y );
			m_vertices.PushBack( m_tl.Z );

			m_vertices.PushBack( m_tr.X );
			m_vertices.PushBack( m_tr.Y );
			m_vertices.PushBack( m_tr.Z );

			m_vertices.PushBack( m_bl.X );
			m_vertices.PushBack( m_bl.Y );
			m_vertices.PushBack( m_bl.Z );

			m_vertices.PushBack( m_br.X );
			m_vertices.PushBack( m_br.Y );
			m_vertices.PushBack( m_br.Z );

			m_indices.PushBack( m_currentNumberOfVertices + TL );
			m_indices.PushBack( m_currentNumberOfVertices + BR );
			m_indices.PushBack( m_currentNumberOfVertices + TR );

			m_indices.PushBack( m_currentNumberOfVertices + TL );
			m_indices.PushBack( m_currentNumberOfVertices + BL );
			m_indices.PushBack( m_currentNumberOfVertices + BR );

			m_currentNumberOfVertices += 4;
		}
	} builder( vertices, indices, extent );

	// append along X axis
	builder.AppendFaces( AXIS_X, Vector( 0.f,  extent,  extent ), /*TopLeft*/
								 Vector( 0.f, -extent,  extent ), /*TopRight*/
								 Vector( 0.f,  extent, -extent ), /*BottomLeft*/
								 Vector( 0.f, -extent, -extent )  /*BottomRight*/ );

	// append along Y axis
	builder.AppendFaces( AXIS_Y, Vector( -extent, 0.f,  extent ), /*TopLeft*/
								 Vector(  extent, 0.f,  extent ), /*TopRight*/
								 Vector( -extent, 0.f, -extent ), /*BottomLeft*/
								 Vector(  extent, 0.f, -extent )  /*BottomRight*/ );

	// append along Z axis
	builder.AppendFaces( AXIS_Z, Vector(  extent,  extent, 0.f ), /*TopLeft*/
								 Vector( -extent,  extent, 0.f ), /*TopRight*/
								 Vector(  extent, -extent, 0.f ), /*BottomLeft*/
								 Vector( -extent, -extent, 0.f )  /*BottomRight*/ );
	
	numberOfVertices = vertices.Size();
	RED_ASSERT( indices.Size() % 3 == 0 );
	numberOfTriangles = indices.Size() / 3;
}

void BuildSphereGeometry( TDynArray< Float >& vertices, Uint32& numberOfVertices, TDynArray< Uint16 >& indices, Uint32& numberOfTriangles, Float radius )
{
	Uint32 width = 32;
	Uint32 height = 16;

	for ( Uint32 t = 0, j = 1; j < height - 1; ++j )
		for( Uint32 i = 0; i < width; ++i )
		{
			Float theta = (Float)j / ((Float)height - 1.0f) * M_PI;
			Float phi   = (Float)i / ((Float)width - 1.0f ) * M_PI * 2.0f;
			
			Float x = Red::Math::MSin( theta ) * Red::Math::MCos( phi ) * radius;
			Float z = Red::Math::MCos( theta ) * radius;
			Float y = -Red::Math::MSin( theta ) * Red::Math::MSin( phi ) * radius;

			vertices.PushBack( x );
			vertices.PushBack( y );
			vertices.PushBack( z );
		}

	vertices.PushBack( 0.0f );
	vertices.PushBack( 0.0f );
	vertices.PushBack( radius );

	vertices.PushBack( 0.0f );
	vertices.PushBack( 0.0f );
	vertices.PushBack( -radius );

	for ( Uint32 t = 0, j = 0; j < height - 3; ++j )
		for ( Uint32 i = 0; i < width - 1; ++i )
		{
			indices.PushBack( (j  )*width + i   );
			indices.PushBack( (j+1)*width + i+1 );
			indices.PushBack( (j  )*width + i+1 );
			indices.PushBack( (j  )*width + i   );
			indices.PushBack( (j+1)*width + i   );
			indices.PushBack( (j+1)*width + i+1 );
		}
	for( Uint32 i = 0; i < width - 1; ++i )
	{
		indices.PushBack( (height-2)*width );
		indices.PushBack( i );
		indices.PushBack( i+1 );
		indices.PushBack( (height-2)*width+1 );
		indices.PushBack( (height-3)*width + i+1 );
		indices.PushBack( (height-3)*width + i );
	}

	numberOfVertices = vertices.Size();
	numberOfTriangles = indices.Size() / 3;
}

void BuildConeGeometry( TDynArray< Float >& vertices, Uint32& numberOfVertices, TDynArray< Uint16 >& indices, Uint32& numberOfTriangles, Float radius, Float innerAngle, Float outerAngle )
{
	TDynArray< Vector > vVertices;
	vVertices.PushBack( Vector::ZEROS );
	const Uint32 numSegments = 18;
	for ( Uint32 i = 0; i < numSegments; ++i )
	{
		const Float angle = (i / (Float)numSegments) * M_PI * 2.0f;
		Vector coneVector = ( Vector::EX * Red::Math::MCos( angle ) ) + ( Vector::EZ * Red::Math::MSin( angle ) );
		vVertices.PushBack( ( Vector::EY * radius * Red::Math::MCos( DEG2RAD( outerAngle * 0.5f  ) ) ) + coneVector * Red::Math::MSin( DEG2RAD( outerAngle * 0.5f ) ) * radius );
	}

	for ( auto& v : vVertices )
	{
		vertices.PushBack( v.X );
		vertices.PushBack( v.Y );
		vertices.PushBack( v.Z );
	}

	// cone from origin to ring
	for ( Uint32 i = 0; i < numSegments - 1; ++i )
	{
		indices.PushBack( 0 );
		indices.PushBack( i + 2 );
		indices.PushBack( i + 1 );
	}
	indices.PushBack( 0 );
	indices.PushBack( 1 );
	indices.PushBack( numSegments );
	
	Uint32 sphereOffset = vVertices.Size();

	// Sphere
	const Int32 numSphereSegments = 3;
	TDynArray< Vector > sphereSegments;
	for ( Int32 i = -numSphereSegments; i <= numSphereSegments; ++i )
	{
		const Float angle = DEG2RAD( 0.5f * outerAngle * i / (Float)numSphereSegments );
		sphereSegments.PushBack( Vector::EY * cos( angle ) * radius + Vector::EZ * sin( angle ) * radius );
		sphereSegments.PushBack( Vector::EY * cos( angle ) * radius + Vector::EX * sin( angle ) * radius );
	}

	for ( auto& v : sphereSegments )
	{
		vertices.PushBack( v.X );
		vertices.PushBack( v.Y );
		vertices.PushBack( v.Z );
	}

	// this is ugly as f*ck - hardcoded non-full sphere aproximation with 2-axis and triangles between those
	// but works out as a rough approximation

	//
	indices.PushBack( sphereOffset + 0 );
	indices.PushBack( sphereOffset + 2 );
	indices.PushBack( sphereOffset + 1 );
	indices.PushBack( sphereOffset + 1 );
	indices.PushBack( sphereOffset + 2 );
	indices.PushBack( sphereOffset + 3 );

	indices.PushBack( sphereOffset + 2 );
	indices.PushBack( sphereOffset + 4 );
	indices.PushBack( sphereOffset + 3 );
	indices.PushBack( sphereOffset + 3 );
	indices.PushBack( sphereOffset + 4 );
	indices.PushBack( sphereOffset + 5 );

	indices.PushBack( sphereOffset + 4 );
	indices.PushBack( sphereOffset + 6 );
	indices.PushBack( sphereOffset + 5 );
	indices.PushBack( sphereOffset + 5 );
	indices.PushBack( sphereOffset + 6 );
	indices.PushBack( sphereOffset + 7 );
	//
	indices.PushBack( sphereOffset + 1 );
	indices.PushBack( sphereOffset + 3 );
	indices.PushBack( sphereOffset + 12 );
	indices.PushBack( sphereOffset + 12 );
	indices.PushBack( sphereOffset + 3 );
	indices.PushBack( sphereOffset + 10 );
	
	indices.PushBack( sphereOffset + 3 );
	indices.PushBack( sphereOffset + 5 );
	indices.PushBack( sphereOffset + 10 );
	indices.PushBack( sphereOffset + 10 );
	indices.PushBack( sphereOffset + 5 );
	indices.PushBack( sphereOffset + 8 );

	indices.PushBack( sphereOffset + 5 );
	indices.PushBack( sphereOffset + 6 );
	indices.PushBack( sphereOffset + 8 );
	indices.PushBack( sphereOffset + 8 );
	indices.PushBack( sphereOffset + 6 );
	indices.PushBack( sphereOffset + 7 );
	//
	indices.PushBack( sphereOffset + 12 );
	indices.PushBack( sphereOffset + 10 );
	indices.PushBack( sphereOffset + 13 );
	indices.PushBack( sphereOffset + 13 );
	indices.PushBack( sphereOffset + 10 );
	indices.PushBack( sphereOffset + 11 );

	indices.PushBack( sphereOffset + 10 );
	indices.PushBack( sphereOffset + 8 );
	indices.PushBack( sphereOffset + 11 );
	indices.PushBack( sphereOffset + 11 );
	indices.PushBack( sphereOffset + 8 );
	indices.PushBack( sphereOffset + 9 );

	indices.PushBack( sphereOffset + 8 );
	indices.PushBack( sphereOffset + 7 );
	indices.PushBack( sphereOffset + 9 );
	indices.PushBack( sphereOffset + 9 );
	indices.PushBack( sphereOffset + 7 );
	indices.PushBack( sphereOffset + 6 );
	//
	indices.PushBack( sphereOffset + 13 );
	indices.PushBack( sphereOffset + 11 );
	indices.PushBack( sphereOffset + 0 );
	indices.PushBack( sphereOffset + 0 );
	indices.PushBack( sphereOffset + 11 );
	indices.PushBack( sphereOffset + 2 );

	indices.PushBack( sphereOffset + 11 );
	indices.PushBack( sphereOffset + 9 );
	indices.PushBack( sphereOffset + 2 );
	indices.PushBack( sphereOffset + 2 );
	indices.PushBack( sphereOffset + 9 );
	indices.PushBack( sphereOffset + 4 );

	indices.PushBack( sphereOffset + 9 );
	indices.PushBack( sphereOffset + 7 );
	indices.PushBack( sphereOffset + 4 );
	indices.PushBack( sphereOffset + 4 );
	indices.PushBack( sphereOffset + 7 );
	indices.PushBack( sphereOffset + 6 );
	
	numberOfVertices = vertices.Size();
	numberOfTriangles = indices.Size() / 3;
}

//////////////////////////////////////////////////////////////////////////

Bool CUmbraTile::AddMesh( const ObjectInfo& objectInfo, const CMesh* mesh, TDynArray< UmbraObjectInfo >& addedObjects )
{
#ifndef NO_RESOURCE_IMPORT
	const CMesh::TLODLevelArray& lodLevels = mesh->GetMeshLODLevels();

	// choose highest non-empty LOD level (eg. first one can be empty if it's a DistantProxy mesh)
	Uint32 targetLOD = 0;
	for ( ; targetLOD < lodLevels.Size(); ++targetLOD )
	{
		if ( !lodLevels[ targetLOD ].m_chunks.Empty() )
		{
			break;
		}
	}
	if ( mesh->IsEntityProxy() )
	{
		// ALWAYS USE LOD1 FOR PROXY_MESHES
		RED_ASSERT( lodLevels.Size() > 1 );
		if ( lodLevels.Size() <= 1 )
		{
			// error
			return false;
		}
		RED_ASSERT( lodLevels[ 0 ].m_chunks.Empty() );
		RED_ASSERT( !lodLevels[ 1 ].m_chunks.Empty() );
		targetLOD = 1;
	}

	UpdateBounds( objectInfo.m_boundingBox );
	if ( objectInfo.m_flags & CCF_OverrideComputationParameters )
	{
		AddSmallestHoleOverride( objectInfo.m_boundingBox, mesh->GetSmallestHoleOverride() );
	}

	Float lodNear = mesh->GetMeshLODLevels()[ targetLOD ].GetDistance();
	Float lodFar = objectInfo.m_autoHideDistance;

	// Extend far distance a small amount, to allow it to remain rendering a little bit beyond the auto-hide, so it can be faded out.
	lodFar += CMeshComponent::AUTOHIDE_MARGIN;

	// do the adjustment of lodNear and lodFar
	if ( lodNear > 0.0f )
	{
		const Float adjustmentFactor = 0.1f;
		const Float maxAdjustment = 2.0f;

		Float adjustment = Min( lodNear * adjustmentFactor, maxAdjustment );
		lodNear -= adjustment;
	}

	ModifyDistancesWithFOVCorrection( lodNear, lodFar );

	UmbraChunk* umbraChunk	= new UmbraChunk();
	umbraChunk->meshId		= objectInfo.m_id;
	umbraChunk->meshName	= mesh->GetFriendlyName();
	umbraChunk->isTerrain	= false;
	umbraChunk->lavaFlags	= objectInfo.m_flags;
	umbraChunk->twoSided	= true;
	UmbraHelpers::QuantizeDistanceLimits( lodNear, lodFar, umbraChunk->lodNearDistanceQuantized, umbraChunk->lodFarDistanceQuantized );

	Uint64 modelCacheId = UmbraHelpers::CalculateUmbraChunkHash( umbraChunk );
	if ( m_modelCache.KeyExist( modelCacheId ) )
	{
		// chunk already in the modelCache, we can safely discard the current one
		RED_ASSERT( umbraChunk );
		delete umbraChunk;
		umbraChunk = nullptr;
		return AddObject( modelCacheId, objectInfo.m_transform, objectInfo.m_boundingBoxCenter, addedObjects );
	}

	if ( mesh->GetChunks().Empty() )
	{
		// chunk already in the modelCache, we can safely discard the current one
		RED_ASSERT( umbraChunk );
		delete umbraChunk;
		umbraChunk = nullptr;
		return false;
	}

	TDynArray< Float > vertices;
	TDynArray< Uint32 > indices;
	Uint32 vertexCount = 0;
	Uint32 triangleCount = 0;

	EObjectType objectType;

	if ( objectInfo.m_flags & CCF_Gate )
	{
		// just use bounding box
		objectType = OT_Gate;
		BuildBoxGeometry( mesh->GetBoundingBox(), vertices, vertexCount, indices, triangleCount );
	}
	else
	{
		// use mesh geometry from desired LOD
		objectType = BuildMeshData( mesh, targetLOD, objectInfo.m_flags, vertices, indices, vertexCount, triangleCount );
	}

	if ( objectType == OT_Invalid )
	{
		RED_ASSERT( umbraChunk );
		delete umbraChunk;
		umbraChunk = nullptr;
		LOG_ENGINE( TXT("'%ls': invalid object"), mesh->GetDepotPath().AsChar() );
		return false;
	}

	const Umbra::SceneModel* insertedModel = m_scene->insertModel( vertices.TypedData(), indices.TypedData(), vertexCount, triangleCount );
	if ( !insertedModel )
	{
		RED_ASSERT( umbraChunk );
		delete umbraChunk;
		umbraChunk = nullptr;
		LOG_ENGINE( TXT("'%ls': insertModel FAILED"), mesh->GetDepotPath().AsChar() );
		return false;
	}

	umbraChunk->model			= const_cast< Umbra::SceneModel* >( insertedModel );

	String meshType;
	switch ( objectType )
	{
	case OT_Gate:
		umbraChunk->flags		= Umbra::SceneObject::GATE | Umbra::SceneObject::TARGET;
		meshType = TXT("GATE");
		break;
	case OT_Occluder:
		umbraChunk->flags		= Umbra::SceneObject::OCCLUDER | Umbra::SceneObject::TARGET;
		meshType = TXT("OCCLUDER");
		break;
	case OT_Target:
		umbraChunk->flags		= Umbra::SceneObject::TARGET;
		meshType = TXT("TARGET");
		break;
	case OT_Volume:
		umbraChunk->twoSided	= false; // volume meshes HAVE to be one-sided, faces pointing outside
		umbraChunk->flags		= Umbra::SceneObject::TARGET | Umbra::SceneObject::VOLUME;
		meshType = TXT("VOLUME");
		break;
	}

	LOG_ENGINE( TXT("%") RED_PRIWu64 TXT(" '%ls': %ls, [%d vertices, %d triangles]"), modelCacheId, mesh->GetDepotPath().AsChar(), meshType.AsChar(), vertexCount, triangleCount );

	m_modelCache.Insert( modelCacheId, umbraChunk );
	return AddObject( modelCacheId, objectInfo.m_transform, objectInfo.m_boundingBoxCenter, addedObjects );
#endif

	return false;
}

Bool CUmbraTile::AddDecal( const ObjectInfo& objectInfo, TDynArray< UmbraObjectInfo >& addedObjects )
{
	UpdateBounds( objectInfo.m_boundingBox );

	Float autohideDistance = objectInfo.m_autoHideDistance;
	{
		autohideDistance += CDecalComponent::AUTOHIDE_MARGIN;
	}

	UmbraChunk* umbraChunk	= new UmbraChunk();
	umbraChunk->meshId		= objectInfo.m_id;
	umbraChunk->meshName	= String::Printf( TXT("Decal: %u"), objectInfo.m_id );
	umbraChunk->twoSided	= false;
	umbraChunk->flags		= Umbra::SceneObject::TARGET | Umbra::SceneObject::VOLUME;
	umbraChunk->lavaFlags	= objectInfo.m_flags;
	umbraChunk->isTerrain	= false;

	Float lodNear = 0.0f;
	Float lodFar = autohideDistance;
	ModifyDistancesWithFOVCorrection( lodNear, lodFar );
	UmbraHelpers::QuantizeDistanceLimits( lodNear, lodFar, umbraChunk->lodNearDistanceQuantized, umbraChunk->lodFarDistanceQuantized );
	Uint64 modelCacheId = UmbraHelpers::CalculateUmbraChunkHash( umbraChunk );
	if ( m_modelCache.KeyExist(modelCacheId) )
	{
		RED_ASSERT( umbraChunk );
		delete umbraChunk;
		umbraChunk = nullptr;
		return AddObject( modelCacheId, objectInfo.m_transform, objectInfo.m_boundingBoxCenter, addedObjects );
	}
	
	// just a box
	const Float extent = 0.5f;
	TDynArray< Float > vertices;
	TDynArray< Uint16 > indices;
	Uint32 vertexCount = 0;
	Uint32 triangleCount = 0;
	BuildBoundingBoxGeometry( vertices, vertexCount, indices, triangleCount, extent );
	const Umbra::SceneModel* model = m_scene->insertModel( vertices.TypedData(), indices.TypedData(), vertexCount, triangleCount );
	RED_ASSERT( model );
	umbraChunk->model = const_cast< Umbra::SceneModel* >( model );

	m_modelCache.Insert( modelCacheId, umbraChunk );
	return AddObject( modelCacheId, objectInfo.m_transform, objectInfo.m_boundingBoxCenter, addedObjects );
}

Bool CUmbraTile::AddDimmer( const ObjectInfo& objectInfo, TDynArray< UmbraObjectInfo >& addedObjects )
{
	UpdateBounds( objectInfo.m_boundingBox );

	UmbraChunk* umbraChunk	= new UmbraChunk();
	umbraChunk->meshId		= objectInfo.m_id;
	umbraChunk->meshName	= String::Printf( TXT("Dimmer: %d"), objectInfo.m_id );
	umbraChunk->twoSided	= false;
	umbraChunk->flags		= Umbra::SceneObject::TARGET | Umbra::SceneObject::VOLUME;
	umbraChunk->isTerrain	= false;
	umbraChunk->lavaFlags	= objectInfo.m_flags;

	Float lodNear = 0.0f;
	Float lodFar = objectInfo.m_autoHideDistance;
	ModifyDistancesWithFOVCorrection( lodNear, lodFar );

	UmbraHelpers::QuantizeDistanceLimits( lodNear, lodFar, umbraChunk->lodNearDistanceQuantized, umbraChunk->lodFarDistanceQuantized );
	Uint64 modelCacheId = UmbraHelpers::CalculateUmbraChunkHash( umbraChunk );
	if ( m_modelCache.KeyExist( modelCacheId ) )
	{
		RED_ASSERT( umbraChunk );
		delete umbraChunk;
		umbraChunk = nullptr;
		return AddObject( modelCacheId, objectInfo.m_transform, objectInfo.m_boundingBoxCenter, addedObjects );
	}

	TDynArray< Float > vertices;
	TDynArray< Uint16 > indices;
	Uint32 triangleCount = 0;
	Uint32 vertexCount = 0;
	BuildBoundingBoxGeometry( vertices, vertexCount, indices, triangleCount );
	const Umbra::SceneModel* model = m_scene->insertModel( vertices.TypedData(), indices.TypedData(), vertexCount, triangleCount );
	RED_ASSERT( model );
	umbraChunk->model = const_cast< Umbra::SceneModel* >( model );

	m_modelCache.Insert( modelCacheId, umbraChunk );
	return AddObject( modelCacheId, objectInfo.m_transform, objectInfo.m_boundingBoxCenter, addedObjects );
}

Bool CUmbraTile::AddStripe( const ObjectInfo& objectInfo, const TDynArray< Vector >& vertices, const TDynArray< Uint16 >& indices, TDynArray< UmbraObjectInfo >& addedObjects )
{
	UpdateBounds( objectInfo.m_boundingBox );	

	UmbraChunk* umbraChunk	= new UmbraChunk();
	umbraChunk->meshId		= objectInfo.m_id;
	umbraChunk->meshName	= String::Printf( TXT("Stripe: %d"), objectInfo.m_id );
	umbraChunk->twoSided	= true;
	umbraChunk->flags		= Umbra::SceneObject::TARGET;
	umbraChunk->isTerrain	= false;
	umbraChunk->lavaFlags	= objectInfo.m_flags;

	Float lodNear = 0.0f;
	Float lodFar = objectInfo.m_autoHideDistance;
	ModifyDistancesWithFOVCorrection( lodNear, lodFar );

	UmbraHelpers::QuantizeDistanceLimits( lodNear, lodFar, umbraChunk->lodNearDistanceQuantized, umbraChunk->lodFarDistanceQuantized );
	Uint64 modelCacheId = UmbraHelpers::CalculateUmbraChunkHash( umbraChunk );
	if ( m_modelCache.KeyExist( modelCacheId ) )
	{
		RED_ASSERT( umbraChunk );
		delete umbraChunk;
		umbraChunk = nullptr;
		return AddObject( modelCacheId, objectInfo.m_transform, objectInfo.m_boundingBoxCenter, addedObjects );
	}

	TDynArray< Float > verts;
	for ( Uint32 i = 0; i < vertices.Size(); ++i )
	{
		verts.PushBack( vertices[i].X );
		verts.PushBack( vertices[i].Y );
		verts.PushBack( vertices[i].Z );
	}
	
	const Umbra::SceneModel* model = m_scene->insertModel( verts.TypedData(), indices.TypedData(), verts.Size(), indices.Size() / 3 );
	RED_ASSERT( model );
	umbraChunk->model = const_cast< Umbra::SceneModel* >( model );

	m_modelCache.Insert( modelCacheId, umbraChunk );
	return AddObject( modelCacheId, objectInfo.m_transform, objectInfo.m_boundingBoxCenter, addedObjects );
}

Bool CUmbraTile::AddPointLight( const ObjectInfo& objectInfo, const Float radius, TDynArray< UmbraObjectInfo >& addedObjects )
{
	UpdateBounds( objectInfo.m_boundingBox );

	UmbraChunk* umbraChunk	= new UmbraChunk();
	umbraChunk->meshId		= objectInfo.m_id;
	umbraChunk->meshName	= String::Printf( TXT("PointLight: %d"), objectInfo.m_id );
	umbraChunk->twoSided	= false;
	umbraChunk->flags		= Umbra::SceneObject::TARGET | Umbra::SceneObject::VOLUME;
	umbraChunk->isTerrain	= false;
	umbraChunk->lavaFlags	= objectInfo.m_flags;

	Float lodNear = 0.0f;
	Float lodFar = objectInfo.m_autoHideDistance;
	ModifyDistancesWithFOVCorrection( lodNear, lodFar );
	UmbraHelpers::QuantizeDistanceLimits( lodNear, lodFar, umbraChunk->lodNearDistanceQuantized, umbraChunk->lodFarDistanceQuantized );

	Uint64 modelCacheId = UmbraHelpers::CalculateUmbraChunkHash( umbraChunk );
	if ( m_modelCache.KeyExist( modelCacheId ) )
	{
		RED_ASSERT( umbraChunk );
		delete umbraChunk;
		umbraChunk = nullptr;
		return AddObject( modelCacheId, objectInfo.m_transform, objectInfo.m_boundingBoxCenter, addedObjects );
	}

	TDynArray< Float > vertices;
	TDynArray< Uint16 > indices;
	Uint32 vertexCount = 0;
	Uint32 triangleCount = 0;
	BuildSphereGeometry( vertices, vertexCount, indices, triangleCount, radius );
	const Umbra::SceneModel* model = m_scene->insertModel( vertices.TypedData(), indices.TypedData(), vertexCount, triangleCount );
	RED_ASSERT( model );
	umbraChunk->model = const_cast< Umbra::SceneModel* >( model );

	m_modelCache.Insert( modelCacheId, umbraChunk );
	return AddObject( modelCacheId, objectInfo.m_transform, objectInfo.m_boundingBoxCenter, addedObjects );
}

Bool CUmbraTile::AddSpotLight( const ObjectInfo& objectInfo, const Float radius, const Float innerAngle, const Float outerAngle, TDynArray< UmbraObjectInfo >& addedObjects )
{
	UpdateBounds( objectInfo.m_boundingBox );

	UmbraChunk* umbraChunk	= new UmbraChunk();
	umbraChunk->meshId		= objectInfo.m_id;
	umbraChunk->meshName	= String::Printf( TXT("SpotLight: %d"), objectInfo.m_id );
	umbraChunk->twoSided	= false;
	umbraChunk->flags		= Umbra::SceneObject::TARGET | Umbra::SceneObject::VOLUME;
	umbraChunk->isTerrain	= false;
	umbraChunk->lavaFlags	= objectInfo.m_flags;

	Float lodNear = 0.0f;
	Float lodFar = objectInfo.m_autoHideDistance;
	ModifyDistancesWithFOVCorrection( lodNear, lodFar );

	UmbraHelpers::QuantizeDistanceLimits( lodNear, lodFar, umbraChunk->lodNearDistanceQuantized, umbraChunk->lodFarDistanceQuantized );
	Uint64 modelCacheId = UmbraHelpers::CalculateUmbraChunkHash( umbraChunk );
	if ( m_modelCache.KeyExist( modelCacheId ) )
	{
		RED_ASSERT( umbraChunk );
		delete umbraChunk;
		umbraChunk = nullptr;
		return AddObject( modelCacheId, objectInfo.m_transform, objectInfo.m_boundingBoxCenter, addedObjects );
	}

	TDynArray< Float > vertices;
	TDynArray< Uint16 > indices;
	Uint32 vertexCount = 0;
	Uint32 triangleCount = 0;
	BuildConeGeometry( vertices, vertexCount, indices, triangleCount, radius, innerAngle, outerAngle );
	const Umbra::SceneModel* model = m_scene->insertModel( vertices.TypedData(), indices.TypedData(), vertexCount, triangleCount );
	RED_ASSERT( model );
	umbraChunk->model = const_cast< Umbra::SceneModel* >( model );

	m_modelCache.Insert( modelCacheId, umbraChunk );
	return AddObject( modelCacheId, objectInfo.m_transform, objectInfo.m_boundingBoxCenter, addedObjects );
}

Bool CUmbraTile::AddTerrain( const CClipMap* clipmap )
{
	Box bBox = m_parentScene->GetBoundingBoxOfTile( m_coordinates );
	String meshName = String::Printf( TXT( "Terrain_%d_%d" ), m_coordinates.X, m_coordinates.Y );
	Uint32 meshId = GetHash( meshName );

	UmbraChunk* umbraChunk	= new UmbraChunk();
	umbraChunk->meshId		= meshId;
	umbraChunk->meshName	= meshName;
	umbraChunk->twoSided	= true;
	umbraChunk->flags		= Umbra::SceneObject::OCCLUDER | Umbra::SceneObject::TARGET;
	umbraChunk->isTerrain	= true;
	umbraChunk->lavaFlags	= 0;
	UmbraHelpers::QuantizeDistanceLimits( 0.0f, 2000.0f, umbraChunk->lodNearDistanceQuantized, umbraChunk->lodFarDistanceQuantized );
	Uint64 modelCacheId = UmbraHelpers::CalculateUmbraChunkHash( umbraChunk );
	if ( m_modelCache.KeyExist( modelCacheId ) )
	{
		RED_ASSERT( umbraChunk );
		delete umbraChunk;
		umbraChunk = nullptr;
		TDynArray< UmbraObjectInfo > dummy;
		return AddObject( modelCacheId, Matrix::IDENTITY, bBox.CalcCenter(), dummy );
	}

	SClipmapParameters clipMapParameters;
	clipmap->GetClipmapParameters( &clipMapParameters );
	Uint32 numberOfStepsPerEdge = clipMapParameters.tileRes;
	Float step = 1.0f / (Float)numberOfStepsPerEdge;
	Float worldStep = step * m_parentScene->GetTileSize();

	TDynArray< Float > vertices;
	TDynArray< Uint32 > indices;

	TDynArray< Vector > verts;
	TDynArray< Bool > isHole;

	const Uint32 clipMapLevel = 0; // always use the most detailed data to get the height and controlmap

	for ( Uint32 j = 0; j <= numberOfStepsPerEdge; ++j )
	for ( Uint32 i = 0; i <= numberOfStepsPerEdge; ++i )
	{
		Float x = bBox.Min.X + i * worldStep;
		Float y = bBox.Min.Y + j * worldStep;
		Vector worldPos( x, y, 0.0f );

		clipmap->GetHeightForWorldPositionSync( worldPos, clipMapLevel, worldPos.Z );
		TControlMapType cmt = clipmap->GetControlmapValueForWorldPositionSync( worldPos, clipMapLevel );

		isHole.PushBack( cmt == 0 );
		m_minZ = Min< Float >( worldPos.Z, m_minZ );
		m_maxZ = Max< Float >( worldPos.Z, m_maxZ );

		m_bbMin.X = Min< Float >( worldPos.X, m_bbMin.X );
		m_bbMin.Y = Min< Float >( worldPos.Y, m_bbMin.Y );
		m_bbMin.Z = Min< Float >( worldPos.Z, m_bbMin.Z );

		m_bbMax.X = Max< Float >( worldPos.X, m_bbMax.X );
		m_bbMax.Y = Max< Float >( worldPos.Y, m_bbMax.Y );
		m_bbMax.Z = Max< Float >( worldPos.Z, m_bbMax.Z );	

		vertices.PushBack( worldPos.X );
		vertices.PushBack( worldPos.Y );
		vertices.PushBack( worldPos.Z );

		verts.PushBack( worldPos );
	}

	for ( Uint32 j = 0; j < numberOfStepsPerEdge; ++j )
	for ( Uint32 i = 0; i < numberOfStepsPerEdge; ++i )
	{
		Uint32 ll = (j+0) * ( numberOfStepsPerEdge + 1 ) + (i+0);
		Uint32 lh = (j+1) * ( numberOfStepsPerEdge + 1 ) + (i+0);
		Uint32 hl = (j+0) * ( numberOfStepsPerEdge + 1 ) + (i+1);
		Uint32 hh = (j+1) * ( numberOfStepsPerEdge + 1 ) + (i+1);

		Bool allOnTerrain = true;
		if ( isHole[ ll ] || isHole[ hh ] || isHole[ hl ] )
		{
			allOnTerrain = false;
		}

		if ( allOnTerrain )
		{
			indices.PushBack( ll );
			indices.PushBack( hh );
			indices.PushBack( hl );
		}

		allOnTerrain = true;
		if ( isHole[ ll ] || isHole[ lh ] || isHole[ hh ] )
		{
			allOnTerrain = false;
		}

		if ( allOnTerrain )
		{
			indices.PushBack( ll );
			indices.PushBack( lh );
			indices.PushBack( hh );
		}
	}

	Uint32 vertexCount = vertices.Size() / 3;
	Uint32 triangleCount = indices.Size() / 3;

	if ( vertexCount == 0 || triangleCount == 0 )
	{
		return false;
	}

	Int64 estimatedCostInBytes = vertices.Size() * sizeof( Float ) + indices.Size() * sizeof( Uint32 );
	Float estimatedCostInKb = estimatedCostInBytes / 1024.f;
	Float estimatedCostinMb = estimatedCostInKb / 1024.f;

	const Umbra::SceneModel* model = m_scene->insertModel( vertices.TypedData(), indices.TypedData(), vertexCount, triangleCount );
	RED_ASSERT( model );
	//LOG_ENGINE( TXT("Adding terrain with %d vertices and %d triangles: %s [%1.4f Mb]"), vertexCount, triangleCount, model != nullptr ? TXT("SUCCESS") : TXT("FAILURE"), estimatedCostinMb );
	umbraChunk->model = const_cast< Umbra::SceneModel* >( model );

	m_modelCache.Insert( modelCacheId, umbraChunk );
	TDynArray< UmbraObjectInfo > dummy;
	return AddObject( modelCacheId, Matrix::IDENTITY, bBox.CalcCenter(), dummy );
}

Bool CUmbraTile::AddObject( const Uint64 modelCacheId, const Matrix& transform, const Vector& bbRefPosition, TDynArray< UmbraObjectInfo >& addedObjects )
{
	UmbraChunk* umbraChunk;
	if ( !m_modelCache.Find( modelCacheId, umbraChunk ) )
	{
		RED_LOG_WARNING( UmbraWarning, TXT( "!_!      Umbra modelCacheID [%") RED_PRIWu64 TXT("] not found in the cache." ), modelCacheId );
		return false;
	}

	Uint32 meshId = umbraChunk->meshId;
	Uint32 transformHash = UmbraHelpers::CalculateTransformHash( transform );
	TObjectCacheKeyType mapKey = UmbraHelpers::CompressToKeyType( meshId, transformHash );
	TObjectIdType compressedObjectId;
	if ( m_objectCache.Find( mapKey, compressedObjectId ) )
	{
		RED_LOG_WARNING( UmbraWarning, TXT("!_! '%ls' [modelCacheID: %") RED_PRIWu64 TXT("; transformHash: %u]"), umbraChunk->meshName.AsChar(), modelCacheId, transformHash );
		RED_LOG_WARNING( UmbraWarning, TXT("!_!      duplicate in current tile: %u"), compressedObjectId );
		return false;
	}

	if ( m_parentScene->FindInCache( mapKey, compressedObjectId ) )
	{
		RED_LOG_WARNING( UmbraWarning, TXT("!_! '%s' [modelCacheID: %") RED_PRIWu64 TXT("; transformHash: %u]"), umbraChunk->meshName.AsChar(), modelCacheId, transformHash );
		RED_LOG_WARNING( UmbraWarning, TXT("!_!      duplicate in parent scene: %u"), compressedObjectId );
		return false;
	}

	Vector position = transform.V[3];

	Umbra::Matrix4x4 mtx;
	Red::System::MemoryCopy( mtx.m, transform.V, 4 * 4 * sizeof( Float ) );

	const Umbra::SceneModel* model = umbraChunk->model;
	RED_ASSERT( model );

	// setup distance limits - LOD support
	Umbra::Vector2 distanceLimits = UmbraHelpers::DequantizeDistanceLimits( umbraChunk->lodNearDistanceQuantized, umbraChunk->lodFarDistanceQuantized );
		
	Uint16 tileId = m_parentScene->GetUniqueTileIdFromPosition( position );
	if ( umbraChunk->isTerrain )
	{
		tileId = m_tileId;
	}

	Uint32 objectId = m_objectId++;
	compressedObjectId = UmbraHelpers::CompressObjectId( tileId, objectId );
	m_objectCache.Insert( mapKey, compressedObjectId );

	LOG_ENGINE( TXT("[%") RED_PRIWu64 TXT("] -> [%u; %u] -> [%") RED_PRIWu64 TXT("] -> [%d]"), modelCacheId, meshId, transformHash, mapKey, compressedObjectId );			
	Umbra::Vector3 distanceBounds;
	distanceBounds.v[0] = bbRefPosition.X;
	distanceBounds.v[1] = bbRefPosition.Y;
	distanceBounds.v[2] = bbRefPosition.Z;

	// insert the object
	const Umbra::SceneObject* sceneobject = m_scene->insertObject( model,
		mtx,
		compressedObjectId,
		umbraChunk->flags,
		Umbra::MF_COLUMN_MAJOR,
		umbraChunk->twoSided ? Umbra::WINDING_TWO_SIDED : Umbra::WINDING_CW,
		&distanceLimits, &distanceBounds, &distanceBounds );

	RED_ASSERT( sceneobject );

	addedObjects.Grow( 1 );
	UmbraObjectInfo& object = addedObjects.Back();
	object.model = const_cast< Umbra::SceneModel* >( model );
	object.distanceLimits = distanceLimits;
	object.flags = umbraChunk->flags;
	object.objectId = compressedObjectId;
	object.transform = mtx;
	object.twoSided = umbraChunk->twoSided;
	object.lavaFlags = umbraChunk->lavaFlags;

	return true;
}

Bool CUmbraTile::AddWaypoint( const Vector& position )
{
	Umbra::Vector3 pos;
	pos.v[0] = position.X;
	pos.v[1] = position.Y;
	pos.v[2] = position.Z;

	m_scene->insertSeedPoint( pos );
	return true;
}

Bool CUmbraTile::AddSmallestHoleOverride( const Box& boundingBox, const Float smallestHoleOverride )
{
	Umbra::Vector3 volumeMin;
	volumeMin.v[0] = boundingBox.Min.A[0];
	volumeMin.v[1] = boundingBox.Min.A[1];
	volumeMin.v[2] = boundingBox.Min.A[2];

	Umbra::Vector3 volumeMax;
	volumeMax.v[0] = boundingBox.Max.A[0];
	volumeMax.v[1] = boundingBox.Max.A[1];
	volumeMax.v[2] = boundingBox.Max.A[2];

	++m_overriddenVolumeId;
	const Umbra::SceneVolume* viewVolume = m_scene->insertViewVolume( volumeMin, volumeMax, m_overriddenVolumeId );
	if ( !viewVolume )
	{
		RED_LOG_ERROR( UmbraError, TXT("Could not insert SmallestHoleOverride for tile [%d; %d] with box: %s"), m_coordinates.X, m_coordinates.Y, ToString( boundingBox ).AsChar() );
		return false;
	}

	m_volumeParameterOverrides.Grow();
	m_volumeParameterOverrides.Back().m_volumeId = m_overriddenVolumeId;
	m_volumeParameterOverrides.Back().m_smallestHoleValue = smallestHoleOverride;
	return true;
}

void CUmbraTile::UpdateBounds( const Box& worldSpaceBoundingBox )
{
	m_minZ = Min< Float >( worldSpaceBoundingBox.Min.Z, m_minZ );
	m_maxZ = Max< Float >( worldSpaceBoundingBox.Max.Z, m_maxZ );

	m_bbMin.X = Min< Float >( worldSpaceBoundingBox.Min.X, m_bbMin.X );
	m_bbMin.Y = Min< Float >( worldSpaceBoundingBox.Min.Y, m_bbMin.Y );
	m_bbMin.Z = Min< Float >( worldSpaceBoundingBox.Min.Z, m_bbMin.Z );

	m_bbMax.X = Max< Float >( worldSpaceBoundingBox.Max.X, m_bbMax.X );
	m_bbMax.Y = Max< Float >( worldSpaceBoundingBox.Max.Y, m_bbMax.Y );
	m_bbMax.Z = Max< Float >( worldSpaceBoundingBox.Max.Z, m_bbMax.Z );	
}
#endif //USE_UMBRA_COOKING

void CUmbraTile::Clear()
{
	m_parentScene->RemoveFromObjectCache( m_objectCache );
	m_objectCache.Clear();
#ifndef NO_UMBRA_DATA_GENERATION
	m_modelCache.Clear();
#endif //NO_UMBRA_DATA_GENERATION
	m_handle.Reset();

	// Recreate the scene
#ifdef USE_UMBRA_COOKING
	if ( m_scene )
	{
		m_scene->release();
		m_scene = Umbra::Scene::create();
	}
#endif //USE_UMBRA_COOKING
}

#ifndef NO_UMBRA_DATA_GENERATION
Bool CUmbraTile::CalculateSceneBounds( STomeDataGenerationContext& context, Umbra::Vector3& volumeMin, Umbra::Vector3& volumeMax )
{
	Float Z = 20.0f * context.umbraTileSize;
	// compute lowest point
	while ( Z > m_minZ )
	{
		Z -= context.umbraTileSize;
	}
	// store lowest point
	context.tileCorner.Z = Z;

	Z = -20.0f * context.umbraTileSize;
	// compute highest point
	while ( Z < m_maxZ )
	{
		Z += context.umbraTileSize;
	}
	// store it
	context.tileCorner.W = Z;

	if ( context.tileCorner.Z >= context.tileCorner.W )
	{
		RED_LOG_ERROR( UmbraError, TXT(" CalculateSceneBounds for tile [%d; %d] has invalid values for Z: [%1.2f; %1.2f] adjusting.."), m_coordinates.X, m_coordinates.Y, context.tileCorner.Z, context.tileCorner.W );
		while ( context.tileCorner.W <= context.tileCorner.Z )
		{
			context.tileCorner.W += context.umbraTileSize;
		}
		RED_LOG_ERROR( UmbraError, TXT(" ..adjusted Z values of tile [%d; %d] to: [%1.2f; %1.2f]."), m_coordinates.X, m_coordinates.Y, context.tileCorner.Z, context.tileCorner.W );
	}

	const Float viewVolumeFactor = 0.4f;
	const Float extendsInAnyDirection = viewVolumeFactor * context.computationParameters.m_tileSize;

	volumeMin.v[0] = context.tileCorner.X - extendsInAnyDirection;
	volumeMin.v[1] = context.tileCorner.Y - extendsInAnyDirection;
	volumeMin.v[2] = context.tileCorner.Z - extendsInAnyDirection;

	volumeMax.v[0] = context.tileCorner.X + context.computationParameters.m_tileSize + extendsInAnyDirection;
	volumeMax.v[1] = context.tileCorner.Y + context.computationParameters.m_tileSize + extendsInAnyDirection;
	volumeMax.v[2] = context.tileCorner.W + context.computationParameters.m_tileSize + extendsInAnyDirection;

	return true;
}
#endif //NO_UMBRA_DATA_GENERATION

void ExpandBoxToMatchSmallestOccluder( Box* box, Float smallestOccluder )
{
	if ( !box )
	{
		return;
	}

	Vector min = box->Min / smallestOccluder;
	Vector max = box->Max / smallestOccluder;
	VectorI minI( (Int32)Red::Math::MFloor( min.X ), (Int32)Red::Math::MFloor( min.Y ), (Int32)Red::Math::MFloor( min.Z ) );
	VectorI maxI( (Int32)Red::Math::MCeil( max.X ), (Int32)Red::Math::MCeil( max.Y ), (Int32)Red::Math::MCeil( max.Z ) );

	Vector resultMin( minI.X * smallestOccluder, minI.Y * smallestOccluder, minI.Z * smallestOccluder );
	Vector resultMax( maxI.X * smallestOccluder, maxI.Y * smallestOccluder, maxI.Z * smallestOccluder );

	box->Min = resultMin;
	box->Max = resultMax;
}

#ifdef USE_UMBRA_COOKING
Bool CUmbraTile::GenerateTomeDataSync( STomeDataGenerationContext& context, Bool dumpScene, Bool dumpRawTomeData )
{
	Umbra::Vector3 viewVolumeMin;
	Umbra::Vector3 viewVolumeMax;
	if ( !CalculateSceneBounds( context, viewVolumeMin, viewVolumeMax ) )
	{
		return false;
	}

	if ( !m_scene->insertViewVolume( viewVolumeMin, viewVolumeMax, 0 ) )
	{
		return false;
	}

	if ( dumpScene )
	{
		return DumpScene( context );
	}

	context.tile = this;

	Uint32 backFaceLimit = 100;
	Int32 tilesPerEdgeHorizontally = context.computationParameters.m_umbraTilesCountPerEdgeOfStreamingTile;
	Int32 tilesPerEdgeVertically = (Uint32)( ( context.tileCorner.W - context.tileCorner.Z ) / context.umbraTileSize );

	Box* bbox = new Box( m_bbMin, m_bbMax );
	ExpandBoxToMatchSmallestOccluder( bbox, context.computationParameters.m_smallestOccluder );

	Bool res = false;
	{
		RegenerationContext regenerationContext;
		regenerationContext.m_id						= context.tileId;
		regenerationContext.m_data						= &m_data;
		regenerationContext.m_scene						= m_scene;
		regenerationContext.m_smallestHole				= context.computationParameters.m_smallestHole;
		regenerationContext.m_smallestOccluder			= context.computationParameters.m_smallestOccluder;
		regenerationContext.m_backFaceLimit				= backFaceLimit;
		regenerationContext.m_tileSize					= context.umbraTileSize;
		regenerationContext.m_tileCorner				= context.tileCorner;
		regenerationContext.m_tilesPerEdgeHorizontally	= tilesPerEdgeHorizontally;
		regenerationContext.m_tilesPerEdgeVertically	= tilesPerEdgeVertically;
		regenerationContext.m_sceneBoundingBox			= bbox;
		regenerationContext.m_intermediateDir			= Move( context.intermediateDirectory );
		regenerationContext.m_forceRegenerate			= context.forceRegenerate;
		regenerationContext.m_volumeParameterOverrides	= m_volumeParameterOverrides;

		CTimeCounter timer;
		res = CUmbraTile::Regenerate( regenerationContext );
		RED_LOG( UmbraInfo, TXT("Computing tile [%d; %d] took: %1.2f ms"), context.tileId.X, context.tileId.Y, timer.GetTimePeriodMS() );
	}

	delete bbox;

	SetStatus( res ? EUTDS_Valid : EUTDS_Invalid );
	SaveGeneratedData();

	if ( dumpRawTomeData )
	{
		RED_VERIFY( DumpTomeData( context ) );
	}

	return res;
}
#endif //USE_UMBRA_COOKING

#ifdef USE_UMBRA_COOKING
Bool CUmbraTile::DumpScene( const STomeDataGenerationContext& context )
{
	String sceneFilename = String::Printf( TXT("%s%dx%d.scene"), context.fileName.AsChar(), m_coordinates.X, m_coordinates.Y );
	Bool result = m_scene->writeToFile( UNICODE_TO_ANSI( sceneFilename.AsChar() ) );
	RED_LOG( UmbraInfo, TXT("Dumping Umbra Scene to '%ls' %s."), sceneFilename.AsChar(), result ? TXT("SUCCEEDED") : TXT("FAILED") );
	return result;
}

Bool CUmbraTile::DumpTomeData( const STomeDataGenerationContext& context )
{
	BufferHandle handle = m_data.AcquireBufferHandleSync();
	if ( !handle->GetData() || !handle->GetSize() )
	{
		// empty buffer, do nothing
		RED_LOG( UmbraInfo, TXT("Tome buffer for tile [%d; %d] is empty."), m_coordinates.X, m_coordinates.Y );
		return true;
	}

	String tomeFileName = String::Printf( TXT("%s%dx%d.tome"), context.fileName.AsChar(), m_coordinates.X, m_coordinates.Y );
	IFile* file = GFileManager->CreateFileWriter( tomeFileName, FOF_AbsolutePath | FOF_Buffered );
	if ( !file )
	{
		RED_LOG_ERROR( UmbraError, TXT("Error opening file '%ls'"), tomeFileName.AsChar() );
		return false;
	}

	file->Serialize( handle->GetData(), handle->GetSize() );

	delete file;
	file = nullptr;
	
	RED_LOG( UmbraInfo, TXT("Saved tome [%d; %d] data to '%ls'"), m_coordinates.X, m_coordinates.Y, tomeFileName.AsChar() );
	return true;
}
#endif USE_UMBRA_COOKING

#ifdef USE_UMBRA_COOKING
Bool CUmbraTile::Regenerate( const RegenerationContext& context )
{
	// check if all coordinated of the tile are aligned to smallest occluder size
	// if not, the CLUSTER_SIZE parameter provides the old behaviour
	Bool isTileAlignedToOccluderSize = false;
	Float multiplier = context.m_tileCorner.X / context.m_smallestOccluder;
	isTileAlignedToOccluderSize = Red::Math::MFloor( multiplier ) == multiplier;
	multiplier = context.m_tileCorner.Y / context.m_smallestOccluder;
	isTileAlignedToOccluderSize = Red::Math::MFloor( multiplier ) == multiplier;
	multiplier = context.m_tileCorner.Z / context.m_smallestOccluder;
	isTileAlignedToOccluderSize = Red::Math::MFloor( multiplier ) == multiplier;

	Umbra::ComputationParams computationParams;
	computationParams.setParam( Umbra::ComputationParams::SMALLEST_HOLE,		context.m_smallestHole );
	computationParams.setParam( Umbra::ComputationParams::BACKFACE_LIMIT,		(Umbra::UINT32)context.m_backFaceLimit );
	computationParams.setParam( Umbra::ComputationParams::SMALLEST_OCCLUDER,	context.m_smallestOccluder );
	Umbra::UINT32 flags = Umbra::ComputationParams::DATA_TOME_MATCH | Umbra::ComputationParams::DATA_ACCURATE_DILATION;
	computationParams.setParam( Umbra::ComputationParams::OUTPUT_FLAGS,			flags );

	for ( auto& volumeOverride : context.m_volumeParameterOverrides )
	{
		computationParams.setVolumeParam( volumeOverride.m_volumeId, Umbra::ComputationParams::SMALLEST_HOLE, volumeOverride.m_smallestHoleValue );
	}

	if ( isTileAlignedToOccluderSize )
	{
		computationParams.setParam( Umbra::ComputationParams::TILE_SIZE,		context.m_tileSize );
	}
	else
	{
		computationParams.setParam( Umbra::ComputationParams::CLUSTER_SIZE,		context.m_tileSize );
	}

//#define USE_UMBRA_TASK
#ifdef USE_UMBRA_TASK
	UmbraAllocator* umbraAllocator = new UmbraAllocator();
	RED_ASSERT( umbraAllocator );
	UmbraLogger* umbraLogger = new UmbraLogger();
	RED_ASSERT( umbraLogger );

	Umbra::Vector3 tomeMin;
	Umbra::Vector3 tomeMax;
	Bool useBB = false;
	if ( context.m_sceneBoundingBox && useBB )
	{
		tomeMin.v[0] = context.m_sceneBoundingBox->Min.X;
		tomeMin.v[1] = context.m_sceneBoundingBox->Min.Y;
		tomeMin.v[2] = context.m_sceneBoundingBox->Min.Z;
		tomeMax.v[0] = context.m_sceneBoundingBox->Max.X;
		tomeMax.v[1] = context.m_sceneBoundingBox->Max.Y;
		tomeMax.v[2] = context.m_sceneBoundingBox->Max.Z;
	}
	else
	{
		tomeMin.v[0] = context.m_tileCorner.X;
		tomeMin.v[1] = context.m_tileCorner.Y;
		tomeMin.v[2] = context.m_tileCorner.Z;
		tomeMax.v[0] = context.m_tileCorner.X + context.m_tilesPerEdgeHorizontally * context.m_tileSize;
		tomeMax.v[1] = context.m_tileCorner.Y + context.m_tilesPerEdgeHorizontally * context.m_tileSize;
		tomeMax.v[2] = context.m_tileCorner.W;
	}
	Umbra::Task* task = Umbra::Task::create( context.m_scene, &tomeMin, &tomeMax, umbraAllocator );
	RED_ASSERT( task );

	task->setLogger( umbraLogger );
	task->setNumThreads( 3 );
	
	// let's hope 3 GB for Umbra will be enough..
	const Int32 memoryBudget = 3 * 1024;
	task->setMemoryUsageLimit( memoryBudget );
	RED_LOG( UmbraInfo, TXT("Memory bugdet for tile computation is %d MB"), memoryBudget );
	task->setComputationParams( computationParams );

	String umbraExeDirectory = String::Printf( TXT("%s..\\dev\\external\\umbra3.3.10-win-shadows-7\\bin\\win64" ), GFileManager->GetBaseDirectory().AsChar() );
	String umbraExeFullPath = String::Printf( TXT("%s\\umbraProcess64.exe" ), umbraExeDirectory.AsChar() );

	task->setRunAsProcess( UNICODE_TO_ANSI( umbraExeFullPath.AsChar() ) );
	task->start( UNICODE_TO_ANSI( umbraExeDirectory.AsChar() ) );

	Uint32 lastProgress = 0;
	while ( !task->isFinished() )
	{
		Uint32 progress = (Uint32)( task->getProgress() * 100.0f );
		if ( progress != lastProgress )
		{
			RED_LOG( UmbraInfo, TXT("Tile [%d; %d] progress: %d%%"), context.m_id.X, context.m_id.Y, progress );
			lastProgress = progress;
		}
		Sleep( 3000 ); // monitor every 3 seconds, Tome creation is on different thread
	}

	Umbra::Task::Error errorCode = task->getError();
	if ( errorCode != Umbra::Task::ERROR_OK )
	{
		// cleanup
		String error( ANSI_TO_UNICODE( task->getErrorString() ) );
		RED_LOG_ERROR( UmbraError, TXT("  Umbra::Task failed with error: %s (%d)"), error.AsChar(), errorCode );
		task->release();
		task = nullptr;
		delete umbraAllocator;
		umbraAllocator = nullptr;
		delete umbraLogger;
		umbraLogger = nullptr;
		return false;
	}

	// clear the data, just to be sure
	context.m_occlusionData->Clear();
	Uint32 desiredDataSize = task->getTomeSize();
	RED_ASSERT( desiredDataSize > 0 );
	context.m_occlusionData->Allocate( desiredDataSize );
	const Umbra::Tome* tome = task->getTome( context.m_occlusionData->GetData(), desiredDataSize );
	// cleanup
	task->release();
	task = nullptr;
	delete umbraAllocator;
	umbraAllocator = nullptr;
	delete umbraLogger;
	umbraLogger = nullptr;
#else
	CUmbraIntermediateResultStorage intermediateResultsStorage( context.m_intermediateDir, context.m_id, context.m_forceRegenerate );

	UmbraAllocator umbraAllocator;
	UmbraLogger umbraLogger;

	Umbra::PlatformServices platformServices;
	platformServices.logger = &umbraLogger;
	platformServices.allocator = &umbraAllocator;

	Umbra::Builder builder;
	builder.init(platformServices);

	Umbra::TomeGenerator tomeGenerator;
	builder.join( tomeGenerator, computationParams );

	for ( Int32 i = 0; i < context.m_tilesPerEdgeHorizontally; ++i )
	{
		RED_LOG_SPAM( UmbraInfo, TXT( "Computing tile [%d; %d], subtile [%d/%d].." ), context.m_id.X, context.m_id.Y, i, context.m_tilesPerEdgeHorizontally );
		
		Double msTimeElapsedComputing = 0.0;

		for ( Int32 j = 0; j < context.m_tilesPerEdgeHorizontally; ++j )
		{
			Umbra::Vector3 tomeMin;
			tomeMin.v[0] = context.m_tileCorner.X + i * context.m_tileSize;
			tomeMin.v[1] = context.m_tileCorner.Y + j * context.m_tileSize;
			tomeMin.v[2] = context.m_tileCorner.Z;

			Umbra::Vector3 tomeMax;
			tomeMax.v[0] = context.m_tileCorner.X + (i+1) * context.m_tileSize;
			tomeMax.v[1] = context.m_tileCorner.Y + (j+1) * context.m_tileSize;
			tomeMax.v[2] = context.m_tileCorner.W;

			//// The call to Builder::split() divides the given streaming block into multiple computation jobs or TileInputs. 
			//// More specifically, a TileInputSet is returned, which can be used to iterate over all the jobs, and perform the visibility computation on them.
			Umbra::TileInputSet* boundedTileInputSet = new Umbra::TileInputSet();
			Umbra::Builder::Error errorCode = builder.split( *boundedTileInputSet, context.m_scene, computationParams, tomeMin, tomeMax );
			RED_ASSERT( errorCode == Umbra::Builder::SUCCESS );
			if ( errorCode != Umbra::Builder::SUCCESS )
			{
				String error;
				switch ( errorCode )
				{
				case Umbra::Builder::ERROR_EMPTY_ITERATOR:	error = TXT("ERROR_EMPTY_ITERATOR");	break;
				case Umbra::Builder::ERROR_PARAM:			error = TXT("ERROR_PARAM");				break;
				case Umbra::Builder::ERROR_OUT_OF_MEMORY:	error = TXT("ERROR_OUT_OF_MEMORY");		break;
				case Umbra::Builder::ERROR_LICENSE_KEY:		error = TXT("ERROR_LICENSE_KEY");	break;
				default: error = TXT("");
				}

				RED_LOG_ERROR( UmbraError, TXT("  Umbra::Builder::split() failed with error: %s (%d)"), error.AsChar(), errorCode );

				delete boundedTileInputSet;
				boundedTileInputSet = nullptr;

				return false;
			}

			struct STileInputInfo
			{
				Int32 i;
				Int32 j;
				Int32 k;
				Umbra::TileInput* tileInput;
			};

			Int32 boundedTileInputSetSize = boundedTileInputSet->size();
			TDynArray< STileInputInfo > tileInputInfos;
			for ( Int32 k = 0; k < boundedTileInputSetSize; ++k )
			{
				STileInputInfo tileInputInfo;
				tileInputInfo.i = i;
				tileInputInfo.j = j;
				tileInputInfo.k = k;
				tileInputInfo.tileInput = new Umbra::TileInput();
				RED_ASSERT( tileInputInfo.tileInput );
				errorCode = boundedTileInputSet->get( *tileInputInfo.tileInput, k );

				RED_ASSERT( errorCode == Umbra::Builder::SUCCESS );
				if ( errorCode != Umbra::Builder::SUCCESS )
				{
					String error;
					switch ( errorCode )
					{
					case Umbra::Builder::ERROR_EMPTY_ITERATOR:	error = TXT("ERROR_EMPTY_ITERATOR");	break;
					case Umbra::Builder::ERROR_PARAM:			error = TXT("ERROR_PARAM");				break;
					case Umbra::Builder::ERROR_OUT_OF_MEMORY:	error = TXT("ERROR_OUT_OF_MEMORY");		break;
					case Umbra::Builder::ERROR_LICENSE_KEY:		error = TXT("ERROR_LICENSE_KEY");	break;
					default: error = TXT("");
					}

					RED_LOG_ERROR( UmbraError, TXT("  Umbra::TiledInputSet::get(%d) failed with error: %s (%d)"), k, error.AsChar(), errorCode );

					for ( Uint32 deletionIndex = 0; deletionIndex < tileInputInfos.Size(); ++deletionIndex )
					{
						delete tileInputInfos[ deletionIndex ].tileInput;
					}
					tileInputInfos.Clear();
					delete boundedTileInputSet;
					boundedTileInputSet = nullptr;

					return false;
				}

				tileInputInfos.PushBack( tileInputInfo );
			}

			delete boundedTileInputSet;
			boundedTileInputSet = nullptr;

			class CTileInputProcessor
			{
			public:
				Umbra::Builder*						m_builder;
				CUmbraIntermediateResultStorage*	m_storage;

				void Compute( const STileInputInfo& inputInfo, Umbra::TileResult*& output )
				{
					if ( !m_storage->Get( *m_builder, inputInfo.i, inputInfo.j, inputInfo.k, inputInfo.tileInput->getHashValue(), *output ) )
					{
						Umbra::Builder::Error errorCode = m_builder->computeTile( *output, *inputInfo.tileInput );
						RED_ASSERT( errorCode == Umbra::Builder::SUCCESS );
						m_storage->Add( inputInfo.i, inputInfo.j, inputInfo.k, inputInfo.tileInput->getHashValue(), *output );
					}
				}
			};

			CTileInputProcessor processor;
			processor.m_builder = &builder;
			processor.m_storage = &intermediateResultsStorage;

			TDynArray< Umbra::TileResult* > tileResults;
			tileResults.Reserve( tileInputInfos.Size() );
			for ( Uint32 insertionIndex = 0; insertionIndex < tileInputInfos.Size(); ++insertionIndex )
			{
				tileResults.PushBack( new Umbra::TileResult() );
			}

			auto params = CParallelForTaskDoubleArray< STileInputInfo, Umbra::TileResult*, CTileInputProcessor >::SParams::Create();
			{
				params->m_inputArray		= tileInputInfos.TypedData();
				params->m_outputArray		= tileResults.TypedData();
				params->m_numElements		= tileInputInfos.Size();
				params->m_processFunc		= &CTileInputProcessor::Compute;
				params->m_processFuncOwner	= &processor;
				params->m_priority			= TSP_Critical;
				params->SetDebugName		( TXT("UmbraTile") );
			}

			{
				CTimeCounter timer;
				params->ProcessNow();
				params->Release();
				msTimeElapsedComputing += timer.GetTimePeriodMS();
			}

			for ( Int32 k = 0; k < boundedTileInputSetSize; ++k )
			{
				Umbra::TileResult* tileResult = tileResults[k];
				RED_ASSERT( tileResult );
				errorCode = tomeGenerator.addTileResult( *tileResult );
				RED_ASSERT( errorCode == Umbra::Builder::SUCCESS );
				if ( errorCode != Umbra::Builder::SUCCESS )
				{
					String error;
					switch ( errorCode )
					{
					case Umbra::Builder::ERROR_EMPTY_ITERATOR:	error = TXT("ERROR_EMPTY_ITERATOR");	break;
					case Umbra::Builder::ERROR_PARAM:			error = TXT("ERROR_PARAM");				break;
					case Umbra::Builder::ERROR_OUT_OF_MEMORY:	error = TXT("ERROR_OUT_OF_MEMORY");		break;
					case Umbra::Builder::ERROR_LICENSE_KEY:		error = TXT("ERROR_LICENSE_KEY");	break;
					default: error = TXT("");
					}

					RED_LOG_ERROR( UmbraError, TXT("  Umbra::TomeGenerator::addTileResult() failed with error: %s (%d)"), error.AsChar(), errorCode );
					for ( Uint32 deletionIndex = 0; deletionIndex < tileInputInfos.Size(); ++deletionIndex )
					{
						delete tileInputInfos[ deletionIndex ].tileInput;
					}
					tileInputInfos.Clear();

					for ( Uint32 deletionIndex = 0; deletionIndex < tileResults.Size(); ++deletionIndex )
					{
						delete tileResults[ deletionIndex ];
					}
					tileResults.Clear();
					return false;
				}
			}

			for ( Uint32 deletionIndex = 0; deletionIndex < tileResults.Size(); ++deletionIndex )
			{
				delete tileResults[ deletionIndex ];
			}
			tileResults.Clear();

			for ( Uint32 deletionIndex = 0; deletionIndex < tileInputInfos.Size(); ++deletionIndex )
			{
				delete tileInputInfos[ deletionIndex ].tileInput;
			}
			tileInputInfos.Clear();
		}

		Int32 sComputing = (Int32)msTimeElapsedComputing / 1000;
		msTimeElapsedComputing -= sComputing * 1000.0;
		RED_LOG_SPAM( UmbraInfo, TXT( "		...computation time: [%ds %1.2fms]" ), sComputing, msTimeElapsedComputing );
	}

	intermediateResultsStorage.Save();

	// clear the data, just to be sure
	context.m_data->Clear();
	Uint32 desiredDataSize = 0;
	Umbra::Builder::Error errorCode = tomeGenerator.getTomeSize( ( Umbra::UINT32& )desiredDataSize );
	RED_ASSERT( errorCode == Umbra::Builder::SUCCESS );
	if ( errorCode != Umbra::Builder::SUCCESS )
	{
		String error;
		switch ( errorCode )
		{
		case Umbra::Builder::ERROR_EMPTY_ITERATOR:	error = TXT("ERROR_EMPTY_ITERATOR");	break;
		case Umbra::Builder::ERROR_PARAM:			error = TXT("ERROR_PARAM");				break;
		case Umbra::Builder::ERROR_OUT_OF_MEMORY:	error = TXT("ERROR_OUT_OF_MEMORY");		break;
		case Umbra::Builder::ERROR_LICENSE_KEY:		error = TXT("ERROR_LICENSE_KEY");	break;
		default: error = TXT("");
		}

		RED_LOG_ERROR( UmbraError, TXT("  Umbra::TomeGenerator::getTomeSize() failed with error: %s (%d)"), error.AsChar(), errorCode );
		return false;
	}
	RED_ASSERT( desiredDataSize > 0 );
	context.m_data->ReallocateBuffer( desiredDataSize );
	BufferHandle handle = context.m_data->AcquireBufferHandleForWritingSync();
	tomeGenerator.getTome( ( Umbra::UINT8* )handle->GetData(), desiredDataSize );
#endif //USE_UMBRA_TASK

	return true;
}
#endif //USE_UMBRA_COOKING

void CUmbraTile::SaveGeneratedData()
{
	RED_ASSERT( GetFile() );

#if !defined(NO_FILE_SOURCE_CONTROL_SUPPORT) && !defined(NO_UMBRA_DATA_GENERATION) && !defined(NO_RESOURCE_IMPORT)
	GetFile()->GetStatus();
	if ( GetFile()->IsNotSynced() )
	{
		RED_LOG_ERROR( UmbraError, TXT( "File '%ls' is out-of-date. Sync before processing." ), GetFile()->GetFileName().AsChar() );
		return;
	}
	if ( GetFile()->IsCheckedOut() || GetFile()->IsLocal() || GetFile()->IsAdded() )
	{
		if ( !Save() )
		{
			RED_LOG_ERROR( UmbraError, TXT( "File '%ls' cannot be saved (save error)." ), GetFile()->GetFileName().AsChar() );
		}
	}
	else
	{
		SChangelist changelist = SChangelist::DEFAULT;
		// Create a changelist only if the automatic changelists are enabled
		if ( GVersionControl->AutomaticChangelistsEnabled() )
		{
			String tags = String::EMPTY;
			SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("SourceControl"), TXT("DepartmentTags"), tags );
			String name = String::Printf( TXT( "%s Update of occlusion data for '%ls'" ), tags.AsChar(), GetFile()->GetDepotPath().AsChar() );
			RED_VERIFY( GVersionControl->CreateChangelist( name, changelist ) );
		}

		GetFile()->SetChangelist( changelist );
		if ( GetFile()->SilentCheckOut() )
		{
			if ( !Save() )
			{
				RED_LOG_ERROR( UmbraError, TXT( "File '%ls' cannot be saved (save error)." ), GetFile()->GetFileName().AsChar() );
			}
		}
		else
		{
			RED_LOG_ERROR( UmbraError, TXT( "File '%ls' cannot be saved (checkout error)." ), GetFile()->GetFileName().AsChar() );
		}
	}
#endif
}

#ifdef USE_UMBRA_COOKING
Bool CUmbraTile::InsertExistingObject( const UmbraObjectInfo& objectInfo )
{
	RED_ASSERT( m_scene );

	Bool objectFound = false;
	Uint32 numberOfObjects = m_scene->getObjectCount();
	for ( Uint32 i = 0; i < numberOfObjects; ++i )
	{
		const Umbra::SceneObject* object = m_scene->getObject( i );
		if ( object->getID() == objectInfo.objectId )
		{
			objectFound = true;
			break;
		}
	}

	if ( objectFound )
	{
		RED_LOG_WARNING( UmbraWarning, TXT("!_!        Umbra: object with the same ObjectID [%d] already found in tile [%d, %d]."), objectInfo.objectId, m_coordinates.X, m_coordinates.Y );
	}

	if ( !objectInfo.model )
	{
		//RED_LOG_WARNING( UmbraWarning, TXT("!_!        Umbra: object with no model (ObjectID [%d]) in tile [%d, %d]."), objectInfo.objectId, m_coordinates.X, m_coordinates.Y );
		//RED_LOG_WARNING( UmbraWarning, TXT("!_!        Umbra: object with ObjectID [%d] (tile [%d, %d]) has no model."), objectInfo.objectId, m_coordinates.X, m_coordinates.Y );
		return false;
	}

	RED_ASSERT( objectInfo.model );
	Uint32 numberOfVertices = objectInfo.model->getVertexCount();
	const Umbra::Vector3* umbraVertices = objectInfo.model->getVertices();
	Float* vertices = new Float[ numberOfVertices * 3 ];
	Uint32 index = 0;
	for ( Uint32 i = 0; i < numberOfVertices; ++i )
	{
		vertices[ index++ ] = umbraVertices[i].v[0];
		vertices[ index++ ] = umbraVertices[i].v[1];
		vertices[ index++ ] = umbraVertices[i].v[2];

		m_minZ = Min< Float >( m_minZ, umbraVertices[i].v[2] );
		m_maxZ = Max< Float >( m_maxZ, umbraVertices[i].v[2] );
	}

	Uint32 numberOfTriangles = objectInfo.model->getTriangleCount();
	const Umbra::Vector3i* umbraTriangles = objectInfo.model->getTriangles();
	Uint32* indices = new Uint32[numberOfTriangles * 3];
	index = 0;
	for ( Uint32 i = 0; i < numberOfTriangles; ++i )
	{
		indices[ index++ ] = umbraTriangles[i].i;
		indices[ index++ ] = umbraTriangles[i].j;
		indices[ index++ ] = umbraTriangles[i].k;
	}

	const Umbra::SceneModel* newModel = m_scene->insertModel( vertices, indices, numberOfVertices, numberOfTriangles );
	RED_ASSERT( newModel );
	delete [] vertices;
	delete [] indices;
	
	const Umbra::SceneObject* sceneObject = m_scene->insertObject( newModel,
		objectInfo.transform,
		objectInfo.objectId,
		objectInfo.flags,
		Umbra::MF_COLUMN_MAJOR,
		objectInfo.twoSided ? Umbra::WINDING_TWO_SIDED : Umbra::WINDING_CW,
		&objectInfo.distanceLimits );

	return sceneObject != nullptr;
}
#endif //USE_UMBRA_COOKING

#ifdef USE_UMBRA_COOKING
Bool CUmbraTile::ShouldGenerateData() const
{
	if ( !m_scene )
	{
		return false;
	}

	Uint32 numberOfObjects = m_scene->getObjectCount();

	RED_LOG( UmbraInfo, TXT("CUmbraTile::ShouldGenerateData() [%d; %d] -> [%d]"), m_coordinates.X, m_coordinates.Y, numberOfObjects );
	// objectCount == 1 means that only terrain is present on current tile. In that case it's pointless to generate the data
	return numberOfObjects > 1;
}
#endif //USE_UMBRA_COOKING

#ifdef USE_UMBRA_COOKING
Uint32 CUmbraTile::GetTileDensity() const
{
	return m_scene ? m_scene->getObjectCount() : 0;
}
#endif //USE_UMBRA_COOKING
#endif //NO_UMBRA_DATA_GENERATION

void CUmbraTile::AddRefTome()
{
	m_tomeRefCount.Increment();
}

void CUmbraTile::ReleaseRefTome()
{
	if ( m_tomeRefCount.Decrement() == 0 )
	{
		RED_LOG_SPAM( UmbraInfo, TXT("!_! Freeing Tome [%d x %d]"), m_coordinates.X, m_coordinates.Y );
		m_handle.Reset();
		if ( m_tome )
		{
			m_tome = nullptr;
		}
	}
}

void CUmbraTile::RequestAsyncLoad( const BufferAsyncCallback& callback )
{
	m_asyncDataHandle = m_data.AcquireBufferHandleAsync( eIOTag_UmbraBuffer, callback );
}

Bool CUmbraTile::HasData( Uint32* dataSize /*=nullptr*/ ) const
{
	Uint32 bufferSize = m_data.GetSize();
	if ( dataSize )
	{
		*dataSize = bufferSize;
	}
	return bufferSize > 0;
}

#endif // USE_UMBRA
