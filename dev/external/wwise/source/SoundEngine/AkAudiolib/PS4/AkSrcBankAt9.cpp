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
#include "AkSrcBankAt9.h"
#include "AkSrcBase.h"
#include "AkCommon.h"
#include "AkATRAC9.h"
#include <ajm/at9_decoder.h>

#include "AkSrcBase.h"

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
IAkSoftwareCodec* CreateATRAC9BankPlugin( void* in_pCtx )
{
	return AkNew( g_LEngineDefaultPoolId, CAkSrcBankAt9( (CAkPBI*)in_pCtx ) );
}

//-----------------------------------------------------------------------------
// Name: CAkSrcBankAt9
// Desc: Constructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSrcBankAt9::CAkSrcBankAt9( CAkPBI * in_pCtx )
: CAkSrcACPBase( in_pCtx )
, m_pSourceFileStartPosition( NULL )
{
}

CAkSrcBankAt9::~CAkSrcBankAt9()
{
}

AKRESULT CAkSrcBankAt9::StartStream()
{	
	AKRESULT result = AK_Success;
	AkUInt8 * pvBuffer;
    AkUInt32 ulBufferSize;
	m_pCtx->GetDataPtr( pvBuffer, ulBufferSize );
    if ( pvBuffer == NULL )
		return AK_Fail;

	AkFileParser::FormatInfo fmtInfo;
	AkFileParser::SeekInfo seekInfo;
	AkFileParser::AnalysisDataChunk analysisDataChunk;
	AKRESULT eResult = AkFileParser::Parse( pvBuffer,			// Data buffer
											ulBufferSize,		// Buffer size
											fmtInfo,			// Returned audio format info.
											&m_markers,			// Markers.
											&m_uPCMLoopStart,	// Beginning of loop.
											&m_uPCMLoopEnd,		// End of loop (inclusive).
											&m_uDataSize,		// Data size.
											&m_uDataOffset,		// Offset to data.
											&analysisDataChunk,	// Analysis data.
											&seekInfo );		// Format specific seek table.
	
	if ( eResult != AK_Success )
    {
        MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader, m_pCtx );
		return AK_InvalidFile;
    }

	if (fmtInfo.pFormat->wFormatTag != AK_WAVE_FORMAT_AT9)
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_FileFormatMismatch, m_pCtx );
		return AK_InvalidFile;
	}

	AKASSERT( fmtInfo.pFormat );
	AKASSERT( fmtInfo.uFormatSize == sizeof( AkATRAC9WaveFormatExtensible ) );
	AKASSERT( fmtInfo.pFormat->cbSize == ( sizeof( AkATRAC9WaveFormatExtensible ) - sizeof( WaveFormatEx ) ) );

	AkATRAC9WaveFormatExtensible* pFormat = (AkATRAC9WaveFormatExtensible*)fmtInfo.pFormat;

	//
	// Keep the At9 specifc configData we'll need later.
	//
	memcpy( &m_At9ConfigData, pFormat->atrac9_configData, sizeof( m_At9ConfigData ) );
	AkUInt32 uPCMBlockAlign = AK_ATRAC9_OUTPUT_BYTES_PER_SAMPLE * pFormat->nChannels;
	AKASSERT(pFormat->atrac9_delaySamplesInputOverlapEncoder == pFormat->wSamplesPerBlock);
	
	//
	// Set format on our context
	//

	AkAudioFormat format;
	format.SetAll(
		pFormat->nSamplesPerSec, 
		pFormat->dwChannelMask, 
		AK_ATRAC9_OUTPUT_BITS_PER_SAMPLE, 
		uPCMBlockAlign,
		AK_INT,
		AK_INTERLEAVED);

	m_pCtx->SetMediaFormat(format);

	if ( analysisDataChunk.uDataSize > 0 )
		m_pAnalysisData = analysisDataChunk.pData;

	// AKASSERT( IsValidFormat( *pFmt ) );

	// setting block sizes for bytes to samples calculations in the parent class.
	m_nSamplesPerBlock = pFormat->wSamplesPerBlock;
	m_nBlockSize = pFormat->nBlockAlign;

	m_DecoderSamplePosition = -m_nSamplesPerBlock;
	m_pSourceFileStartPosition = pvBuffer + m_uDataOffset; // Start of the first sample, we'll need to roll back before decoding.

	m_uCurSample = 0;
	
	AkUInt32 uNumChannels = GetNumChannels( pFormat->dwChannelMask );
	m_uTotalSamples = pFormat->atrac9_totalSamplesPerChannel;

	FixLoopPoints();

	if ( m_pCtx->RequiresSourceSeek() )
	{
		result = SeekToSourceOffset();
	}

	PrepareRingbuffer();

	if ( !Register() )
	{
		return AK_InsufficientMemory;
	}
	
	return result;
}

