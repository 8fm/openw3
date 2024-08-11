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
// AkFDNReverbFX.cpp
//
// FDN Reverb implementation.
//
// Copyright 2007 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkFDNReverbFX.h"
#include <AK/Tools/Common/AkAssert.h>
#include <math.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <stdlib.h>	// qsort

//#define MEMORYOUTPUT
#if defined(_DEBUG) && defined(MEMORYOUTPUT)
#include <stdio.h> 
#endif

// Note: Feedback matrix used (implicit) is HouseHolder matrix that maximizes echo density (no zero entries).
// This matrix recursion can be computed very efficiently in 2N operation using matrix properties. See example below.
// 4 x 4 using following values -2/N, 1-2/N
// -.5, .5, -.5 -.5
// -.5, -.5, .5 -.5
// -.5, -.5, -.5 .5
// .5, -.5, -.5 -.5
// Algorithm: 
// 1) Take the sum of all delay outputs d1in = (d1 + d2 + d3 + d4)
// 2) Multiply by -2/N -> d1in = -0.5(d1 + d2 + d3 + d4)
// 3) Add the full output of one delay line further to effectively change the coefficient that was wrong in the previous computation
// i.e. -> d1in = -0.5(d1 + d2 + d3 + d4) + d2 == -.5d1 + .5d2 -.5d3 -.5d4

static const AkReal32 DCFILTERCUTOFFFREQ = 10.f;
static const AkReal32 TWOPI = 6.2831853071f;
static const AkReal64 IIRCOEFCALCCONST = log(10.0)/4.0;
#ifdef AK_LFECENTER
static const AKSIMD_DECLARE_V4F32( vMinusOne, -1.f, -1.f, -1.f, -1.f );
#endif
static const int iSplat0 = AKSIMD_SHUFFLE(0,0,0,0);

// FIXME: Win32 FDN reverb relies on contiguous channels!
#define LEFTCHANNELOFFSET			(AK_IDX_SETUP_5_FRONTLEFT * io_pBuffer->MaxFrames())
#define RIGHTCHANNELOFFSET			(AK_IDX_SETUP_5_FRONTRIGHT * io_pBuffer->MaxFrames())
#define CENTERCHANNELOFFSET			(AK_IDX_SETUP_5_CENTER * io_pBuffer->MaxFrames())
#define LFECHANNELOFFSET			(AK_IDX_SETUP_5_LFE * io_pBuffer->MaxFrames())
#define LEFTSURROUNDCHANNELOFFSET	(AK_IDX_SETUP_5_REARLEFT * io_pBuffer->MaxFrames())
#define RIGHTSURROUNDCHANNELOFFSET	(AK_IDX_SETUP_5_REARRIGHT * io_pBuffer->MaxFrames())

// Note: Matrix is constructed so that no column * -1 == another regardless of N (ensures proper decorrelation)
// Also sum of each column is == 0 as much as possible. This is not true for N = 4 for channels 5 and 6
// Otherwise constructed to be maximally different for each channel
static const AKSIMD_DECLARE_V4F32( vOutDecorrelationVectorA, 1.f, -1.f,  1.f, -1.f );
static const AKSIMD_DECLARE_V4F32( vOutDecorrelationVectorB, 1.f,  1.f, -1.f, -1.f );
#ifdef AK_LFECENTER
static const AKSIMD_DECLARE_V4F32( vOutDecorrelationVectorC,-1.f,  1.f,  1.f, -1.f );
static const AKSIMD_DECLARE_V4F32( vOutDecorrelationVectorD,-1.f, -1.f, -1.f,  1.f );
static const AKSIMD_DECLARE_V4F32( vOutDecorrelationVectorE, 1.f,  1.f, -1.f,  1.f );
static const AKSIMD_DECLARE_V4F32( vOutDecorrelationVectorF, 1.f, -1.f, -1.f, -1.f );
#endif

//Group 0 1 2 3
//L =	A A A A
//R =	B B B B
//C =	C C C C
//LFE = B -B B -B
//LS =	D E -B B
//RS =	F -D -C -A
//LSR = G -E C -B
//RSR = E D -A A


// Generic N channel routine use the following (explicit) table
static const AKSIMD_DECLARE_V4F32_TYPE vOutDecorrelationVector[][4] =
{
	{ {1.f,-1.f,1.f,-1.f}, {1.f,-1.f,1.f,-1.f}, {1.f,-1.f,1.f,-1.f}, {1.f,-1.f,1.f,-1.f} },
	{ {1.f,1.f,-1.f,-1.f}, {1.f,1.f,-1.f,-1.f}, {1.f,1.f,-1.f,-1.f}, {1.f,1.f,-1.f,-1.f} },
	{ {-1.f,1.f,1.f,-1.f}, {-1.f,1.f,1.f,-1.f}, {-1.f,1.f,1.f,-1.f}, {-1.f,1.f,1.f,-1.f} },
	{ {-1.f,-1.f,-1.f,1.f}, {1.f,1.f,-1.f,1.f}, {-1.f,-1.f,1.f,1.f}, {1.f,1.f,-1.f,-1.f} },
	{ {1.f,-1.f,-1.f,-1.f}, {1.f,1.f,-1.f,1.f}, {1.f,-1.f,-1.f,1.f}, {-1.f,1.f,-1.f,1.f} },
#ifdef AK_71AUDIO
	{ {1.f,-1.f,1.f,1.f}, {1.f,-1.f,-1.f,-1.f}, {-1.f,1.f,1.f,-1.f}, {-1.f,-1.f,1.f,1.f} },
	{ {1.f,1.f,-1.f,1.f}, {-1.f,-1.f,-1.f,1.f}, {-1.f,1.f,-1.f,1.f}, {1.f,-1.f,1.f,-1.f} },
#endif
	{ {1.f,1.f,-1.f,-1.f}, {-1.f,-1.f,1.f,1.f}, {1.f,1.f,-1.f,-1.f}, {-1.f,-1.f,1.f,1.f} },	// LFE
};

static int AkFDNQSortCompare(const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreateMatrixReverbFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CAkFDNReverbFX( ) );
}


// Constructor.
CAkFDNReverbFX::CAkFDNReverbFX()
	: m_fCachedReverbTime( 0.f )
	, m_fCachedHFRatio( 0.f )
{
	m_pParams = NULL;
	m_pAllocator = NULL;
	m_fpPerformDSP = NULL;
	m_pfPreDelayStart = NULL;
	m_pfPreDelayRW = NULL;
	m_pfPreDelayEnd = NULL;
	AKPLATFORM::AkMemSet( m_pfDelayStart, 0, MAXNUMDELAYGROUPS*sizeof(AkReal32*) );
	AKPLATFORM::AkMemSet( m_pfDelayEnd, 0, MAXNUMDELAYGROUPS*sizeof(AkReal32*) );
	AKPLATFORM::AkMemSet( m_pfDelayRead, 0, MAXNUMDELAYS*sizeof(AkReal32*) );
	AKPLATFORM::AkMemSet( m_pfDelayWrite, 0, MAXNUMDELAYGROUPS*sizeof(AkReal32*) );
	AKPLATFORM::AkMemSet( m_uNominalDelayLength, 0, MAXNUMDELAYS*sizeof(AkUInt32) );
	AKPLATFORM::AkMemSet( m_vIIRLPFB0, 0, MAXNUMDELAYS*sizeof(AkReal32) );
	AKPLATFORM::AkMemSet( m_vIIRLPFA1, 0, MAXNUMDELAYS*sizeof(AkReal32) );
	AKPLATFORM::AkMemSet( m_vIIRLPFMem, 0, MAXNUMDELAYS*sizeof(AkReal32) );
}

// Destructor.
CAkFDNReverbFX::~CAkFDNReverbFX()
{
	
}

// Initializes and allocate memory for the effect
AKRESULT CAkFDNReverbFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,		// Memory allocator interface.
								AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context
								AK::IAkPluginParam * in_pParams,			// Effect parameters.
								AkAudioFormat &	in_rFormat					// Required audio input format.
							   )
{
	m_uSampleRate = in_rFormat.uSampleRate;
	m_bIsSentMode = in_pFXCtx->IsSendModeEffect();
	m_pAllocator = in_pAllocator;
	m_pParams = static_cast<CAkFDNReverbFXParams*>(in_pParams);

	// Setup interpolation ramps for wet and dry parameters
	m_fCurrentDry = m_pParams->RTPC.fDryLevel;
	m_fCurrentWet = m_pParams->RTPC.fWetLevel;

	// Init DC filter
	m_fDCCoef = 1.f - (TWOPI * DCFILTERCUTOFFFREQ / m_uSampleRate);

	AK_PERF_RECORDING_RESET();
	m_uTailLength = (AkUInt32)( m_pParams->RTPC.fReverbTime * m_uSampleRate );

	AKRESULT eResult = InitDelayLines( in_rFormat.GetChannelMask() );
	return eResult;
}

// Terminates.
AKRESULT CAkFDNReverbFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	TermDelayLines();

	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Reset or seek to start.
AKRESULT CAkFDNReverbFX::Reset( )
{
	// Note: No need to zero out temp buffer has they first get overwritten every Execute()

	// Reset pre-delay
	if ( m_pfPreDelayStart )
		AkZeroMemLarge( m_pfPreDelayStart, sizeof(AkReal32) * m_uPreDelayLength );

	// Reset FIR LPF filter memory
	m_fFIRLPFMem = 0.f;

	// Reset feedback delay line states
	for ( AkUInt32 i = 0; i < m_pParams->NonRTPC.uNumberOfDelays/4; ++i )
	{
		// Reset IIR LPF filter memory
		m_vIIRLPFMem[i]  = AKSIMD_SETZERO_V4F32();

		// Reset delay line memory
		if ( m_pfDelayStart[i] )
			AkZeroMemLarge( (AkReal32*)m_pfDelayStart[i], sizeof(AkReal32) * (m_uNominalDelayLength[i*4+3]*4) );
	}

	// Reset DC filter
	m_fDCFwdMem = 0.f;
	m_fDCFbkMem = 0.f;

	return AK_Success;
}

// Effect info query.
AKRESULT CAkFDNReverbFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
	out_rPluginInfo.eType = AkPluginTypeEffect;
	out_rPluginInfo.bIsInPlace = true;
	out_rPluginInfo.bIsAsynchronous = false;
	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Execute
