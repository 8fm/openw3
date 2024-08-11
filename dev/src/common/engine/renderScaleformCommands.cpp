/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "renderScaleformCommands.h"

#include "renderProxy.h"
#include "renderFrame.h"

/////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

CGuiRenderCommand_ReleaseMovie::CGuiRenderCommand_ReleaseMovie( SF::Ptr< GFx::Movie >& flashMovieRef )
{
	// Transfer the ptr to the command, to ensure that the last reference will be released on the render thread
	m_flashMovie = flashMovieRef; // addref
	flashMovieRef.Clear();		  // release
}

CGuiRenderCommand_SetGlyphCacheParams::CGuiRenderCommand_SetGlyphCacheParams( const SF::Render::GlyphCacheParams& params, volatile Bool* pResult )
	: m_params( params )
	, m_pResult( pResult )
{
}

CGuiRenderCommand_AddDisplayHandle::CGuiRenderCommand_AddDisplayHandle( const GFx::MovieDisplayHandle& handle, const SFlashMovieLayerInfo& flashMovieLayerInfo )
	: m_hMovieDisplay ( handle )
	, m_flashMovieLayerInfo( flashMovieLayerInfo )
{
}

CGuiRenderCommand_RemoveDisplayHandle::CGuiRenderCommand_RemoveDisplayHandle( const GFx::MovieDisplayHandle& handle, const SFlashMovieLayerInfo& flashMovieLayerInfo )
	: m_hMovieDisplay ( handle )
	, m_flashMovieLayerInfo( flashMovieLayerInfo )
{
}

CGuiRenderCommand_MoveDisplayHandle::CGuiRenderCommand_MoveDisplayHandle( const GFx::MovieDisplayHandle& handle, const SFlashMovieLayerInfo& flashMovieLayerInfo )
	: m_hMovieDisplay ( handle )
	, m_flashMovieLayerInfo( flashMovieLayerInfo )
{
}

CGuiRenderCommand_CreateStreamingPlaceholderTextures::CGuiRenderCommand_CreateStreamingPlaceholderTextures( CScaleformTextureCacheQueue* queue )
	: m_queue ( queue )
{
}

/////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

