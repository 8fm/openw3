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
// AkResamplerWin.cpp
// 
// Windows specific code
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h" 
#include "AkResamplerCommon.h"
#include <AK/Tools/Common/AkPlatformFuncs.h> 
#if defined( AK_CPU_X86 ) && !defined(AK_IOS)
#define AK_MMX
#endif
#define AK_SSE2
#include <AK/SoundEngine/Common/AkSimd.h>

/********************* BYPASS DSP ROUTINES **********************/

// Bypass (no pitch or resampling) with INTERLEAVED signed 16-bit samples optimized for 2 channel signals. 
#if defined( AK_CPU_X86 ) && !defined(AK_IOS)
AKRESULT Bypass_I16_2Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_BYPASS_DSP_SETUP( );

	// IMPORTANT: Second channel access relies on the fact that at least the 
	// first 2 channels of AkAudioBuffer are contiguous in memory.
	AkInt16 * AK_RESTRICT pIn = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData() ) + 2*io_pPitchState->uInFrameOffset;
	AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 0 ) ) + io_pPitchState->uOutFrameOffset;

	// Need to keep the last buffer value in case we start the pitch algo next buffer
	io_pPitchState->iLastValue[0] = pIn[2*uLastSample];
	io_pPitchState->iLastValue[1] = pIn[2*uLastSample+1];

	AkUInt32 uNumIter = uFramesToCopy / 16;
	AkUInt32 uRemaining = uFramesToCopy - (uNumIter*16);

	AKSIMD_V2F32 * AK_RESTRICT pm64In = (AKSIMD_V2F32* AK_RESTRICT) pIn;
	AKSIMD_V4F32 * AK_RESTRICT pm128Out = (AKSIMD_V4F32* AK_RESTRICT) pOut;

	const AKSIMD_V2F32 * AK_RESTRICT pm64InEnd = pm64In + 8*uNumIter;
	const AKSIMD_V4F32 m128Scale = AKSIMD_SET_V4F32( NORMALIZEFACTORI16 );
	const AKSIMD_V4F32 m128Zero = AKSIMD_SETZERO_V4F32();

	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames()/4;

	// Process blocks first
	while ( pm64In < pm64InEnd )
	{
		AKSIMD_V2F32 m64InSamples1 = pm64In[0];	
		AKSIMD_V2F32 m64InSamples2 = pm64In[1];
		AKSIMD_V2F32 m64InSamplesLLow = _mm_slli_pi32( m64InSamples1, 16 );  // Prepare left samples for conversion
		AKSIMD_V2F32 m64InSamplesRLow = _mm_srai_pi32( m64InSamples1, 16 );  // Prepare right samples for conversion
		m64InSamplesLLow = _mm_srai_pi32( m64InSamplesLLow, 16 ); 		
		AKSIMD_V2F32 m64InSamplesLHi = _mm_slli_pi32( m64InSamples2, 16 );  // Prepare left samples for conversion
		AKSIMD_V2F32 m64InSamplesRHi = _mm_srai_pi32( m64InSamples2, 16 );  // Prepare right samples for conversion
		m64InSamplesLHi = _mm_srai_pi32( m64InSamplesLHi, 16 ); 
		AKSIMD_V4F32 m128OutL = _mm_cvtpi32_ps(m128Zero, m64InSamplesLHi );
		AKSIMD_V4F32 m128OutR = _mm_cvtpi32_ps(m128Zero, m64InSamplesRHi );
		m128OutL = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutL, m128OutL ), m64InSamplesLLow );
		m128OutR = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutR, m128OutR ), m64InSamplesRLow );
		AKSIMD_STOREU_V4F32( (AkReal32 *)pm128Out, AKSIMD_MUL_V4F32( m128OutL, m128Scale ) ); 
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[uMaxFrames], AKSIMD_MUL_V4F32( m128OutR, m128Scale ) ); 

		m64InSamples1 = pm64In[2];	
		m64InSamples2 = pm64In[3];
		m64InSamplesLLow = _mm_slli_pi32( m64InSamples1, 16 );  // Prepare left samples for conversion
		m64InSamplesRLow = _mm_srai_pi32( m64InSamples1, 16 );  // Prepare right samples for conversion
		m64InSamplesLLow = _mm_srai_pi32( m64InSamplesLLow, 16 ); 		
		m64InSamplesLHi = _mm_slli_pi32( m64InSamples2, 16 );  // Prepare left samples for conversion
		m64InSamplesRHi = _mm_srai_pi32( m64InSamples2, 16 );  // Prepare right samples for conversion
		m64InSamplesLHi = _mm_srai_pi32( m64InSamplesLHi, 16 ); 
		m128OutL = _mm_cvtpi32_ps(m128Zero, m64InSamplesLHi );
		m128OutR = _mm_cvtpi32_ps(m128Zero, m64InSamplesRHi );
		m128OutL = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutL, m128OutL ), m64InSamplesLLow );
		m128OutR = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutR, m128OutR ), m64InSamplesRLow );
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[1], AKSIMD_MUL_V4F32( m128OutL, m128Scale ) ); 
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[1+uMaxFrames], AKSIMD_MUL_V4F32( m128OutR, m128Scale ) ); 

		m64InSamples1 = pm64In[4];	
		m64InSamples2 = pm64In[5];
		m64InSamplesLLow = _mm_slli_pi32( m64InSamples1, 16 );  // Prepare left samples for conversion
		m64InSamplesRLow = _mm_srai_pi32( m64InSamples1, 16 );  // Prepare right samples for conversion
		m64InSamplesLLow = _mm_srai_pi32( m64InSamplesLLow, 16 ); 		
		m64InSamplesLHi = _mm_slli_pi32( m64InSamples2, 16 );  // Prepare left samples for conversion
		m64InSamplesRHi = _mm_srai_pi32( m64InSamples2, 16 );  // Prepare right samples for conversion
		m64InSamplesLHi = _mm_srai_pi32( m64InSamplesLHi, 16 ); 
		m128OutL = _mm_cvtpi32_ps(m128Zero, m64InSamplesLHi );
		m128OutR = _mm_cvtpi32_ps(m128Zero, m64InSamplesRHi );
		m128OutL = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutL, m128OutL ), m64InSamplesLLow );
		m128OutR = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutR, m128OutR ), m64InSamplesRLow );
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[2], AKSIMD_MUL_V4F32( m128OutL, m128Scale ) ); 
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[2+uMaxFrames], AKSIMD_MUL_V4F32( m128OutR, m128Scale ) ); 

		m64InSamples1 = pm64In[6];	
		m64InSamples2 = pm64In[7];
		m64InSamplesLLow = _mm_slli_pi32( m64InSamples1, 16 );  // Prepare left samples for conversion
		m64InSamplesRLow = _mm_srai_pi32( m64InSamples1, 16 );  // Prepare right samples for conversion
		m64InSamplesLLow = _mm_srai_pi32( m64InSamplesLLow, 16 ); 		
		m64InSamplesLHi = _mm_slli_pi32( m64InSamples2, 16 );  // Prepare left samples for conversion
		m64InSamplesRHi = _mm_srai_pi32( m64InSamples2, 16 );  // Prepare right samples for conversion
		m64InSamplesLHi = _mm_srai_pi32( m64InSamplesLHi, 16 ); 
		m128OutL = _mm_cvtpi32_ps(m128Zero, m64InSamplesLHi );
		m128OutR = _mm_cvtpi32_ps(m128Zero, m64InSamplesRHi );
		m128OutL = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutL, m128OutL ), m64InSamplesLLow );
		m128OutR = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutR, m128OutR ), m64InSamplesRLow );
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[3], AKSIMD_MUL_V4F32( m128OutL, m128Scale ) ); 
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[3+uMaxFrames], AKSIMD_MUL_V4F32( m128OutR, m128Scale ) ); 

		pm64In += 8;
		pm128Out += 4;
	}

	_mm_empty();
	pIn = (AkInt16 * AK_RESTRICT) pm64In;
	pOut = (AkReal32* AK_RESTRICT) pm128Out;

	// Process blocks of 4 first
	uMaxFrames = io_pOutBuffer->MaxFrames();
	while ( uRemaining-- )
	{	
		*pOut = INT16_TO_FLOAT( *pIn++ );
		pOut[uMaxFrames] = INT16_TO_FLOAT( *pIn++ );
		++pOut;
	}

	PITCH_BYPASS_DSP_TEARDOWN( );
}
#endif

