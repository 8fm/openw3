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
#include "AkSrcFileAt9.h"
#include "AkSrcBase.h"
#include "AkCommon.h"
#include "AkATRAC9.h"
#include <ajm/at9_decoder.h>

#include "AkSrcBase.h"

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
IAkSoftwareCodec* CreateATRAC9FilePlugin( void* in_pCtx )
{
	return AkNew( g_LEngineDefaultPoolId, CAkSrcFileAt9( (CAkPBI*)in_pCtx ) );
}

//-----------------------------------------------------------------------------
// Name: CAkSrcBankAt9
// Desc: Constructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSrcFileAt9::CAkSrcFileAt9( CAkPBI * in_pCtx )
: CAkSrcACPBase( in_pCtx )
, m_uBufferIndex( 0 )
, m_ulFileOffsetPrevious( 0 )
, m_bWaitingOnNextBuffer( false )
{
}

CAkSrcFileAt9::~CAkSrcFileAt9()
{
}

AKRESULT CAkSrcFileAt9::ParseHeader( AkUInt8 * in_pBuffer )
{
	AkFileParser::FormatInfo fmtInfo;
	AkFileParser::SeekInfo seekInfo;
	AkFileParser::AnalysisDataChunk analysisDataChunk;
	AKRESULT eResult = AkFileParser::Parse( in_pBuffer,			// Data buffer
											m_ulSizeLeft,		// Buffer size
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

	m_uCurSample = 0;
	
	m_uTotalSamples = pFormat->atrac9_totalSamplesPerChannel;
	m_pNextAddress = in_pBuffer + m_uDataOffset;

	FixLoopPoints();

	AKRESULT result = AK_Success;
	bool bSeekedAtStart = false;
	if ( m_pCtx->RequiresSourceSeek() )
	{
		bSeekedAtStart = true;
	}

	if ( !Register() )
	{
		return AK_InsufficientMemory;
	}

	m_uSampleSkipOnFileSeekStart = m_nSamplesPerBlock;
	PrepareRingbuffer();
	
	// Update stream heuristics.
	AkAutoStmHeuristics heuristics;
    m_pStream->GetHeuristics( heuristics );
    heuristics.fThroughput = (AkReal32) ( pFormat->nBlockAlign * pFormat->nSamplesPerSec / m_nSamplesPerBlock ) / 1000.f;
	heuristics.priority = m_pCtx->GetPriority();
	heuristics.uMinNumBuffers = 2;
    if ( DoLoop() )
    {
        heuristics.uLoopStart = m_ulLoopStart;
        heuristics.uLoopEnd = m_ulLoopEnd;
    }
    m_pStream->SetHeuristics( heuristics );

	// Update stream buffering settings.
	// Cannot receive less than an whole sample frame.
	// m_pStream->SetMinimalBufferSize( 256 );

#ifdef AT9_STREAM_DEBUG_OUTPUT
	char msg[256];
	sprintf( msg, "-- -- -- -- [%p] START START START START \n", this );
	AKPLATFORM::OutputDebugMsg( msg );
#endif

	sptr = NULL;
	
	// Buffers needs to be m_nBlockSize alligned, remainign bytes will go into the stiching buffer
	AkUInt32 ulTruncatedSizeBytes = (m_ulSizeLeft-m_uDataOffset) / m_nBlockSize * m_nBlockSize;

	if (!bSeekedAtStart && ulTruncatedSizeBytes != 0)
	{
		AkUInt32 uNextDataOffset = (m_ulSizeLeft-m_uDataOffset);
		AkUInt32 ulEndLimit = Stream_Doloop() ? m_ulLoopEnd : m_uDataOffset + m_uDataSize;
		AkUInt32 uStitchBufferFillSize = uNextDataOffset - ulTruncatedSizeBytes;
		if ( uNextDataOffset >= ulEndLimit )
		{
			uNextDataOffset = ulEndLimit-m_uDataOffset;
			ulTruncatedSizeBytes = uNextDataOffset / m_nBlockSize * m_nBlockSize;
			uStitchBufferFillSize = uNextDataOffset - ulTruncatedSizeBytes;
		//	if (Stream_Doloop())
		//	{
		//		uNextDataOffset = m_ulLoopStart - m_uDataOffset;
	//		}
		}

		AKRESULT res = m_pACPStreamBuffer[m_uBufferIndex].Set( m_pNextAddress, ulTruncatedSizeBytes, m_ulFileOffsetPrevious, m_nBlockSize, m_nSamplesPerBlock, uStitchBufferFillSize
#ifdef AT9_STREAM_DEBUG
			, 0, ulTruncatedSizeBytes, -m_nSamplesPerBlock, ByteToSampleRelative(ulTruncatedSizeBytes)
#endif
			);

		if (res != AK_Success)
		{
			MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_CannotStartStreamNoMemory, m_pCtx );
			return AK_InvalidFile;
		}
	}
	return result;
}

