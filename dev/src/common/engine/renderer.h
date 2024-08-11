/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "viewportWindowMode.h"
#include "viewport.h"

class CRenderFrame;
class CRenderFrameInfo;
class CBitmapTexture;
class CLockRect;
class CMesh;
class CFurMeshResource;
class IViewport;
class IRenderResource;
class CMaterialGraph;
class IMaterial;
class CCubeTexture;
struct CMaterialUpdateContext;
struct DebugVertex;
class GRenderer;
class IRenderObject;
class IRenderScene;
class IRenderThread;
class IRenderProxy;
class IRenderProxy_Foliage;
class IRenderSkinningData;
class IRenderSwarmData;
class IRenderFence;
class IRenderGameplayRenderTarget;
class CParticleEmitter;
class CRenderParticleEmitter;
class CFoliageCookedData;
class CFoliageInstancesData;
class IRenderMovie;
class IRenderVideo;
class IRenderEntityGroup;
class IRenderVisibilityExclusion;
class RenderProxyInitInfo;
struct SClipmapLevelUpdate;
struct SClipmapStampDataUpdate;
struct SClipmapParameters;
struct STerrainTextureParameters;
struct SScreenshotParameters;
enum ETextureRawFormat : CEnum::TValueType;
enum ETextureCompression : CEnum::TValueType;
#ifdef USE_UMBRA
class CUmbraScene;
#endif
struct SVideoParams;
struct SDynamicDecalInitInfo;
class CTextureArray;
class CSRTBaseTree;
class IRenderScaleform;
class CEnvProbeComponent;
class IEnvProbeDataSource;
class ILoadingScreenFence;
struct SLoadingScreenFenceInitParams;
class IRenderTextureStreamRequest;
class IRenderFramePrefetch;

namespace GpuApi
{
	struct TextureStats;
}

namespace Scaleform { namespace Render {
	class ThreadCommandQueue;
}}

namespace physx { namespace apex {
	class NxUserRenderResourceManager;
}}

struct SSpeedTreeResourceMetrics
{
	enum ResourceType
	{
		RESOURCE_VB,
		RESOURCE_IB,
		RESOURCE_VERTEX_SHADER,
		RESOURCE_PIXEL_SHADER,
		RESOURCE_TEXTURE,
		RESOURCE_OTHER,
		RESOURCE_COUNT
	};

	struct HeapStats
	{
		MemSize m_currentBytes;
		MemSize m_peakBytes;
		MemSize m_currentAllocs;
		MemSize m_peakAllocs;
	};

	struct RenderStats
	{
		Uint64 m_grassLayerCount;
		Uint64 m_visibleGrassCellCount;
		Uint64 m_visibleGrassCellArrayCapacity;
		Uint64 m_visibleGrassCellArraySize;
		Uint64 m_visibleGrassInstanceCount;
		Uint64 m_visibleGrassInstanceArrayCapacity;
		Uint64 m_visibleGrassInstanceArraySize;
		Uint64 m_visibleTreeCellCount;
		Uint64 m_visibleTreeCellArrayCapacity;
		Uint64 m_visibleTreeCellArraySize;
		Uint64 m_visibleTreeInstanceCount;
		Uint64 m_visibleTreeInstanceArrayCapacity;
		Uint64 m_visibleTreeInstanceArraySize;
		Float m_maximumGrassLayerCullDistance;
		Float m_minGrassCellSize;
		Float m_maxGrassCellSize;
		Uint64 m_treesRendered;
		Uint64 m_billboardsRendered;
		Uint64 m_grassRendered;
		Uint64 m_treeDrawcalls;
		Uint64 m_billboardDrawcalls;
		Uint64 m_grassDrawcalls;
	};

	struct SGeneralSpeedTreeStats
	{
		const GpuApi::TextureDesc&	m_textureDesc;
		String						m_fileName;

		SGeneralSpeedTreeStats( const GpuApi::TextureDesc&	textureDesc )
			: m_textureDesc( textureDesc )
		{
			/* intentionally empty */
		}

	private:
		SGeneralSpeedTreeStats& operator=( const SGeneralSpeedTreeStats& );
	};

