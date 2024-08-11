/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderHelpers.h"
#include "renderCollector.h"
#include "renderMeshStreaming.h"
#include "renderDefragHelper.h"
#include "../core/uniqueBuffer.h"
#include "../engine/renderer.h"
#include "../engine/renderVisibilityQuery.h"
#include "../engine/materialCompiler.h"
#include "../engine/inGameConfigRefreshEvent.h"
#include "../engine/inGameConfigGroupBasic.h"

class CRenderPostProcess;
class CRenderMeshBatcher;
class CRenderPartricleBatcher;
class CRenderAtlasManager;
class CRenderSkinManager;
class CRenderTerrainBatcher;
class CRenderFlaresBatcher;
class CRenderBrushFaceBatcher;
class CRenderDistantLightBatcher;
class CRenderDynamicDecalBatcher;
class CRenderSwarmBatcher;
class CRenderViewport;
class CRenderSceneEx;
class CRenderSurfaces;
class CRenderCollector;
class CRenderThread;
class CGameplayEffects;
class CRenderMaterial;
class CRenderMaterialParameters;
class CRenderLodBudgetSystem;
class CRenderTerrainShadows;
class CRenderShadowManager;
class CRenderStateManager;
class CRenderEnvProbe;
class CRenderEnvProbeManager;
struct STerrainTextureParameters;
struct SMergedShadowCascades;
class IRenderProxyLight;
class CRenderProxy_Dimmer;
class CRenderTextureStreamingManager;
class IPlatformViewport;
class CRenderDrawer;
class CRenderShaderPair;
class CRenderShaderQuadruple;
class CRenderShaderCompute;
class CRenderShaderTriple;
class CRenderShaderStreamOut;
class IEnvProbeDataSource;
class CRenderGamplayRenderTarget;
class CRenderVisibilityExclusionMap;
class CRenderFramePrefetch;

#ifdef USE_NVIDIA_FUR
class GFSDK_HairSDK;
class GFSDK_HAIR_LogHandler;
#endif

#ifdef USE_APEX
class CRenderApexBatcher;
#endif

class IRenderScaleform;

#ifdef USE_SCALEFORM
class CRenderScaleform;
#endif

extern Bool GIsRendererTakingUberScreenshot;
extern Bool GIsDuringUberSampleIsFirst;
extern Uint32 GLastScreenshotIndex;

enum ERenderingDeviceState
{
	ERDS_Operational,
	ERDS_Lost_ResourcesNotReleased,
	ERDS_Lost_ResourcesReleased,
	ERDS_NotReset,
};

/// Cascade shadow resources
class CCascadeShadowResources : public Red::System::NonCopyable
{
private:
	Uint32								m_shadowmapResolution;
	Uint16								m_shadowmapMaxCascadeCount;
#ifdef RED_PLATFORM_DURANGO
	GpuApi::TextureRef					m_shadowmapDepthStencilArrayWrite;
#endif
	GpuApi::TextureRef					m_shadowmapDepthStencilArrayRead;
	Bool								m_isRegisteredDynamicTexture;

public:
	CCascadeShadowResources ();
	~CCascadeShadowResources ();

	void Init( Uint16 maxCascadeCount, Uint32 resolution, const char *dynamicTextureName );
	void Shut();

	Uint32 GetResolution() const { return m_shadowmapResolution; }
	Uint32 GetMaxCascadeCount() const { return m_shadowmapMaxCascadeCount; }
	const GpuApi::TextureRef& GetDepthStencilArrayWrite() const 
	{
#ifdef RED_PLATFORM_DURANGO
		return m_shadowmapDepthStencilArrayWrite;
#else
		return m_shadowmapDepthStencilArrayRead;
#endif
	}
	const GpuApi::TextureRef& GetDepthStencilArrayRead() const { return m_shadowmapDepthStencilArrayRead; }
};

struct SGlobalConstantBuffer
{
	Vector TimeVector;
	Vector ViewportSize;
	Vector ViewportSubSize;				
	Vector EnvTranspFilterParams;
	Vector EnvTranspFilterDistMinColor;
	Vector EnvTranspFilterDistMaxColor;
	Vector WeatherShadingParams;
	Vector WeatherAndPrescaleParams;
	Vector SkyboxShadingParams;	
	Vector GlobalLightDir;	
	Vector GlobalLightColorAndSpecScale;
	Vector WaterShadingParams;
	Vector WaterShadingParamsExtra;
	Vector WaterShadingParamsExtra2;
	Vector UnderWaterShadingParams;
	Vector GameplayEffectsRendererParameter;
};

#ifndef RED_FINAL_BUILD

struct SRenderGpuProfilerEntry
{
	Float	m_lastTime;
	String	m_name;
	Bool	m_pending;
	Bool	m_finished;
};

struct SRenderGpuFastProfilerEntry : SRenderGpuProfilerEntry
{
	GpuApi::QueryRef m_timestampQueryStart;
	GpuApi::QueryRef m_timestampQueryEnd;
};

#define NUM_QUERY_STAMPS_MAX 20

struct SRenderGpuHierarchyProfilerEntry : SRenderGpuProfilerEntry
{
	GpuApi::QueryRef				m_timestampQueryStart[NUM_QUERY_STAMPS_MAX];
	Uint32							m_timestampQueryStartCount;
	GpuApi::QueryRef				m_timestampQueryEnd[NUM_QUERY_STAMPS_MAX];
	Uint32							m_timestampQueryEndCount;

	Uint32							m_timestampQueryStartCountMax;
	Uint32							m_timestampQueryEndCountMax;

	Uint32							m_frame;
	Uint32							m_id;	
};

struct GpuScopeInfo
{
	GpuScopeInfo()
		: m_name( CName::NONE )
		, m_parent( CName::NONE ) {}

	GpuScopeInfo( const CName name, const CName parent )
		: m_name( name )
		, m_parent( parent ) {}

	CName m_name;
	CName m_parent;
	Uint32 CalcHash() const
	{
		Uint32 hash = GetHash( m_name + m_parent );
		return hash;
	}

	bool operator==( const GpuScopeInfo& a ) const
	{
		return a.m_name == m_name && a.m_parent == m_parent;
	}
};

class IRenderGpuProfiler
{
public:
	virtual ~IRenderGpuProfiler() {}

	virtual Uint32 AddEntry( const String& name )=0;
	// returns entryId, which may be different that scopeId passed as parameter
	virtual Uint32 StartEntry( const String& name, Uint32 scopeId )=0;
	// use id returned by StartEntry
	virtual void EndEntry( Uint32 entryId )=0;
	virtual Float GetLastTime( Uint32 entryId )=0;
	virtual Uint32 GetEntryCount() const=0;
};


class CRenderGpuFastProfiler : IRenderGpuProfiler
{
private:
	SRenderGpuFastProfilerEntry		m_entries[256];
	Uint32							m_entryCount;

public:
	CRenderGpuFastProfiler()
		: m_entryCount( 0 )
	{
	}

	virtual ~CRenderGpuFastProfiler()
	{
		for ( Uint32 i = 0; i < m_entryCount; ++i )
		{
			GpuApi::SafeRelease( m_entries[i].m_timestampQueryStart );
			GpuApi::SafeRelease( m_entries[i].m_timestampQueryEnd );
		}
	}

