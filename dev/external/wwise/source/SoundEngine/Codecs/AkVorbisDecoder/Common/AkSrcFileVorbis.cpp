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
#include "AkSrcFileVorbis.h"
#include "AkLEngine.h"
#include "AkMonitor.h"
#include "AkProfile.h"
#include "AkVorbisCodebookMgr.h"
#include "AkVorbisCodec.h"
#include <AK/Plugin/AkVorbisFactory.h>	  // For Vorbis CODEC_ID.

#define OUTPUT_CHANNEL_SIZE	(LE_MAX_FRAMES_PER_BUFFER * sizeof(AkInt16))

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
IAkSoftwareCodec* CreateVorbisFilePlugin( void* in_pCtx )
{
	return AkNew( g_LEngineDefaultPoolId, CAkSrcFileVorbis( (CAkPBI*)in_pCtx ) );
}

// Constructor
CAkSrcFileVorbis::CAkSrcFileVorbis( CAkPBI * in_pCtx ) 
: CAkSrcFileBase( in_pCtx )
, m_pStitchStreamBuffer( NULL )
, m_uStitchBufferEndOffset( 0 )
, m_uStitchBufferValidDataSize( 0 )
, m_uStitchBufferLeft( 0 )
{
	AkMemSet(&m_OggPacketHeader,0,sizeof(OggPacketHeader));
	m_pOggPacketData = NULL;
	m_uPacketDataGathered = 0;
	m_uPacketHeaderGathered = 0;

	InitVorbisState();
}

// Destructor
CAkSrcFileVorbis::~CAkSrcFileVorbis()
{
	ReleaseBuffer();
	if ( m_VorbisState.TremorInfo.VorbisDSPState.csi )
		g_VorbisCodebookMgr.ReleaseCodebook( m_VorbisState );
}

