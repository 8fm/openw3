/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../core/gatheredResource.h"
#include "flashMovie.h"
#include "flashPlayer.h"
#include "renderCommands.h"
#include "videoPlayer.h"
#include "../engine/baseEngine.h"
#include "../renderer/renderVideoPlayer.h"

RED_DEFINE_NAME( Video );

// In order to get picked up by the cooker
CGatheredResource resScaleformVideo( TXT("engine\\swf\\videoplayer.redswf"), RGF_Startup );

//////////////////////////////////////////////////////////////////////////
// SVideoParams
//////////////////////////////////////////////////////////////////////////
SVideoParams::SVideoParams( const String& fileName, Uint8 videoParamFlags /*= eVideoParamFlag_None */, EVideoBuffer videoBuffer /*= eVideoBuffer_Default*/ )
	: m_fileName( fileName )
	, m_videoParamFlags( videoParamFlags )
	, m_videoBuffer( videoBuffer )
{
}

//////////////////////////////////////////////////////////////////////////
// IRenderVideo
//////////////////////////////////////////////////////////////////////////
void IRenderVideo::Cancel()
{
	( new CRenderCommand_CancelVideo( this ) )->Commit();
}

//////////////////////////////////////////////////////////////////////////
// CVideoPlayer
//////////////////////////////////////////////////////////////////////////
CVideoPlayer::CVideoPlayer()
{
}

void CVideoPlayer::CreateGlobalVideoPlayer(CFlashPlayer* flashPlayer)
{
	static CFlashMovie* gFlashVideoPlayer = nullptr;
	if (gFlashVideoPlayer)
		return;

	SFlashMovieInitParams flashMovieInitParams;
	flashMovieInitParams.m_layer = eFlashMovieUnderlayDepth_VideoPlayer;
	flashMovieInitParams.m_renderGroup = eFlashMovieRenderGroup_Underlay;
	flashMovieInitParams.m_attachOnStart = false;
	flashMovieInitParams.m_notifyPlayer = false;
	const String swfPath(TXT("engine\\swf\\videoplayer.redswf"));
	const TSoftHandle< CSwfResource > swfHandle(swfPath);
	gFlashVideoPlayer = flashPlayer->CreateMovie( swfHandle, flashMovieInitParams );
	if ( !gFlashVideoPlayer )
	{
		ERR_ENGINE(TXT("CVideoPlayer::Init: failed to create Flash movie %ls"), swfPath.AsChar() );
		return;
	}
	( new CRenderCommand_SetVideoFlash( gFlashVideoPlayer ) )->Commit();
}

Bool CVideoPlayer::DelayedInit()
{
	if (!GIsCooker && GGame && GGame->GetFlashPlayer())
	{
		CreateGlobalVideoPlayer(GGame->GetFlashPlayer());
	}

	return true;
}

CVideoPlayer::~CVideoPlayer()
{
}

IRenderVideo* CVideoPlayer::CreateVideo( CName videoClient, const SVideoParams& videoParams )
{
	IRenderVideo* renderVideo = GRender->CreateVideo( videoClient, videoParams );
	if ( ! renderVideo )
	{
		return nullptr;
	}

	( new CRenderCommand_PlayVideo( renderVideo, eVideoThreadIndex_GameThread ) )->Commit();
	
	return renderVideo;
}

void CVideoPlayer::TogglePause( Bool pause )
{
	( new CRenderCommand_ToggleVideoPause( pause ) )->Commit();
}