// Desc: Execute FDN reverb DSP.
//-----------------------------------------------------------------------------
void CAkFDNReverbFX::Execute( AkAudioBuffer* io_pBuffer )
{
	AK_PERF_RECORDING_START( "MatrixReverb", 25, 30 );

	if ( m_pParams->NonRTPC.bDirty )
	{
		AKRESULT eResult = InitDelayLines( io_pBuffer->GetChannelMask() );
		if ( eResult != AK_Success )
			return; // passthrough
		Reset();
	}

	// Silence LFE channel if LFE processing is disabled to avoid in-phase LFE doubling
#ifdef AK_LFECENTER
	bool bSilenceLFE = m_bIsSentMode && io_pBuffer->HasLFE() && !m_pParams->NonRTPC.uProcessLFE;
	if ( bSilenceLFE )
		AkZeroMemLarge( io_pBuffer->GetLFE(), io_pBuffer->uValidFrames * sizeof(AkReal32) );
#endif

	if ( m_uNumProcessedChannels == 0 )
		return;

	// Update RTPC values as necessary
	if ( m_pParams->RTPC.fReverbTime != m_fCachedReverbTime || m_pParams->RTPC.fHFRatio != m_fCachedHFRatio )
	{
		ComputeIIRLPFCoefs();
		ComputeFIRLPFCoefs();
		m_fCachedReverbTime = m_pParams->RTPC.fReverbTime;
		m_fCachedHFRatio = m_pParams->RTPC.fHFRatio;
		m_uTailLength = (AkUInt32)( m_pParams->RTPC.fReverbTime * m_uSampleRate );
	}

	// Disable dry level for sent mode path (wet still active for flexibility)
	if ( m_bIsSentMode )
	{
		// Send mode has no dry path
		m_fCurrentDry = 0.f;
		m_pParams->RTPC.fDryLevel = 0.f;
	}

	m_FXTailHandler.HandleTail( io_pBuffer, m_uTailLength ); 
	AKASSERT( io_pBuffer->uValidFrames <= io_pBuffer->MaxFrames() );

	if ( io_pBuffer->uValidFrames == 0 )
		return;

	// Dereference required perform method
	(this->*m_fpPerformDSP)( io_pBuffer );

	m_fCurrentDry = m_pParams->RTPC.fDryLevel;
	m_fCurrentWet = m_pParams->RTPC.fWetLevel;

	AK_PERF_RECORDING_STOP( "MatrixReverb", 25, 30 );
}

AKRESULT CAkFDNReverbFX::InitDelayLines( AkChannelMask in_uChannelMask )
{
	TermDelayLines();

	if ( m_pParams->NonRTPC.uDelayLengthsMode == AKDELAYLENGTHSMODE_DEFAULT )
		SetDefaultDelayLengths( ); // Ignore delay parameters and use default values

	// Setup DSP function ptr for current audio format
	AKASSERT( (m_pParams->NonRTPC.uNumberOfDelays % 4 == 0) && (m_pParams->NonRTPC.uNumberOfDelays <= 16) );

	AkChannelMask uChannelMask = in_uChannelMask;
	if ( !m_pParams->NonRTPC.uProcessLFE )
		uChannelMask = uChannelMask & ~AK_SPEAKER_LOW_FREQUENCY;
	m_uNumProcessedChannels = AK::GetNumChannels( uChannelMask );

	switch ( uChannelMask )
	{
	case AK_SPEAKER_SETUP_MONO:
		switch ( m_pParams->NonRTPC.uNumberOfDelays )
		{
		case 4:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessMono4;
			break;
		case 8:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessMono8;
			break;
		case 12:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessMono12;
			break;
		case 16:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessMono16;
			break;
		}
		break;
	case AK_SPEAKER_SETUP_STEREO:
		switch ( m_pParams->NonRTPC.uNumberOfDelays )
		{
		case 4:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessStereo4;
			break;
		case 8:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessStereo8;
			break;
		case 12:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessStereo12;
			break;
		case 16:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessStereo16;
			break;
		}
		break;
#ifdef AK_LFECENTER
	case AK_SPEAKER_SETUP_5:
		switch ( m_pParams->NonRTPC.uNumberOfDelays )
		{
		case 4:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointZero4;
			break;
		case 8:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointZero8;
			break;
		case 12:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointZero12;
			break;
		case 16:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointZero16;
			break;
		}
		break;
	case AK_SPEAKER_SETUP_5POINT1:
		switch ( m_pParams->NonRTPC.uNumberOfDelays )
		{
		case 4:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointOne4;
			break;
		case 8:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointOne8;
			break;
		case 12:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointOne12;
			break;
		case 16:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointOne16;
			break;
		}
		break;
#endif
	default:
#ifndef AK_ANDROID	
		switch ( m_pParams->NonRTPC.uNumberOfDelays )
		{
		case 4:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessN4;
			break;
		case 8:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessN8;
			break;
		case 12:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessN12;
			break;
		case 16:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessN16;
			break;
		}
		break;
#else
	AKASSERT(!"Unsupported channel config in CAkFDNReverbFX::InitDelayLines");
#endif // AK_ANDROID

	
	}

#if defined(_DEBUG) && defined(MEMORYOUTPUT)
	AkUInt32 uTotalMemoryAllocated = 0;
#endif

	////////////////////// Allocate and setup pre-delay line ////////////////////

	m_uPreDelayLength = (AkUInt32) (m_pParams->NonRTPC.fPreDelay * m_uSampleRate);
	if ( m_uPreDelayLength > 0 )
	{
		m_pfPreDelayStart = (AkReal32*) AK_PLUGIN_ALLOC( m_pAllocator, sizeof(AkReal32) * m_uPreDelayLength );
		if ( !m_pfPreDelayStart )
			return AK_InsufficientMemory;
		m_pfPreDelayRW = m_pfPreDelayStart;
		m_pfPreDelayEnd = m_pfPreDelayStart + m_uPreDelayLength;
#if defined(_DEBUG) && defined(MEMORYOUTPUT)
		uTotalMemoryAllocated += sizeof(AkReal32) * m_uPreDelayLength;
#endif
	}

	////////////////////// Convert delay line lengths ////////////////////

	// Ensure the values are prime numbers and sorted in increasing order
	for ( AkUInt32 i = 0; i < m_pParams->NonRTPC.uNumberOfDelays; ++i )
	{
		m_uNominalDelayLength[i] = (AkUInt32)((m_pParams->NonRTPC.fDelayTime[i]/1000.f)*m_uSampleRate);
		MakePrimeNumber( m_uNominalDelayLength[i] );
	}
	qsort(m_uNominalDelayLength, m_pParams->NonRTPC.uNumberOfDelays, sizeof(AkUInt32), AkFDNQSortCompare);

	// Print out some useful information to Wwise debug window
#if defined(_DEBUG) && defined(MEMORYOUTPUT)
	AkReal32 fFreqDensity = 0.f;
	AkReal32 fEchoDensity = 0.f;
	for ( AkUInt32 i = 0; i < m_pParams->NonRTPC.uNumberOfDelays; ++i )
	{
		fFreqDensity += m_uNominalDelayLength[i];
		fEchoDensity += (AkReal32)m_uSampleRate/m_uNominalDelayLength[i];
	}
	fFreqDensity /= m_uSampleRate;
	printf( "Matrix Reverb Statistics\n" );
	printf( "Frequency density: %f\n", fFreqDensity );
	printf( "Echo density: %f\n", fEchoDensity );
#endif

	//////////////////// Initialize delay lines ////////////////////

	// 1) Allocate each delay line using total length to account for possible modulation 
	// 2) Setup read and write pointers according to delay lengths
	// Note: Delay lines are interleaved 4x4 to allow substantial memory and CPU optimizations
	for ( AkUInt32 i = 0; i < m_pParams->NonRTPC.uNumberOfDelays/4; ++i )
	{
		AkUInt32 uMaxFramesPerGroup = m_uNominalDelayLength[i*4+3];
		AkUInt32 uInterleavedDelayLength = uMaxFramesPerGroup*4;
		m_pfDelayStart[i] = (AkReal32*) AK_PLUGIN_ALLOC( m_pAllocator, sizeof(AkReal32) * uInterleavedDelayLength );
		if ( !m_pfDelayStart[i] )
			return AK_InsufficientMemory;
		m_pfDelayWrite[i] = m_pfDelayStart[i];
		m_pfDelayEnd[i] = m_pfDelayStart[i] + uInterleavedDelayLength;
		m_pfDelayRead[i*4] = m_pfDelayStart[i] + ((uMaxFramesPerGroup-m_uNominalDelayLength[i*4])*4);
		m_pfDelayRead[i*4+1] = m_pfDelayStart[i] + ((uMaxFramesPerGroup-m_uNominalDelayLength[i*4+1])*4+1);
		m_pfDelayRead[i*4+2] = m_pfDelayStart[i] + ((uMaxFramesPerGroup-m_uNominalDelayLength[i*4+2])*4+2);
		m_pfDelayRead[i*4+3] = m_pfDelayStart[i] + 3;
#if defined(_DEBUG) && defined(MEMORYOUTPUT)
		uTotalMemoryAllocated += sizeof(AkReal32) * uInterleavedDelayLength;
#endif
	}

// Print out total memory allocated to Wwise debug window
#if defined(_DEBUG) && defined(MEMORYOUTPUT)
	printf( "Total allocated memory: %d\n", uTotalMemoryAllocated );
#endif

	Reset();

	m_pParams->NonRTPC.bDirty = false;

	return AK_Success;
}

void CAkFDNReverbFX::TermDelayLines()
{
	for ( AkUInt32 i = 0; i < MAXNUMDELAYGROUPS; ++i )
	{
		if ( m_pfDelayStart[i] )
		{
			AK_PLUGIN_FREE( m_pAllocator, (AkReal32*)m_pfDelayStart[i] );
			m_pfDelayStart[i] = NULL;
		}
	}

	if ( m_pfPreDelayStart )
	{
		AK_PLUGIN_FREE( m_pAllocator, m_pfPreDelayStart );
		m_pfPreDelayStart = NULL;
	}
}

