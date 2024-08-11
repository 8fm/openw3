/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#ifdef USE_SCALEFORM

#include "guiGlobals.h"

#include "baseEngine.h"
#include "renderer.h"
#include "renderCommands.h"
#include "renderCamera.h"
#include "renderFrame.h"
#include "renderFrameInfo.h"
#include "renderProxy.h"
#include "renderScaleform.h"
#include "renderScaleformCommands.h"
#include "viewport.h"

#include "flashRenderSceneProvider.h"
#include "flashMovieScaleform.h"
#include "flashRenderTargetScaleform.h"

#include "../renderer/renderInterface.h"
#include "../renderer/renderViewport.h"
#include "renderGameplayRenderTargetInterface.h"

//////////////////////////////////////////////////////////////////////////
// CFlashRenderTargetScaleform
//////////////////////////////////////////////////////////////////////////
//SHOW_PostProcess
CFlashRenderTargetScaleform::CFlashRenderTargetScaleform( CFlashMovieScaleform* flashMovie, const String& targetName, Uint32 width, Uint32 height )
	: m_flashMovie( flashMovie )
	, m_targetName( targetName )
	, m_imageWidth( width )
	, m_imageHeight( height )
{
	RED_ASSERT( flashMovie );

	if ( m_flashMovie )
	{
		m_flashMovie->AddRef();
	}
}

CFlashRenderTargetScaleform::~CFlashRenderTargetScaleform()
{
	if ( m_flashMovie )
	{
		m_flashMovie->Release();
		m_flashMovie = nullptr;
	}

	if( m_renderTarget )
	{
		m_renderTarget->Release();
		m_renderTarget = nullptr;
	}
}

void CFlashRenderTargetScaleform::RenderScene( IFlashRenderSceneProvider* renderSceneProvider )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Call from main thread only!" );
	RED_ASSERT( renderSceneProvider );

	if ( ! m_renderTarget )
	{
		TryReplaceImage();
	}

	// TBD: Separate texture creation from replacement?
	if ( GEngine->IsActiveSubsystem( ES_Rendering ) && renderSceneProvider && m_renderTarget )
	{
		renderSceneProvider->RenderWorld( m_renderTarget );	
	}
}

Bool CFlashRenderTargetScaleform::TryReplaceImage()
{
	const Bool hasRender = GRender && GRender->GetRenderScaleform();
	const Bool hasMovie = m_flashMovie && m_flashMovie->GetStatus() == CFlashMovie::Playing;
	if ( ! hasRender || ! hasMovie )
	{
		return false;
	}
	
	SF::GFx::Resource* resource = m_flashMovie->GetGFxResource( m_targetName );
	if ( ! resource )
	{
		// Silent. Maybe hasn't loaded yet from another SWF?
		return false;
	}

	if ( resource->GetResourceType() != SF::GFx::Resource::RT_Image )
	{
		GUI_ERROR( TXT("CFlashRenderTargetScaleform: resource '%ls' is not an image"), m_targetName.AsChar() );
		return false;
	}

	SF::GFx::ImageResource* imageResource = static_cast< SF::GFx::ImageResource* >( resource );

	SF::GFx::ImageBase* image = imageResource->GetImage();
	if ( ! image )
	{
		GUI_ERROR( TXT("CFlashRenderTargetScaleform: resource '%ls' has not image to replace"), m_targetName.AsChar() );
		return false;
	}

// 	SF::Render::ImageSize imageSize = image->GetSize();
// 	if ( imageSize.IsEmpty() )
// 	{
// 		GUI_ERROR( TXT("CFlashRenderTargetScaleform: resource '%ls' has too small an image to replace"), m_targetName.AsChar() );
// 		return false;
// 	}

	IViewport* vp = GGame->GetViewport();
	if ( ! vp )
	{
		return false;
	}

	// Scale down the backing texture so it isn't bigger than the viewport because the buffer rendered into isn't any bigger.
	// The renderInfo needs to be scaled the same

	// TODO: Recreate when CRenderSurfaces is resized!
	const Uint32 vpWidth = vp->GetRendererWidth();
	const Uint32 vpHeight = vp->GetRendererHeight();
	if ( vpWidth < 1 || vpHeight < 1 )
	{
		return false;
	}

	// Have override so don't have to use a huge texture in the SWF just to get replaced!
	Uint32 finalWidth	= m_imageWidth;
	Uint32 finalHeight	= m_imageHeight;
	
	if ( finalWidth > vpWidth )
	{
		const Float shrinkRatio = vpWidth / (Float)finalWidth;
		finalWidth = Uint32((Float)finalWidth * shrinkRatio);
		finalHeight = Uint32((Float)finalHeight * shrinkRatio);
	}

	if ( finalHeight > vpHeight )
	{
		const Float shrinkRatio = vpHeight / (Float)finalHeight;
		finalWidth = Uint32((Float)finalWidth * shrinkRatio);
		finalHeight = Uint32((Float)finalHeight *  shrinkRatio);
	}

	if ( finalWidth < 1 )
	{
		return false;
	}

	if ( finalHeight < 1 )
	{
		return false;
	}
	
	if( !m_renderTarget )
	{
		m_renderTarget = GRender->CreateGameplayRenderTarget( "ScaleformUI" );
	}

	SF::Render::Image* renderImage = GRender->GetRenderScaleform()->CreateRenderTargetImage( m_renderTarget, finalWidth, finalHeight );
	if ( !renderImage )
	{
		GUI_ERROR( TXT("CFlashRenderTargetScaleform: failed to create render target image for resource '%ls'"), m_targetName.AsChar() );
		return false;
	}

	imageResource->SetImage( renderImage );
	renderImage->Release();
	renderImage = nullptr;
	m_flashMovie->ForceUpdateGFxImages();

	return true;
}

#endif // USE_SCALEFORM
