/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../engine/renderVisibilityQuery.h"
#include "../engine/renderer.h"
#include "../engine/renderFrame.h"

class CRenderSurfaces;
struct SVideoParams;

/// NULL renderer
class CNullRender : public IRender
{
public:
	CNullRender();
	
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

	// Get the render thread
	virtual IRenderThread* GetRenderThread();

	//! Is MSAA enabled
	virtual Bool IsMSAAEnabled( const CRenderFrameInfo &frameInfo ) const;

	//! Get currently enabled msaa level (1 if disabled)
	virtual Uint32 GetEnabledMSAALevel( const CRenderFrameInfo &frameInfo ) const;

	//! Create loading screen fence
	virtual ILoadingScreenFence* CreateLoadingScreenFence( const SLoadingScreenFenceInitParams& initParams );

	// Reload all textures ( slow )
	virtual void ReloadTextures();

	// Reload engine shaders ( material graphs )
	virtual void ReloadEngineShaders();

	// Reload simple shaders
	virtual void ReloadSimpleShaders();

	// Recreate platform dependent resources
	virtual void RecreatePlatformResources();

	//dex++: Recreate resources related to shadow system
	virtual void RecreateShadowmapResources();
	//dex--

	virtual void RecalculateTextureStreamingSettings();

public:
	virtual IRenderVideo* CreateVideo( CName videoClient, const SVideoParams& videoParams ) const;

	// Create rendering viewport
	virtual ViewportHandle CreateViewport( void* TopLevelWindow, void* ParentWindow, const String& title, Uint32 width, Uint32 height, EViewportWindowMode windowMode, Bool vsync ) override final;

	// Create game viewport, window will be owned by renderer
	virtual ViewportHandle CreateGameViewport( const String& title, Uint32 width, Uint32 height, EViewportWindowMode windowMode ) override final;

	//! Create render exclusion list - controls global dependencies between object visibility in the rendering
	//! When this group is enabled objects with given IDs will be filtered out from rendering using given RenderMask
	virtual IRenderVisibilityExclusion* CreateVisibilityExclusion( const class GlobalVisID* ids, const Uint32 numIDs, const Uint8 renderMask, const Bool isEnabled ) override;

	//! Create gameplay render target for offscreen rendering
	virtual IRenderGameplayRenderTarget* CreateGameplayRenderTarget( const AnsiChar* tag );

	// Request a resize/recreate of render surfaces
	virtual void RequestResizeRenderSurfaces( Uint32 width, Uint32 height );

	// Render compiled rendering frame
	virtual void RenderFrame( CRenderFrame* frame );

	// Render frame fragments of given sort group
	virtual void RenderFragments( CRenderFrame* frame, ERenderingSortGroup sortGroup, const class RenderingContext& context );

	// Render frame fragments of given sort group inside provided range
	virtual void RenderFragments( CRenderFrame* frame, ERenderingSortGroup sortGroup, const class RenderingContext& context, Uint32 firstFragIndex, Uint32 numFragments );

	// Render frame fragments of given sort groups
	virtual void RenderFragments( CRenderFrame* frame, Int32 sortGroupsCount, const ERenderingSortGroup *sortGroups, const class RenderingContext& context );	

public:
	// Create renderer resource for given texture
	virtual IRenderResource* UploadTexture( const CBitmapTexture* texture );

	// Create renderer resource for mesh
	virtual IRenderResource* UploadMesh( const CMesh* mesh );

	// Create renderer resource for fur mesh
	virtual IRenderResource* UploadFurMesh( const CFurMeshResource* fur ) override;

	// Create renderer resource for material
	virtual IRenderResource* UploadMaterial( const IMaterial* material );

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	// Force recompilation of material
	virtual void ForceMaterialRecompilation( const IMaterial* material );
#endif

#ifndef NO_ASYNCHRONOUS_MATERIALS
	virtual void FlushRecompilingMaterials();
#endif // NO_ASYNCHRONOUS_MATERIALS

	// Create renderer resource for cube texture
	virtual IRenderResource* UploadCube( const CCubeTexture* texture );

