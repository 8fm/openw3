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
// AkResampler.cpp
// 
// Combines software resampling and pitch shifting opreation in one algorithm 
// using linear interpolation.
// Assumes same thread will call both SetPitch and Execute (not locking).
// There is some interpolation done on the pitch control parameter do avoid stepping behavior in transition
// or fast parameter changes.
// 
// We can think of latency / stepping problem by looking at rates at which different thinks occur:
// Control rate: rate at which SetPitch are called -> 1x per buffer (linked with buffer time)
// Interpolation rate: rate at which transitional pitch values are changed 
// -> NUMBLOCKTOREACHTARGET per buffer, necessary to avoid stepping while introduces up to 1 buffer latency
// Audio rate: rate at which samples are calculated == sample rate
// Simplifying assumption -> if its bypassed, its bypassed for the whole buffer
// It is possible to run the pitch algorithm with pitch 0.
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h" 
#include "AkResampler.h"
#include "AkResamplerCommon.h"
#include <math.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkCommon.h"
#include "AkRuntimeEnvironmentMgr.h"
#include "AkSettings.h"
#ifdef PERFORMANCE_BENCHMARK
#include "AkMonitor.h"
#endif

#define AK_LE_MIN_PITCHSHIFTCENTS (-2400.f)
#define AK_LE_MAX_PITCHSHIFTCENTS (2400.f)

#ifdef AK_PS3

#include "AkLEngine.h"

extern char _binary_Bypass_I16_spu_bin_start[];
extern char _binary_Bypass_I16_spu_bin_size[];
extern char _binary_Bypass_Native_spu_bin_start[];
extern char _binary_Bypass_Native_spu_bin_size[];
extern char _binary_Fixed_I16_spu_bin_start[];
extern char _binary_Fixed_I16_spu_bin_size[];
extern char _binary_Fixed_Native_spu_bin_start[];
extern char _binary_Fixed_Native_spu_bin_size[];
extern char _binary_Interpolating_I16_spu_bin_start[];
extern char _binary_Interpolating_I16_spu_bin_size[];
extern char _binary_Interpolating_Native_spu_bin_start[];
extern char _binary_Interpolating_Native_spu_bin_size[];

static AK::MultiCoreServices::BinData JobBinInfo[NumPitchOperatingMode][NumInputDataType/4] = 
{
	{
		{ _binary_Bypass_I16_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Bypass_I16_spu_bin_size ) },
		{ _binary_Bypass_Native_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Bypass_Native_spu_bin_size ) },
	},
	{
		{ _binary_Fixed_I16_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Fixed_I16_spu_bin_size ) },
		{ _binary_Fixed_Native_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Fixed_Native_spu_bin_size ) },
	},
	{
		{ _binary_Interpolating_I16_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Interpolating_I16_spu_bin_size ) },
		{ _binary_Interpolating_Native_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Interpolating_Native_spu_bin_size ) },
	}
};

#else

typedef AKRESULT (*PitchDSPFuncPtr) (	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedFrames,
										AkInternalPitchState * io_pPitchState );