// GetBuffer
void CAkSrcFileVorbis::GetBuffer( AkVPLState & io_state )
{
	// The stream should not start before all headers are decoded
	AKASSERT( m_VorbisState.TremorInfo.ReturnInfo.eDecoderState >= PACKET_STREAM );

	// Check prebuffering status.
	AKRESULT eResult = IsPrebufferingReady();
	if ( AK_EXPECT_FALSE( eResult != AK_DataReady ) )
	{
		io_state.result = eResult;
		return;
	}

	// Note: No need to adjust request to for file end or looping as it will finish the decoding a packet anyhow
	m_VorbisState.TremorInfo.uRequestedFrames = io_state.MaxFrames();

	// Note: We may need to span 2 stream buffer to get sufficient input data. When this occur, we prefer to cache the first buffer
	// to be able to release it right away before getting the second one, rather than run the risk that several stream get into this
	// state and none can advance while waiting for some buffers to be freed.

	if ( m_ulSizeLeft < (m_VorbisState.VorbisInfo.uMaxPacketSize+sizeof(OggPacketHeader)) )
	{
		// Note: Signal the end of file so decoder can flag packets appropriately
		if (!HasNoMoreStreamData())
		{
			if ( m_ulSizeLeft > 0 && m_pStitchStreamBuffer == NULL && !Stream_IsLoopPending() )
			{
				// Cache left hand side buffer, release it and retrieve a new buffer
				// Allocate enough space to stitch new buffer after and ensure we have enough data not to get out of the cached
				// buffer portion. Worst-case is if you hit the largest packet start at the very end of cached data -> 2*m_VorbisState.VorbisInfo.uMaxPacketSize will thus suffice.
				m_pStitchStreamBuffer		= (AkUInt8*)AkAlloc( g_LEngineDefaultPoolId, (m_VorbisState.VorbisInfo.uMaxPacketSize+sizeof(OggPacketHeader))+m_ulSizeLeft);
				if ( !m_pStitchStreamBuffer )
				{
					io_state.result = AK_Fail;
					return;
				}
				m_uStitchBufferEndOffset	= m_ulSizeLeft; 
				m_uStitchBufferLeft			= m_ulSizeLeft;	
				m_uStitchBufferValidDataSize = m_ulSizeLeft;
				AkMemCpy( m_pStitchStreamBuffer, m_pNextAddress, m_ulSizeLeft );
				ConsumeData( m_ulSizeLeft );
				AKASSERT( m_ulSizeLeft == 0 );
				m_pNextAddress = NULL;
			}

			if ( m_ulSizeLeft == 0 )
			{
				ReleaseStreamBuffer();
				io_state.result = FetchStreamBuffer();
				if ( io_state.result == AK_Fail )
					return;
				else if ( io_state.result == AK_DataReady && m_pStitchStreamBuffer )
				{
					// Stitch buffer will be used, copy first bit of new stream buffer data to complete stitch buffer
					AkUInt32 uCopySize = AkMin((m_VorbisState.VorbisInfo.uMaxPacketSize+sizeof(OggPacketHeader)),m_ulSizeLeft);
					AkMemCpy( m_pStitchStreamBuffer+m_uStitchBufferEndOffset, m_pNextAddress, uCopySize );
					m_uStitchBufferLeft += uCopySize;
					m_uStitchBufferValidDataSize += uCopySize;
				}
			}
		}
	}

	// Allocate output buffer for decoded data
	if ( m_VorbisState.TremorInfo.pucData == NULL )
	{
		m_VorbisState.TremorInfo.pucData = (AkUInt8 *) CAkLEngine::GetCachedAudioBuffer( m_VorbisState.TremorInfo.VorbisDSPState.channels * OUTPUT_CHANNEL_SIZE );
		m_VorbisState.TremorInfo.ReturnInfo.uFramesProduced = 0;
		if ( m_VorbisState.TremorInfo.pucData == NULL )
		{
			io_state.result = AK_Fail;
			return;
		}
	}

	m_VorbisState.TremorInfo.bNoMoreInputPackets = Stream_IsLoopPending() || HasNoMoreStreamData();

	// Vorbis is ready, simply decode, from the current buffer in use
	if(m_pStitchStreamBuffer)
	{
		// end of input packets only if stitch buffer entirely contains the next stream buffer.
		m_VorbisState.TremorInfo.bNoMoreInputPackets = m_VorbisState.TremorInfo.bNoMoreInputPackets 
			&& ( ( m_ulSizeLeft - ( m_uStitchBufferValidDataSize - m_uStitchBufferEndOffset ) ) == 0 );

		m_VorbisState.TremorInfo.uInputDataSize = m_uStitchBufferLeft;
		AkUInt8 * pStitchStreamBufferPos = m_pStitchStreamBuffer + ( m_uStitchBufferValidDataSize - m_uStitchBufferLeft );
		DecodeVorbis( &m_VorbisState.TremorInfo, m_VorbisState.VorbisInfo.uMaxPacketSize, pStitchStreamBufferPos, (AkInt16*)m_VorbisState.TremorInfo.pucData );
	
		AKASSERT( m_VorbisState.TremorInfo.ReturnInfo.uInputBytesConsumed <= m_uStitchBufferLeft );
		m_uStitchBufferLeft -= m_VorbisState.TremorInfo.ReturnInfo.uInputBytesConsumed;
		AkUInt16 uStitchBufferConsumed = m_uStitchBufferValidDataSize - m_uStitchBufferLeft;
		if ( uStitchBufferConsumed >= m_uStitchBufferEndOffset )
		{
			// May have consumed some data in the current stream buffer as well
			AkUInt32 uSizeConsumed = uStitchBufferConsumed-m_uStitchBufferEndOffset;
			ConsumeData(uSizeConsumed);

			// Consumed all the previous stream buffer part and can stop using stitched data
			FreeStitchBuffer();
		}
	}
	else
	{
		m_VorbisState.TremorInfo.uInputDataSize = m_ulSizeLeft;
		DecodeVorbis( &m_VorbisState.TremorInfo, m_VorbisState.VorbisInfo.uMaxPacketSize, m_pNextAddress, (AkInt16*)m_VorbisState.TremorInfo.pucData );
		ConsumeData(m_VorbisState.TremorInfo.ReturnInfo.uInputBytesConsumed);
	}

	// Prepare buffer for VPL, update PCM position, handle end of loop/file, post markers and position info.
	SubmitBufferAndUpdateVorbis( io_state );
}

