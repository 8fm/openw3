/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "../core/fileSystemProfilerWrapper.h"
#include "../core/depot.h"
#include "../core/gatheredResource.h"
#include "../engine/localizationManager.h"
#include "../engine/viewport.h"
#include "../engine/game.h"
#include "../engine/guiGlobals.h"
#include "../engine/renderer.h"
#include "../engine/swfResource.h"
#include "../engine/flashValueObject.h"
#include "../engine/renderScaleform.h"
#include "../engine/renderScaleformCommands.h"
#include "../engine/renderFrame.h"
#include "../engine/baseEngine.h"
#include "../engine/renderSettings.h"
#include "../core/depot.h"

#include "renderVideoPlayer.h"
#include "renderVideo.h"
#include "../core/2darray.h"
#include "../core/tokenizer.h"

#if !defined( RED_FINAL_BUILD ) || defined( RED_PROFILE_BUILD )
//# define PROFILE_VIDEO
#endif

#ifdef PROFILE_VIDEO
# ifdef RED_PLATFORM_ORBIS
# include <perf.h>
static Bool GVideoPerfCapture;
static void start_video_perf_capture()
{
	if ( !GVideoPerfCapture )
	{
		const Int32 result = ::sceRazorCpuStartCapture();
		if ( result == SCE_OK )
		{
			GVideoPerfCapture = true;
		}
	}
}

static void end_video_perf_capture()
{
	if ( GVideoPerfCapture )
	{
		::sceRazorCpuStopCapture();
		::sceKernelUsleep( 10 * 1000000 ); // recommended on devnet forums to give razor a chance to detach if the process will exit soon
		GVideoPerfCapture = false;
	}
}