	virtual Uint32 GetEntryCount() const
	{
		return m_entryCount;
	}

	const SRenderGpuFastProfilerEntry* GetEntries() const
	{
		return m_entries;
	}

	virtual Uint32 AddEntry( const String& name )
	{
		SRenderGpuFastProfilerEntry& entry = m_entries[ m_entryCount ];
		entry.m_name = name;
		entry.m_timestampQueryStart = GpuApi::CreateQuery( GpuApi::QT_Timestamp );
		entry.m_timestampQueryEnd = GpuApi::CreateQuery( GpuApi::QT_Timestamp );
		entry.m_pending = false;
		entry.m_finished = true;
		entry.m_lastTime = 0.f;
		m_entryCount++;
		return m_entryCount-1;
	}

	virtual Uint32 StartEntry( const String&name, Uint32 scopeId )
	{
		// scopeId is the same as entryId in fast profiler
		if ( scopeId > m_entryCount )
		{
			return 257;
		}

		SRenderGpuFastProfilerEntry& entry = m_entries[scopeId];
		if (!entry.m_finished)
		{
			return scopeId;
		}

		GpuApi::EndQuery( entry.m_timestampQueryStart );
		entry.m_pending = true;
		entry.m_finished = false;
		return scopeId;
	}

	virtual void EndEntry( Uint32 entryId )
	{
		if (entryId > m_entryCount)
		{
			return;
		}

		SRenderGpuFastProfilerEntry& entry = m_entries[entryId];
		if (!entry.m_pending)
		{
			return;
		}

		GpuApi::EndQuery( entry.m_timestampQueryEnd );
		entry.m_pending = false;
	}

	virtual Float GetLastTime( Uint32 entryId )
	{
		if (entryId > m_entryCount)
		{
			return -1.f;
		}

		SRenderGpuFastProfilerEntry& entry = m_entries[entryId];
		if (entry.m_pending)
		{
			return entry.m_lastTime;
		}

		if (!entry.m_finished)
		{
			Uint64 startTimestamp;
			Uint64 endTimestamp;
			GpuApi::eQueryResult queryResultStart = GpuApi::GetQueryResult( entry.m_timestampQueryStart, startTimestamp, false );
			GpuApi::eQueryResult queryResultEnd = GpuApi::GetQueryResult( entry.m_timestampQueryEnd, endTimestamp, false );

			if ( queryResultStart == GpuApi::QR_Success && queryResultEnd == GpuApi::QR_Success )
			{
				entry.m_lastTime = Float( endTimestamp - startTimestamp );
				entry.m_finished = true;
			}
		}

		return entry.m_lastTime;
	}
};


class CRenderGpuHierarchyProfiler : IRenderGpuProfiler
{

#define SCOPE_NAMES_MAX_COUNT 128

private:
	THashMap< GpuScopeInfo, SRenderGpuHierarchyProfilerEntry >	m_entries;
	GpuScopeInfo												m_alreadyStarted[256];
	Uint32														m_alreadyStartedCount;
	GpuScopeInfo												m_keys[256];

	CName														m_scopeNames[SCOPE_NAMES_MAX_COUNT];

	Uint32 m_keysCount;
	Uint32 m_frame;

	void AddEntry( const GpuScopeInfo& entryKey )
	{
		SRenderGpuHierarchyProfilerEntry entry;
		entry.m_name = entryKey.m_name.AsString();
		entry.m_timestampQueryStartCount = 0;
		entry.m_timestampQueryEndCount = 0;	
		entry.m_timestampQueryStartCountMax = 1;
		entry.m_timestampQueryEndCountMax = 1;
		entry.m_pending = false;
		entry.m_finished = true;
		entry.m_lastTime = 0.f;
		entry.m_frame = m_frame;
		entry.m_id = m_keysCount;

		entry.m_timestampQueryStart[ entry.m_timestampQueryStartCount++ ] = GpuApi::CreateQuery( GpuApi::QT_Timestamp );
		entry.m_timestampQueryEnd[ entry.m_timestampQueryEndCount++ ] = GpuApi::CreateQuery( GpuApi::QT_Timestamp );

		m_entries.Insert( entryKey, entry );

		m_keys[ m_keysCount++ ] = entryKey;
	}

public:
	CRenderGpuHierarchyProfiler()
		: m_frame( 0 )
		, m_keysCount( 0 )
		, m_alreadyStartedCount( 0 )
	{
	}

	virtual ~CRenderGpuHierarchyProfiler()
	{
		Reset();
	}

	void Reset()
	{
		for ( THashMap< GpuScopeInfo, SRenderGpuHierarchyProfilerEntry >::iterator it = m_entries.Begin(); it != m_entries.End(); ++it )
		{			
			for ( Uint32 i = 0; i < it->m_second.m_timestampQueryStartCountMax; ++i )
			{
				GpuApi::SafeRelease( it->m_second.m_timestampQueryStart[i] );
				GpuApi::SafeRelease( it->m_second.m_timestampQueryEnd[i] );
			}

			it->m_second.m_timestampQueryStartCount = 0;
			it->m_second.m_timestampQueryEndCount = 0;
			
			it->m_second.m_timestampQueryStartCountMax = 0;
			it->m_second.m_timestampQueryEndCountMax = 0;

		}
		m_entries.ClearFast();
		m_alreadyStartedCount = 0;
		m_keysCount = 0;
	}

	const GpuScopeInfo* GetScopesInfo() const
	{
		return m_keys;
	}

	virtual Uint32 GetEntryCount() const
	{
		return m_keysCount;
	}
	
	void ClearGatheredTimes()
	{
		m_frame = ( m_frame + 1 ) % 5;
		for ( THashMap< GpuScopeInfo, SRenderGpuHierarchyProfilerEntry >::iterator it = m_entries.Begin(); it != m_entries.End(); ++it )
		{
			if ( it->m_second.m_finished )
			{				
				if( it->m_second.m_timestampQueryEndCount > it->m_second.m_timestampQueryEndCountMax ) it->m_second.m_timestampQueryEndCountMax = it->m_second.m_timestampQueryEndCount;
				if( it->m_second.m_timestampQueryStartCount > it->m_second.m_timestampQueryStartCountMax ) it->m_second.m_timestampQueryStartCountMax = it->m_second.m_timestampQueryStartCount;

				it->m_second.m_timestampQueryStartCount = 1;
				it->m_second.m_timestampQueryEndCount = 1;
				it->m_second.m_frame = m_frame;
			}
		}
	}

	virtual Uint32 AddEntry( const String& name )
	{
		// do nothing - we only add new entries when starting them
		return 257;
	}

