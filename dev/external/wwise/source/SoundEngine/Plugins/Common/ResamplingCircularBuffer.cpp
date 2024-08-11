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

#include "ResamplingCircularBuffer.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

namespace DSP
{
#ifndef __SPU__
	void CAkResamplingCircularBuffer::Reset( )
	{
		CAkCircularBuffer::Reset();
		m_fPastVal = 0.f;
#ifndef AKCIRCULARBUFFER_USESIMD
		m_fInterpLoc = 0.f;
#else
		m_uFloatIndex = SINGLEFRAMEDISTANCE; // Initial index set to 1 -> 0 == previous buffer
#endif
	}

	// Tries to push all frames from a given buffer into circular buffer without overwriting data not yet read. 
	// Actual number of frames pushed returned
	AkUInt32 CAkResamplingCircularBuffer::PushFrames( 
		AkReal32 * in_pfBuffer, 
		AkUInt32 in_NumFrames,
		AkReal32 in_fResamplingFactor )
	{
		return PushFrames( in_pfBuffer, in_NumFrames, m_pfData, in_fResamplingFactor );
	}
#endif

#ifndef AKCIRCULARBUFFER_USESIMD
	// Tries to push all frames from a given buffer into circular buffer without overwriting data not yet read.
	// Actual number of frames consumed returned
	// Resamples data befor pushing to circular buffer
	AkUInt32 CAkResamplingCircularBuffer::PushFrames( 
		AkReal32 * in_pfBuffer, 
		AkUInt32 in_NumFrames,
		AkReal32 * io_pfData,
		AkReal32 in_fResamplingFactor )
	{
		AkUInt32 uBufferFramesAvailable = FramesEmpty();
		const AkUInt16 uInBufferFrames = (AkUInt16)in_NumFrames;
		const AkUInt32 uOutBufferFrames = uBufferFramesAvailable;
		AkUInt32 uWriteOffset = m_uWriteOffset;
		if ( uOutBufferFrames == 0 )
			return 0;

		AkReal32 fInterpLoc = m_fInterpLoc;
		AkReal32 fFrameSkip = in_fResamplingFactor;
		AkUInt16 uFramesConsumed = 0;
		AkUInt16 uFramesProduced = 0;
		AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) in_pfBuffer; 
		AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pfData;

		while ( true )
		{
			AkReal32 fPrevSample;
			AkReal32 fNextSample;
			if ( fInterpLoc < 0.f )
			{
				fPrevSample = m_fPastVal;
				fNextSample = pInBuf[0];
			}
			else
			{
				AkUInt16 uIndex = (AkUInt16)fInterpLoc;		
				// Check input condition
				if ( uIndex >= (uInBufferFrames-1) )
				{
					if ( uIndex > (uInBufferFrames-1) )
					{
						// Both previous and next frames are in the next buffer
						// Keep only fractional part for next buffer
						// No need to cache previous sample
						fInterpLoc -= (AkReal32)uInBufferFrames;
					}
					else
					{
						// Previous frame in this buffer, but next frame only in next buffer
						// Cache and consume previous so that it can be interpolated with next on subsequent iteration
						m_fPastVal = pInBuf[uIndex];
						fInterpLoc -= (AkReal32)(uIndex+1);
					}	
					uFramesConsumed = uInBufferFrames;
					break;
				}

				// In all other cases we have both samples
				fPrevSample = pInBuf[uIndex];
				fNextSample = pInBuf[uIndex+1];
			}
			// Check output condition
			if ( uFramesProduced == uOutBufferFrames )
			{
				// Cache and consume previous so that it can be interpolated with next on subsequent iteration
				m_fPastVal = fPrevSample;
				if ( fInterpLoc > 0.f )
				{
					AkUInt16 uIndex = (AkUInt16)fInterpLoc;
					fInterpLoc -= (AkReal32)(uIndex+1);
					uFramesConsumed = uIndex+1;
				}
				break;
			}

			AkReal32 fSampleDiff = fNextSample - fPrevSample;
			AkReal32 fXDistPrevSample;
			if ( fInterpLoc >= 0.f )
				fXDistPrevSample = fInterpLoc - (AkUInt32)fInterpLoc;
			else
				fXDistPrevSample = 1.f + fInterpLoc;
			pfOutBuf[uWriteOffset] = fPrevSample + ( fXDistPrevSample * fSampleDiff );
			uFramesProduced++;
			fInterpLoc += fFrameSkip;	
			// Advance and wrap circular buffer
			uWriteOffset++;
			if ( uWriteOffset == m_uSize )
				uWriteOffset = 0;

		}

		m_fInterpLoc = fInterpLoc;
		m_uWriteOffset = uWriteOffset;
		m_uFramesReady += uFramesProduced;
#ifdef CIRCULARBUFFER_DEBUGINFO
		m_uTotalFramesPushed += uFramesProduced;
#endif
		return uFramesConsumed;
	}