// Override SubmitBufferAndUpdate(): Check decoder status, post error message if applicable
// and restart DSP if loop is resolved.
void CAkSrcFileVorbis::SubmitBufferAndUpdateVorbis( AkVPLState & io_state )
{
	io_state.result = m_VorbisState.TremorInfo.ReturnInfo.eDecoderStatus;

	if ( io_state.result != AK_Fail )
	{
		// Prepare buffer for VPL, update PCM position, handle end of loop/file, post markers and position info.
		SubmitBufferAndUpdate( 
			m_VorbisState.TremorInfo.pucData, 
			m_VorbisState.TremorInfo.ReturnInfo.uFramesProduced, 
			m_VorbisState.uSampleRate,
			m_VorbisState.TremorInfo.uChannelMask, 
			io_state );

		if ( AK_EXPECT_FALSE( io_state.result == AK_NoDataReady && m_pNextAddress ) ) // Handle decoder starvation: check note below about this condition.
		{
			// The _decoder_ returned no data ready, although streaming data is ready.

			// IMPORTANT: Check m_pNextAddress, not m_ulSizeLeft! m_ulSizeLeft has already been consumed.
			// What we are interested in now is whether we did send streamed data or not. GetStreamBuffer() 
			// had starved, then m_pNextAddress would also be NULL.

			if ( m_VorbisState.TremorInfo.ReturnInfo.uInputBytesConsumed )
			{
				// Change return code to AK_DataReady to force the pipeline to ask us again.
				io_state.result = AK_DataReady;
			}
			else
			{
				// WG-18965: Decoder is not able to consume what is available, and won't be getting any more.
				// Kill this voice to avoid endless ping-pong between source and pitch nodes.
				MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_VorbisDecodeError, m_pCtx );
				io_state.result = AK_Fail;
			}
		}
	}
	else
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_VorbisDecodeError, m_pCtx );
	}
}

// Override OnLoopComplete() handler: "restart DSP" (set primimg frames) and fix decoder status 
// if it's a loop end.
AKRESULT CAkSrcFileVorbis::OnLoopComplete(
	bool in_bEndOfFile		// True if this was the end of file, false otherwise.
	)
{
	// IMPORTANT: Call base first. VorbisDSPRestart() checks the loop count, so it has to be updated first.
	AKRESULT eResult = CAkSrcFileBase::OnLoopComplete( in_bEndOfFile );
	if ( !in_bEndOfFile )
	{
		VorbisDSPRestart( m_VorbisState.VorbisInfo.LoopInfo.uLoopBeginExtra );

		// Vorbis reported no more data due to end-of-loop; reset the state.
		AKASSERT( m_VorbisState.TremorInfo.ReturnInfo.eDecoderStatus == AK_NoMoreData );
		m_VorbisState.TremorInfo.ReturnInfo.eDecoderStatus = AK_DataReady;
	}
	return eResult;
}

// ReleaseBuffer
void CAkSrcFileVorbis::ReleaseBuffer()
{
	if ( m_VorbisState.TremorInfo.pucData )
    {
		CAkLEngine::ReleaseCachedAudioBuffer( AK::GetNumChannels(m_VorbisState.TremorInfo.uChannelMask) * OUTPUT_CHANNEL_SIZE, m_VorbisState.TremorInfo.pucData );
		m_VorbisState.TremorInfo.ReturnInfo.uFramesProduced = 0;
		m_VorbisState.TremorInfo.pucData = NULL;
    }
}