	virtual Uint32 StartEntry( const String& name, Uint32 scopeId )
	{
		if ( scopeId >= SCOPE_NAMES_MAX_COUNT )
		{
			return 257;
		}

		CName scopeName = m_scopeNames[ scopeId ];
		if ( scopeName == CName::NONE )
		{
			scopeName = CName( name );
			m_scopeNames[ scopeId ] = scopeName;
		}

		CName parentName = CName::NONE;
		if ( m_alreadyStartedCount > 0 )
		{
			parentName = m_alreadyStarted[ m_alreadyStartedCount - 1].m_name;
		}

		GpuScopeInfo entryKey( scopeName, parentName );

		if ( !m_entries.KeyExist( entryKey ) )
		{
			AddEntry( entryKey );
		}

		SRenderGpuHierarchyProfilerEntry& entry = m_entries.GetRef( entryKey );
		m_alreadyStarted[ m_alreadyStartedCount++ ] = entryKey;
		if ( !entry.m_finished )
		{			
			// assuming m_timestampQueryStartCount and end_count are always the same size
			if ( entry.m_frame == m_frame && entry.m_timestampQueryStartCount < NUM_QUERY_STAMPS_MAX )
			{
				if( entry.m_timestampQueryStartCount >= entry.m_timestampQueryStartCountMax )
				{
					entry.m_timestampQueryStart[ entry.m_timestampQueryStartCount ] = GpuApi::CreateQuery( GpuApi::QT_Timestamp );
					entry.m_timestampQueryEnd[ entry.m_timestampQueryEndCount ] = GpuApi::CreateQuery( GpuApi::QT_Timestamp );
				}				

				entry.m_timestampQueryStartCount++;
				entry.m_timestampQueryEndCount++;
			}
			else
			{
				return entry.m_id;
			}
		}

		GpuApi::EndQuery( entry.m_timestampQueryStart[ entry.m_timestampQueryStartCount-1 ] );
		entry.m_pending = true;
		entry.m_finished = false;
		return entry.m_id;
	}

	virtual void EndEntry( Uint32 id )
	{
		if ( m_alreadyStartedCount == 0 )
		{
			return;
		}
		m_alreadyStartedCount--;
		GpuScopeInfo temp = m_alreadyStarted[ m_alreadyStartedCount];
		SRenderGpuHierarchyProfilerEntry& entry = m_entries.GetRef( temp );
		if ( entry.m_id != id )
		{
			return;
		}

		if (!entry.m_pending)
		{
			return;
		}

		GpuApi::EndQuery( entry.m_timestampQueryEnd[ entry.m_timestampQueryEndCount-1 ] );
		entry.m_pending = false;
	}

	virtual Float GetLastTime( Uint32 id )//const GpuScopeAndParentNames& entryKey )
	{
		if ( id > m_keysCount )
		{
			return -1.f;
		}

		SRenderGpuHierarchyProfilerEntry& entry = m_entries.GetRef( m_keys[id] );
		if (entry.m_pending)
		{
			return entry.m_lastTime;
		}

		if (!entry.m_finished)
		{
			Float time = 0;
			Uint32 queriesFinished = 0;
			for ( Uint32 i = 0; i < entry.m_timestampQueryStartCount; ++i )
			{
				Uint64 startTimestamp;
				Uint64 endTimestamp;
				GpuApi::eQueryResult queryResultStart = GpuApi::GetQueryResult( entry.m_timestampQueryStart[i], startTimestamp, false );
				GpuApi::eQueryResult queryResultEnd = GpuApi::GetQueryResult( entry.m_timestampQueryEnd[i], endTimestamp, false );

				if ( queryResultStart == GpuApi::QR_Success && queryResultEnd == GpuApi::QR_Success )
				{
					time += Float( endTimestamp - startTimestamp );
					++queriesFinished;
				}
			}

			if ( queriesFinished == entry.m_timestampQueryStartCount )
			{
				entry.m_finished = true;
				entry.m_lastTime = time;
			}
		}

		return entry.m_lastTime;
	}
};

class CRenderGpuProfiler
{
private:
	CRenderGpuFastProfiler m_fastProfiler;
	CRenderGpuHierarchyProfiler m_hierarchyProfiler;

	IRenderGpuProfiler* m_currentProfiler;
	Bool m_frameStatsMode;
	Float m_frequency;

public:
	CRenderGpuProfiler()
		: m_frameStatsMode( false )
	{
		m_currentProfiler = (IRenderGpuProfiler*)&m_fastProfiler;
		m_frequency = GpuApi::GetDeviceUsageStats().m_GPUFrequency;
	}

	void UpdateFrequency( Float freq )
	{
		m_frequency = freq;
	}

	Uint32 AddEntry( const String& name )
	{
		return m_currentProfiler->AddEntry( name );
	}

	Uint32 StartEntry( const String& name, Uint32 id )
	{
		return m_currentProfiler->StartEntry( name, id );
	}

	void EndEntry( Uint32 id )
	{
		m_currentProfiler->EndEntry( id );
	}

	void ChangeMode( Bool frameStatsMode )
	{
		if ( frameStatsMode )
		{
			m_currentProfiler = (IRenderGpuProfiler*)&m_hierarchyProfiler;
		}
		else
		{
			m_currentProfiler = (IRenderGpuProfiler*)&m_fastProfiler;
			m_hierarchyProfiler.Reset();
		}
		m_frameStatsMode = frameStatsMode;
	}

	void GetEntriesTimes( TDynArray< SceneRenderingStats::GpuTimesStat >& result )
	{
		if ( m_frameStatsMode )
		{
			const GpuScopeInfo* scopes = m_hierarchyProfiler.GetScopesInfo();
			for ( Uint32 i = 0; i < m_hierarchyProfiler.GetEntryCount(); ++i )
			{
				result.PushBack( SceneRenderingStats::GpuTimesStat( m_hierarchyProfiler.GetLastTime( i ) / m_frequency, scopes[i].m_name.AsString(), scopes[i].m_parent.AsString() ) );
			}
		}
		else
		{
			const SRenderGpuFastProfilerEntry* entries = m_fastProfiler.GetEntries();
			for ( Uint32 i = 0; i < m_fastProfiler.GetEntryCount(); ++i )
			{
				result.PushBack( SceneRenderingStats::GpuTimesStat( m_fastProfiler.GetLastTime( i ) / m_frequency, entries[i].m_name ) );
			}
		}
	}

	void ClearHierarchyEntries()
	{
		m_hierarchyProfiler.ClearGatheredTimes();
	}
};

class CRenderGpuProfilerControl
{
private:
	CRenderGpuProfiler* m_gpuProfiler;
	String				m_name;
	Uint32				m_id;

public:
	CRenderGpuProfilerControl( CRenderGpuProfiler* gpuProfiler, const String& name, Uint32 id )
		: m_gpuProfiler( gpuProfiler )
		, m_name( name )
	{
		m_id = m_gpuProfiler->StartEntry( name, id );
	}

	~CRenderGpuProfilerControl()
	{
		m_gpuProfiler->EndEntry( m_id );
	}
};

#define PC_SCOPE_GPU(name) static Uint32 __profilerID_ = GetRenderer()->GetGpuProfiler()->AddEntry( TXT(#name) ); \
							CRenderGpuProfilerControl _gpuprofiler##__LINE__##scoped( GetRenderer()->GetGpuProfiler(), TXT(#name), __profilerID_ );
#else
#define PC_SCOPE_GPU(name)
#endif

class CHairWorksRefreshListener : public InGameConfig::IRefreshEventListener
{
public:
	CHairWorksRefreshListener();

	virtual void OnRequestRefresh(const CName& eventName);
	Bool GetAndClearRefresh();

private:
	Red::Threads::CAtomic<Bool> m_refreshRequest;

};

