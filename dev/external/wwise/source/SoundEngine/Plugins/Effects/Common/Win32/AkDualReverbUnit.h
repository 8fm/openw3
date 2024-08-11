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
// AkDualReverbUnit.h
//
// Schroeder comb filter bank and all pass filter reverberation unit implementation.
// 8 Comb filters in parallel followed by 4 all pass filter in series.
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_DUALREVERBUNIT_H_
#define _AK_DUALREVERBUNIT_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <math.h>
#include "AkReverbUnitCommon.h"

////////////////////////// Dual reverberator unit tunings ////////////////////////
// static const AkReal32 g_fReverbSpread = 0.00052154195f;
#define NUMCOMBFILTERSDUAL (16)
static const AkReal32 g_fCombDelayTimesDual[] = {	0.025306f, 0.026939f, 0.028957f, 0.030748f, 
													0.032245f, 0.03381f, 0.035306f, 0.036667f,
													0.0258275419f, 0.027460541f, 0.029478541f, 0.0312695419f,
													0.0327665419f, 0.034331541f, 0.035827541f, 0.0371885419f
};	
 
#define NUMALLPASSFILTERSDUAL (8)
static const AkReal32 g_fAllPassDelayTimesDual[] = {	0.012608f, 0.01f, 0.0077324f, 0.005102f,
														0.0131295419f, 0.01052154195f, 0.00825394195f, 0.00562354195f 
};
/////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Name: class CAkDualReverbUnit
//-----------------------------------------------------------------------------
struct CAkDualReverbUnit
{
public:

    // Constructor/destructor
    CAkDualReverbUnit();
    ~CAkDualReverbUnit();

	// Allocate memory needed by effect and other initializations
    AKRESULT Init(	AK::IAkPluginMemAlloc *	in_pAllocator,		// Memory allocator interface.
					AkUInt32 in_uSampleRate,					// Sample rate
					AkUInt32 in_uMaxFramesPerBuffer );			// Max buffer size	
    
	// Free memory used by effect and effect termination
	void Term( AK::IAkPluginMemAlloc * in_pAllocator );

	// Reset filters
	void Reset( );

	// Change comb filter lengths as a function of reverb time
	void SetReverbTime( AkReal32 in_fReverbTime );

	// Prefetch memory to get filters ready for processing
	AkForceInline void PrimeCombFiltersCurrent( );
	AkForceInline void PrimeAllPassFiltersCurrent( );
	AkForceInline void PrimeCombFiltersNext( bool in_bIsLastBuffer );
	AkForceInline void PrimeAllPassFiltersNext( bool in_bIsLastBuffer );

    // Execute reverb unit processing.
	AkForceInline void ProcessBuffer( AkReal32 * AK_RESTRICT io_pBufferIn, AkReal32 * AK_RESTRICT io_pBufferOut1, AkReal32 * AK_RESTRICT io_pBufferOut2, AkUInt32 in_uFramesToProcess, bool in_bIsLastBuffer );

private:

	// Optimized filtering subroutines
	AkForceInline void AllpassFilterProcessBuffer( AkReal32 * AK_RESTRICT in_pfInOut, AkUInt32 in_uIndex, AkUInt32 in_uFramesToProcess );
	AkForceInline void CombFilterProcessBufferReplace( AkReal32 * AK_RESTRICT in_pfIn, AkReal32 * AK_RESTRICT in_pfOut, AkUInt32 in_uIndex, AkUInt32 in_uFramesToProcess );
	AkForceInline void CombFilterProcessBufferAdd( AkReal32 * AK_RESTRICT in_pfIn, AkReal32 * AK_RESTRICT in_pfOut, AkUInt32 in_uIndex, AkUInt32 in_uFramesToProcess );

private:

	AkUInt32 m_uSampleRate;			// Sample rate
	AkUInt32 m_uMaxFramesPerBuffer;	// Max number of audio frames in buffer

	/////////////////////// COMB FILTER BANK START /////////////////////////
	// 8 comb filters processed in parallel
	AkReal32 * m_pfCFFbkMem;		// Unified feedback memory for CF banks
	AkUInt32 m_uCFTotalFbkMemSize;				// Size of unified CF feedback memory
	AkReal32 * m_pfCFYStart[NUMCOMBFILTERSDUAL];		// Output delay line start
	AkReal32 * m_pfCFYEnd[NUMCOMBFILTERSDUAL];			// Output delay line end
	AkReal32 * m_pfCFYPtr[NUMCOMBFILTERSDUAL];			// Delay line read/write pointer
	AkUInt32 m_uCFDelays[NUMCOMBFILTERSDUAL];		// Comb filter delay times in samples
	AkReal32 m_vCFFwdCoefs[NUMCOMBFILTERSDUAL];		// Feedforward filter coefficients (1-abs(g))
	AkReal32 m_vCFFbkCoefs[NUMCOMBFILTERSDUAL];		// Feedback filter coefficients (g)
	/////////////////////// COMB FILTER BANK END /////////////////////////

