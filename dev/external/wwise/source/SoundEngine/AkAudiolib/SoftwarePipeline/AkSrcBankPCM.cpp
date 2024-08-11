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
#include "AkSrcBankPCM.h"
#include "AkFileParserBase.h"
#include "AkMonitor.h"
#include <AK/MotionEngine/Common/IAkMotionMixBus.h>

//-----------------------------------------------------------------------------
// Name: CAkSrcBankPCM
// Desc: Constructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSrcBankPCM::CAkSrcBankPCM( CAkPBI * in_pCtx )
#ifdef AK_VITA_HW
: CAkSrcBankNgsHw( in_pCtx )
#else
: CAkSrcBaseEx( in_pCtx )
#endif

{
}

//-----------------------------------------------------------------------------
// Name: ~CAkSrcBankPCM
// Desc: Destructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSrcBankPCM::~CAkSrcBankPCM()
{
	ReleaseBuffer();
}

//-----------------------------------------------------------------------------
// Name: StartStream
// Desc: Start to stream data.
//
// Return: Ak_Success:          Stream was started.
//         AK_InvalidParameter: Invalid parameters.
//         AK_Fail:             Failed to start streaming data.
//-----------------------------------------------------------------------------
AKRESULT CAkSrcBankPCM::StartStream()
{
    AkUInt8 * pvBuffer;
    AkUInt32 ulBufferSize;
	m_pCtx->GetDataPtr( pvBuffer, ulBufferSize );
    if ( pvBuffer == NULL )
		return AK_Fail;

	AkFileParser::FormatInfo fmtInfo;
	AkFileParser::AnalysisDataChunk analysisDataChunk;
	AKRESULT eResult = AkFileParser::Parse( pvBuffer,			// Data buffer.
											ulBufferSize,		// Buffer size.
											fmtInfo,			// Returned audio format info.
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

	if ( analysisDataChunk.uDataSize > 0 )
		m_pAnalysisData = analysisDataChunk.pData;

	AKASSERT( m_uDataOffset % 4 == 0 );

	m_pucDataStart = pvBuffer + m_uDataOffset;

	AKASSERT( ( m_uDataSize % pFmt->nBlockAlign ) == 0 );
	m_uTotalSamples = m_uDataSize / pFmt->nBlockAlign;
	
	// Parsed loop start and end are relative to start of data chunk, in sample frames. 

	// Set Loop point to end of file if not looping sound or if there are no loop points.
	// Note. If there are no loop points, LoopEnd is set to 0. 
	if ( m_uPCMLoopEnd == 0
		|| !DoLoop() )
	{
		// No loop point. (Loop points inclusive)
		// Fix PCM LoopEnd.
		AKASSERT( ( m_uDataSize / pFmt->nBlockAlign ) >= 1 );
		m_uPCMLoopEnd = m_uDataSize / pFmt->nBlockAlign - 1;
	}
	
	// Verify data buffer consistency.
	if ( m_uPCMLoopEnd < m_uPCMLoopStart ||
		m_uPCMLoopEnd >= m_uTotalSamples ||
		ulBufferSize != (m_uDataOffset + m_uDataSize) )
	{
        MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader, m_pCtx );
		return AK_Fail;
	}

	// Init state.
	m_uCurSample = 0;

	if ( m_pCtx->RequiresSourceSeek() )
		return SeekToSourceOffset();

	return AK_Success;
}

bool CAkSrcBankPCM::SupportMediaRelocation() const
{
	return true;
}

AKRESULT CAkSrcBankPCM::RelocateMedia( AkUInt8* in_pNewMedia, AkUInt8* in_pOldMedia )
{
	if( m_pucDataStart )
	{
		m_pucDataStart = in_pNewMedia + m_uDataOffset;
	}
	return AK_Success;
}

void CAkSrcBankPCM::GetBuffer( AkVPLState & io_state )
{	
	const AkAudioFormat& rFormat = m_pCtx->GetMediaFormat();
	
	// Clamp requested frames to loop end or eof.
	AkUInt16 uNumFrames = io_state.MaxFrames();
	ClampRequestedFrames( uNumFrames );

	// Submit.

	SubmitBufferAndUpdate( 
		m_pucDataStart + m_uCurSample * rFormat.GetBlockAlign(),	// in_pData
		uNumFrames,
		rFormat.uSampleRate,
		rFormat.GetChannelMask(), 
		io_state );
}

AKRESULT CAkSrcBankPCM::VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset )
{
	if ( eBehavior == AkVirtualQueueBehavior_FromBeginning )
	{
		m_uCurSample = 0;
		ResetLoopCnt();
	}
	else if ( in_bUseSourceOffset && eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
		return SeekToSourceOffset();
	
	return AK_Success;
}

AKRESULT CAkSrcBankPCM::SeekToSourceOffset()
{
	m_uCurSample = GetSourceOffset();
	
	m_pCtx->SetSourceOffsetRemainder( 0 );

	if ( m_uCurSample < m_uTotalSamples )
		return AK_Success;
	else
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_SeekAfterEof, m_pCtx );
		return AK_Fail;
	}
}

AKRESULT CAkSrcBankPCM::ChangeSourcePosition()
{
	AKASSERT( m_pCtx->RequiresSourceSeek() );
	return SeekToSourceOffset();
}