// StartStream
AKRESULT CAkSrcFileVorbis::StartStream( )
{
	if ( m_bFormatReady )
	{
		// Check streaming status if header has already been parsed.
		return IsInitialPrebufferingReady();
	}
	else if ( m_pStream && m_VorbisState.TremorInfo.ReturnInfo.eDecoderState < PACKET_STREAM )
	{
		// Try process first buffer if stream is already created 
		AKRESULT eResult = ProcessFirstBuffer();
		if ( eResult == AK_Success )
			eResult = IsInitialPrebufferingReady();
		return eResult;
	}

	// Specify buffer constraints. By default, no constraint is necessary. But it is obviously harder
	// for any stream manager implementation to increase the minimum buffer size at run time than to decrease
	// it, so we set it to a "worst case" value.
	// Note: With Vorbis, it sometimes happens that the max packet size is larger than 2K.
	AkAutoStmBufSettings bufSettings;
	bufSettings.uBufferSize		= 0;		// No constraint.
	bufSettings.uMinBufferSize	= AK_WORST_CASE_MIN_STREAM_BUFFER_SIZE;
	bufSettings.uBlockSize		= 0;
	AKRESULT eResult = CreateStream( bufSettings, 0 );
	if ( AK_EXPECT_FALSE( eResult != AK_Success ) )
		return eResult;

	bool bUsePrefetchedData;
	eResult = HandlePrefetch( bUsePrefetchedData );
	if ( eResult == AK_Success )
	{
		// Start IO.
        eResult = m_pStream->Start();
		if ( AK_EXPECT_FALSE( eResult != AK_Success ) )
			return eResult;

		// If the header was not parsed from bank data, attempt to fetch and process the first stream buffer now.
		if ( !bUsePrefetchedData )
		{
			eResult = ProcessFirstBuffer();
			if ( eResult == AK_Success )
				eResult = IsInitialPrebufferingReady();
		}
		else
		{
			// If prefetch is used, the full header and setup information should be in the bank 
			LoopInit();
			eResult = DecodeVorbisHeader();
			AKASSERT( eResult != AK_FormatNotReady );
			if ( eResult != AK_Success )
				return eResult;

			VorbisDSPRestart( 0 );
		
			AKASSERT( !IsPreBuffering() );	// This is normally reset in SeekStream(), which is not used here. If we ever do, set m_bWaitForCompleteBuffering to false.
		}
	}

	return eResult;
}

// StopStream
void CAkSrcFileVorbis::StopStream()
{
	TermVorbisState();

	ReleaseBuffer();

	if ( m_VorbisState.pSeekTable )
	{
		AkFree( g_LEngineDefaultPoolId, m_VorbisState.pSeekTable );
		m_VorbisState.pSeekTable = NULL;
	}

	FreeStitchBuffer();

	if ( m_pOggPacketData )
	{
		AkFree( g_LEngineDefaultPoolId, m_pOggPacketData );
		m_pOggPacketData = NULL;
	}

	CAkSrcFileBase::StopStream();
}

// Returns format-specific throughput. NOT USED.
AkReal32 CAkSrcFileVorbis::GetThroughput( const AkAudioFormat & in_rFormat )
{
	AKASSERT( !"Not implemented" );
	return 1.f;
}

AKRESULT CAkSrcFileVorbis::StopLooping()
{
	// Fix the amount of samples to trim out of the loop end to that of the end of file, IF AND ONLY IF
	// streaming has not looped already. In the latter case, this value is normally fixed when looping
	// is resolved (WG-18602).
	if ( !Stream_IsLoopPending() )
		m_VorbisState.TremorInfo.VorbisDSPState.state.extra_samples_end = m_VorbisState.VorbisInfo.uLastGranuleExtra;

	return CAkSrcFileBase::StopLooping();
}

// VirtualOn
void CAkSrcFileVorbis::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	CAkSrcFileBase::VirtualOn( eBehavior );

	if ( eBehavior == AkVirtualQueueBehavior_FromBeginning || eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
	{
		FreeStitchBuffer();
	}
}

