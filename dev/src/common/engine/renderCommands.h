/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/envProbeParams.h"
#include "../engine/skyboxSetupParameters.h"
#include "../engine/lensFlareSetupParameters.h"
#include "../engine/umbraStructures.h"
#include "../engine/umbraQuery.h"
#include "../engine/game.h"
#include "../engine/updateTransformManager.h"
#include "renderCommandInterface.h"
#include "foliageRenderSettings.h"
#include "meshRenderSettings.h"
#include "renderer.h"
#include "screenshotSystem.h"
#include "particleComponent.h"
#include "renderObject.h"
#include "foliageInstance.h"
#include "normalBlendComponent.h"

class ILoadingScreenFence;
class RenderProxyUpdateInfo;
class IRenderProxy;
class IRenderProxyFadeable;
enum EFadeType : Int32;
class IRenderVideo;
class CFlashMovie;
enum EVideoThreadIndex : Int32;
class IRenderTextureStreamRequest;
class IRenderFramePrefetch;

#ifdef RED_RENDER_COMMAND_DEBUG_NAMES

#define DECLARE_RENDER_COMMAND( _class, _name )									\
public: void* operator new( size_t size ) { return IRenderCommand::Alloc( sizeof( _class ) ); } \
public: virtual const Char* Describe() const { return TXT(_name); }

#else

#define DECLARE_RENDER_COMMAND( _class, _name )									\
public: void* operator new( size_t size ) { return IRenderCommand::Alloc( sizeof( _class ) ); }

#endif


// this command should be used when we want to release a render object on the render thread and not block the main thread in the release
class CRenderCommand_ReleaseRenderObjectOnRenderThread : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ReleaseRenderObjectOnRenderThread, "ReleaseRenderObjectOnRenderThread" );

public:
	IRenderObject*	m_renderObject;

public:
	CRenderCommand_ReleaseRenderObjectOnRenderThread( IRenderObject* renderObject );
	~CRenderCommand_ReleaseRenderObjectOnRenderThread();
	virtual void Execute(){}; //nop
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Render passed scene
class CRenderCommand_RenderScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RenderScene, "RenderScene" );

public:
	CRenderFrame*	m_frame;		//!< Frame to render ( camera, dynamic fragments, post process, etc )
	IRenderScene*	m_scene;		//!< Scene to render ( meshes, particles, terrain, lights )
#ifndef NO_EDITOR
	Bool			m_forcePrefetch;
#endif

public:
	CRenderCommand_RenderScene( CRenderFrame* frame, IRenderScene* scene );
#ifndef NO_EDITOR
	CRenderCommand_RenderScene( CRenderFrame* frame, IRenderScene* scene, Bool forcePrefetch );
#endif
	~CRenderCommand_RenderScene();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Prepare render systems for a new frame
class CRenderCommand_NewFrame : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_NewFrame, "NewFrame" );

public:
	CRenderCommand_NewFrame();
	~CRenderCommand_NewFrame();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Cleanup render systems after the frame
class CRenderCommand_EndFrame : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_EndFrame, "EndFrame" );

public:
	CRenderCommand_EndFrame();
	~CRenderCommand_EndFrame();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Cancel texture streaming
class CRenderCommand_CancelTextureStreaming : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_CancelTextureStreaming, "CancelTextureStreaming" );

	Bool m_flushOnlyUnused;

public:
	CRenderCommand_CancelTextureStreaming();
	CRenderCommand_CancelTextureStreaming(Bool flushOnlyUnused);
	~CRenderCommand_CancelTextureStreaming();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////


/// Stop all texture streaming. Should just be used during shutdown
class CRenderCommand_ShutdownTextureStreaming : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ShutdownTextureStreaming, "ShutdownTextureStreaming" );

public:
	CRenderCommand_ShutdownTextureStreaming();
	~CRenderCommand_ShutdownTextureStreaming();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Explicitly request streaming of textures
class CRenderCommand_StartTextureStreamRequest : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_StartTextureStreamRequest, "RequestTextureStreaming" );

private:
	IRenderTextureStreamRequest*	m_request;

public:
	CRenderCommand_StartTextureStreamRequest( IRenderTextureStreamRequest* request );
	~CRenderCommand_StartTextureStreamRequest();
	virtual void Execute();
};

/// Cancel a texture stream request
class CRenderCommand_CancelTextureStreamRequest : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_CancelTextureStreamRequest, "CancelTextureStreamRequest" );

private:
	IRenderTextureStreamRequest*	m_request;

public:
	CRenderCommand_CancelTextureStreamRequest( IRenderTextureStreamRequest* request );
	~CRenderCommand_CancelTextureStreamRequest();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Prefetch a render frame. Do things like get textures streaming, etc. without actually rendering the frame or affecting the
/// scene (will not cause meshes to change LOD or anything).
class CRenderCommand_StartFramePrefetch : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_StartFramePrefetch, "StartFramePrefetch" );

private:
	IRenderFramePrefetch*	m_prefetch;

public:
	CRenderCommand_StartFramePrefetch( IRenderFramePrefetch* prefetch );
	~CRenderCommand_StartFramePrefetch();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Reset distances on all textures. Disable "mip dropping" on textures with waiting prefetches.
class CRenderCommand_PrepareInitialTextureStreaming : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_PrepareInitialTextureStreaming, "PrepareInitialTextureStreaming" );

private:
	IRenderScene*	m_scene;
	Bool			m_resetDistances;

public:

	CRenderCommand_PrepareInitialTextureStreaming( IRenderScene* scene, Bool resetDistances );
	~CRenderCommand_PrepareInitialTextureStreaming();
	virtual void Execute();
};

/// Re-enabled "mip dropping" for textures with prefetches
class CRenderCommand_FinishInitialTextureStreaming : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_FinishInitialTextureStreaming, "FinishInitialTextureStreaming" );

public:
	CRenderCommand_FinishInitialTextureStreaming();
	~CRenderCommand_FinishInitialTextureStreaming();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Start streaming in envprobes for an initial frame
class CRenderCommand_SetupEnvProbesPrefetch : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetupEnvProbesPrefetch, "SetupEnvProbesPrefetch" );

private:	
	IRenderScene*	m_scene;
	Vector			m_position;

public:
	CRenderCommand_SetupEnvProbesPrefetch( IRenderScene* scene, const Vector &position );
	~CRenderCommand_SetupEnvProbesPrefetch();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update texture streaming, without having to render a frame first.
class CRenderCommand_TickTextureStreaming : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_TickTextureStreaming, "TickTextureStreaming" );

private:
	Bool m_flushOnePrefetch;

public:
	CRenderCommand_TickTextureStreaming( Bool flushOnePrefetch = false );
	~CRenderCommand_TickTextureStreaming();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

class CRenderCommand_TextureCacheAttached : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_TextureCacheAttached, "TextureCacheAttached" );

public:
	CRenderCommand_TextureCacheAttached() {}
	~CRenderCommand_TextureCacheAttached() {}
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

class CRenderCommand_ForceUITextureStreaming : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ForceUITextureStreaming, "ForceUITextureStreaming" );

private:
	Bool m_pinTextures;

public:
	CRenderCommand_ForceUITextureStreaming( Bool pinTextures ) : m_pinTextures( pinTextures ) {}
	~CRenderCommand_ForceUITextureStreaming() {}
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Toggle cinematic mode on the renderer - different texture&mesh streaming strategy, may use different shadow settings, etc
class CRenderCommand_ToggleCinematicMode : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ToggleCinematicMode, "ToggleCinematicMode" );

private:
	Bool	m_cinematicMode;

public:
	CRenderCommand_ToggleCinematicMode( const Bool cinematicMode );
	~CRenderCommand_ToggleCinematicMode();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

class CLoadingScreenBlur;

class CRenderCommand_ShowLoadingScreenBlur : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ShowLoadingScreenBlur, "ShowLoadingScreenBlur" );

private:
	Float	m_blurScale;
	Float	m_timeScale;
	Bool	m_useFallback;

public:
	CRenderCommand_ShowLoadingScreenBlur( Float blurScale , Float timeScale, Bool useBlackFallback = false );
	~CRenderCommand_ShowLoadingScreenBlur();
	virtual void Execute();
};

class CRenderCommand_HideLoadingScreenBlur : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_HideLoadingScreenBlur, "HideLoadingScreenBlur" );

private:
	Float	m_fadeTime;

public:
	CRenderCommand_HideLoadingScreenBlur( Float fadeTime );
	~CRenderCommand_HideLoadingScreenBlur();
	virtual void Execute();
};


////////////////////////////////////////////////////////////////////////////////////////////

/// Set loading screen fence
class CRenderCommand_SetLoadingScreenFence : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetLoadingScreenFence, "SetLoadingScreenFence" );

private:
	ILoadingScreenFence* m_fence;
	Float				 m_fadeInTime;
	Bool				 m_hideAtStart;

public:
	CRenderCommand_SetLoadingScreenFence( ILoadingScreenFence* fence, Float fadeInTime, Bool hideAtStart = false );
	~CRenderCommand_SetLoadingScreenFence();
	virtual void Execute();
};

//////////////////////////////////////////////////////////////////////////

class CRenderCommand_FadeOutLoadingScreen : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_FadeOutLoadingScreen, "FadeOutLoadingScreen" );

private:
	ILoadingScreenFence*		m_fence;
	Float						m_fadeOutTime;

public:
	CRenderCommand_FadeOutLoadingScreen( ILoadingScreenFence* fence, Float fadeOutTime );
	~CRenderCommand_FadeOutLoadingScreen();
	virtual void Execute();
};

//////////////////////////////////////////////////////////////////////////

class CRenderCommand_SetLoadingOverlayFlash : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetLoadingOverlayFlash, "SetLoadingOverlayFlash" );

private:
	CFlashMovie*				m_loadingOverlayFlash;

public:
	CRenderCommand_SetLoadingOverlayFlash( CFlashMovie* loadingOverlayFlash );
	~CRenderCommand_SetLoadingOverlayFlash();
	virtual void Execute();
};

//////////////////////////////////////////////////////////////////////////

class CRenderCommand_ToggleLoadingOverlay : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ToggleLoadingOverlay, "ToggleLoadingOverlay" );

private:
	String	m_caption;
	Bool	m_visible;

public:
	CRenderCommand_ToggleLoadingOverlay( Bool visible, const String& caption );
	~CRenderCommand_ToggleLoadingOverlay();
	virtual void Execute();
};

//////////////////////////////////////////////////////////////////////////

