/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

/////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////
#include "../engine/renderCommands.h"
#include "../engine/swfTexture.h"

#include "guiRenderSystemScaleform.h"
#include "renderScaleformHal.h"
#include "renderTexture.h"
#include "renderScene.h"
#include "renderThread.h"
#include "renderGameplayRenderTarget.h"

//////////////////////////////////////////////////////////////////////////

#if defined( DEBUG_USE_GFX_REFERENCE_HAL )

# ifdef RED_PLATFORM_ORBIS

namespace GpuApi
{
	namespace Hacks
	{
		sce::Gnmx::GfxContext& GetGfxContext();
		sce::Gnm::RenderTarget& GetRenderTarget();
		sce::Gnm::DepthRenderTarget& GetDepthStencil();
	}
}

# endif

#endif

#ifdef DEBUG_USE_GFX_REFERENCE_HAL
# if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_ORBIS )
#  define SCALEFORM_RENDER_MSVC "Msvc11"
# else 
#  error Unsupported platform!
# endif

# if defined( _DEBUG )
#  define SCALEFORM_RENDER_LIB_CONFIG "Debug_LCUE"
# elif defined( RED_FINAL_BUILD )
#  define SCALEFORM_RENDER_LIB_CONFIG "Shipping_LCUE"
# else
#  define SCALEFORM_RENDER_LIB_CONFIG "Release_LCUE"
# endif

# if defined( RED_PLATFORM_WINPC )
#  ifndef RED_ARCH_X64
#   define SCALEFORM_RENDER_ARCH "Win32"
#  else
#   define SCALEFORM_RENDER_ARCH "x64"
#  endif
#	if defined( _DEBUG )
#		define SCALEFORM_RENDER_LIB_CONFIG "Debug"
#	elif defined( RED_FINAL_BUILD )
#		define SCALEFORM_RENDER_LIB_CONFIG "Shipping"
#	else
#		define SCALEFORM_RENDER_LIB_CONFIG "Release"
#	endif
# elif defined( RED_PLATFORM_DURANGO )
#   define SCALEFORM_RENDER_ARCH "XboxOne/XDK"
#	if defined( _DEBUG )
#		define SCALEFORM_RENDER_LIB_CONFIG "Debug_MonoD3D"
#	elif defined( RED_FINAL_BUILD )
#		define SCALEFORM_RENDER_LIB_CONFIG "Shipping_MonoD3D"
#	else
#		define SCALEFORM_RENDER_LIB_CONFIG "Release_MonoD3D"
#	endif
# elif defined( RED_PLATFORM_ORBIS )
#   define SCALEFORM_RENDER_ARCH "PS4"
#	if defined( _DEBUG )
#		define SCALEFORM_RENDER_LIB_CONFIG "Debug_LCUE"
#	elif defined( RED_FINAL_BUILD )
#		define SCALEFORM_RENDER_LIB_CONFIG "Shipping_LCUE"
#	else
#		define SCALEFORM_RENDER_LIB_CONFIG "Release_LCUE"
#	endif
# else
#  error Unsupported platform!
# endif

# define SCALEFORM_RENDER_LIB_PATH	"../../../external/gfx4/Lib/" SCALEFORM_RENDER_ARCH "/" SCALEFORM_RENDER_MSVC "/" SCALEFORM_RENDER_LIB_CONFIG "/"

# if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
#  pragma comment( lib, SCALEFORM_RENDER_LIB_PATH "libgfxrender_d3d11.lib" )
#  pragma comment( lib, "d3dcompiler.lib")
# elif defined( RED_PLATFORM_ORBIS )
#  define SCALEFORM_RENDER_ORBIS_SHADERS_LIB "../../../external/gfx4/Lib/" SCALEFORM_RENDER_ARCH "/" SCALEFORM_RENDER_LIB_CONFIG "/" "libgfxshaders.a"
#  pragma comment( lib, SCALEFORM_RENDER_ORBIS_SHADERS_LIB )
#  pragma comment( lib, SCALEFORM_RENDER_LIB_PATH "libgfxrender_ps4.a" )
# else
#  error "GFx render lib not defined!"
# endif
#endif

//////////////////////////////////////////////////////////////////////////