	// Create renderer resource for cube texture
	virtual IRenderResource* UploadEnvProbe( const CEnvProbeComponent* envProbeComponent );

	// Create envprobe data source
	virtual IEnvProbeDataSource* CreateEnvProbeDataSource( CEnvProbeComponent &envProbeComponent );

	// Create renderer resource for texture array
	virtual IRenderResource* UploadTextureArray( const CTextureArray* texture );

	// Create renderer mesh for static debug data
	virtual IRenderResource* UploadDebugMesh( const TDynArray< DebugVertex >& vertices, const TDynArray< Uint32 >& indices );

#ifndef RED_FINAL_BUILD
	virtual GeneralStats GetGeneralMeshStats( GeneralStats& st );
	virtual GeneralStats GetGeneralTextureStats( GeneralStats& st );
	virtual void EnableGpuProfilerFrameStatsMode( Bool enable ) {}
#endif
	virtual const GpuApi::MeshStats* GetMeshStats();
	virtual const GpuApi::TextureStats* GetTextureStats();

#ifndef NO_DEBUG_WINDOWS
	virtual Int8 GetTextureStreamedMipIndex( const IRenderResource* resource );
#endif

public:
	//! Create X2 rendering scene
	virtual IRenderScene* CreateScene( Bool /*isWorldScene = false*/ ) { return nullptr; };

	//! Create X@ rendering proxy in scene
	virtual IRenderProxy* CreateProxy( const RenderProxyInitInfo& /*info*/ )  { return nullptr; };

	virtual IRenderResource* CreateDynamicDecal( const SDynamicDecalInitInfo& /*info*/ ) { return nullptr; }

	//! Create foliage proxy in scene
	virtual IRenderProxy_Foliage* CreateFoliageProxy( const RenderProxyInitInfo& /*info */ ) { return nullptr; };

	//! Create speed tree proxy in scene
	virtual IRenderObject* CreateSpeedTreeProxy() { return nullptr; };

	//! Create terrain proxy in scene
	virtual IRenderProxy* CreateTerrainProxy( const IRenderObject* /*initData*/, const SClipmapParameters& /*clipmapParams*/ ) { return nullptr; };

	//! Create water proxy in scene
	virtual IRenderProxy* CreateWaterProxy( ) { return nullptr; }; //const IRenderObject* initData

	//! Create X2 rendering frame
	virtual CRenderFrame* CreateFrame( CRenderFrame* /*masterFrame*/, const CRenderFrameInfo& /*info*/ )  { return nullptr; };

	//! Create skinning buffer
	virtual IRenderSkinningData* CreateSkinningBuffer( Uint32 /*numMatrices*/, Bool /*extendedBuffer*/ )  { return nullptr; };
	virtual IRenderSkinningData* CreateNonRenderSkinningBuffer( Uint32 /*numMatrices*/ ) { return nullptr; }

	//! Create swarm data buffer
	virtual IRenderSwarmData* CreateSwarmBuffer( Uint32 numBoids )	{ return nullptr; }

	//! Create fence
	virtual IRenderFence* CreateFence()  { return nullptr; };

	//! Create render particle emitter
	virtual IRenderResource* CreateParticleEmitter( const CParticleEmitter* /*particleEmitter*/ ) { return nullptr; }

	//! Create data for foliage
	virtual IRenderObject* CreateFoliageData( CFoliageCookedData* /*data*/) { return nullptr; };

	//! Create tree resource for speed tree rendering
	virtual IRenderObject* CreateSpeedTreeResource( const CSRTBaseTree* /*baseTree*/ ) { return nullptr; };

	virtual void GetSpeedTreeResourceCollision( IRenderObject* /*renderSpeedTreeResource*/, TDynArray< Sphere >& /*collision*/ ) override {}

	virtual void GetSpeedTreeTextureStatistic( IRenderObject* /*renderSpeedTreeResource*/, TDynArray< SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats >& /*statsArray*/ ) override {}

	virtual void ReleaseSpeedTreeTextures( IRenderObject* renderSpeedTreeResource ) override {}