class CRenderCommand_SetVideoFlash : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetVideoFlash, "SetVideoFlash" );

private:
	CFlashMovie*				m_videoFlash;

public:
	CRenderCommand_SetVideoFlash( CFlashMovie* videoFlash );
	~CRenderCommand_SetVideoFlash();
	virtual void Execute();
};

//////////////////////////////////////////////////////////////////////////
class CRenderCommand_ToggleVideoPause : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ToggleVideoPause, "ToggleVideoPause" );

private:
	Bool m_pause;

public:
	CRenderCommand_ToggleVideoPause( Bool pause );
	~CRenderCommand_ToggleVideoPause();
	virtual void Execute();
};

//////////////////////////////////////////////////////////////////////////

class CRenderCommand_PlayVideo : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_PlayVideo, "PlayVideo" );

private:
	IRenderVideo*				m_renderVideo;
	EVideoThreadIndex			m_threadIndex;

public:
	CRenderCommand_PlayVideo( IRenderVideo* renderVideo, EVideoThreadIndex threadIndex );
	~CRenderCommand_PlayVideo();
	virtual void Execute();
};

//////////////////////////////////////////////////////////////////////////

class CRenderCommand_W3HackSetVideoClearRGB : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_W3HackSetVideoClearRGB, "SetVideoClearRGB" );

private:
	Color m_rgb;

public:
	CRenderCommand_W3HackSetVideoClearRGB( const Color& rgb );
	~CRenderCommand_W3HackSetVideoClearRGB();
	virtual void Execute();
};

class CRenderCommand_W3HackShowVideoBackground : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_W3HackShowVideoBackground, "ShowVideobackground" );

public:
	CRenderCommand_W3HackShowVideoBackground();
	~CRenderCommand_W3HackShowVideoBackground();
	virtual void Execute();
};

class CRenderCommand_W3HackHideVideoBackground : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_W3HackHideVideoBackground, "HideVideobackground" );

public:
	CRenderCommand_W3HackHideVideoBackground();
	~CRenderCommand_W3HackHideVideoBackground();
	virtual void Execute();
};

//////////////////////////////////////////////////////////////////////////

class CRenderCommand_SetVideoMasterVolume : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetVideoMasterVolume, "SetVideoMasterVolume" );

private:
	Float m_volumePercent;

public:
	CRenderCommand_SetVideoMasterVolume( Float volumePercent );
	~CRenderCommand_SetVideoMasterVolume();
	virtual void Execute();
};

class CRenderCommand_SetVideoVoiceVolume : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetVideoVoiceVolume, "SetVideoVoiceVolume" );

private:
	Float m_volumePercent;

public:
	CRenderCommand_SetVideoVoiceVolume( Float volumePercent );
	~CRenderCommand_SetVideoVoiceVolume();
	virtual void Execute();
};

class CRenderCommand_SetVideoEffectsVolume : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetVideoEffectsVolume, "SetVideoEffectsVolume" );

private:
	Float m_volumePercent;

public:
	CRenderCommand_SetVideoEffectsVolume( Float volumePercent );
	~CRenderCommand_SetVideoEffectsVolume();
	virtual void Execute();
};

//////////////////////////////////////////////////////////////////////////

class CRenderCommand_PlayLoadingScreenVideo : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_PlayLoadingScreenVideo, "PlayLoadingScreenVideo" );

private:
	IRenderVideo*				m_renderVideo;
	ILoadingScreenFence*		m_fence;

public:
	CRenderCommand_PlayLoadingScreenVideo( IRenderVideo* renderVideo, ILoadingScreenFence* fence );
	~CRenderCommand_PlayLoadingScreenVideo();
	virtual void Execute();
};

//////////////////////////////////////////////////////////////////////////

class CRenderCommand_CancelVideo : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_CancelVideo, "CancelVideo" );

private:
	IRenderVideo*				m_renderVideo;

public:
	CRenderCommand_CancelVideo( IRenderVideo* renderVideo );
	~CRenderCommand_CancelVideo();
	virtual void Execute();
};

//////////////////////////////////////////////////////////////////////////

class CRenderCommand_ToggleLoadingVideoSkip : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ToggleLoadingVideoSkip, "ToggleLoadingVideoSkip" );

private:
	ILoadingScreenFence*		m_fence;
	Bool						m_enabled;

public:
	CRenderCommand_ToggleLoadingVideoSkip( ILoadingScreenFence* fence, Bool enabled );
	~CRenderCommand_ToggleLoadingVideoSkip();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

class CRenderCommand_SetLoadingPCInput : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetLoadingPCInput, "SetLoadingPCInput" );

private:
	ILoadingScreenFence*		m_fence;
	Bool						m_enabled;

public:
	CRenderCommand_SetLoadingPCInput( ILoadingScreenFence* fence, Bool enabled );
	~CRenderCommand_SetLoadingPCInput();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

class CRenderCommand_SetExpansionsAvailable : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetExpansionsAvailable, "SetExpansionsAvailable" );

private:
	ILoadingScreenFence*		m_fence;
	Bool						m_ep1;
	Bool						m_ep2;

public:
	CRenderCommand_SetExpansionsAvailable( ILoadingScreenFence* fence, Bool ep1, Bool ep2 );
	~CRenderCommand_SetExpansionsAvailable();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_SCALEFORM

/// GFx Render Command

namespace Scaleform { namespace Render {
	class ThreadCommand;
} }

class IRenderScaleform;

class CRenderCommand_GuiRenderCommandWrapper : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_GuiRenderCommandWrapper, "GuiRenderCommandWrapper" );

private:
	SF::Ptr<SF::Render::ThreadCommand>	m_command;
	SF::Ptr<IRenderScaleform>			m_renderScaleform;  // So refcount > 0 while it has pending commands

public:
	CRenderCommand_GuiRenderCommandWrapper( SF::Render::ThreadCommand* command, IRenderScaleform* renderScaleform );
	virtual void Execute();
};

#endif

////////////////////////////////////////////////////////////////////////////////////////////

/// Update progress status, works when non interactive rendering is active
class CRenderCommand_UpdateProgressStatus : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateProgressStatus, "UpdateProgressStatus" );

public:
	Char	m_status[ 128 ];

public:
	CRenderCommand_UpdateProgressStatus( const String& status );
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update progress, works when non interactive rendering is active
class CRenderCommand_UpdateProgress : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateProgress, "UpdateProgress" );

public:
	Float	m_progress;

public:
	CRenderCommand_UpdateProgress( Float progress );
	virtual void Execute();
};


#ifdef RED_FINAL_BUILD
	#define UPDATE_LOADING_STATUS( text ) 
#else
	#define UPDATE_LOADING_STATUS( text ) ( new CRenderCommand_UpdateProgressStatus( text ) )->Commit();
#endif

////////////////////////////////////////////////////////////////////////////////////////////

/// Register rendering exclusion object in scene
class CRenderCommand_AddRenderingExclusionToScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_AddRenderingExclusionToScene, "AddRenderingExclusionToScene" );

public:
	IRenderScene*					m_scene;
	IRenderVisibilityExclusion*		m_object;

public:
	CRenderCommand_AddRenderingExclusionToScene( IRenderScene* scene, IRenderVisibilityExclusion* object );
	~CRenderCommand_AddRenderingExclusionToScene();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Unregister rendering exclusion object in scene
class CRenderCommand_RemoveRenderingExclusionToScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveRenderingExclusionToScene, "RemoveRenderingExclusionToScene" );

public:
	IRenderScene*					m_scene;
	IRenderVisibilityExclusion*		m_object;

public:
	CRenderCommand_RemoveRenderingExclusionToScene( IRenderScene* scene, IRenderVisibilityExclusion* object );
	~CRenderCommand_RemoveRenderingExclusionToScene();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Change state of the rendering exclusion object
class CRenderCommand_ToggleRenderingExclusion : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ToggleRenderingExclusion, "ToggleRenderingExclusion" );

public:
	IRenderScene*					m_scene;
	IRenderVisibilityExclusion*		m_object;
	Bool							m_state;

public:
	CRenderCommand_ToggleRenderingExclusion( IRenderScene* scene, IRenderVisibilityExclusion* object, const Bool state );
	~CRenderCommand_ToggleRenderingExclusion();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Add rendering proxy to rendering scene
class CRenderCommand_AddProxyToScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_AddProxyToScene, "AddProxyToScene" );

public:
	IRenderScene*		m_scene;
	IRenderProxy*		m_proxy;

public:
	CRenderCommand_AddProxyToScene( IRenderScene* scene, IRenderProxy* proxy );
	~CRenderCommand_AddProxyToScene();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Add stripe proxy to rendering scene
class CRenderCommand_AddStripeToScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_AddStripeToScene, "AddStripeToScene" );

public:
	IRenderScene*	m_scene;
	IRenderProxy*	m_proxy;

public:
	CRenderCommand_AddStripeToScene( IRenderScene* scene, IRenderProxy* proxy );
	~CRenderCommand_AddStripeToScene();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Add fur proxy to rendering scene
class CRenderCommand_AddFurToScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_AddFurToScene, "AddFurToScene" );

public:
	IRenderScene*	m_scene;
	IRenderProxy*	m_proxy;

public:
	CRenderCommand_AddFurToScene( IRenderScene* scene, IRenderProxy* proxy );
	~CRenderCommand_AddFurToScene();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update fur proxy with wind params to rendering scene
class CRenderCommand_UpdateFurParams : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateFurParams, "UpdateFurParams" );

public:
	IRenderProxy*	m_proxy;
	Vector			m_wind;
	Float			m_wetness;

public:
	CRenderCommand_UpdateFurParams( IRenderProxy* proxy, const Vector& wind, Float wet );
	~CRenderCommand_UpdateFurParams();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR
#ifdef USE_NVIDIA_FUR
/// Update fur proxy params for editor only
class CRenderCommand_EditorSetFurParams : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_EditorSetFurParams, "EditorSetFurParams" );

public:
	IRenderProxy*							m_proxy;
	struct GFSDK_HairInstanceDescriptor*	m_params;
	Uint32									m_materialIndex;

public:
	CRenderCommand_EditorSetFurParams( IRenderProxy* proxy, struct GFSDK_HairInstanceDescriptor* newParams, Uint32 index );
	~CRenderCommand_EditorSetFurParams();
	virtual void Execute();
};
#endif //USE_NVIDIA_FUR
#endif //NO_EDITOR

////////////////////////////////////////////////////////////////////////////////////////////

/// Remove rendering proxy from rendering scene
class CRenderCommand_RemoveProxyFromScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveProxyFromScene, "RemoveProxyFromScene" );