#ifdef RED_PLATFORM_WINPC
typedef void (*MessagePumpFunc)();
namespace Scaleform { namespace Render {
	extern MessagePumpFunc FnPumpMessages;
}}
#endif // RED_PLATFORM_WINPC

CRenderScaleform::CRenderScaleform()
	: m_pRenderer( nullptr )
	, m_pHal( nullptr )
	, m_hRenderThreadId( SF::ThreadId() )
	, m_renderState( RS_Uninitialized )
{
#ifdef RED_PLATFORM_WINPC
	extern void Hack_PumpWindowsMessagesForDXGI();
	SF::Render::FnPumpMessages = &Hack_PumpWindowsMessagesForDXGI;
#endif
}

CRenderScaleform::~CRenderScaleform()
{
	ASSERT( m_renderState <= RS_ShuttingDown );

	// Make sure these were never initialized or cleared with CGuiRenderSystem::Shutdown
	// Because this could occur on the non-renderer thread
	ASSERT( ! m_pRenderer );
	ASSERT( ! m_pHal );
}

Bool CRenderScaleform::Init()
{
	ASSERT( m_renderState == RS_Uninitialized );

	if ( m_renderState != RS_Uninitialized )
	{
		return false;
	}

	m_hRenderThreadId = SF::GetCurrentThreadId();

#ifndef DEBUG_USE_GFX_REFERENCE_HAL
	m_pHal = *SF_NEW CRenderScaleformHAL( this );

	static const SF::UInt32 halFlags = 0;
	CRenderScaleformHALInitParams initParams( halFlags , m_hRenderThreadId );
#elif defined ( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
	m_pHal = *SF_NEW SF::Render::D3D1x::HAL( this );
	SF::Render::D3D1x::HALInitParams initParams( GpuApi::Hacks::GetDevice(), GpuApi::Hacks::GetDeviceContext(), 0, m_hRenderThreadId );
#elif defined( RED_PLATFORM_ORBIS )

	m_pHal = *SF_NEW SF::Render::PS4::HAL( this );
	m_pMemoryManager = SF_NEW SF::Render::PS4::MemoryManager;

	SF::Render::PS4::HALInitParams initParams( GpuApi::Hacks::GetGfxContext(), m_pMemoryManager, 0, m_hRenderThreadId );
#else
# error Unsupported platform!
#endif

	if ( ! m_pHal->InitHAL( initParams ) )
	{
		ASSERT( ! "Failed to initialize HAL" );
		m_pHal->ShutdownHAL();
		m_hRenderThreadId = SF::ThreadId();
		m_renderState = RS_Error;
		return false;
	}
	
#if defined( DEBUG_USE_GFX_REFERENCE_HAL ) && defined( RED_PLATFORM_ORBIS )
	m_pDisplayRT = *SF_NEW SF::Render::RenderTarget(0, SF::Render::RBuffer_Default, SF::Render::ImageSize( 1280, 720 ) );// or 1080... config.ViewSize.Width, config.ViewSize.Height) );
	
	// Not contained in the Orbis impl because can't query them .
	SF::Render::PS4::RenderTargetData::UpdateData(m_pDisplayRT, &GpuApi::Hacks::GetRenderTarget(), 0, &GpuApi::Hacks::GetDepthStencil() );
	m_pHal->SetRenderTarget(m_pDisplayRT, true);
#endif

	m_pRenderer = *SF_NEW SF::Render::Renderer2D( m_pHal );
	
	m_renderState = RS_Ready;


	return true;
}

// Should have a safer way of shutting down, and then rejecting any (erroneous) subsequent commands.
Bool CRenderScaleform::Shutdown()
{
	ASSERT( ! ::SIsMainThread() );
	ASSERT( m_renderState == RS_DeviceLost || m_renderState == RS_Ready );

	// Releasing these on the render thread because the destructor will likely
	// happen in the main thread

	Bool shutdownOk = ( m_renderState == RS_DeviceLost || m_renderState == RS_Ready ); // Not a duplicate shutdown command

	if ( m_pHal )
	{
		shutdownOk &= m_pHal->ShutdownHAL(); // Try to shut down HAL anyway even if in an error state
	}

	m_pHal.Clear();
	
#if defined( DEBUG_USE_GFX_REFERENCE_HAL ) && defined( RED_PLATFORM_ORBIS )
	delete m_pMemoryManager;
#endif

	m_pRenderer = nullptr;
	m_renderState = shutdownOk ? RS_ShuttingDown : RS_Error;

	return shutdownOk;
}