	/////////////////////// ALLPASS FILTER BANK /////////////////////////
	// Interleaved feedforward/feedback to reduce cache misses
	AkUInt32 m_uAPFDelays[NUMALLPASSFILTERSDUAL];		// Allpass filters delay lengths
	AkReal32 * m_pfAPFMem;			// Unified feedforward/feedback memory for APF 
	AkUInt32 m_uAPFTotalMemSize;	// Size of unified APF feedforward/feedback  memory
	AkReal32 * m_pfAPFStart[NUMALLPASSFILTERSDUAL];		// Output delay line start
	AkReal32 * m_pfAPFEnd[NUMALLPASSFILTERSDUAL];		// Output delay line end
	AkReal32 * m_pfAPFPtr[NUMALLPASSFILTERSDUAL];		// Output Delay line read/write pointer
	/////////////////////// ALLPASS FILTER BANK END /////////////////////////
};

AkForceInline void CAkDualReverbUnit::PrimeCombFiltersCurrent( )
{
#ifdef USEPREFETCH
	for ( unsigned int i = 0; i < NUMCOMBFILTERSDUAL; i++ )
		PrefetchFilterMem( m_pfCFYStart[i], m_pfCFYPtr[i], m_pfCFYEnd[i], m_uSampleRate*sizeof(AkReal32) );	
#endif
}

AkForceInline void CAkDualReverbUnit::PrimeAllPassFiltersCurrent( )
{
#ifdef USEPREFETCH
	for ( unsigned int i = 0; i < NUMALLPASSFILTERSDUAL; i++ )
		PrefetchFilterMem( m_pfAPFStart[i], m_pfAPFPtr[i], m_pfAPFEnd[i], m_uSampleRate*2*sizeof(AkReal32) );
#endif
}

AkForceInline void CAkDualReverbUnit::PrimeCombFiltersNext( bool in_bIsLastBuffer )
{
#ifdef USEPREFETCH
	if ( !in_bIsLastBuffer )
	{
		for ( unsigned int i = 0; i < NUMCOMBFILTERSDUAL; i++ )
		{
			AkReal32 * AK_RESTRICT pfNextRoundPrefetechAdd = (AkReal32 * AK_RESTRICT) m_pfCFYPtr[i]+m_uMaxFramesPerBuffer;
			if ( pfNextRoundPrefetechAdd >= m_pfCFYEnd[i] )
				pfNextRoundPrefetechAdd = (AkReal32 * AK_RESTRICT) m_pfCFYStart[i];
			PrefetchFilterMem( m_pfCFYStart[i], pfNextRoundPrefetechAdd, m_pfCFYEnd[i], m_uMaxFramesPerBuffer*sizeof(AkReal32) );	
		}
	}
#endif
}

AkForceInline void CAkDualReverbUnit::PrimeAllPassFiltersNext( bool in_bIsLastBuffer )
{
#ifdef USEPREFETCH
	if ( !in_bIsLastBuffer )
	{
		for ( unsigned int i = 0; i < NUMALLPASSFILTERSDUAL; i++ )
		{
			AkReal32 * AK_RESTRICT pfNextRoundPrefetechAdd = (AkReal32 * AK_RESTRICT) m_pfAPFPtr[i]+m_uMaxFramesPerBuffer*2;
			if ( pfNextRoundPrefetechAdd >= m_pfAPFEnd[i] )
				pfNextRoundPrefetechAdd = (AkReal32 * AK_RESTRICT)m_pfAPFStart[i];
			PrefetchFilterMem( m_pfAPFStart[i], pfNextRoundPrefetechAdd, m_pfAPFEnd[i], m_uMaxFramesPerBuffer*2*sizeof(AkReal32) );
		}
	}
#endif
}

