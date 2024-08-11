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

#ifndef _AK_RESAMPLER_COMMON_H_
#define _AK_RESAMPLER_COMMON_H_

#include "AkInternalPitchState.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>

#ifndef AK_PS3

// Pitch DSP routines declarations

// Bypass (no pitch or resampling) with INTERLEAVED signed 16-bit samples optimized for 2 channel signals. 
#if defined ( AK_CPU_X86 ) || defined ( AK_XBOX360 ) 
AKRESULT Bypass_I16_2Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState );
#endif
AKRESULT Bypass_I16_2ChanSSE2(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState );

// Bypass (no pitch or resampling) with INTERLEAVED signed 16-bit samples for any number of channels. 
AKRESULT Bypass_I16_NChan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState );

// Bypass (no pitch or resampling) with DEINTERLEAVED floating point samples for any number of channels.
AKRESULT Bypass_Native_NChan(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );

// Bypass (no pitch or resampling) with INTERLEAVED signed 16-bit samples for any number of channels (INTERLEAVED OUTPUT) 
#if defined ( AK_CPU_X86 ) || defined ( AK_XBOX360 )
AKRESULT Bypass_I16_NChanVec(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );
#endif
AKRESULT Bypass_I16_NChanVecSSE2(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );
#if defined(AK_IOS) && !defined(AK_CPU_ARM_NEON)
AKRESULT Bypass_I16_1ChanCoreAudio(	AkAudioBuffer * io_pInBuffer, 
								   AkAudioBuffer * io_pOutBuffer,
								   AkUInt32 uRequestedSize,
								   AkInternalPitchState * io_pPitchState );
#endif								   

#if defined( AK_WIIU_SOFTWARE )
AKRESULT Bypass_I16_NChanWiiU(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );
AKRESULT Bypass_I16_2ChanWiiU(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );
AKRESULT Fixed_I16_1ChanWiiU(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );
AKRESULT Fixed_I16_2ChanWiiU(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );
AKRESULT Fixed_I16_1ChanWiiU_2(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );
AKRESULT Fixed_Native_1ChanWiiU(AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );
AKRESULT Fixed_Native_2ChanWiiU(AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );
AKRESULT Interpolating_I16_1ChanWiiU(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState );
AKRESULT Interpolating_I16_2ChanWiiU(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState );
#endif

// Deinterleave floating point samples
void Deinterleave_Native_NChan(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer );

void Interleave_Native_NChan(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer );

// Fixed resampling (no pitch changes) with signed 16-bit samples optimized for one channel signals.
AKRESULT Fixed_I16_1Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState );

// Fixed resampling (no pitch changes) with INTERLEAVED signed 16-bit samples optimized for 2 channel signals.
AKRESULT Fixed_I16_2Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState );	

// Fixed resampling (no pitch changes) with INTERLEAVED signed 16-bit samples for any number of channels.
AKRESULT Fixed_I16_NChan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState );	

// Fixed resampling (no pitch changes) with signed 16-bit samples optimized for 1 to 4 channel signals (vectorized across channels)
AKRESULT Fixed_I16_1To4ChanVec(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );

// Fixed resampling (no pitch changes) with signed 16-bit samples optimized for 5 to 8 channel signals (vectorized across channels)
AKRESULT Fixed_I16_5To8ChanVec(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );

// Fixed resampling (no pitch changes) with floating point samples optimized for one channel signals.
AKRESULT Fixed_Native_1Chan(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );	

// Fixed resampling (no pitch changes) with DEINTERLEAVED floating point samples optimized for 2 channel signals.
AKRESULT Fixed_Native_2Chan(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );	

// Fixed resampling (no pitch changes) with DEINTERLEAVED floating point samples for any number of channels.
AKRESULT Fixed_Native_NChan(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState );	

// Fixed resampling (no pitch changes) with floating point samples optimized for any number of channels
AKRESULT Fixed_Native_NChan_Vec(	AkAudioBuffer * io_pInBuffer, 
									AkAudioBuffer * io_pOutBuffer,
									AkUInt32 uRequestedSize,
									AkInternalPitchState * io_pPitchState );

// Interpolating resampling (pitch changes) with signed 16-bit samples optimized for one channel signals.
AKRESULT Interpolating_I16_1Chan(	AkAudioBuffer * io_pInBuffer, 
									AkAudioBuffer * io_pOutBuffer,
									AkUInt32 uRequestedSize,
									AkInternalPitchState * io_pPitchState );	

// Interpolating resampling (pitch changes) with INTERLEAVED signed 16-bit samples optimized for 2 channel signals.
AKRESULT Interpolating_I16_2Chan(	AkAudioBuffer * io_pInBuffer, 
									AkAudioBuffer * io_pOutBuffer,
									AkUInt32 uRequestedSize,
									AkInternalPitchState * io_pPitchState );	