/// Renderer interface - GPU Api independent
class CRenderInterface : public IRender
{
public:
	//< Values duplicated in shader
	enum { 
		MAX_LIGHTS_PER_TILE					= 256,
		TILED_DEFERRED_DIMMERS_CAPACITY		= 192,
		TILED_DEFERRED_DIMMERS_FADE_TAIL	= 16,
		TILED_DEFERRED_LIGHTS_CAPACITY		= 256,
		MAX_DIMMERS_PER_TILE				= 192
	};

protected:
	TDynArray< CRenderViewport* >		m_viewports;
	CRenderStateManager*				m_stateManager;
	CRenderDrawer*						m_debugDrawer;
	CRenderPostProcess*					m_postProcess;
	CRenderMeshBatcher*					m_meshBatcher;
	CRenderPartricleBatcher*			m_particleBatcher;
	CRenderTerrainBatcher*				m_terrainBatcher;
	CRenderFlaresBatcher*				m_flaresBatcher;
	CRenderDistantLightBatcher*			m_distantLightBatcher;
	CRenderSwarmBatcher*				m_swarmBatcher;
#ifdef USE_APEX
	CRenderApexBatcher*					m_apexBatcher;
#endif
	CRenderDynamicDecalBatcher*			m_dynamicDecalBatcher;
#ifdef USE_NVIDIA_FUR
	GFSDK_HAIR_LogHandler*				m_hairSDKLogHandler;
	GFSDK_HairSDK*						m_hairSDK;
	SFrameTracker						m_hairSimulateFrameTracker;
	SCoherentFrameTracker				m_hairSimulatedLastFrame;
#endif // USE_NVIDIA_FUR
	Int32								m_surfacesHideGuard;
	CRenderSurfaces*					_m_surfaces; //< don't use this directly
	
	Float								m_lastTickDelta;
	Float								m_timeScale;

	Bool								m_viewportsOwnedByRenderThread;
	CRenderThread*						m_renderThread;
	CGameplayEffects*					m_gameplayFX;
	CRenderSkinManager*					m_skinManager;

	CCascadeShadowResources				m_cascadeShadowResources;
	
	//noise permutation texture for shadow filtering
	GpuApi::TextureRef					m_shadowmapNoiseTexture;

	//hires entity shadows shadowmap
	GpuApi::TextureRef					m_hiresEntityShadowmap;

	// data for screenshots (thumbnails for game saves)
	GpuApi::TextureRef					m_backBufferRescaled;

	//shadow manager for point and spotlights
	CRenderShadowManager*				m_shadowManager;
	
	CRenderEnvProbeManager*				m_envProbeManager;

private:
	struct SCameraLightPatch
	{
		Uint32 byteOffset;
		Vector color0;
		Vector color1;
	};
	GpuApi::BufferRef					m_computeConstantBuffer;
#ifndef RED_PLATFORM_ORBIS
	GpuApi::BufferRef					m_computeRawConstantBuffer;
#endif
	Red::UniqueBuffer					m_computeConstantShadowBuffer;
	TDynArray<SCameraLightPatch>		m_cameraLightPatches;
protected:
	GpuApi::BufferRef					m_computeTileLights;
	GpuApi::BufferRef					m_computeTileDimmers;

	GpuApi::BufferRef					m_sharedConstantBuffer;

	//texture preview system (debug page helper for shadows implementation)
	GpuApi::TextureRef					m_DynamicPreviewTexture;
	GpuApi::Uint32						m_DynamicPreviewTextureMip;
	GpuApi::Uint32						m_DynamicPreviewTextureSlice;
	GpuApi::Float						m_DynamicPreviewTextureColorMin;
	GpuApi::Float						m_DynamicPreviewTextureColorMax;

#ifdef USE_SCALEFORM
	CRenderScaleform*					m_renderScaleform;
#endif
	ERenderingDeviceState				m_currentDeviceState;
	TDynArray<CRenderSceneEx*>			m_existingRenderScenes;
	Double								m_lastTextureEvictionTime;
	Bool								m_dropOneFrame;
	IPlatformViewport*					m_platformViewport;
	Bool								m_renderCachets;

	CStandardRand						m_randomNumberGenerator;

	Bool								m_isAsyncMaterialCompilationEnabled;

	CRenderTextureStreamingManager*		m_textureStreamingManager;

	Float								m_lastGPUFrameTime;

	CRenderMeshStreaming				m_meshStreamingStats;

	// Track when rendering has been suppressed. When it's suppressed, we don't want to time-out textures, so we won't accidentally
	// unload something that will be needed as soon as rendering resumes. Since suppression is done on a viewport basis, we'll keep
	// track of how many times it's been suppressed (although it should generally be just once).
	Uint32								m_suppressRendering;

	CRenderCollector					m_collector;

	TQueue< CRenderFramePrefetch* >		m_pendingFramePrefetches;
	Red::Threads::CAtomic< Int32 >		m_numPendingPrefetches;

#ifdef TEXTURE_MEMORY_DEFRAG_ENABLED
	CRenderDefragHelper*				m_defragHelper;
#endif

#ifndef RED_FINAL_BUILD
	CRenderGpuProfiler*					m_gpuProfiler;
#endif

	CHairWorksRefreshListener			m_hairWorksRefreshListener;

public:

#define RENDER_SHADER_GEN(var,name,...) CRenderShaderPair* var
#define RENDER_SHADER_TESS_GEN(var,name,...) CRenderShaderQuadruple* var
#define RENDER_SHADER_COMPUTE(var,name,...) CRenderShaderCompute* var
#define RENDER_SHADER_GEOM_GEN(var, name,...) CRenderShaderTriple* var
#define RENDER_SHADER_SO_GEN(var, name, ...) CRenderShaderStreamOut* var

#ifndef NO_EDITOR
#define RENDER_SHADER_GEN_EDITOR RENDER_SHADER_GEN
#else
#define RENDER_SHADER_GEN_EDITOR(var,name,...)
#endif

#include "..\engine\renderShaders.h"

#undef RENDER_SHADER_GEN
#undef RENDER_SHADER_TESS_GEN
#undef RENDER_SHADER_COMPUTE
#undef RENDER_SHADER_GEOM_GEN
#undef RENDER_SHADER_SO_GEN
#undef RENDER_SHADER_GEN_EDITOR

public:
	//! Does the renderer support array textures 
	Bool SupportsArrayTextures() const;

	//! Does the renderer supports depth bound test
	Bool SupportsDepthBoundTest() const;

	//! Get state manager for renderer
	RED_FORCE_INLINE CRenderStateManager& GetStateManager() { return *m_stateManager; }

	//! Get debug drawer
	RED_FORCE_INLINE CRenderDrawer& GetDebugDrawer() { return *m_debugDrawer; }

	//! Get post process manager
	RED_FORCE_INLINE CRenderPostProcess* GetPostProcess() { return m_postProcess; }

	//! Get mesh rendering batcher
	RED_FORCE_INLINE CRenderMeshBatcher* GetMeshBatcher() { return m_meshBatcher; }
		
	//! Get distant light batcher
	RED_FORCE_INLINE CRenderDistantLightBatcher* GetDistantLightBacher() const { return m_distantLightBatcher; }