static PitchDSPFuncPtr PitchDSPFuncTable[NumPitchOperatingMode][NumInputDataType] = 
{
	{
		//Note: 1 channel cases handled by N routine, stereo is faster with deinterleaving stage built-in
#if defined ( AK_CPU_X86_64 ) || defined( AK_CPU_ARM_NEON )  || defined( AK_PS4 )
		Bypass_I16_NChanVecSSE2,
		Bypass_I16_2ChanSSE2,	
		Bypass_I16_NChanVecSSE2,
		Bypass_I16_NChanVecSSE2,
#elif defined( AK_IOS )
		Bypass_I16_1ChanCoreAudio,
		Bypass_I16_NChan,	
		Bypass_I16_NChan,
		Bypass_I16_NChan,
#elif defined( AK_ANDROID )
		Bypass_I16_NChan,
		Bypass_I16_NChan,	
		Bypass_I16_NChan,
		Bypass_I16_NChan,
#elif defined( AK_WIIU_SOFTWARE )
		Bypass_I16_NChanWiiU,
		Bypass_I16_2ChanWiiU,	
		Bypass_I16_NChanWiiU,
		Bypass_I16_NChanWiiU,
#elif defined( AK_LINUX )
		Bypass_I16_NChan,
		Bypass_I16_NChan,
		Bypass_I16_NChan,
		Bypass_I16_NChan,
		
#else		
		Bypass_I16_NChanVec,
		Bypass_I16_2Chan,	
		Bypass_I16_NChanVec,
		Bypass_I16_NChanVec,
#endif		
		// Note: 1Chan and 2 channel cases handled by N routine
		Bypass_Native_NChan,
		Bypass_Native_NChan,
		Bypass_Native_NChan,
		Bypass_Native_NChan,
	},
#if defined AK_WIN || defined ( AK_APPLE ) || defined ( AK_VITA ) || defined ( AK_ANDROID ) || defined( AK_NACL ) || defined AK_XBOXONE || defined AK_PS4 || defined( AK_LINUX )
	{
		Fixed_I16_1Chan,
		Fixed_I16_2Chan,
		Fixed_I16_NChan,
		Fixed_I16_NChan,
		Fixed_Native_1Chan,
		Fixed_Native_2Chan,
		Fixed_Native_NChan,
		Fixed_Native_NChan,
	},
	{
		Interpolating_I16_1Chan,
		Interpolating_I16_2Chan,
		Interpolating_I16_NChan,
		Interpolating_I16_NChan,
		Interpolating_Native_1Chan,
		Interpolating_Native_2Chan,
		Interpolating_Native_NChan,
		Interpolating_Native_NChan,
	}
#elif defined (AK_WIIU_SOFTWARE)
	{
		Fixed_I16_1ChanWiiU,	/// Fixed_I16_1ChanWiiU_2,
		Fixed_I16_2ChanWiiU,
		Fixed_I16_NChan,
		Fixed_I16_NChan,
		Fixed_Native_1Chan,
		Fixed_Native_2Chan,
		Fixed_Native_NChan,
		Fixed_Native_NChan,
	},	
	{
		Interpolating_I16_1ChanWiiU,
		Interpolating_I16_2ChanWiiU,
		Interpolating_I16_NChan,
		Interpolating_I16_NChan,
		Interpolating_Native_1Chan,
		Interpolating_Native_2Chan,
		Interpolating_Native_NChan,
		Interpolating_Native_NChan,
	}
#else
	{
		Fixed_I16_1Chan,
		Fixed_I16_2Chan,
		Fixed_I16_1To4ChanVec,
		Fixed_I16_5To8ChanVec,
		Fixed_Native_1Chan, 
		Fixed_Native_2Chan, 
		Fixed_Native_NChan_Vec,
		Fixed_Native_NChan_Vec,
	},
	{
		Interpolating_I16_1Chan,
		Interpolating_I16_2Chan,
		Interpolating_I16_1To4ChanVec,
		Interpolating_I16_5To8ChanVec,
		Interpolating_Native_1Chan,
		Interpolating_Native_2Chan,
		Interpolating_Native_NChan_Vec,
		Interpolating_Native_NChan_Vec,
	}
#endif


};
#if defined(AK_CPU_X86) && !defined(AK_IOS)
void CAkResampler::InitDSPFunctTable()
{
	if (AK::AkRuntimeEnvironmentMgr::Instance()->GetSIMDSupport(AK::AK_SIMD_SSE2))
	{
		PitchDSPFuncTable[PitchOperatingMode_Bypass][I16_1Chan] = Bypass_I16_NChanVecSSE2;
		PitchDSPFuncTable[PitchOperatingMode_Bypass][I16_2Chan] = Bypass_I16_2ChanSSE2;
		PitchDSPFuncTable[PitchOperatingMode_Bypass][I16_1To4Chan] = Bypass_I16_NChanVecSSE2;
		PitchDSPFuncTable[PitchOperatingMode_Bypass][I16_5To8Chan] = Bypass_I16_NChanVecSSE2;
	}
}
#endif

#endif


// Constructor
CAkResampler::CAkResampler( )
{
#if defined(AK_IOS) && !defined(AK_CPU_ARM_NEON)	
	m_InternalPitchState.m_pAudioConverter = NULL;
#endif
}

// Destructor
CAkResampler::~CAkResampler( )
{

}