AkForceInline AKSIMD_V4F32 _mm_hadd_ps(AKSIMD_V4F32 i)
{   
	AKSIMD_V4F32 t = AKSIMD_MOVEHL_V4F32(i, i);
	i = AKSIMD_ADD_V4F32(i, t);
	t = AKSIMD_SHUFFLE_V4F32(i, i, 0x55);
	i = AKSIMD_ADD_V4F32(i, t);
	return i;
} 

AkForceInline AKSIMD_V4F32 _mm_rotateleft4_ps( AKSIMD_V4F32 a, AKSIMD_V4F32 b)
{   
	const int iShuffle1 = AKSIMD_SHUFFLE(0,0,3,3);
	const int iShuffle2 = AKSIMD_SHUFFLE(3,0,2,1);
	b = AKSIMD_SHUFFLE_V4F32(a, b, iShuffle1);
	b = AKSIMD_SHUFFLE_V4F32(a, b, iShuffle2);
	return b;
} 

#define GENERICPROCESSSETUP() \
	const AKSIMD_V4F32 vFIRLPFB0 = AKSIMD_LOAD1_V4F32( m_fFIRLPFB0 ); \
	const AKSIMD_V4F32 vFIRLPFB1 = AKSIMD_LOAD1_V4F32( m_fFIRLPFB1 ); \
	const AKSIMD_V4F32 vDCCoefs = AKSIMD_LOAD1_V4F32( m_fDCCoef ); \
	const AkReal32 fFeedbackConstant = -2.f/m_pParams->NonRTPC.uNumberOfDelays; \
	const AKSIMD_V4F32 vFeedbackConstant = AKSIMD_SET_V4F32( fFeedbackConstant ); \
	AkReal32 * AK_RESTRICT pInOut = io_pBuffer->GetChannel(0); \
	AKSIMD_V4F32 vCurDry = AKSIMD_LOAD1_V4F32( m_fCurrentDry ); \
	AKSIMD_V4F32 vCurWet = AKSIMD_LOAD1_V4F32( m_fCurrentWet ); \
	const AkReal32 fDryInc = (m_pParams->RTPC.fDryLevel - m_fCurrentDry) / io_pBuffer->MaxFrames(); \
	const AkReal32 fWetInc = (m_pParams->RTPC.fWetLevel - m_fCurrentWet) / io_pBuffer->MaxFrames(); \
	const AKSIMD_V4F32 vDryInc = AKSIMD_SET_V4F32( fDryInc ); \
	const AKSIMD_V4F32 vWetInc = AKSIMD_SET_V4F32( fWetInc ); \
	AKSIMD_V4F32 vFIRLPFMem = AKSIMD_LOAD1_V4F32( m_fFIRLPFMem ); \
	AKSIMD_V4F32 vDCxn1 = AKSIMD_LOAD1_V4F32( m_fDCFwdMem ); \
	AKSIMD_V4F32 vDCyn1 = AKSIMD_LOAD1_V4F32( m_fDCFbkMem ); \
	const AkReal32 * pfPreDelayStart = m_pfPreDelayStart; \
	AkReal32 * AK_RESTRICT pfPreDelayRW	= m_pfPreDelayRW; \
	const AkReal32 * pfPreDelayEnd = m_pfPreDelayEnd; \
	const bool bPreDelayProcessNeeded = m_pfPreDelayStart != NULL; 

#define DELAY4PROCESSSETUP() \
	const AkReal32* pfDelayStart = m_pfDelayStart[0]; \
	const AkReal32* pfDelayEnd = m_pfDelayEnd[0]; \
	register AkReal32* pfDelayWrite0 = m_pfDelayWrite[0]; \
	register AkReal32* pfDelayRead0 = m_pfDelayRead[0]; \
	register AkReal32* pfDelayRead1 = m_pfDelayRead[1]; \
	register AkReal32* pfDelayRead2 = m_pfDelayRead[2]; \
	register AkReal32* pfDelayRead3 = m_pfDelayRead[3]; \
	const AKSIMD_V4F32 vIIRLPFB0 = m_vIIRLPFB0[0]; \
	const AKSIMD_V4F32 vIIRLPFA1 = m_vIIRLPFA1[0]; \
	register AKSIMD_V4F32 vIIRLPFMem0 = m_vIIRLPFMem[0]; 

#define DELAY8PROCESSSETUP() \
	const AkReal32* pfDelayStart[2] = { m_pfDelayStart[0], m_pfDelayStart[1] }; \
	const AkReal32* pfDelayEnd[2] = { m_pfDelayEnd[0], m_pfDelayEnd[1] }; \
	register AkReal32* pfDelayWrite0 = m_pfDelayWrite[0]; \
	register AkReal32* pfDelayWrite1 = m_pfDelayWrite[1]; \
	register AkReal32* pfDelayRead0 = m_pfDelayRead[0]; \
	register AkReal32* pfDelayRead1 = m_pfDelayRead[1]; \
	register AkReal32* pfDelayRead2 = m_pfDelayRead[2]; \
	register AkReal32* pfDelayRead3 = m_pfDelayRead[3]; \
	register AkReal32* pfDelayRead4 = m_pfDelayRead[4]; \
	register AkReal32* pfDelayRead5 = m_pfDelayRead[5]; \
	register AkReal32* pfDelayRead6 = m_pfDelayRead[6]; \
	register AkReal32* pfDelayRead7 = m_pfDelayRead[7]; \
	const AKSIMD_V4F32 vIIRLPFB0[2] = { m_vIIRLPFB0[0], m_vIIRLPFB0[1] }; \
	const AKSIMD_V4F32 vIIRLPFA1[2] = { m_vIIRLPFA1[0], m_vIIRLPFA1[1] }; \
	register AKSIMD_V4F32 vIIRLPFMem0 = m_vIIRLPFMem[0]; \
	register AKSIMD_V4F32 vIIRLPFMem1 = m_vIIRLPFMem[1]; 

#define DELAY12PROCESSSETUP() \
	const AkReal32* pfDelayStart[3] = { m_pfDelayStart[0], m_pfDelayStart[1], m_pfDelayStart[2] }; \
	const AkReal32* pfDelayEnd[3] = { m_pfDelayEnd[0], m_pfDelayEnd[1], m_pfDelayEnd[2] }; \
	register AkReal32* pfDelayWrite0 = m_pfDelayWrite[0]; \
	register AkReal32* pfDelayWrite1 = m_pfDelayWrite[1]; \
	register AkReal32* pfDelayWrite2 = m_pfDelayWrite[2]; \
	register AkReal32* pfDelayRead0 = m_pfDelayRead[0]; \
	register AkReal32* pfDelayRead1 = m_pfDelayRead[1]; \
	register AkReal32* pfDelayRead2 = m_pfDelayRead[2]; \
	register AkReal32* pfDelayRead3 = m_pfDelayRead[3]; \
	register AkReal32* pfDelayRead4 = m_pfDelayRead[4]; \
	register AkReal32* pfDelayRead5 = m_pfDelayRead[5]; \
	register AkReal32* pfDelayRead6 = m_pfDelayRead[6]; \
	register AkReal32* pfDelayRead7 = m_pfDelayRead[7]; \
	register AkReal32* pfDelayRead8 = m_pfDelayRead[8]; \
	register AkReal32* pfDelayRead9 = m_pfDelayRead[9]; \
	register AkReal32* pfDelayRead10 = m_pfDelayRead[10]; \
	register AkReal32* pfDelayRead11 = m_pfDelayRead[11]; \
	const AKSIMD_V4F32 vIIRLPFB0[3] = { m_vIIRLPFB0[0], m_vIIRLPFB0[1], m_vIIRLPFB0[2] }; \
	const AKSIMD_V4F32 vIIRLPFA1[3] = { m_vIIRLPFA1[0], m_vIIRLPFA1[1], m_vIIRLPFA1[2] }; \
	register AKSIMD_V4F32 vIIRLPFMem0 = m_vIIRLPFMem[0]; \
	register AKSIMD_V4F32 vIIRLPFMem1 = m_vIIRLPFMem[1]; \
	register AKSIMD_V4F32 vIIRLPFMem2 = m_vIIRLPFMem[2]; 

#define DELAY16PROCESSSETUP() \
	const AkReal32* pfDelayStart[4] = { m_pfDelayStart[0], m_pfDelayStart[1], m_pfDelayStart[2], m_pfDelayStart[3] }; \
	const AkReal32* pfDelayEnd[4] = { m_pfDelayEnd[0], m_pfDelayEnd[1], m_pfDelayEnd[2], m_pfDelayEnd[3] }; \
	register AkReal32* pfDelayWrite0 = m_pfDelayWrite[0]; \
	register AkReal32* pfDelayWrite1 = m_pfDelayWrite[1]; \
	register AkReal32* pfDelayWrite2 = m_pfDelayWrite[2]; \
	register AkReal32* pfDelayWrite3 = m_pfDelayWrite[3]; \
	register AkReal32* pfDelayRead0 = m_pfDelayRead[0]; \
	register AkReal32* pfDelayRead1 = m_pfDelayRead[1]; \
	register AkReal32* pfDelayRead2 = m_pfDelayRead[2]; \
	register AkReal32* pfDelayRead3 = m_pfDelayRead[3]; \
	register AkReal32* pfDelayRead4 = m_pfDelayRead[4]; \
	register AkReal32* pfDelayRead5 = m_pfDelayRead[5]; \
	register AkReal32* pfDelayRead6 = m_pfDelayRead[6]; \
	register AkReal32* pfDelayRead7 = m_pfDelayRead[7]; \
	register AkReal32* pfDelayRead8 = m_pfDelayRead[8]; \
	register AkReal32* pfDelayRead9 = m_pfDelayRead[9]; \
	register AkReal32* pfDelayRead10 = m_pfDelayRead[10]; \
	register AkReal32* pfDelayRead11 = m_pfDelayRead[11]; \
	register AkReal32* pfDelayRead12 = m_pfDelayRead[12]; \
	register AkReal32* pfDelayRead13 = m_pfDelayRead[13]; \
	register AkReal32* pfDelayRead14 = m_pfDelayRead[14]; \
	register AkReal32* pfDelayRead15 = m_pfDelayRead[15]; \
	const AKSIMD_V4F32 vIIRLPFB0[4] = { m_vIIRLPFB0[0], m_vIIRLPFB0[1], m_vIIRLPFB0[2], m_vIIRLPFB0[3] }; \
	const AKSIMD_V4F32 vIIRLPFA1[4] = { m_vIIRLPFA1[0], m_vIIRLPFA1[1], m_vIIRLPFA1[2], m_vIIRLPFA1[3] }; \
	register AKSIMD_V4F32 vIIRLPFMem0 = m_vIIRLPFMem[0]; \
	register AKSIMD_V4F32 vIIRLPFMem1 = m_vIIRLPFMem[1]; \
	register AKSIMD_V4F32 vIIRLPFMem2 = m_vIIRLPFMem[2]; \
	register AKSIMD_V4F32 vIIRLPFMem3 = m_vIIRLPFMem[3]; 

