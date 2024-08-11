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
#include "AkSrcBankADPCM.h"
#include "AkLEngine.h"
#include "AkFileParserBase.h"
#include "AkMonitor.h"

//-----------------------------------------------------------------------------
// Name: CAkSrcBankADPCM
// Desc: Constructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSrcBankADPCM::CAkSrcBankADPCM( CAkPBI * in_pCtx )
: CAkSrcBaseEx( in_pCtx )
, m_pucData( NULL )
, m_pOutBuffer( NULL )
{
}

CAkSrcBankADPCM::~CAkSrcBankADPCM()
{
	AKASSERT( !m_pOutBuffer );
}

AKRESULT CAkSrcBankADPCM::StartStream()
{
	AkUInt8 * pvBuffer;
    AkUInt32 ulBufferSize;
	m_pCtx->GetDataPtr( pvBuffer, ulBufferSize );
    if ( pvBuffer == NULL )
		return AK_Fail;

    AkFileParser::FormatInfo fmtInfo;
	AkFileParser::AnalysisDataChunk analysisDataChunk;
	AKRESULT eResult = AkFileParser::Parse( pvBuffer,			// Data buffer
											ulBufferSize,		// Buffer size
											fmtInfo,			// Returned audio format.
											&m_markers,			// Markers.
											&m_uPCMLoopStart,	// Beginning of loop.
											&m_uPCMLoopEnd,		// End of loop (inclusive).
											&m_uDataSize,		// Data size.
											&m_uDataOffset,		// Offset to data.
											&analysisDataChunk,	// Analysis info
											NULL );				// (Format-specific) seek table info (pass NULL is not expected).
							
	if ( eResult != AK_Success )
    {
        MONITOR_SOURCE_ERROR( AkFileParser::ParseResultToMonitorMessage( eResult ), m_pCtx );
		return AK_InvalidFile;
    }

	//Check if the file is of the expected format
	if (fmtInfo.pFormat->wFormatTag != WAVE_FORMAT_ADPCM)
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
		16, 
		2 * pFmt->nChannels, 
		AK_INT,								
		AK_INTERLEAVED);					

	m_pCtx->SetMediaFormat(format);

	if ( analysisDataChunk.uDataSize > 0 )
		m_pAnalysisData = analysisDataChunk.pData;

	AKASSERT( CAkADPCMCodec::IsValidImaAdpcmFormat( *pFmt ) );

	m_uInputBlockSize = pFmt->nBlockAlign;
	m_uTotalSamples = m_uDataSize * ADPCM_SAMPLES_PER_BLOCK / m_uInputBlockSize;

	// Set Loop point to end of file if not looping sound or if there are no loop points.
	// Note. If there are no loop points, LoopEnd is set to 0. 
	if ( m_uPCMLoopEnd == 0 
		|| !DoLoop() )
	{
		// No loop point. 
		m_uPCMLoopEnd = m_uTotalSamples - 1;
	}
	
	// Verify data buffer consistency.
	if ( m_uPCMLoopEnd < m_uPCMLoopStart ||
		m_uPCMLoopEnd >= m_uTotalSamples ||
		ulBufferSize != (m_uDataOffset + m_uDataSize) )
	{
        MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader, m_pCtx );
		eResult = AK_Fail;
	}
	
	AKASSERT( ( m_uPCMLoopStart % ADPCM_SAMPLES_PER_BLOCK ) == 0 
			&& ( (m_uPCMLoopEnd+1) % ADPCM_SAMPLES_PER_BLOCK ) == 0 );

	// Init state.
	m_uCurSample = 0;
	m_pucData = pvBuffer + m_uDataOffset;

	if ( m_pCtx->RequiresSourceSeek() )
		return SeekToSourceOffset();

	return eResult;
}

bool CAkSrcBankADPCM::SupportMediaRelocation() const
{
	return true;
}

