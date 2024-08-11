/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../redThreads/redThreadsAtomic.h"
#include "../core/gatheredResource.h"
#include "../core/2darray.h"
#include "flashMovie.h"
#include "flashPlayer.h"
#include "swfResource.h"
#include "renderCommands.h"
#include "loadingScreen.h"
#include "renderer.h"
#include "testFramework.h"
#include "videoPlayer.h"
#include "../core/depot.h"
#include "../core/xmlReader.h"
#include "../core/scopedPtr.h"
#include "../core/configVar.h"
#include "viewport.h"
#include "gameSession.h"
#include "baseEngine.h"

CGatheredResource resLoadingScreenPaths(TXT("gameplay\\globals\\loadingscreen_paths.csv"), 0 );
CGatheredResource resLoadingScreenSettings( TXT("gameplay\\globals\\loadingscreen_settings.xml"), 0 );

namespace Config
{
	TConfigVar< Float, Validation::FloatRange<0,5,1> > cvLoadingScreenFadeInTime( "LoadingScreen", "FadeInTime", 3.f, eConsoleVarFlag_Save );
	TConfigVar< Float, Validation::FloatRange<0,5,1> > cvLoadingScreenFadeOutTime( "LoadingScreen", "FadeOutTime", 1.5f, eConsoleVarFlag_Save );
	TConfigVar< Float, Validation::FloatRange<0,5,1> > cvLoadingScreenGameRevealTime( "LoadingScreen", "GameRevealTime", 3.f, eConsoleVarFlag_Save );
	
	// Seconds to delay input processing when the skip is shown to give time for the skip indicator to appear and avoid accidental skipping
	TConfigVar< Float, Validation::FloatRange<0,5,1> > cvLoadingScreenSkipInputDelay( "LoadingScreen", "SkipInputDelay", 0.25f, eConsoleVarFlag_Save );

	// Seconds until hiding the skip again. Zero to keep it always shown.
	TConfigVar< Float, Validation::FloatRange<0,5,1> > cvLoadingScreenSkipHideDelay( "LoadingScreen", "SkipHideDelay", 3.f, eConsoleVarFlag_Save );

	// Show the skip with any key or it must be a skip key
	TConfigVar< Bool > cvLoadingScreenSkipShowWithAnyKey( "LoadingScreen", "SkipShowWithAnyKey", true, eConsoleVarFlag_Save );

	// Show the skip automatically without user input
	TConfigVar< Bool > cvLoadingScreenSkipAutoshow( "LoadingScreen", "SkipAutoshow", false, eConsoleVarFlag_Save );

	TConfigVar< Bool > cvLoadingScreenEditorDisabled( "LoadingScreen/Editor", "Disabled", false, eConsoleVarFlag_Save );
	TConfigVar< Bool > cvLoadingScreenDebugDisableForceLoads( "LoadingScreen/Debug", "DisableForceLoads", false, eConsoleVarFlag_Developer );
	TConfigVar< Bool > cvLoadingScreenDebugDisableShow( "LoadingScreen/Debug", "DisableShow", false, eConsoleVarFlag_Developer );
	TConfigVar< Bool > cvLoadingScreenDebugDisableVideos( "LoadingScreen/Debug", "DisableVideos", false, eConsoleVarFlag_Developer );
}

//////////////////////////////////////////////////////////////////////////
// SLoadingScreenParam
//////////////////////////////////////////////////////////////////////////
const SLoadingScreenParam SLoadingScreenParam::DEFAULT( CName(TXT("default")) );
const SLoadingScreenParam SLoadingScreenParam::BLACKSCREEN( CName(TXT("blackscreen")), false );

//////////////////////////////////////////////////////////////////////////
// CLoadingScreen
//////////////////////////////////////////////////////////////////////////

const String CLoadingScreen::IGNORED_VIDEO_NAME = TXT("none");

CLoadingScreen::CLoadingScreen()
	: m_fence( nullptr )
	, m_renderVideo( nullptr )
	, m_saveLock( CGameSessionManager::GAMESAVELOCK_INVALID )
{
}

CLoadingScreen::~CLoadingScreen()
{
	if ( m_renderVideo && m_renderVideo->IsValid() )
	{
		m_renderVideo->Cancel();
		m_renderVideo->Release();
		m_renderVideo = nullptr;
	}

	if ( m_fence )
	{
		( new CRenderCommand_FadeOutLoadingScreen( m_fence, 0.f ) )->Commit();
		m_fence->Release();
		m_fence = nullptr;
	}

	// Sending a null fence so non-interactive rendering will be processed again and actually release any fence held by the render thread.
	// Don't check if m_fence currently nullptr because we may have already released the fence and nulled it out on this side, but the render thread
	// can still have one
	( new CRenderCommand_SetLoadingScreenFence( nullptr, 0.f ) )->Commit();

	GRender->Flush();
}

