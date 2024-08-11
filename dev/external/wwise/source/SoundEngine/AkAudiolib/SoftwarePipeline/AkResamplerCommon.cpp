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
#include "AkResamplerCommon.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

/********************* BYPASS DSP ROUTINES **********************/

// Bypass (no pitch or resampling) with INTERLEAVED signed 16-bit samples for any number of channels. 
AKRESULT Bypass_I16_NChan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_BYPASS_DSP_SETUP( );

	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		AkInt16 * AK_RESTRICT pIn = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData() ) + uNumChannels*io_pPitchState->uInFrameOffset + i;
		AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( AkChannelRemap( i, io_pInBuffer->GetChannelMask() ) ) ) + io_pPitchState->uOutFrameOffset;

		// Need to keep the last buffer value in case we start the pitch algo next buffer
		io_pPitchState->iLastValue[i] = pIn[uNumChannels*uLastSample];

		for ( AkUInt32 j = 0; j < uFramesToCopy; ++j )
		{
			*pOut++ = INT16_TO_FLOAT( *pIn );
			pIn += uNumChannels;

		}
	}

	PITCH_BYPASS_DSP_TEARDOWN( );
}

// Bypass (no pitch or resampling) with DEINTERLEAVED floating point samples for any number of channels.
AKRESULT Bypass_Native_NChan(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState )
{
	PITCH_BYPASS_DSP_SETUP( );
	
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		// Copy the data
		AkReal32 * pInChan = io_pInBuffer->GetChannel( i ) + io_pPitchState->uInFrameOffset;
		AkReal32 * pOutChan = io_pOutBuffer->GetChannel( i ) + io_pPitchState->uOutFrameOffset;
		AKPLATFORM::AkMemCpy( pOutChan, pInChan, uFramesToCopy * sizeof(AkReal32));
		// Need to keep the last buffer value in case we start the pitch algo next buffer (integer format
		io_pPitchState->fLastValue[i] = pInChan[uLastSample];
	}

	PITCH_BYPASS_DSP_TEARDOWN( );
}

// Fixed resampling (no pitch changes) with INTERLEAVED signed 16-bit samples for any number of channels.
AKRESULT Fixed_I16_NChan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	
	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT) io_pInBuffer->GetInterleavedData() + uNumChannels*io_pPitchState->uInFrameOffset - uNumChannels; 
	AkUInt32 uNumIterThisFrame;
	AkUInt32 uStartIndexFP = uIndexFP;
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		FP_INDEX_RESET( uStartIndexFP );

		AkInt16 * AK_RESTRICT pInBufChan = pInBuf + i;
		AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( AkChannelRemap( i, io_pInBuffer->GetChannelMask() ) ) + io_pPitchState->uOutFrameOffset;
		// Use stored value as left value, while right index is on the first sample
		AkInt16 iPreviousFrame = io_pPitchState->iLastValue[i];
		AkUInt32 uIterFrames = uNumIterPreviousFrame;
		while ( uIterFrames-- )
		{
			AkInt32 iSampleDiff = pInBufChan[uNumChannels] - iPreviousFrame;
			*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
			FP_INDEX_ADVANCE();	
		}

		// Determine number of iterations remaining
		const AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
		uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

		// For all other sample frames
		uIterFrames = uNumIterThisFrame;
		while ( uIterFrames-- )
		{
			AkUInt32 uPreviousFrameSamplePos = uPreviousFrameIndex*uNumChannels;
			iPreviousFrame = pInBufChan[uPreviousFrameSamplePos];
			AkInt32 iSampleDiff = pInBufChan[uPreviousFrameSamplePos+uNumChannels] - iPreviousFrame;
			*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
			FP_INDEX_ADVANCE();
		}
	}

	PITCH_SAVE_NEXT_I16_NCHAN( uIndexFP );
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );	
}