// Frame skip includes resampling ratio frameskip = 1 / (pitchratio * (target sr)/(src sr))
// 1 / pitch ratio = frameskip / ( (src sr) / (target sr) )
AkReal32 CAkResampler::GetLastRate() 
{ 
	return ((AkReal32)m_InternalPitchState.uCurrentFrameSkip / FPMUL ) / m_fSampleRateConvertRatio;
}

// Pass on internal pitch state (must be called after Init)
void CAkResampler::SetLastValues( AkReal32 * in_pfLastValues )
{
	if  ( ISI16TYPE( m_DSPFunctionIndex ) )
	{
		for ( AkUInt32 i = 0; i < m_uNumChannels; ++i )
		{
			m_InternalPitchState.iLastValue[i] = FLOAT_TO_INT16( in_pfLastValues[i] );
		}
	}
	else if  ( ISNATIVETYPE( m_DSPFunctionIndex ) )
	{
		for ( AkUInt32 i = 0; i < m_uNumChannels; ++i )
		{
			m_InternalPitchState.fLastValue[i] = in_pfLastValues[i];
		}
	}
	else
	{
		AKASSERT( !"Unsupported format." );
	}
}

// Retrieve internal pitch state
void CAkResampler::GetLastValues( AkReal32 * out_pfLastValues )
{
	if  ( ISI16TYPE( m_DSPFunctionIndex ) )
	{
		for ( AkUInt32 i = 0; i < m_uNumChannels; ++i )
		{
			out_pfLastValues[i] = INT16_TO_FLOAT( m_InternalPitchState.iLastValue[i] );
		}
	}
	else if  ( ISNATIVETYPE( m_DSPFunctionIndex ) )
	{
		for ( AkUInt32 i = 0; i < m_uNumChannels; ++i )
		{
			out_pfLastValues[i] = m_InternalPitchState.fLastValue[i];
		}
	}
	else
	{
		AKASSERT( !"Unsupported format." );
	}
}

void CAkResampler::SwitchTo( const AkAudioFormat & in_fmt, AkReal32 in_fPitch, AkAudioBuffer * io_pIOBuffer, AkUInt32 in_uSampleRate )
{
	AKASSERT( m_uNumChannels == in_fmt.GetNumChannels() );

	AkReal32 fLastValues[ AK_VOICE_MAX_NUM_CHANNELS ];
	GetLastValues( fLastValues );

#ifndef AK_PS3
	bool bOldDeInt = IsPostDeInterleaveRequired();
#endif

	AkReal32 fNewSampleRateConvertRatio = (AkReal32) in_fmt.uSampleRate / in_uSampleRate;
	if ( m_fSampleRateConvertRatio != fNewSampleRateConvertRatio )
	{
		m_fSampleRateConvertRatio = fNewSampleRateConvertRatio;
		m_bFirstSetPitch = true;
	}

	SetPitch( in_fPitch );

	m_uInputBlockAlign = (AkUInt8)in_fmt.GetBlockAlign();
	m_DSPFunctionIndex = GetDSPFunctionIndex( in_fmt );
#if defined(AK_IOS) && !defined(AK_CPU_ARM_NEON)	
	SetupAudioConverter( in_fmt, in_uSampleRate);
#endif	

#ifndef AK_PS3
	bool bNewDeInt = IsPostDeInterleaveRequired();

	if ( !bNewDeInt )
	{
		if ( bOldDeInt )
			DeinterleaveAndSwapOutput( io_pIOBuffer );
	}
	else
	{
		if ( !bOldDeInt )
			InterleaveAndSwapOutput( io_pIOBuffer );
	}
#endif

	SetLastValues( fLastValues );
}

