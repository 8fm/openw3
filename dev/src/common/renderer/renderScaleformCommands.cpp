/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

/////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

#include "../engine/renderScaleformCommands.h"
#include "../engine/scaleformTextureCacheQuee.h"

#include "guiRenderSystemScaleform.h"

//////////////////////////////////////////////////////////////////////////

void CGuiRenderCommand_ReleaseMovie::Execute()
{
	if ( m_flashMovie )
	{
		// avoid debug asserts, should be in dtor
		m_flashMovie->SetCaptureThread( SF::GetCurrentThreadId() );
	}

	m_flashMovie.Clear();
}

//////////////////////////////////////////////////////////////////////////

void CGuiRenderCommand_SetGlyphCacheParams::Execute()
{
	ASSERT( GRender && GRender->GetRenderScaleform() );
	if ( GRender && GRender->GetRenderScaleform() )
	{
		CRenderScaleform* const renderScaleform = static_cast<CRenderScaleform*>(GRender->GetRenderScaleform());
		const Bool result = renderScaleform->SetGlyphCacheParams( m_params );
		if ( m_pResult )
		{
			*m_pResult = result;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CGuiRenderCommand_AddDisplayHandle::Execute()
{
	ASSERT( GRender && GRender->GetRenderScaleform() );
	if ( GRender && GRender->GetRenderScaleform() )
	{
		static_cast<CRenderScaleform*>(GRender->GetRenderScaleform())->AddDisplayHandle( m_hMovieDisplay, m_flashMovieLayerInfo );
	}
}

//////////////////////////////////////////////////////////////////////////

void CGuiRenderCommand_RemoveDisplayHandle::Execute()
{
	ASSERT( GRender && GRender->GetRenderScaleform() );
	if ( GRender && GRender->GetRenderScaleform() )
	{
		static_cast<CRenderScaleform*>(GRender->GetRenderScaleform())->RemoveDisplayHandle( m_hMovieDisplay, m_flashMovieLayerInfo );
	}
}

//////////////////////////////////////////////////////////////////////////

void CGuiRenderCommand_MoveDisplayHandle::Execute()
{
	ASSERT( GRender && GRender->GetRenderScaleform() );
	if ( GRender && GRender->GetRenderScaleform() )
	{
		static_cast<CRenderScaleform*>(GRender->GetRenderScaleform())->MoveDisplayHandle( m_hMovieDisplay, m_flashMovieLayerInfo );
	}
}

//////////////////////////////////////////////////////////////////////////

void CGuiRenderCommand_ShutdownSystem::Execute()
{
	ASSERT( GRender && GRender->GetRenderScaleform() );
	if ( GRender && GRender->GetRenderScaleform() )
	{
		static_cast<CRenderScaleform*>(GRender->GetRenderScaleform())->Shutdown();
	}
}

//////////////////////////////////////////////////////////////////////////

void CGuiRenderCommand_HandleDeviceLost::Execute()
{
	ASSERT( GRender && GRender->GetRenderScaleform() );
	if ( GRender && GRender->GetRenderScaleform() )
	{
		static_cast<CRenderScaleform*>(GRender->GetRenderScaleform())->HandleDeviceLost();
	}
}

//////////////////////////////////////////////////////////////////////////

void CGuiRenderCommand_HandleDeviceReset::Execute()
{
	ASSERT( GRender && GRender->GetRenderScaleform() );
	if ( GRender && GRender->GetRenderScaleform() )
	{
		static_cast<CRenderScaleform*>(GRender->GetRenderScaleform())->HandleDeviceReset();
	}
}

//////////////////////////////////////////////////////////////////////////

void CGuiRenderCommand_CreateStreamingPlaceholderTextures::Execute()
{
	ASSERT( m_queue );
	if( m_queue )
	{
		m_queue->CreatePlaceholderTexture();
	}
}

/////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////