// Fixed resampling (no pitch changes) with DEINTERLEAVED floating point samples for any number of channels.
AKRESULT Fixed_Native_NChan(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	static const AkReal32 fScale = 1.f / SINGLEFRAMEDISTANCE;
	AkUInt32 uNumIterThisFrame;	
	AkUInt32 uStartIndexFP = uIndexFP;
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		FP_INDEX_RESET( uStartIndexFP );

		// Minus one to compensate for offset of 1 due to zero == previous
		AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( i ) + io_pPitchState->uInFrameOffset - 1; 
		AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( i ) + io_pPitchState->uOutFrameOffset;
		// Use stored value as left value, while right index is on the first sample
		AkReal32 fPreviousFrame = io_pPitchState->fLastValue[i];
		AkUInt32 uIterFrames = uNumIterPreviousFrame;
		while ( uIterFrames-- )
		{	
			AkReal32 fSampleDiff = pInBuf[1] - fPreviousFrame;
			*pfOutBuf++ = LINEAR_INTERP_NATIVE( fPreviousFrame, fSampleDiff );
			FP_INDEX_ADVANCE();	
		}

		// Determine number of iterations remaining
		const AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
		uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

		// For all other sample frames
		uIterFrames = uNumIterThisFrame;
		while ( uIterFrames-- )
		{
			fPreviousFrame = pInBuf[uPreviousFrameIndex];
			AkReal32 fSampleDiff = pInBuf[uPreviousFrameIndex+1] - fPreviousFrame;
			*pfOutBuf++ = LINEAR_INTERP_NATIVE( fPreviousFrame, fSampleDiff );
			FP_INDEX_ADVANCE();
		}
	}

	PITCH_SAVE_NEXT_NATIVE_NCHAN( uIndexFP );
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );	
}

// Variable resampling with INTERLEAVED signed 16-bit samples for any number of channels.
AKRESULT Interpolating_I16_NChan(	AkAudioBuffer * io_pInBuffer, 
									AkAudioBuffer * io_pOutBuffer,
									AkUInt32 uRequestedSize,
									AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	PITCH_INTERPOLATION_SETUP( );
	
	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT) io_pInBuffer->GetInterleavedData() + uNumChannels*io_pPitchState->uInFrameOffset - uNumChannels; 
	AkReal32 * AK_RESTRICT pfOutBuf; // Keep last channel values for later usage
	AkReal32 * pfOutBufStart; 
	AkUInt32 uStartIndexFP = uIndexFP;
	AkUInt32 i = 0;
	do
	{
		uRampCount = io_pPitchState->uInterpolationRampCount; 
		FP_INDEX_RESET( uStartIndexFP );

		AkInt16 * AK_RESTRICT pInBufChan = pInBuf + i;
		pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( AkChannelRemap( i, io_pInBuffer->GetChannelMask() ) ) + io_pPitchState->uOutFrameOffset;
		pfOutBufStart = pfOutBuf;
		const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

		// Use stored value as left value, while right index is on the first sample
		AkInt16 iPreviousFrame = io_pPitchState->iLastValue[i];
		AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);		// Not more than output frames
		uMaxNumIter = AkMin( uMaxNumIter, (PITCHRAMPLENGTH-uRampCount)/uRampInc );	// Not longer than interpolation ramp length
		while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
		{	
			AkInt32 iSampleDiff = pInBufChan[uNumChannels] - iPreviousFrame;
			*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
			RESAMPLING_FACTOR_INTERPOLATE();
			FP_INDEX_ADVANCE();	
		}

		const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;
		AkUInt32 uIterFrames = (AkUInt32) (pfOutBufEnd - pfOutBuf);
		uIterFrames = AkMin( uIterFrames, (PITCHRAMPLENGTH-uRampCount)/uRampInc );	// No more than the interpolation ramp length
		while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uIterFrames-- )
		{
			AkUInt32 uPreviousFrameSamplePos = uPreviousFrameIndex*uNumChannels;
			iPreviousFrame = pInBufChan[uPreviousFrameSamplePos];
			AkInt32 iSampleDiff = pInBufChan[uPreviousFrameSamplePos+uNumChannels] - iPreviousFrame;
			*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
			RESAMPLING_FACTOR_INTERPOLATE();
			FP_INDEX_ADVANCE();
		}
	} 
	while (++i < uNumChannels);

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_I16_NCHAN( uIndexFP );
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );	
}

