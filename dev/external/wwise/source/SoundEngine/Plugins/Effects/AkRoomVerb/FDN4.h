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

// 4x4 feedback delay network. 
// Use some temporary buffer to process 1 delay line at a time and localize memory access patterns
// Each delay line applies a normalized gain one pole low pass filter y[n] = fB0 * x[n] - fA1 * y[n-1]
// The feedback matrix used is HouseHolder

// Note: Feedback matrix used (implicit) is HouseHolder matrix that maximizes echo density 
// (no zero entries). This matrix recursion can be computed in 2N operation using matrix properties. 
// 4 x 4 using following values -2/N, 1-2/N
// -.5,  .5, -.5, -.5
// -.5, -.5,  .5, -.5
// -.5, -.5, -.5,  .5
//  .5, -.5, -.5, -.5
// Algorithm: 
// 1) Take the sum of all delay outputs d1 feedback = (d1 + d2 + d3 + d4)
// 2) Multiply by -2/N -> d1 feedback = -0.5(d1 + d2 + d3 + d4)
// 3) Add the full output of one delay line further to effectively change the coefficient 
// that was wrong in the previous computation
// i.e. -> d1 feedback = -0.5(d1 + d2 + d3 + d4) + d2 == -.5d1 + .5d2 -.5d3 -.5d4

#ifndef _AKFDN4_H_
#define _AKFDN4_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#ifndef __SPU__
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#endif
#include "FDNLPFilter.h"
#include "DelayLine.h"
#include "SPUInline.h"
#include <AK/SoundEngine/Common/AkSimd.h>

namespace DSP
{
	class FDN4
	{
	public:

#ifndef __SPU__

		AKRESULT Init(	
			AK::IAkPluginMemAlloc * in_pAllocator, 
			AkUInt32 in_uDelayLineLength[4], 
			AkReal32 in_fDecayTime, 
			AkReal32 in_fHFDecayRatio, 
			AkUInt32 in_uSampleRate );
		void Term( AK::IAkPluginMemAlloc * in_pAllocator );
		void Reset( );
		void ProcessBufferAccum(	
			AkReal32 * in_pfInBuffer, 
			AkReal32 * io_pfOutBuffer, 
			AkUInt32 in_uNumFrames );
		void ProcessBufferAccum(	
			AkReal32 * in_pfInBuffer, 
			AkReal32 * io_pfOutBuffer1, 
			AkReal32 * io_pfOutBuffer2, 
			AkUInt32 in_uNumFrames );
		void ProcessBufferAccum(	
			AkReal32 * in_pfInBuffer, 
			AkReal32 * io_pfOutBuffer1, 
			AkReal32 * io_pfOutBuffer2, 
			AkReal32 * io_pfOutBuffer3, 
			AkUInt32 in_uNumFrames );

#ifdef __PPU__
		AkUInt32 GetScratchMemorySizeRequired( AkUInt32 in_uNumFrames );
#endif

#else
		// Position independent code routine
		SPUInline void ProcessBufferAccum(	
			AkReal32 * in_pfInBuffer, 
			AkReal32 * io_pfOutBuffer, 
			AkUInt32 in_uNumFrames, 
			AkReal32 * in_pfDelayCurPos[4], 
			AkReal32 * in_pfDelayWrapPos[4] );

		SPUInline void ProcessBufferAccum(	
			AkReal32 * in_pfInBuffer, 
			AkReal32 * io_pfOutBuffer1,
			AkReal32 * io_pfOutBuffer2, 
			AkUInt32 in_uNumFrames, 
			AkReal32 * in_pfDelayCurPos[4], 
			AkReal32 * in_pfDelayWrapPos[4] );

		SPUInline void ProcessBufferAccum(	
			AkReal32 * in_pfInBuffer, 
			AkReal32 * io_pfOutBuffer1, 
			AkReal32 * io_pfOutBuffer2, 
			AkReal32 * io_pfOutBuffer3, 
			AkUInt32 in_uNumFrames, 
			AkReal32 * in_pfDelayCurPos[4], 
			AkReal32 * in_pfDelayWrapPos[4] );

		SPUInline void GetDelayMemory(		
			AkReal32 * out_pfCurPos[4],
			AkReal32 * out_pfWrapPos[4],
			AkUInt32 out_uCurOffsetBefore[4],
			AkReal32 *& io_pfScratchMemLoc,
			AkUInt32 in_uNumFrames,
			AkUInt32 in_uDMATag );

		SPUInline void PutDelayMemory(		
			AkUInt32 in_uCurOffsetBefore[4],
			AkReal32 *& io_pfScratchMemLoc,
			AkUInt32 in_uNumFrames,
			AkUInt32 in_uDMATag );

#endif
		void ChangeDecay( AkReal32 in_fDecayTime, AkReal32 in_fHFDecayRatio, AkUInt32 in_uSampleRate );

	protected:
		static AkReal32 ComputeMaxStableHFRatio(	
			AkUInt32 in_uDelayLineLength, 
			AkReal32 in_fDecayTime, 
			AkReal32 in_fHFDecayRatio, 
			AkUInt32 in_uSampleRate );

		AkForceInline void ProcessFeedbackMatrix( AkReal32 in_fDelayOut[4], AkReal32 out_fFeedback[4] );

#ifndef __SPU__

		AkForceInline void ReadAndAttenuateDelays( AkReal32 out_fDelayOut[4] );
		AkForceInline void ReadAndAttenuateDelays( AkReal32 out_fDelayOut[4], AkUInt32 io_fCurOffset[4], AkReal32 io_fFFBk1[4] );

