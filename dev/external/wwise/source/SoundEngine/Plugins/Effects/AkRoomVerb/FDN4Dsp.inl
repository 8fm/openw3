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

#if defined(AKSIMD_V2F32_SUPPORTED)

#include "FDN4Dsp_simdv2.inl"

#elif defined(AKSIMD_V4F32_SUPPORTED) && !defined(AK_PS3) && !(defined(AK_CPU_ARM_NEON) && defined(AK_WIN))

#include "FDN4Dsp_simdv4.inl"

#else

// Basic 4x4 FDN with lowpass filter in feedback loop
// Accumulates single signal into provided output
void FDN4::ProcessBufferAccum(	
	AkReal32 * in_pfInBuffer, 
	AkReal32 * io_pfOutBuffer, 
	AkUInt32 in_uNumFrames )
{
	AkReal32 * AK_RESTRICT pfInBuf = (AkReal32 * AK_RESTRICT) in_pfInBuffer;
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pfOutBuffer;

	AkUInt32 curOffset[4];
	curOffset[0] = FDNDelayLine[0].uCurOffset;
	curOffset[1] = FDNDelayLine[1].uCurOffset;
	curOffset[2] = FDNDelayLine[2].uCurOffset;
	curOffset[3] = FDNDelayLine[3].uCurOffset;

	AkReal32 fFFbk1[4];	// first order feedback memory
	fFFbk1[0] = delayLowPassFilter[0].fFFbk1;
	fFFbk1[1] = delayLowPassFilter[1].fFFbk1;
	fFFbk1[2] = delayLowPassFilter[2].fFFbk1;
	fFFbk1[3] = delayLowPassFilter[3].fFFbk1;

	for ( AkUInt32 i = 0; i < in_uNumFrames; i++ )
	{
		AkReal32 fDelayOut[4];	
		ReadAndAttenuateDelays( fDelayOut, curOffset, fFFbk1 );

		// Sum to output buffer
		// TODO: Use dot product instruction when available
		*pfOutBuf++ += fDelayOut[0] - fDelayOut[1] + fDelayOut[2] - fDelayOut[3];

		// Feedback matrix
		AkReal32 fFeedback[4];
		ProcessFeedbackMatrix( fDelayOut, fFeedback );

		// Input reinjection
		AkReal32 fIn = *pfInBuf++;
		InputReinjection( fFeedback, fIn, curOffset );
	}

	FDNDelayLine[0].uCurOffset = curOffset[0];
	FDNDelayLine[1].uCurOffset = curOffset[1];
	FDNDelayLine[2].uCurOffset = curOffset[2];
	FDNDelayLine[3].uCurOffset = curOffset[3];

	RemoveDenormal( fFFbk1[0] );
	RemoveDenormal( fFFbk1[1] );
	RemoveDenormal( fFFbk1[2] );
	RemoveDenormal( fFFbk1[3] );

	delayLowPassFilter[0].fFFbk1 = fFFbk1[0];
	delayLowPassFilter[1].fFFbk1 = fFFbk1[1];
	delayLowPassFilter[2].fFFbk1 = fFFbk1[2];
	delayLowPassFilter[3].fFFbk1 = fFFbk1[3];
}