	HeapStats m_heapStats;
	HeapStats m_resourceStats[ RESOURCE_COUNT ];
	RenderStats m_renderStats;
};

struct GeneralStats
{
	Uint32 m_characterMemory;
	Uint32 m_environmentMemory;

	GeneralStats()
		: m_characterMemory( 0 )
		, m_environmentMemory( 0 )
	{}

	GeneralStats( Uint32 charMem, Uint32 envMem )
		: m_characterMemory( charMem )
		, m_environmentMemory( envMem )
	{}

	void ResetStats()
	{
		m_characterMemory = 0;
		m_environmentMemory = 0;
	}
};

/// Base rendering interface
class IRender
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_Default, MC_RenderData, 16 );

public:
	virtual ~IRender() {};

public:
	// Initialize renderer
	virtual Bool Init()=0;

	// Update internal state
	virtual void Tick( float timeDelta )=0;

	// Is renderer ready, so we can move on and tick game?
	virtual Bool PrepareRenderer()=0;
	
	//! Flush rendering thread
	virtual void Flush()=0;

	//! Cancel any current texture streaming, and prevent further texture stream updates from running. Blocks until shutdown is complete.
	virtual void ShutdownTextureStreaming()=0;

public:
	// Reload all textures ( slow )
	virtual void ReloadTextures()=0;

	// Reload engine shaders ( material graphs )
	virtual void ReloadEngineShaders()=0;

	// Reload simple shaders
	virtual void ReloadSimpleShaders()=0;

	// Recreate platform dependent resources
	virtual void RecreatePlatformResources()=0;

	// Recreate resources related to shadow system
	virtual void RecreateShadowmapResources()=0;

	// Recreate material resources
	virtual void RecalculateTextureStreamingSettings()=0;

public:
	// Interface
	virtual ViewportHandle CreateViewport( void* TopLevelWindow, void* ParentWindow, const String& title, Uint32 width, Uint32 height, EViewportWindowMode windowMode, Bool vsync )=0;

	// Create game viewport, window will be owned by renderer
	virtual ViewportHandle CreateGameViewport( const String& title, Uint32 width, Uint32 height, EViewportWindowMode windowMode )=0;

	// Request a resize/recreate of render surfaces
	virtual void RequestResizeRenderSurfaces( Uint32 width, Uint32 height )=0;

	// Get the rendering thread
	virtual IRenderThread* GetRenderThread()=0;

	//! Is MSAA enabled
	virtual Bool IsMSAAEnabled( const CRenderFrameInfo &frameInfo ) const=0;

	//! Get currently enabled msaa level (1 if disabled)
	virtual Uint32 GetEnabledMSAALevel( const CRenderFrameInfo &frameInfo ) const=0;

public:
	virtual ILoadingScreenFence* CreateLoadingScreenFence( const SLoadingScreenFenceInitParams& initParams )=0;

public:
	// Create renderer resource for given texture
	virtual IRenderResource* UploadTexture( const CBitmapTexture* texture )=0;

	// Create renderer resource for mesh
	virtual IRenderResource* UploadMesh( const CMesh* mesh )=0;

	// Create renderer resource for fur mesh
	virtual IRenderResource* UploadFurMesh( const CFurMeshResource* fur )=0;

	// Create renderer resource for material
	virtual IRenderResource* UploadMaterial( const IMaterial* material )=0;

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	// Force recompilation of material
	virtual void ForceMaterialRecompilation( const IMaterial* material )=0;
#endif

#ifndef NO_ASYNCHRONOUS_MATERIALS
	virtual void FlushRecompilingMaterials()=0;
#endif // NO_ASYNCHRONOUS_MATERIALS

	// Create renderer resource for cube texture
	virtual IRenderResource* UploadCube( const CCubeTexture* texture )=0;

	// Create renderer resource for envprobe texture
	virtual IRenderResource* UploadEnvProbe( const CEnvProbeComponent* envProbeComponent )=0;

	// Create envprobe data source
	virtual IEnvProbeDataSource* CreateEnvProbeDataSource( CEnvProbeComponent &envProbeComponent )=0;

	// Create renderer resource for texture array
	virtual IRenderResource* UploadTextureArray( const CTextureArray* textureArray )=0;

	// Create renderer mesh for static debug data
	virtual IRenderResource* UploadDebugMesh( const TDynArray< DebugVertex >& vertices, const TDynArray< Uint32 >& indices )=0;