// Bypass (no pitch or resampling) with INTERLEAVED signed 16-bit samples optimized for 2 channel signals. 
AKRESULT Bypass_I16_2ChanSSE2(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_BYPASS_DSP_SETUP( );

	// IMPORTANT: Second channel access relies on the fact that at least the 
	// first 2 channels of AkAudioBuffer are contiguous in memory.
	AkInt16 * AK_RESTRICT pIn = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData() ) + 2*io_pPitchState->uInFrameOffset;
	AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 0 ) ) + io_pPitchState->uOutFrameOffset;

	// Need to keep the last buffer value in case we start the pitch algo next buffer
	io_pPitchState->iLastValue[0] = pIn[2*uLastSample];
	io_pPitchState->iLastValue[1] = pIn[2*uLastSample+1];

	AkUInt32 uNumIter = uFramesToCopy / 16;
	AkUInt32 uRemaining = uFramesToCopy - (uNumIter*16);

	AKSIMD_V4I32 AK_UNALIGNED * AK_RESTRICT pm128In = (AKSIMD_V4I32* AK_RESTRICT) pIn;
	AKSIMD_V4F32 * AK_RESTRICT pm128Out = (AKSIMD_V4F32* AK_RESTRICT) pOut;

	const AKSIMD_V4I32* AK_RESTRICT pm128InEnd = pm128In + 4*uNumIter;
	const AKSIMD_V4F32 m128Scale = AKSIMD_SET_V4F32( NORMALIZEFACTORI16 );
	
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames()/4;

	// Process blocks first
	while ( pm128In < pm128InEnd )
	{
		AKSIMD_V4I32 m128InSamples = AKSIMD_LOADU_V4I32 (& pm128In[0]);
		AKSIMD_V4I32 m128InSamplesL = AKSIMD_SHIFTRIGHTARITH_V4I32(AKSIMD_SHIFTLEFT_V4I32( m128InSamples, 16 ), 16);	// Prepare left samples for conversion
		AKSIMD_V4I32 m128InSamplesR = AKSIMD_SHIFTRIGHTARITH_V4I32( m128InSamples, 16 );						// Prepare right samples for conversion
		AKSIMD_V4F32 m128OutL = AKSIMD_CONVERT_V4I32_TO_V4F32(m128InSamplesL);									// Convert Int32 to floats
		AKSIMD_V4F32 m128OutR = AKSIMD_CONVERT_V4I32_TO_V4F32(m128InSamplesR);									// Convert Int32 to floats
		AKSIMD_STOREU_V4F32( (AkReal32 *)pm128Out, AKSIMD_MUL_V4F32( m128OutL, m128Scale ) ); 
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[uMaxFrames], AKSIMD_MUL_V4F32( m128OutR, m128Scale ) ); 

		m128InSamples = AKSIMD_LOADU_V4I32 (& pm128In[1]);
		m128InSamplesL = AKSIMD_SHIFTRIGHTARITH_V4I32(AKSIMD_SHIFTLEFT_V4I32( m128InSamples, 16 ), 16);			// Prepare left samples for conversion
		m128InSamplesR = AKSIMD_SHIFTRIGHTARITH_V4I32( m128InSamples, 16 );								// Prepare right samples for conversion
		m128OutL = AKSIMD_CONVERT_V4I32_TO_V4F32(m128InSamplesL);											// Convert Int32 to floats
		m128OutR = AKSIMD_CONVERT_V4I32_TO_V4F32(m128InSamplesR);											// Convert Int32 to floats
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[1], AKSIMD_MUL_V4F32( m128OutL, m128Scale ) ); 
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[1+uMaxFrames], AKSIMD_MUL_V4F32( m128OutR, m128Scale ) ); 

		m128InSamples = AKSIMD_LOADU_V4I32 (& pm128In[2]);
		m128InSamplesL = AKSIMD_SHIFTRIGHTARITH_V4I32(AKSIMD_SHIFTLEFT_V4I32( m128InSamples, 16 ), 16);			// Prepare left samples for conversion
		m128InSamplesR = AKSIMD_SHIFTRIGHTARITH_V4I32( m128InSamples, 16 );								// Prepare right samples for conversion
		m128OutL = AKSIMD_CONVERT_V4I32_TO_V4F32(m128InSamplesL);											// Convert Int32 to floats
		m128OutR = AKSIMD_CONVERT_V4I32_TO_V4F32(m128InSamplesR);											// Convert Int32 to floats
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[2], AKSIMD_MUL_V4F32( m128OutL, m128Scale ) ); 
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[2+uMaxFrames], AKSIMD_MUL_V4F32( m128OutR, m128Scale ) ); 

		m128InSamples = AKSIMD_LOADU_V4I32 (& pm128In[3]);
		m128InSamplesL = AKSIMD_SHIFTRIGHTARITH_V4I32(AKSIMD_SHIFTLEFT_V4I32( m128InSamples, 16 ), 16);			// Prepare left samples for conversion
		m128InSamplesR = AKSIMD_SHIFTRIGHTARITH_V4I32( m128InSamples, 16 );								// Prepare right samples for conversion
		m128OutL = AKSIMD_CONVERT_V4I32_TO_V4F32(m128InSamplesL);											// Convert Int32 to floats
		m128OutR = AKSIMD_CONVERT_V4I32_TO_V4F32(m128InSamplesR);											// Convert Int32 to floats
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[3], AKSIMD_MUL_V4F32( m128OutL, m128Scale ) ); 
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[3+uMaxFrames], AKSIMD_MUL_V4F32( m128OutR, m128Scale ) ); 

		pm128In += 4;
		pm128Out += 4;
	}

	pIn = (AkInt16 * AK_RESTRICT) pm128In;
	pOut = (AkReal32* AK_RESTRICT) pm128Out;

	// Process blocks of 4 first
	uMaxFrames = io_pOutBuffer->MaxFrames();
	while ( uRemaining-- )
	{	
		*pOut = INT16_TO_FLOAT( *pIn++ );
		pOut[uMaxFrames] = INT16_TO_FLOAT( *pIn++ );
		++pOut;
	}

	PITCH_BYPASS_DSP_TEARDOWN( );
}

