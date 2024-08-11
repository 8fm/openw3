/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderFrameInfo.h"
#include "renderVisibilityQuery.h"

class CComponent;
class BrushRenderData;
class BrushRenderFace;
class SCookedTerrainChunk;
class CFoliageInstance;
class CFoliageDefinition;

#define DYNAMIC_COLLISION_LIMIT 16

struct SCollisionsDataConstants
{		
	Matrix					m_transformMatrices[ DYNAMIC_COLLISION_LIMIT ];
	Uint32					m_activeCollisionsNum;

	SCollisionsDataConstants()
	{
		for(Uint32 i=0; i<DYNAMIC_COLLISION_LIMIT; i++)
		{	
			m_transformMatrices[ i ] = Matrix::IDENTITY;
		}
		m_activeCollisionsNum = 0;
	};
};

/// Type of rendering proxy
enum ERenderProxyType : Int32
{
	RPT_None		= 0,		//!< Invalid type
	RPT_Mesh		= 1,		//!< Proxy for mesh ( static, skinned )
	RPT_Flare		= 2,		//!< Proxy for flare
	RPT_PointLight	= 3,		//!< Proxy for point light
	RPT_SpotLight	= 4,		//!< Proxy for spot light
	RPT_Particles	= 5,		//!< Proxy for particles
	PRT_Foliage		= 6,		//!< Proxy for foliage
	RPT_Terrain		= 7,		//!< Proxy for terrain
	RPT_Water		= 8,		//!< Proxy for water
	RPT_Apex		= 9,		//!< Proxy for APEX destructible
	RPT_SSDecal		= 10,		//!< Proxy for screen-space decals
	RPT_Stripe		= 11,		//!< Proxy for stripe
	RPT_Fur			= 12,		//!< Proxy for fur
	RPT_Dimmer		= 13,		//!< Proxy for dimmer
	RPT_Swarm		= 14,		//!< Proxy for swarm
};

class CRenderProxyTypeFlag
{
public:
	RED_INLINE CRenderProxyTypeFlag() : m_mask( 0 ) {}
	RED_INLINE explicit CRenderProxyTypeFlag( ERenderProxyType x ) : m_mask( 1 << (Uint32)x ) {}

	RED_INLINE CRenderProxyTypeFlag operator|( const CRenderProxyTypeFlag& other ) const
	{
		return CRenderProxyTypeFlag( m_mask | other.m_mask );
	}

	RED_INLINE CRenderProxyTypeFlag operator|( const ERenderProxyType other ) const
	{
		return CRenderProxyTypeFlag( m_mask | (1 << (Uint32)other) );
	}

	RED_INLINE CRenderProxyTypeFlag operator&( const CRenderProxyTypeFlag& other ) const
	{
		return CRenderProxyTypeFlag( m_mask & other.m_mask );
	}

	RED_INLINE CRenderProxyTypeFlag operator&( const ERenderProxyType other ) const
	{
		return CRenderProxyTypeFlag( m_mask & (1 << (Uint32)other) );
	}

	RED_INLINE Bool operator==( const Uint32 value ) const
	{
		return m_mask == value;
	}
	
	RED_INLINE Bool operator!=( const Uint32 value ) const
	{
		return m_mask != value;
	}

private:
	Uint32 m_mask;

	CRenderProxyTypeFlag( Uint32 mask ): m_mask(mask) {}
};

RED_INLINE CRenderProxyTypeFlag operator|( ERenderProxyType a, ERenderProxyType b )
{
	return CRenderProxyTypeFlag(a) | CRenderProxyTypeFlag(b);
}

RED_INLINE CRenderProxyTypeFlag operator|( ERenderProxyType a, CRenderProxyTypeFlag b )
{
	return CRenderProxyTypeFlag(a) | b;
}