#ifndef RED_FINAL_BUILD
	virtual GeneralStats GetGeneralMeshStats( GeneralStats& st ) = 0;
	virtual GeneralStats GetGeneralTextureStats( GeneralStats& st ) = 0;
	virtual void EnableGpuProfilerFrameStatsMode( Bool enable )=0;
#endif
	virtual const GpuApi::MeshStats* GetMeshStats() = 0;
	virtual const GpuApi::TextureStats* GetTextureStats() = 0;

#ifndef NO_DEBUG_WINDOWS
	// Get renderBaseTexture's streamed mipmap index
	virtual Int8 GetTextureStreamedMipIndex( const IRenderResource* resource )=0;
#endif

public:
	//! Create X2 rendering scene
	virtual IRenderScene* CreateScene( Bool isWorldScene = false )=0;

	//! Create X@ rendering proxy in scene
	virtual IRenderProxy* CreateProxy( const RenderProxyInitInfo& info )=0;

	//! Create dynamic decal object
	virtual IRenderResource* CreateDynamicDecal( const SDynamicDecalInitInfo& info )=0;

#ifdef USE_SPEED_TREE

	//! Create speed tree proxy in scene
	virtual IRenderObject* CreateSpeedTreeProxy()=0;

#endif

	//! Create terrain proxy in scene
	virtual IRenderProxy* CreateTerrainProxy( const IRenderObject* initData, const SClipmapParameters& clipmapParams )=0;

	//! Create water proxy in scene
	virtual IRenderProxy* CreateWaterProxy( )=0;// const IRenderObject* initData )=0;

	//! Create X2 rendering frame
	virtual CRenderFrame* CreateFrame( CRenderFrame* masterFrame, const CRenderFrameInfo& info )=0;

	//! Create skinning buffer
	virtual IRenderSkinningData* CreateSkinningBuffer( Uint32 numMatrices, Bool extendedBuffer )=0;
	//! Create a skinning buffer that should not be used for direct rendering. Writes into a provided buffer.
	virtual IRenderSkinningData* CreateNonRenderSkinningBuffer( Uint32 numMatrices )=0;

	//! Create data buffer for swarm data
	virtual IRenderSwarmData* CreateSwarmBuffer( Uint32 numBoids ) = 0;

	//! Create fence in the command buffer
	virtual IRenderFence* CreateFence()=0;

	//! Create render particle emitter
	virtual IRenderResource* CreateParticleEmitter( const CParticleEmitter* particleEmitter )=0;

	//! Create tree resource for speed tree rendering
	virtual IRenderObject* CreateSpeedTreeResource( const CSRTBaseTree* baseTree ) = 0;

	//! Create gameplay render target for offscreen rendering
	virtual IRenderGameplayRenderTarget* CreateGameplayRenderTarget( const AnsiChar* tag ) = 0;

	//! Create render exclusion list - controls global dependencies between object visibility in the rendering
	//! When this group is enabled objects with given IDs will be filtered out from rendering using given RenderMask
	virtual IRenderVisibilityExclusion* CreateVisibilityExclusion( const class GlobalVisID* ids, const Uint32 numIDs, const Uint8 renderMask, const Bool isEnabled ) = 0;

	//! Create a container for passing instances between engine and renderer side
	virtual void GetSpeedTreeResourceCollision( IRenderObject* renderSpeedTreeResource, TDynArray< Sphere >& collision )=0;
	virtual void PopulateSpeedTreeMetrics( SSpeedTreeResourceMetrics& metrics ) = 0;
	virtual void GetSpeedTreeTextureStatistic( IRenderObject* renderSpeedTreeResource, TDynArray< SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats >& statsArray ) = 0;
	virtual void ReleaseSpeedTreeTextures( IRenderObject* renderSpeedTreeResource ) = 0;

	//! Create data for terrain update
	virtual IRenderObject* CreateTerrainUpdateData( const TDynArray< SClipmapLevelUpdate* >& updates, IRenderObject* material, const STerrainTextureParameters* textureParameters, SClipmapStampDataUpdate* stampData, Vector* colormapParams )=0;
