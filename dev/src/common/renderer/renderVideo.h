/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../engine/videoPlayer.h"

class CRenderVideo : public IRenderVideo
{
private:
	typedef Red::Threads::CMutex CMutex;
	typedef Red::Threads::CScopedLock< CMutex > CScopedLock;

private:
	CMutex				m_mutex;
	String				m_subtitles;
	String				m_cuePoint;
	String				m_metaData;
	Bool				m_hasNewSubtitles;
	Bool				m_hasNewCuePoint;
	Bool				m_hasNewMetaData;
	Bool				m_errorFlag;

private:
	CName				m_videoClient;
	SVideoParams		m_videoParams;
	ERenderVideoState	m_renderVideoState;

public:
	CRenderVideo( CName videoClient, const SVideoParams& videoParams );

public:
	virtual ERenderVideoState GetRenderVideoState() const override { return const_cast< volatile ERenderVideoState& >( m_renderVideoState ); }
	virtual Bool GetErrorFlag() const override { return const_cast<volatile Bool& >( m_errorFlag ); }
	virtual Bool FlushSubtitles( String& outSubtitles ) override;
	virtual Bool FlushCuePoint( String& outCuePoint ) override;
	virtual Bool FlushMetaData( String& outMetaData ) override;

public:
	void SetErrorFlag() { m_errorFlag = true; }
	void Play() { if ( m_renderVideoState == eRenderVideoState_Pending ) m_renderVideoState = eRenderVideoState_Playing; }
	void Stop() { m_renderVideoState = eRenderVideoState_Finished; }
	void UpdateSubtitles( const String& subtitles );
	void UpdateCuePoint( const String& cuePoint );
	void UpdateMetaData( const String& metaData );

public:
	CName GetVideoClient() const { return m_videoClient; }
	const SVideoParams& GetVideoParams() const { return m_videoParams; }
};