	//! Create data for terrain update
	virtual IRenderObject* CreateTerrainUpdateData( const TDynArray< SClipmapLevelUpdate* >& /*updates*/, IRenderObject* /*diffuseTextureArray*/, const STerrainTextureParameters* /*textureParameters*/, SClipmapStampDataUpdate* /*stampUpdate*/, Vector* /*colormapParams*/ ) { return nullptr; }
#ifndef NO_EDITOR
	virtual IRenderObject* CreateEditedTerrainUpdateData( const TDynArray< SClipmapLevelUpdate* >& updates, IRenderObject* material, const STerrainTextureParameters* textureParameters, SClipmapStampDataUpdate* stampData, Vector* colormapParams ) { return nullptr; }
#endif

	//! Create data for generic grass settings update
	virtual IRenderObject* CreateGrassUpdateData( const TDynArray< struct SAutomaticGrassDesc >* /*automaticGrass*/ ) { return nullptr; }

	//! Create movie dumper
	virtual IRenderMovie* CreateMovie( Uint32 /*width*/, Uint32 /*height*/, const String& /*fileName*/ ) { return nullptr; };

	//! Create movie renderer
	virtual IRenderVideo* CreateVideo( const SVideoParams& /*videoParams*/ ) const { return nullptr; }

#ifdef USE_UMBRA
	virtual IRenderObject* UploadOcclusionData( const CUmbraScene* /*umbraScene*/ ) { return nullptr; }
#endif
	
	// Create entity group
	virtual IRenderEntityGroup* CreateEntityGroup() { return nullptr; };

	virtual IRenderScaleform* GetRenderScaleform() const { return nullptr; }

	virtual physx::apex::NxUserRenderResourceManager* CreateApexRenderResourceManager() { return nullptr; }

	virtual IRenderTextureStreamRequest* CreateTextureStreamRequest( Bool /*lockTextures*/ ) { return nullptr; }

	virtual IRenderFramePrefetch* CreateRenderFramePrefetch( CRenderFrame* /*frame*/, IRenderScene* /*scene*/, Bool /*useOcclusion*/ ) override { return nullptr; }

	virtual void PopulateSpeedTreeMetrics( SSpeedTreeResourceMetrics& /*metrics*/ ) {}

	// Fade and blackscreen state info methods
public:
	// Is renderer during fading to/from blackscreen
	virtual Bool IsFading() { return false; }

	// Is blackscreen currently enabled
	virtual Bool IsBlackscreen() { return false; }

	// Are we currently streaming in any textures?
	virtual Bool IsStreamingTextures() const { return false; }

	// Get number of textures and the pending data size
	virtual void GetPendingStreamingTextures( Uint32& outNumTextures, Uint32& outDataSize ) const { outNumTextures = 0; outDataSize = 0; }

	// Are we currently streaming any gui textures?
	virtual Bool IsStreamingGuiTextures() const override { return false; }

	// Are we currently streaming in any meshes ?
	virtual Bool IsStreamingMeshes() const { return false; }

	// Get number of mesh that are being streamed and the pending data size
	virtual void GetPendingStreamingMeshes( Uint32& outNumMeshes, Uint32& outDataSize ) const { outNumMeshes = 0; outDataSize = 0; }
	
	// Are we currently prefetching envprobes
	virtual Bool IsDuringEnvProbesPrefetch() const { return false; }

	// Is device lost? If yes, do not perform any attaches and stuff
	virtual Bool IsDeviceLost(){ return true; }

public:
	virtual void TakeScreenshot( const SScreenshotParameters& /*screenshotParameters*/, Bool* /*finished*/, Bool& /*status*/ ) {}
	virtual void TakeQuickScreenshot( const SScreenshotParameters& /*screenshotParameters*/ ) {}
	virtual void TakeQuickScreenshotNow( const SScreenshotParameters& /*screenshotParameters*/ ){}
	virtual void CastMaterialToTexture( Uint32, Uint32, Uint32, IMaterial*, Float ) { };

	virtual void SetAsyncCompilationMode( bool /*enable*/ ) {}
	virtual Bool GetAsyncCompilationMode() const { return false; }

	virtual Float GetLastGPUFrameDuration() {return 0.f;}
};
