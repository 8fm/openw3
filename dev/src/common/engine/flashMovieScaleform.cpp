/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#ifdef USE_SCALEFORM

#include "../core/configVar.h"

#include "flashMovieScaleform.h"
#include "flashPlayerScaleform.h"
#include "flashFunctionScaleform.h"
#include "flashValueObjectScaleform.h"
#include "flashValueStorageScaleform.h"
#include "flashRenderTargetScaleform.h"
#include "scaleformSystem.h"

#include "renderCommands.h"
#include "renderScaleform.h"
#include "renderScaleformCommands.h"
#include "flashValue.h"

namespace Config
{
	TConfigVar< Bool > cvFlushScaleformMovieAfterRelease( "Scaleform", "FlushMovieAfterRelease", true );
	TConfigVar< Bool > cvFlashCaptureOnAdvance( "Scaleform", "CaptureOnAdvance",  false );
}

CFlashMovieScaleform::CFlashMovieScaleform( IScaleformPlayer* parent, GFx::Movie* gfxMovie, const SFlashMovieLayerInfo& layerInfo, Uint32 flags )
	: TBaseClass( layerInfo )
	, m_parentWeakRef( parent )
	, m_gfxMovie( *gfxMovie )
	, m_flags( flags )
	, m_forceEnableCaptureOnAdvance( false )
{
	ASSERT( parent );
	ASSERT( m_gfxMovie );

	if ( m_gfxMovie )
	{
		m_status = Detached;
		m_gfxMovie->SetUserData( this ); // Link the GFx movie back to this wrapper to make it easy to pass the wrapper to callbacks
		if ( m_flags & eFlashMovieFlags_AttachOnStart )
		{
			DoAttach();
		}
	}
	else
	{
		m_status = Failed;
	}
}

CFlashMovieScaleform::~CFlashMovieScaleform()
{
	ASSERT( m_parentWeakRef );

	if ( m_gfxMovie )
	{
		m_gfxMovie->SetUserData( nullptr );
	}

	DoDetach();

	if ( GIsClosing )
	{
		// Don't mess around with the lack of render thread
		return;
	}

	if ( m_gfxMovie )
	{
		// Destruct movies on the render thread. If you destroy them on another thread, then Scaleform will BLOCK on render-context
		// shutdown, which can cause problems with DXGI needing Win32 messages processsed. If you use the ShutdownRendering() and polling
		// as the docs suggest then there are other issues: You still need to do NextCapture() on the render thread to make it update and that
		// can deadlock because of a mutex it locks and a mutex in the movie dtor on the main thread. When a SF movie destructs off the render
		// thread, then it sends a render command to update some stuff - the above-mentioned mutex deadlock can happen because the render command
		// is spinning for some space in the render command buffer, but the render thread is blocked waiting for the mutex and won't process any
		// command to clean up space.
		//
		// tl;dr - destruct GFx::Movie's on the render thread. You'll be glad you did.

		CGuiRenderCommand<TReleaseMovie>::Send( m_gfxMovie );
		if ( Config::cvFlushScaleformMovieAfterRelease.Get() && ::SIsMainThread() )
		{
			GRender->Flush();
		}
	}
}

void CFlashMovieScaleform::Tick( Float timeDelta )
{
	if ( m_gfxMovie && !GIsClosing ) // If we are closing the game, stop right there. A bit of an hack, but there some hard crash in scaleform because of dependency with our system that are being shutdown 
	{
		m_gfxMovie->Advance( timeDelta, GetCatchupFrames(), m_forceEnableCaptureOnAdvance || Config::cvFlashCaptureOnAdvance.Get() );
	}
}

void CFlashMovieScaleform::Capture( Bool force )
{
	if ( !m_forceEnableCaptureOnAdvance && !Config::cvFlashCaptureOnAdvance.Get() && m_gfxMovie )
	{
		const Bool onRenderChangeOnly = !force;
		m_gfxMovie->Capture( onRenderChangeOnly );
	}
}

void CFlashMovieScaleform::SetViewport( const Rect& rect )
{
	if ( m_gfxMovie )
	{
		// Adjust for the cachets and preserve 16:9 aspect ratio.
		//! this upscale to 16:9 viewport size for scaleform is only temporary to the time when UI team fix problem with multi aspect ratio support 
#ifndef RED_VIEWPORT_TURN_ON_CACHETS_16_9
		if((Float)rect.Width()/(Float)rect.Height() <= 1.777777777777778f)
		{
			Uint32 horizontalPlusWidth = (Uint32)(rect.Height()*1.777777777777778f);
			Int32 horizonatlPlusX = ((Int32)rect.Width() - (Int32)horizontalPlusWidth)/2;
			const GFx::Viewport viewport( horizontalPlusWidth, rect.Height() , horizonatlPlusX, 0, horizontalPlusWidth, rect.Height() );
			m_gfxMovie->SetViewport( viewport );
		}
		else
#endif
		{
			const GFx::Viewport viewport( rect.Width(), rect.Height(), 0, 0, rect.Width(), rect.Height() );
			m_gfxMovie->SetViewport( viewport );
		}
	}
}