public:
	IRenderScene*		m_scene;
	IRenderProxy*		m_proxy;

public:
	CRenderCommand_RemoveProxyFromScene( IRenderScene* scene, IRenderProxy* proxy );
	~CRenderCommand_RemoveProxyFromScene();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Remove stripe proxy to rendering scene
class CRenderCommand_RemoveStripeFromScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveStripeFromScene, "RemoveStripeFromScene" );

public:
	IRenderScene*	m_scene;
	IRenderProxy*	m_proxy;

public:
	CRenderCommand_RemoveStripeFromScene( IRenderScene* scene, IRenderProxy* proxy );
	~CRenderCommand_RemoveStripeFromScene();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Remove fur rendering proxy from rendering scene
class CRenderCommand_RemoveFurFromScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveFurFromScene, "RemoveFurFromScene" );

public:
	IRenderScene*	m_scene;
	IRenderProxy*	m_proxy;

public:
	CRenderCommand_RemoveFurFromScene( IRenderScene* scene, IRenderProxy* proxy );
	~CRenderCommand_RemoveFurFromScene();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
/// Upload occlusion data
class CRenderCommand_UploadOcclusionDataToScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UploadOcclusionDataToScene, "UploadOcclusionDataToScene" );

public:
	CUmbraScene*				m_umbraScene;
	IRenderScene*				m_renderScene;
	IRenderObject*				m_occlusionData;
	TVisibleChunksIndices		m_remapTable;
	TObjectIDToIndexMap			m_objectIDToIndexMap;

public:
	CRenderCommand_UploadOcclusionDataToScene( CUmbraScene* umbraScene, IRenderScene* renderScene, IRenderObject* occlusionData, TVisibleChunksIndices& remapTable, TObjectIDToIndexMap& objectIDToIndexMap );
	~CRenderCommand_UploadOcclusionDataToScene();
	virtual void Execute();
};
#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
class CRenderCommand_SetDoorState : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetDoorState, "SetDoorState" );

public:
	IRenderScene*	m_scene;
	TObjectIdType	m_objectId;
	Bool			m_opened;

public:
	CRenderCommand_SetDoorState( IRenderScene* scene, TObjectIdType objectId, Bool opened );
	virtual ~CRenderCommand_SetDoorState();

	virtual void Execute();
};
#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
class CRenderCommand_SetCutsceneModeForGates : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetCutsceneModeForGates, "SetCutsceneModeForGates" );

public:
	IRenderScene*	m_scene;
	Bool			m_isCutscene;

public:
	CRenderCommand_SetCutsceneModeForGates( IRenderScene* scene, Bool isCutscene );
	virtual ~CRenderCommand_SetCutsceneModeForGates();

	virtual void Execute();
};
#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
class CRenderCommand_ClearOcclusionData : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ClearOcclusionData, "ClearOcclusionData" );

public:
	CUmbraScene*	m_umbraScene;
	IRenderScene*	m_scene;

public:
	CRenderCommand_ClearOcclusionData( CUmbraScene* umbraScene, IRenderScene* scene );
	virtual ~CRenderCommand_ClearOcclusionData();

	virtual void Execute();
};
#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
class CRenderCommand_UploadObjectCache : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UploadObjectCache, "UploadObjectCache" );

public:
	IRenderScene* m_scene;
	TObjectCache m_objectCache;

public:
	CRenderCommand_UploadObjectCache( IRenderScene* scene, TObjectCache& objectCache );
	virtual ~CRenderCommand_UploadObjectCache();

	virtual void Execute();
};
#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
class CRenderCommand_SetValidityOfOcclusionData : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetValidityOfOcclusionData, "SetValidityOfOcclusionData" );

public:
	IRenderScene*	m_scene;
	Bool			m_isDataValid;

public:
	CRenderCommand_SetValidityOfOcclusionData( IRenderScene* scene, Bool isDataValid );
	virtual ~CRenderCommand_SetValidityOfOcclusionData();

	virtual void Execute();
};
#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
class CRenderCommand_UpdateQueryThreshold : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateQueryThreshold, "UpdateQueryThreshold" );

public:
	IRenderScene*	m_scene;
	Float			m_value;

public:
	CRenderCommand_UpdateQueryThreshold( IRenderScene* scene, Float value );
	virtual ~CRenderCommand_UpdateQueryThreshold();

	virtual void Execute();
};
#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
#ifndef NO_EDITOR
class CRenderCommand_DumpVisibleMeshes : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_DumpVisibleMeshes, "DumpVisibleMeshes" );

public:
	IRenderScene*			m_scene;
	TLoadedComponentsMap	m_map;
	String					m_path;

public:
	CRenderCommand_DumpVisibleMeshes( IRenderScene* scene, const TLoadedComponentsMap& componentMap, const String& path );
	virtual ~CRenderCommand_DumpVisibleMeshes();

	virtual void Execute();
};
#endif // NO_EDITOR

#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
class CRenderCommand_PerformVisibilityQueries : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_PerformVisibilityQueries, "PerformVisibilityQueries" );

public:
	IRenderScene*			m_scene;
	UmbraQueryBatch			m_queryList;

public:
	CRenderCommand_PerformVisibilityQueries( IRenderScene* scene, UmbraQueryBatch&& queryList );
	virtual ~CRenderCommand_PerformVisibilityQueries();

	virtual void Execute();
};
#endif // USE_UMBRA


////////////////////////////////////////////////////////////////////////////////////////////

/// Relink rendering proxy ( update bounding box and local to world matrix )
class CRenderCommand_RelinkProxy : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RelinkProxy, "RelinkProxy" );

public:
	IRenderProxy*			m_proxy;
	Box						m_boundingBox;
	Matrix					m_localToWorld;

public:
	CRenderCommand_RelinkProxy( IRenderProxy* proxy, const RenderProxyUpdateInfo& data );
	~CRenderCommand_RelinkProxy();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Relink rendering proxies ( update bounding boxes and local to world matrices )
class CRenderCommand_BatchSkinAndRelinkProxies : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_BatchSkinAndRelinkProxies, "BatchSkinAndRelinkProxies" );

public:
	Uint8					m_proxyCount;
	IRenderProxy*			m_proxies		[UPDATE_CONTEXT_BATCH_SIZE];
	IRenderObject*			m_skinningData	[UPDATE_CONTEXT_BATCH_SIZE];
	RenderProxyRelinkInfo	m_relinkInfos	[UPDATE_CONTEXT_BATCH_SIZE];

public:
	CRenderCommand_BatchSkinAndRelinkProxies( Uint8 proxyCount, IRenderProxy** proxies, IRenderObject** skinningData, const RenderProxyRelinkInfo* relinkInfos );
	~CRenderCommand_BatchSkinAndRelinkProxies();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Env probe transformation changed
class CRenderCommand_EnvProbeParamsChanged : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_EnvProbeParamsChanged, "EnvProbeParamsChanged" );

public:
	IRenderResource*	m_probe;
	SEnvProbeParams		m_params;

public:
	CRenderCommand_EnvProbeParamsChanged( IRenderResource* probe, const SEnvProbeParams& params );
	~CRenderCommand_EnvProbeParamsChanged();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update skinning data for mesh proxy
class CRenderCommand_UpdateSkinningDataAndRelink : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateSkinningDataAndRelink, "UpdateSkinningDataAndRelink" );

public:
	Matrix				m_localToWorld;
	Box					m_boundingBox;
	Vector				m_headCenterPosAndDimming;
	Vector				m_headCenterAxisX;
	Vector				m_headCenterAxisY;
	Vector				m_headCenterAxisZ;
	IRenderProxy*		m_proxy;
	IRenderObject*		m_data;
	Bool				m_transformOnly; // drey todo

public:
	CRenderCommand_UpdateSkinningDataAndRelink( IRenderProxy* proxy, IRenderObject* data, const RenderProxyUpdateInfo& data2 );
	~CRenderCommand_UpdateSkinningDataAndRelink();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update dissolving override for mesh proxy
class CRenderCommand_SetDestructionMeshDissolving : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetDestructionMeshDissolving, "SetDestructionMeshDissolving" );

public:
	IRenderProxy*			m_proxy;
	Bool					m_useDissolve;
public:
	CRenderCommand_SetDestructionMeshDissolving( IRenderProxy*	proxy, Bool useDissolve = true );
	~CRenderCommand_SetDestructionMeshDissolving();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update active indices
class CRenderCommand_UpdateDestructionMeshActiveIndices : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateDestructionMeshActiveIndices, "SetDestructionMeshDissolving" );

public:
	IRenderProxy*					m_proxy;
	TDynArray< Uint16 >				m_activeIndices;
	TDynArray< Uint32 >				m_chunkOffsets;
	TDynArray< Uint32 >				m_chunkNumIndices;
public:
	CRenderCommand_UpdateDestructionMeshActiveIndices( IRenderProxy*	proxy, TDynArray< Uint16 >&& activeIndices, TDynArray< Uint32 >&& newOffsets, TDynArray< Uint32 >&& chunkNumIndices  );
	~CRenderCommand_UpdateDestructionMeshActiveIndices();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Render light parameter
enum ERenderLightParameter : Uint32
{
	RLP_Attenuation,
	RLP_Brightness,
	RLP_Radius,
	RLP_Color,
	RLP_AllowDistantFade
};

/// Update light parameter
class CRenderCommand_UpdateLightProxyParameter : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateLightProxyParameter, "UpdateLightProxyParameter" );

public:
	IRenderProxy*			m_proxy;
	Vector					m_data;
	ERenderLightParameter	m_parameter;	

public:
	CRenderCommand_UpdateLightProxyParameter( IRenderProxy* proxy, const Vector& data, ERenderLightParameter parameter );
	~CRenderCommand_UpdateLightProxyParameter();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Fence in the command buffer
class CRenderCommand_Fence : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_Fence, "Fence" );

public:
	IRenderFence*		m_fence;

public:
	CRenderCommand_Fence( IRenderFence* fence );
	~CRenderCommand_Fence();
	virtual void Commit();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update selection hit proxy ID for rendering proxy
class CRenderCommand_UpdateHitProxyID : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateHitProxyID, "UpdateHitProxyID" );

public:
	IRenderProxy*		m_proxy;		//!< Render proxy object
	CHitProxyID			m_id;			//!< Hit proxy ID for that object

public:
	CRenderCommand_UpdateHitProxyID( IRenderProxy* proxy, const CHitProxyID& id );
	~CRenderCommand_UpdateHitProxyID();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update selection flag for object