/// Fade type
enum EFadeType : Int32
{
	FT_None,				//!< No fade
	FT_FadeIn,				//!< Fade in
	FT_FadeInStart,			//!< Fade in, meant to be used on proxies during initialization.
	FT_FadeOut,				//!< Fade out
	FT_FadeOutAndDestroy,	//!< Fade out and destroy yourself 
};

/// Packed render data
class RenderProxyInitData
{
public:
	Matrix							m_localToWorld;
	Uint32							m_occlusionId;

	RenderProxyInitData( ERenderProxyType type, const Uint32 sizeofData );

	RED_FORCE_INLINE const ERenderProxyType GetType() const { return m_type; }

private:
	ERenderProxyType				m_type;
};

/// Packed mesh data
class RenderProxyMeshInitData : public RenderProxyInitData
{
public:
	const class CMesh*				m_mesh;
	Uint8							m_lightChannels;
	Int8							m_forcedLODLevel;
	Int8							m_shadowBias;
	Uint8							m_renderingPlane;
	Uint32							m_occlusionId;
	Uint8							m_castingShadows:1;
	Uint8							m_castingShadowsFromLocalLightsOnly:1;
	Uint8							m_cameraTransformTranslate:1;
	Uint8							m_cameraTransformRotate:1;
	Uint8							m_castingShadowsAlways:1;
	Uint8							m_noAutoHide:1;
	Uint8							m_noFOVAdjustedLOD:1;
	Uint8							m_fadeOnCameraCollision:1;
	Uint8							m_visible:1;
	Uint8							m_useCustomReferencePoint:1;
	Uint8							m_forceNoUmbraCulling:1;
	Float							m_autoHideDistance;
	Vector							m_customReferencePoint;

	RenderProxyMeshInitData();
};

/// Packed dimmer data
class RenderProxyDimmerInitData : public RenderProxyInitData
{
public:
	Float							m_autoHideDistance;
	Uint8							m_dimmerType;
	Float							m_ambientLevel;
	Float							m_marginFactor;
	Uint8							m_areaMarker:1;

	RenderProxyDimmerInitData();
};

/// Packed decal data
class RenderProxyDecalInitData : public RenderProxyInitData
{
public:
	Float							m_autoHideDistance;
	Float							m_fadeTime;
	const class CBitmapTexture*		m_texture;
	Color							m_specularColor;
	Float							m_normalThreshold;
	Float							m_specularity;
	Uint8							m_verticalFlip:1;
	Uint8							m_horizontalFlip:1;

	RenderProxyDecalInitData();
};

/// Packed light data
class RenderProxyLightInitData : public RenderProxyInitData
{
public:
	Color							m_color;
	Float							m_radius;
	Float							m_brightness;
	Float							m_attenuation;
	Float							m_autoHideDistance;
	Float							m_autoHideRange;
	Float							m_shadowFadeDistance;
	Float							m_shadowFadeRange;
	Float							m_shadowBlendFactor;
	Vector							m_lightFlickering;
	Uint8							m_shadowCastingMode;
	Uint8							m_dynamicShadowsFaceMask;
	Uint8							m_envColorGroup;
	Uint32							m_lightUsageMask;
	Uint8							m_allowDistanceFade:1;

	RenderProxyLightInitData( ERenderProxyType type, const Uint32 sizeofData );
};

/// Packed light data
class RenderProxyPointLightInitData : public RenderProxyLightInitData
{
public:
	Uint8							m_dynamicShadowsFaceMask;

	RenderProxyPointLightInitData();
};

/// Packed light data
class RenderProxySpotLightInitData : public RenderProxyLightInitData
{
public:
	Float							m_innerAngle;
	Float							m_outerAngle;
	Float							m_softness;
	Float							m_projectionTextureAngle;
	Float							m_projectionTexureUBias;
	Float							m_projectionTexureVBias;
	const class CBitmapTexture*		m_projectionTexture;

	RenderProxySpotLightInitData();
};