#define GENERICPROCESSTEARDOWN() \
	AKSIMD_STORE1_V4F32( &m_fFIRLPFMem, vFIRLPFMem ); \
	AKSIMD_STORE1_V4F32( &m_fDCFwdMem, vDCxn1 );	\
	AKSIMD_STORE1_V4F32( &m_fDCFbkMem , vDCyn1 ); \
	m_pfPreDelayRW = pfPreDelayRW; 

#define DELAY16PROCESSTEARDOWN() \
	m_vIIRLPFMem[0] = vIIRLPFMem0; \
	m_vIIRLPFMem[1] = vIIRLPFMem1; \
	m_vIIRLPFMem[2] = vIIRLPFMem2; \
	m_vIIRLPFMem[3] = vIIRLPFMem3; \
	m_pfDelayWrite[0] = pfDelayWrite0; \
	m_pfDelayWrite[1] = pfDelayWrite1; \
	m_pfDelayWrite[2] = pfDelayWrite2; \
	m_pfDelayWrite[3] = pfDelayWrite3; \
	m_pfDelayRead[0] = pfDelayRead0; \
	m_pfDelayRead[1] = pfDelayRead1; \
	m_pfDelayRead[2] = pfDelayRead2; \
	m_pfDelayRead[3] = pfDelayRead3; \
	m_pfDelayRead[4] = pfDelayRead4; \
	m_pfDelayRead[5] = pfDelayRead5; \
	m_pfDelayRead[6] = pfDelayRead6; \
	m_pfDelayRead[7] = pfDelayRead7; \
	m_pfDelayRead[8] = pfDelayRead8; \
	m_pfDelayRead[9] = pfDelayRead9; \
	m_pfDelayRead[10] = pfDelayRead10; \
	m_pfDelayRead[11] = pfDelayRead11; \
	m_pfDelayRead[12] = pfDelayRead12; \
	m_pfDelayRead[13] = pfDelayRead13; \
	m_pfDelayRead[14] = pfDelayRead14; \
	m_pfDelayRead[15] = pfDelayRead15; 

#define DELAY12PROCESSTEARDOWN() \
	m_vIIRLPFMem[0] = vIIRLPFMem0; \
	m_vIIRLPFMem[1] = vIIRLPFMem1; \
	m_vIIRLPFMem[2] = vIIRLPFMem2; \
	m_pfDelayWrite[0] = pfDelayWrite0; \
	m_pfDelayWrite[1] = pfDelayWrite1; \
	m_pfDelayWrite[2] = pfDelayWrite2; \
	m_pfDelayRead[0] = pfDelayRead0; \
	m_pfDelayRead[1] = pfDelayRead1; \
	m_pfDelayRead[2] = pfDelayRead2; \
	m_pfDelayRead[3] = pfDelayRead3; \
	m_pfDelayRead[4] = pfDelayRead4; \
	m_pfDelayRead[5] = pfDelayRead5; \
	m_pfDelayRead[6] = pfDelayRead6; \
	m_pfDelayRead[7] = pfDelayRead7; \
	m_pfDelayRead[8] = pfDelayRead8; \
	m_pfDelayRead[9] = pfDelayRead9; \
	m_pfDelayRead[10] = pfDelayRead10; \
	m_pfDelayRead[11] = pfDelayRead11; 

#define DELAY8PROCESSTEARDOWN() \
	m_vIIRLPFMem[0] = vIIRLPFMem0; \
	m_vIIRLPFMem[1] = vIIRLPFMem1; \
	m_pfDelayWrite[0] = pfDelayWrite0; \
	m_pfDelayWrite[1] = pfDelayWrite1; \
	m_pfDelayRead[0] = pfDelayRead0; \
	m_pfDelayRead[1] = pfDelayRead1; \
	m_pfDelayRead[2] = pfDelayRead2; \
	m_pfDelayRead[3] = pfDelayRead3; \
	m_pfDelayRead[4] = pfDelayRead4; \
	m_pfDelayRead[5] = pfDelayRead5; \
	m_pfDelayRead[6] = pfDelayRead6; \
	m_pfDelayRead[7] = pfDelayRead7; 

#define DELAY4PROCESSTEARDOWN() \
	m_vIIRLPFMem[0] = vIIRLPFMem0; \
	m_pfDelayWrite[0] = pfDelayWrite0; \
	m_pfDelayRead[0] = pfDelayRead0; \
	m_pfDelayRead[1] = pfDelayRead1; \
	m_pfDelayRead[2] = pfDelayRead2; \
	m_pfDelayRead[3] = pfDelayRead3; 

#define DAMPEDDELAYSGROUP0() \
	AKSIMD_V4F32 vDelayOut0 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead0 ); \
	AKSIMD_V4F32 vDelayOut1 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead1 ); \
	AKSIMD_V4F32 vDelayOut2 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead2 ); \
	AKSIMD_V4F32 vDelayOut3 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead3 ); \
	pfDelayRead0+=4; \
	if ( pfDelayRead0 >= pfDelayEnd[0] ) \
	pfDelayRead0 = (AkReal32*)pfDelayStart[0]; \
	pfDelayRead1+=4; \
	if ( pfDelayRead1 >= pfDelayEnd[0] ) \
	pfDelayRead1 = (AkReal32*)pfDelayStart[0] + 1; \
	pfDelayRead2+=4; \
	if ( pfDelayRead2 >= pfDelayEnd[0] ) \
	pfDelayRead2 = (AkReal32*)pfDelayStart[0] + 2; \
	pfDelayRead3+=4; \
	if ( pfDelayRead3 >= pfDelayEnd[0] ) \
	pfDelayRead3 = (AkReal32*)pfDelayStart[0] + 3; \
	vDelayOut0 = AKSIMD_UNPACKLO_V4F32( vDelayOut0, vDelayOut1 ); \
	vDelayOut1 = AKSIMD_UNPACKLO_V4F32( vDelayOut2, vDelayOut3 ); \
	vDelayOut0 = AKSIMD_MOVELH_V4F32( vDelayOut0, vDelayOut1 );	\
	AKSIMD_V4F32 vFbk = AKSIMD_MUL_V4F32( vIIRLPFA1[0], vIIRLPFMem0 ); \
	AKSIMD_V4F32 vDampedDelayOutputs0 = AKSIMD_MADD_V4F32( vDelayOut0, vIIRLPFB0[0], vFbk ); \
	vIIRLPFMem0 = vDampedDelayOutputs0; 

#define DAMPEDDELAYSGROUP0ALT() \
	AKSIMD_V4F32 vDelayOut0 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead0 ); \
	AKSIMD_V4F32 vDelayOut1 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead1 ); \
	AKSIMD_V4F32 vDelayOut2 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead2 ); \
	AKSIMD_V4F32 vDelayOut3 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead3 ); \
	pfDelayRead0+=4; \
	if ( pfDelayRead0 >= pfDelayEnd ) \
	pfDelayRead0 = (AkReal32*)pfDelayStart; \
	pfDelayRead1+=4; \
	if ( pfDelayRead1 >= pfDelayEnd ) \
	pfDelayRead1 = (AkReal32*)pfDelayStart + 1; \
	pfDelayRead2+=4; \
	if ( pfDelayRead2 >= pfDelayEnd ) \
	pfDelayRead2 = (AkReal32*)pfDelayStart + 2; \
	pfDelayRead3+=4; \
	if ( pfDelayRead3 >= pfDelayEnd ) \
	pfDelayRead3 = (AkReal32*)pfDelayStart + 3; \
	vDelayOut0 = AKSIMD_UNPACKLO_V4F32( vDelayOut0, vDelayOut1 ); \
	vDelayOut1 = AKSIMD_UNPACKLO_V4F32( vDelayOut2, vDelayOut3 ); \
	vDelayOut0 = AKSIMD_MOVELH_V4F32( vDelayOut0, vDelayOut1 );	\
	AKSIMD_V4F32 vFbk = AKSIMD_MUL_V4F32( vIIRLPFA1, vIIRLPFMem0 ); \
	AKSIMD_V4F32 vDampedDelayOutputs0 = AKSIMD_MADD_V4F32( vDelayOut0, vIIRLPFB0, vFbk ); \
	vIIRLPFMem0 = vDampedDelayOutputs0; 

#define DAMPEDDELAYSGROUP1() \
	vDelayOut0 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead4 ); \
	vDelayOut1 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead5 ); \
	vDelayOut2 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead6 ); \
	vDelayOut3 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead7 ); \
	pfDelayRead4+=4; \
	if ( pfDelayRead4 >= pfDelayEnd[1] ) \
	pfDelayRead4 = (AkReal32*)pfDelayStart[1]; \
	pfDelayRead5+=4; \
	if ( pfDelayRead5 >= pfDelayEnd[1] ) \
	pfDelayRead5 = (AkReal32*)pfDelayStart[1] + 1; \
	pfDelayRead6+=4; \
	if ( pfDelayRead6 >= pfDelayEnd[1] ) \
	pfDelayRead6 = (AkReal32*)pfDelayStart[1] + 2; \
	pfDelayRead7+=4; \
	if ( pfDelayRead7 >= pfDelayEnd[1] ) \
	pfDelayRead7 = (AkReal32*)pfDelayStart[1] + 3; \
	vDelayOut0 = AKSIMD_UNPACKLO_V4F32( vDelayOut0, vDelayOut1 ); \
	vDelayOut1 = AKSIMD_UNPACKLO_V4F32( vDelayOut2, vDelayOut3 ); \
	vDelayOut0 = AKSIMD_MOVELH_V4F32( vDelayOut0, vDelayOut1 );	\
	vFbk = AKSIMD_MUL_V4F32( vIIRLPFA1[1], vIIRLPFMem1 ); \
	AKSIMD_V4F32 vDampedDelayOutputs1 = AKSIMD_MADD_V4F32( vDelayOut0, vIIRLPFB0[1], vFbk ); \
	vIIRLPFMem1 = vDampedDelayOutputs1; 