// Variable resampling with DEINTERLEAVED floating point samples for any number of channels.
AKRESULT Interpolating_Native_NChan(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	PITCH_INTERPOLATION_SETUP( );

	static const AkReal32 fScale = 1.f / SINGLEFRAMEDISTANCE;
	AkReal32 * AK_RESTRICT pfOutBuf; // Keep last channel values for later usage
	AkReal32 * pfOutBufStart; 
	AkUInt32 uStartIndexFP = uIndexFP;
	AkUInt32 i = 0;
	do
	{
		uRampCount = io_pPitchState->uInterpolationRampCount; 
		FP_INDEX_RESET( uStartIndexFP );

		// Minus one to compensate for offset of 1 due to zero == previous
		AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( i ) + io_pPitchState->uInFrameOffset - 1; 
		pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( i ) + io_pPitchState->uOutFrameOffset;
		pfOutBufStart = pfOutBuf;
		const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

		// Use stored value as left value, while right index is on the first sample
		AkReal32 fPreviousFrame = io_pPitchState->fLastValue[i];
		AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);		// Not more than output frames
		uMaxNumIter = AkMin( uMaxNumIter, (PITCHRAMPLENGTH-uRampCount)/uRampInc );	// Not longer than interpolation ramp length
		while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
		{	
			AkReal32 fSampleDiff = pInBuf[1] - fPreviousFrame;
			*pfOutBuf++ = LINEAR_INTERP_NATIVE( fPreviousFrame, fSampleDiff );
			RESAMPLING_FACTOR_INTERPOLATE();
			FP_INDEX_ADVANCE();	
		}

		const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;
		AkUInt32 uIterFrames = (AkUInt32) (pfOutBufEnd - pfOutBuf);
		uIterFrames = AkMin( uIterFrames, (PITCHRAMPLENGTH-uRampCount)/uRampInc );	// No more than the interpolation ramp length
		while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uIterFrames-- )
		{
			fPreviousFrame = pInBuf[uPreviousFrameIndex];
			AkReal32 fSampleDiff= pInBuf[uPreviousFrameIndex+1] - fPreviousFrame;
			*pfOutBuf++ = LINEAR_INTERP_NATIVE( fPreviousFrame, fSampleDiff );
			RESAMPLING_FACTOR_INTERPOLATE();
			FP_INDEX_ADVANCE();
		}
	} 
	while (++i < uNumChannels);

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_NATIVE_NCHAN( uIndexFP );
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );	
}

// Deinterleave floating point samples (used in conjunction with some N-channel routines)
void Deinterleave_Native_NChan(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer )
{
	const AkUInt32 uFramesToCopy = io_pOutBuffer->uValidFrames;
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		AkReal32 * AK_RESTRICT pIn = (AkReal32 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData() ) + i;
		AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( AkChannelRemap( i, io_pInBuffer->GetChannelMask() ) ) );
		for ( AkUInt32 j = 0; j < uFramesToCopy; ++j )
		{
			*pOut++ = *pIn;
			pIn += uNumChannels;
		}
	}
}

// Interleave floating point samples (for sample-accurate stitching of mixed interleaveness material)
void Interleave_Native_NChan(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer )
{
	const AkUInt32 uFramesToCopy = io_pOutBuffer->uValidFrames;
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		AkReal32 * AK_RESTRICT pIn = (AkReal32 * AK_RESTRICT)( io_pInBuffer->GetChannel( AkChannelRemap( i, io_pInBuffer->GetChannelMask() ) ) );
		AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetInterleavedData() ) + i;
		for ( AkUInt32 j = 0; j < uFramesToCopy; ++j )
		{
			*pOut = *pIn++;
			pOut += uNumChannels;
		}
	}
}