/// Render proxy initialization info
class RenderProxyInitInfo
{
public:
	const CComponent*						m_component;			//!< Source component
	TRenderVisibilityQueryID				m_visibilityQuery;		//!< Visibility query responsible for collecting visibility status of this proxy being rendered
	class IRenderEntityGroup*				m_entityGroup;			//!< Shared entity group for rendering
	const RenderProxyInitData*				m_packedData;			//!< Packed rendering data
	Bool									m_usesVertexCollapse;	//!< Proxy uses vertex collapse

public:
	RenderProxyInitInfo();

	//! Extract world space bounding box
	Box ExtractBoundingBox() const;

	//! Extract local to world matrix
	Matrix ExtractLocalToWorld() const;

	//! Extract local transform matrix (used by camera transforms)
	Matrix ExtractLocalMatrix() const;

	//! Extract rendering plane
	Uint8 ExtractRenderingPlane() const;

	//! Is camera transform
	Bool IsCameraTransform() const;

	//! Is camera transform
	Bool IsCameraTransformWithRotation() const;

	//! Is camera transform
	Bool IsCameraTransformWithoutRotation() const;

	//! Is this proxy a foreground object
	Bool IsCastingShadowsFromLocalLightsOnly() const;
};

/// Proxy update info
class RenderProxyUpdateInfo
{
public:
	const Box*		m_boundingBox;		//!< New bounding box ( should point to new bounding box )
	const Matrix*	m_localToWorld;		//!< New local to world matrix ( should point to new local to world matrix )

public:
	RenderProxyUpdateInfo();
};

/// Proxy to rendering scene object
class IRenderProxy : public IRenderObject
{
	DECLARE_RENDER_OBJECT_MEMORYCLASS( MC_RenderProxy )
protected:
	ERenderProxyType		m_type;

public:
	//! Get type of proxy
	RED_INLINE ERenderProxyType GetType() const { return m_type; }

	//! Set selection color
	virtual void SetSelectionColor( const Vector &/*selectionColor*/ ){};

public:
	IRenderProxy( ERenderProxyType type );
	virtual ~IRenderProxy();

	virtual void QueryVisible( Bool & /*visible*/ ) {};
	virtual void SetVisible( Bool /*visible*/ ) {};

#ifndef NO_DEBUG_PAGES
	virtual void GetDescription( TDynArray< String >& /*descriptionLines*/ ) const {}
	virtual Bool ValidateOptimization( TDynArray< String >* /*commentLines*/ ) const { return true; }
#endif
};

/// Artist side scene rendering stats
struct SceneRenderingStats
{
	enum { ENVPROBE_STATS_CAPACITY = 7 };

	/// EnvProbe rendering stats
	struct EnvProbeStats
	{
		void Reset()
		{
			Red::System::MemorySet( this, 0, sizeof(EnvProbeStats) );
		}

		Uint32		m_debugId;
		Bool		m_hasProbeObject;
		Bool		m_isGlobalProbe;
		Bool		m_isDuringUpdate;
		Uint32		m_weightOneThousands;
		Uint32		m_lastUpdateDelayMillis;
		Uint32		m_lastUpdateDurationMillis;
	};
	
	struct GpuTimesStat
	{
		GpuTimesStat( Float time, const String& name, const String& parent = String::EMPTY )
			: m_name( name )
			, m_parent( parent )
			, m_time( time ) {}

		String	m_name;
		String	m_parent;
		Float	m_time;
	};

	Uint32		m_numSceneTriangles;		//!< Number of triangles rendered
	Uint32		m_numSceneChunks;			//!< Number of mesh chunks rendered

	Uint32		m_numSceneTrianglesSkinned;	//!< Number of skinned triangles rendered
	Uint32		m_numSceneChunksSkinned;	//!< Number of skinned mesh chunks rendered
	Uint32		m_numSceneVertsSkinned;

	Uint32		m_numSceneTrianglesStatic;	//!< Number of static triangles rendered
	Uint32		m_numSceneChunksStatic;		//!< Number of static mesh chunks rendered
	Uint32		m_numSceneVertsStatic;