#define DAMPEDDELAYSGROUP2() \
	vDelayOut0 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead8 ); \
	vDelayOut1 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead9 ); \
	vDelayOut2 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead10 ); \
	vDelayOut3 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead11 ); \
	pfDelayRead8+=4; \
	if ( pfDelayRead8 >= pfDelayEnd[2] ) \
	pfDelayRead8 = (AkReal32*)pfDelayStart[2]; \
	pfDelayRead9+=4; \
	if ( pfDelayRead9 >= pfDelayEnd[2] ) \
	pfDelayRead9 = (AkReal32*)pfDelayStart[2] + 1; \
	pfDelayRead10+=4; \
	if ( pfDelayRead10 >= pfDelayEnd[2] ) \
	pfDelayRead10 = (AkReal32*)pfDelayStart[2] + 2; \
	pfDelayRead11+=4; \
	if ( pfDelayRead11 >= pfDelayEnd[2] ) \
	pfDelayRead11 = (AkReal32*)pfDelayStart[2] + 3; \
	vDelayOut0 = AKSIMD_UNPACKLO_V4F32( vDelayOut0, vDelayOut1 ); \
	vDelayOut1 = AKSIMD_UNPACKLO_V4F32( vDelayOut2, vDelayOut3 ); \
	vDelayOut0 = AKSIMD_MOVELH_V4F32( vDelayOut0, vDelayOut1 );	\
	vFbk = AKSIMD_MUL_V4F32( vIIRLPFA1[2], vIIRLPFMem2 ); \
	AKSIMD_V4F32 vDampedDelayOutputs2 = AKSIMD_MADD_V4F32( vDelayOut0, vIIRLPFB0[2], vFbk ); \
	vIIRLPFMem2 = vDampedDelayOutputs2; 

#define DAMPEDDELAYSGROUP3() \
	vDelayOut0 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead12 ); \
	vDelayOut1 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead13 ); \
	vDelayOut2 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead14 ); \
	vDelayOut3 =  AKSIMD_LOAD_SS_V4F32( pfDelayRead15 ); \
	pfDelayRead12+=4; \
	if ( pfDelayRead12 >= pfDelayEnd[3] ) \
	pfDelayRead12 = (AkReal32*)pfDelayStart[3]; \
	pfDelayRead13+=4; \
	if ( pfDelayRead13 >= pfDelayEnd[3] ) \
	pfDelayRead13 = (AkReal32*)pfDelayStart[3] + 1; \
	pfDelayRead14+=4; \
	if ( pfDelayRead14 >= pfDelayEnd[3] ) \
	pfDelayRead14 = (AkReal32*)pfDelayStart[3] + 2; \
	pfDelayRead15+=4; \
	if ( pfDelayRead15 >= pfDelayEnd[3] ) \
	pfDelayRead15 = (AkReal32*)pfDelayStart[3] + 3; \
	vDelayOut0 = AKSIMD_UNPACKLO_V4F32( vDelayOut0, vDelayOut1 ); \
	vDelayOut1 = AKSIMD_UNPACKLO_V4F32( vDelayOut2, vDelayOut3 ); \
	vDelayOut0 = AKSIMD_MOVELH_V4F32( vDelayOut0, vDelayOut1 );	\
	vFbk = AKSIMD_MUL_V4F32( vIIRLPFA1[3], vIIRLPFMem3 ); \
	AKSIMD_V4F32 vDampedDelayOutputs3 = AKSIMD_MADD_V4F32( vDelayOut0, vIIRLPFB0[3], vFbk ); \
	vIIRLPFMem3 = vDampedDelayOutputs3; 

#define DELAY16INJECTION() \
	vInputReinjection0 = AKSIMD_ADD_V4F32( vInputReinjection0, vFIROut ); \
	vInputReinjection1 = AKSIMD_ADD_V4F32( vInputReinjection1, vFIROut ); \
	vInputReinjection2 = AKSIMD_ADD_V4F32( vInputReinjection2, vFIROut ); \
	vInputReinjection3 = AKSIMD_ADD_V4F32( vInputReinjection3, vFIROut ); \
	AKSIMD_STORE_V4F32( pfDelayWrite0, vInputReinjection0 ); \
	AKSIMD_STORE_V4F32( pfDelayWrite1, vInputReinjection1 ); \
	AKSIMD_STORE_V4F32( pfDelayWrite2, vInputReinjection2 ); \
	AKSIMD_STORE_V4F32( pfDelayWrite3, vInputReinjection3 ); \
	pfDelayWrite0+=4; \
	if ( pfDelayWrite0 >= pfDelayEnd[0] ) \
	pfDelayWrite0 = (AkReal32*)pfDelayStart[0]; \
	pfDelayWrite1+=4; \
	if ( pfDelayWrite1 >= pfDelayEnd[1] ) \
	pfDelayWrite1 = (AkReal32*)pfDelayStart[1]; \
	pfDelayWrite2+=4; \
	if ( pfDelayWrite2 >= pfDelayEnd[2] ) \
	pfDelayWrite2 = (AkReal32*)pfDelayStart[2]; \
	pfDelayWrite3+=4; \
	if ( pfDelayWrite3 >= pfDelayEnd[3] ) \
	pfDelayWrite3 = (AkReal32*)pfDelayStart[3]; 

#define DELAY12INJECTION() \
	vInputReinjection0 = AKSIMD_ADD_V4F32( vInputReinjection0, vFIROut ); \
	vInputReinjection1 = AKSIMD_ADD_V4F32( vInputReinjection1, vFIROut ); \
	vInputReinjection2 = AKSIMD_ADD_V4F32( vInputReinjection2, vFIROut ); \
	AKSIMD_STORE_V4F32( pfDelayWrite0, vInputReinjection0 ); \
	AKSIMD_STORE_V4F32( pfDelayWrite1, vInputReinjection1 ); \
	AKSIMD_STORE_V4F32( pfDelayWrite2, vInputReinjection2 ); \
	pfDelayWrite0+=4; \
	if ( pfDelayWrite0 >= pfDelayEnd[0] ) \
	pfDelayWrite0 = (AkReal32*)pfDelayStart[0]; \
	pfDelayWrite1+=4; \
	if ( pfDelayWrite1 >= pfDelayEnd[1] ) \
	pfDelayWrite1 = (AkReal32*)pfDelayStart[1]; \
	pfDelayWrite2+=4; \
	if ( pfDelayWrite2 >= pfDelayEnd[2] ) \
	pfDelayWrite2 = (AkReal32*)pfDelayStart[2]; 

#define DELAY8INJECTION() \
	vInputReinjection0 = AKSIMD_ADD_V4F32( vInputReinjection0, vFIROut ); \
	vInputReinjection1 = AKSIMD_ADD_V4F32( vInputReinjection1, vFIROut ); \
	AKSIMD_STORE_V4F32( pfDelayWrite0, vInputReinjection0 ); \
	AKSIMD_STORE_V4F32( pfDelayWrite1, vInputReinjection1 ); \
	pfDelayWrite0+=4; \
	if ( pfDelayWrite0 >= pfDelayEnd[0] ) \
	pfDelayWrite0 = (AkReal32*)pfDelayStart[0]; \
	pfDelayWrite1+=4; \
	if ( pfDelayWrite1 >= pfDelayEnd[1] ) \
	pfDelayWrite1 = (AkReal32*)pfDelayStart[1]; 

#define DELAY4INJECTION() \
	vInputReinjection0 = AKSIMD_ADD_V4F32( vInputReinjection0, vFIROut ); \
	AKSIMD_STORE_V4F32( pfDelayWrite0, vInputReinjection0 ); \
	pfDelayWrite0+=4; \
	if ( pfDelayWrite0 >= pfDelayEnd ) \
	pfDelayWrite0 = (AkReal32*)pfDelayStart; 

#define SCALEINOUTN(__NUMCHANNELS__) \
	AKSIMD_V4F32 vIn[AK_VOICE_MAX_NUM_CHANNELS];\
	AkUInt32 index = 0;\
	do\
	{\
		vIn[index] = AKSIMD_LOAD_SS_V4F32(&pInOut[index*io_pBuffer->MaxFrames()]); \
		vOut[index] = _mm_hadd_ps( vOut[index] ); \
		++index;\
	}\
	while ( index < __NUMCHANNELS__ );\
	\
	vScaleFactor = _mm_hadd_ps( vScaleFactor ); \
	vScaleFactor = AKSIMD_SHUFFLE_V4F32( vScaleFactor, vScaleFactor, iSplat0); \
	\
	vCurDry = AKSIMD_ADD_SS_V4F32( vCurDry, vDryInc ); \
	vCurWet = AKSIMD_ADD_SS_V4F32( vCurWet, vWetInc ); \
	\
	index = 0;\
	do\
	{\
		vOut[index] = AKSIMD_MADD_SS_V4F32( vIn[index], vCurDry, AKSIMD_MUL_SS_V4F32( vOut[index], vCurWet ) ); \
		AKSIMD_STORE1_V4F32( &pInOut[index*io_pBuffer->MaxFrames()], vOut[index] ); \
		++index;\
	}\
	while ( index < __NUMCHANNELS__ );\
	++pInOut;