// VirtualOff
AKRESULT CAkSrcFileVorbis::VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset )
{
	AKRESULT eResult = CAkSrcFileBase::VirtualOff( eBehavior, in_bUseSourceOffset );

	AKASSERT( m_VorbisState.TremorInfo.ReturnInfo.eDecoderState >= PACKET_STREAM );
	
	if ( eBehavior == AkVirtualQueueBehavior_FromBeginning || eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
	{
		AkUInt32 uSrcOffsetRemainder = m_pCtx->GetSourceOffsetRemainder();
		m_pCtx->SetSourceOffsetRemainder( 0 );
		m_uCurSample += uSrcOffsetRemainder;

		VorbisDSPRestart((AkUInt16) uSrcOffsetRemainder );
	}

	return eResult;
}

//================================================================================
// Decoding of seek table and 3 Vorbis headers
//================================================================================
AKRESULT CAkSrcFileVorbis::DecodeVorbisHeader( )
{
	// Try to get the setup, comment and codebook headers and set up the Vorbis decoder
	// Any error while decoding header is fatal.
	while( m_VorbisState.TremorInfo.ReturnInfo.eDecoderState < PACKET_STREAM ) 
	{
		// Exit if no data left
		if ( m_ulSizeLeft == 0 )
		{
			return AK_FormatNotReady;
		}

		// Read seek table
		if ( m_VorbisState.TremorInfo.ReturnInfo.eDecoderState < SEEKTABLEINTIALIZED )
		{
			if ( m_VorbisState.uSeekTableSizeRead < m_VorbisState.VorbisInfo.dwSeekTableSize )
			{
				AkUInt32 uCopySize = AkMin( m_ulSizeLeft, m_VorbisState.VorbisInfo.dwSeekTableSize - m_VorbisState.uSeekTableSizeRead );
				
				// Read all seek table items
				// Note: Always copy seek table if it exists.
				AKASSERT( m_VorbisState.pSeekTable != NULL && m_VorbisState.VorbisInfo.dwSeekTableSize > 0 );
				AkMemCpy( (AkUInt8*)m_VorbisState.pSeekTable + m_VorbisState.uSeekTableSizeRead, m_pNextAddress, uCopySize ); 

				m_VorbisState.uSeekTableSizeRead += uCopySize;
				ConsumeData( uCopySize );
			}

			if ( m_VorbisState.uSeekTableSizeRead == m_VorbisState.VorbisInfo.dwSeekTableSize )
				m_VorbisState.TremorInfo.ReturnInfo.eDecoderState = SEEKTABLEINTIALIZED;
		}

		// Read Vorbis header packets
		while ( m_VorbisState.TremorInfo.ReturnInfo.eDecoderState == SEEKTABLEINTIALIZED ) 
		{
			// Get the next packet
			ogg_packet OggPacket;
			AKRESULT eResult = GetNextPacket( OggPacket );
			if ( eResult == AK_NoDataReady ) 
			{
				return AK_FormatNotReady;
			}
			else if ( eResult == AK_Fail || eResult == AK_NoMoreData || eResult == AK_InsufficientMemory )
			{
				return AK_Fail;
			}
			else
			{
				AKASSERT( eResult == AK_DataReady );

				CAkVorbisAllocator * pAllocator = g_VorbisCodebookMgr.Decodebook( m_VorbisState, m_pCtx, &OggPacket );
				if ( !pAllocator )
					return AK_Fail;

				m_VorbisState.TremorInfo.VorbisDSPState.csi = (codec_setup_info *) pAllocator->GetAddress();
				m_VorbisState.TremorInfo.ReturnInfo.eDecoderState = PACKET_STREAM;
			}
		}
	}

	// Only get here once all three headers are parsed, complete Vorbis setup
	AKASSERT( m_VorbisState.TremorInfo.ReturnInfo.eDecoderState == PACKET_STREAM );

	// Initialize global decoder state
	AkInt32 iResult = vorbis_dsp_init(&m_VorbisState.TremorInfo.VorbisDSPState, AK::GetNumChannels(m_VorbisState.TremorInfo.uChannelMask) );
	if ( iResult )
	{
		// DO NOT ASSERT! Can fail because of failed _ogg_malloc(). AKASSERT( !"Failure initializing Vorbis decoder." );
		return AK_Fail;
	}

    return AK_Success;
}

void CAkSrcFileVorbis::InitVorbisState( )
{
	memset(&m_VorbisState, 0, sizeof(AkVorbisSourceState));
}

void CAkSrcFileVorbis::TermVorbisState( )
{
	vorbis_dsp_clear( &m_VorbisState.TremorInfo.VorbisDSPState );
}

AKRESULT CAkSrcFileVorbis::InitVorbisInfo()
{
	// IMPORTANT: Seek table needs to be allocated if it is present, to permit arbitrary seeking of the sound.
	// It is the sole responsibility of the designer to include it or not.
	if ( m_VorbisState.VorbisInfo.dwSeekTableSize )
	{
		// Allocate seek table
		m_VorbisState.pSeekTable = (AkVorbisSeekTableItem *) AkAlloc( g_LEngineDefaultPoolId, m_VorbisState.VorbisInfo.dwSeekTableSize );
		if ( m_VorbisState.pSeekTable == NULL )
			return AK_InsufficientMemory;
	}

	m_VorbisState.TremorInfo.ReturnInfo.eDecoderState = INITIALIZED;

	return AK_Success;
}

//================================================================================
//================================================================================
AKRESULT CAkSrcFileVorbis::ProcessFirstBuffer()
{
	EnterPreBufferingState();
	
	AKASSERT( m_ulSizeLeft == 0 );
	AkUInt8 * pBuffer;
    AKRESULT eResult = m_pStream->GetBuffer(
                            (void*&)pBuffer,	// Address of granted data space.
                            m_ulSizeLeft,		// Size of granted data space.
                            AK_PERF_OFFLINE_RENDERING );

	if ( eResult == AK_NoDataReady )
    {
        // Not ready. Leave.
        return AK_FormatNotReady;
    }
    else if ( eResult != AK_DataReady && eResult != AK_NoMoreData )
    {
        // IO error.
        return AK_Fail;
    }

    if ( m_VorbisState.TremorInfo.ReturnInfo.eDecoderState == UNINITIALIZED )
	{
		// Parse header. 
		eResult = ParseHeader( pBuffer );
		if ( AK_EXPECT_FALSE( eResult != AK_Success ) )
			return eResult;

		AKASSERT( m_uTotalSamples && m_uDataSize );

		LoopInit();

		// Process buffer
		eResult = ProcessStreamBuffer( pBuffer );
		if ( AK_EXPECT_FALSE( eResult != AK_Success ) )
			return eResult;

		// "Consume" header.
		AKASSERT( m_ulSizeLeft >= m_uDataOffset || !"Header must be entirely contained within first stream buffer" );
		ConsumeData( m_uDataOffset );
	}
	else
	{
		// Process buffer
		eResult = ProcessStreamBuffer( pBuffer );
		if ( AK_EXPECT_FALSE( eResult != AK_Success ) )
			return eResult;
	}

	AKASSERT( m_VorbisState.TremorInfo.ReturnInfo.eDecoderState < PACKET_STREAM );

	// Need to setup headers first
	eResult = DecodeVorbisHeader(); 
	if ( eResult != AK_Success )
	{
		if ( eResult == AK_FormatNotReady && m_ulSizeLeft == 0 )
		{
			ReleaseStreamBuffer();
			m_pNextAddress = NULL;
		}
		return eResult;
	}

	AkUInt32 uSrcOffsetRemainder = 0;

	if( m_pCtx->RequiresSourceSeek() )
	{
		eResult = SeekToSourceOffset();

		// Flush streamed input.
		if ( m_ulSizeLeft != 0 )
		{
			ReleaseStreamBuffer();
			m_pNextAddress = NULL;
			m_ulSizeLeft = 0;
		}

		uSrcOffsetRemainder = m_pCtx->GetSourceOffsetRemainder();
		m_pCtx->SetSourceOffsetRemainder( 0 );
		m_uCurSample += uSrcOffsetRemainder;
	}

	VorbisDSPRestart( (AkUInt16) uSrcOffsetRemainder );

	m_bFormatReady = true;
	return eResult;
}
//================================================================================
// ParseHeader
// Parse header information
//================================================================================
AKRESULT CAkSrcFileVorbis::ParseHeader( 
	AkUInt8 * in_pBuffer	// Buffer to parse
	)
{
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

	//Check if the file is of the expected format
	if (fmtInfo.pFormat->wFormatTag != AK_WAVE_FORMAT_VORBIS)
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_FileFormatMismatch, m_pCtx );
		return AK_InvalidFile;
	}

	// Vorbis expects a WaveFormatVorbis
	WaveFormatVorbis * pFmt = (WaveFormatVorbis *)fmtInfo.pFormat;
	AKASSERT( fmtInfo.uFormatSize == sizeof( WaveFormatVorbis ) );

	//Setup format on the PBI
	AkAudioFormat format;
	format.SetAll(
		pFmt->nSamplesPerSec, 
		pFmt->dwChannelMask, 
		16, 
		pFmt->nChannels * sizeof(AkInt16),
		AK_INT,							
		AK_INTERLEAVED);					

	m_pCtx->SetMediaFormat(format);

	// Store analysis data if it was present. 
	// Note: StoreAnalysisData may fail, but it is not critical.
	if ( analysisDataChunk.uDataSize > 0 )
		StoreAnalysisData( analysisDataChunk );

	m_uTotalSamples = pFmt->vorbisHeader.dwTotalPCMFrames;
	
	m_VorbisState.VorbisInfo = pFmt->vorbisHeader;

	m_VorbisState.TremorInfo.uChannelMask = pFmt->dwChannelMask;
	m_VorbisState.uSampleRate = pFmt->nSamplesPerSec;

	// Fix loop start and loop end for no SMPL chunk
	if((m_uPCMLoopStart == 0) && (m_uPCMLoopEnd == 0))
	{	
		m_uPCMLoopEnd = m_uTotalSamples-1;
	}

	// Loop points. If not looping or ulLoopEnd is 0 (no loop points),
    // set loop points to data chunk boundaries.
    if ( DoLoop() )
    {   
		// NOTE: Disregard loop points contained in Fmt chunk and use VorbisInfo instead
		m_ulLoopStart = m_VorbisState.VorbisInfo.LoopInfo.dwLoopStartPacketOffset + m_uDataOffset + m_VorbisState.VorbisInfo.dwSeekTableSize;
		m_ulLoopEnd = m_VorbisState.VorbisInfo.LoopInfo.dwLoopEndPacketOffset + m_uDataOffset + m_VorbisState.VorbisInfo.dwSeekTableSize;
    }
	else
	{
		m_ulLoopStart = m_uDataOffset + m_VorbisState.VorbisInfo.dwVorbisDataOffset; // VorbisDataOffset == seek table + vorbis header
		m_ulLoopEnd = m_uDataOffset + m_uDataSize;
	}

	AKASSERT( m_pStream );
	// Update stream heuristics.
	AkAutoStmHeuristics heuristics;
    m_pStream->GetHeuristics( heuristics );
	GetStreamLoopHeuristic( DoLoop(), heuristics );
	heuristics.fThroughput = pFmt->nAvgBytesPerSec / 1000.f;	// Throughput (bytes/ms)
    heuristics.priority = m_pCtx->GetPriority();	// Priority.
    m_pStream->SetHeuristics( heuristics );

	eResult = InitVorbisInfo();
	if ( eResult != AK_Success )
		return eResult;

	// Update stream buffering settings.
	// Cannot receive less than the max packet size.
	return m_pStream->SetMinimalBufferSize( 2 * m_VorbisState.VorbisInfo.uMaxPacketSize );
}

