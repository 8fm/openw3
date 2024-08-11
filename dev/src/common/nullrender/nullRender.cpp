/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "nullRender.h"
#include "../engine/platformViewport.h"
#include "../core/functionBuilder.h"
#include "../core/scriptStackFrame.h"

Float GRenderSettingsMipBias = 0.f;

// Required to build wcc
namespace Debug
{
	Int32			GRenderTargetZoomShift	= 0;
	Int32			GRenderTargetOffsetX	= 0;
	Int32			GRenderTargetOffsetY	= 0;
}

extern IRender* SCreateRender( IPlatformViewport* )
{
	return new CNullRender();
}

CNullRender::CNullRender()
{
}
	
Bool CNullRender::Init()
{
	return true;
}

void CNullRender::Tick( float timeDelta )
{
}


Bool CNullRender::PrepareRenderer()
{
	return true;
}


void CNullRender::Flush()
{
}


void CNullRender::ShutdownTextureStreaming()
{
}


ViewportHandle CNullRender::CreateViewport( void* TopLevelWindow, void* ParentWindow, const String& title, Uint32 width, Uint32 height, EViewportWindowMode windowMode, Bool vsync )
{
	return ViewportHandle();
}

ViewportHandle CNullRender::CreateGameViewport( const String& title, Uint32 width, Uint32 height, EViewportWindowMode windowMode )
{
	return ViewportHandle();
}

IRenderVisibilityExclusion* CNullRender::CreateVisibilityExclusion( const class GlobalVisID* ids, const Uint32 numIDs, const Uint8 renderMask, const Bool isEnabled )
{
	return nullptr;
}

//! Create gameplay render target for offscreen rendering
IRenderGameplayRenderTarget* CNullRender::CreateGameplayRenderTarget( const AnsiChar* tag )
{
	return nullptr;
}

void CNullRender::RequestResizeRenderSurfaces( Uint32 width, Uint32 height )
{
}

void CNullRender::RenderFrame( CRenderFrame* frame )
{
}

void CNullRender::RenderFragments( CRenderFrame* frame, ERenderingSortGroup sortGroup, const class RenderingContext& context )
{
}

void CNullRender::RenderFragments( CRenderFrame* frame, ERenderingSortGroup sortGroup, const class RenderingContext& context, Uint32 firstFragIndex, Uint32 numFragments )
{
}

void CNullRender::RenderFragments( CRenderFrame* frame, Int32 sortGroupsCount, const ERenderingSortGroup *sortGroups, const class RenderingContext& context )
{
}

IRenderResource* CNullRender::UploadTexture( const CBitmapTexture* texture )
{
	return nullptr;
}

IRenderResource* CNullRender::UploadMesh( const CMesh* mesh )
{
	return nullptr;
}

IRenderResource* CNullRender::UploadFurMesh( const CFurMeshResource* fur )
{
	return nullptr;
}

IRenderResource* CNullRender::UploadMaterial( const IMaterial* material )
{
	return nullptr;
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
void CNullRender::ForceMaterialRecompilation( const IMaterial* material )
{
}
#endif

#ifndef NO_ASYNCHRONOUS_MATERIALS
void CNullRender::FlushRecompilingMaterials()
{
}
#endif // NO_ASYNCHRONOUS_MATERIALS

IRenderResource* CNullRender::UploadCube( const CCubeTexture* texture )
{
	return nullptr;
}

IRenderResource* CNullRender::UploadEnvProbe( const CEnvProbeComponent* envProbeComponent )
{
	return nullptr;
}

IEnvProbeDataSource* CNullRender::CreateEnvProbeDataSource( CEnvProbeComponent &envProbeComponent )
{
	return nullptr;
}

IRenderResource* CNullRender::UploadTextureArray( const CTextureArray* texture )
{
	return nullptr;
}

IRenderThread* CNullRender::GetRenderThread()
{
	return nullptr;
}

Bool CNullRender::IsMSAAEnabled( const CRenderFrameInfo &frameInfo ) const
{
	return false;
}

Uint32 CNullRender::GetEnabledMSAALevel( const CRenderFrameInfo &frameInfo ) const
{
	return 1;
}

void CNullRender::ReloadTextures()
{
}

void CNullRender::ReloadEngineShaders()
{
}

//dex++: Recreate resources related to shadow system
void CNullRender::RecreateShadowmapResources()
{
}
//dex--

void CNullRender::RecalculateTextureStreamingSettings()
{

}

void CNullRender::ReloadSimpleShaders()
{
}

void CNullRender::RecreatePlatformResources()
{
}

IRenderResource* CNullRender::UploadDebugMesh( const TDynArray< DebugVertex >& vertices, const TDynArray< Uint32 >& indices )
{
	return nullptr;
}

#ifndef RED_FINAL_BUILD
GeneralStats CNullRender::GetGeneralMeshStats( GeneralStats& st )
{
	return st;
}
GeneralStats CNullRender::GetGeneralTextureStats( GeneralStats& st )
{
	return st;
}
#endif

const GpuApi::MeshStats* CNullRender::GetMeshStats()
{
	return nullptr;
}

const GpuApi::TextureStats* CNullRender::GetTextureStats()
{
	return nullptr;
}

#ifndef NO_DEBUG_WINDOWS
Int8 CNullRender::GetTextureStreamedMipIndex( const IRenderResource* resource )
{
	return -1;
}
#endif

ILoadingScreenFence* CNullRender::CreateLoadingScreenFence( const SLoadingScreenFenceInitParams& initParams )
{
	return nullptr;
}

IRenderVideo* CNullRender::CreateVideo( CName videoClient, const SVideoParams& videoParams ) const
{
	return nullptr;
}

// Methods used by importers - added because importers may also be useful during cooking process

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
Uint8* GrabRenderSurfacesThumbnail( const GpuApi::Rect* srcRect, Uint32& size )
{
	return nullptr;
}
#endif

IMaterial* GRenderProxyGlobalMaterial = nullptr;

void RegisterRendererDebugWindows()
{
	/* intentionally empty */
}

static void funcEnableCatView( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, fadeInTime, 1.0f );
	FINISH_PARAMETERS;

	//! NOP
}