class CRenderCommand_SetSelectionFlag : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetSelectionFlag, "SetSelectionFlag" );

public:
	IRenderProxy*		m_proxy;		//!< Render proxy object
	Bool				m_flag;			//!< Proxy selection flag

public:
	CRenderCommand_SetSelectionFlag( IRenderProxy* proxy, Bool selectionFlag );
	~CRenderCommand_SetSelectionFlag();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update force hide flag for object
class CRenderCommand_SetAutoFade : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetAutoFade, "SetAutoFade" );

public:
	IRenderScene*			m_scene;		//!< Render scene object
	IRenderProxy*			m_proxy;		//!< Fade proxy object
	EFadeType				m_type;			//!< Fade type

public:
	CRenderCommand_SetAutoFade( IRenderScene* scene, IRenderProxy* proxy, EFadeType type );
	~CRenderCommand_SetAutoFade();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update temporary fade
class CRenderCommand_SetTemporaryFade : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetTemporaryFade, "UpdateTemporaryFade" );

public:
	IRenderProxy*			m_proxy;		//!< Render proxy object

public:
	CRenderCommand_SetTemporaryFade( IRenderProxy* proxy );
	~CRenderCommand_SetTemporaryFade();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update effect parameters
class CRenderCommand_UpdateEffectParameters : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateEffectParameters, "UpdateEffectParameters" );

public:
	IRenderProxy*		m_proxy;		//!< Render proxy object
	Vector				m_paramValue;	//!< Parameter value
	Int32					m_paramIndex;	//!< Parameter index

public:
	CRenderCommand_UpdateEffectParameters( IRenderProxy* proxy, const Vector& paramValue, Uint32 paramIndex );
	~CRenderCommand_UpdateEffectParameters();
	virtual void Execute();
};

/// Update effect parameters in batch
class CRenderCommand_UpdateOverrideParametersBatch : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateOverrideParametersBatch, "UpdateOverrideParametersBatch" );

public:
	IRenderProxy*		m_proxy;		//!< Render proxy object
	Vector				m_paramValue;	//!< Parameter value

public:
	CRenderCommand_UpdateOverrideParametersBatch( IRenderProxy* proxy, const Vector& paramValue );
	~CRenderCommand_UpdateOverrideParametersBatch();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update light parameter
class CRenderCommand_UpdateLightParameter : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateLightParameter, "UpdateLightParameter" );

public:
	IRenderProxy*		m_proxy;		//!< Render proxy object
	Float				m_paramValue;	//!< Parameter value
	CName				m_paramName;	//!< Parameter name

public:
	CRenderCommand_UpdateLightParameter( IRenderProxy* proxy, CName paramName, Float value );
	~CRenderCommand_UpdateLightParameter();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update light color
class CRenderCommand_UpdateLightColor : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateLightColor, "UpdateLightColor" );

public:
	IRenderProxy*		m_proxy;		//!< Render proxy object
	Color				m_color;		//!< Parameter value

public:
	CRenderCommand_UpdateLightColor( IRenderProxy* proxy, Color value );
	~CRenderCommand_UpdateLightColor();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update color shift matrices
class CRenderCommand_UpdateColorShiftMatrices : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateColorShiftMatrices, "UpdateColorShiftMatrices" );

public:
	IRenderProxy*		m_proxy;		//!< Render proxy object
	Matrix				m_colorShift0;	//!< Color shift matrix for region 0
	Matrix				m_colorShift1;	//!< Color shift matrix for region 1

public:
	CRenderCommand_UpdateColorShiftMatrices( IRenderProxy* proxy, const Matrix& region0, const Matrix& region1 );
	~CRenderCommand_UpdateColorShiftMatrices();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Update context for particle simulation.
struct SSimulationContextUpdate;
class CRenderCommand_UpdateParticlesSimulatationContext : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateParticlesSimulatationContext, "UpdateParticlesSimulationContext" );

public:
	IRenderProxy*				m_proxy;			//!< Render proxy object for particle component
	SSimulationContextUpdate	m_contextUpdate;	//!< Context data for the update

public:
	CRenderCommand_UpdateParticlesSimulatationContext( IRenderProxy* proxy, const SSimulationContextUpdate & update );
	~CRenderCommand_UpdateParticlesSimulatationContext();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

class CRenderCommand_UpdateParticlesSimulatationContextAndRelink : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateParticlesSimulatationContextAndRelink, "UpdateParticlesSimulationContextAndRelink" );

public:
	Matrix						m_localToWorld;
	IRenderProxy*				m_proxy;			//!< Render proxy object for particle component
	SSimulationContextUpdate	m_contextUpdate;	//!< Context data for the update

public:

	CRenderCommand_UpdateParticlesSimulatationContextAndRelink( IRenderProxy* proxy, const SSimulationContextUpdate & update, const RenderProxyUpdateInfo& data  );
	~CRenderCommand_UpdateParticlesSimulatationContextAndRelink();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

struct SUpdateParticlesBatchedCommand;
class CRenderCommand_BatchUpdateParticlesSimulatationContextAndRelink : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_BatchUpdateParticlesSimulatationContextAndRelink, "BatchUpdateParticlesSimulatationContextAndRelink" );

public:
	Uint8						   m_proxyCount;
	SUpdateParticlesBatchedCommand m_batchedCommands[UPDATE_CONTEXT_BATCH_SIZE];

public:

	CRenderCommand_BatchUpdateParticlesSimulatationContextAndRelink( Uint8 proxyCount, SUpdateParticlesBatchedCommand* m_batchedCommands );
	~CRenderCommand_BatchUpdateParticlesSimulatationContextAndRelink();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Take regular screenshot
class CRenderCommand_TakeScreenshot : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_TakeScreenshot, "TakeScreenshot" );

public:
	SScreenshotParameters	m_screenshotParameters;			//!< Screenshot parameters

public:
	CRenderCommand_TakeScreenshot( const SScreenshotParameters& screenshotParameters );
	~CRenderCommand_TakeScreenshot();
	virtual void Execute();
};

/// Take antialiased screenshot in "uber" quality
class CRenderCommand_TakeUberScreenshot : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_TakeUberScreenshot, "TakeUberScreenshot" );

public:
	CRenderFrame*			m_frame;						//!< Frame to render ( camera, dynamic fragments, post process, etc )
	IRenderScene*			m_scene;						//!< Scene to render ( meshes, particles, terrain, lights )
	SScreenshotParameters	m_screenshotParameters;			//!< Screenshot parameters
	Bool*					m_status;						//!< was screenshot successful
	
public:
	CRenderCommand_TakeUberScreenshot( CRenderFrame* frame, IRenderScene* scene, const SScreenshotParameters& screenshotParameters, Bool& status );
	~CRenderCommand_TakeUberScreenshot();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Grab rendered frame to move
class CRenderCommand_GrabMovieFrame : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_GrabMovieFrame, "GrabMovieFrame" );

public:
	IRenderMovie*		m_movie;		//!< Movie being grabbed

public:
	CRenderCommand_GrabMovieFrame( IRenderMovie* movie );
	~CRenderCommand_GrabMovieFrame();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Start continuous grabbing of screenshots
class CRenderCommand_ToggleContinuousScreenshot : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ToggleContinuousScreenshot, "ToggleContinuousScreenshot" );

public:
	Bool						m_isEnabled;
	EFrameCaptureSaveFormat		m_saveFormat;
	Bool						m_useUbersampling;

public:
	CRenderCommand_ToggleContinuousScreenshot( Bool isEnabled, EFrameCaptureSaveFormat saveFormat, Bool ubersample = false );
	~CRenderCommand_ToggleContinuousScreenshot();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Mesh material highlight
class CRenderCommand_ToggleMeshMaterialHighlight : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ToggleMeshMaterialHighlight, "ToggleMeshMaterialHighlight" );

public:
	IRenderResource*		m_mesh;
	TDynArray< Uint32 >		m_chunkIndices;

public:
	CRenderCommand_ToggleMeshMaterialHighlight( CMesh* mesh, Uint32 materialIndex );
	~CRenderCommand_ToggleMeshMaterialHighlight();
	virtual void Execute();
};

/// Mesh chunk highlight
class CRenderCommand_ToggleMeshChunkHighlight : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ToggleMeshChunkHighlight, "ToggleMeshMaterialHighlight" );

public:
	IRenderResource*		m_mesh;
	TDynArray< Uint32 >		m_chunkIndices;

public:
	CRenderCommand_ToggleMeshChunkHighlight( CMesh* mesh, Uint32 chunkIndex );
	CRenderCommand_ToggleMeshChunkHighlight( CMesh* mesh, const TDynArray< Uint32 >& chunkIndices );
	~CRenderCommand_ToggleMeshChunkHighlight();
	virtual void Execute();
};

class CRenderCommand_UpdateMeshRenderParams : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateMeshRenderParams, "UpdateMeshRenderParams" );

public:
	IRenderProxy*		m_meshProxy;
	SMeshRenderParams	m_meshRenderParams;

public:
	CRenderCommand_UpdateMeshRenderParams( IRenderProxy* proxy, const SMeshRenderParams& params );
	~CRenderCommand_UpdateMeshRenderParams();
	virtual void Execute();

};

////////////////////////////////////////////////////////////////////////////////////////////

/// Suppress or allow normal scene rendering. Suppressing will cause rendering to the viewport to be skipped, and texture streaming to be
/// effectively frozen. While rendering is suppressed on at least one viewport, textures will not be automatically "expired". Textures can
/// still be streamed in as needed (and unloaded to make room for new, more important, textures).
///
/// Rendering suppression work on a ref-counted system, so if a viewport is suppressed 3 times, it must be allowed 3 times in order for
/// things to resume.
class CRenderCommand_SuppressSceneRendering : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SuppressSceneRendering, "SuppressSceneRendering" );

public:
	IRenderScene*	m_restoreScene;
	CRenderFrame*	m_restoreFrame;
	IViewport*		m_viewport;
	Bool			m_state;

public:
	// Setting suppressed rendering on _must_ have a matching setting it off. If there are more "on"s than "off"s, then rendering
	// will remain off.
	//
	// Optional restoreScene & restoreFrame allow "priming" the texture streaming for a given frame. This way, any textures used in
	// that frame will continue to stream in if needed, and any textures not used will be evicted first in the event of running out of
	// texture budget.
	CRenderCommand_SuppressSceneRendering( IViewport* viewport, Bool state, IRenderScene* restoreScene = nullptr, CRenderFrame* restoreFrame = nullptr );
	~CRenderCommand_SuppressSceneRendering();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Force all fade transtions on given scene to finish
