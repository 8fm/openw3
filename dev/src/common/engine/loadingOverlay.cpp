/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../core/gatheredResource.h"
#include "flashMovie.h"
#include "flashPlayer.h"
#include "swfResource.h"
#include "renderCommands.h"
#include "renderer.h"
#include "../core/depot.h"
#include "../core/configVar.h"
#include "viewport.h"
#include "loadingOverlay.h"

CGatheredResource resLoadingScreenOverlay(TXT("gameplay\\gui_new\\swf\\overlay\\00_loading.redswf"), 0);

static CFlashMovie* CreateLoadingOverlayFlash()
{
	if ( !GGame || !GGame->GetFlashPlayer() )
	{
		ERR_ENGINE(TXT("CreateLoadingOverlayFlash: Flash player not available"));
		return nullptr;
	}

	SFlashMovieInitParams flashMovieInitParams;
	flashMovieInitParams.m_layer = eFlashMovieOverlayDepth_LoadingOverlay;
	flashMovieInitParams.m_renderGroup = eFlashMovieRenderGroup_Overlay;
	flashMovieInitParams.m_attachOnStart = false;
	flashMovieInitParams.m_notifyPlayer = false;
	flashMovieInitParams.m_waitForLoadFinish = true;
	const TSoftHandle< CSwfResource > swfHandle(ANSI_TO_UNICODE(resLoadingScreenOverlay.GetPath().ToAnsiString()));
	CFlashMovie* flashMovie = ( GGame && GGame->GetFlashPlayer() ) ? GGame->GetFlashPlayer()->CreateMovie( swfHandle, flashMovieInitParams ) : nullptr;
	if ( !flashMovie )
	{
		ERR_ENGINE(TXT("CreateLoadingOverlayFlash: failed to create Flash movie %ls"), swfHandle.GetPath().AsChar() );
		return nullptr;
	}

	return flashMovie;
}

//////////////////////////////////////////////////////////////////////////
// CLoadingScreen
//////////////////////////////////////////////////////////////////////////
CLoadingOverlay::CLoadingOverlay()
{
}

CLoadingOverlay::~CLoadingOverlay()
{
}

Bool CLoadingOverlay::DelayedInit()
{
	if ( ! GIsCooker)
	{
		CFlashMovie* flashMovie = CreateLoadingOverlayFlash();
		if ( !flashMovie )
		{
			return false;
		}

		( new CRenderCommand_SetLoadingOverlayFlash( flashMovie ) )->Commit();
	}

	return true;
}

void CLoadingOverlay::ToggleVisible( Bool visible, const String& reason )
{
	( new CRenderCommand_ToggleLoadingOverlay( visible, reason ) )->Commit();
}