static void funcDisableCatView( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, fadeOutTime, 1.0f );
	FINISH_PARAMETERS;

	//! NOP
}

static void funcSetPositionCatView( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position , Vector::ZEROS );
	GET_PARAMETER_OPT( Bool, autoPositioning, false );
	FINISH_PARAMETERS;

	//! NOP
}

static void funcSetTintColorsCatView(IScriptable* context,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, tintNear , Vector::ZEROS );
	GET_PARAMETER( Vector, tintFar , Vector::ZEROS );
	FINISH_PARAMETERS;

	//! NOP
}

static void funcSetBrightnessCatView( IScriptable* context,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, brightStrength, 1.0f );
	FINISH_PARAMETERS;

	//! NOP
}

static void funcSetViewRangeCatView( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, viewRanger, 30.0f );
	FINISH_PARAMETERS;

	//! NOP
}

static void funcSetHightlightCatView( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, color, Vector::ZEROS );
	GET_PARAMETER_OPT( Float, hightlightInterior, 0.05f );
	GET_PARAMETER_OPT( Float, blurSize, 1.5f );
	FINISH_PARAMETERS;

	//! NOP
}

static void funcEnableDrunkFx( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, fadeInTime, 1.0f );
	FINISH_PARAMETERS;

	//! NOP
}

static void funcDisableDrunkFx( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, fadeOutTime, 1.0f );
	FINISH_PARAMETERS;

	//! NOP
}

static void funcScaleDrunkFx( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, scale , 1.0f );
	FINISH_PARAMETERS;

	//! NOP
}

static void funcSetFogDensityCatView( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, density, 0.0f );
	GET_PARAMETER_OPT( Float, startOffset, 0.0f );
	FINISH_PARAMETERS;

	//! NOP
}

void ExportGameplayFxFunctions()
{
	NATIVE_GLOBAL_FUNCTION( "EnableCatViewFx" , funcEnableCatView );
	NATIVE_GLOBAL_FUNCTION( "DisableCatViewFx" , funcDisableCatView );
	NATIVE_GLOBAL_FUNCTION( "SetPositionCatViewFx" , funcSetPositionCatView );
	NATIVE_GLOBAL_FUNCTION( "SetTintColorsCatViewFx" , funcSetTintColorsCatView );
	NATIVE_GLOBAL_FUNCTION( "SetBrightnessCatViewFx" , funcSetBrightnessCatView );
	NATIVE_GLOBAL_FUNCTION( "SetViewRangeCatViewFx" , funcSetViewRangeCatView );
	NATIVE_GLOBAL_FUNCTION( "SetHightlightCatViewFx" , funcSetHightlightCatView );
	NATIVE_GLOBAL_FUNCTION( "EnableDrunkFx" , funcEnableDrunkFx );
	NATIVE_GLOBAL_FUNCTION( "DisableDrunkFx" , funcDisableDrunkFx );
	NATIVE_GLOBAL_FUNCTION( "ScaleDrunkFx" , funcScaleDrunkFx );
	NATIVE_GLOBAL_FUNCTION( "SetFogDensityCatViewFx" , funcSetFogDensityCatView );
}
