/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderEnvProbeDynamicData.h"
#include "renderCollector.h"
#include "renderScene.h"
#include "../engine/envProbeParams.h"

class CRenderEnvProbe;
class CRenderInterface;

/// Env probe type
enum EEnvProbeType
{
	ENVPROBETYPE_Ambient,
	ENVPROBETYPE_Reflection,

	ENVPROBETYPE_MAX,
};

/// Render EnvProbe manager
class CRenderEnvProbeManager
{
public:
	enum
	{
		GLOBAL_SLOT_INDEX				= 0,

		AMBIENT_CUBE_RESOLUTION			= 16,
		REFLECTION_CUBE_RESOLUTION		= 128,
		MAX_CUBE_RESOLUTION				= AMBIENT_CUBE_RESOLUTION > REFLECTION_CUBE_RESOLUTION ? AMBIENT_CUBE_RESOLUTION : REFLECTION_CUBE_RESOLUTION,

		CUBE_ARRAY_CAPACITY				= 7,

		AMBIENT_CUBE_NUM_MIPS			= 2,
		REFLECTION_CUBE_NUM_MIPS		= 6,
	};

public:
	struct SCubeSlotData
	{
		SCubeSlotData ();

		void ImportRenderEnvProbeData( const CRenderEnvProbe &envProbe );
		const GpuApi::TextureRef& GetFaceTexture( eEnvProbeBufferTexture texType ) const;
		
		Float m_weight;
		Bool m_needsReflectionBaseUpdate;
		Bool m_isInReflectionAtlas;
		Uint32 m_reflectionBaseUpdateCounter;
		SEnvProbeParams m_probeParams;
		GpuApi::TextureRef m_faceTextures[ENVPROBEBUFFERTEX_MAX];			//< ace_optimize: we can get rid of most of these since we have cached unpacked textures
	};

	struct SGenPerFaceData
	{
		void Reset()
		{
			tiledConstantsSnaphot.ClearFast();
		}

		TDynArray<Uint8, MC_EnvProbeRender> tiledConstantsSnaphot;
	};

private:
	GpuApi::TextureRef	m_tempFaceTexture;
	GpuApi::TextureRef	m_intermediateReflectionCube;					// ace_optimize: redundant
	GpuApi::TextureRef  m_tempParaboloid[2];
	GpuApi::TextureRef  m_mergeSumTexture;
	GpuApi::TextureRef  m_cachedUnpackedAmbientScene;
	GpuApi::TextureRef  m_cachedUnpackedAmbientSkyFactor;
	GpuApi::TextureRef	m_cachedUnpackedAlbedo;
	GpuApi::TextureRef	m_cachedUnpackedNormals;
	GpuApi::TextureRef	m_cachedUnpackedDepthAndSky;
	GpuApi::TextureRef	m_reflectionAtlasBaseCurrent;
	GpuApi::TextureRef	m_reflectionAtlasBaseBlendSrc;
	GpuApi::TextureRef	m_reflectionAtlasBaseBlendDest;
	GpuApi::TextureRef  m_ambientAtlas;
	GpuApi::TextureRef	m_reflectionAtlas;
	GpuApi::TextureRef	m_tempReflectionAtlas;		// ace_optimize: get rid of this
	GpuApi::TextureRef	m_surfaceShadowMask;		// ace_optimize: use some other texture
	TDynArray< SCollectedEnvProbeLight > m_tempCollectedLightsArray;	
	TDynArray< IRenderProxyLight* > m_tempLightsArray;	
	TDynArray<SCubeSlotData>	m_cubeSlotsData;
	Uint32						m_nestingOrder[CUBE_ARRAY_CAPACITY];
	Float						m_lastUpdateTime;
	Float						m_lastUpdateGameTime;
	CRenderEnvProbe				*m_pCurrGenProbe;
	EnvProbeDataSourcePtr		m_currLoadingDataSource;
	EnvProbeDataSourcePtr		m_cancelledLoadingDataSource; //< data source which we want to wait until it's finished until new data source gets issued to load (because of async data loading which can't be cancelled immediately), to prevent going overbudget (buffers async data source loads into are budgeted)
	Float						m_currGenStartUpdateTime;
	Int32						m_currGenProgress;
	Float						m_currGenBlendoverFactor;
	Float						m_currGenBlendoverAccum;
	CRenderFrameInfo			m_currGenRenderFrameInfo;
	SGenPerFaceData				m_currGenPerFaceData[6];
	Uint32						m_finalCompositionCounter;
	Bool						m_isHelperTexturesInit;
	Bool							m_isDuringPrefetch;
	Bool							m_isPostPrefetch;
	Bool							m_needsPostPrefetchUpdate;
	Vector							m_prefetchPosition;

public:
	CRenderEnvProbeManager();
	~CRenderEnvProbeManager();
		
	/// Update manager
	void Update( CRenderCollector &collector );

	/// Get ambient envprobe cubes texture array
	const GpuApi::TextureRef& GetAmbientEnvProbeAtlas() const { return m_ambientAtlas; }

	/// Get reflection envprobe texture atlas
	const GpuApi::TextureRef& GetReflectionEnvProbeAtlas() const { return m_reflectionAtlas; }

	/// Get given slot data
	const SCubeSlotData& GetCubeSlotData( Uint32 slotIndex ) const { return m_cubeSlotsData[slotIndex]; }

