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
#include "AkSrcBankVorbis.h"
#include "AkLEngine.h"
#include "AkMonitor.h"
#include "AkVorbisCodebookMgr.h"

#define OUTPUT_CHANNEL_SIZE	(LE_MAX_FRAMES_PER_BUFFER * sizeof(AkInt16))

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
IAkSoftwareCodec* CreateVorbisBankPlugin( void* in_pCtx )
{
	return AkNew( g_LEngineDefaultPoolId, CAkSrcBankVorbis( (CAkPBI*)in_pCtx ) );
}

// Constructor
CAkSrcBankVorbis::CAkSrcBankVorbis( CAkPBI * in_pCtx ) 
: CAkSrcBaseEx( in_pCtx )
, m_pucData( NULL )		// current data pointer
, m_pucDataStart( NULL )// start of audio data
{
	// do this here as well as it is legal to be StopStream'ed
	// without having been StartStream'ed
	InitVorbisState();
}

// Destructor
CAkSrcBankVorbis::~CAkSrcBankVorbis()
{
	AKASSERT( !m_VorbisState.TremorInfo.pucData );
	if ( m_VorbisState.TremorInfo.VorbisDSPState.csi )
		g_VorbisCodebookMgr.ReleaseCodebook( m_VorbisState );
}

// GetBuffer
void CAkSrcBankVorbis::GetBuffer( AkVPLState & io_state )
{
	// The stream should not start before all headers are decoded
	AKASSERT( m_VorbisState.TremorInfo.ReturnInfo.eDecoderState >= PACKET_STREAM );

	// Allocate output buffer for decoded data
	if ( m_VorbisState.TremorInfo.pucData == NULL )
	{
		m_VorbisState.TremorInfo.pucData = (AkUInt8 *) CAkLEngine::GetCachedAudioBuffer( m_VorbisState.TremorInfo.VorbisDSPState.channels * OUTPUT_CHANNEL_SIZE);
		m_VorbisState.TremorInfo.ReturnInfo.uFramesProduced = 0;
		if ( m_VorbisState.TremorInfo.pucData == NULL )
		{
			io_state.result = AK_Fail;
			return;
		}
	}

	// Note: No need to adjust request to for file end or looping as it will finish the decoding a packet anyhow
	m_VorbisState.TremorInfo.uRequestedFrames = io_state.MaxFrames();

	// Trim input data to loop end if necessary
	m_VorbisState.TremorInfo.uInputDataSize =  GetMaxInputDataSize();
	m_VorbisState.TremorInfo.bNoMoreInputPackets = true;

	DecodeVorbis( &m_VorbisState.TremorInfo, m_VorbisState.VorbisInfo.uMaxPacketSize, m_pucData, (AkInt16*)m_VorbisState.TremorInfo.pucData );
	
	io_state.result = m_VorbisState.TremorInfo.ReturnInfo.eDecoderStatus;

	if ( AK_EXPECT_FALSE( io_state.result == AK_Fail ) )
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_VorbisDecodeError, m_pCtx );
		return;
	}

	// Advance m_pucData based on how many packets were consumed
	m_pucData += m_VorbisState.TremorInfo.ReturnInfo.uInputBytesConsumed;

	// Prepare buffer for VPL, update PCM position, handle end of loop/file, post markers and position info.
	SubmitBufferAndUpdate( 
		m_VorbisState.TremorInfo.pucData, 
		(AkUInt16)m_VorbisState.TremorInfo.ReturnInfo.uFramesProduced, 
		m_VorbisState.uSampleRate,
		m_VorbisState.TremorInfo.uChannelMask, 
		io_state );
}

// ReleaseBuffer
void CAkSrcBankVorbis::ReleaseBuffer()
{
	if ( m_VorbisState.TremorInfo.pucData )
    {
		CAkLEngine::ReleaseCachedAudioBuffer( AK::GetNumChannels(m_VorbisState.TremorInfo.uChannelMask) * OUTPUT_CHANNEL_SIZE, m_VorbisState.TremorInfo.pucData );
		m_VorbisState.TremorInfo.ReturnInfo.uFramesProduced = 0;
		m_VorbisState.TremorInfo.pucData = NULL;
    }
}

