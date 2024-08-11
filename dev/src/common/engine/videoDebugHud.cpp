/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "videoPlayer.h"
#include "flashMovie.h"
#include "videoDebugHud.h"
#include "soundSystem.h"
#include "baseEngine.h"
#include "renderSettings.h"

#include "../core/gatheredResource.h"

// In order to get picked up by the cooker
CGatheredResource resVideoDebugHud( TXT("engine\\swf\\videodebughud.redswf"), RGF_Startup );

#ifndef NO_DEBUG_PAGES

RED_DEFINE_STATIC_NAME( VideoClient_VideoDebugHud );

CVideoDebugHud* GVideoDebugHud;

CVideoDebugHud::CVideoDebugHud()
	: m_currentVideo( nullptr )
	, m_flashVideoDebugHud( nullptr )
	, m_isFlashRegistered( false )
	, m_isVideoTitleSet( false )
{
}

CVideoDebugHud::~CVideoDebugHud()
{
	CancelVideo();
	CloseVideoDebugHud();
}

void CVideoDebugHud::Tick( Float timeDelta )
{
	PC_SCOPE_PIX( VideoDebugHud );
	Rect viewportRect = Rect::EMPTY;

	IViewport* viewport = GGame ? GGame->GetViewport() : nullptr;
	if ( viewport )
	{
		viewportRect.m_left = viewport->GetX();
		viewportRect.m_right = viewportRect.m_left + viewport->GetWidth();
		viewportRect.m_top = viewport->GetY();
		viewportRect.m_bottom = viewportRect.m_top + viewport->GetHeight();
		if( Config::cvForcedRendererOverlayResolution.Get() )
		{
			viewportRect.m_left = 0;
			viewportRect.m_right = Config::cvForcedRendererResolutionWidth.Get();
			viewportRect.m_top = 0;
			viewportRect.m_bottom = Config::cvForcedRendererResolutionHeight.Get();	
		}
	}

	if ( m_flashVideoDebugHud )
	{
		m_flashVideoDebugHud->SetViewport( viewportRect );
		m_flashVideoDebugHud->Tick( timeDelta );
	}

	if ( !m_currentVideo || !m_currentVideo->IsValid() )
	{
		CancelVideo();
		CloseVideoDebugHud();
		return;
	}

	if ( !m_isFlashRegistered )
	{
		return;
	}

	if ( ! m_isVideoTitleSet )
	{
		m_flashObjects.m_tfTitle.SetText( m_currentVideoTitle );
		m_isVideoTitleSet = true;
	}

	String tmp;
	if ( m_currentVideo->FlushSubtitles( tmp ) )
	{
		m_flashObjects.m_tfSubtitles.SetText( tmp );
	}

	if ( m_currentVideo->FlushCuePoint( tmp ) )
	{
		m_flashObjects.m_tfCuePoint.SetText( tmp );
	}

	if ( m_currentVideo->FlushMetaData( tmp ) )
	{
		TDynArray< String > data = tmp.Split( TXT(",") );
		m_flashObjects.m_tfMetadata.SetText( tmp );
	}
}

void CVideoDebugHud::OnFlashExternalInterface( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args, CFlashValue& outRetval )
{
	const Uint32 argCount = args.Size();
	
	if ( methodName.EqualsNC( TXT("registerVideoDebugHud") ) )
	{
		if ( argCount != 1 )
		{
			ERR_ENGINE(TXT("CRenderLoadingScreen::OnExternalInterface: Expected argCount 1"));
			return;
		}
		if ( ! args[0].IsFlashDisplayObject() )
		{
			ERR_ENGINE(TXT("CRenderLoadingScreen::OnExternalInterface: Expected arg[0] to be a display object"));
			return;
		}
		CFlashValue flashSprite = args[0];
	
		RegisterVideoDebugHud( flashSprite );
	}
	else
	{
		ERR_ENGINE(TXT("Unknown external interface %ls"), methodName.AsChar() );
	}
}

Bool CVideoDebugHud::RegisterVideoDebugHud( CFlashValue& flashSprite )
{
	if ( ! flashSprite.IsFlashDisplayObject() )
	{
		ERR_ENGINE(TXT("RegisterVideoDebugHud: Not a flash sprite"));
		return false;
	}

	m_flashObjects.m_debugHud = flashSprite;

	Bool retval = true;
	retval &= flashSprite.GetMember( TXT("tfTitle"), m_flashObjects.m_tfTitle );
	retval &= flashSprite.GetMember( TXT("tfMetadata"), m_flashObjects.m_tfMetadata );
	retval &= flashSprite.GetMember( TXT("tfCuePoint"), m_flashObjects.m_tfCuePoint );
	retval &= flashSprite.GetMember( TXT("tfSubtitles"), m_flashObjects.m_tfSubtitles );

	if ( ! retval )
	{
		ERR_ENGINE(TXT("RegisterVideoDebugHud failed to get all Flash objects!"));
	}

	m_isFlashRegistered = true;

	ClearFlashText();

	return retval;
}


