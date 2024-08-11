/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkSrcBase.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkSrcBase.h"
#include "AkPBI.h"
#include "AkPositionRepository.h"

CAkSrcBaseEx::CAkSrcBaseEx( CAkPBI * in_pCtx )
	: CAkVPLSrcNode( in_pCtx )
	, m_uTotalSamples( 0 )
	, m_uCurSample( 0 )
	, m_uDataSize( 0 )
	, m_uDataOffset( 0 )
	, m_uPCMLoopStart( 0 )
	, m_uPCMLoopEnd( 0 )
	, m_uLastEnvelopePtIdx( 0 )
{
	m_uLoopCnt = m_pCtx->GetLooping();
}

CAkSrcBaseEx::~CAkSrcBaseEx()
{
}

void CAkSrcBaseEx::StopStream()
{
	m_markers.Free();
}

AKRESULT CAkSrcBaseEx::StopLooping()
{
	m_uLoopCnt = 1;
	return AK_Success;
}

AKRESULT CAkSrcBaseEx::TimeSkip( AkUInt32 & io_uFrames )
{
    AKRESULT eResult = AK_DataReady;

	// Store current sample position for markers and io_uFrames adjustment. 
	AkUInt32 uCurrSample = m_uCurSample;
	
	m_uCurSample += io_uFrames;

#if AK_VOICE_BUFFER_POSITION_TRACKING
	char msg[256];
	sprintf( msg, "[p:%p] TimeSkip %i / %i(%i) \n", this, m_uCurSample, m_uTotalSamples, m_uPCMLoopEnd );
	AKPLATFORM::OutputDebugMsg( msg );
#endif

	if ( !DoLoop() )
	{
		// Check end of file. 
		if ( m_uCurSample >= m_uTotalSamples )
		{
			// Adjust io_uFrames, return no more data (sound will stop).
			AKASSERT( io_uFrames >= (m_uTotalSamples - uCurrSample) );
			io_uFrames = m_uTotalSamples - uCurrSample;
			eResult = AK_NoMoreData;
		}
	}
	else
	{
		// Check loop.
		if ( m_uCurSample > m_uPCMLoopEnd ) // >= m_uPCMLoopEnd + 1 )
		{
			// Decrement loop count, adjust io_uFrames and place m_uCurSample to loop start.
			// IMPORTANT: Do not call OnLoopComplete. In virtual mode, only the current sample count is 
			// maintained, and sources have released their resources. They should get up to speed in VirtualOff().
			if ( m_uLoopCnt > 0 )
				--m_uLoopCnt;

			AKASSERT( io_uFrames >= (m_uPCMLoopEnd + 1 - uCurrSample) );
			io_uFrames = m_uPCMLoopEnd + 1 - uCurrSample;
			m_uCurSample = m_uPCMLoopStart;
		}
	}

	// Post markers and update position info.
	TimeSkipMarkersAndPosition( uCurrSample, io_uFrames, m_uTotalSamples );

	return eResult;
}

AKRESULT CAkSrcBaseEx::Seek()
{
	if ( m_pCtx->RequiresSourceSeek() )
	{
		AKRESULT eResult = CAkVPLSrcNode::Seek();
		// Ignore return code and do this anyway.
		UpdatePositionInfo( 0.0f, m_uCurSample, m_uTotalSamples );
		return eResult;
	}
	return AK_Success;
}

//#ifdef AK_VITA_HW
// Update current position for hardware voices.
AKRESULT CAkSrcBaseEx::HardwareVoiceUpdate( AkUInt32 in_NextSample, AkReal32 in_fPitch )
{
	AKRESULT eResult = AK_DataReady;
	
	UpdatePositionInfo( in_fPitch, m_uCurSample, m_uTotalSamples );

	if ( in_NextSample < m_uCurSample )
	{
		// Markers and position info.
		NotifyRelevantMarkers( m_uCurSample, m_uPCMLoopEnd+1 );
		NotifyRelevantMarkers( m_uPCMLoopStart, in_NextSample );

		eResult = OnLoopComplete( !DoLoop() );
		if ( eResult != AK_NoMoreData )
		{
			in_NextSample = m_uPCMLoopStart;  // Vita Todo: if needed = loop start + drift
		}	
	}
	else
	{
		// Markers and position info.
		NotifyRelevantMarkers( m_uCurSample, in_NextSample );
	}

	m_uCurSample = in_NextSample;
	return eResult;
}
//#endif