	Uint32		m_numSceneBatches;			//!< Number of batches (instanced or not)
	Uint32		m_numSceneVerts;			//!< Number of vertices

	Uint32		m_numSceneInstancedBatches;		//!< Number of instanced batches
	Uint32		m_biggestSceneBatch;			//!< Biggest mesh batch in the scene	
	Uint32		m_biggestSceneInstancedBatch;	//!< Biggest instanced mesh batch in the scene
	Uint32		m_smallestSceneBatch;
	Uint32		m_smallestSceneInstancedBatch;	//!< Smallest instanced mesh batch in the scene
	TDynArray< GpuTimesStat >	m_gpuTimeStats;	//!< array with gpu times
	// rendering device stats
	Uint32		m_numConstantBufferUpdates;		//!< Number of constant buffer updates
#ifdef RED_PLATFORM_CONSOLE
	Uint32		m_constantMemoryLoad;			//!< Amount of constant data memory being pushed
	Bool		m_isConstantBufferSafe;			//!< Is per-frame constant buffer memory usage within safe bounds?
#endif

	// terrain hardware stats
	Uint64		m_terrainVerticesRead;					//!< Vertices read by the GPU
	Uint64		m_terrainPrimitivesRead;				//!< Primitves read by the GPU
	Uint64		m_terrainVertexShaderInvocations;		//!< Number of Vertex Shader invocations
	Uint64		m_terrainPrimitivesSentToRasterizer;	//!< Number of primitves sent to the rasterizer
	Uint64		m_terrainPrimitivesRendered;			//!< Number of primitives actually rendered, including cutting the offscreen parts.
	Uint64		m_terrainPixelShaderInvocations;		//!< Number of pixel shader incovations
	Uint64		m_terrainHullShaderInvocations;			//!< Number of hull shader invocations
	Uint64		m_terrainDomainShaderInvocations;		//!< Number of domain shader incovations

	// dimmers
	Uint32		m_numDimmers;

	// lights
	Uint32		m_numLights;
	Uint32		m_numLightWithShadows;
	Uint32		m_numLightWithStaticShadows;
	Uint32		m_numDistantLights;

	// more stats for particles
	Uint32		m_numParticles;					//!< Number of active particles
	Uint32		m_numParticleEmitters;			//!< Number of particle emitters
	Uint32		m_numParticleMeshChunks;		//!< Number of particle chunks
	Uint32		m_numParticleMeshTriangles;		//!< Number of particle chunks

	// general shadows for shadow cascades
	Uint32		m_numCascadeProxiesTotal;				//!< Total number of mesh proxies rendered into the shadow cascades
	Uint32		m_numCascadeChunksTotal;				//!< Total number of mesh chunks rendered into the shadow cascades
	Uint32		m_numCascadeTrianglesTotal;				//!< Total number of triangles rendered into the shadow cascades
	Uint32		m_numCascadeProxiesOptimized;			//!< Number of proxies that were optimized away because they were fully enclosed in smaller cascade
	Uint32		m_numCascadeProxiesTerrainCulled;		//!< Number of proxies that were culled because they were in the terrain shadows
	Uint32		m_numCascadeProxiesFrustumCulled;		//!< Number of proxies that were culled because they are not in the frustum
	Uint32		m_numCascadeProxiesExlusionFiltered;	//!< Number of proxies that were optimized because of visibility exclusion

	// specialized stats for each cascade
	Uint32		m_numCascadeProxies[ MAX_CASCADES ];		//!< Per cascade number of rendered mesh proxies
	Uint32		m_numCascadeChunks[ MAX_CASCADES ];			//!< Per cascade number of mesh chunks rendered
	Uint32		m_numCascadeTriangles[ MAX_CASCADES ];		//!< Per cascade number of triangles from mesh proxies