AKRESULT CAkSrcBankAt9::RelocateMedia( AkUInt8* in_pNewMedia, AkUInt8* in_pOldMedia )
{
	AkUIntPtr uMemoryOffset = (AkUIntPtr)in_pNewMedia - (AkUIntPtr)in_pOldMedia;

	m_pSourceFileStartPosition = m_pSourceFileStartPosition + uMemoryOffset;

	return AK_Success;
}

bool CAkSrcBankAt9::Register()
{
	if ( !CAkSrcACPBase<CAkSrcBaseEx>::Register() ||
		CAkLEngine::GetACPManager()->Register(this) != AK_Success)
	{
		return false;
	}

	return true;
}

void CAkSrcBankAt9::Unregister()
{
	CAkLEngine::GetACPManager()->Unregister(this);
	CAkSrcACPBase<CAkSrcBaseEx>::Unregister();
}

int CAkSrcBankAt9::SetDecodingInputBuffers(AkInt32 in_uSampleStart, AkInt32 in_uSampleEnd, AkUInt16 in_uBufferIndex, AkUInt32& out_remainingSize)
{
	AKASSERT(in_uSampleStart % m_nSamplesPerBlock == 0);
	AKASSERT(in_uSampleEnd % m_nSamplesPerBlock == 0);
	AKASSERT((in_uSampleEnd-in_uSampleStart) % m_nSamplesPerBlock == 0);

	AkUInt32 uNumChannels = GetNumChannels( this->m_pCtx->GetMediaFormat().uChannelMask );

	AkUInt32 uBytesStart = SampleToByteRelative( in_uSampleStart );
	size_t uBytesLength = SampleToByte( in_uSampleEnd - in_uSampleStart );
	
	m_InputBufferDesc[in_uBufferIndex].pAddress = m_pSourceFileStartPosition + uBytesStart;
	m_InputBufferDesc[in_uBufferIndex].szSize = uBytesLength;

	AKASSERT( m_InputBufferDesc[in_uBufferIndex].szSize % m_nBlockSize == 0);
	AKASSERT( m_InputBufferDesc[in_uBufferIndex].szSize <= SCE_AJM_DEC_AT9_MAX_INPUT_BUFFER_SIZE );

#ifdef AT9_STREAM_DEBUG_OUTPUT
		char msg[256];
		sprintf( msg, "-- -- -- -- SetDecodingInputBuffers target %i [%i,%i] %p, %i \n", in_uBufferIndex, in_uSampleStart, in_uSampleEnd, m_InputBufferDesc[in_uBufferIndex].pAddress, m_InputBufferDesc[in_uBufferIndex].szSize);
		AKPLATFORM::OutputDebugMsg( msg );
#endif
	out_remainingSize = 0;
	return in_uBufferIndex+1;
}

AkUInt32 CAkSrcBankAt9::DecodingEnded()
{
	if (m_InitResult.sResult.iResult < 0 || m_DecodeResult.sResult.iResult < 0 )
	{ 
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_ATRAC9DecodeFailed, m_pCtx);
		m_DecodingFailed = true;
		m_DecodingStarted = false;
		return 0;
	}

	//AKASSERT( m_OutputBufferDesc[0].szSize + m_OutputBufferDesc[1].szSize == m_DecodeResult.sStream.iSizeProduced );
	//AKASSERT( m_InputBufferDesc[0].szSize + m_InputBufferDesc[1].szSize == (m_DecodeResult.sStream.iSizeConsumed) );
	m_DecodingStarted = false;

	return m_InputBufferDesc[0].szSize + m_InputBufferDesc[1].szSize + m_InputBufferDesc[2].szSize;
}

void CAkSrcBankAt9::FixLoopPoints()
{
	if (this->m_uPCMLoopEnd == 0)
	{
		m_uPCMLoopEnd = m_uTotalSamples-1;
	}
}

void CAkSrcBankAt9::VirtualOn( AkVirtualQueueBehavior eBehavior )
{	
	m_IsVirtual = true;
	if ( eBehavior != AkVirtualQueueBehavior_Resume )
	{
		Unregister();
	}

	CAkSrcBaseEx::VirtualOn(eBehavior);
}


AKRESULT CAkSrcBankAt9::VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset )
{
	AKRESULT result = AK_Success;
	m_IsVirtual = false;
	if ( eBehavior != AkVirtualQueueBehavior_Resume )
	{
		if( !Register() )
		{
			return AK_Fail;
		}

		if ( eBehavior == AkVirtualQueueBehavior_FromBeginning )
		{
			m_uCurSample = 0;
			ResetLoopCnt();
		}
		else if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
		{		
			if ( in_bUseSourceOffset )
			{
				result = SeekToSourceOffset();
			}
			else
			{
				if ( m_uCurSample >= m_uTotalSamples )
				{
					MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_SeekAfterEof, m_pCtx );
					result = AK_Fail;
				}
			}
			
		}

		PrepareRingbuffer();
	}

	if (result != AK_Success)
	{
		return result;
	}
		
	return CAkSrcACPBase<CAkSrcBaseEx>::VirtualOff( eBehavior, in_bUseSourceOffset );;
}