// Finds the closest offset in file that corresponds to the desired sample position.
// Returns the file offset (in bytes, relative to the beginning of the file) and its associated sample position.
// Returns AK_Fail if the codec is unable to seek.
AKRESULT CAkSrcFileVorbis::FindClosestFileOffset( 
	AkUInt32 in_uDesiredSample,		// Desired sample position in file.
	AkUInt32 & out_uSeekedSample,	// Returned sample where file position was set.
	AkUInt32 & out_uFileOffset		// Returned file offset where file position was set.
	)
{
	AKRESULT eResult = VorbisSeek( m_VorbisState, in_uDesiredSample, out_uSeekedSample, out_uFileOffset );
	if ( eResult == AK_Fail )
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_VorbisRequireSeekTable, m_pCtx );
		return AK_Fail;
	}

	out_uFileOffset += m_uDataOffset;

	return AK_Success;
}

void CAkSrcFileVorbis::FreeStitchBuffer()
{
	if ( m_pStitchStreamBuffer )
	{
		AkFree( g_LEngineDefaultPoolId, m_pStitchStreamBuffer );
		m_pStitchStreamBuffer = NULL;
		m_uStitchBufferEndOffset = 0;
		m_uStitchBufferLeft = 0;
		m_uStitchBufferValidDataSize = 0;
	}
}