#define START_VIDEO_PERF_CAPTURE( videoName )\
do\
{\
	const Char* const videoNameTxt = MACRO_TXT(#videoName);\
	if ( m_playingVideo.GetVideoParams().m_fileName.ContainsSubstring( videoNameTxt ) )\
	{\
		start_video_perf_capture();\
	}\
} while( 0 )

#define END_VIDEO_PERF_CAPTURE()\
do\
{\
	end_video_perf_capture();\
} while( 0 )

# endif // RED_PLATFORM_ORBIS
# else
#  define START_VIDEO_PERF_CAPTURE( videoName )
#  define END_VIDEO_PERF_CAPTURE()
#endif // PROFILE_VIDEO

namespace Config
{
	//const Char* const INTRO_USM = TXT("cutscenes\\pre_rendered_cutscenes\\intro.usm");
	TConfigVar< Int32 > cvDebugForceVoiceTrack( "Video/Debug", "ForceVoiceTrack", -1, eConsoleVarFlag_Developer );
	TConfigVar< Int32 > cvDebugForceSubtitlesTrack( "Video/Debug", "ForceSubtitlesTrack", -1, eConsoleVarFlag_Developer );
	TConfigVar< Bool > cvDebugUseOverrideBuffer( "Video/Debug", "UseOverrideBuffer", false, eConsoleVarFlag_Developer );
	TConfigVar< Bool > cvDebugDisableVideoAudio( "Video/Debug", "DisableAudio", false, eConsoleVarFlag_Developer );
	TConfigVar< Bool > cvDebugDisableVideoLoop( "Video/Debug", "DisableLoop", false, eConsoleVarFlag_Developer );
	TConfigVar< String > cvDebugForceVideoFile("Video/Debug", "ForceVideoFile", String::EMPTY, eConsoleVarFlag_Developer );

	// Default video buffer params
	TConfigVar< Float > cvVideoBuffer_Default_BufferTime( "Video/Buffer/Default", "BufferTime", .5f );
	TConfigVar< Float > cvVideoBuffer_Default_ReloadThresholdTime( "Video/Buffer/Default", "ReloadThesholdTime", .3f );
	TConfigVar< Int32 > cvVideoBuffer_Default_NumberOfFramePools( "Video/Buffer/Default", "NumberOfFramePools", 2 );

	// Background video buffer params
	TConfigVar< Float > cvVideoBuffer_Background_BufferTime( "Video/Buffer/Background", "BufferTime", 1.f );
	TConfigVar< Float > cvVideoBuffer_Background_ReloadThresholdTime( "Video/Buffer/Background", "ReloadThesholdTime", .3f );
	TConfigVar< Int32 > cvVideoBuffer_Background_NumberOfFramePools( "Video/Buffer/Background", "NumberOfFramePools", 2 );

	// Loading video buffer params
	TConfigVar< Float > cvVideoBuffer_Loading_BufferTime( "Video/Buffer/Loading", "BufferTime", 5.f );
	TConfigVar< Float > cvVideoBuffer_Loading_ReloadThresholdTime( "Video/Buffer/Loading", "ReloadThesholdTime", 2.f );
	TConfigVar< Int32 > cvVideoBuffer_Loading_NumberOfFramePools( "Video/Buffer/Loading", "NumberOfFramePools", 1 );

	// Bumpers video buffer params, try to buffer USM in one shot without reloading and potential I/O contention
	// Read in up to max possible bumper length for TRC compliance in case somebody goes with one merged mega-bumper USM.
	TConfigVar< Float > cvVideoBuffer_Bumpers_BufferTime( "Video/Buffer/Bumpers", "BufferTime", 60.f );
	TConfigVar< Float > cvVideoBuffer_Bumpers_ReloadThresholdTime( "Video/Buffer/Bumpers", "ReloadThesholdTime", .001f );
	TConfigVar< Int32 > cvVideoBuffer_Bumpers_NumberOfFramePools( "Video/Buffer/Bumpers", "NumberOfFramePools", 3 );

	// Override video buffer params. Mostly just for debug/experimentation
	TConfigVar< Float > cvVideoBuffer_Override_BufferTime( "Video/Buffer/Override", "BufferTime", 1.f );
	TConfigVar< Float > cvVideoBuffer_Override_ReloadThresholdTime( "Video/Buffer/Override", "ReloadThesholdTime", 1.f );
	TConfigVar< Int32 > cvVideoBuffer_Override_NumberOfFramePools( "Video/Buffer/Override", "NumberOfFramePools", 1 );
}

static void GetVideoBufferParams( EVideoBuffer buffer, Float& outBufferTime, Float& outReloadThresholdTime, Int32& outNumberOfFramePools )
{
	if( Config::cvDebugUseOverrideBuffer.Get() )
	{
		outBufferTime = Config::cvVideoBuffer_Override_BufferTime.Get();
		outReloadThresholdTime = Config::cvVideoBuffer_Override_ReloadThresholdTime.Get();
		outNumberOfFramePools = Config::cvVideoBuffer_Override_NumberOfFramePools.Get();
		return;
	}

	switch( buffer )
	{
	case eVideoBuffer_Short:
		{
			outBufferTime = Config::cvVideoBuffer_Background_BufferTime.Get();
			outReloadThresholdTime = Config::cvVideoBuffer_Background_ReloadThresholdTime.Get();
			outNumberOfFramePools = Config::cvVideoBuffer_Background_NumberOfFramePools.Get();
		}
		break;
	case eVideoBuffer_Long:
		{
			outBufferTime = Config::cvVideoBuffer_Loading_BufferTime.Get();
			outReloadThresholdTime = Config::cvVideoBuffer_Loading_ReloadThresholdTime.Get();
			outNumberOfFramePools = Config::cvVideoBuffer_Loading_NumberOfFramePools.Get();
		}
		break;
	case eVideoBuffer_Bumpers:
		{
			outBufferTime = Config::cvVideoBuffer_Bumpers_BufferTime.Get();
			outReloadThresholdTime = Config::cvVideoBuffer_Bumpers_ReloadThresholdTime.Get();
			outNumberOfFramePools = Config::cvVideoBuffer_Bumpers_NumberOfFramePools.Get();
		}
		break;
	case eVideoBuffer_Default: /* fall through */
	default:
		{
			outBufferTime = Config::cvVideoBuffer_Default_BufferTime.Get();
			outReloadThresholdTime = Config::cvVideoBuffer_Default_ReloadThresholdTime.Get();
			outNumberOfFramePools = Config::cvVideoBuffer_Default_NumberOfFramePools.Get();
		}
		break;
	}
}

CGatheredResource resVideoLanguages( TXT("engine\\misc\\video_languages.csv"), RGF_Startup );

//////////////////////////////////////////////////////////////////////////
// CRenderVideoPlayer
//////////////////////////////////////////////////////////////////////////
CRenderVideoPlayer::SFlashFuncImportDesc CRenderVideoPlayer::sm_flashFunctionImportTable[] = {
	{ TXT("_HOOK_playVideo"), &CRenderVideoPlayer::m_playVideoHook },
	{ TXT("_HOOK_stopVideo"), &CRenderVideoPlayer::m_stopVideoHook },
	{ TXT("_HOOK_pauseVideo"), &CRenderVideoPlayer::m_pauseVideoHook },
	{ TXT("_HOOK_unpauseVideo"), &CRenderVideoPlayer::m_unpauseVideoHook },
	{ TXT("_HOOK_clearVideo"), &CRenderVideoPlayer::m_clearVideoHook },
	{ TXT("_HOOK_showBackground"), &CRenderVideoPlayer::m_showBackgroundHook },
	{ TXT("_HOOK_hideBackground"), &CRenderVideoPlayer::m_hideBackgroundHook },
};

CRenderVideoPlayer::SFlashFuncExportDesc CRenderVideoPlayer::sm_flashFunctionExportTable[] = {
	{ TXT("_NATIVE_onVideoEvent"), &CRenderVideoPlayer::flFnOnVideoEvent },
	{ TXT("_NATIVE_onVideoSubtitles"), &CRenderVideoPlayer::flFnOnVideoSubtitles },
	{ TXT("_NATIVE_onVideoCuePoint"), &CRenderVideoPlayer::flFnOnVideoCuePoint},
	{ TXT("_NATIVE_onVideoMetaData"), &CRenderVideoPlayer::flFnOnVideoMetaData },
};

CRenderVideoPlayer::CRenderVideoPlayer()
	: m_videoFlash( nullptr )
	, m_isFlashRegistered( false )
	, m_isAllVideosPaused( false )
	, m_isShowingBackground( false )
	, m_masterVolumePercent( 1.f )
	, m_voiceVolumePercent( 1.f )
	, m_effectsVolumePercent( 1.f )
{
	class CDeferredInit_VideoPlayer : public IDeferredInitDelegate
	{
	public:
		CRenderVideoPlayer* mPlayer;
		CDeferredInit_VideoPlayer(CRenderVideoPlayer* player) : mPlayer(player) {}
		void OnBaseEngineInit()
		{
			mPlayer->InitLanguageTracks();
		}
	};


	if (GDeferredInit)
	{
		GDeferredInit->AddDelegate(new CDeferredInit_VideoPlayer(this));
	}
	else
	{
		InitLanguageTracks();
	}
}

void CRenderVideoPlayer::InitWithFlash( CFlashMovie* videoFlash )
{
	if ( videoFlash )
	{
		RED_FATAL_ASSERT( ! m_videoFlash, "Flash already initialized!" );
		m_videoFlash = videoFlash;
		m_videoFlash->UpdateCaptureThread();
		m_videoFlash->SetExternalInterfaceOverride( this );
		m_videoFlash->Attach();
		InitFlashFunctions();
	}
	else
	{
		Shutdown();
	}
}

void CRenderVideoPlayer::Shutdown()
{
	if ( m_playingVideo.IsValid() )
	{
		m_playingVideo.m_renderVideo->Stop();
	}

	for ( Uint32 i = 0; i < eVideoThreadIndex_Count; ++i )
	{
		TDynArray< SVideoContext >& videoPlayQueue = m_threadData[ i ].m_videoPlayQueue;
		for ( const SVideoContext& videoContext : videoPlayQueue )
		{
			videoContext.m_renderVideo->Stop();
		}
		videoPlayQueue.Clear();
	}

	CleanupVideoFlash();

	if ( m_videoFlash )
	{
		m_videoFlash->UpdateCaptureThread();
		m_videoFlash->SetUserData( nullptr );
		RED_VERIFY( m_videoFlash->Release() == 0 );
		m_videoFlash = nullptr;
	}
}

CRenderVideoPlayer::~CRenderVideoPlayer()
{
	//Shutdown();
}

void CRenderVideoPlayer::InitLanguageTracks()
{
	THandle< C2dArray > ar = resVideoLanguages.LoadAndGet< C2dArray >();
	if ( ! ar )
	{
		VIDEO_ERROR(TXT("CRenderVideoPlayer: Failed to load language track settings!"));
		return;
	}

	struct SCsvMap
	{
		const Char*		m_header;
		Uint32*			m_column;
	};

	struct ConvertTrack
	{
		Int32 operator()( const String& val ) const 
		{
			Int32 track = SLanguageTrackInfo::INVALID_TRACK;
			::FromString( val, track );

			return track;
		}
	};

	Uint32 colLanguage;
	Uint32 colVoiceTrack;
	Uint32 colSubtitlesTrack;
	Uint32 colAltVoiceTrack;
	Uint32 colAltSubtitlesTrack;
	SCsvMap headers[] = { { TXT("Language"), &colLanguage },
						  { TXT("VoiceTrack"), &colVoiceTrack }, { TXT("SubtitlesTrack"), &colSubtitlesTrack},
						  { TXT("AltVoiceTrack"), &colAltVoiceTrack }, { TXT("AltSubtitlesTrack"), &colAltSubtitlesTrack },
	};

	for ( Uint32 i = 0; i < ARRAY_COUNT_U32(headers); ++i )
	{
		const SCsvMap& csvMap = headers[i];
		if ( !ar->FindHeader( csvMap.m_header, *(csvMap.m_column) ) )
		{
			VIDEO_ERROR(TXT("CRenderVideoPlayer: Failed to load language track settings! Can't find header %ls"), csvMap.m_header );
		}
	}

	ConvertTrack convTrack;
	for ( Uint32 i = 0; i < ar->GetNumberOfRows(); ++i )
	{
		const String& language = ar->GetValueRef( colLanguage, i );
		SLanguageTrackInfo trackInfo;

		trackInfo.m_voiceTrack = convTrack( ar->GetValue( colVoiceTrack, i ) );
		trackInfo.m_subtitlesTrack = convTrack( ar->GetValue( colSubtitlesTrack, i ) );
		trackInfo.m_altVoiceTrack = convTrack( ar->GetValue( colAltVoiceTrack, i ) );
		trackInfo.m_altSubtitlesTrack = convTrack( ar->GetValue( colAltSubtitlesTrack, i ) );

		if ( !m_languageTracks.Insert( language.ToUpper(), trackInfo ) )
		{
			VIDEO_WARN(TXT("CRenderVideoPlayer: langauge %ls already mapped to tracks!"), language.AsChar() );
		}
	}
}

void CRenderVideoPlayer::SetMasterVolumePercent( Float percent )
{
	m_masterVolumePercent = percent;
	UpdateFlashSound();	
}

void CRenderVideoPlayer::SetVoiceVolumePercent( Float percent )
{
	m_voiceVolumePercent = percent;
	UpdateFlashSound();
}

void CRenderVideoPlayer::SetEffectsVolumePercent( Float percent )
{
	m_effectsVolumePercent = percent;
	UpdateFlashSound();
}

void CRenderVideoPlayer::UpdateFlashSound()
{
	if ( m_isFlashRegistered )
	{
		extern Float GHackVideoSFXVolume;
		extern Float GHackVideoVoiceVolume;

		Float soundTransformVolume = 1.f;
		Float subSoundTransformVolume = 1.f;

		if ( !IsPlayingBumpers() )
		{
			 // Don't change volume in the middle of bumpers if user settings load and change it
			soundTransformVolume = m_effectsVolumePercent * m_masterVolumePercent;
			subSoundTransformVolume = m_voiceVolumePercent * m_masterVolumePercent;

			// Hack. Sorry, but sounds terrible if you adjust separately.
			if ( m_playingVideo.IsValid() && m_playingVideo.GetVideoParams().m_fileName.ContainsSubstring(TXT("intro.usm")) )
			{
				subSoundTransformVolume = soundTransformVolume;
			}
		}

		// Have to reset the soundTransform for the update to occur, because, well, Flash.
		// Set it before updating the Flash, which will trigger its own update
		// When using 5.1ch, it doesn't use a subaudio sounstream, so actually doesn't use the subSoundTransform.
		// We've modified the videoSoundSystems to work around this, but it has to go around the Flash shit to do so
		GHackVideoSFXVolume = soundTransformVolume;
		GHackVideoVoiceVolume = subSoundTransformVolume;

		RED_VERIFY( m_flashSoundTransform.SetMemberFlashNumber( TXT("volume"), soundTransformVolume ) );
		RED_VERIFY( m_flashNetStream.SetMemberFlashValue( TXT("soundTransform"), m_flashSoundTransform ) );

		RED_VERIFY( m_flashSubSoundTransform.SetMemberFlashNumber( TXT("volume"), subSoundTransformVolume ) );
		RED_VERIFY( m_flashNetStream.SetMemberFlashValue( TXT("subSoundTransform"), m_flashSubSoundTransform ) );
	}
}

void CRenderVideoPlayer::MuteFlashSound()
{
	if ( m_isFlashRegistered )
	{
		// Have to reset the soundTransform for the update to occur, because, well, Flash.
		RED_VERIFY( m_flashSoundTransform.SetMemberFlashNumber( TXT("volume"), 0.f ) );
		RED_VERIFY( m_flashNetStream.SetMemberFlashValue( TXT("soundTransform"), m_flashSoundTransform ) );

		RED_VERIFY( m_flashSubSoundTransform.SetMemberFlashNumber( TXT("volume"), 0.f ) );
		RED_VERIFY( m_flashNetStream.SetMemberFlashValue( TXT("subSoundTransform"), m_flashSubSoundTransform ) );
	}
}

void CRenderVideoPlayer::InitFlashFunctions()
{
	PC_SCOPE( InitFlashFunctions );

	ASSERT( m_videoFlash );
	if ( ! m_videoFlash )
	{
		return;
	}

	const Uint32 len = ARRAY_COUNT_U32( sm_flashFunctionExportTable );
	for ( Uint32 i = 0; i < len; ++i )
	{
		CFlashFunction* flashFunction = CreateFlashFunction( sm_flashFunctionExportTable[i].m_flashFuncExport );
		RED_ASSERT( flashFunction );
		if ( flashFunction )
		{
			VERIFY( m_flashFunctionMap.Insert( sm_flashFunctionExportTable[i].m_memberName, flashFunction ) );
		}
	}
}

void CRenderVideoPlayer::DiscardFlashFunctions()
{
	PC_SCOPE( DiscardFlashFunctions );

	//CHANGEME: Could do with smart pointers...
	for ( TFlashFunctionMap::const_iterator it = m_flashFunctionMap.Begin(); it != m_flashFunctionMap.End(); ++it )
	{
		CFlashFunction* flashFunction = it->m_second;
		if ( flashFunction )
		{
			VERIFY( flashFunction->Release() == 0 ); 
		}
	}
	m_flashFunctionMap.Clear();
}

CFlashFunction* CRenderVideoPlayer::CreateFlashFunction( TFlashFunctionExport flashFunc )
{
	PC_SCOPE( CreateFlashFunction );

	ASSERT( m_videoFlash );
	if ( ! m_videoFlash )
	{
		return nullptr;
	}

	CFlashFunctionHandler* flashFunctionHandler = Flash::CreateFunctionHandler( this, flashFunc );
	ASSERT( flashFunctionHandler );
	if ( ! flashFunctionHandler )
	{
		return nullptr;
	}

	CFlashFunction* flashFunction = m_videoFlash->CreateFunction( flashFunctionHandler );
	ASSERT( flashFunction );
	flashFunctionHandler->Release(); // CFlashFunction addref'd

	return flashFunction;
}

void CRenderVideoPlayer::OnFlashExternalInterface( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args, CFlashValue& outRetval )
{
	if ( methodName.EqualsNC( TXT("registerVideoPlayer") ) )
	{
		const Uint32 argCount = args.Size();
		const Uint32 numExpectedArgs = 2;
		if ( argCount != numExpectedArgs )
		{
			VIDEO_ERROR( TXT("registerVideoPlayer: expected '%u' arguments, but received '%u'."), numExpectedArgs, argCount );
			return;
		}

		if ( ! args[0].IsFlashDisplayObject() )
		{
			VIDEO_ERROR( TXT("registerVideoPlayer: Expected arg[0] to be a Flash DisplayObject") );
			return;
		}

		if ( ! args[1].IsInstanceOf( TXT("flash.net.NetStream") ) )
		{
			VIDEO_ERROR( TXT("registerVideoPlayer: Expected arg[1] to be a Flash NetStream") );
			return;
		}

		CFlashValue flashVideoPlayer = args[0];
		CFlashValue flashNetStream = args[1];
		RegisterFlashVideoPlayer( flashVideoPlayer, flashNetStream );
	}
	else
	{
		VIDEO_ERROR( TXT("Unexpected external interface callback '%ls'"), methodName.AsChar() );
	}
}

// FIXME: should pass in from Flash instead of reparsing out
static Bool ParseNumAudioTracks( const String& metadata, Uint32& numAudioTracks )
{
	if ( metadata.Empty() )
	{
		WARN_RENDERER(TXT("ParseNumAudioTracks failed: metadata empty"));
		return false;
	}

	CTokenizer tokenizer( metadata, TXT(", ") );
	for ( Uint32 i = 0; i < tokenizer.GetNumTokens(); ++i )
	{
		String token = tokenizer.GetToken( i );
		if ( token.BeginsWith(TXT("audioTracksCount:") ) )
		{
			if ( i + 1 >= tokenizer.GetNumTokens() )
			{
				WARN_RENDERER(TXT("ParseNumAudioTracks failed: audioTracksCount incomplete"));
				return false;
			}
			token = tokenizer.GetToken( i + 1 );
			const Char* pch = token.AsChar();
			if ( ! GParseInteger( pch, numAudioTracks ) )
			{
				WARN_RENDERER(TXT("ParseNumAudioTracks failed: audioTracksCount incomplete"));
				return false;
			}

			return true;
		}
	}

	return false;
}

void CRenderVideoPlayer::Tick( Float timeDelta )
{
	PC_SCOPE_PIX( CRenderVideoPlayer_RenderTick );
	Rect viewportRect = Rect::EMPTY;

	IViewport* viewport = GGame ? GGame->GetViewport() : QuickBoot::g_quickInitViewport.Get();
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

	// When alt-tabbed out the viewport is empty but we need to pause the video to avoid
	// the speed-up when returning
	const Bool forceTick = m_playingVideo.m_pauseRequest != PauseRequest_None;

	// However, we can't just tick if the viewport is empty because we'll have to render frame
	// to process the image update requests and eventually OOM! Perhaps the "real fix" is to modify
	// SF video and not perform another update request if the previous one isn't finished yet, but
	// not totally sure how that'll work properly with video frame skipping right now.
	if ( m_videoFlash && (!viewportRect.IsEmpty() || forceTick ) )
	{
		PC_SCOPE_PIX( CRenderVideoPlayer_TickState );

		TickState();

		m_videoFlash->SetViewport( viewportRect );
		m_videoFlash->Tick( timeDelta );
	}
}

void CRenderVideoPlayer::CleanupVideoFlash()
{
	{
		PC_SCOPE( UnhookFlashFunctions );

		size_t len = sizeof(sm_flashFunctionImportTable)/sizeof(sm_flashFunctionImportTable[0]);
		for ( size_t i = 0; i < len; ++i )
		{
			CFlashValue& flashHook = this->*sm_flashFunctionImportTable[ i ].m_flashFuncImport;
			flashHook.Clear();
		}
	}
	
	DiscardFlashFunctions();

	m_flashNetStream.Clear();
}

void CRenderVideoPlayer::TickState()
{
	if ( !m_isFlashRegistered )
	{
		return;
	}

	if( ! m_playingVideo.IsValid() )
	{
		if ( !m_isAllVideosPaused )
		{
			PlayNextVideoInQueue();
		}
	}
	else	
	{
		if ( m_playingVideo.m_stopRequested )
		{
			UpdateVideoStop();
		}
		else
		{
			UpdateVideoPause();
		}
	}
}

#ifdef RED_LOGGING_ENABLED
const Char* GetVideoBufferTxtForLog( EVideoBuffer buffer )
{
	const Char* txt = TXT("<Unknown>");
	switch( buffer )
	{
	case eVideoBuffer_Default:
		txt = TXT("eVideoBuffer_Default");
		break;
	case eVideoBuffer_Short:
		txt = TXT("eVideoBuffer_Short");
		break;
	case eVideoBuffer_Long:
		txt = TXT("eVideoBuffer_Long");
		break;
	case eVideoBuffer_Bumpers:
		txt = TXT("eVideoBuffer_Bumpers");
		break;
	default:
		break;
	}

	return txt;
}
#endif // RED_LOGGING_ENABLED

void CRenderVideoPlayer::PlayNextVideoInQueue()
{
	// Update next video here so given a chance to stay in the queue if a preemptive video comes in first
	// Check render thread first, so going backwards
	for ( Int32 j = eVideoThreadIndex_Count-1; j >= 0; --j )
	{
		if ( m_threadData[ j ].m_locked )
		{
			continue;
		}

		TDynArray< SVideoContext >& videoPlayQueue = m_threadData[ j ].m_videoPlayQueue;
		if ( ! videoPlayQueue.Empty() )
		{
			m_playingVideo = videoPlayQueue.Front();
			videoPlayQueue.RemoveAt( 0 );
			RED_ASSERT( m_playingVideo.m_renderVideo->GetRenderVideoState() == eRenderVideoState_Pending, TXT("Invalid video render state '%u'!"), m_playingVideo.m_renderVideo->GetRenderVideoState() );

			const String& forceVideoFile = Config::cvDebugForceVideoFile.Get();
			const String& videoFile = !forceVideoFile.Empty() ? forceVideoFile : m_playingVideo.GetVideoParams().m_fileName;
#ifdef RED_LOGGING_ENABLED
			if ( !forceVideoFile.Empty() )
			{
				VIDEO_LOG(TXT("cvDebugForceVideoFile: '%ls'"), forceVideoFile.AsChar() );
			}
#endif

			CFlashString* tmpFlashString = m_videoFlash->CreateString( videoFile );
			CFlashValue flashVideoFile( tmpFlashString->AsFlashValue() );
			tmpFlashString->Release();

#ifdef RED_PROFILE_FILE_SYSTEM
			RedIOProfiler::ProfileSignalVideoPlay();
#endif

			Float bufferTime = 1.f;
			Float reloadThresholdTime = 1.f;
			Int32 numberOfFramePools = 1;
			const EVideoBuffer buffer = m_playingVideo.GetVideoParams().m_videoBuffer;
			GetVideoBufferParams( buffer, bufferTime, reloadThresholdTime, numberOfFramePools );

			LOG_RENDERER( TXT("Video buffer params for bufferType %ls: bufferTime=%1.2f, reloadThresholdTime=%1.2f, numberOfFramePools=%d, cvDebugUseOverrideBuffer=%d"),
				GetVideoBufferTxtForLog( buffer ), bufferTime, reloadThresholdTime, numberOfFramePools, (Config::cvDebugUseOverrideBuffer.Get() ? 1 : 0) );

			Bool allSet = true;
			allSet &= m_flashNetStream.SetMemberFlashNumber( TXT("bufferTime"), bufferTime );
			allSet &= m_flashNetStream.SetMemberFlashNumber( TXT("reloadThresholdTime"), reloadThresholdTime );
			allSet &= m_flashNetStream.SetMemberFlashNumber( TXT("numberOfFramePools"), numberOfFramePools );
			if ( ! allSet )
			{
				VIDEO_WARN(TXT("CRenderVideoPlayer: failed to set all netstream buffering properties!"));
			}

			if ( ! m_playVideoHook.InvokeSelf( flashVideoFile ) )
			{
				VIDEO_ERROR(TXT("CRenderVideoPlayer: Failed to invoke playVideoHook!"));
				OnVideoError();
			}
			break;
		}
	}
}

void CRenderVideoPlayer::UpdateVideoPause()
{
	const EVideoPauseRequest pauseRequest = m_playingVideo.m_pauseRequest;
	m_playingVideo.m_pauseRequest = PauseRequest_None;
	if ( pauseRequest == PauseRequest_Pause )
	{
		if ( ! m_pauseVideoHook.InvokeSelf() )
		{
			VIDEO_ERROR(TXT("CRenderVideoPlayer: Failed to pause video '%ls'"), m_playingVideo.GetVideoParams().m_fileName.AsChar() );
		}
	}
	else if ( pauseRequest == PauseRequest_Unpause )
	{
		if ( ! m_unpauseVideoHook.InvokeSelf() )
		{
			VIDEO_ERROR(TXT("CRenderVideoPlayer: Failed to unpause video '%ls'"), m_playingVideo.GetVideoParams().m_fileName.AsChar() );
		}
		m_unpauseVideoHook.InvokeSelf();
	}
}

void CRenderVideoPlayer::UpdateVideoStop()
{
	if ( ! m_playingVideo.m_stopping )
	{
		// Set stopping before invoking in case ever updates event before calling advance
		m_playingVideo.m_stopping = true;
		if ( ! m_stopVideoHook.InvokeSelf() )
		{
			VIDEO_ERROR(TXT("CRenderVideoPlayer: Failed to invoke stopVideoHook!"));
			OnVideoError();
		}
	}
}

void CRenderVideoPlayer::StopAllVideos()
{
	if ( m_playingVideo.IsValid() )
	{
		CancelVideo( m_playingVideo.m_renderVideo.Get() );
	}
	
	for ( Uint32 i = 0; i < eVideoThreadIndex_Count; ++i )
	{
		TDynArray< SVideoContext >& videoPlayQueue = m_threadData[ i ].m_videoPlayQueue;
		for ( const SVideoContext& videoContext : videoPlayQueue )
		{
			videoContext.m_renderVideo->Stop();
		}
		videoPlayQueue.Clear();
	}
}

void CRenderVideoPlayer::LockVideoThread( Bool locked, EVideoThreadIndex threadIndex )
{
	m_threadData[ threadIndex ].m_locked = locked;
}

Bool CRenderVideoPlayer::IsPlayingVideo() const
{
	return m_playingVideo.IsValid() && !m_playingVideo.m_stopRequested;
}

Bool CRenderVideoPlayer::IsPlayingBumpers() const
{
	return IsPlayingVideo() && (m_playingVideo.GetVideoParams().m_videoParamFlags & eVideoParamFlag_Bumpers) != 0;
}

void CRenderVideoPlayer::ToggleVideoPause( Bool pause )
{
	const Bool pauseChanged = m_isAllVideosPaused ^ pause;
	if ( pauseChanged )
	{
		m_isAllVideosPaused = pause;
		if ( IsPlayingVideo() )
		{
			m_playingVideo.m_pauseRequest = pause ? PauseRequest_Pause : PauseRequest_Unpause;
			if ( pause )
			{
				// Make sure Scaleform CRI video player is paused. If the game suspends before video paused,
				// the CRI decoder will speed things up once unsuspended.
				// Tick value slightly more than 1/24.f in order to make sure the GFx player advance updates
				// video instead of doing a fractional advance, which doesn't. Ticking some smaller value might often work
				// but relies on the time delta build up from previous advances.
				Tick( 0.05f );
			}
		}
	}
}

void CRenderVideoPlayer::PlayVideo( CRenderVideo* renderVideo, EVideoThreadIndex threadIndex )
{
	RED_FATAL_ASSERT( renderVideo, "No video!" );

	const SVideoParams& videoParams = renderVideo->GetVideoParams();

	TDynArray< SVideoContext >& videoPlayQueue = m_threadData[ threadIndex ].m_videoPlayQueue;

	if ( (videoParams.m_videoParamFlags & eVideoParamFlag_Preemptive) != 0 )
	{
		const Bool playingVideo = m_playingVideo.IsValid() && !m_playingVideo.m_stopRequested;
		if ( playingVideo && !m_threadData[ threadIndex ].m_locked )
		{
			CancelVideo( m_playingVideo.m_renderVideo.Get() );
		}

		videoPlayQueue.Insert( 0, SVideoContext( renderVideo ) );
	}
	else
	{
		videoPlayQueue.PushBack( SVideoContext( renderVideo ) );
	}
}

void CRenderVideoPlayer::CancelVideo( CRenderVideo* renderVideo )
{
	RED_FATAL_ASSERT( renderVideo, "No video!" );

	// Already cancelled or played to finish
	if ( ! renderVideo->IsValid() )
	{
		return;
	}

	// Was already playing
	if ( m_playingVideo.IsValid() && m_playingVideo.m_renderVideo.Get() == renderVideo )
	{
		m_playingVideo.m_stopRequested = true;
		return;
	}

	// Wasn't playing yet, remove from play queue
	for ( Uint32 i = 0; i < eVideoThreadIndex_Count; ++i )
	{
		TDynArray< SVideoContext >& videoPlayQueue = m_threadData[ i ].m_videoPlayQueue;
		for ( Int32 j = videoPlayQueue.SizeInt()-1; j >= 0; --j )
		{
			const SVideoContext& videoContext = videoPlayQueue[ j ];
			if ( videoContext.m_renderVideo.Get() == renderVideo )
			{
				videoContext.m_renderVideo->Stop();
				videoPlayQueue.RemoveAt( j );
				break;
			}
		}
	}
}

void CRenderVideoPlayer::ShowBackground( const Color& rbg )
{
	m_isShowingBackground = true;

	const Uint32 flashColor = (rbg.R << 16) | (rbg.G << 8) | rbg.B;
	if ( ! m_showBackgroundHook.InvokeSelf( CFlashValue( flashColor ) ) )
	{
		VIDEO_ERROR(TXT("CRenderVideoPlayer: Failed to invoke showBackgroundHook!"));
	}
}

void CRenderVideoPlayer::HideBackground()
{
	m_isShowingBackground = false;

	if ( ! m_hideBackgroundHook.InvokeSelf() )
	{
		VIDEO_ERROR(TXT("CRenderVideoPlayer: Failed to invoke hideBackgroundHook!"));
	}
}

void CRenderVideoPlayer::OnVideoStarted()
{
	START_VIDEO_PERF_CAPTURE( bumpers.usm );

	m_playingVideo.m_renderVideo->Play();
}

void CRenderVideoPlayer::SetVoiceTrack( Bool isMultiTrack )
{
	Int32 voiceTrack = 0;
	Int32 subtitleTrack = 1; // Scaleform subtracts one from subtitle index for the CRI player
	Bool useAltTrack = false;

	const SVideoParams& videoParams = m_playingVideo.GetVideoParams();

	m_playingVideo.m_isMultiTrack = isMultiTrack;

	if ( isMultiTrack && !GDeferredInit ) // and so the hacks begin - need to wait for the streaming installer to properly attach languages, but this bumpers crap comes before it
	{
		String audioLang;
		String subtitleLang;
		SLocalizationManager::GetInstance().GetGameLanguageName( audioLang, subtitleLang );

		String hackRealSubtitleLang = subtitleLang;

		if ( subtitleLang.EqualsNC( TXT("tr")) || subtitleLang.EqualsNC( TXT("cn")) )
		{
			// #hack: get driven by EN subtitle timings
			const_cast< SVideoParams& >( videoParams ).m_videoParamFlags |= eVideoParamFlag_ExternalSubs;
			subtitleLang = TXT("EN");
		}

		VIDEO_LOG(TXT("OnVideoStarted: audioLang=%ls, subtitleLang=%ls, hackRealSubtitleLang=%ls, USM=%ls"), audioLang.AsChar(), subtitleLang.AsChar(), hackRealSubtitleLang.AsChar(), videoParams.m_fileName.AsChar());

		const SLanguageTrackInfo* voiceTrackInfo = m_languageTracks.FindPtr( audioLang );
		const SLanguageTrackInfo* subtitleTrackInfo = m_languageTracks.FindPtr( subtitleLang );

		if ( ! voiceTrackInfo )
		{
			VIDEO_ERROR(TXT("No voice track information for %ls"), audioLang.AsChar() );
			return;
		}

		if ( ! subtitleTrackInfo )
		{
			VIDEO_ERROR(TXT("No subtitle track information for %ls"), audioLang.AsChar() );
			return;
		}
		
		Bool useAltTrack = (videoParams.m_videoParamFlags & eVideoParamFlag_AlternateTrack) != 0;
		
		subtitleTrack = useAltTrack ? subtitleTrackInfo->m_altSubtitlesTrack : subtitleTrackInfo->m_subtitlesTrack;
		voiceTrack = useAltTrack ? voiceTrackInfo->m_altVoiceTrack : voiceTrackInfo->m_voiceTrack;

		if ( Config::cvDebugForceSubtitlesTrack.Get() > -1 )
		{
			subtitleTrack = Config::cvDebugForceSubtitlesTrack.Get();
			VIDEO_LOG(TXT("cvDebugForceSubtitlesTrack forcing subtitle track to %d"), subtitleTrack );
		}
		if ( Config::cvDebugForceVoiceTrack.Get() > -1 )
		{
			voiceTrack = Config::cvDebugForceVoiceTrack.Get();
			VIDEO_LOG(TXT("cvDebugForceVoiceTrack forcing voice track to %d"), voiceTrack );
		}

		if ( (videoParams.m_videoParamFlags & eVideoParamFlag_ExternalSubs) != 0 )
		{
			// E.g., if r4data\movies\cutscenes\storybook\st_1.usm,
			// get r4data\movies\cutscenes\storybook\subs\st_1_en.subs or alt-subs
			const CFilePath filePath( videoParams.m_fileName );
			const String subsPath = String::Printf(TXT("%ls%ls%ls%ls_%ls.subs"),
				(videoParams.m_fileName.BeginsWith(TXT("dlc\\")) ? TXT("") : TXT("movies\\")),
				filePath.GetPathString(true).AsChar(),
				( useAltTrack ? TXT("altsubs\\") : TXT("subs\\") ),
				filePath.GetFileName().AsChar(),
				hackRealSubtitleLang.ToLower().AsChar() );

			m_playingVideo.m_videoSubs.Reset( new CVideoSubtitles( subsPath ) );
			if ( m_playingVideo.m_videoSubs->IsValid() )
			{
				VIDEO_LOG(TXT("Loaded %u external subtitles '%ls' for USM '%ls'"), 
					m_playingVideo.m_videoSubs->GetSize(),
					subsPath.AsChar(),
					videoParams.m_fileName.AsChar());
			}
			else
			{
				VIDEO_ERROR(TXT("FAILED to load cuepoint-driven subtitles '%ls' for USM '%ls'"), subsPath.AsChar(), videoParams.m_fileName.AsChar());
				m_playingVideo.m_videoSubs.Reset( nullptr );
			}	
		}
	}

	const Int32 BASE_AUDIO_TRACK = 0;
	Int32 audioTrack = BASE_AUDIO_TRACK;
	if ( Config::cvDebugDisableVideoAudio.Get() )
	{
		VIDEO_LOG(TXT("cvDebugDisableVideoAudio: AUDIO DISABLED!"));
		voiceTrack = -1;
		subtitleTrack = -1;
		audioTrack = -1;
	}

	VIDEO_LOG(TXT("OnVideoStarted: fileName='%ls' subtitleTrack=%d%ls, voiceTrack=%d%ls, audioTrack=%d, isMultiTrack=[%ls], GDeferredInit=[%ls]"), 
		videoParams.m_fileName.AsChar(),
		subtitleTrack, (useAltTrack ? TXT(" [alt]") : TXT("") ),
		voiceTrack, (useAltTrack ? TXT(" [alt]") : TXT("") ),
		audioTrack,
		(isMultiTrack ? TXT("y") : TXT("n")),
		(GDeferredInit ? TXT("y") : TXT("n")));

	Bool playLooped = (m_playingVideo.GetVideoParams().m_videoParamFlags & eVideoParamFlag_PlayLooped) != 0;
	if ( Config::cvDebugDisableVideoLoop.Get() )
	{
		VIDEO_WARN(TXT("cvDebugDisableVideoLoop=1"));
		playLooped = false;
	}

	Bool allParamsSet = m_flashNetStream.SetMemberFlashNumber(TXT("subtitleTrack"), subtitleTrack);
	allParamsSet &= m_flashNetStream.SetMemberFlashNumber(TXT("audioTrack"), audioTrack );
	allParamsSet &= m_flashNetStream.SetMemberFlashNumber(TXT("voiceTrack"), voiceTrack); // 5.1 ch
	allParamsSet &= m_flashNetStream.SetMemberFlashBool(TXT("loop"), playLooped );
	RED_ASSERT( allParamsSet, TXT("Not all video parameters set!"));

	UpdateFlashSound();
}

void CRenderVideoPlayer::OnVideoStopped()
{
	// IMPORTANT: If the video runs to the end, we still need to close the netstream and clear the video texture here.
	// It used to be covered in ActionScript, but this allows control from C++ if later needed in order to cover gaps.
	// If anything, should probably rename the hook.
	if ( ! m_stopVideoHook.InvokeSelf() )
	{
		VIDEO_ERROR(TXT("CRenderVideoPlayer: Failed to invoke stopVideoHook!"));
	}

	m_playingVideo.m_renderVideo->Stop();
	m_playingVideo = SVideoContext();

	END_VIDEO_PERF_CAPTURE();

#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileSignalVideoStop();
#endif
}

void CRenderVideoPlayer::OnVideoError()
{
	VIDEO_ERROR(TXT("CRenderVideoPlayer::OnVideoError: for '%ls'. Internal CRIPlayer error or file not found"), m_playingVideo.GetVideoParams().m_fileName.AsChar() );

	// IMPORTANT: If the video runs to the end, we still need to close the netstream and clear the video texture here.
	// It used to be covered in ActionScript, but this allows control from C++ if later needed in order to cover gaps.
	// If anything, should probably rename the hook.
	if ( ! m_stopVideoHook.InvokeSelf() )
	{
		VIDEO_ERROR(TXT("CRenderVideoPlayer: Failed to invoke stopVideoHook!"));
	}

	m_playingVideo.m_renderVideo->SetErrorFlag();
	m_playingVideo.m_renderVideo->Stop();
	m_playingVideo = SVideoContext();

#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileSignalVideoStop();
#endif
}

void CRenderVideoPlayer::OnVideoSubtitles( const String& subtitles )
{
	if ( !m_playingVideo.m_videoSubs )
	{
		VIDEO_LOG(TXT("Subtitles [embedded]: '%ls'"), subtitles.AsChar());
		m_playingVideo.m_renderVideo->UpdateSubtitles( subtitles );
	}
	else
	{
		// #hack: tbd: no blank lines in our subtitles file or else the internal index will get out of sync...
		if ( subtitles.Empty() )
		{
			VIDEO_LOG(TXT("Subtitles [external(blank)]: '%ls'"), String::EMPTY.AsChar());
			m_playingVideo.m_renderVideo->UpdateSubtitles( String::EMPTY );
		}
		else
		{
			// Use override external subs file
			const CVideoSubtitles& videoSubs = *m_playingVideo.m_videoSubs;
			if ( m_playingVideo.m_hackyExternalSubsIndex < videoSubs.GetSize() )
			{
				const String& overrideSubtitles = videoSubs[  m_playingVideo.m_hackyExternalSubsIndex ];
				VIDEO_LOG(TXT("Subtitles [external]: '%ls'"), overrideSubtitles.AsChar());
				m_playingVideo.m_renderVideo->UpdateSubtitles( overrideSubtitles );
				++m_playingVideo.m_hackyExternalSubsIndex;
			}
			else
			{
				VIDEO_ERROR(TXT("External subtitles has out of range index %u (num subtitles %u)"),  m_playingVideo.m_hackyExternalSubsIndex, videoSubs.GetSize());
			}
		}
	}
}

void CRenderVideoPlayer::OnVideoCuePoint( const String& cuePoint )
{
	const Bool useAltTrack = (m_playingVideo.GetVideoParams().m_videoParamFlags & eVideoParamFlag_AlternateTrack) != 0;
	const size_t mainLen = Red::System::StringLengthCompileTime( TXT("_main") );
	const size_t altLen = Red::System::StringLengthCompileTime( TXT("_alt") );

	if ( cuePoint.BeginsWith(TXT("subs_start")) )
	{
		if ( ! m_playingVideo.m_videoSubs )
		{
			VIDEO_WARN(TXT("Cue point %ls but no subtitles loaded!"), cuePoint.AsChar());
			return;
		}

		const CVideoSubtitles& videoSubs = *m_playingVideo.m_videoSubs;
		const Char* ch = cuePoint.AsChar() + Red::System::StringLengthCompileTime(TXT("subs_start"));
		if ( Red::System::StringCompare( ch, TXT("_main"), mainLen ) == 0 )
		{
			if ( useAltTrack )
			{
				return;
			}
			ch += mainLen;
		}
		else if ( Red::System::StringCompare( ch, TXT("_alt"), altLen ) == 0 )
		{
			if ( !useAltTrack )
			{
				return;
			}
			ch += altLen;
		}

		Uint32 index = 0;
		if ( !GParseInteger( ch, index ) )
		{
			VIDEO_ERROR(TXT("Subtitles cuepoint %ls has no index number"), cuePoint.AsChar());
			return;
		}

		if ( index < videoSubs.GetSize() )
		{
			const String& text = videoSubs[ index ];
			OnVideoSubtitles( text );
		}
		else
		{
			VIDEO_ERROR(TXT("Subtitles cuepoint %ls has out of range index %u (num subtitles %u)"), cuePoint.AsChar(), index, videoSubs.GetSize());
		}
	}
	else if ( cuePoint.BeginsWith(TXT("subs_end")) )
	{
		if ( ! m_playingVideo.m_videoSubs )
		{
			VIDEO_WARN(TXT("Cue point %ls but no subtitles loaded!"), cuePoint.AsChar());
			return;
		}

		const Char* ch = cuePoint.AsChar() + Red::System::StringLengthCompileTime(TXT("subs_end"));
		if ( Red::System::StringCompare( ch, TXT("_main"), mainLen ) == 0 )
		{
			if ( useAltTrack )
			{
				return;
			}
			ch += Red::System::StringLengthCompileTime( TXT("_main") );
		}
		else if ( Red::System::StringCompare( ch, TXT("_alt"), altLen ) == 0 )
		{
			if ( !useAltTrack )
			{
				return;
			}
			ch += Red::System::StringLengthCompileTime( TXT("_alt") );
		}

		OnVideoSubtitles( String::EMPTY );
	}
	else if ( cuePoint.EqualsNC(TXT("stop")) )
	{
		 m_playingVideo.m_stopRequested = true;
	}
	else
	{
		m_playingVideo.m_renderVideo->UpdateCuePoint( cuePoint );
	}
}

Bool CRenderVideoPlayer::RegisterFlashVideoPlayer( CFlashValue& flashVideoPlayer, const CFlashValue& flashNetStream )
{
	m_flashNetStream = flashNetStream;

	if ( !flashNetStream.GetMember( TXT("soundTransform" ), m_flashSoundTransform ) || !m_flashSoundTransform.IsFlashObject() )
	{
		VIDEO_ERROR(TXT("Failed to get soundTransform from netstream!"));
		return false;
	}

	if ( !flashNetStream.GetMember( TXT("subSoundTransform" ), m_flashSubSoundTransform ) || !m_flashSubSoundTransform.IsFlashObject() )
	{
		VIDEO_ERROR(TXT("Failed to get subSoundTransform from netstream!"));
		return false;
	}

	Uint32 len = ARRAY_COUNT_U32(sm_flashFunctionImportTable);
	for ( Uint32 i = 0; i < len; ++i )
	{
		if ( ! flashVideoPlayer.GetMember( sm_flashFunctionImportTable[ i ].m_memberName, this->*sm_flashFunctionImportTable[ i ].m_flashFuncImport ) )
		{
			VIDEO_ERROR( TXT("Failed to import Flash function '%ls'"), sm_flashFunctionImportTable[ i ].m_memberName );
			return false;
		}
	}

	for ( TFlashFunctionMap::const_iterator it = m_flashFunctionMap.Begin(); it != m_flashFunctionMap.End(); ++it )
	{
		const Char* memberName = it->m_first;
		const CFlashFunction* flashFunction = it->m_second;
		ASSERT( memberName && flashFunction );

		CFlashValue checkIfExistsToAvoidScaryFlashExceptionErrorMessagesInLogFile;
		if ( ! flashVideoPlayer.GetMember( memberName, checkIfExistsToAvoidScaryFlashExceptionErrorMessagesInLogFile ) )
		{
			VIDEO_WARN( TXT("Failed to set member flash function '%ls'. Doesn't exist in sprite"), memberName );
		}
		else if ( ! flashVideoPlayer.SetMemberFlashFunction( memberName, *flashFunction ) )
		{
			VIDEO_WARN( TXT("Failed to set member flash function '%ls'"), memberName );
		}
	}

	// Disable open timeout
	if ( ! m_flashNetStream.SetMemberFlashNumber( TXT("openTimeout"), 0. ) )
	{
		VIDEO_WARN(TXT("Failed to set NetStream openTimeout"));
	}

	m_isFlashRegistered = true;

	UpdateFlashSound();

	return true;
}

void CRenderVideoPlayer::OnVideoEvent( EVideoEvent videoEvent )
{
	switch ( videoEvent )
	{
	case VideoEvent_Stopped:
		OnVideoStopped();
		break;
	case VideoEvent_Error:
		OnVideoError();
		break;
	case VideoEvent_Started:
		OnVideoStarted();
		break;
	default:
		break;
	}
}

void CRenderVideoPlayer::OnVideoMetaData( const String& metaData )
{
	VIDEO_LOG(TXT("OnVideoMetaData: '%ls'"), metaData.AsChar());

	// 1. AS3_VideoProviderNetStream.cpp calls video functions during tick in this order
	/*
	NotifyVideoCharacters();
	SendNotification(NetStreamInterface::PlayStart);
	SendMetaData();
	pVideoPlayer->Play();
	*/

	// 2. If we wait until after the tick pVideoPlayer->Play() will have been called and it'll be too late to replace
	// the center voice track
	/*
	void VideoPlayerImpl::ReplaceCenterVoice(int track_index)
	{
	if (Stat == Ready)
	pCriPlayer->ReplaceCenterVoice(track_index);
	}
	*/

	// 3. Therefore we set the voice track NOW
	Uint32 numParsedAudioTracks = 0;
	// If fail to parse the number of tracks, assume multitrack as a failsafe
	const Bool selectTrack = !ParseNumAudioTracks( metaData, numParsedAudioTracks ) || numParsedAudioTracks > 1;
	SetVoiceTrack( selectTrack );

	m_playingVideo.m_renderVideo->UpdateMetaData( metaData );
}

void CRenderVideoPlayer::flFnOnVideoEvent( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args )
{
	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("OnVideoEvent: Function must be bound to a Flash DisplayObject before calling.") );
		return;
	}

	const Uint32 expectedNumArgs = 1;
	Uint32 numArgs = args.Size();
	if ( numArgs != expectedNumArgs )
	{
		GUI_ERROR( TXT("OnVideoEvent: Incorrect number of arguments passed. Expected '%u', received '%u'"), expectedNumArgs, numArgs );
		return;
	}

	if ( ! args[0].IsFlashInt() )
	{
		GUI_ERROR( TXT("OnVideoEvent: Expected arg[0] to be a Flash int") );
		return;
	}

	EVideoEvent videoEvent = static_cast< EVideoEvent >( args[0].GetFlashInt() );
	OnVideoEvent( videoEvent );	
}