#define SCALEINOUTFIVEPOINTONE() \
	AKSIMD_V4F32 vInL = AKSIMD_LOAD_SS_V4F32(&pInOut[LEFTCHANNELOFFSET]); \
	AKSIMD_V4F32 vInR = AKSIMD_LOAD_SS_V4F32(&pInOut[RIGHTCHANNELOFFSET]); \
	AKSIMD_V4F32 vInC = AKSIMD_LOAD_SS_V4F32(&pInOut[CENTERCHANNELOFFSET]); \
	AKSIMD_V4F32 vInLFE = AKSIMD_LOAD_SS_V4F32(&pInOut[LFECHANNELOFFSET]); \
	AKSIMD_V4F32 vInLS = AKSIMD_LOAD_SS_V4F32(&pInOut[LEFTSURROUNDCHANNELOFFSET]); \
	AKSIMD_V4F32 vInRS = AKSIMD_LOAD_SS_V4F32(&pInOut[RIGHTSURROUNDCHANNELOFFSET]); \
	\
	vOutL = _mm_hadd_ps( vOutL ); \
	vOutR = _mm_hadd_ps( vOutR ); \
	vOutC = _mm_hadd_ps( vOutC ); \
	vOutLFE = _mm_hadd_ps( vOutLFE ); \
	vOutLS = _mm_hadd_ps( vOutLS ); \
	vOutRS = _mm_hadd_ps( vOutRS ); \
	vScaleFactor = _mm_hadd_ps( vScaleFactor ); \
	vScaleFactor = AKSIMD_SHUFFLE_V4F32( vScaleFactor, vScaleFactor, iSplat0); \
	\
	vCurDry = AKSIMD_ADD_SS_V4F32( vCurDry, vDryInc ); \
	vCurWet = AKSIMD_ADD_SS_V4F32( vCurWet, vWetInc ); \
	\
	vOutL = AKSIMD_MADD_SS_V4F32( vInL, vCurDry, AKSIMD_MUL_SS_V4F32( vOutL, vCurWet ) ); \
	vOutR = AKSIMD_MADD_SS_V4F32( vInR, vCurDry, AKSIMD_MUL_SS_V4F32( vOutR, vCurWet ) ); \
	vOutC = AKSIMD_MADD_SS_V4F32( vInC, vCurDry, AKSIMD_MUL_SS_V4F32( vOutC, vCurWet ) ); \
	vOutLFE = AKSIMD_MADD_SS_V4F32( vInLFE, vCurDry, AKSIMD_MUL_SS_V4F32( vOutLFE, vCurWet ) ); \
	vOutLS = AKSIMD_MADD_SS_V4F32( vInLS, vCurDry, AKSIMD_MUL_SS_V4F32( vOutLS, vCurWet ) ); \
	vOutRS = AKSIMD_MADD_SS_V4F32( vInRS, vCurDry, AKSIMD_MUL_SS_V4F32( vOutRS, vCurWet ) ); \
	AKSIMD_STORE1_V4F32( &pInOut[LEFTCHANNELOFFSET], vOutL ); \
	AKSIMD_STORE1_V4F32( &pInOut[RIGHTCHANNELOFFSET], vOutR ); \
	AKSIMD_STORE1_V4F32( &pInOut[CENTERCHANNELOFFSET], vOutC ); \
	AKSIMD_STORE1_V4F32( &pInOut[LFECHANNELOFFSET], vOutLFE ); \
	AKSIMD_STORE1_V4F32( &pInOut[LEFTSURROUNDCHANNELOFFSET], vOutLS ); \
	AKSIMD_STORE1_V4F32( &pInOut[RIGHTSURROUNDCHANNELOFFSET], vOutRS ); \
	++pInOut; 

#define SCALEINOUTFIVEPOINTZERO() \
	AKSIMD_V4F32 vInL = AKSIMD_LOAD_SS_V4F32(&pInOut[LEFTCHANNELOFFSET]); \
	AKSIMD_V4F32 vInR = AKSIMD_LOAD_SS_V4F32(&pInOut[RIGHTCHANNELOFFSET]); \
	AKSIMD_V4F32 vInC = AKSIMD_LOAD_SS_V4F32(&pInOut[CENTERCHANNELOFFSET]); \
	AKSIMD_V4F32 vInLS = AKSIMD_LOAD_SS_V4F32(&pInOut[LEFTSURROUNDCHANNELOFFSET]); \
	AKSIMD_V4F32 vInRS = AKSIMD_LOAD_SS_V4F32(&pInOut[RIGHTSURROUNDCHANNELOFFSET]); \
	\
	vOutL = _mm_hadd_ps( vOutL ); \
	vOutR = _mm_hadd_ps( vOutR ); \
	vOutC = _mm_hadd_ps( vOutC ); \
	vOutLS = _mm_hadd_ps( vOutLS ); \
	vOutRS = _mm_hadd_ps( vOutRS ); \
	vScaleFactor = _mm_hadd_ps( vScaleFactor ); \
	vScaleFactor = AKSIMD_SHUFFLE_V4F32( vScaleFactor, vScaleFactor, iSplat0); \
	\
	vCurDry = AKSIMD_ADD_SS_V4F32( vCurDry, vDryInc ); \
	vCurWet = AKSIMD_ADD_SS_V4F32( vCurWet, vWetInc ); \
	\
	vOutL = AKSIMD_MADD_SS_V4F32( vInL, vCurDry, AKSIMD_MUL_SS_V4F32( vOutL, vCurWet ) ); \
	vOutR = AKSIMD_MADD_SS_V4F32( vInR, vCurDry, AKSIMD_MUL_SS_V4F32( vOutR, vCurWet ) ); \
	vOutC = AKSIMD_MADD_SS_V4F32( vInC, vCurDry, AKSIMD_MUL_SS_V4F32( vOutC, vCurWet ) ); \
	vOutLS = AKSIMD_MADD_SS_V4F32( vInLS, vCurDry, AKSIMD_MUL_SS_V4F32( vOutLS, vCurWet ) ); \
	vOutRS = AKSIMD_MADD_SS_V4F32( vInRS, vCurDry, AKSIMD_MUL_SS_V4F32( vOutRS, vCurWet ) ); \
	AKSIMD_STORE1_V4F32( &pInOut[LEFTCHANNELOFFSET], vOutL ); \
	AKSIMD_STORE1_V4F32( &pInOut[RIGHTCHANNELOFFSET], vOutR ); \
	AKSIMD_STORE1_V4F32( &pInOut[CENTERCHANNELOFFSET], vOutC ); \
	AKSIMD_STORE1_V4F32( &pInOut[LEFTSURROUNDCHANNELOFFSET], vOutLS ); \
	AKSIMD_STORE1_V4F32( &pInOut[RIGHTSURROUNDCHANNELOFFSET], vOutRS ); \
	++pInOut; 

#define SCALEINOUTSTEREO() \
	AKSIMD_V4F32 vInL = AKSIMD_LOAD_SS_V4F32(&pInOut[LEFTCHANNELOFFSET]); \
	AKSIMD_V4F32 vInR = AKSIMD_LOAD_SS_V4F32(&pInOut[RIGHTCHANNELOFFSET]); \
	\
	vOutL = _mm_hadd_ps( vOutL ); \
	vOutR = _mm_hadd_ps( vOutR ); \
	vScaleFactor = _mm_hadd_ps( vScaleFactor ); \
	vScaleFactor = AKSIMD_SHUFFLE_V4F32( vScaleFactor, vScaleFactor, iSplat0); \
	\
	vCurDry = AKSIMD_ADD_SS_V4F32( vCurDry, vDryInc ); \
	vCurWet = AKSIMD_ADD_SS_V4F32( vCurWet, vWetInc ); \
	\
	vOutL = AKSIMD_MADD_SS_V4F32( vInL, vCurDry, AKSIMD_MUL_SS_V4F32( vOutL, vCurWet ) ); \
	vOutR = AKSIMD_MADD_SS_V4F32( vInR, vCurDry, AKSIMD_MUL_SS_V4F32( vOutR, vCurWet ) ); \
	AKSIMD_STORE1_V4F32( &pInOut[LEFTCHANNELOFFSET], vOutL ); \
	AKSIMD_STORE1_V4F32( &pInOut[RIGHTCHANNELOFFSET], vOutR ); \
	++pInOut; 

#define SCALEINOUTMONO() \
	AKSIMD_V4F32 vIn = AKSIMD_LOAD_SS_V4F32(pInOut); \
	\
	vOut = _mm_hadd_ps( vOut ); \
	vScaleFactor = _mm_hadd_ps( vScaleFactor ); \
	vScaleFactor = AKSIMD_SHUFFLE_V4F32( vScaleFactor, vScaleFactor, iSplat0); \
	\
	vCurDry = AKSIMD_ADD_SS_V4F32( vCurDry, vDryInc ); \
	vCurWet = AKSIMD_ADD_SS_V4F32( vCurWet, vWetInc ); \
	\
	vOut = AKSIMD_MADD_SS_V4F32( vIn, vCurDry, AKSIMD_MUL_SS_V4F32( vOut, vCurWet ) ); \
	AKSIMD_STORE1_V4F32( pInOut, vOut ); \
	++pInOut; 

#define COMPUTEFEEDBACK16() \
	vScaleFactor = AKSIMD_MUL_V4F32( vScaleFactor, vFeedbackConstant ); \
	AKSIMD_V4F32 vPreviousVec = AKSIMD_ADD_V4F32( vDampedDelayOutputs0, vScaleFactor ); \
	AKSIMD_V4F32 vCurrentVec = AKSIMD_ADD_V4F32( vDampedDelayOutputs3, vScaleFactor ); \
	AKSIMD_V4F32 vInputReinjection3 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = AKSIMD_ADD_V4F32( vDampedDelayOutputs2, vScaleFactor ); \
	AKSIMD_V4F32 vInputReinjection2 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = AKSIMD_ADD_V4F32( vDampedDelayOutputs1, vScaleFactor ); \
	AKSIMD_V4F32 vInputReinjection1 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = AKSIMD_ADD_V4F32( vDampedDelayOutputs0, vScaleFactor ); \
	AKSIMD_V4F32 vInputReinjection0 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); 

#define COMPUTEFEEDBACK12() \
	vScaleFactor = AKSIMD_MUL_V4F32( vScaleFactor, vFeedbackConstant ); \
	AKSIMD_V4F32 vPreviousVec = AKSIMD_ADD_V4F32( vDampedDelayOutputs0, vScaleFactor ); \
	AKSIMD_V4F32 vCurrentVec = AKSIMD_ADD_V4F32( vDampedDelayOutputs2, vScaleFactor ); \
	AKSIMD_V4F32 vInputReinjection2 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = AKSIMD_ADD_V4F32( vDampedDelayOutputs1, vScaleFactor ); \
	AKSIMD_V4F32 vInputReinjection1 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = AKSIMD_ADD_V4F32( vDampedDelayOutputs0, vScaleFactor ); \
	AKSIMD_V4F32 vInputReinjection0 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); 