//-----------------------------------------------------------------------------
// Name: GetDuration()
// Desc: Get the duration of the source.
//
// Return: AkReal32 : duration of the source in milliseconds.
//
//-----------------------------------------------------------------------------
AkReal32 CAkSrcBaseEx::GetDuration() const
{	 
    AkUInt16 uNumLoops = m_pCtx->GetLooping();
    if ( uNumLoops == 0 )
        return 0;

	AkReal32 fTotalNumSamples = (AkReal32)m_uTotalSamples + (AkReal32)(uNumLoops-1)*( m_uPCMLoopEnd + 1 - m_uPCMLoopStart );
	return ( fTotalNumSamples * 1000.f ) / (AkReal32)m_pCtx->GetMediaFormat().uSampleRate;		// mSec.
}

AkReal32 CAkSrcBaseEx::GetDurationNoLoop() const
{
    return ( m_uTotalSamples * 1000.f ) / (AkReal32)m_pCtx->GetMediaFormat().uSampleRate;	// mSec.
}

// Returns estimate of relative loudness at current position, compared to the loudest point of the sound, in dBs (always negative).
// in_uBufferedFrames is the number of samples that are currently buffered in the pitch node (used for interpolation).
// Sources hold the position of the next buffer to be acquired, so they substract in_uBufferedFrames
// from it to get a position that is more accurate than simply "the next block".
AkReal32 CAkSrcBaseEx::GetAnalyzedEnvelope( AkUInt32 in_uBufferedFrames )
{
	// Linear search, starting with last point visited.
	if ( m_pAnalysisData && m_pAnalysisData->uNumEnvelopePoints > 0 )
	{
		AKASSERT( m_uLastEnvelopePtIdx < m_pAnalysisData->uNumEnvelopePoints );
		
		const AkUInt32 uNumEntries = m_pAnalysisData->uNumEnvelopePoints;
		AkUInt32 uCurEntry = m_uLastEnvelopePtIdx;
		AkFileParser::EnvelopePoint * pPoint = m_pAnalysisData->arEnvelope + uCurEntry;
		AkUInt32 uPrevPosition = pPoint->uPosition;
		AkUInt16 uPrevAttenuation = pPoint->uAttenuation;
		AkUInt32 uNextEntry = uCurEntry + 1;

		// Note: in_uBufferedFrames may be larger than m_uCurSample after looping.
		const AkUInt32 uCurSample = ( in_uBufferedFrames <= m_uCurSample ) ? (m_uCurSample - in_uBufferedFrames) : 0;
		
		do
		{
			while ( uNextEntry < uNumEntries )
			{
				pPoint = m_pAnalysisData->arEnvelope + uNextEntry;
				AkUInt32 uNextPosition = pPoint->uPosition;
				if ( uCurSample >= uPrevPosition
					&& uCurSample < uNextPosition )
				{
					// CurSample is in range [curEntry.position,nextEntry.position[
					m_uLastEnvelopePtIdx = uCurEntry;

					// Linear interpolation.
					AkReal32 fPrevAttenuation = (AkReal32)uPrevAttenuation;
					AkReal32 fAttenuation = fPrevAttenuation + ( ( uCurSample - uPrevPosition ) * ( (AkReal32)pPoint->uAttenuation - fPrevAttenuation ) ) / ( uNextPosition - uPrevPosition );

					// Normalize.
					return -fAttenuation - m_pAnalysisData->fEnvelopePeak;
				}
				uPrevPosition = uNextPosition;
				uPrevAttenuation = pPoint->uAttenuation;
				++uCurEntry;
				++uNextEntry;
			}

			// Last point?
			if ( uCurSample >= pPoint->uPosition )
			{
				m_uLastEnvelopePtIdx = uCurEntry;
				
				// Normalize.
				return -(AkReal32)pPoint->uAttenuation - m_pAnalysisData->fEnvelopePeak;
			}

			// Did not find point (must have looped). Restart from beginning.
			// Note: loop interpolation is not supported.
			uCurEntry = 0;
			pPoint = m_pAnalysisData->arEnvelope;
			uPrevPosition = pPoint->uPosition;
			uPrevAttenuation = pPoint->uAttenuation;
			uNextEntry = 1;
		}
		while ( true );
	}
	return 0;
}