AkUInt8 CAkResampler::GetDSPFunctionIndex( const AkAudioFormat & in_fmt ) const
{
	AkUInt8 uDSPFunctionIndex = 0;

	switch ( in_fmt.GetBitsPerSample() )
	{
	case 16:
		AKASSERT( in_fmt.GetInterleaveID() == AK_INTERLEAVED );
		switch ( m_uNumChannels )
		{
		case 1:
			uDSPFunctionIndex = 0;
			break;
		case 2:
			uDSPFunctionIndex = 1;
			break;
		case 3:
		case 4:
			uDSPFunctionIndex = 2;
			break;
		default:
			uDSPFunctionIndex = 3;
			break;
		}
		break;
	case 32:
		AKASSERT( in_fmt.GetInterleaveID() == AK_NONINTERLEAVED );
		switch ( m_uNumChannels )
		{
		case 1:
			uDSPFunctionIndex = 4;
			break;
		case 2:
			uDSPFunctionIndex = 5;
			break;
		case 3:
		case 4:
			uDSPFunctionIndex = 6;
			break;
		default:
			uDSPFunctionIndex = 7;
			break;
		}
		break;
	default:
		AKASSERT( !"Invalid sample resolution." );
		return 0xFF;
	}

	return uDSPFunctionIndex;
}

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Setup converter to use appropriate conversion routine
//-----------------------------------------------------------------------------
void CAkResampler::Init( AkAudioFormat * io_pFormat, AkUInt32 in_uSampleRate )
{ 
	m_InternalPitchState.uOutFrameOffset = 0;
	m_InternalPitchState.uInFrameOffset = 0;

	m_InternalPitchState.uFloatIndex = SINGLEFRAMEDISTANCE; // Initial index set to 1 -> 0 == previous buffer
	// Note: No need to set last values since float index initial value is 1 

	// Pitch interpolation variables
	m_InternalPitchState.uCurrentFrameSkip = 0;				// Current frame skip
	m_InternalPitchState.uTargetFrameSkip = 0;				// Target frame skip
	m_InternalPitchState.uInterpolationRampCount = 0;		// Sample count for pitch parameter interpolation 
	m_fTargetPitchVal = 0.0f;								// Target pitch value
	m_bFirstSetPitch = true;

	// The interpolation must be resized
	// sample rate at 48000 khz -- interpolation over 1024	samples // Normal / high quality
	// sample rate at 24000 khz -- interpolation over 512	samples // Custom low PC
	// sample rate at 375 khz	-- interpolation over 8		samples // Motion
	m_InternalPitchState.uInterpolationRampInc = PITCHRAMPBASERATE / in_uSampleRate;

	// Set resampling ratio
	m_fSampleRateConvertRatio = (AkReal32) io_pFormat->uSampleRate / in_uSampleRate;
	m_uNumChannels = (AkUInt8)AK::GetNumChannels( io_pFormat->GetChannelMask() );
	m_uInputBlockAlign = (AkUInt8)io_pFormat->GetBlockAlign();
	AKASSERT( m_uNumChannels <= 8 );

	m_DSPFunctionIndex = GetDSPFunctionIndex( *io_pFormat );
#if defined(AK_IOS) && !defined(AK_CPU_ARM_NEON)	
	SetupAudioConverter( *io_pFormat, in_uSampleRate );
#endif	
	m_PitchOperationMode = PitchOperatingMode_Bypass; // Will be set every time by SetPitch()

#ifdef PERFORMANCE_BENCHMARK
	m_fTotalTime = 0.f;
	m_uNumberCalls = 0;
#endif
}

#ifdef AK_PS3 