AKRESULT CAkSrcFileAt9::RelocateMedia( AkUInt8* in_pNewMedia, AkUInt8* in_pOldMedia )
{
	AkUIntPtr uMemoryOffset = (AkUIntPtr)in_pNewMedia - (AkUIntPtr)in_pOldMedia;

	return CAkSrcFileBase::RelocateMedia(in_pNewMedia, in_pOldMedia);
}

bool CAkSrcFileAt9::Register()
{
	if ( !CAkSrcACPBase::Register() ||
		CAkLEngine::GetACPManager()->Register(this) != AK_Success)
	{
		return false;
	}

	return true;
}

void CAkSrcFileAt9::Unregister()
{
	if (m_pStream)
	{
		ReleaseStreamBuffer();  // Is that necessary?
	}
	CAkLEngine::GetACPManager()->Unregister(this);
	CAkSrcACPBase::Unregister();
}

// Returns format-specific throughput.
AkReal32 CAkSrcFileAt9::GetThroughput( const AkAudioFormat & in_rFormat )
{
	return (AkReal32)( m_nSamplesPerBlock * in_rFormat.uSampleRate ) / 1000.f;
}

// Finds the closest offset in file that corresponds to the desired sample position.
// Returns the file offset (in bytes, relative to the beginning of the file) and its associated sample position.
// Returns AK_Fail if the codec is unable to seek.
AKRESULT CAkSrcFileAt9::FindClosestFileOffset( //todo: rename since it's not const anymore
	AkUInt32 in_uDesiredSample,		// Desired sample position in file.
	AkUInt32 & out_uSeekedSample,	// Returned sample where file position was set.
	AkUInt32 & out_uFileOffset		// Returned file offset where file position was set.
	)
{	
	// IMPORTANT: Never access BlockAlign from PBI format!
	AkInt32 numBlocks = (in_uDesiredSample / m_nSamplesPerBlock) - 1;
	AkInt32 roundedSample = numBlocks * m_nSamplesPerBlock;
	out_uFileOffset = m_uDataOffset + SampleToByteRelative(roundedSample); 

	AKASSERT((int)in_uDesiredSample >= roundedSample);
	m_uSampleSkipOnFileSeekStart = in_uDesiredSample - roundedSample;
	m_DecoderSamplePosition = roundedSample;
	out_uSeekedSample = in_uDesiredSample;

	AKASSERT ( m_DecoderSamplePosition % m_nSamplesPerBlock == 0 );

#ifdef AT9_STREAM_DEBUG_OUTPUT
	char msg[256];
	sprintf( msg, "FindClosestFileOffset %p %i, %i, %i \n", this, m_DecoderSamplePosition, m_uSampleSkipOnFileSeekStart, m_nSamplesPerBlock );
	AKPLATFORM::OutputDebugMsg( msg );
#endif

	PrepareRingbuffer();
	
	return AK_Success;
}