// GetNextPacket
// Tries to gather complete packets from stream buffer
AKRESULT CAkSrcFileVorbis::GetNextPacket( ogg_packet & out_OggPacket )
{
	while ( true )
	{
		AKASSERT( m_pStream != NULL );

		if ( m_ulSizeLeft == 0 )
		{
			// If we don't have data unconsumed and we will never get any more we are done decoding
			if ( HasNoMoreStreamData() )
				return AK_NoMoreData;

			// See if we can get more data from stream manager.
			ReleaseStreamBuffer();
			AKRESULT eStmResult = FetchStreamBuffer();
			if ( eStmResult != AK_DataReady )
				return eStmResult;
		}

		bool bHeaderReady = false;
		// Gather header
		if ( m_uPacketHeaderGathered < sizeof(OggPacketHeader) )
		{
			if ( m_ulSizeLeft > 0 )
			{
				// Accumulate data into packet header
				AkUInt32 uCopySize = AkMin( m_ulSizeLeft, sizeof(OggPacketHeader)-m_uPacketHeaderGathered );
				AkUInt8 * uHeaderPtr = (AkUInt8 *) &m_OggPacketHeader;
				AkMemCpy( uHeaderPtr+m_uPacketHeaderGathered, m_pNextAddress, uCopySize );
				m_uPacketHeaderGathered += uCopySize;
				ConsumeData( uCopySize );
				if ( m_uPacketHeaderGathered == sizeof(OggPacketHeader) )
				{
					bHeaderReady = true;
				}
			}
		}
		else
		{
			bHeaderReady = true;
		}

		// Gather packet data
		if ( bHeaderReady )
		{
			if ( m_uPacketDataGathered == 0 )
			{
				// Just finished packet header, allocate space for accumulating packet data
				if ( m_pOggPacketData )
				{
					AkFree( g_LEngineDefaultPoolId, m_pOggPacketData );
					m_pOggPacketData = NULL;
				}
				m_pOggPacketData = (AkUInt8*) AkAlloc( g_LEngineDefaultPoolId, m_OggPacketHeader.uPacketSize );
				if ( m_pOggPacketData == NULL )
				{
					return AK_InsufficientMemory;
				}
			}

			AKASSERT( m_uPacketDataGathered <= m_OggPacketHeader.uPacketSize );
			if ( m_uPacketDataGathered < m_OggPacketHeader.uPacketSize && m_ulSizeLeft > 0 )
			{
				// Accumulate data into packet
				AkUInt32 uCopySize = AkMin( m_ulSizeLeft, m_OggPacketHeader.uPacketSize-m_uPacketDataGathered );
				AkMemCpy( m_pOggPacketData+m_uPacketDataGathered, m_pNextAddress, uCopySize );
				m_uPacketDataGathered += uCopySize;
				ConsumeData( uCopySize );
			}
		}

		if ( m_uPacketHeaderGathered == sizeof(OggPacketHeader) && m_uPacketDataGathered == m_OggPacketHeader.uPacketSize )
		{
			// Build packet
			UnpackPacket((AkUInt8*)&m_OggPacketHeader,m_pOggPacketData,false,out_OggPacket);

			// Prepare for next GetNextPacket() 
			m_uPacketHeaderGathered = 0;
			m_uPacketDataGathered = 0;
			return AK_DataReady;	// Packet ready
		}
		// Not a full packet yet, keep gathering
	}
}

void CAkSrcFileVorbis::LoopInit()
{
	m_uCurSample = 0;

	// Init state.
	ResetLoopCnt();
}

AKRESULT CAkSrcFileVorbis::ChangeSourcePosition()
{
	AKRESULT eResult = CAkSrcFileBase::ChangeSourcePosition();

	if ( eResult == AK_Success )
	{	
		FreeStitchBuffer();

		AkUInt32 uSrcOffsetRemainder = m_pCtx->GetSourceOffsetRemainder();
		m_pCtx->SetSourceOffsetRemainder( 0 );
		m_uCurSample += uSrcOffsetRemainder;
			
		VorbisDSPRestart( (AkUInt16) uSrcOffsetRemainder );
	}

	return eResult;
}