Bool CLoadingScreen::Init()
{
	if ( ! InitLoadingScreenMap() )
	{
		return false;
	}

	if ( !ParseLoadingScreenSettings() )
	{
		WARN_ENGINE(TXT("Failed to parse loading screen settings."));

		// Set some default skip keys
		m_videoSkipKeys.Set( IK_Pad_X_SQUARE );
		m_videoSkipKeys.Set( IK_RightMouse );
		m_videoSkipKeys.Set( IK_Enter );
		m_videoSkipKeys.Set( IK_Space );

		return false;
	}

	return true;
}

Bool CLoadingScreen::InitLoadingScreenMap()
{
	C2dArray* ar = resLoadingScreenPaths.LoadAndGet<C2dArray>();
	if ( !ar )
	{
		ERR_ENGINE(TXT("CLoadingScreen failed to find settings CSV"));
		return false;
	}

	Uint32 nameCol = 0;
	if ( !ar->FindHeader( TXT("Name"), nameCol ) )
	{
		ERR_ENGINE(TXT("CLoadingScreen CSV is missing Name column"));
		return false;
	}

	Uint32 pathCol = 0;
	if ( !ar->FindHeader( TXT("LoadingScreen"), pathCol ) )
	{
		ERR_ENGINE(TXT("CLoadingScreen CSV is missing LoadingScreen column"));
		return false;
	}

	for ( Uint32 i = 0; i < ar->GetNumberOfRows(); ++i )
	{
		const CName name( ar->GetValueRef( nameCol, i ) );

		String path;
		CFilePath::GetConformedPath( ar->GetValueRef( pathCol, i ), path );

		if ( !m_loadingScreenMap.Insert( name, path) )
		{
			WARN_ENGINE(TXT("Duplicate loading screen name %ls"), name.AsChar() );
		}
	}

	return true;
}

Bool CLoadingScreen::ParseLoadingScreenSettings()
{
	Red::TScopedPtr< CXMLReader > xml( GDepot->LoadXML( resLoadingScreenSettings.GetPath().ToString() ) ); 

	struct SScopedNode : Red::System::NonCopyable
	{
		CXMLReader&	m_xml;
		Bool		m_found;

		SScopedNode( CXMLReader& xml, const String& name )
			: m_xml( xml )
		{
			m_found = m_xml.BeginNode( name );
		}

		~SScopedNode()
		{
			if ( m_found )
			{
				m_xml.EndNode();
			}
		}

		operator Bool const() { return m_found; }
	};

	if ( !xml )
	{
		ERR_ENGINE(TXT("CLoadingScreen failed to load settings XML"));
		return false;
	}

	m_videoSkipKeys.ClearAll();

	SScopedNode loadingScreenNode(*xml, TXT("LoadingScreen"));
	if ( loadingScreenNode )
	{
		// <Video>
		{
			SScopedNode videNode(*xml, TXT("Video"));
			if ( videNode )
			{
				const CEnum* inputKeyEnum = SRTTI::GetInstance().FindEnum( CNAME( EInputKey ) );
				if ( !inputKeyEnum )
				{
					ERR_ENGINE(TXT("ParseLoadingScreenSettings: failed to find EInputKey enum RTTI"));
					return false;
				}

				for (;;)
				{
					SScopedNode skipKeyNode(*xml, TXT("SkipKey"));
					if ( ! skipKeyNode )
					{
						break;
					}

					String tmp;
					if ( !xml->Value( tmp ) )
					{
						ERR_ENGINE(TXT("ParseLoadingScreenSettings failed to parse node SkipKey"));
						return false;
					}

					const CName keyName( tmp );
					Int32 skipKey = IK_None;
					if ( ! inputKeyEnum->FindValue( keyName, skipKey ) )
					{
						ERR_ENGINE(TXT("ParseLoadingScreenSettings failed to find input key '%ls'"), keyName.AsString().AsChar() );
						return false;
					}
					if ( skipKey <= IK_None || skipKey >= IK_Count )
					{
						ERR_ENGINE(TXT("Invalid input key '%ls'"), keyName.AsString().AsChar() );
						return false;
					}

					m_videoSkipKeys.Set( skipKey );
				}
			}
		} // </Video>
	}

	return true;
}

Bool CLoadingScreen::Show( const SLoadingScreenParam& param )
{
	String* path = m_loadingScreenMap.FindPtr( param.m_contextName );
	if ( !path )
	{
		WARN_ENGINE(TXT("CLoadingScreen::Show: Using default loading screen. No SWF for %ls in loading screen CSV"), param.m_contextName.AsChar() )
		path = m_loadingScreenMap.FindPtr( CNAME(default) );
	}

	if ( !path )
	{
		ERR_ENGINE(TXT("CLoadingScreen::ShowLocal: No SWF for %ls in loading screen CSV"), param.m_contextName.AsChar() );
		path = &String::EMPTY;
		// Keep going and do non-interactive rendering. Don't totally fail because of Flash
	}

	return ShowWithSwf( *path, param.m_contextName.AsChar(), param );
}