// todo: move to base class
// Discard everything currently in the ring buffer
void CAkSrcFileAt9::PrepareRingbuffer()
{
	m_Ringbuffer.Reset();
	//m_DecoderSamplePosition = m_uCurSample-m_nSamplesPerBlock;
	AKASSERT( m_uSampleSkipOnFileSeekStart >= m_nSamplesPerBlock );
	m_Ringbuffer.m_SkipFramesStart.SetSkipFrames( 0, m_uSampleSkipOnFileSeekStart, false );	
	m_uSampleSkipOnFileSeekStart = 0;

	EnterPreBufferingState();
	m_uDecoderLoopCntAhead = 0;

	m_pACPStreamBuffer[0].Reset();
	m_pACPStreamBuffer[1].Reset();
}

AKRESULT CAkSrcFileAt9::GrabNextStreamBuffer()
{
#ifdef AT9_STREAM_DEBUG_OUTPUT
	char msg[256];
	sprintf( msg, "######## ########  GrabNextStreamBuffer left=%i \n", m_ulSizeLeft );
	AKPLATFORM::OutputDebugMsg( msg );
#endif

#ifdef AT9_STREAM_DEBUG
	AKASSERT( m_ulSizeLeft == m_pACPStreamBuffer[m_uBufferIndex].GetSizeLeft() );

	AkInt32 ulNextBufferStartSample;
	AkInt32 ulNextBufferEndSample;

	AkInt32 ulNextBufferStartByte;
	AkInt32 ulNextBufferEndByte;
#endif

	AkUInt32 uNextDataOffset = GetDataOffsetFromHeader();
	AkUInt8* pPrevNextAddress = m_pNextAddress;
	AkUInt32 ulPrevSizeLeft = m_ulSizeLeft;
	m_pNextAddress = NULL;
	m_ulSizeLeft = 0;
	
	AKRESULT res = FetchStreamBuffer();
	if (res == AK_NoDataReady)
	{
#ifdef AT9_STREAM_DEBUG_OUTPUT
		sprintf( msg, "-- -- -- -- [%p] GrabNextStreamBuffer() AK_NoDataReady \n", this );
		AKPLATFORM::OutputDebugMsg( msg );
#endif
		m_pNextAddress = pPrevNextAddress;
		m_ulSizeLeft = ulPrevSizeLeft;
		m_bWaitingOnNextBuffer = true;
		return res;
	}
	else if (res != AK_DataReady)
	{
		AKASSERT( res != AK_Success ); // FetchStreamBuffer should return AK_DataReady when succesfull.
		m_DecodingFailed = true;
		return res;
	}
	m_bWaitingOnNextBuffer = false;

	AKASSERT(m_pNextAddress != NULL);

	AkUInt32 stitchingBytesNeeded = m_pACPStreamBuffer[m_uBufferIndex].NeedStitching();
	// Complete stich buffer of previous stream buffer
	if (stitchingBytesNeeded > 0)
	{
		m_pACPStreamBuffer[m_uBufferIndex].Stitch( m_pNextAddress, stitchingBytesNeeded);
		ConsumeData(stitchingBytesNeeded);
		uNextDataOffset += stitchingBytesNeeded;
		AKASSERT( (uNextDataOffset % m_nBlockSize) == 0 );
	}

	AkUInt32 ulTruncadedSizeBytes = (m_ulSizeLeft) / m_nBlockSize * m_nBlockSize;
#ifdef AT9_STREAM_DEBUG
	ulNextBufferStartSample = ByteToSampleRelative(uNextDataOffset);
	ulNextBufferEndSample = ulNextBufferStartSample + ByteToSample(ulTruncadedSizeBytes);
	ulNextBufferStartByte = uNextDataOffset;
	ulNextBufferEndByte = uNextDataOffset + ulTruncadedSizeBytes;
#endif
	m_uBufferIndex = !m_uBufferIndex;
	AKASSERT(m_ulSizeLeft >= ulTruncadedSizeBytes);
	AKRESULT resSet = m_pACPStreamBuffer[m_uBufferIndex].Set( m_pNextAddress, ulTruncadedSizeBytes, m_ulFileOffsetPrevious, m_nBlockSize, m_nSamplesPerBlock, m_ulSizeLeft - ulTruncadedSizeBytes
#ifdef AT9_STREAM_DEBUG
		, ulNextBufferStartByte, ulNextBufferEndByte, ulNextBufferStartSample, ulNextBufferEndSample  
#endif
		);

	if (resSet != AK_Success)
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_CannotStartStreamNoMemory, m_pCtx );
		return AK_InvalidFile;
	}

	AKASSERT (m_pACPStreamBuffer[m_uBufferIndex].GetSizeLeft() == m_ulSizeLeft);

	return res;
}