void CRenderScaleform::BeginFrame()
{
	ASSERT( ! ::SIsMainThread() );

	if ( m_renderState != RS_Ready )
	{
		return;
	}

	m_pRenderer->BeginFrame();

#if defined( DEBUG_USE_GFX_REFERENCE_HAL ) && defined( RED_PLATFORM_ORBIS )
	m_pHal->SetGfxContext( &GpuApi::Hacks::GetGfxContext() ); // just blindly set for the moment, although should probably save old one for address comparison
	// Notify the HAL of the current backbuffer, and have the HAL set it in the Gnmx::GfxContext.
	SF::Render::PS4::RenderTargetData::UpdateData( m_pDisplayRT, &GpuApi::Hacks::GetRenderTarget(), 0, &GpuApi::Hacks::GetDepthStencil() );
	m_pHal->SetRenderTarget(m_pDisplayRT, true);
#endif
}

void CRenderScaleform::EndFrame()
{
	ASSERT( ! ::SIsMainThread() );

	if ( m_renderState != RS_Ready )
	{
		return;
	}

	CRenderScaleformTextureManager* textureManager = static_cast< CRenderScaleformTextureManager* >( GetTextureManager() );
	textureManager->ProcessTextures();

	m_pRenderer->EndFrame();

	//FIXME: Why no AMP symbols on Durango?
#if ! defined( RED_PLATFORM_CONSOLE ) && ! defined( SF_BUILD_SHIPPING )
	SF::AmpServer::GetInstance().AdvanceFrame();
#endif
}

Bool CRenderScaleform::SetGlyphCacheParams( const SF::Render::GlyphCacheParams& params )
{
	if ( ! m_pRenderer )
	{
		ERR_RENDERER(TXT("SetGlyphCacheParams: No renderer!"));
		return false;
	}

	if ( !m_pRenderer->GetGlyphCacheConfig()->SetParams(params) )
	{
		ERR_RENDERER(TXT("SetGlyphCacheParams: Failed to apply new params!"));
		return false;
	}

	return true;
}

void CRenderScaleform::Render( CRenderFrame* frame )
{
	ASSERT( ! ::SIsMainThread() );

	if ( m_renderState != RS_Ready )
	{
		return;
	}

	CRenderScaleformTextureManager* textureManager = static_cast< CRenderScaleformTextureManager* >( GetTextureManager() );
	textureManager->SetCurrentTime( frame->GetFrameInfo().m_engineTime );

	PC_SCOPE_PIX( CFlashPlayer_Render );

	{
		PC_SCOPE_PIX( RenderSceneUnderlays );

		// Render underlay regardless of SHOW_GUI (should really be a different show flag now)
		for ( SFlashMovieLayerScene& layerScene : m_layerSceneUnderlays )
		{
			GFx::MovieDisplayHandle& displayHandle = layerScene.m_displayHandle;
			if ( displayHandle.NextCapture( m_pRenderer->GetContextNotify() ) )
			{
				m_pRenderer->Display( displayHandle);
			}
		}
	}

	if ( ! frame->GetFrameInfo().IsShowFlagOn( SHOW_GUI ) )
	{
		return;
	}

	{
		PC_SCOPE_PIX( RenderSceneDefault );

		for ( SFlashMovieLayerScene& layerScene : m_layerScenes )
		{
			GFx::MovieDisplayHandle& displayHandle = layerScene.m_displayHandle;
			if ( displayHandle.NextCapture( m_pRenderer->GetContextNotify() ) )
			{
				m_pRenderer->Display( displayHandle);
			}
		}
	}
}