	//! Get particle rendering batcher
	RED_FORCE_INLINE CRenderPartricleBatcher* GetParticleBatcher() { return m_particleBatcher; }

	//! Get mesh terrain batcher
	RED_FORCE_INLINE CRenderTerrainBatcher* GetTerrainBatcher() { return m_terrainBatcher; }

	//! Get mesh terrain batcher
	RED_FORCE_INLINE CRenderFlaresBatcher* GetFlaresBatcher() { return m_flaresBatcher; }

	//! Get swarm rendering batcher
	RED_FORCE_INLINE CRenderSwarmBatcher* GetSwarmBatcher() { return m_swarmBatcher; }

#ifdef USE_APEX
	RED_FORCE_INLINE CRenderApexBatcher* GetApexBatcher() const { return m_apexBatcher; }
#endif

	RED_FORCE_INLINE CRenderDynamicDecalBatcher* GetDynamicDecalBatcher() const { return m_dynamicDecalBatcher; }

#ifdef USE_NVIDIA_FUR
	RED_FORCE_INLINE GFSDK_HairSDK* GetHairSDK() const { return m_hairSDK; }
#endif

	//! Get skin manager
	RED_FORCE_INLINE CRenderSkinManager* GetSkinManager() { return m_skinManager; }

	//! Get gameplay FX manager
	RED_FORCE_INLINE CGameplayEffects* GetGameplayFX() { return m_gameplayFX; }

	//! Get last tick delta
	RED_FORCE_INLINE Float GetLastTickDelta() const { return m_lastTickDelta; }
	
	//! Get "game" time scale
	RED_FORCE_INLINE Float GetTimeScale() const { return m_timeScale; }	

	//! Get render scenes list
	RED_FORCE_INLINE TDynArray<CRenderSceneEx*>& GetRenderScenesList() { return m_existingRenderScenes; }

	//! Get global cascades resources
	RED_FORCE_INLINE const CCascadeShadowResources& GetGlobalCascadesShadowResources() const { return m_cascadeShadowResources; }

	//! Get the general shadow manager
	RED_FORCE_INLINE CRenderShadowManager* GetShadowManager() const { return m_shadowManager; }

	//! Get the noise texture for shadowmap sample rotation
	RED_FORCE_INLINE GpuApi::TextureRef	GetShadowmapNoiseTexture() const { return m_shadowmapNoiseTexture; }

	//! Get the manager for the environment probes
	RED_FORCE_INLINE CRenderEnvProbeManager* GetEnvProbeManager() const { return m_envProbeManager; }

	//! Get the platform widget (specialized viewport)
	RED_FORCE_INLINE IPlatformViewport* GetPlatformViewport() { return m_platformViewport; }

	//! Rendering side random number generator
	RED_FORCE_INLINE CStandardRand& GetRandomNumberGenerator() { return m_randomNumberGenerator; }

	//! Get the texture stremaing manager
	RED_INLINE CRenderTextureStreamingManager* GetTextureStreamingManager() const { return m_textureStreamingManager; }

	//! Get the mesh streaming stats
	RED_INLINE CRenderMeshStreaming& GetMeshStreamingStats() { return m_meshStreamingStats; }

public:
	CRenderInterface( IPlatformViewport* platformViewport );
	virtual ~CRenderInterface();
	
public:
	// Initialize renderer
	virtual Bool Init();

	// Update internal state
	virtual void Tick( float timeDelta );

	// Is renderer ready, so we can move on and tick game?
	virtual Bool PrepareRenderer();

	//! Flush rendering thread
	virtual void Flush();

	virtual void ShutdownTextureStreaming() override;

	//! Get Surfaces (Render Targets)
	virtual CRenderSurfaces* GetSurfaces() const;

	//! Get the render thread
	virtual IRenderThread* GetRenderThread();

	//! Is MSAA enabled
	virtual Bool IsMSAAEnabled( const CRenderFrameInfo &frameInfo ) const;

	//! Get currently enabled msaa level (1 if disabled)
	virtual Uint32 GetEnabledMSAALevel( const CRenderFrameInfo &frameInfo ) const;

public:
	virtual ILoadingScreenFence* CreateLoadingScreenFence( const SLoadingScreenFenceInitParams& initParams );

public:
	// Reload all textures
	virtual void ReloadTextures();

	// Reload all engine shaders
	virtual void ReloadEngineShaders();

	// Reload all simple shaders
	virtual void ReloadSimpleShaders();

	// Recreate platform dependent resources
	virtual void RecreatePlatformResources();

	// Recreate resources related to shadow system
	virtual void RecreateShadowmapResources();

	// Recreate material resources
	virtual void RecalculateTextureStreamingSettings();

public:
	// Create rendering viewport
	virtual ViewportHandle CreateViewport( void* TopLevelWindow, void* ParentWindow, const String& title, Uint32 width, Uint32 height, EViewportWindowMode windowMode, Bool vsync = false ) override final;

	// Create game viewport, window will be owned by renderer
	virtual ViewportHandle CreateGameViewport( const String& title, Uint32 width, Uint32 height, EViewportWindowMode windowMode ) override final;

	//! Create gameplay render target for offscreen rendering
	virtual IRenderGameplayRenderTarget* CreateGameplayRenderTarget( const AnsiChar* tag );

	// Request a resize/recreate render surfaces (engine side)
	virtual void RequestResizeRenderSurfaces( Uint32 width, Uint32 height );

	// Resize/recreate render surfaces (renderer side)
	void ResizeRenderSurfaces( Uint32 width, Uint32 height );

public:
	// Create renderer resource for given texture
	virtual IRenderResource* UploadTexture( const CBitmapTexture* texture );

	// Create renderer resource for mesh
	virtual IRenderResource* UploadMesh( const CMesh* mesh );

	// Create renderer resource for fur mesh
	virtual IRenderResource* UploadFurMesh( const CFurMeshResource* fur ) override;

	// Create renderer resource for material
	virtual IRenderResource* UploadMaterial( const IMaterial* material );

	// Create renderer resource for cube texture
	virtual IRenderResource* UploadCube( const CCubeTexture* texture );

	// Create renderer resource for envprobe texture
	virtual IRenderResource* UploadEnvProbe( const CEnvProbeComponent* envProbeComponent );

	//! Create render exclusion list - controls global dependencies between object visibility in the rendering
	virtual IRenderVisibilityExclusion* CreateVisibilityExclusion( const GlobalVisID* ids, const Uint32 numIDs, const Uint8 renderMask, const Bool isEnabled ) override;

	// Create envprobe data source
	virtual IEnvProbeDataSource* CreateEnvProbeDataSource( CEnvProbeComponent &envProbeComponent );

	// Create renderer resource for texture array
	virtual IRenderResource* UploadTextureArray( const CTextureArray* textureArray );