// Bypass (no pitch or resampling) with signed 16-bit samples vectorized for any number of channels.
// As opposed to other routines, this only does the format conversion and deinterleaving is performed once 
// whole buffer is ready to be sent downstream to the pipeline
#if defined( AK_CPU_X86 ) && !defined(AK_IOS)
AKRESULT Bypass_I16_NChanVec(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState )
{
	PITCH_BYPASS_DSP_SETUP( );

	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	AkInt16 * AK_RESTRICT pIn = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset*uNumChannels;
	AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetInterleavedData( ) ) + io_pPitchState->uOutFrameOffset*uNumChannels;
	
	// Note: No input data constraint (MMX)
	// Note: Cannot assume output data alignment is on 16 byte boundaries
	const AkUInt32 uSamplesToCopy = uFramesToCopy*uNumChannels;
	const AkUInt32 uNumIter = uSamplesToCopy / 16;
	AkUInt32 uRemaining = uSamplesToCopy - (uNumIter*16);
	
	AKSIMD_V2F32 * AK_RESTRICT pm64In = (AKSIMD_V2F32 * AK_RESTRICT) pIn;
	AKSIMD_V4F32 * AK_RESTRICT pm128Out = (AKSIMD_V4F32 * AK_RESTRICT) pOut;
	const AKSIMD_V2F32 * AK_RESTRICT pm64InEnd = pm64In + 4*uNumIter;

	const AKSIMD_V4F32 m128Scale = AKSIMD_SET_V4F32( NORMALIZEFACTORI16 );
	const AKSIMD_V2F32 m64Zero = _mm_setzero_si64();
	const AKSIMD_V4F32 m128Zero = AKSIMD_SETZERO_V4F32();

	// Process blocks of 16 frames
	while ( pm64In < pm64InEnd )
	{
		AKSIMD_V2F32 m64InSamples1 = pm64In[0];
		AKSIMD_V2F32 m64InSamples2 = pm64In[1];
		AKSIMD_V2F32 m64InSamples3 = pm64In[2];
		AKSIMD_V2F32 m64InSamples4 = pm64In[3];
		pm64In += 4;
		
		AKSIMD_V2F32 m64Sign = _mm_cmpgt_pi16(m64Zero,m64InSamples1);									// Retrieve sign for proper sign extension	
		AKSIMD_V4F32 m128Tmp = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64InSamples1, m64Sign));	// Interleave to 32 bits and convert (hi)
		AKSIMD_V4F32 m128Out1 = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp, m128Tmp), _mm_unpacklo_pi16(m64InSamples1, m64Sign)); // Interleave to 32 bits and convert (lo) and merge with previous result by shifting up high part

		m64Sign = _mm_cmpgt_pi16(m64Zero,m64InSamples2);				
		m128Tmp = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64InSamples2, m64Sign));	
		AKSIMD_V4F32 m128Out2 = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp, m128Tmp), _mm_unpacklo_pi16(m64InSamples2, m64Sign));

		m64Sign = _mm_cmpgt_pi16(m64Zero,m64InSamples3);				
		m128Tmp = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64InSamples3, m64Sign));	
		AKSIMD_V4F32 m128Out3 = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp, m128Tmp), _mm_unpacklo_pi16(m64InSamples3, m64Sign));

		m64Sign = _mm_cmpgt_pi16(m64Zero,m64InSamples4);				
		m128Tmp = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64InSamples4, m64Sign));	
		AKSIMD_V4F32 m128Out4 = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp, m128Tmp), _mm_unpacklo_pi16(m64InSamples4, m64Sign));

		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[0], AKSIMD_MUL_V4F32( m128Out1, m128Scale ) ); 
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[1], AKSIMD_MUL_V4F32( m128Out2, m128Scale ) ); 
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[2], AKSIMD_MUL_V4F32( m128Out3, m128Scale ) ); 
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[3], AKSIMD_MUL_V4F32( m128Out4, m128Scale ) ); 
		pm128Out += 4;
	}

	_mm_empty();

	// Advance data pointers for remaining samples
	pIn = (AkInt16 * AK_RESTRICT) pm64In;
	pOut = (AkReal32 * AK_RESTRICT) pm128Out;

	// Deal with remaining samples
	while ( uRemaining-- )
	{
		*pOut++ = INT16_TO_FLOAT( *pIn++ ) ;
	}

	// Need to keep the last buffer value in case we start the pitch algo next buffer for each channel
	pIn -= uNumChannels;
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		io_pPitchState->iLastValue[i] = pIn[i];
	}

	PITCH_BYPASS_DSP_TEARDOWN( );
}
#endif