// Basic 4x4 FDN with lowpass filter in feedback loop
// Accumulates two decorrelated signals
void FDN4::ProcessBufferAccum(	
	AkReal32 * in_pfInBuffer, 
	AkReal32 * io_pfOutBuffer1, 
	AkReal32 * io_pfOutBuffer2, 
	AkUInt32 in_uNumFrames )
{
	AkReal32 * AK_RESTRICT pfInBuf = (AkReal32 * AK_RESTRICT) in_pfInBuffer;
	AkReal32 * AK_RESTRICT pfOutBuf1 = (AkReal32 * AK_RESTRICT) io_pfOutBuffer1;
	AkReal32 * AK_RESTRICT pfOutBuf2 = (AkReal32 * AK_RESTRICT) io_pfOutBuffer2;

	AkUInt32 curOffset[4];
	curOffset[0] = FDNDelayLine[0].uCurOffset;
	curOffset[1] = FDNDelayLine[1].uCurOffset;
	curOffset[2] = FDNDelayLine[2].uCurOffset;
	curOffset[3] = FDNDelayLine[3].uCurOffset;

	AkReal32 fFFbk1[4];	// first order feedback memory
	fFFbk1[0] = delayLowPassFilter[0].fFFbk1;
	fFFbk1[1] = delayLowPassFilter[1].fFFbk1;
	fFFbk1[2] = delayLowPassFilter[2].fFFbk1;
	fFFbk1[3] = delayLowPassFilter[3].fFFbk1;

	for ( AkUInt32 i = 0; i < in_uNumFrames; i++ )
	{
		AkReal32 fDelayOut[4];	
		ReadAndAttenuateDelays( fDelayOut, curOffset, fFFbk1 );

		// Sum to output buffer
		// TODO: Use dot product instruction when available
		*pfOutBuf1++ += fDelayOut[0] - fDelayOut[1] + fDelayOut[2] - fDelayOut[3];
		*pfOutBuf2++ += fDelayOut[0] + fDelayOut[1] - fDelayOut[2] - fDelayOut[3];

		// Feedback matrix
		AkReal32 fFeedback[4];
		ProcessFeedbackMatrix( fDelayOut, fFeedback );

		// Input reinjection
		AkReal32 fIn = *pfInBuf++;
		InputReinjection( fFeedback, fIn, curOffset );
	}

	FDNDelayLine[0].uCurOffset = curOffset[0];
	FDNDelayLine[1].uCurOffset = curOffset[1];
	FDNDelayLine[2].uCurOffset = curOffset[2];
	FDNDelayLine[3].uCurOffset = curOffset[3];

	RemoveDenormal( fFFbk1[0] );
	RemoveDenormal( fFFbk1[1] );
	RemoveDenormal( fFFbk1[2] );
	RemoveDenormal( fFFbk1[3] );

	delayLowPassFilter[0].fFFbk1 = fFFbk1[0];
	delayLowPassFilter[1].fFFbk1 = fFFbk1[1];
	delayLowPassFilter[2].fFFbk1 = fFFbk1[2];
	delayLowPassFilter[3].fFFbk1 = fFFbk1[3];
}

// Basic 4x4 FDN with lowpass filter in feedback loop
// Accumulates three decorrelated signals
void FDN4::ProcessBufferAccum(	
	AkReal32 * in_pfInBuffer, 
	AkReal32 * io_pfOutBuffer1, 
	AkReal32 * io_pfOutBuffer2, 
	AkReal32 * io_pfOutBuffer3, 
	AkUInt32 in_uNumFrames )
{
	AkReal32 * AK_RESTRICT pfInBuf = (AkReal32 * AK_RESTRICT) in_pfInBuffer;
	AkReal32 * AK_RESTRICT pfOutBuf1 = (AkReal32 * AK_RESTRICT) io_pfOutBuffer1;
	AkReal32 * AK_RESTRICT pfOutBuf2 = (AkReal32 * AK_RESTRICT) io_pfOutBuffer2;
	AkReal32 * AK_RESTRICT pfOutBuf3 = (AkReal32 * AK_RESTRICT) io_pfOutBuffer3;

	AkUInt32 curOffset[4];
	curOffset[0] = FDNDelayLine[0].uCurOffset;
	curOffset[1] = FDNDelayLine[1].uCurOffset;
	curOffset[2] = FDNDelayLine[2].uCurOffset;
	curOffset[3] = FDNDelayLine[3].uCurOffset;

	AkReal32 fFFbk1[4];	// first order feedback memory
	fFFbk1[0] = delayLowPassFilter[0].fFFbk1;
	fFFbk1[1] = delayLowPassFilter[1].fFFbk1;
	fFFbk1[2] = delayLowPassFilter[2].fFFbk1;
	fFFbk1[3] = delayLowPassFilter[3].fFFbk1;

	for ( AkUInt32 i = 0; i < in_uNumFrames; i++ )
	{
		AkReal32 fDelayOut[4];	
		ReadAndAttenuateDelays( fDelayOut, curOffset, fFFbk1 );

		// Sum to output buffer
		// TODO: Use dot product instruction when available
		*pfOutBuf1++ += fDelayOut[0] - fDelayOut[1] + fDelayOut[2] - fDelayOut[3];
		*pfOutBuf2++ += fDelayOut[0] + fDelayOut[1] - fDelayOut[2] - fDelayOut[3];
		*pfOutBuf3++ += fDelayOut[0] - fDelayOut[1] - fDelayOut[2] + fDelayOut[3];

		// Feedback matrix
		AkReal32 fFeedback[4];
		ProcessFeedbackMatrix( fDelayOut, fFeedback );

		// Input reinjection
		AkReal32 fIn = *pfInBuf++;
		InputReinjection( fFeedback, fIn, curOffset );
	}

	FDNDelayLine[0].uCurOffset = curOffset[0];
	FDNDelayLine[1].uCurOffset = curOffset[1];
	FDNDelayLine[2].uCurOffset = curOffset[2];
	FDNDelayLine[3].uCurOffset = curOffset[3];

	RemoveDenormal( fFFbk1[0] );
	RemoveDenormal( fFFbk1[1] );
	RemoveDenormal( fFFbk1[2] );
	RemoveDenormal( fFFbk1[3] );

	delayLowPassFilter[0].fFFbk1 = fFFbk1[0];
	delayLowPassFilter[1].fFFbk1 = fFFbk1[1];
	delayLowPassFilter[2].fFFbk1 = fFFbk1[2];
	delayLowPassFilter[3].fFFbk1 = fFFbk1[3];
}