Bool CLoadingScreen::ShowWithSwf( const String& swfPath, const String& name, const SLoadingScreenParam& param )
{
	if ( ! AllowLoadingScreenStart() )
	{
		return false;
	}

	PrepareForLoadingScreenStart();

	if ( m_fence )
	{
		m_fence->Release();
		m_fence = nullptr;
	}

	if ( m_renderVideo )
	{
		m_renderVideo->Cancel();
		m_renderVideo->Release();
		m_renderVideo = nullptr;
	}

	const Bool emptyOrIgnored = ( param.m_videoToPlay.Empty() || param.m_videoToPlay == IGNORED_VIDEO_NAME );
	const Bool hideAtStart = !emptyOrIgnored && !Config::cvLoadingScreenDebugDisableVideos.Get();
	// The null renderer won't create a fence
	m_fence = CreateLoadingScreenFence( param.m_initString, swfPath, name );
	if ( m_fence )
	{
		const Float fadeInTime = Config::cvLoadingScreenFadeInTime.Get();
		( new CRenderCommand_SetLoadingScreenFence( m_fence, fadeInTime, hideAtStart ) )->Commit();
	}

	if ( !emptyOrIgnored )
	{
		PlayVideo( param.m_videoToPlay );
	}

	SGameSessionManager::GetInstance().CreateNoSaveLock( TXT("LoadingScreen"), m_saveLock );

	return true;
}

Bool CLoadingScreen::StartFadeOut()
{
	if ( ! m_fence )
	{
		return false;
	}

	FadeInBlackscreenIfNeeded();

	const Float fadeOutTime = Config::cvLoadingScreenFadeOutTime.Get();
	( new CRenderCommand_FadeOutLoadingScreen( m_fence, fadeOutTime ) )->Commit();

	SGameSessionManager::GetInstance().ReleaseNoSaveLock( m_saveLock );
	m_saveLock = CGameSessionManager::GAMESAVELOCK_INVALID;

	return true;
}

void CLoadingScreen::FadeInBlackscreenIfNeeded()
{
	if ( ! GGame )
	{
		return;
	}

	if ( GGame->IsCurrentlyPlayingNonGameplayScene() )
	{
		LOG_ENGINE(TXT("Loading screen skipping own fade in. Non gameplay scene active"));
	}
	else
	{
		if ( !GGame->HasBlackscreenRequested() )
		{
			LOG_ENGINE(TXT("Loading screen fade in"));
			GGame->SetBlackscreen( true, TXT("Loading screen end blackscreen") );
			const Float gameRevealTime = Config::cvLoadingScreenGameRevealTime.Get();
			GGame->StartFade( true, TXT("Loading screen fade in"), gameRevealTime );
		}
		else
		{
			LOG_ENGINE(TXT("Loading screen skipping own fade in. Current fade or blackscreen reasons: ON %ls, OFF: %ls'"),
				GGame->GetBlackscreenOnReason().AsChar(),
				GGame->GetBlackscreenOffReason().AsChar() );
		}
	}
}

ILoadingScreenFence* CLoadingScreen::CreateLoadingScreenFence( const String& initString, const String& swfPath, const String& name )
{
	if ( !GGame || !GGame->GetFlashPlayer() )
	{
		ERR_ENGINE(TXT("CreateLoadingScreenFence: Flash player not available"));
		// Keep going and do non-interactive rendering. Don't totally fail because of Flash
	}

	SFlashMovieInitParams flashMovieInitParams;
	flashMovieInitParams.m_layer = eFlashMovieOverlayDepth_LoadingScreen;
	flashMovieInitParams.m_renderGroup = eFlashMovieRenderGroup_Overlay;
	flashMovieInitParams.m_attachOnStart = false;
	flashMovieInitParams.m_notifyPlayer = false;
	flashMovieInitParams.m_waitForLoadFinish = true;
	const TSoftHandle< CSwfResource > swfHandle( swfPath );
	CFlashMovie* flashMovie = ( GGame && GGame->GetFlashPlayer() ) ? GGame->GetFlashPlayer()->CreateMovie( swfHandle, flashMovieInitParams ) : nullptr;
	if ( !flashMovie )
	{
		ERR_ENGINE(TXT("CreateLoadingScreenFence: failed to create Flash movie %ls"), swfPath.AsChar() );
		// Keep going and do non-interactive rendering. Don't totally fail because of Flash
	}

	SLoadingScreenFenceInitParams loadingSceenFenceInitParams( flashMovie, GGame->GetViewport(), initString, name );

	return GRender->CreateLoadingScreenFence( loadingSceenFenceInitParams );
}