// Prepare output buffer and update source. Call this at the end of GetBuffer().
// - Attaches a buffer to the pipeline's io_state.
// - Posts markers and position information.
// - Updates the output (PCM) sample position. Call this after having produced output data
// in GetBuffer(). 
// - Handles source-side looping, invokes OnLoopComplete() if applicable.
// - Sets the status to pass to the pipeline: AK_NoDataReady, AK_DataReady, AK_NoMoreData, AK_Fail.
// IMPORTANT: Do not call this if there had been an error.
void CAkSrcBaseEx::SubmitBufferAndUpdate( 
	void * in_pData, 
	AkUInt16 in_uNumSamplesProduced, 
	AkUInt32 in_uSampleRate, 
	AkChannelMask in_channelMask, 
	AkVPLState & io_state
	)
{
	AKASSERT( m_uCurSample + in_uNumSamplesProduced <= m_uTotalSamples );

	if ( in_uNumSamplesProduced == 0 )
	{
		io_state.uValidFrames = 0;
		io_state.result = AK_NoDataReady;
		return;
	}

	// Prepare VPL buffer. Assume data is ready (not end of file).
	io_state.AttachInterleavedData( in_pData, in_uNumSamplesProduced, in_uNumSamplesProduced, in_channelMask );

	// Markers and position info.
	CopyRelevantMarkers( io_state, m_uCurSample );

	io_state.posInfo.uSampleRate = in_uSampleRate;
	io_state.posInfo.uStartPos = m_uCurSample;
	io_state.posInfo.uFileEnd = m_uTotalSamples;

	m_uCurSample += in_uNumSamplesProduced;

	io_state.result = HandleLoopingOrEndOfFile();
}

// Sub-helper: Compares the output sample counter and returns 
// AK_NoMoreData if the end of file was reached, or AK_DataReady otherwise.
// Invokes OnLoopComplete() loop if applicable.
AKRESULT CAkSrcBaseEx::HandleLoopingOrEndOfFile()
{
	AKASSERT( m_uCurSample <= m_uTotalSamples );

	AKRESULT eResult = AK_DataReady;

	// Resolve looping or end of file.
	if ( !DoLoop() )
	{
		if ( m_uCurSample >= m_uTotalSamples )
		{			
			// Loop not pending: end of file
			eResult = OnLoopComplete( true );
		}
	}
	else
	{
		// Loop pending. 
		// Number of frames produced should have been truncated up to the loop end.
		AKASSERT( m_uCurSample <= ( m_uPCMLoopEnd + 1 ) );

		if ( m_uCurSample > m_uPCMLoopEnd ) // >= ( m_uPCMLoopEnd + 1 )
		{
			// Reached end of loop.
			m_uCurSample = m_uPCMLoopStart;
			eResult = OnLoopComplete( false );
		}
	}
	return eResult;
}

void CAkSrcBaseEx::CopyRelevantMarkers( 
	AkPipelineBuffer & io_buffer, 
	AkUInt32 in_ulBufferStartPos
	)
{
	m_markers.CopyRelevantMarkers( 
		m_pCtx,
		io_buffer, 
		in_ulBufferStartPos );
}

void CAkSrcBaseEx::NotifyRelevantMarkers( AkUInt32 in_uStartSample, AkUInt32 in_uEndSample )	// Region: [in_uStartSample, in_uEndSample[
{
	m_markers.NotifyRelevantMarkers( m_pCtx, in_uStartSample, in_uEndSample );
}