/*	Alternate version: extracted individual delay line wrappings out of inner loop. no measurable performance gain on XBox360.
void FDN4::ProcessBufferAccum(	
	AkReal32 * in_pfInBuffer, 
	AkReal32 * io_pfOutBuffer1, 
	AkReal32 * io_pfOutBuffer2, 
	AkReal32 * io_pfOutBuffer3, 
	AkUInt32 in_uNumFrames )
{
	AkReal32 * AK_RESTRICT pfInBuf = (AkReal32 * AK_RESTRICT) in_pfInBuffer;
	AkReal32 * AK_RESTRICT pfOutBuf1 = (AkReal32 * AK_RESTRICT) io_pfOutBuffer1;
	AkReal32 * AK_RESTRICT pfOutBuf2 = (AkReal32 * AK_RESTRICT) io_pfOutBuffer2;
	AkReal32 * AK_RESTRICT pfOutBuf3 = (AkReal32 * AK_RESTRICT) io_pfOutBuffer3;

	AkUInt32 curOffset[4];
	curOffset[0] = FDNDelayLine[0].uCurOffset;
	curOffset[1] = FDNDelayLine[1].uCurOffset;
	curOffset[2] = FDNDelayLine[2].uCurOffset;
	curOffset[3] = FDNDelayLine[3].uCurOffset;

	AkReal32 fFFbk1[4];	// first order feedback memory
	fFFbk1[0] = delayLowPassFilter[0].fFFbk1;
	fFFbk1[1] = delayLowPassFilter[1].fFFbk1;
	fFFbk1[2] = delayLowPassFilter[2].fFFbk1;
	fFFbk1[3] = delayLowPassFilter[3].fFFbk1;

	AkUInt32 uToProcess = in_uNumFrames;
	while ( uToProcess )
	{
		AkUInt32 uLeft0 = FDNDelayLine[0].uDelayLineLength - curOffset[0];
		AkUInt32 uLeft1 = FDNDelayLine[1].uDelayLineLength - curOffset[1];
		AkUInt32 uLeft2 = FDNDelayLine[2].uDelayLineLength - curOffset[2];
		AkUInt32 uLeft3 = FDNDelayLine[3].uDelayLineLength - curOffset[3];

		AkUInt32 uProcessThisIter = AkMin( uLeft0, uLeft1 );
		uProcessThisIter = AkMin( uProcessThisIter, uLeft2 );
		uProcessThisIter = AkMin( uProcessThisIter, uLeft3 );
		uProcessThisIter = AkMin( uProcessThisIter, uToProcess );

		for ( AkUInt32 i = 0; i < uProcessThisIter; i++ )
		{
			AkReal32 fDelayOut[4];	
			ReadAndAttenuateDelays( fDelayOut, curOffset, fFFbk1 );

			// Sum to output buffer
			// TODO: Use dot product instruction when available
			*pfOutBuf1++ += fDelayOut[0] - fDelayOut[1] + fDelayOut[2] - fDelayOut[3];
			*pfOutBuf2++ += fDelayOut[0] + fDelayOut[1] - fDelayOut[2] - fDelayOut[3];
			*pfOutBuf3++ += fDelayOut[0] - fDelayOut[1] - fDelayOut[2] + fDelayOut[3];

			// Feedback matrix
			AkReal32 fFeedback[4];
			ProcessFeedbackMatrix( fDelayOut, fFeedback );

			// Input reinjection
			AkReal32 fIn = *pfInBuf++;
			InputReinjectionNoWrapCheck( fFeedback, fIn, curOffset );
		}

		if ( curOffset[0] == FDNDelayLine[0].uDelayLineLength )
			curOffset[0] = 0;

		if ( curOffset[1] == FDNDelayLine[1].uDelayLineLength )
			curOffset[1] = 0;

		if ( curOffset[2] == FDNDelayLine[2].uDelayLineLength )
			curOffset[2] = 0;

		if ( curOffset[3] == FDNDelayLine[3].uDelayLineLength )
			curOffset[3] = 0;

		uToProcess -= uProcessThisIter;
	}

	FDNDelayLine[0].uCurOffset = curOffset[0];
	FDNDelayLine[1].uCurOffset = curOffset[1];
	FDNDelayLine[2].uCurOffset = curOffset[2];
	FDNDelayLine[3].uCurOffset = curOffset[3];

	delayLowPassFilter[0].fFFbk1 = fFFbk1[0];
	delayLowPassFilter[1].fFFbk1 = fFFbk1[1];
	delayLowPassFilter[2].fFFbk1 = fFFbk1[2];
	delayLowPassFilter[3].fFFbk1 = fFFbk1[3];
}
*/
#ifdef __PPU__
AkUInt32 FDN4::GetScratchMemorySizeRequired( AkUInt32 in_uNumFrames )
{
	AkUInt32 uFDNScratchMem = FDNDelayLine[0].GetScratchMemorySizeRequired( in_uNumFrames );
	uFDNScratchMem += FDNDelayLine[1].GetScratchMemorySizeRequired( in_uNumFrames );
	uFDNScratchMem += FDNDelayLine[2].GetScratchMemorySizeRequired( in_uNumFrames );
	uFDNScratchMem += FDNDelayLine[3].GetScratchMemorySizeRequired( in_uNumFrames );
	return uFDNScratchMem;
}
#endif