#ifndef NO_EDITOR
	virtual IRenderObject* CreateEditedTerrainUpdateData( const TDynArray< SClipmapLevelUpdate* >& updates, IRenderObject* material, const STerrainTextureParameters* textureParameters, SClipmapStampDataUpdate* stampData, Vector* colormapParams )=0;
#endif

	//! Create data for generic grass settings update
	virtual IRenderObject* CreateGrassUpdateData( const TDynArray< struct SAutomaticGrassDesc >* automaticGrass ) = 0;
	
	// HACK DX10 no movie dump for now
	////! Create movie dumper
	//virtual IRenderMovie* CreateMovie( Uint32 width, Uint32 height, const String& fileName )=0;

	//! Create movie renderer
	virtual IRenderVideo* CreateVideo( CName videoClient, const SVideoParams& videoParams ) const=0;

#ifdef USE_UMBRA
	//! Create occlusion data
	virtual IRenderObject* UploadOcclusionData( const CUmbraScene* umbraScene )=0;
#endif // USE_UMBRA

	// Create entity group
	virtual IRenderEntityGroup* CreateEntityGroup()=0;
		
	virtual IRenderScaleform* GetRenderScaleform() const =0;

#ifdef USE_APEX
	virtual physx::apex::NxUserRenderResourceManager* CreateApexRenderResourceManager() = 0;
#endif

	virtual IRenderTextureStreamRequest* CreateTextureStreamRequest( Bool lockTextures ) = 0;

	virtual IRenderFramePrefetch* CreateRenderFramePrefetch( CRenderFrame* frame, IRenderScene* scene, Bool useOcclusion = true ) = 0;

public:
	//! Get fallback shader
	CMaterialGraph* GetFallbackShader();

	// Fade and blackscreen state info methods
public:
	// Is renderer during fading to/from blackscreen
	virtual Bool IsFading()=0;

	// Is blackscreen currently enabled
	virtual Bool IsBlackscreen()=0;

	// Are we currently streaming in any textures?
	virtual Bool IsStreamingTextures() const = 0;

	// Get number of textures and the pending data size
	virtual void GetPendingStreamingTextures( Uint32& outNumTextures, Uint32& outDataSize ) const = 0;

	// Are we currently streaming any gui textures?
	virtual Bool IsStreamingGuiTextures() const = 0;

	// Are we currently streaming in any meshes ?
	virtual Bool IsStreamingMeshes() const = 0;

	// Get number of mesh that are being streamed and the pending data size
	virtual void GetPendingStreamingMeshes( Uint32& outNumMeshes, Uint32& outDataSize ) const = 0;

	// Are we currently prefetching envprobes
	virtual Bool IsDuringEnvProbesPrefetch() const = 0;

	// Is device lost? If yes, do not perform any attaches and stuff
	virtual Bool IsDeviceLost()=0;

	// Force fake device reset - HACK :( 
	virtual void ForceFakeDeviceReset();
	virtual void ForceFakeDeviceLost();
	virtual void ForceFakeDeviceUnlost();

public:
	virtual void TakeScreenshot( const SScreenshotParameters& screenshotParameters, Bool* finished, Bool& status ) = 0;
	virtual void TakeQuickScreenshot( const SScreenshotParameters& screenshotParameters ) = 0;
	virtual void TakeQuickScreenshotNow( const SScreenshotParameters& screenshotParameters ) = 0;
	virtual void CastMaterialToTexture( Uint32 elements, Uint32 texWidth, Uint32 texHeight, IMaterial* material, Float distance ) = 0;

	virtual void SetAsyncCompilationMode( bool enable ) = 0;
	virtual Bool GetAsyncCompilationMode() const = 0;

	virtual Float GetLastGPUFrameDuration() = 0;
};

/// Render interface
extern IRender* GRender;