	// Create renderer mesh for static debug data
	virtual IRenderResource* UploadDebugMesh( const TDynArray< DebugVertex >& vertices, const TDynArray< Uint32 >& indices );

#ifndef RED_FINAL_BUILD
	virtual GeneralStats GetGeneralMeshStats( GeneralStats& st );
	virtual GeneralStats GetGeneralTextureStats( GeneralStats& st );
	virtual void EnableGpuProfilerFrameStatsMode( Bool enable ) { if ( m_gpuProfiler ) m_gpuProfiler->ChangeMode( enable ); }
	RED_INLINE CRenderGpuProfiler* GetGpuProfiler() const { return m_gpuProfiler; }
#endif
	virtual const GpuApi::MeshStats* GetMeshStats();
	virtual const GpuApi::TextureStats* GetTextureStats();

#ifndef NO_DEBUG_WINDOWS
	// Returns -1 if null or not a texture
	virtual Int8 GetTextureStreamedMipIndex( const IRenderResource* resource );
#endif

public:
	//! Create X2 rendering scene
	virtual IRenderScene* CreateScene( Bool isWorldScene = false );

	//! Create X@ rendering proxy in scene
	virtual IRenderProxy* CreateProxy( const RenderProxyInitInfo& info );

	//! Create a dynamic decal object
	virtual IRenderResource* CreateDynamicDecal( const SDynamicDecalInitInfo& info );

#ifdef USE_SPEED_TREE

	//! Create speed tree proxy in scene
	virtual IRenderObject* CreateSpeedTreeProxy();

#endif

	//! Create terrain proxy in scene
	virtual IRenderProxy* CreateTerrainProxy( const IRenderObject* initData, const SClipmapParameters& clipmapParams );

	//! Create water proxy in scene
	virtual IRenderProxy* CreateWaterProxy( );

	//! Create X2 rendering frame
	virtual CRenderFrame* CreateFrame( CRenderFrame* masterFrame, const CRenderFrameInfo& info );

	//! Create skinning buffer
	virtual IRenderSkinningData* CreateSkinningBuffer( Uint32 numMatrices, Bool extendedBuffer );

	//! Create skinning buffer for the use outside of the rendering
	virtual IRenderSkinningData* CreateNonRenderSkinningBuffer( Uint32 numMatrices );

	//! Create data buffer for swarm data
	virtual IRenderSwarmData* CreateSwarmBuffer( Uint32 numBoids );

	//! Create fence
	virtual IRenderFence* CreateFence();

	//! Create render particle emitter
	virtual IRenderResource* CreateParticleEmitter( const CParticleEmitter* particleEmitter );

	//! Create data for terrain update
	virtual IRenderObject* CreateTerrainUpdateData( const TDynArray< SClipmapLevelUpdate* >& updates, IRenderObject* material, const STerrainTextureParameters* textureParameters, SClipmapStampDataUpdate* stampUpdate, Vector* colormapParams );
#ifndef NO_EDITOR
	virtual IRenderObject* CreateEditedTerrainUpdateData( const TDynArray< SClipmapLevelUpdate* >& updates, IRenderObject* material, const STerrainTextureParameters* textureParameters, SClipmapStampDataUpdate* stampData, Vector* colormapParams );
#endif

	//! Create data for generic grass settings update
	virtual IRenderObject* CreateGrassUpdateData( const TDynArray< struct SAutomaticGrassDesc >* automaticGrass );

	//! Create tree resource for speed tree rendering
	virtual IRenderObject* CreateSpeedTreeResource( const CSRTBaseTree* baseTree );

	// Get memory stats from speed tree
	virtual void PopulateSpeedTreeMetrics( SSpeedTreeResourceMetrics& metrics );

	virtual void GetSpeedTreeResourceCollision( IRenderObject* renderSpeedTreeResource, TDynArray< Sphere >& collision ) override;

	virtual void GetSpeedTreeTextureStatistic( IRenderObject* renderSpeedTreeResource, TDynArray< SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats >& statsArray ) override;

	virtual void ReleaseSpeedTreeTextures( IRenderObject* renderSpeedTreeResource ) override;

	//HACK DX10 no movie for now
	////! Create movie dumper, will ask for compression and such
	//virtual IRenderMovie* CreateMovie( Uint32 width, Uint32 height, const String& fileName );

	//! Create movie renderer
	virtual IRenderVideo* CreateVideo( CName videoClient, const SVideoParams& videoParams ) const;

#ifdef USE_UMBRA
	virtual IRenderObject* UploadOcclusionData( const CUmbraScene* umbraScene );
#endif

	//! Create entity grouping object
	virtual IRenderEntityGroup* CreateEntityGroup();
	
	//! Get the Scaleform rendering interface
	virtual IRenderScaleform* GetRenderScaleform() const;

#ifdef USE_APEX
	virtual physx::apex::NxUserRenderResourceManager* CreateApexRenderResourceManager();
#endif

	virtual IRenderTextureStreamRequest* CreateTextureStreamRequest( Bool lockTextures ) override;

	virtual IRenderFramePrefetch* CreateRenderFramePrefetch( CRenderFrame* frame, IRenderScene* scene, Bool useOcclusion = true ) override;


public:
	Bool InitDevice( Uint32 width, Uint32 height, Bool fullscreen, Bool vsync );
	void CloseDevice();
	void ReleaseResources();
	void ViewportClosed( CRenderViewport* viewport );

	void Suspend();
	void Resume();

	void InitShadowmapResources();
	void ReleaseShadowmapResources();
	void RenderFrame( CRenderFrame* frame, CRenderSceneEx* scene );
	void SetupSurfacesAvailabilityGuard( Bool isAvailable ); //< this function may be used recursively
	void SetupCachets( Bool enable ){ m_renderCachets = enable; }

	void NewFrame();
	void EndFrame();

	// Screenshots
	void TakeOneUberScreenshot( CRenderFrame* frame, CRenderSceneEx* scene, const SScreenshotParameters& screenshotParameters, Bool* status );
	void TakeOneRegularScreenshot( const SScreenshotParameters& screenshotParameters );
	void TakeOneRegularScreenshotNow( const SScreenshotParameters& screenshotParameters );

	static Uint32 GetTileDeferredConstantsDataSize();
	void ExportTiledDeferredConstants( void *outData ) const;
	void ImportTiledDeferredConstants( const void *data );
	void CalculateTiledDeferredConstants( CRenderCollector& collector, const CCascadeShadowResources &cascadeShadowResources, Bool flush = true );
	void CalculateTiledDeferredConstants_Various( const CRenderFrameInfo &info );
	void CalculateTiledDeferredConstants_CascadeShadows( const CRenderFrameInfo &info, const SMergedShadowCascades *mergedShadowCascades, const CCascadeShadowResources &cascadeShadowResources );
	void CalculateTiledDeferredConstants_TerrainShadows( const CRenderFrameInfo &info, const CRenderSceneEx *scene );
	void CalculateTiledDeferredConstants_Lights( const CRenderFrameInfo *cameraLightsInfo, const Vector &cameraPosition, const SWorldRenderSettings &worldRenderSettings, const CEnvColorGroupsParametersAtPoint &colorGroupsParams, Uint32 totalLights, const IRenderProxyLight * const *lights, Bool allowShadows, Bool usedDistanceLights = true, Float cameraInteriorFactor = 0 );
	void CalculateTiledDeferredConstants_Dimmers( const CRenderFrameInfo &info, Uint32 totalDimmers, const CRenderProxy_Dimmer * const *dimmers );
	void FlushTiledDeferredConstants( Bool allowPatch = true );
	Bool HasTiledDeferredConstantsPatch() const;
	GpuApi::BufferRef GetTileDeferredConstantsBuffer() const { return m_computeConstantBuffer; }
		