void CAkSrcFileAt9::UpdateStreamBuffer()
{
	AKRESULT result;
	AkUInt32 uInputDataSize = m_ulSizeLeft;
		
	if ( uInputDataSize <= SampleToByte( GetNumSamplesToDecode()) )
	{
#ifdef AT9_STREAM_DEBUG_OUTPUT
		/*char msg[512];
		sprintf( msg, "** ** ** ** [%p] UpdateStreamBuffer will grab more data, only %lu left, need %lu \n", this, uInputDataSize, SampleToByte(GetNumSamplesToDecode()));
		AKPLATFORM::OutputDebugMsg( msg );*/
#endif
		if (!HasNoMoreStreamData())
		{
			GrabNextStreamBuffer();
		}
	}
}

int CAkSrcFileAt9::SetDecodingInputBuffers(AkInt32 in_uSampleStart, AkInt32 in_uSampleEnd, AkUInt16 in_uBufferIndex, AkUInt32& out_remainingSize )
{

#ifdef AT9_STREAM_DEBUG
	AKASSERT( ((m_pACPStreamBuffer[m_uBufferIndex].GetBufferEndInBytes() - m_pACPStreamBuffer[m_uBufferIndex].GetBufferStartInBytes()) == m_pACPStreamBuffer[m_uBufferIndex].GetSizeLeft()) );
#endif

#ifdef AT9_STREAM_DEBUG_OUTPUT
		char msg[256];
		sprintf( msg, "-- -- -- -- SetDecodingInputBuffers target %i [%i,%i] x%p, x%i \n", in_uBufferIndex, in_uSampleStart, in_uSampleEnd, m_InputBufferDesc[in_uBufferIndex].pAddress, m_InputBufferDesc[in_uBufferIndex].szSize);
		AKPLATFORM::OutputDebugMsg( msg );
#endif

#ifdef AT9_STREAM_DEBUG
	/*if (!in_startDecodingOffset && m_pACPStreamBuffer[!m_uBufferIndex].IsEmptyOrAllDataInDecoder() && m_ulFileOffset != 128 && m_ulFileOffset != m_ulLoopStart)
	{
		AKASSERT( (GetDataOffsetFromHeader() ) == m_pACPStreamBuffer[m_uBufferIndex].GetBufferEndInBytes() );
	}*/

	AKASSERT(in_uSampleStart % m_nSamplesPerBlock == 0);
	AKASSERT(in_uSampleEnd % m_nSamplesPerBlock == 0);
	AKASSERT((in_uSampleEnd-in_uSampleStart) % m_nSamplesPerBlock == 0);
#endif

	size_t uBytesLength = SampleToByte(in_uSampleEnd - in_uSampleStart);

	AkUInt8 inputIndex = in_uBufferIndex;
	AkInt32 uSampleStart = in_uSampleStart;


	// Some data is left in the previous buffer, decode that first
	if ( !m_pACPStreamBuffer[!m_uBufferIndex].IsEmptyOrAllDataInDecoder() )
	{
#ifdef AT9_STREAM_DEBUG
		AKASSERT( ((m_pACPStreamBuffer[!m_uBufferIndex].GetBufferEndInBytes() - m_pACPStreamBuffer[!m_uBufferIndex].GetBufferStartInBytes()) == m_pACPStreamBuffer[!m_uBufferIndex].GetSizeLeft()) );
		AKASSERT( m_pACPStreamBuffer[!m_uBufferIndex].GetBufferStartInSample() == uSampleStart );
			
		AKASSERT( GetBufferStartInBytesInSample(!m_uBufferIndex) == uSampleStart);
#endif
		AkUInt32 decodeSize = m_pACPStreamBuffer[!m_uBufferIndex].GetBytesLeftInStreamBuffer();

		// min
		decodeSize = (decodeSize <= uBytesLength) ? decodeSize : uBytesLength;

		m_InputBufferDesc[inputIndex].pAddress = m_pACPStreamBuffer[!m_uBufferIndex].GetAddress();
		m_InputBufferDesc[inputIndex].szSize = decodeSize;

#ifdef AT9_STREAM_DEBUG
		AKASSERT(decodeSize > 0);
#endif
		m_pACPStreamBuffer[!m_uBufferIndex].SetInDecoderBytes(decodeSize, ByteToSample(decodeSize));

#ifdef AT9_STREAM_DEBUG_OUTPUT
		char msg[256];
		/*sprintf( msg, "-- -- -- -- [%p] SetDecodingInputBuffers target [%i, %i](%i)s, [%i, %i]b \n", in_uBufferIndex, 
																	m_pACPStreamBuffer[!m_uBufferIndex].GetBufferStartInSample(), m_pACPStreamBuffer[!m_uBufferIndex].GetBufferStartInSample() + ByteToSample(decodeSize), ByteToSample(decodeSize),
																	m_pACPStreamBuffer[!m_uBufferIndex].GetBufferStartInBytes(), m_pACPStreamBuffer[!m_uBufferIndex].GetBufferStartInBytes() + decodeSize); //SampleToByte(in_uSampleEnd-in_uSampleStart)
		AKPLATFORM::OutputDebugMsg( msg );*/
#endif
		uBytesLength -= decodeSize;
		uSampleStart += ByteToSample(decodeSize);

#ifdef AT9_STREAM_DEBUG
		AKASSERT( ( (m_pACPStreamBuffer[!m_uBufferIndex].GetBufferStartInBytes() + decodeSize) == m_pACPStreamBuffer[!m_uBufferIndex].GetBufferEndInBytes() - m_pACPStreamBuffer[!m_uBufferIndex].GetStitchBufferFillSize()) );
#endif
		inputIndex++;

		if (uSampleStart >= m_uPCMLoopEnd && Stream_Doloop())
		{
#ifdef AT9_STREAM_DEBUG
			AKASSERT( uBytesLength == 0 );
#endif
			uSampleStart = m_uPCMLoopStart - m_nSamplesPerBlock;
		}
		
#ifdef AT9_STREAM_DEBUG_OUTPUT
		sprintf( msg, "-- -- -- -- [%p] SetDecodingInputBuffers leftovers [%i | %i] \n", this, m_pACPStreamBuffer[0].GetInDecoderBytes(), m_pACPStreamBuffer[1].GetInDecoderBytes() );
		AKPLATFORM::OutputDebugMsg( msg );
#endif
	}

	// Some data is left in the previous STITCH buffer, decode that first
	if ( uBytesLength > 0 && m_pACPStreamBuffer[!m_uBufferIndex].HasStitchBuffer())
	{
		AkUInt32 decodeSize = m_pACPStreamBuffer[!m_uBufferIndex].GetStitchBufferFillSize();
		AKASSERT(uBytesLength >= decodeSize);
#ifdef AT9_STREAM_DEBUG
		char msg[256];
		sprintf( msg, "-- -- -- -- [%p] SetDecodingInputBuffers STITCH target [%i, %i]b \n", in_uBufferIndex, 
																m_pACPStreamBuffer[!m_uBufferIndex].GetBufferEndInBytes() - m_pACPStreamBuffer[!m_uBufferIndex].GetStitchBufferFillSize(), m_pACPStreamBuffer[!m_uBufferIndex].GetBufferEndInBytes()); //SampleToByte(in_uSampleEnd-in_uSampleStart)
		AKPLATFORM::OutputDebugMsg( msg );
#endif
		m_InputBufferDesc[inputIndex].pAddress = m_pACPStreamBuffer[!m_uBufferIndex].GetStitchBuffer();
		m_InputBufferDesc[inputIndex].szSize = decodeSize;

		m_pACPStreamBuffer[!m_uBufferIndex].SetInDecoderBytes(decodeSize, ByteToSample(decodeSize));

		inputIndex++;

		uBytesLength -= decodeSize;
		uSampleStart += ByteToSample(decodeSize);

		out_remainingSize = ByteToSample(SampleToByte(in_uSampleEnd - in_uSampleStart) - ( SampleToByte(in_uSampleEnd - in_uSampleStart) - uBytesLength ));

		return inputIndex; // todo: investigate why setting up a third decoding task fails
	}
	
	// Default decode, if we still need to or can
	if (uBytesLength > 0 && !m_pACPStreamBuffer[m_uBufferIndex].IsEmptyOrAllDataInDecoder())
	{
#ifdef AT9_STREAM_DEBUG
		AKASSERT( m_ulSizeLeft == m_pACPStreamBuffer[m_uBufferIndex].GetSizeLeft() );
		AKASSERT( m_pACPStreamBuffer[m_uBufferIndex].GetBufferStartInSample() == uSampleStart || m_pACPStreamBuffer[m_uBufferIndex].GetBufferStartInSample() == 0 ); // looping, todo, generic

		AKASSERT( m_pACPStreamBuffer[m_uBufferIndex].GetAddress() == m_pNextAddress );
		AKASSERT( m_pACPStreamBuffer[m_uBufferIndex].GetSizeLeft() >= uBytesLength );
#endif

		m_InputBufferDesc[inputIndex].pAddress = m_pACPStreamBuffer[m_uBufferIndex].GetAddress();
		m_InputBufferDesc[inputIndex].szSize = uBytesLength;

		m_pACPStreamBuffer[m_uBufferIndex].SetInDecoderBytes(m_InputBufferDesc[inputIndex].szSize, ByteToSample( m_InputBufferDesc[inputIndex].szSize ));
#ifdef AT9_STREAM_DEBUG_OUTPUT
		/*char msg[256];
		sprintf( msg, "-- -- -- -- [%p] SetDecodingInputBuffers target [%i, %i](%i)s, [%i, %i]b \n", in_uBufferIndex, 
																	m_pACPStreamBuffer[m_uBufferIndex].GetBufferStartInSample(), m_pACPStreamBuffer[m_uBufferIndex].GetBufferStartInSample() + ByteToSample(uBytesLength), ByteToSample(uBytesLength),
																	m_pACPStreamBuffer[m_uBufferIndex].GetBufferStartInBytes(), m_pACPStreamBuffer[m_uBufferIndex].GetBufferStartInBytes() + uBytesLength);
		AKPLATFORM::OutputDebugMsg( msg );*/
#endif
#ifdef AT9_STREAM_DEBUG
		AKASSERT( m_pACPStreamBuffer[m_uBufferIndex].GetBufferStartInSample() + ByteToSample(uBytesLength) == in_uSampleEnd );
		AKASSERT(m_pACPStreamBuffer[m_uBufferIndex].IsBufferDecoding());
#endif
		inputIndex++;
		uBytesLength = 0;
	}
	
	out_remainingSize = ByteToSample(uBytesLength);
	return inputIndex;
}