void FDN4::ReadAndAttenuateDelays( AkReal32 out_fDelayOut[4] )
{
	out_fDelayOut[0] = FDNDelayLine[0].ReadSample( );
	out_fDelayOut[1] = FDNDelayLine[1].ReadSample( );
	out_fDelayOut[2] = FDNDelayLine[2].ReadSample( );
	out_fDelayOut[3] = FDNDelayLine[3].ReadSample( );
	// Low-pass attenuation
	out_fDelayOut[0] = delayLowPassFilter[0].ProcessSample( out_fDelayOut[0] );
	out_fDelayOut[1] = delayLowPassFilter[1].ProcessSample( out_fDelayOut[1] );
	out_fDelayOut[2] = delayLowPassFilter[2].ProcessSample( out_fDelayOut[2] );
	out_fDelayOut[3] = delayLowPassFilter[3].ProcessSample( out_fDelayOut[3] );
}

void FDN4::ReadAndAttenuateDelays( AkReal32 out_fDelayOut[4], AkUInt32 io_fCurOffset[4], AkReal32 io_fFFBk1[4] )
{
	out_fDelayOut[0] = FDNDelayLine[0].ReadSample( io_fCurOffset[0] );
	out_fDelayOut[1] = FDNDelayLine[1].ReadSample( io_fCurOffset[1] );
	out_fDelayOut[2] = FDNDelayLine[2].ReadSample( io_fCurOffset[2] );
	out_fDelayOut[3] = FDNDelayLine[3].ReadSample( io_fCurOffset[3] );
	// Low-pass attenuation
	out_fDelayOut[0] = delayLowPassFilter[0].ProcessSample( out_fDelayOut[0], io_fFFBk1[0] );
	out_fDelayOut[1] = delayLowPassFilter[1].ProcessSample( out_fDelayOut[1], io_fFFBk1[1] );
	out_fDelayOut[2] = delayLowPassFilter[2].ProcessSample( out_fDelayOut[2], io_fFFBk1[2] );
	out_fDelayOut[3] = delayLowPassFilter[3].ProcessSample( out_fDelayOut[3], io_fFFBk1[3] );
}

