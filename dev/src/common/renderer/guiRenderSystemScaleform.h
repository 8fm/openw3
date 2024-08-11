/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

#include "renderScaleformIncludes.h"

#include "../engine/renderScaleform.h"
#include "../engine/renderObject.h"
#include "../engine/flashMovie.h"

enum EFlashOverlay : Int32;

class CRenderScaleformHAL;
class CRenderFrame;
class IRenderGameplayRenderTarger;

struct SFlashMovieLayerScene
{
	GFx::MovieDisplayHandle	m_displayHandle;
	Int32					m_layerDepth;

	SFlashMovieLayerScene()
	{}

	SFlashMovieLayerScene( const GFx::MovieDisplayHandle& displayHandle, Int32 layerDepth )
		: m_displayHandle( displayHandle )
		, m_layerDepth( layerDepth )
	{}

	bool operator<( const SFlashMovieLayerScene& rhs ) const
	{
		return m_layerDepth < rhs.m_layerDepth;
	}

	Bool operator==( const SFlashMovieLayerScene& rhs )
	{
		return m_displayHandle == rhs.m_displayHandle;
	}
};

class CRenderScaleform
	: public IRenderScaleform
	, public SF::Render::ThreadCommandQueue
{
public:
	typedef TSortedArray< SFlashMovieLayerScene > TFlashMovieLayerScenes;

private:
	SF::Ptr<SF::Render::Renderer2D>			m_pRenderer;
#ifndef	DEBUG_USE_GFX_REFERENCE_HAL
	SF::Ptr<CRenderScaleformHAL>			m_pHal;
//#elif ( SF_D3D_VERSION == 10 ) || ( SF_D3D_VERSION == 11 )
#elif defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
	SF::Ptr< SF::Render::D3D1x::HAL >		m_pHal;
#elif defined( RED_PLATFORM_ORBIS )
	SF::Render::PS4::MemoryManager*			m_pMemoryManager;
	SF::Ptr< SF::Render::RenderTarget >		m_pDisplayRT;
	SF::Ptr< SF::Render::PS4::HAL >			m_pHal;
#else
# error "Reference implementation not supported"
#endif
	SF::ThreadId							m_hRenderThreadId;
	ERenderState							m_renderState;

private:
	TFlashMovieLayerScenes					m_layerSceneUnderlays;
	TFlashMovieLayerScenes					m_layerScenes;
	TFlashMovieLayerScenes					m_layerSceneOverlays;

public:
	CRenderScaleform();
	virtual ~CRenderScaleform();

public:
	Bool									SetGlyphCacheParams( const SF::Render::GlyphCacheParams& params );

public:
	virtual	SF::Render::Image*				CreateImage( IRenderResource* renderResource, Uint32 imageWidth, Uint32 imageHeight, Uint32 imageUseFlags ) override;
	virtual SF::Render::TextureManager*		GetTextureManager() const override; //!< Can be called from the main thread, although the texture manager may not have been created yet
	virtual void							SendThreadCommand( SF::Render::ThreadCommand* command ) override;
	virtual SF::Render::Image*				CreateRenderTargetImage( IRenderGameplayRenderTarget* renderTarget, Uint32 imageWidth, Uint32 imageHeight ) override;
	
	// SF::Render::ThreadCommandQueue implementation
public:
	RED_INLINE SF::Render::ThreadCommandQueue* GetCommandQueue() { return this; }
	virtual void PushThreadCommand( SF::Render::ThreadCommand* command );
	virtual void GetRenderInterfaces( SF::Render::Interfaces* p );
		
	// Functions to be called only on the render thread
public:
	void BeginFrame();
	void Render( CRenderFrame* frame );
	void RenderOverlay( CRenderFrame* frame );
	void EndFrame();

public:

	Bool Init();
	Bool Shutdown();

	void AddDisplayHandle( const GFx::MovieDisplayHandle& handle, const SFlashMovieLayerInfo& flashMovieLayerInfo );
	void RemoveDisplayHandle( const GFx::MovieDisplayHandle& handle, const SFlashMovieLayerInfo& flashMovieLayerInfo );
	void MoveDisplayHandle( const GFx::MovieDisplayHandle& handle, const SFlashMovieLayerInfo& flashMovieLayerInfo );
	
	void HandleDeviceLost();

	void HandleDeviceReset();

	virtual ERenderState GetRenderState() const override { return m_renderState; }
	
};

/////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////