// StartStream
AKRESULT CAkSrcBankVorbis::StartStream()
{
    AKASSERT( m_markers.m_hdrMarkers.uNumMarkers == 0 && m_markers.m_pMarkers == NULL ); 

    AkUInt8 * pvBuffer;
    AkUInt32 ulBufferSize;
	m_pCtx->GetDataPtr( pvBuffer, ulBufferSize );
	if ( pvBuffer == NULL )
	{
		return AK_Fail;
	}

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
											&analysisDataChunk,	// Analysis info.
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

	if ( analysisDataChunk.uDataSize > 0 )
		m_pAnalysisData = analysisDataChunk.pData;

	m_pucDataStart = pvBuffer + m_uDataOffset;

	m_uTotalSamples = pFmt->vorbisHeader.dwTotalPCMFrames;
	
	m_VorbisState.VorbisInfo = pFmt->vorbisHeader;

	m_VorbisState.uSampleRate = pFmt->nSamplesPerSec;
	m_VorbisState.TremorInfo.uChannelMask = pFmt->dwChannelMask;

	// Fix loop start and loop end for no SMPL chunk
	if ( m_uPCMLoopEnd == 0 )
	{	
		m_uPCMLoopEnd = m_uTotalSamples-1;
	}

	// Verify data buffer consistency.
	if ( m_uPCMLoopEnd < m_uPCMLoopStart 
		|| m_uPCMLoopEnd >= m_uTotalSamples
		|| ulBufferSize != (m_uDataOffset + m_uDataSize) )
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader, m_pCtx );
		return AK_Fail;
	}

	m_pucData = m_pucDataStart;
	LoopInit();

	eResult = DecodeVorbisHeader();

	if( eResult == AK_Success )
	{
		AkUInt32 uSrcOffsetRemainder = 0;

		if( m_pCtx->RequiresSourceSeek() )
		{
			eResult = SeekToNativeOffset();

			uSrcOffsetRemainder = m_pCtx->GetSourceOffsetRemainder();
			m_pCtx->SetSourceOffsetRemainder( 0 );
			m_uCurSample += uSrcOffsetRemainder;
		}

		VorbisDSPRestart((AkUInt16) uSrcOffsetRemainder );
	}

	return eResult;
}

bool CAkSrcBankVorbis::SupportMediaRelocation() const
{
	return true;
}

AKRESULT CAkSrcBankVorbis::RelocateMedia( AkUInt8* in_pNewMedia, AkUInt8* in_pOldMedia )
{
	AkUIntPtr uMemoryOffset = (AkUIntPtr)in_pNewMedia - (AkUIntPtr)in_pOldMedia;
	m_pucData = m_pucData + uMemoryOffset;
	m_pucDataStart = m_pucDataStart + uMemoryOffset;

	return AK_Success;
}

// StopStream
void CAkSrcBankVorbis::StopStream()
{
	TermVorbisState();

	ReleaseBuffer();
	if ( m_VorbisState.pSeekTable )
	{
		AkFree( g_LEngineDefaultPoolId, m_VorbisState.pSeekTable );
		m_VorbisState.pSeekTable = NULL;
	}

	CAkSrcBaseEx::StopStream();
}

AKRESULT CAkSrcBankVorbis::StopLooping()
{
	m_VorbisState.TremorInfo.VorbisDSPState.state.extra_samples_end = m_VorbisState.VorbisInfo.uLastGranuleExtra;
	return CAkSrcBaseEx::StopLooping();
}