void CFlashMovieScaleform::DoAttach()
{
	if ( m_status == Detached )
	{
		if ( (m_flags & eFlashMovieFlags_NotifyPlayer) && m_parentWeakRef )
		{
			m_parentWeakRef->RegisterScaleformMovie( this );
		}

		if ( m_gfxMovie )
		{
			const GFx::MovieDisplayHandle dh = m_gfxMovie->GetDisplayHandle();
			CGuiRenderCommand<TAddDisplayHandle>::Send( dh, GetLayerInfo() );
		}
		m_status = Playing;
	}
}

void CFlashMovieScaleform::DoDetach()
{
	if ( m_status == Playing )
	{
		if ( (m_flags & eFlashMovieFlags_NotifyPlayer) && m_parentWeakRef )
		{
			m_parentWeakRef->UnregisterScaleformMovie( this );
		}

		if ( m_gfxMovie )
		{
			const GFx::MovieDisplayHandle dh = m_gfxMovie->GetDisplayHandle();
			CGuiRenderCommand<TRemoveDisplayHandle>::Send( dh, GetLayerInfo() );
		}
		m_status = Detached;
	}
}

void CFlashMovieScaleform::Attach()
{
	DoAttach();	
}

void CFlashMovieScaleform::Detach()
{
	DoDetach();
}

void CFlashMovieScaleform::OnLayerInfoChanged( const SFlashMovieLayerInfo& layerInfo )
{
	if ( Playing == m_status && m_gfxMovie )
	{
		const GFx::MovieDisplayHandle dh = m_gfxMovie->GetDisplayHandle();
		CGuiRenderCommand<TMoveDisplayHandle>::Send( dh, GetLayerInfo() );
	}
}

CFlashFunction* CFlashMovieScaleform::CreateFunction( CFlashFunctionHandler* flashFunctionHandler )
{
	ASSERT( flashFunctionHandler );
	if ( ! flashFunctionHandler )
	{
		return nullptr;
	}

	CFlashFunction* flashFunction = new CFlashFunctionScaleform( this, flashFunctionHandler );
	return flashFunction;
}

CFlashObject* CFlashMovieScaleform::CreateObject( const String& flashClassName )
{
	return new CFlashObjectScaleform( this, flashClassName );
}

CFlashArray* CFlashMovieScaleform::CreateArray()
{
	return new CFlashArrayScaleform( this );
}

CFlashString* CFlashMovieScaleform::CreateString( const String& value )
{
	return new CFlashStringScaleform( this, value );
}

CFlashValueStorage* CFlashMovieScaleform::CreateFlashValueStorage()
{
	CFlashValueStorageScaleform* flashValueStorageScaleform = new CFlashValueStorageScaleform( this );
	return flashValueStorageScaleform;
}

void CFlashMovieScaleform::OnGFxKeyEvent( const GFx::KeyEvent& event )
{
	// Note: The original EInputKey may have been mapped to a different SF KeyCode. E.g., analog stick up/down to arrows
	const EInputKey key = static_cast< EInputKey >( event.KeyCode );
	if ( m_gfxMovie && IsInputSourceEnabled( ISF_Key ) && ! IsKeyIgnored( key ) )
	{
		m_gfxMovie->HandleEvent( event );
	}
}

void CFlashMovieScaleform::OnGFxMouseEvent( const GFx::MouseEvent& event )
{
	if ( m_gfxMovie && IsInputSourceEnabled( ISF_Mouse ) )
	{
		m_gfxMovie->HandleEvent( event );
	}
}

void CFlashMovieScaleform::OnGFxGamePadAnalogEvent( const GFx::GamePadAnalogEvent& event )
{
	if ( m_gfxMovie && IsInputSourceEnabled( ISF_Analog ) )
	{
		m_gfxMovie->HandleEvent( event );
	}
}

CFlashRenderTarget* CFlashMovieScaleform::CreateRenderTarget( const String& targetName, Uint32 width, Uint32 height )
{
	return new CFlashRenderTargetScaleform( this, targetName, width, height );
}

SF::GFx::Resource* CFlashMovieScaleform::GetGFxResource( const String& resourceName ) const
{
	SF::GFx::Resource* res = nullptr;
	if ( m_gfxMovie )
	{
		res = m_gfxMovie->GetMovieDef()->GetResource( FLASH_TXT_TO_UTF8(resourceName.AsChar()) );
	}

	return res;
}

void CFlashMovieScaleform::ForceUpdateGFxImages()
{
	if ( m_gfxMovie )
	{
		m_gfxMovie->ForceUpdateImages();
	}
}

void CFlashMovieScaleform::UpdateCaptureThread()
{
	if ( m_gfxMovie )
	{
		m_gfxMovie->SetCaptureThread( SF::GetCurrentThreadId() );
	}


	if ( !::SIsMainThread() )
	{
		m_forceEnableCaptureOnAdvance = true;
	}
}

void CFlashMovieScaleform::SetBackgroundColor( const Color& color )
{
	if ( m_gfxMovie )
	{
		m_gfxMovie->SetBackgroundColor( SF::Render::Color( color.R, color.G, color.B, color.A ) );
	}
}

void CFlashMovieScaleform::SetBackgroundAlpha( Float alpha )
{
	if ( m_gfxMovie )
	{
		m_gfxMovie->SetBackgroundAlpha( alpha );
	}
}

Float CFlashMovieScaleform::GetBackgroundAlpha() const
{
	return m_gfxMovie ? m_gfxMovie->GetBackgroundAlpha() : 0.f;
}

#endif // USE_SCALEFORM