	/// Get slots data nesting order
	const Uint32* GetCubeSlotsNestingOrder() const { return m_nestingOrder; }

public:
	static Uint32 GetEnvProbeDataSourceSize();
	static Uint32 GetMaxProbeTypeResolution();
	static Uint16 GetEnvProbeTypeNumMips( EEnvProbeType probeType );

public:
	void SetupPrefetch( const Vector &position );
	Bool IsDuringPrefetch() const;
	void UpdatePrefetch();
	void CancelPrefetch();

protected:
	/// Init helper textures
	void InitHelperTextures();

	/// Update envProbes
	void UpdateEnvProbes( const CRenderCollector &mainRenderCollector, CRenderEnvProbe *forcedProbeToRender, Float updateTimeDelta, Bool forceInstantUpdate );

	/// Update slots
	void UpdateSlots( const Vector &cameraPosition, Float updateTimeDelta, Bool enableFastSelection );

	/// Update rendering stats
	void UpdateRenderingStats();

	/// Update blendover
	void UpdateBlendover( Float updateTimeDelta );

	/// Unpack intermediate cube
	void UnpackIntermediateCube( CRenderCubeTexture *interiorFallbackCube, GpuApi::TextureRef texSource, GpuApi::TextureRef texTarget, Uint32 cubeRowIndex, Bool sampleLinear );

	/// Filter reflection atlas
	void FilterReflectionAtlas( const GpuApi::TextureRef &texReflectionAtlas, Int32 segmentIndex, Bool usePrepareMerge );

	/// Copy to temp paraboloid
	void CopyToTempParaboloid( const GpuApi::TextureRef &texReflectionAtlas );

	/// Downscale temp paraboloid
	void DownscaleTempParaboloid( const GpuApi::TextureRef &texReflectionAtlas, Int32 segmentIndex, Uint32 mipIndex, Bool usePrepareMerge );

	/// Blur temp paraboloid
	void BlurTempParaboloid( const GpuApi::TextureRef &texReflectionAtlas, Int32 segmentIndex, Uint32 mipIndex, Bool usePrepareMerge );

	/// Merge ambient atlas
	void MergeAmbientAtlas( Float gameTime, const CEnvAmbientProbesGenParametersAtPoint &ambientParams );

	/// Merge ambient atlas
	void MergeReflectionAtlas( CRenderCollector &collector, const GpuApi::TextureRef &texReflectionAtlas, const GpuApi::TextureRef &texMergeSource );

	/// Init cached ambient textures
	void InitCachedUnpackedAmbientTextures( const SRenderEnvProbeDynamicData &dynData, CRenderEnvProbeManager::SCubeSlotData &slotData );

	/// Init cached reflection textures
	void InitCachedUnpackedReflectionTextures( const CRenderEnvProbe &envProbe, CRenderEnvProbeManager::SCubeSlotData &slotData );

	/// Update given envProbe
	Bool UpdateEnvProbe( const CRenderCollector &mainRenderCollector, CRenderInterface &renderer, Float updateTimeDelta );

protected:
	Bool ProbeQualifiesForPrefetch( const CRenderEnvProbe &envProbe ) const;
	Bool ProbeQualifiesForUpdate( const CRenderFrameInfo &info, const CRenderEnvProbe &envProbe ) const;
	SCubeSlotData& RefCubeSlotData( Uint32 slotIndex ) { return m_cubeSlotsData[slotIndex]; }
	Bool UpdateCurrEnvProbeFaceTexturesLoading( Bool allowUnpack );
	void UpdateNestingOrder();
	void CancelCurrProbeUpdate();
};


//
// These are inlined here, so that they can be used for cooking as well, without duplicating code.
//

RED_INLINE Uint32 GetEnvProbeDataSourceResolution()
{
	return Max( CRenderEnvProbeManager::AMBIENT_CUBE_RESOLUTION, CRenderEnvProbeManager::REFLECTION_CUBE_RESOLUTION );
}

RED_INLINE Uint32 GetEnvProbeDataSourceFaceSize()
{
	const Uint32 dataSourceSize = CRenderEnvProbeManager::GetEnvProbeDataSourceSize();
	ASSERT( 0 == dataSourceSize % 6 );
	return dataSourceSize / 6;
}

RED_INLINE Uint32 GetEnvProbeDataSourceBufferOffset( eEnvProbeBufferTexture buffType )
{
	const Uint32 resolution = GetEnvProbeDataSourceResolution();
	const Uint32 fullBuffSize = 4 * resolution * resolution;

	Uint32 off = 0;
	if ( buffType > ENVPROBEBUFFERTEX_Albedo )
		off += fullBuffSize / 2;
	if ( buffType > ENVPROBEBUFFERTEX_Normals )
		off += fullBuffSize * 3 / 4;
	if ( buffType > ENVPROBEBUFFERTEX_GlobalShadow )
		off += fullBuffSize;
	if ( buffType > ENVPROBEBUFFERTEX_Depth )
		off += fullBuffSize / 2;

	return off;
}

RED_INLINE Uint32 GetEnvProbeDataSourceBufferOffset( Uint32 faceIndex, eEnvProbeBufferTexture buffType )
{
	return faceIndex * GetEnvProbeDataSourceFaceSize() + GetEnvProbeDataSourceBufferOffset( buffType );
}


RED_INLINE Uint32 CRenderEnvProbeManager::GetMaxProbeTypeResolution()
{
	return MAX_CUBE_RESOLUTION;
}

RED_INLINE Uint32 CRenderEnvProbeManager::GetEnvProbeDataSourceSize()
{
	const Uint32 resolution = GetEnvProbeDataSourceResolution();
	return 6 * (2 + 3 + 4 + 2) * resolution * resolution;
}
