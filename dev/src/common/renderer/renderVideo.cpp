/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderVideo.h"
#include "../redSystem/crt.h"

CRenderVideo::CRenderVideo( CName videoClient, const SVideoParams& videoParams )
	: m_videoClient( videoClient )
	, m_videoParams( videoParams )
	, m_renderVideoState( eRenderVideoState_Pending )
	, m_hasNewSubtitles( false )
	, m_hasNewCuePoint( false )
	, m_errorFlag( false )
{
	RED_ASSERT( videoClient != CName::NONE,	TXT("Should have a proper video client name for '%ls'!"), videoParams.m_fileName.AsChar() );

	// Hack for parsing cue-point subs out of file name. E.g., st_1|0.usm or st_1|1.usm
	const Char* ch = Red::System::StringSearchLast( m_videoParams.m_fileName.AsChar(), TXT('|') );
	const size_t usmLen = Red::System::StringLengthCompileTime(TXT(".usm"));
	if ( ch && ch > m_videoParams.m_fileName.AsChar() )
	{
		const Char* const pipe = ch;
		++ch;
		Int32 track = -1;
		if ( GParseInteger( ch, track ) && Red::System::StringCompareNoCase( ch, TXT(".usm"), usmLen ) == 0 )
		{
			const size_t count = static_cast< size_t >( pipe - m_videoParams.m_fileName.AsChar() ); // exclude |
			m_videoParams.m_fileName = m_videoParams.m_fileName.LeftString( count ) + TXT(".usm");

			const Bool useAltTrack = track > 1 ;
			m_videoParams.m_videoParamFlags |= ( eVideoParamFlag_ExternalSubs | ( useAltTrack ? eVideoParamFlag_AlternateTrack : eVideoParamFlag_None ) );	
		}
		else
		{
			WARN_RENDERER(TXT("Video '%ls' seems like it might have an alt track, but failed to parse it from the filename"), m_videoParams.m_fileName.AsChar() );
		}
	}
}

void CRenderVideo::UpdateSubtitles( const String& subtitles )
{
	CScopedLock lock( m_mutex );
	m_subtitles = subtitles;
	m_hasNewSubtitles = true;
}


void CRenderVideo::UpdateMetaData( const String& metaData )
{
	CScopedLock lock( m_mutex );
	m_metaData = metaData;
	m_hasNewMetaData = true;
}

Bool CRenderVideo::FlushSubtitles( String& outSubtitles )
{
	CScopedLock lock( m_mutex );
	if ( m_hasNewSubtitles )
	{
		m_hasNewSubtitles = false;
		outSubtitles = Move( m_subtitles );
		return true;		
	}
	return false;
}

void CRenderVideo::UpdateCuePoint( const String& cuePoint )
{
	CScopedLock lock( m_mutex );
	m_cuePoint = cuePoint;
	m_hasNewCuePoint = true;
}

Bool CRenderVideo::FlushCuePoint( String& outCuePoint )
{
	CScopedLock lock( m_mutex );
	if ( m_hasNewCuePoint )
	{
		m_hasNewCuePoint = false;
		outCuePoint = Move( m_cuePoint );
		return true;		
	}
	return false;
}

Bool CRenderVideo::FlushMetaData( String& outMetaData )
{
	CScopedLock lock( m_mutex );
	if ( m_hasNewMetaData )
	{
		m_hasNewMetaData = false;
		outMetaData = Move( m_metaData );
		return true;		
	}
	return false;
}