void CVideoDebugHud::CancelVideo()
{
	if ( m_currentVideo )
	{
		m_currentVideo->Cancel();
		m_currentVideo->Release();
		m_currentVideo = nullptr;
	}
}

void CVideoDebugHud::CloseVideoDebugHud()
{
	if ( m_isFlashRegistered )
	{
		m_flashObjects = FlashObjects();

		if ( m_flashVideoDebugHud )
		{
			m_flashVideoDebugHud->Release();
			m_flashVideoDebugHud = nullptr;
		}

		m_isFlashRegistered = false;
		m_isVideoTitleSet = false;

		ToggleGui( true );
	}
}

void CVideoDebugHud::OpenVideoDebugHud()
{
	if ( !GGame || !GGame->GetFlashPlayer() )
	{
		ERR_ENGINE(TXT("OpenVideoDebugHud: cannot get flash player!"));
	}

	if ( !m_flashVideoDebugHud )
	{
		SFlashMovieInitParams flashMovieInitParams;
		flashMovieInitParams.m_layer = 999;
		flashMovieInitParams.m_renderGroup = eFlashMovieRenderGroup_Overlay; // to be immune from CGame::ToggleHud()
		flashMovieInitParams.m_attachOnStart = false;
		flashMovieInitParams.m_notifyPlayer = false;
		const String swfPath(TXT("engine\\swf\\videodebughud.redswf"));
		const TSoftHandle< CSwfResource > swfHandle(swfPath);
		m_flashVideoDebugHud = GGame->GetFlashPlayer()->CreateMovie( swfHandle, flashMovieInitParams );
		if ( !m_flashVideoDebugHud )
		{
			ERR_ENGINE(TXT("OpenVideoDebugHud: failed to create Flash movie %ls"), swfPath.AsChar() );
			return;
		}

		m_flashVideoDebugHud->SetExternalInterfaceOverride( this );
		m_flashVideoDebugHud->Attach();
	}

	ClearFlashText();

	ToggleGui( false );
}

Bool CVideoDebugHud::PlayVideo( const String& safeDepotPath, Uint8 extraVideoParamFlags /*= 0*/ )
{
	CancelVideo();
	OpenVideoDebugHud();

	String videoFileName = safeDepotPath.StringAfter( TXT("movies\\") ).AsChar();
	if ( videoFileName.Empty() )
	{
		CloseVideoDebugHud();
		return false;
	}

	const SVideoParams videoParams( videoFileName, eVideoParamFlag_Preemptive | extraVideoParamFlags );
	m_currentVideo = GGame->GetVideoPlayer()->CreateVideo( CNAME(VideoClient_VideoDebugHud), videoParams );
	if ( !m_currentVideo )
	{
		CloseVideoDebugHud();
		return false;
	}

	GSoundSystem->SoundSwitch( "game_state", "movie" );

	const CFilePath filePath( safeDepotPath );

	m_currentVideoTitle = filePath.GetFileNameWithExt();
	if ( (extraVideoParamFlags & eVideoParamFlag_AlternateTrack) != 0)
	{
		m_currentVideoTitle += TXT(" ALT");
	}
	else
	{
		m_currentVideoTitle += TXT(" MAIN");
	}

	m_isVideoTitleSet = false;

	return true;
}

void CVideoDebugHud::StopVideo()
{
	CancelVideo();
	CloseVideoDebugHud();
}

void CVideoDebugHud::ClearFlashText()
{
	if ( m_isFlashRegistered )
	{
		m_flashObjects.m_tfTitle.SetText( TXT("<No Video>") );
		m_flashObjects.m_tfCuePoint.SetText( TXT("<Last Cuepoint>") );
		m_flashObjects.m_tfMetadata.SetText( TXT("<Metadata>"));
		m_flashObjects.m_tfSubtitles.SetText( String::EMPTY );
	}
}

void CVideoDebugHud::ToggleGui( Bool onOff )
{
	IViewport* vp = GGame->GetViewport();
	if ( ! vp )
	{
		return;
	}

	if ( onOff )
	{
		vp->SetRenderingMask( SHOW_GUI );
	}
	else
	{
		vp->ClearRenderingMask( SHOW_GUI );
	}
}

#endif // #ifndef NO_DEBUG_PAGES