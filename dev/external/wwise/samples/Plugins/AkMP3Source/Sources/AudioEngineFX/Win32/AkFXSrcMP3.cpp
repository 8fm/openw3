//////////////////////////////////////////////////////////////////////
//
// AkFXSrcMP3.cpp
//
// Sample capture of audio input to be used as a source.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkFXSrcMP3.h"
#include "AkMP3TagReader.h"
#include <AK/Tools/Common/AkAssert.h>
#include <math.h>
#include <tchar.h>

#define MP3_BLOCK_SIZE 1024 // 522

static struct
{
	TCHAR tszPath[ MAX_PATH ];
	AkMP3BaseInfo info;
	AkTimeMs msStartAt;
	AkMP3Callback funcCallback;
	void * pCallbackCookie;
} g_AkMP3Info = { 0 };

void AkMP3PrepareForPlayback(
	LPCTSTR in_tszFilename,			///< In: path of MP3 file to read, relative to the Low-Level I/O Base Path.
	AkMP3BaseInfo & in_info,		///< In: structure describing the MP3 file, as given by AkMP3InfoRead.
	AkTimeMs in_msStartAt,			///< In: optional playback start position, in milliseconds (should be in range [0, in_info.msDuration])
	AkMP3Callback in_funcCallback,	///< In: optional callback function. will be called every audio frame.
	void * in_pCallbackCookie		///< In: cookie which will be passed to callback function.
	)
{
	_tcsncpy_s( g_AkMP3Info.tszPath, MAX_PATH, in_tszFilename, _TRUNCATE );
	g_AkMP3Info.info = in_info;
	g_AkMP3Info.msStartAt = in_msStartAt;
	g_AkMP3Info.funcCallback = in_funcCallback;
	g_AkMP3Info.pCallbackCookie = in_pCallbackCookie;
}

static AkUInt32 AkMP3SeekToTime( const AkMP3BaseInfo & in_info, AkTimeMs in_msTime ) // returns file offset
{
	if ( in_info.bToc )
	{
		float fPct = 100.0f * ( (float) in_msTime / (float) in_info.msDuration );
		if ( fPct < 0.0f )
			fPct = 0.0f;
		else if ( fPct > 100.0f )
			fPct = 100.0f;

		int iIdx = min( 99, (int) fPct );

		float fA = (float) in_info.aToc[ iIdx ];
		float fB = ( iIdx < 99 ) ? (float) in_info.aToc[ iIdx + 1 ] : 256.0f;

		float fPos = fA + ( fB - fA ) * ( fPct - (float) iIdx ); // 0-256 position in file

		return (AkUInt32) ( ( fPos / 256.0f ) * (float) in_info.uMPEGStreamSize ) + in_info.uFirstFrameOffset;
	}
	else // CBR -- just a simple ratio
	{
		float fFrac = ( (float) in_msTime / (float) in_info.msDuration );

		return (AkUInt32) ( fFrac * in_info.uMPEGStreamSize ) + in_info.uFirstFrameOffset;
	}
}

AK::IAkPlugin* CreateMP3Source( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkFXSrcMP3() );
}

CAkFXSrcMP3::CAkFXSrcMP3()
	: m_uNumChannels( 0 )
	, m_uSampleRate( 0 )
	, m_msDuration( 0 )
	, m_pSharedParams( NULL )
	, m_pSourceFXContext( NULL )
	, m_pDstBufPos( NULL )
	, m_uDsBufSize( 0 )
	, m_pStreamBuffer( NULL )
	, m_pStreamBufferPos( NULL )
	, m_uStreamBufferSize( 0 )
	, m_pStream( NULL )
	, m_funcCallback( NULL )
	, m_pCallbackCookie( NULL )
	, m_uFramePos( 0 )
{
	::ZeroMemory( &m_hdrACM, sizeof( ACMSTREAMHEADER ) );
}

CAkFXSrcMP3::~CAkFXSrcMP3()
{
    if ( m_pStream )
        m_pStream->Destroy( );
}