AkForceInline void CAkDualReverbUnit::ProcessBuffer( AkReal32 * AK_RESTRICT io_pBufferIn, AkReal32 * AK_RESTRICT io_pBufferOut1, AkReal32 * AK_RESTRICT io_pBufferOut2, AkUInt32 in_uFramesToProcess, bool in_bIsLastBuffer )
{
	////////////// COMB FILTER BANK /////////////	
	PrimeCombFiltersNext( in_bIsLastBuffer );
	AkUInt32 uSecondUnitOffset = NUMCOMBFILTERSDUAL/2;
	CombFilterProcessBufferReplace( io_pBufferIn, io_pBufferOut1, 0, in_uFramesToProcess );
	CombFilterProcessBufferReplace( io_pBufferIn, io_pBufferOut2, uSecondUnitOffset, in_uFramesToProcess );
	for ( unsigned int i = 1; i < uSecondUnitOffset; i++ )
	{
		CombFilterProcessBufferAdd( io_pBufferIn, io_pBufferOut1, i, in_uFramesToProcess );
		CombFilterProcessBufferAdd( io_pBufferIn, io_pBufferOut2, i+uSecondUnitOffset, in_uFramesToProcess );	
	}
	////////////// COMB FILTER BANK /////////////

	////////////// ALLPASS FILTER BANK /////////////
	PrimeAllPassFiltersNext( in_bIsLastBuffer );
	uSecondUnitOffset = NUMALLPASSFILTERSDUAL/2;
	for ( unsigned int i = 0; i < uSecondUnitOffset; i++ )
	{
		AllpassFilterProcessBuffer( io_pBufferOut1, i, in_uFramesToProcess );
		AllpassFilterProcessBuffer( io_pBufferOut2, i+uSecondUnitOffset, in_uFramesToProcess );
	}
	////////////// ALLPASS FILTER BANK /////////////
}

AkForceInline void CAkDualReverbUnit::AllpassFilterProcessBuffer( AkReal32 * AK_RESTRICT in_pfInOut, AkUInt32 in_uIndex, AkUInt32 in_uFramesToProcessThisLoop )
{
	AkReal32 * AK_RESTRICT pfAPFMem = (AkReal32 * AK_RESTRICT) m_pfAPFPtr[in_uIndex];
	AkUInt32 uFramesBeforeWrap = AkMin( AkUInt32(m_pfAPFEnd[in_uIndex]-pfAPFMem)/2, in_uFramesToProcessThisLoop );

	// if min is sample left to compute no one wraps
	if ( uFramesBeforeWrap == in_uFramesToProcessThisLoop )
	{
		for ( unsigned int j = 0; j < uFramesBeforeWrap; ++j )
		{
			*in_pfInOut = AllPassFilterProcessFrame(in_pfInOut,pfAPFMem);
			++in_pfInOut;
			pfAPFMem += 2;
		} 
		m_pfAPFPtr[in_uIndex] = pfAPFMem;
	}
	// otherwise handle minimum number of wraps
	else
	{
		AkUInt32 uFramesProcessed = 0;
		const AkReal32 * pfAPFEnd = m_pfAPFEnd[in_uIndex];
		while ( uFramesProcessed < in_uFramesToProcessThisLoop )
		{
			// Input and output delay lines may not wrap at the same time
			for ( unsigned int j = 0; j < uFramesBeforeWrap; ++j )
			{
				*in_pfInOut = AllPassFilterProcessFrame(in_pfInOut,pfAPFMem);
				++in_pfInOut;
				pfAPFMem += 2;
			} 
	
			if ( pfAPFMem >= pfAPFEnd )
				pfAPFMem = (AkReal32 * AK_RESTRICT)m_pfAPFStart[in_uIndex];

			uFramesProcessed += uFramesBeforeWrap;
			uFramesBeforeWrap = AkMin( AkUInt32(pfAPFEnd-pfAPFMem)/2, in_uFramesToProcessThisLoop-uFramesProcessed );
		}
		m_pfAPFPtr[in_uIndex] = pfAPFMem;
	}
}

