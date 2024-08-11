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

#include "stdafx.h"
#include "AkSrcFilePCM.h"
#include "AkFileParserBase.h"
#include "AkMonitor.h"
#include "AkProfile.h"

CAkSrcFilePCM::CAkSrcFilePCM( CAkPBI * in_pCtx )
	: CAkSrcFileBase( in_pCtx )
	, m_pStitchBuffer( NULL )
	, m_uNumBytesBuffered( 0 )
	, m_uSizeToRelease( 0 )
{
}

CAkSrcFilePCM::~CAkSrcFilePCM()
{
}

void CAkSrcFilePCM::StopStream()
{
	if ( m_pStitchBuffer )
	{
		AkFree( g_LEngineDefaultPoolId, m_pStitchBuffer );
		m_pStitchBuffer = NULL;
		m_uNumBytesBuffered = 0;
	}
	CAkSrcFileBase::StopStream();
}

// IMPORTANT: Never access BlockAlign from PBI format! 
void CAkSrcFilePCM::GetBuffer( AkVPLState & io_state )
{
    AKASSERT( m_pStream != NULL );

	AKRESULT eResult = IsPrebufferingReady();
	if ( AK_EXPECT_FALSE( eResult != AK_DataReady ) )
	{
		io_state.result = eResult;
		return;
	}

    // See if we need to get more data from stream manager.
    if ( m_ulSizeLeft == 0 )
    {
    	AKASSERT( !HasNoMoreStreamData() );
		eResult = FetchStreamBuffer();
		if ( AK_EXPECT_FALSE( eResult != AK_DataReady ) )
		{
			io_state.result = eResult;
			return;
		}
	}

    // Deal with NoMoreData return flag.
	AkUInt16 usBlockAlign = GetBlockAlign();
	// Number of whole sample frames.
	AkUInt32 uFramesLeft = m_ulSizeLeft / usBlockAlign;

	// At this point, some data is ready, or there is no more data.
    // Maybe loop end or data chunk sizes were invalid.
    if ( m_ulSizeLeft == 0 && HasNoMoreStreamData() )
    {
        AKASSERT( !"Invalid loop back boundary. Wrong values in file header? Source failure." );
		io_state.result = AK_Fail;
        return;
    }

	const AkAudioFormat& rFormatInfo = m_pCtx->GetMediaFormat();

    // Give what the client wants, or what we have.
    AkUInt16 uMaxFrames = (AkUInt16)AkMin( io_state.MaxFrames(), uFramesLeft );

	void * pSubmitData = NULL;
    
    // Might need to buffer streamed data if previous sample frame was split between
	// 2 streaming buffers (this should never happen in mono or stereo).
	if ( 0 == m_uNumBytesBuffered )
	{
		// Using streamed buffer directly: free any previously allocated stitch buffer.
		if ( m_pStitchBuffer )
		{
			AkFree( g_LEngineDefaultPoolId, m_pStitchBuffer );
			m_pStitchBuffer = NULL;
		}

		// Submit streaming buffer directly.
		pSubmitData = m_pNextAddress;

		m_uSizeToRelease = uMaxFrames * usBlockAlign;

		// Check if data left after this frame represents less than one whole sample frame.
		// In such a case, buffer it.
		AKASSERT( m_ulSizeLeft >= m_uSizeToRelease );
		AkUInt32 uSizeLeftAfterRelease = m_ulSizeLeft - m_uSizeToRelease;
		if ( uSizeLeftAfterRelease < usBlockAlign 
			&& uSizeLeftAfterRelease > 0 )
		{
			AKASSERT( !m_pStitchBuffer );
			m_pStitchBuffer = (AkUInt8*)AkAlloc( g_LEngineDefaultPoolId, usBlockAlign );

			if ( m_pStitchBuffer )
			{			
				m_uNumBytesBuffered = (AkUInt16)uSizeLeftAfterRelease;
				AKPLATFORM::AkMemCpy( m_pStitchBuffer, m_pNextAddress + m_uSizeToRelease, m_uNumBytesBuffered );

				// Increment m_uSizeToRelease a bit so that pointers updates in ReleaseBuffer()
				// take it into account.
				m_uSizeToRelease += m_uNumBytesBuffered;
			}
			else
			{
				// Cannot allocate stitch buffer: This error is unrecoverable. 
				io_state.result = AK_Fail;
				return;
			}
		}
	}
	else
	{
		AKASSERT( m_pStitchBuffer );
		AKASSERT( m_uNumBytesBuffered < usBlockAlign );
		AkUInt32 uNumBytesToCopy = usBlockAlign - m_uNumBytesBuffered;
		if ( uNumBytesToCopy > m_ulSizeLeft )
		{
			AKASSERT( !"Streaming granularity is smaller than the size of a sample frame" );
			io_state.result = AK_Fail;
			return;
		}

		AKASSERT( m_uNumBytesBuffered + uNumBytesToCopy == usBlockAlign ); 
		AKPLATFORM::AkMemCpy( m_pStitchBuffer + m_uNumBytesBuffered, m_pNextAddress, uNumBytesToCopy );

		// Now that we prepared our stitch buffer for output, recompute data size presented to the pipeline.
		uMaxFrames = 1;

		// Reset state. 
		m_uSizeToRelease = (AkUInt16)uNumBytesToCopy;
		m_uNumBytesBuffered = 0;

		// Submit stitch buffer.
		pSubmitData = m_pStitchBuffer;
	}

	// Prepare buffer for VPL, update PCM position, handle end of loop/file, post markers and position info.
	SubmitBufferAndUpdate( pSubmitData, uMaxFrames, rFormatInfo.uSampleRate, rFormatInfo.uChannelMask, io_state );
}