AkUInt32 CAkSrcFileAt9::DecodingEnded()
{	
	if (m_InitResult.sResult.iResult < 0 || m_DecodeResult.sResult.iResult < 0 )
	{ 
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_ATRAC9DecodeFailed, m_pCtx);
		m_DecodingFailed = true;
		m_DecodingStarted = false;
		return 0;
	}

#ifdef AT9_STREAM_DEBUG
	AKASSERT( m_OutputBufferDesc[0].szSize + m_OutputBufferDesc[1].szSize == m_DecodeResult.sStream.iSizeProduced );
	AKASSERT( m_InputBufferDesc[0].szSize + m_InputBufferDesc[1].szSize + m_InputBufferDesc[2].szSize == (m_DecodeResult.sStream.iSizeConsumed) );
#endif

	int previousBufferAdv = 0;

	if ( !m_pACPStreamBuffer[!m_uBufferIndex].IsEmpty() )
	{
		previousBufferAdv = m_pACPStreamBuffer[!m_uBufferIndex].GetInDecoderBytes();

		m_pACPStreamBuffer[!m_uBufferIndex].DecodingCompleted( m_nBlockSize, m_nSamplesPerBlock );
#ifdef AT9_STREAM_DEBUG //+-
		AKASSERT(m_pACPStreamBuffer[!m_uBufferIndex].IsEmpty());
#endif
		ReleaseStreamBuffer();
	}
	
	ConsumeData(m_DecodeResult.sStream.iSizeConsumed - previousBufferAdv);
	m_ulFileOffsetPrevious += m_DecodeResult.sStream.iSizeConsumed;

	if (m_pACPStreamBuffer[m_uBufferIndex].IsBufferDecoding())
	{
		m_pACPStreamBuffer[m_uBufferIndex].DecodingCompleted( m_nBlockSize, m_nSamplesPerBlock );
	}