// Interpolating resampling (pitch changes) with signed 16-bit samples optimized for 1 to 4 channel signals (vectorized across channels)
AKRESULT Interpolating_I16_1To4ChanVec(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState );

// Interpolating resampling (pitch changes) with signed 16-bit samples optimized for 5 to 8 channel signals (vectorized across channels)
AKRESULT Interpolating_I16_5To8ChanVec(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState );

// Interpolating resampling (pitch changes) with INTERLEAVED signed 16-bit samples for any number of channels.
AKRESULT Interpolating_I16_NChan(		AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState );	

// Interpolating resampling (pitch changes) with floating point samples optimized for one channel signals.
AKRESULT Interpolating_Native_1Chan(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState );	

//  Interpolating resampling (pitch changes) with DEINTERLEAVED floating point samples optimized for 2 channel signals.
AKRESULT Interpolating_Native_2Chan(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState );

//  Interpolating resampling (pitch changes) with DEINTERLEAVED floating point samples for any number of channels.
AKRESULT Interpolating_Native_NChan(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState );

//  Interpolating resampling (pitch changes) with DEINTERLEAVED floating point samples optimized for any number of channels.
AKRESULT Interpolating_Native_NChan_Vec(	AkAudioBuffer * io_pInBuffer, 
											AkAudioBuffer * io_pOutBuffer,
											AkUInt32 uRequestedSize,
											AkInternalPitchState * io_pPitchState );


#endif


// Floating point conversion routines defines
#define DENORMALIZEFACTORI16	(32767)
#define NORMALIZEFACTORI16		(0.000030517578125f)	// 1/32768
#define SAMPLEMINI16			(-32768)
#define SAMPLEMAXI16			(32767)

#define FLOAT_TO_INT16(__val__) ((AkInt16)AkMax( AkMin( (__val__) * DENORMALIZEFACTORI16, SAMPLEMAXI16 ), SAMPLEMINI16 ))
#define INT16_TO_FLOAT(__val__) ((AkReal32)(__val__) * NORMALIZEFACTORI16)

// Macros for processing common to all routines

#define PITCH_BYPASS_DSP_SETUP( ) \
	AKASSERT( io_pOutBuffer->MaxFrames() >= io_pPitchState->uOutFrameOffset ); \
	AkUInt32 uInBufferFrames = io_pInBuffer->uValidFrames; \
	AkUInt32 uOutBufferFrames = uRequestedSize - io_pPitchState->uOutFrameOffset; \
	AkUInt32 uFramesToCopy = AkMin( uOutBufferFrames, uInBufferFrames ); \
	AkInt32 uLastSample = static_cast<AkInt32>(uFramesToCopy - 1); 

#define PITCH_BYPASS_DSP_TEARDOWN( ) \
	AKASSERT( uFramesToCopy <= io_pInBuffer->uValidFrames );\
    io_pInBuffer->uValidFrames -= (AkUInt16)uFramesToCopy;\
    AKASSERT( ( io_pPitchState->uOutFrameOffset + uFramesToCopy ) <= uRequestedSize );	\
	io_pOutBuffer->uValidFrames = (AkUInt16)(io_pPitchState->uOutFrameOffset + uFramesToCopy);\
	io_pPitchState->uFloatIndex = SINGLEFRAMEDISTANCE;\
	if ( uFramesToCopy == uInBufferFrames )\
		io_pPitchState->uInFrameOffset = 0;\
	else\
		io_pPitchState->uInFrameOffset += uFramesToCopy;\
	if ( uFramesToCopy == uOutBufferFrames )\
		return AK_DataReady;\
	else\
	{\
		io_pPitchState->uOutFrameOffset += uFramesToCopy;\
		return AK_DataNeeded;\
	}