void CAkSrcFilePCM::ReleaseBuffer()
{
    AKASSERT( m_pStream != NULL );
    AKASSERT( m_uSizeToRelease <= m_ulSizeLeft || !"Invalid released data size" );

	m_ulSizeLeft -= m_uSizeToRelease;
    m_pNextAddress += m_uSizeToRelease;
	m_uSizeToRelease = 0;	

	if ( m_ulSizeLeft == 0 )
		ReleaseStreamBuffer();
}

void CAkSrcFilePCM::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	// Free stitch buffer if playback will be reset (that is, FromBeginning or 
	// FromElapsedTime before last streaming buffer.
	if ( m_pStitchBuffer )
	{
		if ( eBehavior == AkVirtualQueueBehavior_FromBeginning 
			|| eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
		{
			AkFree( g_LEngineDefaultPoolId, m_pStitchBuffer );
			m_pStitchBuffer = NULL;
			m_uNumBytesBuffered = 0;
		}
	}
	CAkSrcFileBase::VirtualOn( eBehavior );
}

AKRESULT CAkSrcFilePCM::ParseHeader( 
	AkUInt8 * in_pBuffer	// Buffer to parse
	)
{
    AKASSERT( m_pStream );

    // Got the first buffer. Parse.
	AkFileParser::FormatInfo fmtInfo;
	AkFileParser::AnalysisDataChunk analysisDataChunk;
	AKRESULT eResult = AkFileParser::Parse( in_pBuffer,			// Data buffer.
											m_ulSizeLeft,		// Buffer size.
											fmtInfo,			// Returned audio format info.
											&m_markers,			// Markers.
											&m_uPCMLoopStart,	// Beginning of loop.
											&m_uPCMLoopEnd,		// End of loop (inclusive).
											&m_uDataSize,		// Data size.
											&m_uDataOffset,		// Offset to data.
											&analysisDataChunk,	// Analysis data.
											NULL );				// (Format-specific) seek table info (pass NULL is not expected).

    if ( eResult != AK_Success )
    {
        MONITOR_SOURCE_ERROR( AkFileParser::ParseResultToMonitorMessage( eResult ), m_pCtx );
        return eResult;
    }

	//Check if the file is of the expected format: WAVE_FORMAT_EXTENSIBLE
	if (fmtInfo.pFormat->wFormatTag != WAVE_FORMAT_EXTENSIBLE)
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_FileFormatMismatch, m_pCtx );
		return AK_InvalidFile;
	}

	WaveFormatExtensible * pFmt = fmtInfo.pFormat;
	AKASSERT( fmtInfo.uFormatSize == sizeof( WaveFormatExtensible ) );

	//Setup format on the PBI	
	AkAudioFormat format;
	format.SetAll(
		pFmt->nSamplesPerSec, 
		pFmt->dwChannelMask, 
		pFmt->wBitsPerSample, 
		pFmt->nBlockAlign, 
		AK_INT,								
		AK_INTERLEAVED);					

	m_pCtx->SetMediaFormat(format);

	// Store analysis data if it was present. 
	// Note: StoreAnalysisData may fail, but it is not critical.
	if ( analysisDataChunk.uDataSize > 0 )
		StoreAnalysisData( analysisDataChunk );

	m_uTotalSamples = m_uDataSize / pFmt->nBlockAlign;

	
	AKASSERT( m_uDataOffset % 4 == 0 );

    // Set loop points.
    AkUInt32 ulEndOfData = m_uDataOffset + m_uDataSize;

    // Loop points. If not looping or ulLoopEnd is 0 (no loop points),
    // set loop points to data chunk boundaries.
    if ( 0 == m_uPCMLoopEnd 
		|| !DoLoop() )
    {
        // Loop start = start of data.
        m_ulLoopStart   = m_uDataOffset;
        // Loop end = end of data.
        m_ulLoopEnd    = ulEndOfData;
        
        // Fix PCM LoopEnd:
        AKASSERT( ( ( m_ulLoopEnd - m_uDataOffset ) / pFmt->nBlockAlign ) >= 1 );
		m_uPCMLoopEnd  = ( m_ulLoopEnd - m_uDataOffset ) / pFmt->nBlockAlign - 1;
    }
    else
    {	
		// If LoopEnd is non zero, then it is in sample frames and the loop should include the end sample.
        // Convert to bytes offset, from beginning of FILE.
		AkUInt32 uiBlockAlign = pFmt->nBlockAlign;

		m_ulLoopStart = m_uDataOffset + uiBlockAlign*m_uPCMLoopStart;
		m_ulLoopEnd  = m_uDataOffset + uiBlockAlign*(m_uPCMLoopEnd+1);

		// Verify loop. Invalid if loop start or loop end passed the last sample of the file, or if loop end before loop start.
	    if ( m_uPCMLoopEnd < m_uPCMLoopStart ||
			 m_ulLoopStart > ulEndOfData || 
	         m_ulLoopEnd > ulEndOfData )
	    {
	    	MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader, m_pCtx );
	        return AK_InvalidFile;
	    }
    }

    // Update stream heuristics.
	AkAutoStmHeuristics heuristics;
    m_pStream->GetHeuristics( heuristics );

    // Throughput.
    heuristics.fThroughput = (AkReal32) ( pFmt->nBlockAlign * pFmt->nSamplesPerSec ) / 1000.f;

    // Looping.
    if ( DoLoop() )
    {
        heuristics.uLoopStart = m_ulLoopStart;
        heuristics.uLoopEnd = m_ulLoopEnd;
    }

    // Priority.
    heuristics.priority = m_pCtx->GetPriority();

    m_pStream->SetHeuristics( heuristics );

	// Update stream buffering settings.
	// Cannot receive less than an whole sample frame.
	return m_pStream->SetMinimalBufferSize( pFmt->nBlockAlign );
}