Bool CLoadingScreen::IsShown() const
{
	if ( m_renderVideo && !m_renderVideo->IsValid() )
	{
		m_renderVideo->Release();
		m_renderVideo = nullptr;
	}

	if ( m_fence && !m_fence->IsActive() )
	{
		m_fence->Release();
		m_fence = nullptr;
	}

	RED_FATAL_ASSERT( !m_renderVideo || m_fence, "Render video without a fence!" );
	return m_fence != nullptr;
}

Bool CLoadingScreen::IsPlayingVideo() const
{
	if ( m_renderVideo && !m_renderVideo->IsValid() )
	{
		m_renderVideo->Release();
		m_renderVideo = nullptr;
	}

	return m_renderVideo != nullptr;
}

Bool CLoadingScreen::PlayVideo( const String& videoFile )
{
	if ( !m_fence )
	{
		return false;
	}

	if ( m_renderVideo )
	{
		return false;
	}

	if ( Config::cvLoadingScreenDebugDisableVideos.Get() )
	{
		LOG_ENGINE(TXT("CLoadingScreen cvLoadingScreenDebugDisableVideos: 1"));
		return false;
	}


	// Bit of a hack, but don't want to expose this too generically at the game level
	// FIXME: enable when I know which storybook video should need this!!
	const Bool isStorybookVideo =  false && videoFile.BeginsWith(TXT("cutscenes\\storybook\\"));
	
	//FIXME: Need to know from the journal!
	const Bool useAltTrack = false && isStorybookVideo;

	Uint8 extraFlags = 0;
	if ( isStorybookVideo )
	{
		extraFlags |= eVideoParamFlag_ExternalSubs;
	}
	if ( useAltTrack )
	{
		extraFlags |= eVideoParamFlag_AlternateTrack;
	}
	
	SVideoParams videoParams( videoFile, eVideoParamFlag_Preemptive | extraFlags, eVideoBuffer_Long );
	m_renderVideo = GRender->CreateVideo( CNAME(LoadingVideo), videoParams );
	( new CRenderCommand_PlayLoadingScreenVideo( m_renderVideo, m_fence ) )->Commit();

	return true;
}

Bool CLoadingScreen::ToggleVideoSkip( Bool enabled )
{
	if ( !m_fence )
	{
		return false;
	}

	( new CRenderCommand_ToggleLoadingVideoSkip( m_fence, enabled ) )->Commit();

	return true;
}

Bool CLoadingScreen::IsVideoSkipKey( EInputKey key ) const
{
	return m_videoSkipKeys.Get( key );
}

Bool CLoadingScreen::SetPCInput( Bool enabled )
{
	if ( !m_fence )
	{
		return false;
	}

	( new CRenderCommand_SetLoadingPCInput( m_fence, enabled ) )->Commit();

	return true;
}

Bool CLoadingScreen::SetExpansionsAvailable( Bool ep1, Bool ep2 )
{
	if ( !m_fence )
	{
		return false;
	}

	( new CRenderCommand_SetExpansionsAvailable( m_fence, ep1, ep2 ) )->Commit();

	return true;
}

const String& CLoadingScreen::GetIgnoredVideoName()
{
	return IGNORED_VIDEO_NAME;
}

Bool CLoadingScreen::AllowLoadingScreenStart() const
{
#ifndef NO_TEST_FRAMEWORK
	if ( STestFramework::GetInstance().IsActive() )
	{
		return  STestFramework::GetInstance().AllowVideos();
	}
#endif
	if ( GIsCooker )
	{
		return false;
	}

	if ( Config::cvLoadingScreenDebugDisableShow.Get() )
	{
		LOG_ENGINE(TXT("CLoadingScreen cvLoadingScreenDebugDisableShow: 1"));
		return false;
	}

	if ( GIsEditor )
	{
		const Bool loadingScreenInEditor = !Config::cvLoadingScreenEditorDisabled.Get();
		LOG_ENGINE(TXT("CLoadingScreen loadingScreenInEditor: %d"), loadingScreenInEditor );
		return loadingScreenInEditor;
	}

	return true;
}

void CLoadingScreen::PrepareForLoadingScreenStart()
{
	if ( GIsEditor )
	{
		// This or we create the HWND on the render thread and pump messages there again. But we need to make
		// the updates thread safe, which they weren't. Doing it here so the editor can show a ghost window
		// unless you're using loading screen. Once disabled, there's no reenabling windows ghosting.
#ifdef RED_PLATFORM_WINPC
		::DisableProcessWindowsGhosting();
#endif
	}
}