AKRESULT CAkSrcBankADPCM::RelocateMedia( AkUInt8* in_pNewMedia, AkUInt8* in_pOldMedia )
{
	m_pucData = m_pucData + (AkUIntPtr)in_pNewMedia - (AkUIntPtr)in_pOldMedia;
	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: StopStream
// Desc: Stop streaming data.
//
// Return: Ak_Success:          Stream was stopped.
//         AK_InvalidParameter: Invalid parameters.
//         AK_Fail:             Failed to stop the stream.
//-----------------------------------------------------------------------------
void CAkSrcBankADPCM::StopStream()
{
	ReleaseBuffer();
	CAkSrcBaseEx::StopStream();
}

void CAkSrcBankADPCM::ReleaseBuffer()
{
	if ( m_pOutBuffer )
	{
		AkUInt32 uPCMBlockAlign = GetNumChannels( m_pCtx->GetMediaFormat().GetChannelMask() ) * sizeof(AkInt16);
		CAkLEngine::ReleaseCachedAudioBuffer( AK_NUM_VOICE_REFILL_FRAMES * uPCMBlockAlign, m_pOutBuffer );
		m_pOutBuffer = NULL;
	}
}

void CAkSrcBankADPCM::GetBuffer( AkVPLState & io_state )
{	
	const AkAudioFormat& rFormat = m_pCtx->GetMediaFormat();

	AkChannelMask uChannelMask = rFormat.GetChannelMask();
	AkUInt32 uNumChannels = GetNumChannels( uChannelMask );
	AkUInt32 ulPCMBlockAlign = uNumChannels * sizeof(AkInt16);

	// Allocate output buffer for decoded data
	AKASSERT( !m_pOutBuffer );
	m_pOutBuffer = (AkUInt8 *) CAkLEngine::GetCachedAudioBuffer( AK_NUM_VOICE_REFILL_FRAMES * ulPCMBlockAlign ); 
	if ( !m_pOutBuffer )
	{
		io_state.result = AK_Fail;
        return;
	}

	AkPrefetchZeroAligned(m_pOutBuffer, AK_NUM_VOICE_REFILL_FRAMES * ulPCMBlockAlign);

	// Clamp requested frames to loop end or eof.
	AkUInt16 uNumFrames = io_state.MaxFrames();
	ClampRequestedFrames( uNumFrames );

	AkUInt32 nADPCMBlocks = uNumFrames / ADPCM_SAMPLES_PER_BLOCK; // number of ADPCM blocks to process to fill the output buffer
	AKASSERT( nADPCMBlocks > 0 );	// cannot starve, should have already returned AK_NoMoreData if finished.
	uNumFrames = (AkUInt16)( nADPCMBlocks * ADPCM_SAMPLES_PER_BLOCK );	// Round to ADPCM block

	AkUInt8 * pOutBuffer = m_pOutBuffer;
	for ( AkUInt32 iChan = 0; iChan < uNumChannels; ++iChan )
		CAkADPCMCodec::Decode( m_pucData + iChan * ADPCM_BLOCK_SIZE, pOutBuffer + iChan * sizeof(AkInt16), nADPCMBlocks, m_uInputBlockSize, uNumChannels );

	// Move input pointer.
	// Note: If loop occurs, it is placed at loop start in loop complete handler.
	m_pucData += ( nADPCMBlocks * m_uInputBlockSize );

	// Submit.
	SubmitBufferAndUpdate( 
		m_pOutBuffer,	// in_pData, 
		uNumFrames,
		rFormat.uSampleRate,
		uChannelMask, 
		io_state );
}

// Override OnLoopComplete() handler: Place input pointer.
AKRESULT CAkSrcBankADPCM::OnLoopComplete(
	bool in_bEndOfFile		// True if this was the end of file, false otherwise.
	)
{
	//if ( !in_bEndOfFile ) doesn't matter...
	m_pucData = ConvertToInputDataAddress( m_uCurSample );
	return CAkSrcBaseEx::OnLoopComplete( in_bEndOfFile );
}

AKRESULT CAkSrcBankADPCM::VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset )
{
	if ( eBehavior == AkVirtualQueueBehavior_FromBeginning )
	{
		m_uCurSample = 0;
		m_pucData = ConvertToInputDataAddress( m_uCurSample );
		ResetLoopCnt();
	}
	else if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
	{
		if ( in_bUseSourceOffset )
			return SeekToSourceOffset();
		else
			m_pucData = ConvertToInputDataAddress( m_uCurSample );
	}	
	return AK_Success;
}

AKRESULT CAkSrcBankADPCM::SeekToSourceOffset()
{
	if ( m_pCtx->RequiresSourceSeek() )
	{
		AkUInt32 uSourceOffset = GetSourceOffset();

		AkUInt32 l_numADPCMBlock = ( uSourceOffset / ADPCM_SAMPLES_PER_BLOCK );
		m_uCurSample = l_numADPCMBlock * ADPCM_SAMPLES_PER_BLOCK;

		m_pCtx->SetSourceOffsetRemainder( uSourceOffset - m_uCurSample );

		if ( m_uCurSample < m_uTotalSamples )
		{
			// Fix input.
			m_pucData = ConvertToInputDataAddress( m_uCurSample );
			return AK_Success;
		}
		else
		{
			MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_SeekAfterEof, m_pCtx );
			return AK_Fail;
		}
	}
	return AK_Success;
}

AKRESULT CAkSrcBankADPCM::ChangeSourcePosition()
{
	return SeekToSourceOffset();
}