AKRESULT Bypass_I16_NChanVecSSE2(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState )
{
	PITCH_BYPASS_DSP_SETUP( );

	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	AkInt16 * AK_RESTRICT pIn = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset*uNumChannels;
	AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetInterleavedData( ) ) + io_pPitchState->uOutFrameOffset*uNumChannels;
	// Note: No input data constraint (MMX)
	// Note: Cannot assume output data alignment is on 16 byte boundaries
	const AkUInt32 uSamplesToCopy = uFramesToCopy*uNumChannels;
	const AkUInt32 uNumIter = uSamplesToCopy / 16;
	AkUInt32 uRemaining = uSamplesToCopy - (uNumIter*16);
	
	AKSIMD_V4I32 AK_UNALIGNED * AK_RESTRICT pm128In = (AKSIMD_V4I32 * AK_RESTRICT) pIn;
	AKSIMD_V4F32 * AK_RESTRICT pm128Out = (AKSIMD_V4F32 * AK_RESTRICT) pOut;
	const AKSIMD_V4I32 * AK_RESTRICT pm128InEnd = pm128In + 2*uNumIter;

	const AKSIMD_V4F32 m128Scale = AKSIMD_SET_V4F32( NORMALIZEFACTORI16 );
	const AKSIMD_V4I32 m128iZero = AKSIMD_SETZERO_V4I32();

	// Process blocks of 16 frames
	while ( pm128In < pm128InEnd )
	{
		AKSIMD_V4I32 m128InSamples1 = AKSIMD_LOADU_V4I32 (& pm128In[0] );
		AKSIMD_V4I32 m128InSamples2 = AKSIMD_LOADU_V4I32 (& pm128In[1] );
		/*AKSIMD_V4I32 m128InSamples1 = pm128In[0];
		AKSIMD_V4I32 m128InSamples2 = pm128In[1];*/
			
		pm128In += 2;

		AKSIMD_V4I32 m128Sign = AKSIMD_CMPGT_V8I16(m128iZero, m128InSamples1);				// Retrieve sign for proper sign extension	
		AKSIMD_V4I32 m128InSamplesLo = AKSIMD_UNPACKLO_VECTOR8I16(m128InSamples1, m128Sign);		// Interleave (lo) sign and int16 to int 32 bits
		AKSIMD_V4I32 m128InSamplesHi = AKSIMD_UNPACKHI_VECTOR8I16(m128InSamples1, m128Sign);		// Interleave (hi) sign and int16 to int 32 bits
		AKSIMD_V4F32 m128out1 = AKSIMD_CONVERT_V4I32_TO_V4F32(m128InSamplesLo);							// Convert int 32 bits to floats
		AKSIMD_V4F32 m128out2 = AKSIMD_CONVERT_V4I32_TO_V4F32(m128InSamplesHi);							// Convert int 32 bits to floats

		 m128Sign = AKSIMD_CMPGT_V8I16(m128iZero, m128InSamples2);						// Retrieve sign for proper sign extension	
		 m128InSamplesLo = AKSIMD_UNPACKLO_VECTOR8I16(m128InSamples2, m128Sign);			// Interleave (lo) sign and int16 to int 32 bits
		 m128InSamplesHi = AKSIMD_UNPACKHI_VECTOR8I16(m128InSamples2, m128Sign);			// Interleave (hi) sign and int16 to int 32 bits
		AKSIMD_V4F32 m128out3 = AKSIMD_CONVERT_V4I32_TO_V4F32(m128InSamplesLo);							// Convert int 32 bits to floats
		AKSIMD_V4F32 m128out4 = AKSIMD_CONVERT_V4I32_TO_V4F32(m128InSamplesHi);							// Convert int 32 bits to floats

		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[0], AKSIMD_MUL_V4F32( m128out1, m128Scale ) ); 
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[1], AKSIMD_MUL_V4F32( m128out2, m128Scale ) ); 
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[2], AKSIMD_MUL_V4F32( m128out3, m128Scale ) ); 
		AKSIMD_STOREU_V4F32( (AkReal32 *)&pm128Out[3], AKSIMD_MUL_V4F32( m128out4, m128Scale ) ); 
		pm128Out += 4;
	}

	// Advance data pointers for remaining samples
	pIn = (AkInt16 * AK_RESTRICT) pm128In;
	pOut = (AkReal32 * AK_RESTRICT) pm128Out;

	// Deal with remaining samples
	while ( uRemaining-- )
	{
		*pOut++ = INT16_TO_FLOAT( *pIn++ ) ;
	}

	// Need to keep the last buffer value in case we start the pitch algo next buffer for each channel
	pIn -= uNumChannels;
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		io_pPitchState->iLastValue[i] = pIn[i];
	}

	PITCH_BYPASS_DSP_TEARDOWN( );
}

///********************* FIXED RESAMPLING DSP ROUTINES **********************/

// Fixed resampling (no pitch changes) with signed 16-bit samples optimized for one channel signals.

#if defined(AK_CPU_ARM_NEON)

#define FETCH_FRAMES_1CHAN( _i ) \
	{AkInt16 iPrev = pInBuf[uPreviousFrameIndex]; \
	AkInt16 iNext = pInBuf[uPreviousFrameIndex+1]; \
	prevFrames[_i] = iPrev; \
	nextFrames[_i] = iNext;} \
	interpLocFP[_i] = uIndexFP & FPMASK; \
	uIndexFP += uFrameSkipFP; \
	uPreviousFrameIndex = uIndexFP >> FPBITS;

#define FETCH_FRAMES_2CHAN( _i ) \
	{AkUInt32 uPreviousFrameSamplePosL = uPreviousFrameIndex*2; \
	AkInt16 iPrevL = pInBuf[uPreviousFrameSamplePosL]; \
	AkInt16 iPrevR = pInBuf[uPreviousFrameSamplePosL+1]; \
	AkInt16 iNextL = pInBuf[uPreviousFrameSamplePosL+2]; \
	AkInt16 iNextR = pInBuf[uPreviousFrameSamplePosL+3]; \
	prevFramesL[_i] = iPrevL; \
	prevFramesR[_i] = iPrevR; \
	nextFramesL[_i] = iNextL; \
	nextFramesR[_i] = iNextR;} \
	interpLocFP[_i] = uIndexFP & FPMASK; \
	uIndexFP += uFrameSkipFP; \
	uPreviousFrameIndex = uIndexFP >> FPBITS;

// Vectorized version tested on Vita (1.4x speed of scalar version below)
AKRESULT Fixed_I16_1Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );
	
	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT) io_pInBuffer->GetInterleavedData( ) + io_pPitchState->uInFrameOffset - 1; 

	// Retrieve output buffer information
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;

	// Use stored value as left value, while right index is on the first sample
	AkInt16 iPreviousFrame = *io_pPitchState->iLastValue;
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
 	while ( uIterFrames-- )
	{
		AkInt32 iSampleDiff = pInBuf[1] - iPreviousFrame;
		*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
		FP_INDEX_ADVANCE();
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
	AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	AK_ALIGN_SIMD( AkInt32 prevFrames[ 4 ] );
	AK_ALIGN_SIMD( AkInt32 nextFrames[ 4 ] );
	AK_ALIGN_SIMD( AkUInt32 interpLocFP[ 4 ] );

	const AKSIMD_V4F32 vfScaleFPMUL = AKSIMD_SET_V4F32( 1.0f / (float) FPMUL );
	const AKSIMD_V4F32 vfScaleInt16 = AKSIMD_SET_V4F32( NORMALIZEFACTORI16 );

	// For all other sample frames
	uIterFrames = uNumIterThisFrame;

	while ( uIterFrames > 4 )
	{
		uIterFrames -= 4;

		// Unrolling this causes the SN compiler (SDK 995) to generate very slow code, so keep it as a loop.
		for ( unsigned int i = 0; i < 4; ++i )
		{
			FETCH_FRAMES_1CHAN( i );
		}

		AKSIMD_V4I32 mPrevFrames1 = AKSIMD_LOAD_V4I32 ( (AKSIMD_V4I32 *) ( prevFrames ) );
		AKSIMD_V4I32 mNextFrames1 = AKSIMD_LOAD_V4I32 ( (AKSIMD_V4I32 *) ( nextFrames ) );
		AKSIMD_V4I32 mInterpLocFP1 = AKSIMD_LOAD_V4I32 ( (AKSIMD_V4I32 *) ( interpLocFP ) );
		AKSIMD_V4F32 vfPrevFrames1 = AKSIMD_CONVERT_V4I32_TO_V4F32( mPrevFrames1 );
		AKSIMD_V4F32 vfNextFrames1 = AKSIMD_CONVERT_V4I32_TO_V4F32( mNextFrames1 );
		AKSIMD_V4F32 vfInterpLocFP1 = AKSIMD_MUL_V4F32( AKSIMD_CONVERT_V4I32_TO_V4F32( mInterpLocFP1 ), vfScaleFPMUL );
		AKSIMD_V4F32 vfDiff1 = AKSIMD_SUB_V4F32( vfNextFrames1, vfPrevFrames1 );
		AKSIMD_V4F32 vfResult1 = AKSIMD_MADD_V4F32( vfDiff1, vfInterpLocFP1, vfPrevFrames1 );
		AKSIMD_STOREU_V4F32( pfOutBuf, AKSIMD_MUL_V4F32( vfResult1, vfScaleInt16 ) ); 
		
		pfOutBuf += 4;
	}

	uInterpLocFP = uIndexFP & FPMASK;

	while ( uIterFrames-- )
	{
		iPreviousFrame = pInBuf[uPreviousFrameIndex];
		AkInt32 iSampleDiff = pInBuf[uPreviousFrameIndex+1] - iPreviousFrame;	
		*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
		FP_INDEX_ADVANCE();
	}

	PITCH_SAVE_NEXT_I16_1CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );
}