// Returns format-specific throughput.
AkReal32 CAkSrcFilePCM::GetThroughput( const AkAudioFormat & in_rFormat )	
{
	return (AkReal32)( GetBlockAlign() * in_rFormat.uSampleRate ) / 1000.f;
}

// Finds the closest offset in file that corresponds to the desired sample position.
// Returns the file offset (in bytes, relative to the beginning of the file) and its associated sample position.
// Returns AK_Fail if the codec is unable to seek.
AKRESULT CAkSrcFilePCM::FindClosestFileOffset( 
	AkUInt32 in_uDesiredSample,		// Desired sample position in file.
	AkUInt32 & out_uSeekedSample,	// Returned sample where file position was set.
	AkUInt32 & out_uFileOffset		// Returned file offset where file position was set.
	)
{
	// IMPORTANT: Never access BlockAlign from PBI format!
	out_uSeekedSample = in_uDesiredSample;
	out_uFileOffset = m_uDataOffset + in_uDesiredSample * GetBlockAlign();
	return AK_Success;
}

// Returns block align. This function is virtual, in order for derived sources to manage their own 
// block align independently of that of the pipeline/PBI (read 24 bits).
AkUInt16 CAkSrcFilePCM::GetBlockAlign() const
{
	return (AkUInt16)m_pCtx->GetMediaFormat().GetBlockAlign();
}

// Overrides CAkSrcFileBase in order to clear out PCM-specific status
AKRESULT CAkSrcFilePCM::ChangeSourcePosition()
{
	m_uSizeToRelease = 0;
	if ( m_pStitchBuffer )
	{
		AkFree( g_LEngineDefaultPoolId, m_pStitchBuffer );
		m_pStitchBuffer = NULL;
		m_uNumBytesBuffered = 0;
	}
	return CAkSrcFileBase::ChangeSourcePosition();
}

bool CAkSrcFilePCM::MustRelocatePitchInputBufferOnMediaRelocation()
{
	return m_bIsReadingPrefecth;
}