void CRenderScaleform::RenderOverlay( CRenderFrame* renderFrame )
{
	PC_SCOPE_PIX( RenderSceneOverlay );

	ASSERT( ! ::SIsMainThread() );

	if ( !GGame || !GGame->GetViewport() )
	{
		return;
	}

	if ( m_renderState != RS_Ready )
	{
		return;
	}

	// Render overlay regardless of SHOW_GUI (should really be a different show flag now)
	for ( SFlashMovieLayerScene& layerScene : m_layerSceneOverlays )
	{
		GFx::MovieDisplayHandle& displayHandle = layerScene.m_displayHandle;
		if ( displayHandle.NextCapture( m_pRenderer->GetContextNotify() ) )
		{
			m_pRenderer->Display( displayHandle);
		}
	}

}

//////////////////////////////////////////////////////////////////////////

SF::Render::TextureManager* CRenderScaleform::GetTextureManager() const
{
	RED_WARNING ( m_pHal, "HAL should not be null" );
	return m_pHal ? m_pHal->GetTextureManager() : 0;
}

//////////////////////////////////////////////////////////////////////////
SF::Render::Image* CRenderScaleform::CreateImage( IRenderResource* renderResource, Uint32 imageWidth, Uint32 imageHeight, Uint32 imageUseFlags )
{
#ifndef DEBUG_USE_GFX_REFERENCE_HAL
	CRenderScaleformTextureManager* textureManager = static_cast< CRenderScaleformTextureManager* >( GetTextureManager() );
	SF::Render::TextureImage* textureImage = nullptr;

	if ( ! textureManager || ! renderResource )
	{
		return nullptr;
	}

	RED_ASSERT( renderResource->IsRenderTexture(), TXT("Trying to create scaleform image from a render resource that isn't a texture!") );
	if ( renderResource->IsRenderTexture() )
	{
		CRenderTexture* renderTexture = static_cast< CRenderTexture* >( renderResource );

		const GpuApi::TextureRef& texRef = renderTexture->GetTextureRef();
		GpuApi::TextureDesc texDesc;
		GpuApi::GetTextureDesc( texRef, texDesc );
		const GpuApi::eTextureFormat gpuTextureFormat = texDesc.format;

		SF::Render::ImageFormat imageFormat = SF::Render::Image_None;

		switch ( gpuTextureFormat )
		{
		case GpuApi::TEXFMT_R8G8B8A8:
			imageFormat = SF::Render::Image_R8G8B8A8;
			break;
		case GpuApi::TEXFMT_R8G8B8X8:
			imageFormat = SF::Render::Image_R8G8B8;
			break;
		case GpuApi::TEXFMT_A8:
			imageFormat = SF::Render::Image_A8;
			break;
		case GpuApi::TEXFMT_BC1:
			imageFormat = SF::Render::Image_BC1;
			break;
		case GpuApi::TEXFMT_BC2:
			imageFormat = SF::Render::Image_BC2;
			break;
		case GpuApi::TEXFMT_BC3:
			imageFormat = SF::Render::Image_BC3;
			break;
		case GpuApi::TEXFMT_BC7:
			imageFormat = SF::Render::Image_BC7;
			break;
		default:
			RED_HALT( "Unsupported texture format: %" RED_PRIWas, GpuApi::GetTextureFormatName( gpuTextureFormat ) );
			break;
		};

		if ( imageFormat == SF::Render::Image_None )
		{
			return nullptr;
		}

		SF::Ptr< SF::Render::Texture > scaleformTexture = *textureManager->CreateTexture( texRef ); // gpuapi texture is addref'd
		if ( ! scaleformTexture )
		{
			return nullptr;
		}
		
		if  ( ! scaleformTexture->Initialize() ) // TBD: Without a pImage this scans the format table based on the d3d texdesc
		{
			scaleformTexture->Release();
			return nullptr;
		}
		
		const Uint32 unsupportedFlags = SF::Render::ImageUse_ReadOnly_Mask | SF::Render::ImageUse_GenMipmaps | SF::Render::ImageUse_RenderTarget;
	//	RED_ASSERT( ( imageUseFlags & unsupportedFlags ) == 0, TXT("Scaleform requesting texture from image '%ls', with unsupported use flags. Do we need to generate mips offline maybe?"), depotPath.AsChar() );

		Uint32 actualFlags = imageUseFlags;
		actualFlags &= ~unsupportedFlags;					// Make sure the unsupported flags are not present
		actualFlags |= SF::Render::ImageUse_InitOnly;		// Must initialize now

		textureImage = SF_NEW SF::Render::TextureImage( imageFormat, SF::Render::ImageSize( texDesc.width, texDesc.height ), actualFlags, scaleformTexture );
		textureImage->SetMatrix( SF::Render::Matrix2F::Scaling( imageWidth / Float(texDesc.width), imageHeight / Float(texDesc.height )) );
	}

	return textureImage;
	
#else
	RED_HALT( "DEBUG_USE_GFX_REFERENCE_HAL defined. Must creat texture image from file" )
	return nullptr;
#endif
}

