/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../core/hashmap.h"

class IViewport;
class CVideoSubtitles;

#include "../core/sharedPtr.h"
#include "../engine/scaleformConfig.h"
#include "../engine/videoPlayer.h"
#include "../engine/flashFunction.h"
#include "../engine/flashPlayer.h"
#include "../core/refCountPointer.h"
#include "../engine/videoSubtitles.h"
#include "renderVideo.h"

//////////////////////////////////////////////////////////////////////////
// EVideoThreadIndex
//////////////////////////////////////////////////////////////////////////
enum EVideoThreadIndex : Int32
{
	eVideoThreadIndex_GameThread,
	eVideoThreadIndex_RenderThread,
	eVideoThreadIndex_Count,
};

//FIXME: Too much setup copy boilerplate between this and movie, hud, and sprite adapters

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CFlashMovie;

//////////////////////////////////////////////////////////////////////////
// CRenderVideoPlayer
//////////////////////////////////////////////////////////////////////////
class CRenderVideoPlayer : public IFlashExternalInterfaceHandler
{
private:
	typedef CVideoPlayer TBaseClass;
	typedef	Flash::TFlashFuncPtr< CRenderVideoPlayer >::Type	TFlashFunctionExport;
	typedef CFlashValue CRenderVideoPlayer::*					TFlashFunctionImport;
	typedef THashMap< const Char*, CFlashFunction* >			TFlashFunctionMap;

private:
	enum EVideoEventType
	{
		eVideoEventType_Invalid,
		eVideoEventType_Started,
		eVideoEventType_Stopped,
		eVideoEventType_Subtitles,
	};

	struct SVideoEvent
	{
		EVideoEventType m_videoEventType;
		CName			m_videoClient;
		String			m_data;

		SVideoEvent()
			: m_videoEventType( eVideoEventType_Invalid )
		{}

		SVideoEvent( EVideoEventType videoEventType, CName videoClient, const String& data = String::EMPTY )
			: m_videoEventType( videoEventType )
			, m_videoClient( videoClient )
			, m_data( data )
		{}
	};

private:
	struct SFlashFuncImportDesc
	{
		const Char*							m_memberName;
		TFlashFunctionImport				m_flashFuncImport;	
	};

	struct SFlashFuncExportDesc
	{
		const Char*							m_memberName;
		TFlashFunctionExport				m_flashFuncExport;
	};

private:
	static SFlashFuncImportDesc				sm_flashFunctionImportTable[];
	static SFlashFuncExportDesc				sm_flashFunctionExportTable[];

private:
	TFlashFunctionMap						m_flashFunctionMap;

private:
	Float									m_masterVolumePercent;
	Float									m_voiceVolumePercent;
	Float									m_effectsVolumePercent;

private:
	typedef Red::Threads::CMutex CMutex;
	typedef Red::Threads::CScopedLock< CMutex > CScopedLock;

private:
	CFlashMovie*							m_videoFlash;
	Bool									m_isFlashRegistered;
	Bool									m_isAllVideosPaused;
	Bool									m_isShowingBackground;

private:
	CFlashValue								m_flashNetStream;
	CFlashValue								m_flashSoundTransform;
	CFlashValue								m_flashSubSoundTransform;
	CFlashValue								m_playVideoHook;
	CFlashValue								m_stopVideoHook;
	CFlashValue								m_pauseVideoHook;
	CFlashValue								m_unpauseVideoHook;
	CFlashValue								m_clearVideoHook;
	CFlashValue								m_showBackgroundHook;
	CFlashValue								m_hideBackgroundHook;

private:
	mutable CMutex							m_gameEventsMutex;

private:

	enum EVideoPauseRequest
	{
		PauseRequest_None,
		PauseRequest_Pause,
		PauseRequest_Unpause,
	};

	// FIXME: shouldn't use as queue entry since stop requested doesn't make sense until playing (otherwise just removed from queue/nextVideo)
	struct SVideoContext
	{
		TRefCountPointer< CRenderVideo >	m_renderVideo;
		Red::TSharedPtr< CVideoSubtitles >	m_videoSubs;
		Uint32								m_hackyExternalSubsIndex; // For Turkish subtitles, we use some other built-in subtitles track to drive them
		EVideoPauseRequest					m_pauseRequest;
		Bool								m_stopRequested:1;
		Bool								m_stopping:1;
		Bool								m_isMultiTrack:1;

