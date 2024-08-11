/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Mesh drawing stats
class MeshDrawingStats
{
public:
	Uint32		m_numBatches;
	Uint32		m_numInstancedBatches;
	Uint32		m_biggestBatch;
	Uint32		m_biggestInstancedBatch;
	Uint32		m_smallestBatch;
	Uint32		m_smallestInstancedBatch;

	Uint32		m_numChunks;
	Uint32		m_numTriangles;
	Uint32		m_numVertices;

	Uint32		m_numChunksSkinned;
	Uint32		m_numTrianglesSkinned;
	Uint32		m_numVerticesSkinned;

	Uint32		m_numChunksStatic;
	Uint32		m_numTrianglesStatic;
	Uint32		m_numVerticesStatic;

	// occlusion stats
	Uint32		m_renderedStaticProxies;
	Uint32		m_renderedDynamicProxies;
	Uint32		m_renderedDynamicDecals;
	Uint32		m_renderedDynamicDecalsCount;
	Uint32		m_renderedFurthestProxies;

	Uint32		m_occludedStaticProxies;
	Uint32		m_occludedDynamicProxies;
	Uint32		m_occludedFurthestProxies;
	Uint32		m_occludedDynamicDecals;

	Uint32		m_visibleObjects;
	Double		m_occlusionTimeQuery;
	Double		m_occlusionTimeDynamicObjects;
	Double		m_occlusionTimeVisibilityByDistance;

	Double		m_furthestProxiesTime;
	Double		m_furthestProxiesOcclusionTime;
	Double		m_furthestProxiesDistanceTime;
	Double		m_furthestProxiesCollectionTime;

	Uint32		m_renderedShadowStaticProxies;
	Uint32		m_renderedShadowDynamicProxies;

	Double		m_shadowQueryTime;
	Double		m_stStatic;
	Double		m_stStaticDistance;
	Double		m_stStaticCollection;
	Uint32		m_stStaticCulledByDistance;

	Double		m_stDynamic;
	Double		m_stDynamicDistance;
	Double		m_stDynamicUmbra;
	Double		m_stDynamicCollection;

	MeshDrawingStats()
		: m_numBatches( 0 )
		, m_numInstancedBatches( 0 )
		, m_biggestBatch( 0 )
		, m_biggestInstancedBatch( 0 )
		, m_smallestBatch( UINT32_MAX )
		, m_smallestInstancedBatch( UINT32_MAX )

		, m_numChunks( 0 )
		, m_numTriangles( 0 )
		, m_numVertices( 0 )

		, m_numChunksStatic( 0 )
		, m_numTrianglesStatic( 0 )
		, m_numVerticesStatic( 0 )

		, m_numChunksSkinned( 0 )
		, m_numTrianglesSkinned( 0 )
		, m_numVerticesSkinned( 0 )

		, m_visibleObjects( 0 )
		, m_occlusionTimeQuery( 0.0 )
		, m_occlusionTimeDynamicObjects( 0.0 )
		, m_occlusionTimeVisibilityByDistance( 0.0 )

		, m_furthestProxiesTime( 0.0 )
		, m_furthestProxiesOcclusionTime( 0.0 )
		, m_furthestProxiesDistanceTime( 0.0 )
		, m_furthestProxiesCollectionTime( 0.0 )

		, m_renderedShadowStaticProxies( 0 )
		, m_renderedShadowDynamicProxies( 0 )

		, m_renderedStaticProxies( 0 )
		, m_renderedDynamicProxies( 0 )
		, m_renderedFurthestProxies( 0 )
		, m_renderedDynamicDecals( 0 )
		, m_renderedDynamicDecalsCount( 0 )
		, m_occludedStaticProxies( 0 )
		, m_occludedDynamicProxies( 0 )
		, m_occludedFurthestProxies( 0 )
		, m_occludedDynamicDecals( 0 )

		// shadows
		, m_shadowQueryTime( 0.0 )
		, m_stStatic( 0.0 )
		, m_stStaticDistance( 0.0 )
		, m_stStaticCollection( 0.0 )
		, m_stStaticCulledByDistance( 0 )

		, m_stDynamic( 0.0 )
		, m_stDynamicDistance( 0.0 )
		, m_stDynamicUmbra( 0.0 )
		, m_stDynamicCollection( 0.0 )
	{};