		AkForceInline void InputReinjection( AkReal32 io_fFeedback[4], AkReal32 in_fIn );
		AkForceInline void InputReinjectionNoWrapCheck( AkReal32 io_fFeedback[4], AkReal32 in_fIn );
		AkForceInline void InputReinjection( AkReal32 io_fFeedback[4], AkReal32 in_fIn, AkUInt32 io_fCurOffset[4] );
		AkForceInline void InputReinjectionNoWrapCheck( AkReal32 io_fFeedback[4], AkReal32 in_fIn, AkUInt32 io_fCurOffset[4] );
		
#else
		AkForceInline void ReadAndAttenuateDelays( AkReal32 out_fDelayOut[4], AkReal32 * in_pfDelayCurPos[4] );

		AkForceInline void InputReinjection(	
			AkReal32 io_fFeedback[4], 
			AkReal32 in_fIn,
			AkReal32 * io_pfDelayCurPos[4], 
			AkReal32 * in_pfDelayWrapPos[4] );

#endif

#if defined(AKSIMD_V2F32_SUPPORTED)
		AkForceInline void ReadAndAttenuateDelays( AKSIMD_V2F32 &vfDelay0, AKSIMD_V2F32 &vfDelay1, AKSIMD_V2F32 &vfDelay2, AKSIMD_V2F32 &vfDelay3, 
							AKSIMD_V2F32 &vfDelay0_, AKSIMD_V2F32 &vfDelay1_, AKSIMD_V2F32 &vfDelay2_, AKSIMD_V2F32 &vfDelay3_,
							AKSIMD_V2F32 &vfFFbk1, AKSIMD_V2F32 vfB0, AKSIMD_V2F32 vfA1, AKSIMD_V2F32 &vfFFbk1_, AKSIMD_V2F32 vfB0_, AKSIMD_V2F32 vfA1_);
		AkForceInline void FeedbackAndInputReinjection( AKSIMD_V2F32 vfDelay0, AKSIMD_V2F32 vfDelay1, AKSIMD_V2F32 vfDelay2, AKSIMD_V2F32 vfDelay3, AkReal32 * AK_RESTRICT &pfInBuf );

		AkForceInline void ReadAndAttenuateDelays2( AKSIMD_V2F32 &vfDelayOut, AKSIMD_V2F32 &vfDelayOut_, AKSIMD_V2F32 vfB0, AKSIMD_V2F32 &vfFFbk1, AKSIMD_V2F32 vfA1, AKSIMD_V2F32 vfB0_, AKSIMD_V2F32 &vfFFbk1_, AKSIMD_V2F32 vfA1_ );
		AkForceInline void FeedbackAndReinjection2( AKSIMD_V2F32 vfPartialSum, AKSIMD_V2F32 &vfDelayOut, AKSIMD_V2F32 &vfDelayOut_, AkReal32 * AK_RESTRICT &pfInBuf);
#elif defined(AKSIMD_V4F32_SUPPORTED)
		AkForceInline void ApplyLowPassFilter( AKSIMD_V4F32 &vfTime0, AKSIMD_V4F32 &vfTime1, AKSIMD_V4F32 &vfTime2, AKSIMD_V4F32 &vfTime3, AKSIMD_V4F32 &vfB0, AKSIMD_V4F32 &vfA1, AKSIMD_V4F32 &vfFFbk );
		AkForceInline void ReadAndAttenuateDelays( AKSIMD_V4F32 &vfDelay0, AKSIMD_V4F32 &vfDelay1, AKSIMD_V4F32 &vfDelay2, AKSIMD_V4F32 &vfDelay3, AKSIMD_V4F32 &vfFFbk, AKSIMD_V4F32 &vfB0, AKSIMD_V4F32 &vfA1 );
		AkForceInline void FeedbackAndInputReinjection( AKSIMD_V4F32 &vfDelay0, AKSIMD_V4F32 &vfDelay1, AKSIMD_V4F32 &vfDelay2, AKSIMD_V4F32 &vfDelay3, AkReal32 * AK_RESTRICT &pfInBuf );
		AkForceInline void ReadAndAttenuateDelays2( AKSIMD_V4F32 &vfDelayOut, AKSIMD_V4F32 &vfB0, AKSIMD_V4F32 &vfFFbk, AKSIMD_V4F32 &vfA1 );
	#if defined(AK_CPU_ARM_NEON)
		AkForceInline void FeedbackAndReinjection2( AKSIMD_V2F32 &vfPartialSum, AKSIMD_V4F32 &vfDelayOut, AkReal32 * AK_RESTRICT &pfInBuf);
	#else
		AkForceInline void FeedbackAndReinjection2( AKSIMD_V4F32 &vfDelayOut, AkReal32 * AK_RESTRICT &pfInBuf);
	#endif
#endif

#if defined(AKSIMD_V2F32_SUPPORTED) || defined(AKSIMD_V4F32_SUPPORTED)
		//Generic function for the FDN process.  The goal of the template is to keep the same code 
		//for the 3 variants: single, dual and triple output buffers.  
		//Normally, if the compiler does its job all the code should be inlined properly, as in the non-templated version.
		//The only difference between the 3 versions is the way SumToOutputBuffers is done.  This is now handled by the 
		//policy class for each of the 3 versions.
		//This way is less trouble to maintain, there only one copy of the code :)
		template<class OutputPolicy>
		AkForceInline void GenericProcessBuffer(AkReal32 * in_pfInBuffer, AkUInt32 in_uNumFrames, OutputPolicy in_Output);
#endif

		DSP::DelayLine FDNDelayLine[4];
		FDNLPFilter delayLowPassFilter[4];
	};
}
#endif // _AKFDN4_H_