AKRESULT CAkFXSrcMP3::Init(	
	AK::IAkPluginMemAlloc *			in_pAllocator,    		// Memory allocator interface.
	AK::IAkSourcePluginContext *	in_pSourceFXContext,	// Source FX context
	AK::IAkPluginParam *			in_pParams,				// Effect parameters.
	AkAudioFormat &					io_rFormat				// Output audio format.
	)
{
	if ( ( ( g_AkMP3Info.info.uNumChannels != 1 ) && ( g_AkMP3Info.info.uNumChannels != 2 ) )
		|| ( ( g_AkMP3Info.info.uSampleRate < 8000 ) || ( g_AkMP3Info.info.uSampleRate > 48000 ) ) )
		return AK_Fail;

	// Keep source FX context (looping info etc.)
	m_pSourceFXContext = in_pSourceFXContext;

	// Set parameters access
	m_pSharedParams = reinterpret_cast<CAkFxSrcMP3Params*>(in_pParams);

	// define desired output format
	WAVEFORMATEX wfxOut;
	wfxOut.wFormatTag = WAVE_FORMAT_PCM;
	wfxOut.nChannels = g_AkMP3Info.info.uNumChannels; 
	wfxOut.nSamplesPerSec = g_AkMP3Info.info.uSampleRate; 
	wfxOut.wBitsPerSample = 16; // 16 bits
	wfxOut.nBlockAlign = 2 * g_AkMP3Info.info.uNumChannels; // 4 bytes of data at a time are useful (1 sample)
	wfxOut.nAvgBytesPerSec = wfxOut.nBlockAlign * g_AkMP3Info.info.uSampleRate; // byte-rate
	wfxOut.cbSize = 0; // no more data to follow

	// define MP3 input format
	MPEGLAYER3WAVEFORMAT wfxIn;
	wfxIn.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
	wfxIn.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
	wfxIn.wfx.nChannels = g_AkMP3Info.info.uNumChannels;
	wfxIn.wfx.nAvgBytesPerSec = g_AkMP3Info.info.uBitRate * (1000 / 8); 
	wfxIn.wfx.wBitsPerSample = 0;
	wfxIn.wfx.nBlockAlign = 1;
	wfxIn.wfx.nSamplesPerSec = g_AkMP3Info.info.uSampleRate;
	wfxIn.fdwFlags = MPEGLAYER3_FLAG_PADDING_OFF;
	wfxIn.nBlockSize = MP3_BLOCK_SIZE;
	wfxIn.nFramesPerBlock = 1;
	wfxIn.nCodecDelay = 1393;
	wfxIn.wID = MPEGLAYER3_ID_MPEG;

	m_hMP3Stream = NULL;
	MMRESULT mmr = acmStreamOpen( &m_hMP3Stream, NULL, (LPWAVEFORMATEX) &wfxIn, &wfxOut, NULL, 0, 0, 0 );
	if ( mmr != MMSYSERR_NOERROR )
		return AK_Fail;

	// MP3 stream converter opened correctly
	// now, let's open a file, read in a bunch of MP3 data, and convert it!

	// open file

	AkAutoStmHeuristics heuristics;
    heuristics.uMinNumBuffers = 0;
    heuristics.fThroughput = g_AkMP3Info.info.uBitRate / 8.0f;
    heuristics.uLoopStart = 0;
    heuristics.uLoopEnd = 0;
    heuristics.priority = AK_MIN_PRIORITY;

	AkFileSystemFlags fileSystemFlags( AKCOMPANYID_AUDIOKINETIC, AKSOURCEID_MP3, NULL, 0, false, false, AK_INVALID_FILE_ID );

	AKRESULT eResult = in_pSourceFXContext->GetStreamMgr()->CreateAuto( 
		g_AkMP3Info.tszPath,
		&fileSystemFlags,   // File system special parameters.
		heuristics,         // Auto stream heuristics.
		NULL,               // Stream buffer constraints: none.
		m_pStream, true );  // IAkAutoStream ** out_ppStream.
	if ( eResult != AK_Success )
		return AK_Fail;

#ifndef AK_OPTIMIZED
	// Truncate first characters.
	size_t len = wcslen( g_AkMP3Info.tszPath );
	if ( len < AK_MONITOR_STREAMNAME_MAXLENGTH )
		m_pStream->SetStreamName( g_AkMP3Info.tszPath );
	else
	{
		wchar_t szName[AK_MONITOR_STREAMNAME_MAXLENGTH];
		const wchar_t * pSrcStr = (const wchar_t*)g_AkMP3Info.tszPath + len - AK_MONITOR_STREAMNAME_MAXLENGTH + 1;
		AKPLATFORM::SafeStrCpy( szName, pSrcStr, AK_MONITOR_STREAMNAME_MAXLENGTH );
		szName[0] = '.';
		szName[1] = '.';
		szName[2] = '.';
		m_pStream->SetStreamName( szName );
	}
#endif

	// Seek to starting position

	AkUInt32 uSeekOffset;
	if ( g_AkMP3Info.msStartAt != 0 )
	{
		uSeekOffset = AkMP3SeekToTime( g_AkMP3Info.info, g_AkMP3Info.msStartAt );
		m_uFramePos = (AkUInt32) ( (double) g_AkMP3Info.msStartAt * g_AkMP3Info.info.uSampleRate / 1000.0 );
	}
	else
	{
		uSeekOffset = g_AkMP3Info.info.uFirstFrameOffset;
		m_uFramePos = 0;
	}

	if ( uSeekOffset )
	{
		AkInt64 iRealOffset;
		eResult = m_pStream->SetPosition( uSeekOffset, AK_MoveBegin, &iRealOffset );
		if ( eResult != AK_Success )
			return AK_Fail;
	}

	m_pStream->Start();

	// prepare the decoder

	DWORD dwDstSize = 0;
	mmr = acmStreamSize( m_hMP3Stream, MP3_BLOCK_SIZE, &dwDstSize, ACM_STREAMSIZEF_SOURCE );
	if ( mmr != MMSYSERR_NOERROR || dwDstSize == 0 )
		return AK_Fail;

	m_hdrACM.cbStruct = sizeof(ACMSTREAMHEADER );
	m_hdrACM.pbSrc = (BYTE *) AK_PLUGIN_ALLOC( in_pAllocator, MP3_BLOCK_SIZE );
	m_hdrACM.cbSrcLength = MP3_BLOCK_SIZE;
	m_hdrACM.pbDst = (BYTE *) AK_PLUGIN_ALLOC( in_pAllocator, dwDstSize );
	m_hdrACM.cbDstLength = dwDstSize;

	mmr = acmStreamPrepareHeader( m_hMP3Stream, &m_hdrACM, 0 );
	if ( mmr != MMSYSERR_NOERROR )
		return AK_Fail;

	m_hdrACM.cbSrcLengthUsed = MP3_BLOCK_SIZE; // for our own management
	m_pDstBufPos = (AkInt16 *) m_hdrACM.pbDst;
	m_uDsBufSize = 0;

	AkUInt32 channelMask;
	switch( g_AkMP3Info.info.uNumChannels )
	{
	case 1:
		channelMask = AK_SPEAKER_SETUP_MONO;
		break;

	case 2:
		channelMask = AK_SPEAKER_SETUP_STEREO;
		break;

	default:
		AKASSERT( !"Unhandle number of channel" );
		channelMask = AK_SPEAKER_SETUP_MONO;
		break;
	}

	// new one
	io_rFormat.SetAll( 
		g_AkMP3Info.info.uSampleRate, 
		channelMask, 
		16, 
		2 * g_AkMP3Info.info.uNumChannels, 
		AK_INT, 
		AK_INTERLEAVED
		);

	m_uNumChannels = g_AkMP3Info.info.uNumChannels;
	m_uSampleRate = g_AkMP3Info.info.uSampleRate;
	m_msDuration = g_AkMP3Info.info.msDuration;
	m_funcCallback =  g_AkMP3Info.funcCallback;
	m_pCallbackCookie = g_AkMP3Info.pCallbackCookie;

	return AK_Success;
}