class CRenderCommand_FinishFades : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_FinishFades, "FinishFades" );

public:
	IRenderScene*	m_scene;

public:
	CRenderCommand_FinishFades( IRenderScene* scene );
	~CRenderCommand_FinishFades();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Special proxy highligh shit
class CRenderCommand_ToggleMeshSelectionOverride : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ToggleMeshSelectionOverride, "ToggleMeshSelectionOverride" );

public:
	Bool			m_state;

public:
	CRenderCommand_ToggleMeshSelectionOverride( Bool state );
	~CRenderCommand_ToggleMeshSelectionOverride();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////
// Normal-blend stuff

class CRenderCommand_SetNormalBlendMaterial : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetNormalBlendMaterial, "SetNormalBlendMaterial" );

public:
	IRenderProxy*		m_proxy;
	IRenderResource*	m_material;	
	IRenderResource*	m_parameters;

	IRenderResource*	m_sourceBaseMaterial;	//!< The material that should be replaced. Can be NULL
	IRenderResource*	m_sourceNormalTexture;	//!< The normal-map texture that should be replaced (any material that uses this will be replaced). Can be NULL

public:
	CRenderCommand_SetNormalBlendMaterial( IRenderProxy* proxy, IMaterial* normalBlendMaterial, IMaterial* sourceBaseMaterial, ITexture* sourceNormalTexture );
	~CRenderCommand_SetNormalBlendMaterial();
	virtual void Execute();
};

class CRenderCommand_DefineNormalBlendAreas : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_DefineNormalBlendAreas, "DefineNormalBlendAreas" );

public:
	IRenderProxy*	m_proxy;
	Uint32			m_numAreas;
	Uint32			m_firstArea;
	Float			m_areas[NUM_NORMALBLEND_AREAS * 4];	//!< 4 floats per area: left,top,right,bottom.

public:
	CRenderCommand_DefineNormalBlendAreas( IRenderProxy* proxy,  Uint32 firstArea, Uint32 numAreas, const Vector* areas );
	~CRenderCommand_DefineNormalBlendAreas();
	virtual void Execute();
};

class CRenderCommand_UpdateNormalBlendWeights : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateNormalBlendWeights, "UpdateMimicWeights" );

public:
	IRenderProxy*		m_proxy;
	Uint32				m_numWeights;
	Uint32				m_firstWeight;
	Float				m_weights[NUM_NORMALBLEND_AREAS];

public:
	CRenderCommand_UpdateNormalBlendWeights( IRenderProxy* proxy, Uint32 firstWeight, Uint32 numWeights, const Float *weights );
	~CRenderCommand_UpdateNormalBlendWeights();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Override proxy material
class CRenderCommand_OverrideProxyMaterial : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_OverrideProxyMaterial, "OverrideProxyMaterial" );

public:
	IRenderProxy*					m_proxy;
	IRenderResource*				m_material;	
	IRenderResource*				m_parameters;
	Bool							m_drawOriginal;

public:
	CRenderCommand_OverrideProxyMaterial( IRenderProxy* proxy, IMaterial* material, Bool drawOriginal );
	~CRenderCommand_OverrideProxyMaterial();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Disable proxy material override
class CRenderCommand_DisableProxyMaterialOverride : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_DisableProxyMaterialOverride, "DisableProxyMaterialOverride" );

public:
	IRenderProxy*					m_proxy;

public:
	CRenderCommand_DisableProxyMaterialOverride( IRenderProxy* proxy );
	~CRenderCommand_DisableProxyMaterialOverride();
	virtual void Execute();
};


////////////////////////////////////////////////////////////////////////////////////////////

class CRenderCommand_DisableAllGameplayEffects : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_DisableAllGameplayEffects, "DisableAllGameplayEffects" );

public:
	CRenderCommand_DisableAllGameplayEffects();
	~CRenderCommand_DisableAllGameplayEffects();
	virtual void Execute();
};


/// Add gameplay post fx
class CRenderCommand_AddFocusModePostFx : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_AddFocusModePostFx, "AddCatPostFx" );

private:	
	Float m_desaturation;
	Float m_highlightBoost;

public:
	CRenderCommand_AddFocusModePostFx( Float desaturation, Float highlightBoost );
	~CRenderCommand_AddFocusModePostFx();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Remove gameplay post fx
class CRenderCommand_RemoveFocusModePostFx : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveFocusModePostFx, "RemoveFocusModePostFx" );

private:
	Bool m_forceDisable;

public:
	CRenderCommand_RemoveFocusModePostFx( Bool forceDisable );
	~CRenderCommand_RemoveFocusModePostFx();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Set focus highlight fade parameters
class CRenderCommand_UpdateFocusHighlightFading : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateFocusHighlightFading, "UpdateFocusHighlightFading" );

private:
	Float m_fadeNear;
	Float m_fadeFar;
	Float m_dimmingTime;
	Float m_dimmingSpeed;
	
public:
	/// Set up fading parameters for the focus highlight. For simplicity, these are set directly with no transitions.
	///
	/// fadeNear:		Distance from camera where fading starts
	/// fadeFar:		Distance from camera where faded out entirely
		
	CRenderCommand_UpdateFocusHighlightFading( Float fadeNear, Float fadeFar, Float dimmingTime, Float dimmingSpeed );
	~CRenderCommand_UpdateFocusHighlightFading();
	virtual void Execute();
};

class CRenderCommand_SetDimmingFocusMode : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetDimmingFocusMode, "SetDimmingFocusMode" );

	Bool m_dimming;

public:
	CRenderCommand_SetDimmingFocusMode( Bool dimming );
	~CRenderCommand_SetDimmingFocusMode();
	virtual void Execute();
};

class CRenderCommand_EnableExtendedFocusModePostFx : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_EnableExtendedFocusModePostFx, "EnableExtendedFocusModePostFx" );

	Float m_fadeInTime;

public:
	CRenderCommand_EnableExtendedFocusModePostFx( Float fadeInTime );
	~CRenderCommand_EnableExtendedFocusModePostFx();
	virtual void Execute();
};

class CRenderCommand_DisableExtendedFocusModePostFx : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_DisableExtendedFocusModePostFx, "DisableExtendedFocusModePostFx" );

	Float m_fadeOutTime;

public:
	CRenderCommand_DisableExtendedFocusModePostFx( Float fadeOutTime );
	~CRenderCommand_DisableExtendedFocusModePostFx();
	virtual void Execute();
};

class CRenderCommand_FocusModeSetPlayerPosition : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_FocusModeSetPlayerPosition, "FocusModeSetPlayerPosition" );

	Vector	m_position;

public:
	CRenderCommand_FocusModeSetPlayerPosition( const Vector& position );
	~CRenderCommand_FocusModeSetPlayerPosition();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////
/// Set CatView Effect FX 

class CRenderCommand_EnableCatViewPostFx : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_EnableCatViewPostFx, "EnableCatViewPostFx" );

	Float m_fadeInTime;

public:
	CRenderCommand_EnableCatViewPostFx( Float fadeInTime );
	~CRenderCommand_EnableCatViewPostFx();
	virtual void Execute();
};

class CRenderCommand_DisableCatViewPostFx : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_DisableCatViewPostFx, "DisableCatViewPostFx" );

	Float m_fadeOutTime;

public:
	CRenderCommand_DisableCatViewPostFx( Float fadeOutTime );
	~CRenderCommand_DisableCatViewPostFx();
	virtual void Execute();
};


class CRenderCommand_CatViewSetPlayerPosition : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_CatViewSetPlayerPosition, "CatViewSetPlayerPosition" );

	Vector	m_position;
	Bool	m_autoPositioning;

public:
	CRenderCommand_CatViewSetPlayerPosition( const Vector& position , Bool autoPositioning = false );
	~CRenderCommand_CatViewSetPlayerPosition();
	virtual void Execute();
};

class CRenderCommand_CatViewSetTintColors : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_CatViewSetTintColors, "CatViewSetPlayerPosition" );

	Vector	m_tintNear;
	Vector	m_tintFar;
	Float	m_desaturation;

public:
	CRenderCommand_CatViewSetTintColors( const Vector& tintNear , const Vector& tintFar, Float desaturation = 1.0f );
	~CRenderCommand_CatViewSetTintColors();
	virtual void Execute();
};

class CRenderCommand_CatViewSetBrightness : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_CatViewSetBrightness, "CatViewSetBrightness" );

	Float m_brightStrength;

public:
	CRenderCommand_CatViewSetBrightness( Float brightStrength );
	~CRenderCommand_CatViewSetBrightness();
	virtual void Execute();
};

class CRenderCommand_CatViewSetViewRange : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_CatViewSetViewRange, "CatViewSetViewRange" );

	Float m_viewRanger;

public:
	CRenderCommand_CatViewSetViewRange( Float viewRanger );
	~CRenderCommand_CatViewSetViewRange();
	virtual void Execute();
};

class CRenderCommand_CatViewSetPulseParams : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_CatViewSetPulseParams, "CatViewSetPulseParams" );

	Float m_base;
	Float m_scale;
	Float m_speed;

public:
	CRenderCommand_CatViewSetPulseParams( Float base, Float scale, Float speed );
	~CRenderCommand_CatViewSetPulseParams();
	virtual void Execute();
};

class CRenderCommand_CatViewSetHightlight : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_CatViewSetHightlight, "CatViewSetHightlight" );

	Vector	m_color;
	Float	m_hightlightInterior;
	Float	m_blurSize;

public:
	CRenderCommand_CatViewSetHightlight( Vector color , Float hightlightInterior , Float blurSize );
	~CRenderCommand_CatViewSetHightlight();
	virtual void Execute();
};

class CRenderCommand_CatViewSetFog : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_CatViewSetFog, "CatViewSetHightlight" );

	Float	m_fogDensity;
	Float	m_fogStartOffset;

public:
	CRenderCommand_CatViewSetFog( Float fogDensity , Float fogStartOffset );
	~CRenderCommand_CatViewSetFog();
	virtual void Execute();
};


////////////////////////////////////////////////////////////////////////////////////////////
/// Add gameplay post fx SURFACE FROST / BURN
class CRenderCommand_InitSurfacePostFx : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_InitSurfacePostFx, "InitSurfacePostFx" );

private:
	Vector m_fillColor;

public:	
	CRenderCommand_InitSurfacePostFx( const Vector& fillColor );
	~CRenderCommand_InitSurfacePostFx();
	virtual void Execute();
};