//////////////////////////////////////////////////////////////////////////

SF::Render::Image* CRenderScaleform::CreateRenderTargetImage( IRenderGameplayRenderTarget* renderTarget , Uint32 imageWidth, Uint32 imageHeight )
{
#ifndef DEBUG_USE_GFX_REFERENCE_HAL
	if( !renderTarget )
	{
		return nullptr;
	}

	CRenderScaleformTextureManager* textureManager = static_cast< CRenderScaleformTextureManager* >( GetTextureManager() );
	SF::Render::TextureImage* textureImage = nullptr;

	if ( ! textureManager )
	{
		return nullptr;
	}

	if( !renderTarget->RequestResizeRenderSurfaces( imageWidth , imageHeight ) )
	{
		return nullptr;
	}

	Uint32 textureWidth = renderTarget->GetTextureWidth();
	Uint32 textureHeight = renderTarget->GetTextureHeight();
	SF::Render::ImageSize imageSize( textureWidth, textureHeight );

	CRenderScaleformTexture* texture = static_cast< CRenderScaleformTexture* >( textureManager->CreateTexture( renderTarget->GetGpuTexture(), imageSize ) );

	if  ( ! texture->Initialize() )
	{
		texture->Release();
		return nullptr;
	}
		
	const Uint32 unsupportedFlags = SF::Render::ImageUse_ReadOnly_Mask | SF::Render::ImageUse_GenMipmaps | SF::Render::ImageUse_RenderTarget;
	//	RED_ASSERT( ( imageUseFlags & unsupportedFlags ) == 0, TXT("Scaleform requesting texture from image '%ls', with unsupported use flags. Do we need to generate mips offline maybe?"), depotPath.AsChar() );

	Uint32 actualFlags = 0;//ImageUse_RenderTarget | ImageUse_NoDataLoss | ImageUse_Update;

	textureImage = SF_NEW SF::Render::TextureImage( SF::Render::Image_3DS_R8G8B8A8, imageSize, actualFlags, texture );
	textureImage->SetMatrix( SF::Render::Matrix2F::Scaling( textureWidth / Float(imageWidth), textureHeight / Float(imageHeight) ) );

	texture->Release();

	return textureImage;
#else
	RED_HALT( "DEBUG_USE_GFX_REFERENCE_HAL defined." );
	return nullptr;
#endif
}

//////////////////////////////////////////////////////////////////////////

void CRenderScaleform::SendThreadCommand( SF::Render::ThreadCommand* command )
{
	PushThreadCommand( command );
}

//////////////////////////////////////////////////////////////////////////