#ifdef AT9_STREAM_DEBUG //+-
	AKASSERT (m_pACPStreamBuffer[m_uBufferIndex].GetSizeLeft() == m_ulSizeLeft);
#endif

	if ( m_pACPStreamBuffer[m_uBufferIndex].IsEmpty() )
	{
		AKASSERT(m_ulSizeLeft == 0);
		ReleaseStreamBuffer();
	}
	m_DecodingStarted = false;

	return m_InputBufferDesc[0].szSize + m_InputBufferDesc[1].szSize + m_InputBufferDesc[2].szSize;
}

void CAkSrcFileAt9::FixLoopPoints()
{
	// Set loop points.
    AkUInt32 ulEndOfData = m_uDataOffset + m_uDataSize;
	
	if (m_uPCMLoopEnd == 0)
	{
		m_uPCMLoopEnd = m_uTotalSamples-1;
	}
	AkUInt32 uLoopEnd = m_uPCMLoopEnd+1;
	AkInt32 iFixedEndOfLoop = FloorSampleToBlockSize(uLoopEnd + m_nSamplesPerBlock-1);
	AkInt32 iFixedStartOfLoop = FloorSampleToBlockSize( m_uPCMLoopStart ) - m_nSamplesPerBlock;

	m_ulLoopStart = m_uDataOffset + SampleToByteRelative(iFixedStartOfLoop); 
	m_ulLoopEnd = m_uDataOffset + SampleToByteRelative(iFixedEndOfLoop);

}

