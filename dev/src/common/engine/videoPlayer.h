/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "renderObject.h"

RED_DECLARE_NAME( Video );

#define VIDEO_LOG( format, ... )	RED_LOG( Video, format, ## __VA_ARGS__ )
#define VIDEO_WARN( format, ... )	RED_LOG( Video, format, ## __VA_ARGS__ )
#define VIDEO_ERROR( format, ... )	RED_LOG( Video, format, ## __VA_ARGS__ )

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CRenderFrame;
class IViewport;

//////////////////////////////////////////////////////////////////////////
// EVideoParamFlags
//////////////////////////////////////////////////////////////////////////
enum EVideoParamFlags : Uint8
{
	eVideoParamFlag_None				= 0,
	eVideoParamFlag_PlayLooped			= FLAG(0), //!< Video loops until stopped explicitly
	eVideoParamFlag_Preemptive			= FLAG(1), //!< Preempts currently playing video and goes to the front of the play list
	eVideoParamFlag_AlternateTrack		= FLAG(2), //!< Play the alternate audio/subtitle track.
	eVideoParamFlag_ExternalSubs		= FLAG(3), //!< Use loose subtitles driven by cue point file. Hack in order to support more subtitle tracks.
	eVideoParamFlag_Bumpers				= FLAG(4), //!< Bumper video
//	eVideoParamFlag_NeverPause			= FLAG(XYZ), //!< Video cannot be paused, might be useful for backgrounds. Use carefully because might violate TRCs/XRs.
};

enum EVideoBuffer : Uint8
{
	eVideoBuffer_Default,
	eVideoBuffer_Short,
	eVideoBuffer_Long,
	eVideoBuffer_Bumpers,
};

//////////////////////////////////////////////////////////////////////////
// SVideoParams
//////////////////////////////////////////////////////////////////////////
struct SVideoParams
{
	String				m_fileName;
	Uint8				m_videoParamFlags;
	EVideoBuffer		m_videoBuffer;

	explicit SVideoParams( const String& fileName, Uint8 videoParamFlags = eVideoParamFlag_None, EVideoBuffer videoBuffer = eVideoBuffer_Default );

	Bool operator==( const SVideoParams& rhs ) const
	{
		return m_fileName == rhs.m_fileName && m_videoParamFlags == rhs.m_videoParamFlags && m_videoBuffer == rhs.m_videoBuffer;
	}

	Bool operator!=( const SVideoParams& rhs ) const
	{
		return !(*this == rhs);
	}
};

//////////////////////////////////////////////////////////////////////////
// ERenderVideoState
//////////////////////////////////////////////////////////////////////////
enum ERenderVideoState
{
	eRenderVideoState_Pending,
	eRenderVideoState_Playing,
	eRenderVideoState_Finished,
};

//////////////////////////////////////////////////////////////////////////
// IRenderVideo
//////////////////////////////////////////////////////////////////////////
class IRenderVideo : public IRenderObject
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

public:
	virtual ERenderVideoState GetRenderVideoState() const=0;
	virtual Bool GetErrorFlag() const=0;

	//! Gets current subtitles; returns true if new subtitles.
	//! Empty outSubtitles means displayed subtitles should be cleared.
	virtual Bool FlushSubtitles( String& outSubtitles )=0;
	virtual Bool FlushCuePoint( String& outCuePoint )=0;
	virtual Bool FlushMetaData( String& outMetaData )=0;

public:
	void Cancel();
	Bool IsValid() const { return GetRenderVideoState() != eRenderVideoState_Finished; }
};

//////////////////////////////////////////////////////////////////////////
// CVideoPlayer
//////////////////////////////////////////////////////////////////////////
class CVideoPlayer
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	CVideoPlayer();
	virtual ~CVideoPlayer();
	Bool DelayedInit();

	static void CreateGlobalVideoPlayer(CFlashPlayer*);

public:
	IRenderVideo*	CreateVideo( CName videoClient, const SVideoParams& videoParams );
	void			TogglePause( Bool pause );
};