void CRenderScaleform::PushThreadCommand( SF::Render::ThreadCommand* command )
{
	ASSERT( command );
	ASSERT( GRender->GetRenderThread() ); // Sent a command too soon during startup?

	SF::Ptr<SF::Render::ThreadCommand> cmd = command;

	if ( cmd )
	{
		if ( SF::GetCurrentThreadId() == m_hRenderThreadId )
		{
			cmd->Execute();
		}
		else
		{
			( new CRenderCommand_GuiRenderCommandWrapper( cmd, this ) )->Commit();
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CRenderScaleform::AddDisplayHandle( const GFx::MovieDisplayHandle& handle, const SFlashMovieLayerInfo& flashMovieLayerInfo )
{
	ASSERT( ! ::SIsMainThread() );
	ASSERT( m_renderState != RS_ShuttingDown );

	SFlashMovieLayerScene layerScene( handle, flashMovieLayerInfo.m_layerDepth );
	switch ( flashMovieLayerInfo.m_renderGroup )
	{
	case eFlashMovieRenderGroup_Default:
		m_layerScenes.Insert( Move( layerScene ) );
		break;
	case eFlashMovieRenderGroup_Underlay:
		m_layerSceneUnderlays.Insert( Move( layerScene ) );
		break;
	case eFlashMovieRenderGroup_Overlay:
		m_layerSceneOverlays.Insert( Move( layerScene ) );
		break;
	default:
		HALT( "Unhandled renderGroup %u", flashMovieLayerInfo.m_renderGroup );
		break;
	}
}

void CRenderScaleform::RemoveDisplayHandle( const GFx::MovieDisplayHandle& handle, const SFlashMovieLayerInfo& flashMovieLayerInfo )
{
	ASSERT( ! ::SIsMainThread() );
	ASSERT( m_renderState != RS_ShuttingDown );

	SFlashMovieLayerScene layerScene( handle, flashMovieLayerInfo.m_layerDepth );
	switch ( flashMovieLayerInfo.m_renderGroup )
	{
	case eFlashMovieRenderGroup_Default:
		m_layerScenes.Remove( layerScene );
		break;
	case eFlashMovieRenderGroup_Underlay:
		m_layerSceneUnderlays.Remove( layerScene );
		break;
	case eFlashMovieRenderGroup_Overlay:
		m_layerSceneOverlays.Remove( layerScene );
		break;
	default:
		HALT( "Unhandled renderGroup %u", flashMovieLayerInfo.m_renderGroup );
		break;
	}
}

void CRenderScaleform::MoveDisplayHandle( const GFx::MovieDisplayHandle& handle, const SFlashMovieLayerInfo& flashMovieLayerInfo )
{
	ASSERT( ! ::SIsMainThread() );
	ASSERT( m_renderState != RS_ShuttingDown );

	SFlashMovieLayerScene layerScene( handle, flashMovieLayerInfo.m_layerDepth );

	m_layerSceneUnderlays.Remove( layerScene );
	m_layerScenes.Remove( layerScene );
	m_layerSceneOverlays.Remove( layerScene );

	AddDisplayHandle( handle, flashMovieLayerInfo );
}

//////////////////////////////////////////////////////////////////////////

void CRenderScaleform::HandleDeviceLost()
{
#ifndef RED_PLATFORM_ORBIS
	ASSERT( ! ::SIsMainThread() );
	ASSERT( m_renderState == RS_Uninitialized || m_renderState == RS_Ready || m_renderState == RS_ShuttingDown );
	ASSERT( m_renderState == RS_Uninitialized || m_pHal );

	if ( m_renderState == RS_Uninitialized || m_renderState == RS_ShuttingDown )
	{
		return;
	}

	if ( m_pHal )
	{
		m_pHal->PrepareForReset();
		m_renderState = RS_DeviceLost;
	}
	else
	{
		m_renderState = RS_Error;
	}
#endif // RED_PLATFORM_ORBIS
}

void CRenderScaleform::HandleDeviceReset()
{
#ifndef RED_PLATFORM_ORBIS
	ASSERT( ! ::SIsMainThread() );
	ASSERT( m_renderState == RS_Uninitialized || m_renderState == RS_DeviceLost || m_renderState == RS_ShuttingDown );
	ASSERT(  m_renderState == RS_Uninitialized || m_pHal );

	if ( m_renderState == RS_Uninitialized || m_renderState == RS_ShuttingDown )
	{
		return;
	}

	if ( m_renderState == RS_DeviceLost && 
		 m_pHal && m_pHal->IsInitialized() && 
		 m_pHal->RestoreAfterReset() )
	{
		m_renderState = RS_Ready;
	}
	else
	{
		// Could possibly try to keep restoring...
		m_renderState = RS_Error;
	}
#endif // RED_PLATFORM_ORBIS
}

void CRenderScaleform::GetRenderInterfaces( SF::Render::Interfaces* p )
{
	p->pRenderer2D = m_pRenderer;
	p->pHAL = m_pHal;
	p->pTextureManager = m_pHal->GetTextureManager();
	p->RenderThreadID = m_pHal->GetRenderThreadId();
}

////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////