// Vectorized version tested on Vita
AKRESULT Fixed_I16_2Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();

	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT) io_pInBuffer->GetInterleavedData() + 2*io_pPitchState->uInFrameOffset - 2; 
	
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;

	// Use stored value as left value, while right index is on the first sample
	AkInt16 iPreviousFrameL = io_pPitchState->iLastValue[0];
	AkInt16 iPreviousFrameR = io_pPitchState->iLastValue[1];
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	while ( uIterFrames-- )
	{
		AkInt32 iSampleDiffL = pInBuf[2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[3] - iPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_I16( iPreviousFrameL, iSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_I16( iPreviousFrameR, iSampleDiffR );
		++pfOutBuf;
		FP_INDEX_ADVANCE();	
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
	AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	AK_ALIGN_SIMD( AkInt32 prevFramesL[ 4 ] );
	AK_ALIGN_SIMD( AkInt32 prevFramesR[ 4 ] );
	AK_ALIGN_SIMD( AkInt32 nextFramesL[ 4 ] );
	AK_ALIGN_SIMD( AkInt32 nextFramesR[ 4 ] );
	AK_ALIGN_SIMD( AkUInt32 interpLocFP[ 4 ] );

	const AKSIMD_V4F32 vfScaleFPMUL = AKSIMD_SET_V4F32( 1.0f / (float) FPMUL );
	const AKSIMD_V4F32 vfScaleInt16 = AKSIMD_SET_V4F32( NORMALIZEFACTORI16 );

	// For all other sample frames
	uIterFrames = uNumIterThisFrame;

	while ( uIterFrames > 4 )
	{
		uIterFrames -= 4;

		for ( unsigned int i = 0; i < 4; ++i )
		{
			FETCH_FRAMES_2CHAN( i );
		}

		AKSIMD_V4I32 mPrevFramesL = AKSIMD_LOAD_V4I32 ( (AKSIMD_V4I32 *) ( prevFramesL ) );
		AKSIMD_V4I32 mPrevFramesR = AKSIMD_LOAD_V4I32 ( (AKSIMD_V4I32 *) ( prevFramesR ) );
		AKSIMD_V4I32 mNextFramesL = AKSIMD_LOAD_V4I32 ( (AKSIMD_V4I32 *) ( nextFramesL ) );
		AKSIMD_V4I32 mNextFramesR = AKSIMD_LOAD_V4I32 ( (AKSIMD_V4I32 *) ( nextFramesR ) );
		AKSIMD_V4I32 mInterpLocFP1 = AKSIMD_LOAD_V4I32 ( (AKSIMD_V4I32 *) ( interpLocFP ) );
		AKSIMD_V4F32 vfPrevFramesL = AKSIMD_CONVERT_V4I32_TO_V4F32( mPrevFramesL );
		AKSIMD_V4F32 vfPrevFramesR = AKSIMD_CONVERT_V4I32_TO_V4F32( mPrevFramesR );
		AKSIMD_V4F32 vfNextFramesL = AKSIMD_CONVERT_V4I32_TO_V4F32( mNextFramesL );
		AKSIMD_V4F32 vfNextFramesR = AKSIMD_CONVERT_V4I32_TO_V4F32( mNextFramesR );
		AKSIMD_V4F32 vfInterpLocFP1 = AKSIMD_MUL_V4F32( AKSIMD_CONVERT_V4I32_TO_V4F32( mInterpLocFP1 ), vfScaleFPMUL );
		AKSIMD_V4F32 vfDiffL = AKSIMD_SUB_V4F32( vfNextFramesL, vfPrevFramesL );
		AKSIMD_V4F32 vfDiffR = AKSIMD_SUB_V4F32( vfNextFramesR, vfPrevFramesR );
		AKSIMD_V4F32 vfResultL = AKSIMD_MADD_V4F32( vfDiffL, vfInterpLocFP1, vfPrevFramesL );
		AKSIMD_V4F32 vfResultR = AKSIMD_MADD_V4F32( vfDiffR, vfInterpLocFP1, vfPrevFramesR );
		AKSIMD_STOREU_V4F32( pfOutBuf, AKSIMD_MUL_V4F32( vfResultL, vfScaleInt16 ) ); 
		AKSIMD_STOREU_V4F32( pfOutBuf + uMaxFrames, AKSIMD_MUL_V4F32( vfResultR, vfScaleInt16 ) ); 
		
		pfOutBuf += 4;
	}

	uInterpLocFP = uIndexFP & FPMASK;

	while ( uIterFrames-- )
	{
		AkUInt32 uPreviousFrameSamplePosL = uPreviousFrameIndex*2;
		iPreviousFrameL = pInBuf[uPreviousFrameSamplePosL];
		iPreviousFrameR = pInBuf[uPreviousFrameSamplePosL+1];
		AkInt32 iSampleDiffL = pInBuf[uPreviousFrameSamplePosL+2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[uPreviousFrameSamplePosL+3] - iPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_I16( iPreviousFrameL, iSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_I16( iPreviousFrameR, iSampleDiffR );
		++pfOutBuf;
		FP_INDEX_ADVANCE();
	}

	PITCH_SAVE_NEXT_I16_2CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );	
}

#else

AKRESULT Fixed_I16_1Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );
	
	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT) io_pInBuffer->GetInterleavedData( ) + io_pPitchState->uInFrameOffset - 1; 

	// Retrieve output buffer information
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;

	// Use stored value as left value, while right index is on the first sample
	AkInt16 iPreviousFrame = *io_pPitchState->iLastValue;
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
 	while ( uIterFrames-- )
	{
		AkInt32 iSampleDiff = pInBuf[1] - iPreviousFrame;
		*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
		FP_INDEX_ADVANCE();
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
	AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// For all other sample frames
	uIterFrames = uNumIterThisFrame;
	while ( uIterFrames-- )
	{
		iPreviousFrame = pInBuf[uPreviousFrameIndex];
		AkInt32 iSampleDiff = pInBuf[uPreviousFrameIndex+1] - iPreviousFrame;	
		*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
		FP_INDEX_ADVANCE();
	}

	PITCH_SAVE_NEXT_I16_1CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );
}