AKRESULT CAkFXSrcMP3::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if ( m_hdrACM.pbSrc )
		AK_PLUGIN_FREE( in_pAllocator, m_hdrACM.pbSrc );

	if ( m_hdrACM.pbDst )
		AK_PLUGIN_FREE( in_pAllocator, m_hdrACM.pbDst );

	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}


AKRESULT CAkFXSrcMP3::Reset( )
{
	return AK_Success;
}

AKRESULT CAkFXSrcMP3::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
    out_rPluginInfo.eType = AkPluginTypeSource;
	out_rPluginInfo.bIsInPlace = true;
	out_rPluginInfo.bIsAsynchronous = false;
    return AK_Success;
}

void CAkFXSrcMP3::Execute( 
		AkAudioBuffer *	io_pBufferOut		///< In/Out audio buffer data structure (in-place processing)
#ifdef AK_PS3
		, AK::MultiCoreServices::DspProcess*&	out_pDspProcess	///< Asynchronous DSP process utilities on PS3
#endif
		)
{
	if ( m_funcCallback )
	{
		AkTimeMs msPos = ( 20 * m_uFramePos ) / ( m_uSampleRate / 50 );
		m_funcCallback( m_pCallbackCookie, msPos );
	}
	
	AkUInt32 uValidFrames = 0;
	AkUInt32 uMaxFrames = io_pBufferOut->MaxFrames();

	AkInt16 * AK_RESTRICT pBufOut = (AkInt16 *) ( io_pBufferOut->GetInterleavedData() );

	while( uValidFrames < uMaxFrames )
	{
		if ( !m_uDsBufSize )
		{
			// The decoder might have filled the destination buffer without exhausting the input data;
			// in this case it will report '0' as cbSrcLengthUsed, and we should not read in new data.
			if ( m_hdrACM.cbSrcLengthUsed == m_hdrACM.cbSrcLength )
			{
				if ( !m_uStreamBufferSize )
				{
					if ( m_pStreamBuffer )
					{
						m_pStream->ReleaseBuffer();
						m_pStreamBuffer = NULL;
					}

					io_pBufferOut->eState = m_pStream->GetBuffer( m_pStreamBuffer, m_uStreamBufferSize, false );

					if (	io_pBufferOut->eState == AK_NoDataReady
						||	io_pBufferOut->eState == AK_Fail )
					{
						io_pBufferOut->uValidFrames = 0;
						return;
					}
					else if ( m_uStreamBufferSize == 0 )
					{
						io_pBufferOut->eState = AK_NoMoreData;
						return;
					}

					m_pStreamBufferPos = (AkUInt8 *) m_pStreamBuffer;
				}

				AKASSERT( m_uStreamBufferSize );

				AkUInt32 uToCopy = min( MP3_BLOCK_SIZE, m_uStreamBufferSize );
				memcpy( m_hdrACM.pbSrc, m_pStreamBufferPos, uToCopy ); // ACM requires that the source data always be at the same memory address

				m_uStreamBufferSize -= uToCopy;
				m_pStreamBufferPos += uToCopy;
				m_hdrACM.cbSrcLength = uToCopy;
  			}

			// convert the data
			MMRESULT mmr = acmStreamConvert( m_hMP3Stream, &m_hdrACM, ACM_STREAMCONVERTF_BLOCKALIGN );
			AKASSERT( mmr == 0 );

			if (  m_hdrACM.cbSrcLengthUsed == 0 && m_hdrACM.cbDstLengthUsed == 0 )
				break;
	
			m_pDstBufPos = (AkInt16 *) m_hdrACM.pbDst;
			m_uDsBufSize = m_hdrACM.cbDstLengthUsed;
		}

		if ( m_uNumChannels == 1 )
		{
			AkUInt32 uToCopy = min( ( uMaxFrames - uValidFrames ) << 1, m_uDsBufSize );
			memcpy( pBufOut, m_pDstBufPos, uToCopy );

			m_uDsBufSize -= uToCopy;

			pBufOut += uToCopy >> 1;
			m_pDstBufPos += uToCopy >> 1;
			uValidFrames += uToCopy >> 1;
			m_uFramePos += uToCopy >> 1;
		}
		else // stereo
		{
			AkUInt32 uToCopy = min( ( uMaxFrames - uValidFrames ) << 2, m_uDsBufSize );
			memcpy( pBufOut, m_pDstBufPos, uToCopy );

			m_uDsBufSize -= uToCopy;

			pBufOut += uToCopy >> 1;
			m_pDstBufPos += uToCopy >> 1;
			uValidFrames += uToCopy >> 2;
			m_uFramePos += uToCopy >> 2;
		}
	}

	io_pBufferOut->uValidFrames = (AkUInt16)uValidFrames;
	io_pBufferOut->eState = AK_DataReady;
}

AkReal32 CAkFXSrcMP3::GetDuration() const
{
	return static_cast<AkReal32>(m_msDuration);
}