// VirtualOff
AKRESULT CAkSrcBankVorbis::VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset )
{
	AKRESULT eResult = AK_Success;
	AkUInt32 uSrcOffsetRemainder = 0;

	AKASSERT( m_VorbisState.TremorInfo.ReturnInfo.eDecoderState >= PACKET_STREAM );

	if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
	{
		if ( !in_bUseSourceOffset )
		{
			eResult = VirtualSeek( m_uCurSample );	// Use virtual pointer.
			if( eResult != AK_Success )
			{
				// We failed to seek, most chances are we failed because we had no seek table
				// Default seeking at the beginning.
				m_uCurSample = 0;
				eResult = VirtualSeek( m_uCurSample );	// Use virtual pointer.
			}
		}
		else
		{
			eResult = SeekToNativeOffset();	// Use source offset specified by behavioral engine.
		}

		uSrcOffsetRemainder = m_pCtx->GetSourceOffsetRemainder();
		m_pCtx->SetSourceOffsetRemainder( 0 );
		m_uCurSample += uSrcOffsetRemainder;
	}
	else if ( eBehavior == AkVirtualQueueBehavior_FromBeginning ) 
	{
		// Setup completed go to data directly
		m_pucData = m_pucDataStart + m_VorbisState.VorbisInfo.dwVorbisDataOffset;
		LoopInit();
	}
	else
	{
		// Nothing to do for resume mode
		return AK_Success;
	}

	VorbisDSPRestart( (AkUInt16) uSrcOffsetRemainder );

	return eResult;
}

// VirtualSeek
// Determine where to seek to using seek table
AKRESULT CAkSrcBankVorbis::VirtualSeek( AkUInt32 & io_uSeekPosition )
{
	AkUInt32 uFileOffset;
	AKRESULT eResult = VorbisSeek( m_VorbisState, io_uSeekPosition, io_uSeekPosition, uFileOffset );
	if ( eResult != AK_Success )
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_VorbisRequireSeekTable, m_pCtx );
		return AK_Fail;
	}

	m_pucData = m_pucDataStart + uFileOffset;

	return AK_Success;
}

//================================================================================
// Decoding of seek table and 3 Vorbis headers
//================================================================================
AKRESULT CAkSrcBankVorbis::DecodeVorbisHeader()
{
	AKASSERT( m_VorbisState.TremorInfo.ReturnInfo.eDecoderState == UNINITIALIZED ); 

	AKRESULT eResult = InitVorbisInfo();
	if( eResult != AK_Success )
	{
		return eResult;
	}

	// Note: Always copy seek table if it exists.
	// Read all seek table items
	if ( m_VorbisState.VorbisInfo.dwSeekTableSize > 0 )
	{
		AKASSERT( m_VorbisState.pSeekTable != NULL );	
		AkMemCpy( m_VorbisState.pSeekTable, m_pucData, m_VorbisState.VorbisInfo.dwSeekTableSize ); 	
		m_pucData += m_VorbisState.VorbisInfo.dwSeekTableSize;
	}

	// Read Vorbis header packets

	// Get the next packet
	ogg_packet Packet;
	UnpackPacket(m_pucData,m_pucData+sizeof(OggPacketHeader),false,Packet);
	m_pucData += sizeof(OggPacketHeader) + Packet.buffer.size;
	
	// Synthesize Vorbis header
	CAkVorbisAllocator * pAllocator = g_VorbisCodebookMgr.Decodebook( m_VorbisState, m_pCtx, &Packet );
	if ( !pAllocator )
		return AK_Fail;

	m_VorbisState.TremorInfo.VorbisDSPState.csi = (codec_setup_info *) pAllocator->GetAddress();

	// Only get here once codebook header is parsed, complete Vorbis setup
	// Initialize global decoder state
	AkInt32 iResult = vorbis_dsp_init( &(m_VorbisState.TremorInfo.VorbisDSPState), AK::GetNumChannels(m_VorbisState.TremorInfo.uChannelMask) );
	if ( iResult )
	{
		// DO NOT ASSERT! Can fail because of failed _ogg_malloc(). AKASSERT( !"Failure initializing Vorbis decoder." );
		return AK_Fail;
	}
	
	m_VorbisState.TremorInfo.ReturnInfo.eDecoderState = PACKET_STREAM;

	return AK_Success;
}

void CAkSrcBankVorbis::InitVorbisState( )
{
	memset(&m_VorbisState, 0, sizeof(AkVorbisSourceState));
}