// Fixed resampling (no pitch changes) with INTERLEAVED signed 16-bit samples optimized for 2 channel signals.
AKRESULT Fixed_I16_2Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();

	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT) io_pInBuffer->GetInterleavedData() + 2*io_pPitchState->uInFrameOffset - 2; 
	
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;

	// Use stored value as left value, while right index is on the first sample
	AkInt16 iPreviousFrameL = io_pPitchState->iLastValue[0];
	AkInt16 iPreviousFrameR = io_pPitchState->iLastValue[1];
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	while ( uIterFrames-- )
	{
		AkInt32 iSampleDiffL = pInBuf[2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[3] - iPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_I16( iPreviousFrameL, iSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_I16( iPreviousFrameR, iSampleDiffR );
		++pfOutBuf;
		FP_INDEX_ADVANCE();	
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
	AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// For all other sample frames
	uIterFrames = uNumIterThisFrame;
	while ( uIterFrames-- )
	{
		AkUInt32 uPreviousFrameSamplePosL = uPreviousFrameIndex*2;
		iPreviousFrameL = pInBuf[uPreviousFrameSamplePosL];
		iPreviousFrameR = pInBuf[uPreviousFrameSamplePosL+1];
		AkInt32 iSampleDiffL = pInBuf[uPreviousFrameSamplePosL+2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[uPreviousFrameSamplePosL+3] - iPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_I16( iPreviousFrameL, iSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_I16( iPreviousFrameR, iSampleDiffR );
		++pfOutBuf;
		FP_INDEX_ADVANCE();
	}

	PITCH_SAVE_NEXT_I16_2CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );	
}

#endif

// Fixed resampling (no pitch changes) with floating point samples, optimized for one channel signals.
AKRESULT Fixed_Native_1Chan(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );

	// Minus one to compensate for offset of 1 due to zero == previous
	AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( 0 ) + io_pPitchState->uInFrameOffset - 1; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;

	// Use stored value as left value, while right index is on the first sample
	static const AkReal32 fScale = 1.f / SINGLEFRAMEDISTANCE;
	AkReal32 fLeftSample = *io_pPitchState->fLastValue;
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	while ( uIterFrames-- )
	{
		AkReal32 fSampleDiff = pInBuf[1] - fLeftSample;
		*pfOutBuf++ = LINEAR_INTERP_NATIVE( fLeftSample, fSampleDiff );
		FP_INDEX_ADVANCE();
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
	AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// For all other sample frames
	uIterFrames = uNumIterThisFrame;
	while ( uIterFrames-- )
	{
		fLeftSample = pInBuf[uPreviousFrameIndex];
		AkReal32 fSampleDiff = pInBuf[uPreviousFrameIndex+1] - fLeftSample;
		*pfOutBuf++ = LINEAR_INTERP_NATIVE( fLeftSample, fSampleDiff );
		FP_INDEX_ADVANCE();
	}

	PITCH_SAVE_NEXT_NATIVE_1CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );	
}

// Fixed resampling (no pitch changes) with DEINTERLEAVED floating point samples, optimized for 2 channel signals.
AKRESULT Fixed_Native_2Chan(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();

	// Minus one to compensate for offset of 1 due to zero == previous
	AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( 0 ) + io_pPitchState->uInFrameOffset - 1; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;

	// Use stored value as left value, while right index is on the first sample
	AkReal32 fPreviousFrameL = io_pPitchState->fLastValue[0];
	AkReal32 fPreviousFrameR = io_pPitchState->fLastValue[1];
	static const AkReal32 fScale = 1.f / SINGLEFRAMEDISTANCE;
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	while ( uIterFrames-- )
	{
		AkReal32 fSampleDiffL = pInBuf[1] - fPreviousFrameL;
		AkReal32 fSampleDiffR = pInBuf[1+uMaxFrames] - fPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_NATIVE( fPreviousFrameL, fSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_NATIVE( fPreviousFrameR, fSampleDiffR );
		++pfOutBuf;
		FP_INDEX_ADVANCE();
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
	AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// For all other sample frames
	uIterFrames = uNumIterThisFrame;
	while ( uIterFrames-- )
	{
		AkUInt32 uPreviousFrameR = uPreviousFrameIndex+uMaxFrames;
		fPreviousFrameL = pInBuf[uPreviousFrameIndex];
		AkReal32 fSampleDiffL = pInBuf[uPreviousFrameIndex+1] - fPreviousFrameL;
		fPreviousFrameR = pInBuf[uPreviousFrameR];	
		AkReal32 fSampleDiffR = pInBuf[uPreviousFrameR+1] - fPreviousFrameR;	
		*pfOutBuf = LINEAR_INTERP_NATIVE( fPreviousFrameL, fSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_NATIVE( fPreviousFrameR, fSampleDiffR );
		++pfOutBuf;
		FP_INDEX_ADVANCE();
	}

	PITCH_SAVE_NEXT_NATIVE_2CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );	
}

/********************* INTERPOLATING RESAMPLING DSP ROUTINES **********************/

// Interpolating resampling (pitch changes) with signed 16-bit samples, optimized for one channel signals.
AKRESULT Interpolating_I16_1Chan(	AkAudioBuffer * io_pInBuffer, 
									AkAudioBuffer * io_pOutBuffer,
									AkUInt32 uRequestedSize,
									AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );
	
	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT) io_pInBuffer->GetInterleavedData( ) + io_pPitchState->uInFrameOffset - 1; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

	PITCH_INTERPOLATION_SETUP( );

	// Use stored value as left value, while right index is on the first sample
	AkInt16 iPreviousFrame = *io_pPitchState->iLastValue;
	AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);		// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, (PITCHRAMPLENGTH-uRampCount)/uRampInc );	// Not longer than interpolation ramp length
	while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
	{
		AkInt32 iSampleDiff = pInBuf[1] - iPreviousFrame;
		*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );	
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	// For all other sample frames that need interpolation
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;
	AkUInt32 uIterFrames = (AkUInt32) (pfOutBufEnd - pfOutBuf);			// No more than the output buffer length
	uIterFrames = AkMin( uIterFrames, (PITCHRAMPLENGTH-uRampCount)/uRampInc );	// No more than the interpolation ramp length
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uIterFrames-- )
	{
		iPreviousFrame = pInBuf[uPreviousFrameIndex];
		AkInt32 iSampleDiff = pInBuf[uPreviousFrameIndex+1] - iPreviousFrame;	
		*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
		RESAMPLING_FACTOR_INTERPOLATE();	
		FP_INDEX_ADVANCE();
	}

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_I16_1CHAN();
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );
}