void FDN4::InputReinjection( AkReal32 io_fFeedback[4], AkReal32 in_fIn )
{
	io_fFeedback[0] += in_fIn;
	io_fFeedback[1] += in_fIn;
	io_fFeedback[2] += in_fIn;
	io_fFeedback[3] += in_fIn;

	FDNDelayLine[0].WriteSample( io_fFeedback[0] );
	FDNDelayLine[1].WriteSample( io_fFeedback[1] );
	FDNDelayLine[2].WriteSample( io_fFeedback[2] );
	FDNDelayLine[3].WriteSample( io_fFeedback[3] );
}

void FDN4::InputReinjectionNoWrapCheck( AkReal32 io_fFeedback[4], AkReal32 in_fIn )
{
	io_fFeedback[0] += in_fIn;
	io_fFeedback[1] += in_fIn;
	io_fFeedback[2] += in_fIn;
	io_fFeedback[3] += in_fIn;

	FDNDelayLine[0].WriteSampleNoWrapCheck( io_fFeedback[0] );
	FDNDelayLine[1].WriteSampleNoWrapCheck( io_fFeedback[1] );
	FDNDelayLine[2].WriteSampleNoWrapCheck( io_fFeedback[2] );
	FDNDelayLine[3].WriteSampleNoWrapCheck( io_fFeedback[3] );
}

void FDN4::InputReinjection( AkReal32 io_fFeedback[4], AkReal32 in_fIn, AkUInt32 io_fCurOffset[4] )
{
	io_fFeedback[0] += in_fIn;
	io_fFeedback[1] += in_fIn;
	io_fFeedback[2] += in_fIn;
	io_fFeedback[3] += in_fIn;
	FDNDelayLine[0].WriteSample( io_fFeedback[0], io_fCurOffset[0] );
	FDNDelayLine[1].WriteSample( io_fFeedback[1], io_fCurOffset[1] );
	FDNDelayLine[2].WriteSample( io_fFeedback[2], io_fCurOffset[2] );
	FDNDelayLine[3].WriteSample( io_fFeedback[3], io_fCurOffset[3] );
}

void FDN4::InputReinjectionNoWrapCheck( AkReal32 io_fFeedback[4], AkReal32 in_fIn, AkUInt32 io_fCurOffset[4] )
{
	io_fFeedback[0] += in_fIn;
	io_fFeedback[1] += in_fIn;
	io_fFeedback[2] += in_fIn;
	io_fFeedback[3] += in_fIn;
	FDNDelayLine[0].WriteSampleNoWrapCheck( io_fFeedback[0], io_fCurOffset[0] );
	FDNDelayLine[1].WriteSampleNoWrapCheck( io_fFeedback[1], io_fCurOffset[1] );
	FDNDelayLine[2].WriteSampleNoWrapCheck( io_fFeedback[2], io_fCurOffset[2] );
	FDNDelayLine[3].WriteSampleNoWrapCheck( io_fFeedback[3], io_fCurOffset[3] );
}

#endif