	// specialized shadows for dynamic shadows
	Uint32		m_numDynamicShadowsRegions;					//!< Number of allocated and rendered dynamic shadowmap regions
	Uint32		m_numDynamicShadowsProxies;					//!< Number of mesh proxies rendered into dynamic shadows
	Uint32		m_numDynamicShadowsChunks;					//!< Number of mesh chunks rendered into dynamic shadows
	Uint32		m_numDynamicShadowsTriangles;				//!< Number of mesh triangles rendered into dynamic shadows

	// specialized stats for static shadows
	Uint32		m_numStaticShadowsCubes;					//!< Number of static cubes rendered this frame
	Uint32		m_numStaticShadowsProxies;					//!< Number of mesh proxies rendered into static cubes this frame
	Uint32		m_numStaticShadowsChunks;					//!< Number of mesh chunks rendered into static cubes this frame
	Uint32		m_numStaticShadowsTriangles;				//!< Number of mesh triangles rendered into static cubes this frame

	// terrain
	Uint32		m_numTerrainShadowsProxies;					//!< Number of proxies rendered into the terrain shadow buffers
	Uint32		m_numTerrainShadowsChunks;					//!< Number of mesh chunks rendered into the terrain shadow buffers
	Uint32		m_numTerrainShadowsTriangles;				//!< Number of mesh triangles rendered into the terrain shadow buffers

	// decals
	Uint32		m_numDecalProxies;							//!< Number of rendered decal proxies
	Uint32		m_numDecalChunks;							//!< Number of rendered decal mesh chunks
	Uint32		m_numDecalTriangles;						//!< Number of rendered decal mesh triangles

	// error/warning flags
	Bool		m_flagDynamicShadowsReduces;				//!< Size of the dynamic shadowmap was reduced because there is to many lights
	Bool		m_flagDynamicShadowsLimit;					//!< Limit of dynamic lights was reached
	Bool		m_flagStaticShadowsLimit;					//!< Limit of static lights was reached

	// hi res actors
	Uint32		m_numHiResShadowsActors;					//!< Number of high-resolution actors rendered
	Uint32		m_numHiResShadowsProxies;					//!< Number of proxies rendered for high resolution shadows
	Uint32		m_numHiResShadowsChunks;					//!< Number of chunks rendered for high resolution shadows
	Uint32		m_numHiResShadowsTriangles;					//!< Number of triangles rendered for high resolution shadows

	// apex
	Uint32		m_numApexClothsUpdated;						//<! Number of apex cloths ticked
	Uint32		m_numApexClothsRendered;					//<! Number of apex cloths rendered
	Uint32		m_numApexClothsRenderedSM;					//<! Number of times apex cloths rendered into shadow maps
	Uint32		m_numApexDestructiblesUpdated;				//<! Number of apex destructibles ticked
	Uint32		m_numApexDestructiblesRendered;				//<! Number of apex destructibles rendered
	Uint32		m_numApexDestructiblesRenderedSM;			//<! Number of times apex destructibles rendered into shadow maps
	Uint32		m_numApexResourcesRendered;					//!< Number of apex render resources rendered (could be different from the number of cloths+destructibles)
	Uint32		m_numApexVBUpdated;							//!< Number of times Apex VB::writeBuffer is called (just the non-fast path calls are counted)
	Uint32		m_numApexIBUpdated;							//!< Number of times Apex IB::writeBuffer is called
	Uint32		m_numApexBBUpdated;							//!< Number of times Apex BB::writeBuffer is called
	Uint32		m_numApexVBSemanticsUpdated;				//!< Number of apex vertex buffer semantics updated. Each sematic gets written separately...
	Uint32		m_numApexVBUpdatedFastPath;					//!< Number of apex vertex buffers updated through the "fast path"

	// envProbes
	Uint32			m_numEnvProbeStats;
	EnvProbeStats	m_envProbeStats[ENVPROBE_STATS_CAPACITY];
	Uint32			m_totalEnvProbeObjects;
	Uint32			m_totalGlobalEnvProbeObjects;