#define COMPUTEFEEDBACK8() \
	vScaleFactor = AKSIMD_MUL_V4F32( vScaleFactor, vFeedbackConstant ); \
	AKSIMD_V4F32 vPreviousVec = AKSIMD_ADD_V4F32( vDampedDelayOutputs0, vScaleFactor ); \
	AKSIMD_V4F32 vCurrentVec = AKSIMD_ADD_V4F32( vDampedDelayOutputs1, vScaleFactor ); \
	AKSIMD_V4F32 vInputReinjection1 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); \
	AKSIMD_V4F32 vInputReinjection0 = _mm_rotateleft4_ps( vPreviousVec, vCurrentVec ); 

#define COMPUTEFEEDBACK4() \
	vScaleFactor = AKSIMD_MUL_V4F32( vScaleFactor, vFeedbackConstant ); \
	AKSIMD_V4F32 vPreviousVec = AKSIMD_ADD_V4F32( vDampedDelayOutputs0, vScaleFactor ); \
	AKSIMD_V4F32 vInputReinjection0 = _mm_rotateleft4_ps( vPreviousVec, vPreviousVec ); 

#define PROCESSPREDELAY() \
	AKSIMD_V4F32 vPreDelayOut; \
	if ( bPreDelayProcessNeeded ) \
		{ \
		vPreDelayOut = AKSIMD_LOAD_SS_V4F32( pfPreDelayRW ); \
		AKSIMD_STORE1_V4F32( pfPreDelayRW, vDCOut ); \
		++pfPreDelayRW; \
		if ( pfPreDelayRW == pfPreDelayEnd ) \
		pfPreDelayRW = (AkReal32*) pfPreDelayStart; \
		} \
	else \
	{ \
		vPreDelayOut = vDCOut; \
	} 

#define PROCESSTONECORRECTIONFILTER() \
	AKSIMD_V4F32 vFIROut = AKSIMD_MADD_SS_V4F32( vFIRLPFB0, vPreDelayOut, AKSIMD_MUL_SS_V4F32( vFIRLPFB1, vFIRLPFMem ) ); \
	vFIRLPFMem = vPreDelayOut; \
	vFIROut = AKSIMD_SHUFFLE_V4F32( vFIROut, vFIROut, iSplat0); 

#define PROCESSDCFILTER(__INPUT__) \
	AKSIMD_V4F32 vDCOut = AKSIMD_SUB_SS_V4F32( AKSIMD_MADD_SS_V4F32( vDCCoefs, vDCyn1, __INPUT__ ), vDCxn1 ); \
	vDCxn1 = __INPUT__; \
	vDCyn1 = vDCOut; 

#define MIXFIVEPOINTZERO() \
	AKSIMD_V4F32 vTmp = AKSIMD_ADD_SS_V4F32( vInL, vInR ); \
	AKSIMD_V4F32 vMixIn = AKSIMD_ADD_SS_V4F32( vInC, vInLS ); \
	vTmp = AKSIMD_ADD_SS_V4F32( vTmp, vInRS ); \
	vMixIn = AKSIMD_ADD_SS_V4F32( vMixIn, vTmp ); 

#define MIXFIVEPOINTONE() \
	AKSIMD_V4F32 vTmp = AKSIMD_ADD_SS_V4F32( vInL, vInR ); \
	AKSIMD_V4F32 vMixIn = AKSIMD_ADD_SS_V4F32( vInC, vInLFE ); \
	vTmp = AKSIMD_ADD_SS_V4F32( vTmp, vInLS ); \
	vMixIn = AKSIMD_ADD_SS_V4F32( vMixIn, vInRS ); \
	vMixIn = AKSIMD_ADD_SS_V4F32( vMixIn, vTmp ); 
		
#define MIXN(__NUMCHANNELS__) \
	AKSIMD_V4F32 vMixIn = vIn[0];\
	index = 1;\
	while ( index < __NUMCHANNELS__ ) \
	{\
		vMixIn = AKSIMD_ADD_SS_V4F32( vMixIn, vIn[index] ); \
		++index;\
	}

// Original
void CAkFDNReverbFX::ProcessMono4( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY4PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0ALT();
		AKSIMD_V4F32 vOut = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		SCALEINOUTMONO();
		COMPUTEFEEDBACK4();
		PROCESSDCFILTER( vIn );
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY4INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY4PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessMono8( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY8PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AKSIMD_V4F32 vOut = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOut = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOut );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );
		
		SCALEINOUTMONO();
		COMPUTEFEEDBACK8();
		PROCESSDCFILTER( vIn );
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY8INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY8PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessMono12( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY12PROCESSSETUP();
	
	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AKSIMD_V4F32 vOut = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOut = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOut );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOut = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOut );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs2 );

		SCALEINOUTMONO();
		COMPUTEFEEDBACK12();
		PROCESSDCFILTER( vIn );	
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY12INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY12PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessMono16( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY16PROCESSSETUP();
	
	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AKSIMD_V4F32 vOut = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOut = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOut );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOut = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOut );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		vOut = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs3, vOut );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs3 );

		SCALEINOUTMONO();
		COMPUTEFEEDBACK16();
		PROCESSDCFILTER( vIn );
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY16INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY16PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessStereo4( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY4PROCESSSETUP();
	
	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0ALT();
		AKSIMD_V4F32 vOutL = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutR = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		SCALEINOUTSTEREO();
		COMPUTEFEEDBACK4();
		AKSIMD_V4F32 vMixIn = AKSIMD_ADD_SS_V4F32( vInL, vInR );	
		PROCESSDCFILTER( vMixIn );
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY4INJECTION();
	
		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY4PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessStereo8( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY8PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AKSIMD_V4F32 vOutL = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutR = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );

		SCALEINOUTSTEREO();
		COMPUTEFEEDBACK8();
		AKSIMD_V4F32 vMixIn = AKSIMD_ADD_SS_V4F32( vInL, vInR );
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY8INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY8PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessStereo12( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY12PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AKSIMD_V4F32 vOutL = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutR = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs2 );

		SCALEINOUTSTEREO();
		COMPUTEFEEDBACK12();
		AKSIMD_V4F32 vMixIn = AKSIMD_ADD_SS_V4F32( vInL, vInR );
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY12INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();

	DELAY12PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessStereo16( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY16PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AKSIMD_V4F32 vOutL = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutR = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs3, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutR );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs3 );

		SCALEINOUTSTEREO();
		COMPUTEFEEDBACK16();
		AKSIMD_V4F32 vMixIn = AKSIMD_ADD_SS_V4F32( vInL, vInR );
		PROCESSDCFILTER( vMixIn );			
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY16INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY16PROCESSTEARDOWN();
}

#ifdef AK_LFECENTER
void CAkFDNReverbFX::ProcessFivePointZero4( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY4PROCESSSETUP();
	
	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0ALT();
		AKSIMD_V4F32 vOutL = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutR = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutC = AKSIMD_MUL_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutLS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutRS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		SCALEINOUTFIVEPOINTZERO();
		COMPUTEFEEDBACK4();
		MIXFIVEPOINTZERO();
		PROCESSDCFILTER( vMixIn );	
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY4INJECTION();
	
		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY4PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointZero8( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY8PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AKSIMD_V4F32 vOutL = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutR = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutC = AKSIMD_MUL_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutLS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutRS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = AKSIMD_MADD_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLS = AKSIMD_MADD_V4F32( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );
	
		SCALEINOUTFIVEPOINTZERO();
		COMPUTEFEEDBACK8();
		MIXFIVEPOINTZERO();
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY8INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY8PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointZero12( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY12PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AKSIMD_V4F32 vOutL = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutR = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutC = AKSIMD_MUL_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutLS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutRS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = AKSIMD_MADD_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLS = AKSIMD_MADD_V4F32( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vOutC = AKSIMD_MADD_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs2, vOutC );
		vOutLS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs2, vOutLS );
		vOutRS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorC, vMinusOne ), vDampedDelayOutputs2, vOutRS );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs2 );

		SCALEINOUTFIVEPOINTZERO();
		COMPUTEFEEDBACK12();
		MIXFIVEPOINTZERO();
		PROCESSDCFILTER( vMixIn );			
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY12INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY12PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointZero16( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY16PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();		
		AKSIMD_V4F32 vOutL = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutR = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutC = AKSIMD_MUL_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutLS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutRS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = AKSIMD_MADD_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLS = AKSIMD_MADD_V4F32( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vOutC = AKSIMD_MADD_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs2, vOutC );
		vOutLS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs2, vOutLS );
		vOutRS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorC, vMinusOne ), vDampedDelayOutputs2, vOutRS );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs3, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutR );
		vOutC = AKSIMD_MADD_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs3, vOutC );
		vOutLS = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutLS );
		vOutRS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vMinusOne ), vDampedDelayOutputs3, vOutRS );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs3 );

		SCALEINOUTFIVEPOINTZERO();
		COMPUTEFEEDBACK16();
		MIXFIVEPOINTZERO();
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY16INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY16PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointOne4( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY4PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0ALT();
		AKSIMD_V4F32 vOutL = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutR = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutC = AKSIMD_MUL_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutLFE = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutLS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutRS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;
		
		SCALEINOUTFIVEPOINTONE();
		COMPUTEFEEDBACK4();
		MIXFIVEPOINTONE();
		PROCESSDCFILTER( vMixIn );	
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY4INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY4PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointOne8( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY8PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AKSIMD_V4F32 vOutL = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutR = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutC = AKSIMD_MUL_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutLFE = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutLS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutRS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = AKSIMD_MADD_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLFE = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs1, vOutLFE );
		vOutLS = AKSIMD_MADD_V4F32( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );
	
		SCALEINOUTFIVEPOINTONE();
		COMPUTEFEEDBACK8();
		MIXFIVEPOINTONE();
		PROCESSDCFILTER( vMixIn );	
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY8INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY8PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointOne12( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY12PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AKSIMD_V4F32 vOutL = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutR = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutC = AKSIMD_MUL_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutLFE = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutLS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutRS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = AKSIMD_MADD_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLFE = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs1, vOutLFE );
		vOutLS = AKSIMD_MADD_V4F32( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vOutC = AKSIMD_MADD_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs2, vOutC );
		vOutLFE = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutLFE );
		vOutLS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs2, vOutLS );
		vOutRS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorC, vMinusOne ), vDampedDelayOutputs2, vOutRS );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs2 );
		
		SCALEINOUTFIVEPOINTONE();
		COMPUTEFEEDBACK12();
		MIXFIVEPOINTONE();
		PROCESSDCFILTER( vMixIn );	
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY12INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY12PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointOne16( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY16PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AKSIMD_V4F32 vOutL = AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutR = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutC = AKSIMD_MUL_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutLFE = AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutLS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vOutRS = AKSIMD_MUL_V4F32( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = AKSIMD_MADD_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLFE = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs1, vOutLFE );
		vOutLS = AKSIMD_MADD_V4F32( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vOutC = AKSIMD_MADD_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs2, vOutC );
		vOutLFE = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutLFE );
		vOutLS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs2, vOutLS );
		vOutRS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorC, vMinusOne ), vDampedDelayOutputs2, vOutRS );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		vOutL = AKSIMD_MADD_V4F32( vOutDecorrelationVectorA, vDampedDelayOutputs3, vOutL );
		vOutR = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutR );
		vOutC = AKSIMD_MADD_V4F32( vOutDecorrelationVectorC, vDampedDelayOutputs3, vOutC );
		vOutLFE = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs3, vOutLFE );
		vOutLS = AKSIMD_MADD_V4F32( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutLS );
		vOutRS = AKSIMD_MADD_V4F32( AKSIMD_MUL_V4F32( vOutDecorrelationVectorA, vMinusOne ), vDampedDelayOutputs3, vOutRS );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs3 );

		SCALEINOUTFIVEPOINTONE();
		COMPUTEFEEDBACK16();
		MIXFIVEPOINTONE();
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY16INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY16PROCESSTEARDOWN();
}
#endif //AK_LFECENTER