#else
	// Tries to push all frames from a given buffer into circular buffer without overwriting data not yet read.
	// Actual number of frames consumed returned
	// Resamples data befor pushing to circular buffer
	AkUInt32 CAkResamplingCircularBuffer::PushFrames( 
		AkReal32 * in_pfBuffer, 
		AkUInt32 in_NumFrames,
		AkReal32 * io_pfData,
		AkReal32 in_fResamplingFactor )
	{
		AkUInt32 uBufferFramesAvailable = FramesEmpty();
		const AkUInt16 uInBufferFrames = (AkUInt16)in_NumFrames;
		const AkUInt32 uOutBufferFrames = uBufferFramesAvailable;
		AkUInt32 uWriteOffset = m_uWriteOffset;
		const AkUInt32 uSize = m_uSize;

		// Minus one to compensate for offset of 1 due to zero == previous
		AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) in_pfBuffer - 1; 
		AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pfData;
		const AkUInt32 uFrameSkipFP = (AkUInt32) ( in_fResamplingFactor * FPMUL + 0.5 ) ; // round;
		const AkUInt32 uNumIterPreviousFrame = AkMin( (SINGLEFRAMEDISTANCE - m_uFloatIndex + (uFrameSkipFP-1)) / uFrameSkipFP, uOutBufferFrames);
		__vector4 vu4FrameSkipFP = __lvlx( &uFrameSkipFP, 0 );
		vu4FrameSkipFP = __vspltw( vu4FrameSkipFP, 0 );
		static const __vector4 vu4AndFPMASK = { 9.1834e-041 /*FPMASK*/, 9.1834e-041 /*FPMASK*/, 9.1834e-041 /*FPMASK*/, 9.1834e-041 /*FPMASK*/ };
		AkUInt32 uNumIterThisFrame;	
		__vector4 vu4IndexFP;

		vu4IndexFP = __lvlx( &m_uFloatIndex, 0 );
		vu4IndexFP = __vspltw( vu4IndexFP, 0 );
		__vector4 vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
		__vector4 vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	

		// Use stored value as left value, while right index is on the first sample
		__vector4 v4PreviousFrame = __lvlx( &m_fPastVal, 0 );
		__vector4 v4CurrentFrame = __lvlx( &pInBuf[1], 0 );
		AkUInt32 uIterFrames = uNumIterPreviousFrame;
		while ( uIterFrames-- )
		{	
			// Linear interpolation
			__vector4 v4FrameDiff = __vsubfp( v4CurrentFrame, v4PreviousFrame );
			__vector4 v4Out = __vmaddfp( v4FrameDiff, vf4InterpLoc, v4PreviousFrame );
			// Store, advance and wrap circular buffer
			__stvlx( v4Out, pfOutBuf, uWriteOffset*sizeof(AkReal32) );
			uWriteOffset++;
			if ( uWriteOffset == uSize )
				uWriteOffset = 0;
			// Index advance and interpolation factor computation
			vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
			vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
			vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
		}

		// Determine number of iterations remaining
		const AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - vu4IndexFP.u[0] + (uFrameSkipFP-1)) / uFrameSkipFP;
		uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

		// For all other sample frames
		AkUInt32 uTotalReaminingIterations = uNumIterThisFrame;
		while ( uTotalReaminingIterations )
		{
			AkUInt32 uFramesBeforeWrap = uSize - uWriteOffset;
			uIterFrames = AkMin( uTotalReaminingIterations, uFramesBeforeWrap );
			uTotalReaminingIterations -= uIterFrames;
			while ( uIterFrames-- )
			{
				AkUInt32 uPreviousFrameIndex = (vu4IndexFP.u[0] >> FPBITS);
				v4PreviousFrame = __lvlx( &pInBuf[uPreviousFrameIndex], 0 );
				v4CurrentFrame = __lvlx( &pInBuf[uPreviousFrameIndex+1], 0 );
				// Linear interpolation
				__vector4 v4FrameDiff = __vsubfp( v4CurrentFrame, v4PreviousFrame );
				__vector4 v4Out = __vmaddfp( v4FrameDiff, vf4InterpLoc, v4PreviousFrame );
				// Store, advance and wrap circular buffer
				__stvlx( v4Out, pfOutBuf, uWriteOffset*sizeof(AkReal32) );
				uWriteOffset++;
				// Index advance and interpolation factor computation
				vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
				vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
				vf4InterpLoc = __vcuxwfp( vu4InterpLocFP, FPBITS );	
			}
			if ( uWriteOffset == uSize )
				uWriteOffset = 0;	
		}	

		const AkUInt32 uFramesConsumed = AkMin( vu4IndexFP.u[0] >> FPBITS, uInBufferFrames ); 
		if ( uFramesConsumed )
		{
			AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) in_pfBuffer - 1; 
			m_fPastVal = pInBuf[uFramesConsumed];
		}

		AKASSERT( vu4IndexFP.u[0] >= uFramesConsumed * FPMUL );
		m_uFloatIndex = vu4IndexFP.u[0] - uFramesConsumed * FPMUL;
		AkUInt32 uFramesProduced = uNumIterPreviousFrame + uNumIterThisFrame;
		AKASSERT( uFramesProduced <= uOutBufferFrames );
		m_uWriteOffset = uWriteOffset;
		m_uFramesReady += uFramesProduced;
#ifdef CIRCULARBUFFER_DEBUGINFO
		m_uTotalFramesPushed += uFramesProduced;
#endif
		return uFramesConsumed;
	}
#endif

	
} // namespace DSP