		SVideoContext()
			: m_renderVideo( nullptr )
			, m_hackyExternalSubsIndex( 0 )
			, m_pauseRequest( PauseRequest_None )
			, m_stopRequested( false )
			, m_stopping( false )
			, m_isMultiTrack( false )
		{}

		SVideoContext( CRenderVideo* renderVideo )
			: m_renderVideo( renderVideo )
			, m_hackyExternalSubsIndex( 0 )
			, m_pauseRequest( PauseRequest_None )
			, m_stopRequested( false )
			, m_stopping( false )
			, m_isMultiTrack( false )
		{}

		Bool IsValid() const { return m_renderVideo.Get() != nullptr; }
		Bool IsMultiTrack() const { return m_isMultiTrack; }
		const SVideoParams& GetVideoParams() const { RED_FATAL_ASSERT( m_renderVideo, "No video"); return m_renderVideo->GetVideoParams(); }
		const CName GetVideoClient() const { RED_FATAL_ASSERT( m_renderVideo, "No video"); return m_renderVideo->GetVideoClient(); }
	};

private:
	struct SThreadData
	{
		Bool							m_locked;
		TDynArray< SVideoContext >		m_videoPlayQueue;

		SThreadData()
			: m_locked( false )
		{}
	};

private:
	SThreadData							m_threadData[eVideoThreadIndex_Count];

private:
	SVideoContext						m_playingVideo;

private:

	struct SLanguageTrackInfo
	{
		static const Uint32 INVALID_TRACK = -1;

		Int32 m_voiceTrack;
		Int32 m_subtitlesTrack;
		Int32 m_altVoiceTrack;
		Int32 m_altSubtitlesTrack;

		SLanguageTrackInfo()
			: m_voiceTrack( INVALID_TRACK )
			, m_subtitlesTrack( INVALID_TRACK )
			, m_altVoiceTrack( INVALID_TRACK )
			, m_altSubtitlesTrack( INVALID_TRACK )
		{}
	};

	THashMap< String, SLanguageTrackInfo >	m_languageTracks;

private:
	// Matches values in videoplayer Flash
	enum EVideoEvent
	{
		VideoEvent_Started,
		VideoEvent_Stopped,
		VideoEvent_Error,
	};

public:
	void InitWithFlash( CFlashMovie* videoFlash );

public:
	void PlayVideo( CRenderVideo* renderVideo, EVideoThreadIndex threadIndex );
	void CancelVideo( CRenderVideo* renderVideo );
	void ToggleVideoPause( Bool pause );

public:
	void ShowBackground( const Color& rbg );
	void HideBackground();
	Bool IsShowingBackground() const { return m_isShowingBackground; }

public:
	void SetMasterVolumePercent( Float percent );
	void SetVoiceVolumePercent( Float percent );
	void SetEffectsVolumePercent( Float percent );

public:
	CRenderVideoPlayer();
	~CRenderVideoPlayer();

public:
	//! IFlashExternalInterfaceHandler functions
	virtual void OnFlashExternalInterface( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args, CFlashValue& outRetval ) override;

public:
 	void	Tick( Float timeDelta );
	void	StopAllVideos();
	void	LockVideoThread( Bool locked, EVideoThreadIndex threadIndex );

public:
	void	OnVideoEvent( EVideoEvent videoEvent );
	void	OnVideoSubtitles( const String& subtitles );
	void	OnVideoCuePoint( const String& cuePoint );
	void	OnVideoMetaData( const String& metaData );

public:
	Bool	IsPlayingVideo() const;
	Bool	IsPlayingBumpers() const;

private:
	void			Init();
	void			Shutdown();

private:
	Bool			RegisterFlashVideoPlayer( CFlashValue& flashVideoPlayer, const CFlashValue& flashNetStream );

private:
	void			InitFlashFunctions();
	void			InitLanguageTracks();
	void			DiscardFlashFunctions();
	CFlashFunction*	CreateFlashFunction( TFlashFunctionExport flashFunc );
	void			CleanupVideoFlash();

private:
	void			UpdateFlashSound();

private:
	void			OnVideoStarted();
	void			OnVideoStopped();
	void			OnVideoError();

private:
	void			SetVoiceTrack( Bool isMultiTrack );

private:
	void			TickState();
	void			PlayNextVideoInQueue();
	void			UpdateVideoPause();
	void			UpdateVideoStop();

private:
	void			MuteFlashSound();

private:
	void			flFnOnVideoEvent( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args );
	void			flFnOnVideoSubtitles( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args );
	void			flFnOnVideoMetaData( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args );
	void			flFnOnVideoCuePoint( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args );
};