class CRenderCommand_AddSurfacePostFx : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_AddSurfacePostFx, "AddSurfacePostFx" );

private:
	Vector m_position;
	Float m_fadeInTime;
	Float m_fadeOutTime;
	Float m_activeTime;
	Float m_range;
	Uint32 m_type;

public:
	CRenderCommand_AddSurfacePostFx( const Vector& position, Float fadeInTime, Float fadeOutTime, Float activeTime, Float range, Uint32 type );
	~CRenderCommand_AddSurfacePostFx();
	virtual void Execute();
};

/// Add gameplay post fx
class CRenderCommand_AddSepiaPostFx : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_AddSepiaPostFx, "AddSepiaPostFx" );

private:
	Float m_fadeInTime;

public:
	CRenderCommand_AddSepiaPostFx( Float fadeInTime );
	~CRenderCommand_AddSepiaPostFx();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Remove gameplay post fx
class CRenderCommand_RemoveSepiaPostFx : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveSepiaPostFx, "RemoveSepiaPostFx" );

private:
	Float m_fadeOutTime;

public:
	CRenderCommand_RemoveSepiaPostFx( Float fadeOutTime );
	~CRenderCommand_RemoveSepiaPostFx();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Add gameplay post fx
class CRenderCommand_AddDrunkPostFx : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_AddDrunkPostFx, "AddDrunkPostFx" );

private:
	Float m_fadeInTime;

public:
	CRenderCommand_AddDrunkPostFx( Float fadeInTime );
	~CRenderCommand_AddDrunkPostFx();
	virtual void Execute();
};

/// Remove gameplay post fx
class CRenderCommand_RemoveDrunkPostFx : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveDrunkPostFx, "RemoveDrunkPostFx" );

private:
	Float m_fadeOutTime;

public:
	CRenderCommand_RemoveDrunkPostFx( Float fadeOutTime );
	~CRenderCommand_RemoveDrunkPostFx();
	virtual void Execute();
};

/// Remove gameplay post fx
class CRenderCommand_ScaleDrunkPostFx : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ScaleDrunkPostFx, "ScaleDrunkPostFx" );

private:
	Float m_scale;

public:
	CRenderCommand_ScaleDrunkPostFx( Float scale );
	~CRenderCommand_ScaleDrunkPostFx();
	virtual void Execute();
};


////////////////////////////////////////////////////////////////////////////////////////////

/// Full screen fade out
class CRenderCommand_ScreenFadeOut : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ScreenFadeOut, "ScreenFadeOut" );

public:
	Color		m_color;		//!< Fadeout color
	Float		m_time;			//!< Time

public:
	CRenderCommand_ScreenFadeOut( const Color& color, Float time );
	~CRenderCommand_ScreenFadeOut();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Full screen fade in
class CRenderCommand_ScreenFadeIn : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ScreenFadeIn, "ScreenFadeIn" );

public:
	Float		m_time;			//!< Time

public:
	CRenderCommand_ScreenFadeIn( Float time );
	~CRenderCommand_ScreenFadeIn();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Sets full screen fade in and keeps it
class CRenderCommand_SetScreenFade : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetScreenFade, "SetScreenFade" );

public:
	Bool m_isIn;
	Float m_progress;
	Color m_color;

public:
	CRenderCommand_SetScreenFade( Bool isIn, Float progress, const Color& color );
	virtual void Execute();
};
////////////////////////////////////////////////////////////////////////////////////////////

/// Full screen fade in
class CRenderCommand_UpdateFadeParameters : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateFadeParameters, "UpdateFadeParameters" );

public:
	Float		m_deltaTime;			//!< Time delta for update

public:
	CRenderCommand_UpdateFadeParameters( Float deltaTime );
	~CRenderCommand_UpdateFadeParameters();
	virtual void Execute();
};

////////////////////////////////////////////////////////////////////////////////////////////

/// Change scene forced LOD
class CRenderCommand_ChangeSceneForcedLOD : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ChangeSceneForcedLOD, "ChangeForcedLOD" );

public:
	IRenderScene*		m_scene;
	Int32					m_forceLOD;

public:
	CRenderCommand_ChangeSceneForcedLOD( IRenderScene* scene, Int32 forceLOD );
	~CRenderCommand_ChangeSceneForcedLOD();
	virtual void Execute();
};

//////////////////////////////////////////////////////////////////////////

// Suspend rendering
class CRenderCommand_SuspendRendering : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SuspendRendering, "SuspendRendering" );

public:
	CRenderCommand_SuspendRendering() {};
	virtual void Execute();
};

// Resume rendering
class CRenderCommand_ResumeRendering : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ResumeRendering, "ResumeRendering" );

public:
	CRenderCommand_ResumeRendering() {};
	virtual void Execute();
};

#ifndef NO_EDITOR
//////////////////////////////////////////////////////////////////////////
// Those commands purpose is to make particles editor keep simulating as long as possible, without simulation restart.

// Update emitter, meaning any change other than adding/removing emitter
class CRenderCommand_UpdateCreateParticleEmitter : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateCreateParticleEmitter, "UpdateCreateParticleEmitter" );

	IRenderProxy*					m_renderProxy;
	IRenderResource*				m_emitter;
	IRenderResource*				m_material;
	IRenderResource*				m_materialParameters;
	EEnvColorGroup					m_envColorGroup;

public:
	CRenderCommand_UpdateCreateParticleEmitter( IRenderProxy* proxy, IRenderResource* emitter, IRenderResource* material, IRenderResource* materialParameters, EEnvColorGroup envColorGroup );
	~CRenderCommand_UpdateCreateParticleEmitter();
	virtual void Execute();
};

// Remove emitter (emitter has been removed from the emitters graph)
class CRenderCommand_RemoveParticleEmitter : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveParticleEmitter, "RemoveParticleEmitter" );

	IRenderProxy*					m_renderProxy;
	Int32								m_uniqueId;

public:
	CRenderCommand_RemoveParticleEmitter( IRenderProxy* proxy, Int32 uniqueId );
	~CRenderCommand_RemoveParticleEmitter();
	virtual void Execute();
};

#endif

// update srender settings from world scene debugger
class CRenderCommand_UpdateFoliageRenderParams : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateFoliageRenderParams, "UpdateFoliageRenderSettings" );

public:
	IRenderObject*			m_proxy;
	SFoliageRenderParams	m_foliageSRenderParams;

public:
	CRenderCommand_UpdateFoliageRenderParams( IRenderObject* ro, SFoliageRenderParams params );
	~CRenderCommand_UpdateFoliageRenderParams();
	virtual void Execute();
};

/// Add speed tree rendering proxy to rendering scene
class CRenderCommand_AddSpeedTreeProxyToScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_AddSpeedTreeProxyToScene, "AddSpeedTreeProxyToScene" );

public:
	IRenderScene*			m_scene;
	IRenderObject*			m_proxy;

public:
	CRenderCommand_AddSpeedTreeProxyToScene( IRenderScene* scene, IRenderObject* proxy );
	~CRenderCommand_AddSpeedTreeProxyToScene();
	virtual void Execute();
};

/// Remove speed tree rendering proxy from rendering scene
class CRenderCommand_RemoveSpeedTreeProxyFromScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveSpeedTreeProxyFromScene, "RemoveSpeedTreeProxyFromScene" );

public:
	IRenderScene*			m_scene;
	IRenderObject*			m_proxy;

public:
	CRenderCommand_RemoveSpeedTreeProxyFromScene( IRenderScene* scene, IRenderObject* proxy );
	~CRenderCommand_RemoveSpeedTreeProxyFromScene();
	virtual void Execute();
};

class CRenderCommand_UpdateSpeedTreeInstances : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateSpeedTreeInstances, "UpdateSpeedTreeInstances" );
	
	RenderObjectHandle m_speedTreeProxy;
	SFoliageUpdateRequest m_updates;

public:

	CRenderCommand_UpdateSpeedTreeInstances( RenderObjectHandle	speedTreeProxy, SFoliageUpdateRequest updates );
	~CRenderCommand_UpdateSpeedTreeInstances();
	virtual void Execute();
};

// Upload speed tree instance to the renderer
class CRenderCommand_CreateSpeedTreeInstances : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_CreateSpeedTreeInstances, "CreateSpeedTreeInstance" );

	RenderObjectHandle			m_speedTreeProxy;
	RenderObjectHandle			m_baseTree;
	FoliageInstanceContainer	m_instancesData;
	Box m_box;

public:
	CRenderCommand_CreateSpeedTreeInstances( RenderObjectHandle	speedTreeProxy, RenderObjectHandle baseTree, const FoliageInstanceContainer & instancesData, const Box& rect );
	~CRenderCommand_CreateSpeedTreeInstances();
	virtual void Execute();
};

// Upload speed tree instance to the renderer
class CRenderCommand_CreateSpeedTreeDynamicInstances : public CRenderCommand_CreateSpeedTreeInstances
{
	DECLARE_RENDER_COMMAND( CRenderCommand_CreateSpeedTreeDynamicInstances, "CreateSpeedTreeDynamicInstance" );

public:
	CRenderCommand_CreateSpeedTreeDynamicInstances( RenderObjectHandle	speedTreeProxy, RenderObjectHandle baseTree, const FoliageInstanceContainer & instancesData, const Box& rect );
	~CRenderCommand_CreateSpeedTreeDynamicInstances();
	virtual void Execute();
};

// Remove speed tree instances from the renderer
class CRenderCommand_RemoveSpeedTreeInstancesRadius : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveSpeedTreeInstancesRadius, "RemoveSpeedTreeInstancesRadius" );

	RenderObjectHandle		m_speedTreeProxy;
	RenderObjectHandle		m_baseTree;
	Vector3					m_position;
	Float					m_radius;

public:
	CRenderCommand_RemoveSpeedTreeInstancesRadius( RenderObjectHandle speedTreeProxy, RenderObjectHandle baseTree, const Vector3& position, Float radius );
	~CRenderCommand_RemoveSpeedTreeInstancesRadius();
	virtual void Execute();
};

// Remove speed tree instances from the renderer
class CRenderCommand_RemoveSpeedTreeDynamicInstancesRadius : public CRenderCommand_RemoveSpeedTreeInstancesRadius
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveSpeedTreeDynamicInstancesRadius, "RemoveSpeedTreeDynamicInstancesRadius" );