//WG-9595
// removed the assert: AKASSERT( uInBufferFrames > 0 );
// was not able to comment it inline due to the usages of the backslash thing in the macro
// This assert pops when using motion with pitch, but everything seem to be handled just fine.
#define PITCH_FIXED_DSP_SETUP( ) \
	AKASSERT( io_pOutBuffer->MaxFrames() >= io_pPitchState->uOutFrameOffset );\
	AkUInt32 uInBufferFrames = io_pInBuffer->uValidFrames;	\
	AkUInt32 uOutBufferFrames = uRequestedSize - io_pPitchState->uOutFrameOffset;\
	AkUInt32 uIndexFP = io_pPitchState->uFloatIndex;\
	AkUInt32 uFrameSkipFP = io_pPitchState->uCurrentFrameSkip;\
	AkUInt32 uPreviousFrameIndex = uIndexFP >> FPBITS;\
	AkUInt32 uInterpLocFP = uIndexFP & FPMASK;\
	AkUInt32 uNumIterPreviousFrame = (SINGLEFRAMEDISTANCE - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;\
	uNumIterPreviousFrame = AkMin( uOutBufferFrames, uNumIterPreviousFrame );

#define PITCH_FIXED_DSP_SETUP_VEC( )\
	AKASSERT( io_pOutBuffer->MaxFrames() >= io_pPitchState->uOutFrameOffset );\
	const AkUInt32 uInBufferFrames = io_pInBuffer->uValidFrames;	\
	const AkUInt32 uOutBufferFrames = uRequestedSize - io_pPitchState->uOutFrameOffset;\
	AkUInt32 uFrameSkipFP = io_pPitchState->uCurrentFrameSkip;\
	const AkUInt32 uNumIterPreviousFrame = AkMin( (SINGLEFRAMEDISTANCE - io_pPitchState->uFloatIndex + (uFrameSkipFP-1)) / uFrameSkipFP, uOutBufferFrames);\
	__vector4 vu4IndexFP = __lvlx( &io_pPitchState->uFloatIndex, 0 );\
	vu4IndexFP = __vspltw( vu4IndexFP, 0 );\
	__vector4 vu4FrameSkipFP = __lvlx( &uFrameSkipFP, 0 );\
	vu4FrameSkipFP = __vspltw( vu4FrameSkipFP, 0 );\
	static const __vector4 vu4AndFPMASK = { 9.1834e-041 /*FPMASK*/, 9.1834e-041 /*FPMASK*/, 9.1834e-041 /*FPMASK*/, 9.1834e-041 /*FPMASK*/ };\
	__vector4 vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );\
	__vector4 vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	

#define PITCH_SAVE_NEXT_I16_1CHAN( ) \
	AkUInt32 uFramesConsumed = AkMin( uPreviousFrameIndex, uInBufferFrames );\
	if ( uFramesConsumed )\
		*io_pPitchState->iLastValue = pInBuf[uFramesConsumed];

#define PITCH_SAVE_NEXT_I16_2CHAN( ) \
	AkUInt32 uFramesConsumed = AkMin( uPreviousFrameIndex, uInBufferFrames );\
	if ( uFramesConsumed )\
	{\
		io_pPitchState->iLastValue[0] = pInBuf[2*uFramesConsumed];\
		io_pPitchState->iLastValue[1] = pInBuf[2*uFramesConsumed+1];\
	}

#define PITCH_SAVE_NEXT_I16_NCHAN( __uIndexFP__ ) \
	const AkUInt32 uFramesConsumed = AkMin( (__uIndexFP__) >> FPBITS, uInBufferFrames ); \
	if ( uFramesConsumed )\
	{\
		for ( AkUInt32 k = 0; k < uNumChannels; ++k ) \
			io_pPitchState->iLastValue[k] = pInBuf[uNumChannels*uFramesConsumed+k];\
	}

#define PITCH_SAVE_NEXT_NATIVE_1CHAN( ) \
	AkUInt32 uFramesConsumed = AkMin( uPreviousFrameIndex, uInBufferFrames );\
	if ( uFramesConsumed )\
		*io_pPitchState->fLastValue = pInBuf[uFramesConsumed];

#define PITCH_SAVE_NEXT_NATIVE_2CHAN( ) \
	AkUInt32 uFramesConsumed = AkMin( uPreviousFrameIndex, uInBufferFrames );\
	if ( uFramesConsumed )\
	{\
		io_pPitchState->fLastValue[0] = pInBuf[uFramesConsumed];\
		io_pPitchState->fLastValue[1] = pInBuf[uFramesConsumed+uMaxFrames];\
	}

#define PITCH_SAVE_NEXT_NATIVE_NCHAN( __uIndexFP__ ) \
	const AkUInt32 uFramesConsumed = AkMin( (__uIndexFP__) >> FPBITS, uInBufferFrames ); \
	if ( uFramesConsumed )\
	{\
		for ( AkUInt32 k = 0; k < uNumChannels; ++k ) \
		{ \
			AkReal32 * pChannelStart = io_pInBuffer->GetChannel( k ) + io_pPitchState->uInFrameOffset - 1;  \
			io_pPitchState->fLastValue[k] = pChannelStart[uFramesConsumed];\
		} \
	}

#define FP_INDEX_RESET( __startvalue__ ) \
	uIndexFP = __startvalue__;	\
	uPreviousFrameIndex = uIndexFP >> FPBITS;\
	uInterpLocFP = uIndexFP & FPMASK;

#define FP_INDEX_ADVANCE() \
	uIndexFP += uFrameSkipFP;	\
	uPreviousFrameIndex = uIndexFP >> FPBITS;\
	uInterpLocFP = uIndexFP & FPMASK;