// Redondant, remove
AKRESULT CAkSrcFileAt9::ChangeSourcePosition()
{
	return CAkSrcFileBase::ChangeSourcePosition();
}

void CAkSrcFileAt9::VirtualOn( AkVirtualQueueBehavior eBehavior )
{	
#ifdef AT9_STREAM_DEBUG_OUTPUT
	char msg[256];
	sprintf( msg, "VirtualOn\n" );
	AKPLATFORM::OutputDebugMsg( msg );
#endif
	m_IsVirtual = true;
	if ( eBehavior != AkVirtualQueueBehavior_Resume )
	{
		Unregister();
	}

	CAkSrcFileBase::VirtualOn(eBehavior);
}

AKRESULT CAkSrcFileAt9::VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset )
{
#ifdef AT9_STREAM_DEBUG_OUTPUT
	char msg[256];
	sprintf( msg, "VirtualOff\n" );
	AKPLATFORM::OutputDebugMsg( msg );
#endif
	m_IsVirtual = false;
	AKRESULT result = CAkSrcFileBase::VirtualOff(eBehavior, in_bUseSourceOffset);

	if ( eBehavior != AkVirtualQueueBehavior_Resume )
	{
		if( !Register() )
		{
			return AK_Fail;
		}
	}

	return result;
}