	// flares
	Uint32		m_numActiveFlares;

	// Umbra
	Uint32		m_visibleObjects;
	Double		m_occlusionTimeQuery;						//!< Time spent in Umbra occlusion query
	Double		m_occlusionTimeDynamicObjects;				//!< Time spent checking occlusion of dynamic geometry
	Double		m_occlusionTimeVisibilityByDistance;		//!< Time spent determining visibility by distance
	Double		m_occlusionTimeTerrain;
	Double		m_occlusionTimeBuildingTerrainQuadtree;

	Double		m_furthestProxiesTime;
	Double		m_furthestProxiesOcclusionTime;
	Double		m_furthestProxiesDistanceTime;
	Double		m_furthestProxiesCollectionTime;

	// render element map
	Uint32		m_registeredStaticProxies;
	Uint32		m_registeredDynamicProxies;
	Uint32		m_registeredFurthestProxies;
	Uint32		m_registeredDynamicDecals;

	Uint32		m_renderedStaticProxies;
	Uint32		m_renderedDynamicProxies;
	Uint32		m_renderedFurthestProxies;
	Uint32		m_renderedDynamicDecals;
	Uint32		m_renderedDynamicDecalsCount;

	Uint32		m_occludedDynamicProxies;
	Uint32		m_occludedFurthestProxies;
	Uint32		m_occludedDynamicDecals;

	Uint32		m_renderedShadowStaticProxies;
	Uint32		m_renderedShadowDynamicProxies;

	Uint32		m_reMapStatsStaticMeshes;
	Uint32		m_reMapStatsDynamicMeshes;
	Uint32		m_reMapStatsMeshesNotInObjectCache;
	Uint32		m_reMapStatsApex;
	Uint32		m_reMapStatsBakedDecals;
	Uint32		m_reMapStatsNonBakedDecals;
	Uint32		m_reMapStatsBakedDimmers;
	Uint32		m_reMapStatsNonBakedDimmers;
	Uint32		m_reMapStatsBakedStripes;
	Uint32		m_reMapStatsNonBakedStripes;
	Uint32		m_reMapStatsFlares;
	Uint32		m_reMapStatsFur;
	Uint32		m_reMapStatsParticles;
	Uint32		m_reMapStatsBakedPointLights;
	Uint32		m_reMapStatsNonBakedPointLights;
	Uint32		m_reMapStatsBakedSpotLights;
	Uint32		m_reMapStatsNonBakedSpotLights;

	// shadows
	Double		m_shadowQueryTime;
	Double		m_stStatic;
	Double		m_stStaticDistance;
	Double		m_stStaticCollection;
	Uint32		m_stStaticCulledByDistance;

	Double		m_stDynamic;
	Double		m_stDynamicDistance;
	Double		m_stDynamicUmbra;
	Double		m_stDynamicCollection;

	SceneRenderingStats()
	{
		Red::System::MemorySet( this, 0, sizeof(SceneRenderingStats) );
	}
};

/// Proxy to rendering scene
class IRenderScene : public IRenderObject
{
public:
	virtual ~IRenderScene() {};

	//! Get current render stats
	virtual SceneRenderingStats GetRenderStats() const=0;

	//! Create visibility query wrapper, may return NULL. Note - this is no longer returning an abstract interface
	virtual TRenderVisibilityQueryID CreateVisibilityQuery() = 0;

	//! Release visibility query wrapper
	virtual void ReleaseVisibilityQuery( const TRenderVisibilityQueryID ) = 0;

	//! Get the result of a query
	virtual enum ERenderVisibilityResult GetVisibilityQueryResult( const TRenderVisibilityQueryID queryId ) const = 0;

	//! Get the result of a query
	virtual Bool GetVisibilityQueryResult( Uint32 elementsCount, Int16* indexes, void* inputPos, void* outputPos, size_t stride ) const = 0;

};