void CAkSrcBaseEx::UpdatePositionInfo( AkReal32 in_fLastRate, AkUInt32 in_uStartPos, AkUInt32 in_uFileEnd )
{
	if ( m_pCtx->GetRegisteredNotif() & AK_EnableGetSourcePlayPosition )
	{
		AkBufferPosInformation bufferPosInfo;
			
		bufferPosInfo.uSampleRate = m_pCtx->GetMediaFormat().uSampleRate;
		bufferPosInfo.uStartPos = in_uStartPos;
		bufferPosInfo.uFileEnd = in_uFileEnd;
		bufferPosInfo.fLastRate = in_fLastRate;

		g_pPositionRepository->UpdatePositionInfo( m_pCtx->GetPlayingID(), &bufferPosInfo, this );
	}
}

void CAkSrcBaseEx::TimeSkipMarkersAndPosition( AkUInt32 in_ulCurrSampleOffset, AkUInt32 in_uSkippedSamples, AkUInt32 in_uFileEnd ) // Region: [in_ulCurrSampleOffset, in_ulCurrSampleOffset+in_uSkippedSamples[
{
	m_markers.TimeSkipMarkers( m_pCtx, in_ulCurrSampleOffset, in_uSkippedSamples, in_uFileEnd );
	UpdatePositionInfo( 1.0f, in_ulCurrSampleOffset, in_uFileEnd );
}

// Get the seek value from PBI and returns it as the number 
// of PCM samples (source's rate) from the beginning of the file.
// Considers markers if applicable, and adjusts the loop count.
AkUInt32 CAkSrcBaseEx::GetSourceOffset()
{
	AKASSERT( m_pCtx->RequiresSourceSeek() );

	// Get the seek position from PBI (absolute, with looping unwrapped).
	AkUInt32 uAbsoluteSourceOffset;
	bool bSnapToMarker;
	if ( m_pCtx->IsSeekRelativeToDuration() )
	{
		// Special case infinite looping.
		AkReal32 uSourceDuration = ( m_pCtx->GetLooping() != LOOPING_INFINITE ) ? GetDuration() : GetDurationNoLoop();	
		uAbsoluteSourceOffset = m_pCtx->GetSeekPosition( uSourceDuration, bSnapToMarker );
	}
	else
	{
		uAbsoluteSourceOffset = m_pCtx->GetSeekPosition( bSnapToMarker );
	}

	// Convert the absolute seeking value into relative value from the beginning
	// of the file, adjusting the loop count.
	AkUInt32 uRelativeSampleOffset;
	AbsoluteToRelativeSourceOffset( uAbsoluteSourceOffset, uRelativeSampleOffset, m_uLoopCnt );

	// Snap value to marker if applicable.
	if ( bSnapToMarker )
	{
		const AkAudioMarker * pMarker = m_markers.GetClosestMarker( uRelativeSampleOffset );
		if ( pMarker )
		{
			uRelativeSampleOffset = pMarker->dwPosition;
			// Now that we've found the real position, we need to pass it again through the conversion
			// in case the marker was over, or slightly after the loop point.
			::AbsoluteToRelativeSourceOffset( uRelativeSampleOffset,
                                    m_uPCMLoopStart,
                                    m_uPCMLoopEnd,
                                    m_uLoopCnt,
                                    uRelativeSampleOffset, 
                                    m_uLoopCnt
                                    );
		}
		else
		{
			// No marker: cannot snap to anything.
			MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_SeekNoMarker, m_pCtx );
		}
	}

	return uRelativeSampleOffset;
}

// Helper: Converts an absolute source position (which takes looping region into account) into 
// a value that is relative to the beginning of the file, and the number of loops remaining.
void CAkSrcBaseEx::AbsoluteToRelativeSourceOffset( 
	AkUInt32 in_uAbsoluteSourcePosition, 
	AkUInt32 & out_uRelativeSourceOffset,
	AkUInt16 & out_uRemainingLoops 
	) const
{
    ::AbsoluteToRelativeSourceOffset( in_uAbsoluteSourcePosition,
                                    m_uPCMLoopStart,
                                    m_uPCMLoopEnd,
                                    m_pCtx->GetLooping(),
                                    out_uRelativeSourceOffset, 
                                    out_uRemainingLoops
                                    );
}