void CAkSrcBankVorbis::TermVorbisState( )
{
	vorbis_dsp_clear( &m_VorbisState.TremorInfo.VorbisDSPState );
}

AKRESULT CAkSrcBankVorbis::InitVorbisInfo()
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

void CAkSrcBankVorbis::LoopInit()
{
	m_uCurSample = 0;

	// Init state.
	ResetLoopCnt();
}

AkUInt32 CAkSrcBankVorbis::GetMaxInputDataSize()
{
	if ( !DoLoop() )
	{
		return ( m_pucDataStart + m_uDataSize - m_pucData );
	}
	else
	{
		// Seek table is inserted in front of encoded data so we need to offset by SeekTable size
		return ( m_pucDataStart + m_VorbisState.VorbisInfo.LoopInfo.dwLoopEndPacketOffset + m_VorbisState.VorbisInfo.dwSeekTableSize - m_pucData );
	}
}

// Helper: Seek to source offset obtained from PBI (number of samples at the pipeline's sample rate).
AKRESULT CAkSrcBankVorbis::SeekToNativeOffset()
{
	if ( !m_VorbisState.pSeekTable )
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_VorbisRequireSeekTable, m_pCtx );
		return AK_Fail;
	}
	AKASSERT( m_VorbisState.TremorInfo.ReturnInfo.eDecoderState >= PACKET_STREAM );

	// Note: Force using 64 bit calculation to avoid uint32 overflow.
	AkUInt64 uSourceOffset = GetSourceOffset();

	if ( uSourceOffset >= m_uTotalSamples )
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_SeekAfterEof, m_pCtx );
		return AK_Fail;
	}

	AkUInt32 uRealOffset = (AkUInt32)uSourceOffset;
	if ( VirtualSeek( uRealOffset ) != AK_Success )
		return AK_Fail;

	m_uCurSample = uRealOffset;
	
	// Store error in PBI.	
	AKASSERT( (int)uSourceOffset - (int)uRealOffset >= 0 );
	m_pCtx->SetSourceOffsetRemainder( (AkUInt32)uSourceOffset - uRealOffset );

	return AK_Success;
}

AKRESULT CAkSrcBankVorbis::ChangeSourcePosition()
{
	// Compute new file offset and seek stream.
	// Note: The seek time is stored in the PBI's source offset. However it represents the absolute 
	// time from the beginning of the sound, taking the loop region into account.

	AKRESULT eResult = SeekToNativeOffset();

	AkUInt32 uSrcOffsetRemainder = m_pCtx->GetSourceOffsetRemainder();
	m_pCtx->SetSourceOffsetRemainder( 0 );
	m_uCurSample += uSrcOffsetRemainder;

	VorbisDSPRestart( (AkUInt16) uSrcOffsetRemainder );

	return eResult;
}

// Override OnLoopComplete() handler: "restart DSP" (set primimg frames) and fix decoder status 
// if it's a loop end, and place input pointer.
AKRESULT CAkSrcBankVorbis::OnLoopComplete(
	bool in_bEndOfFile		// True if this was the end of file, false otherwise.
	)
{
	// IMPORTANT: Call base first. VorbisDSPRestart() checks the loop count, so it has to be updated first.
	AKRESULT eResult = CAkSrcBaseEx::OnLoopComplete( in_bEndOfFile );
	if ( !in_bEndOfFile )
	{
		m_pucData = m_pucDataStart + m_VorbisState.VorbisInfo.LoopInfo.dwLoopStartPacketOffset + m_VorbisState.VorbisInfo.dwSeekTableSize;
		
		VorbisDSPRestart( m_VorbisState.VorbisInfo.LoopInfo.uLoopBeginExtra );

		// Vorbis reported no more data due to end-of-loop; reset the state.
		AKASSERT( m_VorbisState.TremorInfo.ReturnInfo.eDecoderStatus == AK_NoMoreData );
		m_VorbisState.TremorInfo.ReturnInfo.eDecoderStatus = AK_DataReady;
	}
	return eResult;
}