public:
	CRenderCommand_RemoveSpeedTreeDynamicInstancesRadius( RenderObjectHandle speedTreeProxy, RenderObjectHandle baseTree, const Vector3& position, Float radius );
	~CRenderCommand_RemoveSpeedTreeDynamicInstancesRadius();
	virtual void Execute();
};

// Remove speed tree instances from the renderer
class CRenderCommand_RemoveSpeedTreeInstancesRect : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveSpeedTreeInstancesRect, "RemoveSpeedTreeInstancesRect" );

	RenderObjectHandle		m_speedTreeProxy;
	RenderObjectHandle		m_baseTree;
	Box						m_rect;

public:
	CRenderCommand_RemoveSpeedTreeInstancesRect( RenderObjectHandle speedTreeProxy, RenderObjectHandle baseTree, const Box& rect );
	~CRenderCommand_RemoveSpeedTreeInstancesRect();
	virtual void Execute();
};

// Refresh terrain-based grass generation
class CRenderCommand_RefreshGenericGrass : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RefreshGenericGrass, "RefreshGenericGrass" );

	RenderObjectHandle			m_speedTreeProxy;

public:
	CRenderCommand_RefreshGenericGrass( RenderObjectHandle speedTreeProxy );
	~CRenderCommand_RefreshGenericGrass();
	virtual void Execute();
};

// Update grass colliders
class CRenderCommand_UpdateDynamicGrassColissions : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateDynamicGrassColissions, "UpdateDynamicGrassColissions" );

	RenderObjectHandle					m_speedTreeProxy;
	TDynArray< SDynamicCollider >		m_collisionsPos;

public:
	CRenderCommand_UpdateDynamicGrassColissions( RenderObjectHandle speedTreeProxy, const TDynArray< SDynamicCollider >& collisions );
	~CRenderCommand_UpdateDynamicGrassColissions();
	virtual void Execute();
};

// Refresh terrain-based grass generation
class CRenderCommand_UpdateGenericGrassMask : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateGenericGrassMask, "UpdateGenericGrassMask" );

	IRenderProxy*			m_terrainProxy;
	Uint8*					m_grassMask;
	Uint32					m_grassMaskRes;

public:
	CRenderCommand_UpdateGenericGrassMask( IRenderProxy* terrainProxy, Uint8* grassMaskUpdate, Uint32 grassMaskResUpdate );
	~CRenderCommand_UpdateGenericGrassMask();
	virtual void Execute();
};

// Refresh terrain-based grass generation
class CRenderCommand_UploadGenericGrassOccurrenceMap : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UploadGenericGrassOccurrenceMap, "UploadGenericGrassOccurrenceMap" );

	RenderObjectHandle		m_proxy;
	TDynArray< CGrassCellMask >	m_cellMasks;

public:
	CRenderCommand_UploadGenericGrassOccurrenceMap( RenderObjectHandle proxy, const TDynArray< CGrassCellMask >& cellMasks );
	~CRenderCommand_UploadGenericGrassOccurrenceMap();
	virtual void Execute();
};

// Refresh speedtree render budgets
class CRenderCommand_UpdateFoliageBudgets : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateFoliageBudgets, "UpdateFoliageBudgets" );
	IRenderProxy*			m_speedTreeProxy;
	Float					m_grassInstancesPerSqM;
	Float					m_treeInstancesPerSqM;
	Float					m_grassLayersPerSqM;

public:
	CRenderCommand_UpdateFoliageBudgets( IRenderProxy* speedTreeProxy, Float grassInstancesPerSqM, Float treeInstancesPerSqM, Float grassLayersPerSqM );
	~CRenderCommand_UpdateFoliageBudgets();
	virtual void Execute();
};

enum EFoliageVisualisationMode : Uint32
{
	VISUALISE_NONE,
	VISUALISE_GRASSINSTANCES,
	VISUALISE_TREEINSTANCES,
	VISUALISE_GRASSLAYERS,
};

// Switch speedtree density visualiser
class CRenderCommand_SetFoliageVisualisation : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetFoliageVisualisation, "SetFoliageVisualisation" );
	IRenderProxy*			m_speedTreeProxy;

public:
	
	CRenderCommand_SetFoliageVisualisation( IRenderProxy* speedTreeProxy, EFoliageVisualisationMode mode );
	~CRenderCommand_SetFoliageVisualisation();
	virtual void Execute();

private:
	EFoliageVisualisationMode m_mode;
};

class CRenderCommand_SetupTreeFading : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetupTreeFading, "SetupTreeFading" );
	Bool	m_enable;

public:
	CRenderCommand_SetupTreeFading( Bool enable );
	~CRenderCommand_SetupTreeFading();
	virtual void Execute();
};

class CRenderCommand_SetupCachets : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetupCachets, "SetupCachets" );
	Bool	m_enable;

public:
	CRenderCommand_SetupCachets( Bool enable );
	~CRenderCommand_SetupCachets();
	virtual void Execute();
};

class CRenderCommand_SetTreeFadingReferencePoints : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetTreeFadingReferencePoints, "SetTreeFadingReferencePoints" );
	IRenderProxy*	m_speedTreeProxy;
	Vector			m_leftReference;
	Vector			m_rightReference;
	Vector			m_centerReference;

public:
	CRenderCommand_SetTreeFadingReferencePoints( IRenderProxy* speedTreeProxy, const Vector& left, const Vector& right, const Vector& center );
	~CRenderCommand_SetTreeFadingReferencePoints();
	virtual void Execute();
};


class CRenderCommand_UpdateClipmap : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateClipmap, "UpdateClipmap" );

	IRenderProxy*		m_terrainProxy;
	IRenderObject*		m_update;

	CRenderCommand_UpdateClipmap( IRenderProxy* terrainProxy, IRenderObject* update );
	~CRenderCommand_UpdateClipmap();
	virtual void Execute();
};

class CRenderCommand_UpdateGrassSetup : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateGrassSetup, "UpdateGrassSetup" );

	IRenderProxy*		m_terrainProxy;
	IRenderObject*		m_vegetationProxy;
	IRenderObject*		m_update;

	CRenderCommand_UpdateGrassSetup( IRenderProxy* terrainProxy, IRenderObject* vegetationProxy, IRenderObject* update );
	~CRenderCommand_UpdateGrassSetup();
	virtual void Execute();
};

class CRenderCommand_SetTerrainCustomOverlay : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetTerrainCustomOverlay, "SetTerrainCustomOverlay" );

	IRenderProxy*		m_terrainProxy;
	TDynArray< Uint32 >	m_data;
	Uint32				m_width;
	Uint32				m_height;

public:
	CRenderCommand_SetTerrainCustomOverlay( IRenderProxy* terrainProxy, const TDynArray< Uint32 >& data, Uint32 width, Uint32 height );
	~CRenderCommand_SetTerrainCustomOverlay();
	virtual void Execute();
};

class CRenderCommand_ClearTerrainCustomOverlay : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ClearTerrainCustomOverlay, "ClearTerrainCustomOverlay" );

	IRenderProxy*		m_terrainProxy;

public:
	CRenderCommand_ClearTerrainCustomOverlay( IRenderProxy* terrainProxy );
	~CRenderCommand_ClearTerrainCustomOverlay();
	virtual void Execute();
};

/// Add terrain rendering proxy to rendering scene
class CRenderCommand_SetTerrainProxyToScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetTerrainProxyToScene, "SetTerrainProxyToScene" );

public:
	IRenderScene*			m_scene;
	IRenderProxy*			m_proxy;

public:
	CRenderCommand_SetTerrainProxyToScene( IRenderScene* scene, IRenderProxy* proxy );
	~CRenderCommand_SetTerrainProxyToScene();
	virtual void Execute();
};

/// Remove terrain rendering proxy from rendering scene
class CRenderCommand_RemoveTerrainProxyFromScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveTerrainProxyFromScene, "RemoveTerrainProxyFromScene" );

public:
	IRenderScene*			m_scene;
	IRenderProxy*			m_proxy;

public:
	CRenderCommand_RemoveTerrainProxyFromScene( IRenderScene* scene, IRenderProxy* proxy );
	~CRenderCommand_RemoveTerrainProxyFromScene();
	virtual void Execute();
};

class CRenderCommand_UpdateTerrainWaterLevels : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateTerrainWaterLevels, "UpdateTerrainWaterLevels" );

public:
	IRenderProxy*		m_terrainProxy;
	TDynArray< Float >	m_minWaterLevels;

public:
	CRenderCommand_UpdateTerrainWaterLevels( IRenderProxy* terrainProxy, const TDynArray< Float >& minWaterLevels );
	~CRenderCommand_UpdateTerrainWaterLevels();
	virtual void Execute();
};

class CRenderCommand_UpdateTerrainTileHeightRanges : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateTerrainTileHeightRanges, "UpdateTerrainTileHeightRanges" );

public:
	IRenderProxy*			m_terrainProxy;
	TDynArray< Vector2 >	m_tileHeightRanges;

public:
	CRenderCommand_UpdateTerrainTileHeightRanges( IRenderProxy* terrainProxy, const TDynArray< Vector2 >& tileHeightRanges );
	~CRenderCommand_UpdateTerrainTileHeightRanges();
	virtual void Execute();
};

class CRenderCommand_SetupEnvironmentElementsVisibility : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetupEnvironmentElementsVisibility, "SetupTerrainVisibility" );
	Bool		  m_terrainVisible;
	Bool		  m_foliageVisible;
	Bool		  m_waterVisible;
	IRenderScene* m_scene;

public:
	CRenderCommand_SetupEnvironmentElementsVisibility( IRenderScene* scene, Bool terrainVisible, Bool foliageVisible, Bool waterVisible );
	~CRenderCommand_SetupEnvironmentElementsVisibility();
	virtual void Execute();
};

/// Add water proxy to rendering scene
class CRenderCommand_SetWaterProxyToScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetWaterProxyToScene, "SetWaterProxyToScene" );

public:
	IRenderScene*			m_scene;
	IRenderProxy*			m_proxy;

public:
	CRenderCommand_SetWaterProxyToScene( IRenderScene* scene, IRenderProxy* proxy );
	~CRenderCommand_SetWaterProxyToScene();
	virtual void Execute();
};

/// Remove water proxy from rendering scene
class CRenderCommand_RemoveWaterProxyFromScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveWaterProxyFromScene, "RemoveWaterProxyFromScene" );

public:
	IRenderScene*			m_scene;
	IRenderProxy*			m_proxy;

public:
	CRenderCommand_RemoveWaterProxyFromScene( IRenderScene* scene, IRenderProxy* proxy );
	~CRenderCommand_RemoveWaterProxyFromScene();
	virtual void Execute();
};