#define LINEAR_INTERP_I16( __PreviousFrame__, __FrameDiff__ ) \
	(AkReal32) ( ( __PreviousFrame__ << FPBITS ) + ( __FrameDiff__ * (AkInt32) uInterpLocFP ) ) * (NORMALIZEFACTORI16 / FPMUL);

#define LINEAR_INTERP_NATIVE( __PreviousFrame__, __FrameDiff__ ) \
	__PreviousFrame__ + ( uInterpLocFP * fScale * __FrameDiff__ );

#define PITCH_FIXED_DSP_TEARDOWN( __uIndexFP__ ) \
	AKASSERT( __uIndexFP__ >= uFramesConsumed * FPMUL );\
	io_pPitchState->uFloatIndex = __uIndexFP__ - uFramesConsumed * FPMUL;\
    io_pInBuffer->uValidFrames -= (AkUInt16)uFramesConsumed;\
	AkUInt32 uFramesProduced = uNumIterPreviousFrame + uNumIterThisFrame;\
	AKASSERT( uFramesProduced <= uOutBufferFrames );\
	io_pOutBuffer->uValidFrames = (AkUInt16)(io_pPitchState->uOutFrameOffset + uFramesProduced);\
	if ( uFramesConsumed == uInBufferFrames )\
		io_pPitchState->uInFrameOffset = 0;\
	else\
		io_pPitchState->uInFrameOffset += uFramesConsumed;\
	if ( uFramesProduced == uOutBufferFrames )\
		return AK_DataReady;\
	else\
	{\
		io_pPitchState->uOutFrameOffset += uFramesProduced;\
		return AK_DataNeeded;\
	}

#define PITCH_INTERPOLATION_SETUP( ) \
	AkUInt32 uRampCount = io_pPitchState->uInterpolationRampCount;\
	AkUInt32 uRampInc = io_pPitchState->uInterpolationRampInc;\
	const AkInt32 iStartFrameSkip = (AkInt32) uFrameSkipFP;\
	const AkInt32 iTargetFrameSkip = (AkInt32) io_pPitchState->uTargetFrameSkip;\
	const AkInt32 iFrameSkipDiff = iTargetFrameSkip - iStartFrameSkip;\
	const AkInt32 iScaledStartFrameSkip = iStartFrameSkip * PITCHRAMPLENGTH;

#define RESAMPLING_FACTOR_INTERPOLATE( ) \
	uRampCount += uRampInc;	\
	uFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uRampCount)/PITCHRAMPLENGTH;

#define PITCH_INTERPOLATION_TEARDOWN( ) \
	io_pPitchState->uInterpolationRampCount = uRampCount;
	
#define PITCH_INTERPOLATING_DSP_SETUP PITCH_FIXED_DSP_SETUP

#define PITCH_INTERPOLATING_DSP_TEARDOWN( __uIndexFP__ ) \
	AKASSERT( __uIndexFP__ >= uFramesConsumed * FPMUL );\
	io_pPitchState->uFloatIndex = __uIndexFP__ - uFramesConsumed * FPMUL;\
    io_pInBuffer->uValidFrames -= (AkUInt16)uFramesConsumed;\
	AKASSERT( uFramesProduced <= uOutBufferFrames );\
	io_pOutBuffer->uValidFrames = (AkUInt16)(io_pPitchState->uOutFrameOffset + uFramesProduced);\
	if ( uFramesConsumed == uInBufferFrames )\
		io_pPitchState->uInFrameOffset = 0;\
	else\
		io_pPitchState->uInFrameOffset += uFramesConsumed;\
	if ( uFramesProduced == uOutBufferFrames )\
		return AK_DataReady;\
	else\
	{\
		io_pPitchState->uOutFrameOffset += uFramesProduced;\
		return AK_DataNeeded;\
	}

static AkForceInline AkUInt32 AkChannelRemap( AkUInt32 in_uInputChannelIndex, AkChannelMask in_uChannelMask )
{
	AkUInt32 uOutChannelIndex = in_uInputChannelIndex;
#ifdef AK_LFECENTER
	if( AK::HasLFE( in_uChannelMask ) )
	{
		AkUInt32 uBelowLFEChannelMask = in_uChannelMask & (AK_SPEAKER_LOW_FREQUENCY-1);
		AkUInt32 uInputLFEIndex = 0;
		while( uBelowLFEChannelMask )
		{ 
			++uInputLFEIndex; 
			uBelowLFEChannelMask &= uBelowLFEChannelMask-1; 
		} 
		if ( in_uInputChannelIndex == uInputLFEIndex )
			uOutChannelIndex = AK::GetNumChannels( in_uChannelMask ) - 1;
		else if ( in_uInputChannelIndex > uInputLFEIndex )
			uOutChannelIndex = in_uInputChannelIndex - 1;
	}
#endif
	return uOutChannelIndex;
}

#endif // _AK_RESAMPLER_COMMON_H_