void CAkResampler::ExecutePS3(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								struct AkVPLState & io_state )
{
	AK::MultiCoreServices::DspProcess * pDsp = CAkLEngine::GetDspProcess();

	// Setup DSP process
	pDsp->ResetDspProcess( false );
	pDsp->SetDspProcess( JobBinInfo[m_PitchOperationMode][m_DSPFunctionIndex/4] );

	m_InternalPitchState.uInValidFrames = io_pInBuffer->uValidFrames;
	m_InternalPitchState.uOutValidFrames = 0;
	m_InternalPitchState.eState = AK_Fail;
	m_InternalPitchState.uChannelMask = io_pInBuffer->GetChannelMask();
	m_InternalPitchState.uOutMaxFrames = io_pOutBuffer->MaxFrames();
	
	AkUInt8 * pInputStartAddress;
	if ( ISNATIVETYPE( m_DSPFunctionIndex ) )
	{
		// Output buffer (first channel position, others are offset by MaxFrames)
		AKASSERT( io_pInBuffer->MaxFrames() == io_pOutBuffer->MaxFrames() );
		pInputStartAddress = (AkUInt8 *)( (AkReal32*)io_pInBuffer->GetDataStartDMA() + m_InternalPitchState.uInFrameOffset ); // used to calculate input offset	
	}
	else
	{
		pInputStartAddress = (AkUInt8 *)io_pInBuffer->GetInterleavedData() + m_InternalPitchState.uInFrameOffset * m_uInputBlockAlign;	
	}

	m_InternalPitchState.uInOffset = (AkUInt32)pInputStartAddress & 0xF;
	// Output buffer (first channel position, others are offset by MaxFrames)
	m_InternalPitchState.pOutBuffer = (AkReal32*)io_pOutBuffer->GetDataStartDMA( ) + m_InternalPitchState.uOutFrameOffset;

	// Parameter packaging
	pDsp->AddDspProcessSmallDma( &m_InternalPitchState, sizeof(AkInternalPitchState) );

	if ( ISNATIVETYPE( m_DSPFunctionIndex ) )
	{
		// Deinterleaved input buffer, use 1 DMA per channel
		for ( AkUInt32 i = 0; i < m_uNumChannels; ++i )
		{
			// Note: This works because uInOffset is the same for all channel ( a consequence that the channels are separated by a 16 byte multiple )
			AkUInt8* pDmaStart = (AkUInt8*) ((AkUInt32)( io_pInBuffer->GetChannel( i ) + m_InternalPitchState.uInFrameOffset ) & ~0xf );
			AkUInt32 uDmaSize = AK_ALIGN_SIZE_FOR_DMA( io_pInBuffer->uValidFrames * sizeof(AkReal32) + m_InternalPitchState.uInOffset );
			pDsp->AddDspProcessDma( pDmaStart, uDmaSize );
		}
	}
	else
	{
		// Interleaved input buffer, use one or 2 DMAs dependent on size
		AkUInt8 * pDmaStart = (AkUInt8 *) ( (AkUInt32) pInputStartAddress & ~0xf ); // dma is 16-aligned
		AkUInt32 uDmaSize = AK_ALIGN_SIZE_FOR_DMA( io_pInBuffer->uValidFrames * m_uInputBlockAlign + m_InternalPitchState.uInOffset );
		pDsp->AddDspProcessDma( pDmaStart, uDmaSize );
	}

	// Allocate worst-case scenario output buffer (SPU)
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();
	// TODO: Local storage could be reduced to (uMaxFrames-m_InternalPitchState.uOutFrameOffset)*io_pInBuffer->NumChannels()*sizeof(AkReal32) + uOutOffset 
	AkUInt32 uOutputSize = io_pInBuffer->NumChannels() * uMaxFrames * sizeof( AkReal32 );
	AKASSERT( uOutputSize % 16 == 0 );
	pDsp->SetOutputBufferSize( uOutputSize + 16 ); // padding necessary for unaligned vector write

	io_state.result = AK_ProcessNeeded;
}

#else

//-----------------------------------------------------------------------------
// Name: Execute
//-----------------------------------------------------------------------------
AKRESULT CAkResampler::Execute(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer )
{
	AKASSERT( io_pInBuffer != NULL );
	AKASSERT( io_pOutBuffer != NULL );
	AKASSERT( m_InternalPitchState.uRequestedFrames <= io_pOutBuffer->MaxFrames() );

#ifdef PERFORMANCE_BENCHMARK
	AkInt64 TimeBefore;
	AKPLATFORM::PerformanceCounter( &TimeBefore ); 
#endif

	// Call appropriate DSP function

	if ( io_pInBuffer->uValidFrames == 0 )
		return AK_NoMoreData; // WG-15893

	AKRESULT eResult;
	do
	{
		eResult = (PitchDSPFuncTable[m_PitchOperationMode][m_DSPFunctionIndex])( io_pInBuffer, io_pOutBuffer, m_InternalPitchState.uRequestedFrames, &m_InternalPitchState );
		if ( m_PitchOperationMode == PitchOperatingMode_Interpolating && m_InternalPitchState.uInterpolationRampCount >= PITCHRAMPLENGTH )
		{
			m_InternalPitchState.uCurrentFrameSkip = m_InternalPitchState.uTargetFrameSkip;
			m_PitchOperationMode = PitchOperatingMode_Fixed;
			// Note: It is ok to go to fixed mode (even if it should have gone to bypass mode) for the remainder of this buffer
			// It will go back to bypass mode after next SetPitch() is called
		}
		
	} 
	while( io_pInBuffer->uValidFrames > 0 && 
		   io_pOutBuffer->uValidFrames < m_InternalPitchState.uRequestedFrames );

#ifdef PERFORMANCE_BENCHMARK
	AkInt64 TimeAfter;
	AKPLATFORM::PerformanceCounter( &TimeAfter );
	AkReal32 fElapsed = AKPLATFORM::Elapsed( TimeAfter, TimeBefore );
	m_fTotalTime += fElapsed;
	++m_uNumberCalls;
#endif

	return eResult;
}