// updating water textures
class CRenderCommand_UpdateWaterProxy : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateWaterProxy, "UpdateWaterProxy" );

	IRenderProxy*		m_waterProxy;
	IRenderObject*		m_textureArray;		

	CRenderCommand_UpdateWaterProxy( IRenderProxy* waterProxy, IRenderObject* textureArray );
	~CRenderCommand_UpdateWaterProxy();
	virtual void Execute();
};

// updating water simulation state (height map)
class CGlobalWaterUpdateParams;
class CRenderCommand_SimulateWaterProxy : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SimulateWaterProxy, "SimulateWaterProxy" );

	IRenderProxy*				m_waterProxy;
	CGlobalWaterUpdateParams*	m_waterParams;

	CRenderCommand_SimulateWaterProxy( IRenderProxy* waterProxy, CGlobalWaterUpdateParams* waterParams );
	~CRenderCommand_SimulateWaterProxy();
	virtual void Execute();
};

/// Add local water shape
class CLocalWaterShapesParams;
class CRenderCommand_AddWaterProxyLocalShape : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_AddWaterProxyLocalShape, "AddWaterProxyLocalShape" );

	IRenderProxy*				m_waterProxy;
	CLocalWaterShapesParams*	m_shapesParams;

	CRenderCommand_AddWaterProxyLocalShape( IRenderProxy* waterProxy, CLocalWaterShapesParams* shapes );
	~CRenderCommand_AddWaterProxyLocalShape();
	virtual void Execute();
};

/// Setup skybox
class CRenderCommand_SkyboxSetup : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SkyboxSetup, "SkyboxSetup" );

public:
	IRenderScene *m_scene;
	SSkyboxSetupParameters m_params;

public:
	CRenderCommand_SkyboxSetup( IRenderScene *scene, const SSkyboxSetupParameters &setupParams );
	virtual ~CRenderCommand_SkyboxSetup();
	virtual void Execute();
};

/// Setup lens flare
class CRenderCommand_LensFlareSetup : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_LensFlareSetup, "LensFlareSetup" );

public:
	IRenderScene *m_scene;
	SLensFlareGroupsSetupParameters m_params;

public:
	CRenderCommand_LensFlareSetup( IRenderScene *scene, const SLensFlareGroupsSetupParameters &setupParams );
	virtual ~CRenderCommand_LensFlareSetup();
	virtual void Execute();
};

// When viewport / window size (or mode) is changed, commit this command, so the renderer can properly respond
class CRenderCommand_HandleResizeEvent : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_HandleResizeEvent, "HandleResizeEvent" );

public:
	CRenderCommand_HandleResizeEvent( Uint32 width, Uint32 height );
	virtual void Execute();

private:
	Uint32 m_width;
	Uint32 m_height;

};

// Change the entity group hires shadow flags
class CRenderCommand_SetEntityGroupHiResShadows : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetEntityGroupHiResShadows, "SetEntityGroupHiResShadows" );

public:
	IRenderEntityGroup*		m_group;
	Bool					m_flag;

public:
	CRenderCommand_SetEntityGroupHiResShadows( IRenderEntityGroup* group, Bool flag );
	~CRenderCommand_SetEntityGroupHiResShadows();
	virtual void Execute();
};

// Change the entity group shadow flags
class CRenderCommand_SetEntityGroupShadows : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetEntityGroupShadows, "SetEntityGroupShadows" );

public:
	IRenderEntityGroup*		m_group;
	Bool					m_flag;

public:
	CRenderCommand_SetEntityGroupShadows( IRenderEntityGroup* group, Bool flag );
	~CRenderCommand_SetEntityGroupShadows();
	virtual void Execute();
};

// Bind/Unbind rendering proxy from entity group (used for dynamic entities like inventory)
class CRenderCommand_BindEntityGroupToProxy : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_BindEntityGroupToProxy, "BindEntityGroupToProxy" );

public:
	IRenderEntityGroup*		m_group;
	IRenderProxy*			m_proxy;

public:
	CRenderCommand_BindEntityGroupToProxy( IRenderEntityGroup* group, IRenderProxy*	proxy );
	~CRenderCommand_BindEntityGroupToProxy();
	virtual void Execute();
};

#ifdef USE_APEX
namespace physx { namespace apex { class NxApexRenderable; } }

class CRenderCommand_UpdateApexRenderable : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateApexRenderable, "UpdateApexRenderable" );

public:
	IRenderProxy*					m_proxy;
	physx::apex::NxApexRenderable*	m_renderable;
	Box								m_boundingBox;
	Matrix							m_localToWorld;
	Float							m_wetness;

public:
	CRenderCommand_UpdateApexRenderable( IRenderProxy* proxy, physx::apex::NxApexRenderable* renderable, const Box& bbox, const Matrix& l2w, Float w );
	~CRenderCommand_UpdateApexRenderable();
	virtual void Execute();
};
#endif



class CRenderCommand_UpdateTerrainShadows : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateTerrainShadows, "UpdateTerrainShadows" );

protected:
	IRenderScene*	m_renderScene;

public:
	CRenderCommand_UpdateTerrainShadows( IRenderScene* renderScene );
	~CRenderCommand_UpdateTerrainShadows();
	virtual void Execute();
};


class CRenderCommand_SetProxyLightChannels : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetProxyLightChannels, "SetProxyLightChannels" );

protected:
	IRenderProxy*	m_proxy;
	Uint8			m_lightChannels;
	Uint8			m_mask;

public:
	CRenderCommand_SetProxyLightChannels( IRenderProxy* proxy, Uint8 lightChannels, Uint8 mask );
	~CRenderCommand_SetProxyLightChannels();
	virtual void Execute();
};

class CRenderCommand_UpdateMorphRatio : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateMorphRatio, "UpdateMorphRatio" );

protected:
	IRenderProxy*	m_proxy;
	Float			m_ratio;

public:
	CRenderCommand_UpdateMorphRatio( IRenderProxy* proxy, Float ratio );
	~CRenderCommand_UpdateMorphRatio();
	virtual void Execute();
};

class CRenderCommand_SetClippingEllipseMatrix : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetClippingEllipseMatrix, "SetClippingEllipseMatrix" );

protected:
	IRenderProxy*	m_proxy;
	Matrix			m_meshToEllipse;

public:
	CRenderCommand_SetClippingEllipseMatrix( IRenderProxy* proxy, const Matrix& meshToEllipse );
	~CRenderCommand_SetClippingEllipseMatrix();
	virtual void Execute();
};

class CRenderCommand_ClearClippingEllipseMatrix : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_ClearClippingEllipseMatrix, "ClearClippingEllipseMatrix" );

protected:
	IRenderProxy*	m_proxy;

public:
	CRenderCommand_ClearClippingEllipseMatrix( IRenderProxy* proxy );
	~CRenderCommand_ClearClippingEllipseMatrix();
	virtual void Execute();
};



class CRenderCommand_AddDynamicDecalToScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_AddDynamicDecalToScene, "AddDynamicDecalToScene" );

public:
	IRenderScene*		m_scene;
	IRenderResource*	m_decal;
	Bool				m_projectOnlyOnStatic;

public:
	CRenderCommand_AddDynamicDecalToScene( IRenderScene* scene, IRenderResource* decal, Bool projectOnlyOnStatic = false );
	~CRenderCommand_AddDynamicDecalToScene();
	virtual void Execute();
};

// Like CRenderCommand_AddDynamicDecalToScene, except it only spawns the decal on the specified render proxies.
class CRenderCommand_AddDynamicDecalToSceneForProxies : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_AddDynamicDecalToSceneForProxies, "AddDynamicDecalToSceneForProxies" );

public:
	IRenderScene*				m_scene;
	IRenderResource*			m_decal;
	TDynArray< IRenderProxy* >	m_targetProxies;

public:
	CRenderCommand_AddDynamicDecalToSceneForProxies( IRenderScene* scene, IRenderResource* decal, const TDynArray< IRenderProxy* >& proxies );
	~CRenderCommand_AddDynamicDecalToSceneForProxies();
	virtual void Execute();
};

class CRenderCommand_RemoveDynamicDecalFromScene : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_RemoveDynamicDecalFromScene, "RemoveDynamicDecalFromScene" );

public:
	IRenderScene*			m_scene;
	IRenderResource*		m_decal;

public:
	CRenderCommand_RemoveDynamicDecalFromScene( IRenderScene* scene, IRenderResource* decal );
	~CRenderCommand_RemoveDynamicDecalFromScene();
	virtual void Execute();
};

class CRenderCommand_UpdateStripeProperties : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateStripeProperties, "UpdateStripeProperties" );

protected:
	IRenderProxy*							m_proxy;
	struct SRenderProxyStripeProperties*	m_properties;

public:
	CRenderCommand_UpdateStripeProperties( IRenderProxy* proxy, struct SRenderProxyStripeProperties* properties );
	~CRenderCommand_UpdateStripeProperties();
	virtual void Execute();
};

class CRenderCommand_UpdateTerrainScreenSpaceErrorThreshold : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateTerrainScreenSpaceErrorThreshold, "UpdateTerrainScreenSpaceErrorThreshold" );

protected:
	IRenderProxy*	m_proxy;
	Float			m_terrainScreenSpaceErrorThreshold;

public:
	// Use < 0 to revert to global default
	CRenderCommand_UpdateTerrainScreenSpaceErrorThreshold( IRenderProxy* proxy, Float screenSpaceErrorThreshold );
	~CRenderCommand_UpdateTerrainScreenSpaceErrorThreshold();
	virtual void Execute();
};

class CRenderCommand_SetParticlePriority : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_SetParticlePriority, "SetParticlePriority" );

protected:
	IRenderProxy*	m_proxy;
	Uint8			m_priority;

public:
	CRenderCommand_SetParticlePriority( IRenderProxy* proxy, Uint8 priority );
	~CRenderCommand_SetParticlePriority();
	virtual void Execute();
};

/// update swarm data
class CRenderCommand_UpdateSwarmData : public IRenderCommand
{
	DECLARE_RENDER_COMMAND( CRenderCommand_UpdateSwarmData, "UpdateSwarmData" );

public:
	IRenderProxy*		m_proxy;
	IRenderSwarmData*	m_data;
	Uint32				m_numBoids;

public:
	CRenderCommand_UpdateSwarmData( IRenderProxy* proxy, IRenderSwarmData* data, Uint32 numBoids );
	~CRenderCommand_UpdateSwarmData();
	virtual void Execute();
};