#ifndef AK_ANDROID
void CAkFDNReverbFX::ProcessN4( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY4PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	const AkUInt32 uNumProcessedChannels = m_uNumProcessedChannels;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0ALT();
		AkUInt32 i = 0;
		AKSIMD_V4F32 vOut[AK_VOICE_MAX_NUM_CHANNELS];
		do 
		{
			vOut[i] = AKSIMD_MUL_V4F32( vOutDecorrelationVector[i][0], vDampedDelayOutputs0 );
			++i;
		}
		while ( i < uNumProcessedChannels );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		SCALEINOUTN( uNumProcessedChannels );
		COMPUTEFEEDBACK4();
		MIXN( uNumProcessedChannels );
		PROCESSDCFILTER( vMixIn );	
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY4INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY4PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessN8( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY8PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	const AkUInt32 uNumProcessedChannels = m_uNumProcessedChannels;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AkUInt32 i = 0;
		AKSIMD_V4F32 vOut[AK_VOICE_MAX_NUM_CHANNELS];
		do 
		{
			vOut[i] = AKSIMD_MUL_V4F32( vOutDecorrelationVector[i][0], vDampedDelayOutputs0 );
			++i;
		}
		while ( i < uNumProcessedChannels );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		i = 0;
		do 
		{
			vOut[i] = AKSIMD_MADD_V4F32( vOutDecorrelationVector[i][1], vDampedDelayOutputs1, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );

		SCALEINOUTN( uNumProcessedChannels );
		COMPUTEFEEDBACK8();
		MIXN( uNumProcessedChannels );
		PROCESSDCFILTER( vMixIn );	
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY8INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY8PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessN12( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY12PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	const AkUInt32 uNumProcessedChannels = m_uNumProcessedChannels;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AkUInt32 i = 0;
		AKSIMD_V4F32 vOut[AK_VOICE_MAX_NUM_CHANNELS];
		do 
		{
			vOut[i] = AKSIMD_MUL_V4F32( vOutDecorrelationVector[i][0], vDampedDelayOutputs0 );
			++i;
		}
		while ( i < uNumProcessedChannels );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		i = 0;
		do 
		{
			vOut[i] = AKSIMD_MADD_V4F32( vOutDecorrelationVector[i][1], vDampedDelayOutputs1, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		i = 0;
		do 
		{
			vOut[i] = AKSIMD_MADD_V4F32( vOutDecorrelationVector[i][2], vDampedDelayOutputs2, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs2 );

		SCALEINOUTN( uNumProcessedChannels );
		COMPUTEFEEDBACK12();
		MIXN( uNumProcessedChannels );
		PROCESSDCFILTER( vMixIn );	
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY12INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY12PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessN16( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY16PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	const AkUInt32 uNumProcessedChannels = m_uNumProcessedChannels;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AkUInt32 i = 0;
		AKSIMD_V4F32 vOut[AK_VOICE_MAX_NUM_CHANNELS];
		do 
		{
			vOut[i] = AKSIMD_MUL_V4F32( vOutDecorrelationVector[i][0], vDampedDelayOutputs0 );
			++i;
		}
		while ( i < uNumProcessedChannels );
		AKSIMD_V4F32 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		i = 0;
		do 
		{
			vOut[i] = AKSIMD_MADD_V4F32( vOutDecorrelationVector[i][1], vDampedDelayOutputs1, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		i = 0;
		do 
		{
			vOut[i] = AKSIMD_MADD_V4F32( vOutDecorrelationVector[i][2], vDampedDelayOutputs2, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		i = 0;
		do 
		{
			vOut[i] = AKSIMD_MADD_V4F32( vOutDecorrelationVector[i][3], vDampedDelayOutputs3, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = AKSIMD_ADD_V4F32( vScaleFactor, vDampedDelayOutputs3 );

		SCALEINOUTN( uNumProcessedChannels );
		COMPUTEFEEDBACK16();
		MIXN( uNumProcessedChannels );
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY16INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY16PROCESSTEARDOWN();
}
#endif // AK_ANDROID

//////////////////// Feedback LPF damping coefficients initialization ////////////////////
void CAkFDNReverbFX::ComputeIIRLPFCoefs( )
{
	// Note: Coefficients are computed as double but run-time computations are float
	AkReal64 fSamplingPeriod = 1.0 / (AkReal64) m_uSampleRate;
	AkReal64 fReverbTimeRatio = 1.0 / (AkReal64) m_pParams->RTPC.fHFRatio;
	AkReal64 fReverbTimeRatioScale = 1.0 - (1.0/(fReverbTimeRatio*fReverbTimeRatio));

	// Note: For larger delay length values with small reverb time and high HFRatio, the system may become unstable.
	// Fix: Look for those potential instabilities and update fReverbTimeRatioScale (dependent on HFRatio) untill it leads
	// to a stable system. The effective HFRatio may not be exactly the same as the user selected. It needs to be the same
	// for all filter computations.
	AkUInt32 uMaxDelayTime = m_uNominalDelayLength[m_pParams->NonRTPC.uNumberOfDelays-1];
	// Compute A1 coefficient for maximum delay length (worst case)
	AkReal64 fDelayTimeWC = uMaxDelayTime * fSamplingPeriod;
	AkReal64 fB0WC = pow( 0.001, fDelayTimeWC / (AkReal64) m_pParams->RTPC.fReverbTime );
	AkReal64 fScaleFactorWC = log10(fB0WC)*IIRCOEFCALCCONST;
	AkReal64 fA1WC = (fScaleFactorWC*fReverbTimeRatioScale);
	if ( fA1WC > 1.0 )
	{
		// Compute new fReverbTimeRatioScale for worst case scenario to avoid unstability
		fReverbTimeRatioScale = 1.0 / fScaleFactorWC;
	}

	for ( AkUInt32 i = 0; i < m_pParams->NonRTPC.uNumberOfDelays; ++i )
	{
		AkReal64 fDelayTime = m_uNominalDelayLength[i] * fSamplingPeriod;
		AkReal64 fB0 = pow( 0.001, fDelayTime / (AkReal64) m_pParams->RTPC.fReverbTime );
		AKASSERT( fB0 >= 0.f && fB0 < 1.f );
		AkReal64 fA1 = (log10(fB0)*IIRCOEFCALCCONST*fReverbTimeRatioScale);
		AKSIMD_GETELEMENT_V4F32( m_vIIRLPFB0[i/4], i%4 ) = (AkReal32)( fB0*(1.0-fA1) );
		AKSIMD_GETELEMENT_V4F32( m_vIIRLPFA1[i/4], i%4 ) = (AkReal32)fA1;
	}
}

//////////////////// Tone correction LPF intialization ////////////////////
void CAkFDNReverbFX::ComputeFIRLPFCoefs( )
{
	AkReal64 fReverbTimeRatio = 1.0 / (AkReal64) m_pParams->RTPC.fHFRatio;
	AkReal64 fBeta = (1.0 - fReverbTimeRatio)/(1.0 + fReverbTimeRatio);
	AkReal64 fOneMinusBeta = (1 - fBeta);
	m_fFIRLPFB0 = (AkReal32) ( 1.0 / fOneMinusBeta );
	m_fFIRLPFB1 = (AkReal32) ( -fBeta / fOneMinusBeta );
}


//////////// Change an input integer value into its next prime value /////
void CAkFDNReverbFX::MakePrimeNumber( AkUInt32 & in_uIn )
{
	// First ensure its odd
	if ( (in_uIn & 1) == 0) 
		in_uIn++;	

	// Only need to compute up to square root (math theorem)
	AkInt32 iStop = (AkInt32) sqrt((AkReal64) in_uIn) + 1; 
	while ( true ) 
	{
		bool bFoundDivisor = false;
		for (AkInt32 i = 3; i < iStop; i+=2 )
		{
			if ( (in_uIn % i) == 0) 
			{
				// Can be divided by some number so not prime
				bFoundDivisor = true;
				break;
			}
		}

		if (!bFoundDivisor)
			break;		// Could not find dividors so its a prime number
		in_uIn += 2;	// Otherwise try the next odd number
	}
}

void CAkFDNReverbFX::SetDefaultDelayLengths( )
{
	for ( AkUInt32 i = 0; i < m_pParams->NonRTPC.uNumberOfDelays; ++i )
	{
		m_pParams->NonRTPC.fDelayTime[i] = g_fDefaultDelayLengths[i]; 
	}
}