	MeshDrawingStats& operator+=( const MeshDrawingStats& other )
	{
		m_numBatches 						+= other.m_numBatches;
		m_numInstancedBatches 				+= other.m_numInstancedBatches;
		m_biggestBatch 						= Max( m_biggestBatch, other.m_biggestBatch );
		m_biggestInstancedBatch 			= Max( m_biggestInstancedBatch, other.m_biggestInstancedBatch );
		m_smallestBatch 					= Min( m_smallestBatch, other.m_smallestBatch );
		m_smallestInstancedBatch 			= Min( m_smallestInstancedBatch, other.m_smallestInstancedBatch );

		m_numChunks 						+= other.m_numChunks;
		m_numTriangles						+= other.m_numTriangles;
		m_numVertices						+= other.m_numVertices;

		m_numChunksStatic					+= other.m_numChunksStatic;
		m_numTrianglesStatic				+= other.m_numTrianglesStatic;
		m_numVerticesStatic					+= other.m_numVerticesStatic;

		m_numChunksSkinned					+= other.m_numChunksSkinned;
		m_numTrianglesSkinned				+= other.m_numTrianglesSkinned;
		m_numVerticesSkinned				+= other.m_numVerticesSkinned;

		m_visibleObjects					= other.m_visibleObjects;
		m_occlusionTimeQuery				= other.m_occlusionTimeQuery;
		m_occlusionTimeDynamicObjects		= other.m_occlusionTimeDynamicObjects;
		m_occlusionTimeVisibilityByDistance	= other.m_occlusionTimeVisibilityByDistance;

		m_furthestProxiesTime				= other.m_furthestProxiesTime;
		m_furthestProxiesOcclusionTime		= other.m_furthestProxiesOcclusionTime;
		m_furthestProxiesDistanceTime		= other.m_furthestProxiesDistanceTime;
		m_furthestProxiesCollectionTime		= other.m_furthestProxiesCollectionTime;

		m_renderedShadowStaticProxies		= other.m_renderedShadowStaticProxies;
		m_renderedShadowDynamicProxies		= other.m_renderedShadowDynamicProxies;

		m_renderedStaticProxies				= other.m_renderedStaticProxies;
		m_renderedDynamicProxies			= other.m_renderedDynamicProxies;
		m_renderedFurthestProxies			= other.m_renderedFurthestProxies;
		m_renderedDynamicDecals				= other.m_renderedDynamicDecals;
		m_renderedDynamicDecalsCount		= other.m_renderedDynamicDecalsCount;

		m_occludedStaticProxies				= other.m_occludedStaticProxies;
		m_occludedDynamicProxies			= other.m_occludedDynamicProxies;
		m_occludedFurthestProxies			= other.m_occludedFurthestProxies;
		m_occludedDynamicDecals				= other.m_occludedDynamicDecals;

		m_shadowQueryTime					= other.m_shadowQueryTime;
		m_stStatic							= other.m_stStatic;
		m_stStaticDistance					= other.m_stStaticDistance;
		m_stStaticCollection				= other.m_stStaticCollection;
		m_stStaticCulledByDistance			= other.m_stStaticCulledByDistance;

		m_stDynamic							= other.m_stDynamic;
		m_stDynamicDistance					= other.m_stDynamicDistance;
		m_stDynamicUmbra					= other.m_stDynamicUmbra;
		m_stDynamicCollection				= other.m_stDynamicCollection;

		return *this;
	}

	void Append( Uint8 vertexFactory, Uint16 numVertices, Uint16 numIndices, Uint32 fragCount, Uint32 instanceCount )
	{
#ifndef RED_FINAL_BUILD

		// Stats
		const Uint32 totalNumTriangles = instanceCount * numIndices / 3 * fragCount;
		const Uint32 totalNumVertices = instanceCount * numVertices;

		if ( vertexFactory == MVF_MeshStatic )
		{
			m_numChunksStatic += instanceCount;
			m_numTrianglesStatic += totalNumTriangles;
		}
		if ( vertexFactory == MVF_MeshSkinned || vertexFactory == MVF_MeshDestruction )
		{
			m_numChunksSkinned += instanceCount;
			m_numTrianglesSkinned += totalNumTriangles;
		}

		m_numChunks += instanceCount;
		m_numTriangles += totalNumTriangles;
		m_numVertices += totalNumVertices;

#endif // !RED_FINAL_BUILD
	}
};