bool CAkResampler::IsPostDeInterleaveRequired()
{
#if defined(AK_WIN) || defined(AK_XBOX360) || defined(AK_APPLE) || defined(AK_WIIU_SOFTWARE) || defined(AK_XBOXONE) || defined(AK_PS4)
	bool ret = 
#ifndef AK_XBOX360
		m_PitchOperationMode == PitchOperatingMode_Bypass &&
#endif			
		m_uNumChannels > 2 && 
		!ISNATIVETYPE( m_DSPFunctionIndex );
	return ret;
#else
	return false;
#endif
}


AKRESULT CAkResampler::DeinterleaveAndSwapOutput( AkAudioBuffer * io_pIOBuffer )
{
	// Do this in a temporary allocated buffer and release the one provided (buffer swap) once its done).
	AkPipelineBuffer DeinterleavedBuffer = *((AkPipelineBuffer*)io_pIOBuffer);
	AKRESULT eStatus = DeinterleavedBuffer.GetCachedBuffer( io_pIOBuffer->MaxFrames(), io_pIOBuffer->GetChannelMask() );
	if ( eStatus != AK_Success )
		return eStatus;

	DeinterleavedBuffer.uValidFrames = io_pIOBuffer->uValidFrames;
	Deinterleave_Native_NChan( io_pIOBuffer, &DeinterleavedBuffer );

	// Release pipeline buffer and swap for the deinterleaved buffer
	((AkPipelineBuffer*)io_pIOBuffer)->ReleaseCachedBuffer();
	*io_pIOBuffer = *((AkAudioBuffer*)&DeinterleavedBuffer); //Just overwrite AkAudioBuffer part

	return AK_Success;
}

AKRESULT CAkResampler::InterleaveAndSwapOutput( AkAudioBuffer * io_pIOBuffer )
{
	// Do this in a temporary allocated buffer and release the one provided (buffer swap) once its done).
	AkPipelineBuffer InterleavedBuffer = *((AkPipelineBuffer*)io_pIOBuffer);
	AKRESULT eStatus = InterleavedBuffer.GetCachedBuffer( io_pIOBuffer->MaxFrames(), io_pIOBuffer->GetChannelMask() );
	if ( eStatus != AK_Success )
		return eStatus;

	InterleavedBuffer.uValidFrames = io_pIOBuffer->uValidFrames;
	Interleave_Native_NChan( io_pIOBuffer, &InterleavedBuffer );

	// Release pipeline buffer and swap for the deinterleaved buffer
	((AkPipelineBuffer*)io_pIOBuffer)->ReleaseCachedBuffer();
	*io_pIOBuffer = *((AkAudioBuffer*)&InterleavedBuffer); //Just overwrite AkAudioBuffer part

	return AK_Success;
}

#endif

bool CAkResampler::HasOffsets()
{
	return ( m_InternalPitchState.uOutFrameOffset || m_InternalPitchState.uInFrameOffset );
}