// Interpolating resampling (pitch changes) with INTERLEAVED signed 16-bit samples optimized for 2 channel signals.
AKRESULT Interpolating_I16_2Chan(	AkAudioBuffer * io_pInBuffer, 
									AkAudioBuffer * io_pOutBuffer,
									AkUInt32 uRequestedSize,
									AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();

	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT) io_pInBuffer->GetInterleavedData() + 2*io_pPitchState->uInFrameOffset - 2; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

	PITCH_INTERPOLATION_SETUP( );

	// Use stored value as left value, while right index is on the first sample
	AkInt16 iPreviousFrameL = io_pPitchState->iLastValue[0];
	AkInt16 iPreviousFrameR = io_pPitchState->iLastValue[1];
	AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);		// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, (PITCHRAMPLENGTH-uRampCount)/uRampInc );	// Not longer than interpolation ramp length
	while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
	{
		AkInt32 iSampleDiffL = pInBuf[2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[3] - iPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_I16( iPreviousFrameL, iSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_I16( iPreviousFrameR, iSampleDiffR );
		++pfOutBuf;
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	// Avoid inner branch by splitting in 2 cases
	// For all other sample frames that need interpolation
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;
	AkUInt32 uIterFrames = (AkUInt32) (pfOutBufEnd - pfOutBuf);
	uIterFrames = AkMin( uIterFrames, (PITCHRAMPLENGTH-uRampCount)/uRampInc );	// No more than the interpolation ramp length
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uIterFrames-- )
	{
		AkUInt32 uPreviousFrameSamplePosL = uPreviousFrameIndex*2;
		iPreviousFrameL = pInBuf[uPreviousFrameSamplePosL];
		iPreviousFrameR = pInBuf[uPreviousFrameSamplePosL+1];
		AkInt32 iSampleDiffL = pInBuf[uPreviousFrameSamplePosL+2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[uPreviousFrameSamplePosL+3] - iPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_I16( iPreviousFrameL, iSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_I16( iPreviousFrameR, iSampleDiffR );
		++pfOutBuf;
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_I16_2CHAN();
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );
}

// Interpolating resampling (pitch changes) with floating point samples optimized for one channel signals.
AKRESULT Interpolating_Native_1Chan(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
									AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );

	// Minus one to compensate for offset of 1 due to zero == previous
	AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( 0 ) + io_pPitchState->uInFrameOffset - 1; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

	PITCH_INTERPOLATION_SETUP( );

	// Use stored value as left value, while right index is on the first sample
	// Note: No interpolation necessary for the first few frames
	static const AkReal32 fScale = 1.f / SINGLEFRAMEDISTANCE;
	AkReal32 fLeftSample = *io_pPitchState->fLastValue;
	AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);		// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, (PITCHRAMPLENGTH-uRampCount)/uRampInc );	// Not longer than interpolation ramp length
	while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
	{
		AkReal32 fSampleDiff = pInBuf[1] - fLeftSample;
		*pfOutBuf++ = LINEAR_INTERP_NATIVE( fLeftSample, fSampleDiff );
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	// Avoid inner branch by splitting in 2 cases
	// For all other sample frames that need interpolation
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;
	AkUInt32 uIterFrames = (AkUInt32) (pfOutBufEnd - pfOutBuf);
	uIterFrames = AkMin( uIterFrames, (PITCHRAMPLENGTH-uRampCount)/uRampInc );	// No more than the interpolation ramp length
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uIterFrames-- )
	{
		fLeftSample = pInBuf[uPreviousFrameIndex];
		AkReal32 fSampleDiff = pInBuf[uPreviousFrameIndex+1] - fLeftSample;
		*pfOutBuf++ = LINEAR_INTERP_NATIVE( fLeftSample, fSampleDiff );
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_NATIVE_1CHAN();
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );
}

//  Interpolating resampling (pitch changes) with DEINTERLEAVED floating point samples optimized for 2 channel signals.
AKRESULT Interpolating_Native_2Chan(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();

	// Minus one to compensate for offset of 1 due to zero == previous
	AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( 0 ) + io_pPitchState->uInFrameOffset - 1; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

	PITCH_INTERPOLATION_SETUP( );

	// Use stored value as left value, while right index is on the first sample
	// Note: No interpolation necessary for the first few frames
	AkReal32 fPreviousFrameL = io_pPitchState->fLastValue[0];
	AkReal32 fPreviousFrameR = io_pPitchState->fLastValue[1];
	static const AkReal32 fScale = 1.f / SINGLEFRAMEDISTANCE;
	AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);		// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, (PITCHRAMPLENGTH-uRampCount)/uRampInc );	// Not longer than interpolation ramp length
	while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
	{
		AkReal32 fSampleDiffL = pInBuf[1] - fPreviousFrameL;
		AkReal32 fSampleDiffR = pInBuf[1+uMaxFrames] - fPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_NATIVE( fPreviousFrameL, fSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_NATIVE( fPreviousFrameR, fSampleDiffR );
		++pfOutBuf;
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	// For all other sample frames that need interpolation
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;
	AkUInt32 uIterFrames = (AkUInt32) (pfOutBufEnd - pfOutBuf);
	uIterFrames = AkMin( uIterFrames, (PITCHRAMPLENGTH-uRampCount)/uRampInc );	// No more than the interpolation ramp length
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uIterFrames-- )
	{
		// Linear interpolation and index advance
		AkUInt32 uPreviousFrameR = uPreviousFrameIndex+uMaxFrames;
		fPreviousFrameL = pInBuf[uPreviousFrameIndex];
		AkReal32 fSampleDiffL = pInBuf[uPreviousFrameIndex+1] - fPreviousFrameL;
		fPreviousFrameR = pInBuf[uPreviousFrameR];	
		AkReal32 fSampleDiffR = pInBuf[uPreviousFrameR+1] - fPreviousFrameR;	
		*pfOutBuf = LINEAR_INTERP_NATIVE( fPreviousFrameL, fSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_NATIVE( fPreviousFrameR, fSampleDiffR );
		++pfOutBuf;
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_NATIVE_2CHAN();
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );
}

//// Takes 2 vectores of 16 bit values and returns the result of the 8 multiplication in 2 vector of 32-bit values
//static AkForceInline void _mm_mul_ext32( AKSIMD_V4I32 * vResultLow, AKSIMD_V4I32 * vResultHigh, AKSIMD_V4I32 a , AKSIMD_V4I32 b)
//{
//	AKSIMD_V4I32 vHi = _mm_mulhi_epi16(a, b);
//	AKSIMD_V4I32 vLow = _mm_mullo_epi16(a, b);
//	*vResultHigh = AKSIMD_UNPACKHI_VECTOR8I16(vHi, vLow);
//	*vResultLow = AKSIMD_UNPACKLO_VECTOR8I16(vHi, vLow); 
//}

//static inline void PRECISION_EXTENDED_MULT_VEC( AKSIMD_V4I32 * vOutLo, AKSIMD_V4I32 * vOutHi, AKSIMD_V4I32 vi32AValue, AKSIMD_V4I32 vu16BValue )
//{
//	static const AKSIMD_V4I32 vOffset = _mm_set1_epi32( (1<<16) );
//	static const AKSIMD_V4I32 vLower16BitsMask = _mm_set1_epi32( 0xFFFF );
//
//	// Offset into unsigned range, since this is effectively a 17 bit value
//	AKSIMD_V4I32 vu32Value = _mm_add_epi32( vi32AValue, vOffset );
//	// Low part 16-bit value in 32-bit container for multiplication
//	AKSIMD_V4I32 vu32A2 = AKSIMD_SHIFTRIGHTARITH_V4I32(vu32Value, 16);			
//	// High part 16-bit value in 32-bit container for multiplication
//	AKSIMD_V4I32 vu32A1 = AKSIMD_AND_V4I32(vu32Value, vLower16BitsMask);	
//	// 16-bit multiplication result held in 32-bit container with 2^16 offset since this is the high multiplication part
//	AKSIMD_V4I32 vu32A2B1_Lo, vu32A2B1_Hi;
//	_mm_mul_ext32( vu32A2B1_Lo, vu32A2B1_Hi, vu32A2, vu16BValue );
//	vu32A2B1_Hi = AKSIMD_SHIFTLEFT_V4I32(vu32A2B1_Hi,16); 
//	vu32A2B1_Lo = AKSIMD_SHIFTLEFT_V4I32(vu32A2B1_Lo,16);
//	// 16-bit multiplication result held in 32-bit container, no offset as this is the low part of the multiplication	
//	AKSIMD_V4I32 vu32A1B1_Lo, vu32A1B1_Hi;
//	_mm_mul_ext32( vu32A1B1_Lo, vu32A1B1_Hi, vu32A1, vu16BValue );
//	// Sum high and low multiplication parts
//	*vOutHi = _mm_add_epi32( vu32A2B1_Hi, vu32A1B1_Hi );
//	*vOutLo = _mm_add_epi32( vu32A2B1_Lo, vu32A1B1_Lo ); 
//	// Subtract unsigned offset considering the multiplication it went through
//	AKSIMD_V4I32 vScaledOffsetLo, vScaledOffsetHi;
//	_mm_mul_ext32( vScaledOffsetLo, vScaledOffsetHi, vu16BValue, vOffset );
//	*vOutHi = _mm_sub_epi32( *vOutHi, vScaledOffsetHi ); 
//	*vOutLo = _mm_sub_epi32( *vOutLo, vScaledOffsetLo ); 
//}

//static inline AkInt32 PRECISION_EXTENDED_MULT( AkInt32 i32AValue, AkUInt16 u16BValue )
//{
//	AkUInt32 u32Value = i32AValue + (1<<16);	// Offset into unsigned range, since this is effectively a 17 bit value
//	AkUInt32 u32A1 = (u32Value & 0xFFFF);	// High part 16-bit value in 32-bit container for multiplication
//	AkUInt32 u32A2 = (u32Value >> 16);		// Low part 16-bit value in 32-bit container for multiplication
//	// 16-bit multiplication result held in 32-bit container with 2^16 offset since this is the high multiplication part
//	AkUInt32 u32A2B1 = (u32A2 * u16BValue) << 16; 
//	// 16-bit multiplication result held in 32-bit container, no offset as this is the low part of the multiplication
//	AkUInt32 u32A1B1 = u32A1 * u16BValue; 
//	AkUInt32 u32MultTotal = u32A2B1 + u32A1B1; // Sum high and low multiplication parts
//	AkInt32 i32MultResult = u32MultTotal - u16BValue*(1<<16); // Subtract unsigned offset considering the multiplication it went through
//	return i32MultResult;
//}
//
//// Fixed resampling (no pitch changes) with INTERLEAVED signed 16-bit samples for any number of channels.
//AKRESULT Fixed_I16_NChan(	AkAudioBuffer * io_pInBuffer, 
//							AkAudioBuffer * io_pOutBuffer,
//							AkUInt32 uRequestedSize,
//							AkInternalPitchState * io_pPitchState )
//{
//	PITCH_FIXED_DSP_SETUP( );
//	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
//	
//	// Minus one to compensate for offset of 1 due to zero == previous
//	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT) io_pInBuffer->GetInterleavedData() + uNumChannels*io_pPitchState->uInFrameOffset - uNumChannels; 
//	AkUInt32 uNumIterThisFrame;
//	AkUInt32 uStartIndexFP = uIndexFP;
//	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
//	{
//		FP_INDEX_RESET( uStartIndexFP );
//
//		AkInt16 * AK_RESTRICT pInBufChan = pInBuf + i;
//		AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( AkChannelRemap( i, io_pInBuffer->GetChannelMask() ) ) + io_pPitchState->uOutFrameOffset;
//		// Use stored value as left value, while right index is on the first sample
//		AkInt16 iPreviousFrame = io_pPitchState->iLastValue[i];
//		AkUInt32 uIterFrames = uNumIterPreviousFrame;
//		while ( uIterFrames-- )
//		{
//			AkInt32 iSampleDiff = pInBufChan[uNumChannels] - iPreviousFrame;
//			//*pfOutBuf++ = (AkReal32) ( ( iPreviousFrame << FPBITS ) + ( iSampleDiff * (AkInt32) uInterpLocFP ) ) * (NORMALIZEFACTORI16 / FPMUL);
//			AkInt32 i32PreviousFrame = iPreviousFrame << FPBITS;
//			AkInt32 i32MultResult = PRECISION_EXTENDED_MULT( iSampleDiff, (AkUInt16)uInterpLocFP );
//			AkInt32 i32Sum = i32PreviousFrame + i32MultResult;
//			AkReal32 fSum = (AkReal32)i32Sum;
//			AkReal32 fOut = fSum * (NORMALIZEFACTORI16 / FPMUL);
//			*pfOutBuf++ = fOut;
//
//			FP_INDEX_ADVANCE();	
//		}
//
//		// Determine number of iterations remaining
//		const AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
//		uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );
//
//		// For all other sample frames
//		uIterFrames = uNumIterThisFrame;
//		while ( uIterFrames-- )
//		{
//			AkUInt32 uPreviousFrameSamplePos = uPreviousFrameIndex*uNumChannels;
//			iPreviousFrame = pInBufChan[uPreviousFrameSamplePos];
//			AkInt32 iSampleDiff = pInBufChan[uPreviousFrameSamplePos+uNumChannels] - iPreviousFrame;
//			*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
//			FP_INDEX_ADVANCE();
//		}
//	}
//
//	PITCH_SAVE_NEXT_I16_NCHAN( uIndexFP );
//	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );	
//}