	void GrabLocalReflectionsData( CRenderCollector& collector );
	void ProcessLocalReflections( CRenderCollector& collector );
	void PreRenderNormalFrame( CRenderCollector& collector );
	void RenderNormalFrame( CRenderCollector& collector );
	void RenderRainEffect(  CRenderCollector& collector );
	void RenderHitProxyFrame( CRenderCollector& collector );
	void RenderShadows( const CRenderFrameInfo &info, const SMergedShadowCascades &cascades, const CCascadeShadowResources &cascadesResources );
	void BindForwardConsts( const CRenderFrameInfo& info, const CCascadeShadowResources &cascadeShadowResources, CRenderSurfaces *surfaces, Bool bindEnvProbesResources, GpuApi::eShaderType shaderTarget );
	void UnbindForwardConsts( const CRenderFrameInfo& info, GpuApi::eShaderType shaderTarget );	
	void RenderGlobalShadowMask( CRenderCollector& collector, const CCascadeShadowResources &cascadeShadowResources, const GpuApi::TextureRef &texRenderTarget, const GpuApi::TextureRef &texGBuffer1, const GpuApi::TextureRef &texGBuffer2, const GpuApi::TextureRef &texDepthBuffer );
	void RenderTiledDeferred( const CRenderFrameInfo& info, EPostProcessCategoryFlags postprocessFlags, ERenderTargetName rtnTarget );
	void RenderTiledDeferredEnvProbe( const CRenderFrameInfo& info, CRenderEnvProbe &envProbe, const GpuApi::TextureRef &texRenderTarget, const GpuApi::TextureRef &texGBuffer0, const GpuApi::TextureRef &texGBuffer1, const GpuApi::TextureRef &texShadowMask, const GpuApi::TextureRef &texDepthBuffer, Uint32 faceIndex );
	void CalculateSharedConstants( const CRenderFrameInfo &info, Uint32 fullRenderTargetWidth, Uint32 fullRenderTargetHeight, Int32 forcedEnvProbeIndex, Float forcedEnvProbeWeight );
	void BindSharedConstants( GpuApi::eShaderType shaderStage );
	void UnbindSharedConstants( GpuApi::eShaderType shaderStage );

	void RenderDeferredSetupGBuffer( const CRenderFrameInfo &info );
	// Render basic opaque static meshes into gbuffer. Assumes gbuffer is already bound
	void RenderDeferredFillGBuffer_StaticMeshes( CRenderCollector& collector );
	// Render non-static meshes and all the other stuff (terrain, stripes, etc). Assumes gbuffer is already bound
	void RenderDeferredFillGBuffer_NonStatics( CRenderCollector& collector );
	void RenderDeferredPostGBuffer( CRenderCollector& collector, Float effectStrength );

	void UpdateCameraInteriorFactor( CRenderCollector& collector );
	void UpdateLocalShadowMaps( CRenderCollector& collector );

	void SetupForSceneRender( const CRenderFrameInfo& info );

	void RenderOpaqueAfterGBuffer( CRenderCollector& collector );
	Bool DrawSSAO( const CRenderFrameInfo &info, ERenderTargetName rtnTexture, EPostProcessCategoryFlags postProcessFlags );
	void RenderThumbnailScreenshot( const CRenderFrameInfo& info );
	void RenderDebug( CRenderCollector& collector );
	void Render2D( CRenderCollector& collector, Bool supressSceneRendering );
	void RenderFinal2D( CRenderFrame* frame, CRenderFrameInfo& info , Bool supressSceneRendering, Bool dynamicRescalingAllowedByLoadingBlur );
	void RenderDebugOverlay( CRenderCollector& collector, CRenderFrame* frame, class CRenderSurfaces* surfaces, class CRenderViewport* viewport );
	void ClearGBuffer( const CRenderFrameInfo& info, bool forceClearAll );	
	void RenderVolumes( const CRenderFrameInfo& info, CRenderCollector& collector, GpuApi::TextureRef sampleTexture );
	void RenderWaterBlends( const CRenderFrameInfo& info, CRenderCollector& collector, GpuApi::TextureRef sampleTexture );

	Bool CanUseResourceCache() const;

	GpuApi::TextureRef GetHiResEntityShadowmap( CRenderCollector& collector ) const;

#ifdef USE_NVIDIA_FUR
	void UpdateFurSimulation( CRenderCollector& collector );
	void UpdateHairWorksPreset();
#endif

	//texture preview hacks
	void SetTexturePreview( GpuApi::TextureRef tex, Uint32 mipIndex/*=0*/, Uint32 sliceIndex/*=0*/, Float colorMin/*=0.0f*/, Float colorMax/*=1.0f*/ );
	void RenderTexturePreview();

public:
	// Are we currently prefetching envprobes
	virtual Bool IsDuringEnvProbesPrefetch() const override;

public:
	// Is renderer during fading to/from blackscreen
	virtual Bool IsFading();

	// Is black screen currently enabled
	virtual Bool IsBlackscreen();

	// Are we currently streaming in any textures?
	virtual Bool IsStreamingTextures() const override;

	// Get number of textures and the pending data size
	virtual void GetPendingStreamingTextures( Uint32& outNumTextures, Uint32& outDataSize ) const override;

	// Are we currently streaming any gui textures?
	virtual Bool IsStreamingGuiTextures() const override;

	// Are we currently streaming in any meshes ?
	virtual Bool IsStreamingMeshes() const  override;

	// Get number of mesh that are being streamed and the pending data size
	virtual void GetPendingStreamingMeshes( Uint32& outNumMeshes, Uint32& outDataSize ) const  override;

public:

	// Get current device state
	ERenderingDeviceState GetCurrentDeviceState() const { return m_currentDeviceState; }

	// Refresh current device state
	void RefreshDeviceState();

	// On device lost release rendering resources
	void ReleaseRenderingResources();

	// Reset device
	Bool ResetDevice();

	virtual Bool IsDeviceLost(){ return m_currentDeviceState != ERDS_Operational; }

	// Force fake device reset - HACK :( 
	virtual void ForceFakeDeviceReset();
	virtual void ForceFakeDeviceLost();
	virtual void ForceFakeDeviceUnlost();

public:
	virtual void TakeScreenshot( const SScreenshotParameters& screenshotParameters, Bool* finished, Bool& status );
	virtual void TakeQuickScreenshot( const SScreenshotParameters& screenshotParameters );
	virtual void TakeQuickScreenshotNow( const SScreenshotParameters& screenshotParameters );
	virtual void CastMaterialToTexture( Uint32 elem, Uint32 texWidth, Uint32 texHeight, IMaterial* material, Float distance );

	// Bind best fit normals texture
	void BindBestFitNormalsTexture( const CRenderFrameInfo &info );

	// Bind the sky shadow texture depending on env settings
	void BindGlobalSkyShadowTextures( const CRenderFrameInfo &info, GpuApi::eShaderType shaderTarget );

	// Sets the sync/async material compilation mode
	void SetAsyncCompilationMode( bool enable );

	// Gets the state of sync/async material compilation mode
	Bool GetAsyncCompilationMode() const;

private:
	// Init tiledDeferred constants related data
	void InitTiledDeferredConstants();

public:

	// Copy a rectangle from one texture into a rectangle in another texture. If the rectangles are different sizes, the content
	// will be stretched. If the texture formats are different, the content will be converted.
	// Depending on configuration may bind shaders, textures, or change compute constants or VSC_Custom_0. Render target
	// setup may be changed by this function as well.

	void StretchRect( const GpuApi::TextureRef& from, const Rect& fromArea, const GpuApi::TextureRef& to, const Rect& toArea );
	// Source and target rectangles fill the entire textures.
	void StretchRect( const GpuApi::TextureRef& from, const GpuApi::TextureRef& to );

	// Copy texture level to another texture level (resolution should match). Depending on platform this may bind shaders, textures or change compute constants or  VSC_Custom_0. Render target
	// setup may be changed by this function as well.
	void CopyTextureData( const GpuApi::TextureRef& destRef, Uint32 destMipLevel, Uint32 destArraySlice, const GpuApi::TextureRef& srcRef, Uint32 srcMipLevel, Uint32 srcArraySlice );
	void CopyTextureData( const GpuApi::TextureRef& destRef, Uint32 destMipLevel, Uint32 destArraySlice, GpuApi::Rect destRect, const GpuApi::TextureRef& srcRef, Uint32 srcMipLevel, Uint32 srcArraySlice, GpuApi::Rect srcRect );

	// Update texture data (does not require srcMemory to persists beyond this function call)
	void LoadTextureData2D(const GpuApi::TextureRef &destTex, Uint32 mipLevel, Uint32 arraySlice, const GpuApi::Rect* destRect, const void *srcMemory, Uint32 srcPitch);
	void LoadBufferData(const GpuApi::BufferRef &destBuffer, Uint32 offset, Uint32 size, const void *srcMemory);

	// Clear the given color/depth/stencil target. There are variations that clear a specific target, or that clear the current
	// render target setup.
	// Depending on configuration, may bind shaders, textures, or change VSC_Custom_0 or PSC_Custom_0.
	// The variations that clear specific targets may change the render target setup as well. The versions that clear the currently
	// bound targets will not affect the setup.

	void ClearColorTarget( const GpuApi::TextureRef& target, const Vector& value );
	void ClearColorTarget( const Vector& value );
	void ClearDepthTarget( const GpuApi::TextureRef& target, Float depthValue, Int32 slice = -1 );
	void ClearDepthTarget( Float depthValue );
	void ClearStencilTarget( const GpuApi::TextureRef& target, Uint8 stencilValue, Int32 slice = -1 );
	void ClearStencilTarget( Uint8 stencilValue );
	void ClearDepthStencilTarget( const GpuApi::TextureRef& target, Float depthValue, Uint8 stencilValue, Int32 slice = -1 );
	void ClearDepthStencilTarget( Float depthValue, Uint8 stencilValue );

	void ClearBufferUAV_Uint( const GpuApi::BufferRef& target, const Uint32 values[4] );

	void GenerateMipmaps( const GpuApi::TextureRef& texture );

	virtual Float GetLastGPUFrameDuration();


public:
#ifndef NO_ASYNCHRONOUS_MATERIALS
	static void TickRecompilingMaterials();
	virtual void FlushRecompilingMaterials() override;
#endif // NO_ASYNCHRONOUS_MATERIALS

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	virtual void ForceMaterialRecompilation( const IMaterial* material );
#endif //NO_RUNTIME_MATERIAL_COMPILATION


	// Add a frame prefetch to queue, to be processed during scene rendering. Must be called from render thread, not thread safe.
	void EnqueueFramePrefetch( IRenderFramePrefetch* prefetch );

	Bool HasPendingPrefetch() const { return m_numPendingPrefetches.GetValue() > 0; }

	void FlushOneFramePrefetch();

	CRenderFramePrefetch* GetNextPendingFramePrefetch() const;

private:
	// If there's a pending frame prefetch, get it started. This will run asynchronously, and should not be left running when there's
	// a possibility of the scene or occlusion data changing. Returns the prefetch that was started, or null if there wasn't one. The
	// returned CRenderFramePrefetch should be Released by the caller (probably by calling FinishFramePrefetch).
	CRenderFramePrefetch* StartNextFramePrefetch();
	// Make sure the frame prefetch has finished collecting proxies. It will continue running the prefetch process, but after this
	// point it's safe to change the scene, load new occlusion data, etc. This function will call Release on the provided prefetch.
	void FinishFramePrefetch( CRenderFramePrefetch* prefetch );

public:

	void ResetTextureStreamingDistances( CRenderSceneEx* scene );

	// Do any setup required for the first rendered frame after a loading screen.
	void SetupInitialFrame( CRenderFrame* frame, CRenderSceneEx* scene, Bool multipleAngles, Bool unloadExisting );

	// If scene rendering has been suppressed at least once, certain events will not happen (such as timing out render textures).
	// The number of times this is called with 'true' should generally match the times with 'false' in order to have normal updates.
	void SuppressSceneRendering( Bool suppress );

private:

#ifndef RED_FINAL_BUILD
		void UpdateRenderingStats( CRenderCollector& collector );
#endif

#if MICROSOFT_ATG_DYNAMIC_SCALING
protected:
	Vector	m_lastCameraPosition;
	Vector	m_lastCameraDirection;
#endif
};

RED_FORCE_INLINE CRenderInterface* GetRenderer()
{
	return static_cast< CRenderInterface* >( GRender );
}

///////////////////////////////////////////////

#ifdef USE_SPECIAL_PIX_MODE

#define PC_SCOPE_RENDER( name, level ) PC_SCOPE_PIX( name ) \
										ProfilerAnnotation _renderprofile##__LINE__##scoped( TXT(#name) );
#define PC_SCOPE_RENDER_LVL0( name ) PC_SCOPE_RENDER( name, 0 )
#define PC_SCOPE_RENDER_LVL1( name ) PC_SCOPE_RENDER( name, 1 )
#define PC_SCOPE_RENDER_LVL2( name ) PC_SCOPE_RENDER( name, 2 )

#else

#ifdef USE_PROFILER
  #define PC_SCOPE_RENDER( name, level ) PC_SCOPE_LVL( name, level, PBC_RENDER ) \
	ProfilerAnnotation _renderprofile##__LINE__##scoped( TXT(#name) );
#else
  #define PC_SCOPE_RENDER( name, level )
#endif
#define PC_SCOPE_RENDER_LVL0( name ) PC_SCOPE_RENDER( name, 0 ); \
										PC_SCOPE_GPU(name);
#define PC_SCOPE_RENDER_LVL1( name ) PC_SCOPE_RENDER( name, 1 )
#define PC_SCOPE_RENDER_LVL2( name ) PC_SCOPE_RENDER( name, 2 )

#endif

class ProfilerAnnotation
{
#ifdef _DEBUG
	Char* debugName;
#endif
public:
	ProfilerAnnotation( const Char* name )
	{
#ifdef _DEBUG
		debugName = const_cast<Char*>(name);
#endif
		GpuApi::BeginProfilerBlock( name );
	}

	~ProfilerAnnotation()
	{
		GpuApi::EndProfilerBlock();
	}
};
///////////////////////////////////////////////