AkForceInline void CAkDualReverbUnit::CombFilterProcessBufferReplace( AkReal32 * AK_RESTRICT in_pfIn, AkReal32 * AK_RESTRICT in_pfOut, AkUInt32 in_uIndex, AkUInt32 in_uFramesToProcessThisLoop )
{
	const AkReal32 fCFFwdCoefs = m_vCFFwdCoefs[in_uIndex];
	const AkReal32 fCFFbkCoefs = m_vCFFbkCoefs[in_uIndex];
	AkReal32 * AK_RESTRICT pfCFMem = (AkReal32 * AK_RESTRICT)m_pfCFYPtr[in_uIndex];
	AkUInt32 uFramesBeforeWrap = AkMin( AkUInt32(m_pfCFYEnd[in_uIndex]-m_pfCFYPtr[in_uIndex]), in_uFramesToProcessThisLoop );

	// if min is sample left to compute no one wraps
	if ( uFramesBeforeWrap == in_uFramesToProcessThisLoop )
	{
		for ( unsigned int j = 0; j < uFramesBeforeWrap; ++j )
		{
			*in_pfOut++ = CombFilterProcessFrame(in_pfIn,pfCFMem,fCFFwdCoefs,fCFFbkCoefs);
			++in_pfIn;
			++pfCFMem;
		} 
		m_pfCFYPtr[in_uIndex] = pfCFMem;
	}
	// otherwise handle minimum number of wraps
	else
	{
		// NOTE: Because delay line >= TEMP_BUFFER_SIZE no more than 1 wrap is possible
		const AkReal32 * AK_RESTRICT pfCFYEnd = m_pfCFYEnd[in_uIndex];
		for ( unsigned int j = 0; j < uFramesBeforeWrap; ++j )
		{
			*in_pfOut++ = CombFilterProcessFrame(in_pfIn,pfCFMem,fCFFwdCoefs,fCFFbkCoefs);
			++in_pfIn;
			++pfCFMem;
		} 
		
		if ( pfCFMem >= pfCFYEnd )
			pfCFMem = (AkReal32 * AK_RESTRICT)m_pfCFYStart[in_uIndex];	

		AkUInt32 uFramesProcessed = uFramesBeforeWrap;
		uFramesBeforeWrap = AkMin( AkUInt32(pfCFYEnd-pfCFMem), in_uFramesToProcessThisLoop-uFramesProcessed );

		for ( unsigned int j = 0; j < uFramesBeforeWrap; ++j )
		{
			*in_pfOut++ = CombFilterProcessFrame(in_pfIn,pfCFMem,fCFFwdCoefs,fCFFbkCoefs);
			++in_pfIn;
			++pfCFMem;
		} 

		m_pfCFYPtr[in_uIndex] = pfCFMem;
	}
}

AkForceInline void CAkDualReverbUnit::CombFilterProcessBufferAdd( AkReal32 * AK_RESTRICT in_pfIn, AkReal32 * AK_RESTRICT in_pfOut, AkUInt32 in_uIndex, AkUInt32 in_uFramesToProcessThisLoop )
{
	AkReal32 fCFFwdCoefs = m_vCFFwdCoefs[in_uIndex];
	AkReal32 fCFFbkCoefs = m_vCFFbkCoefs[in_uIndex];
	AkReal32 * AK_RESTRICT pfCFMem = (AkReal32 * AK_RESTRICT)m_pfCFYPtr[in_uIndex];
	AkUInt32 uFramesBeforeWrap = AkMin( AkUInt32(m_pfCFYEnd[in_uIndex]-m_pfCFYPtr[in_uIndex]), in_uFramesToProcessThisLoop );

	// if min is sample left to compute no one wraps
	if ( uFramesBeforeWrap == in_uFramesToProcessThisLoop )
	{
		for ( unsigned int j = 0; j < uFramesBeforeWrap; ++j )
		{
			*in_pfOut++ += CombFilterProcessFrame(in_pfIn,pfCFMem,fCFFwdCoefs,fCFFbkCoefs);
			++in_pfIn;
			++pfCFMem;
		} 
		m_pfCFYPtr[in_uIndex] = pfCFMem;
	}
	// otherwise handle minimum number of wraps
	else
	{
		// NOTE: Because delay line >= TEMP_BUFFER_SIZE no more than 1 wrap is possible
		const AkReal32 * AK_RESTRICT pfCFYEnd = m_pfCFYEnd[in_uIndex];
		for ( unsigned int j = 0; j < uFramesBeforeWrap; ++j )
		{
			*in_pfOut++ += CombFilterProcessFrame(in_pfIn,pfCFMem,fCFFwdCoefs,fCFFbkCoefs);
			++in_pfIn;
			++pfCFMem;
		} 

		if ( pfCFMem >= pfCFYEnd )
			pfCFMem = (AkReal32 * AK_RESTRICT)m_pfCFYStart[in_uIndex];
		
		AkUInt32 uFramesProcessed = uFramesBeforeWrap;
		uFramesBeforeWrap = AkMin( AkUInt32(pfCFYEnd-pfCFMem), in_uFramesToProcessThisLoop-uFramesProcessed );

		for ( unsigned int j = 0; j < uFramesBeforeWrap; ++j )
		{
			*in_pfOut++ += CombFilterProcessFrame(in_pfIn,pfCFMem,fCFFwdCoefs,fCFFbkCoefs);
			++in_pfIn;
			++pfCFMem;
		} 

		m_pfCFYPtr[in_uIndex] = pfCFMem;
	}
}

#endif // _AK_DUALREVERBUNIT_H_