void CRenderVideoPlayer::flFnOnVideoSubtitles( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args )
{
	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("OnVideoSubtitles: Function must be bound to a Flash DisplayObject before calling.") );
		return;
	}

	const Uint32 expectedNumArgs = 1;
	Uint32 numArgs = args.Size();
	if ( numArgs != expectedNumArgs )
	{
		GUI_ERROR( TXT("OnVideoSubtitles: Incorrect number of arguments passed. Expected '%u', received '%u'"), expectedNumArgs, numArgs );
		return;
	}

	if ( ! args[0].IsFlashString() )
	{
		GUI_ERROR( TXT("OnVideoSubtitles: Expected arg[0] to be a Flash string") );
		return;
	}

	const String subtitles = args[0].GetFlashString();
	OnVideoSubtitles( subtitles );
}

void CRenderVideoPlayer::flFnOnVideoCuePoint( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args )
{
	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("OnVideoCuePoint: Function must be bound to a Flash DisplayObject before calling.") );
		return;
	}

	const Uint32 expectedNumArgs = 1;
	Uint32 numArgs = args.Size();
	if ( numArgs != expectedNumArgs )
	{
		GUI_ERROR( TXT("OnVideoCuePoint: Incorrect number of arguments passed. Expected '%u', received '%u'"), expectedNumArgs, numArgs );
		return;
	}

	if ( ! args[0].IsFlashString() )
	{
		GUI_ERROR( TXT("OnVideoCuePoint: Expected arg[0] to be a Flash string") );
		return;
	}

	const String cuePoint = args[0].GetFlashString();
	OnVideoCuePoint( cuePoint );
}

void CRenderVideoPlayer::flFnOnVideoMetaData( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args )
{
	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("OnVideoMetaData: Function must be bound to a Flash DisplayObject before calling.") );
		return;
	}

	const Uint32 expectedNumArgs = 1;
	Uint32 numArgs = args.Size();
	if ( numArgs != expectedNumArgs )
	{
		GUI_ERROR( TXT("OnVideoMetaData: Incorrect number of arguments passed. Expected '%u', received '%u'."), expectedNumArgs, numArgs );
		return;
	}

	if ( ! args[0].IsFlashString() )
	{
		GUI_ERROR( TXT("OnVideoMetaData: Expected arg[0] to be a Flash string") );
		return;
	}

	const String metaData = args[0].GetFlashString();
	OnVideoMetaData( metaData );
}