void CAkResampler::ResetOffsets()
{
	m_InternalPitchState.uOutFrameOffset = 0;
	m_InternalPitchState.uInFrameOffset = 0;
}

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminate conversion
//-----------------------------------------------------------------------------
void CAkResampler::Term()
{
#ifdef PERFORMANCE_BENCHMARK
	AkOSChar szString[64];
	AK_OSPRINTF( szString, 64, AKTEXT("%f\n"), m_fTotalTime/m_uNumberCalls );
	AKPLATFORM::OutputDebugMsg( szString );
	MONITOR_MSG( szString );
#endif 

#if defined(AK_IOS) && !defined(AK_CPU_ARM_NEON)
	if(m_InternalPitchState.m_pAudioConverter)
	{
		DisposeAudioConverter(m_InternalPitchState.m_pAudioConverter);
		m_InternalPitchState.m_pAudioConverter = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Name: SetPitch
// Desc: Change pitch shift (value provided in cents)
//-----------------------------------------------------------------------------
void CAkResampler::SetPitch( AkReal32 in_fPitchVal )
{
	// Clip pitch value to supported range
	in_fPitchVal = AkMath::Max( in_fPitchVal, AK_LE_MIN_PITCHSHIFTCENTS ); 
	in_fPitchVal = AkMath::Min( in_fPitchVal, AK_LE_MAX_PITCHSHIFTCENTS );

	if ( AK_EXPECT_FALSE( m_bFirstSetPitch ) )
	{	
		// No interpolation required
		m_InternalPitchState.uCurrentFrameSkip = (AkUInt32) ( ( m_fSampleRateConvertRatio * powf(2.f, in_fPitchVal / 1200.f ) ) * FPMUL + 0.5 ) ; // round
		m_InternalPitchState.uTargetFrameSkip = m_InternalPitchState.uCurrentFrameSkip;				
		m_InternalPitchState.uInterpolationRampCount = PITCHRAMPLENGTH;
		m_fTargetPitchVal = in_fPitchVal;
		m_bFirstSetPitch = false;
	}

	if ( in_fPitchVal != m_fTargetPitchVal )
	{
		if ( m_PitchOperationMode == PitchOperatingMode_Interpolating )
		{
			m_InternalPitchState.uCurrentFrameSkip = (AkInt32) m_InternalPitchState.uCurrentFrameSkip + 
				( (AkInt32) m_InternalPitchState.uTargetFrameSkip - (AkInt32) m_InternalPitchState.uCurrentFrameSkip ) *
				(AkInt32) m_InternalPitchState.uInterpolationRampCount / (AkInt32) PITCHRAMPLENGTH;
		}
		// New pitch interpolation is required
		m_InternalPitchState.uInterpolationRampCount = 0;	
		m_InternalPitchState.uTargetFrameSkip = (AkUInt32) ( ( m_fSampleRateConvertRatio * powf(2.f, in_fPitchVal / 1200.f ) ) * FPMUL + 0.5 ) ; // round
		m_fTargetPitchVal = in_fPitchVal;
	}

	if ( m_InternalPitchState.uCurrentFrameSkip != m_InternalPitchState.uTargetFrameSkip )
	{	
		// Route to appropriate pitch interpolating DSP
		m_PitchOperationMode = PitchOperatingMode_Interpolating;
	}
	else
	{
		// No interpolation required.
		// Bypass if effective resampling is within our fixed point precision
		if ( m_InternalPitchState.uCurrentFrameSkip == SINGLEFRAMEDISTANCE )
		{
			// Note: route the next execute dereference to appropriate DSP
			m_PitchOperationMode = PitchOperatingMode_Bypass;
		}
		else
		{
			// Done with the pitch change but need to resample at constant ratio
			m_PitchOperationMode = PitchOperatingMode_Fixed;
		}
	}
}

void CAkResampler::SetPitchForTimeSkip( AkReal32 in_fPitchVal )
{
	if ( m_bFirstSetPitch || in_fPitchVal != m_fTargetPitchVal )
	{
		// Clip pitch value to supported range
		in_fPitchVal = AkMath::Max( in_fPitchVal, AK_LE_MIN_PITCHSHIFTCENTS ); 
		in_fPitchVal = AkMath::Min( in_fPitchVal, AK_LE_MAX_PITCHSHIFTCENTS );

		m_InternalPitchState.uCurrentFrameSkip = (AkUInt32) ( ( m_fSampleRateConvertRatio * powf(2.f, in_fPitchVal / 1200.f ) ) * FPMUL + 0.5 ) ; // round
		m_InternalPitchState.uTargetFrameSkip = m_InternalPitchState.uCurrentFrameSkip;
		m_InternalPitchState.uInterpolationRampCount = PITCHRAMPLENGTH;
		m_fTargetPitchVal = in_fPitchVal;
		m_bFirstSetPitch = false;
	}
}


