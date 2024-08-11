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
// AkReverbUnit.h
//
// Schroeder comb filter bank and all pass filter reverberation unit implementation.
// 8 Comb filters in parallel followed by 4 all pass filter in series.
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_REVERBUNIT_H_
#define _AK_REVERBUNIT_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <math.h>
#include "AkReverbUnitCommon.h"

////////////////////////// Reverberator unit tunings ////////////////////////
#define NUMCOMBFILTERS (8)
static const AkReal32 g_fCombDelayTimes[] = {	0.025306f, 0.026939f, 0.028957f, 0.030748f, 
												0.032245f, 0.03381f, 0.035306f, 0.036667f };	
#define NUMALLPASSFILTERS (4)
static const AkReal32 g_fAllPassDelayTimes[] = { 0.012608f, 0.01f, 0.0077324f, 0.005102f };
/////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Name: class CAkReverbUnit
//-----------------------------------------------------------------------------
struct CAkReverbUnit
{
public:

	CAkReverbUnit();
	~CAkReverbUnit();

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
	AkForceInline void ProcessBuffer( AkReal32 * AK_RESTRICT io_pBufferIn, AkReal32 * AK_RESTRICT io_pBufferOut, AkUInt32 in_uFramesToProcess, bool in_bIsLastBuffer );

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
	AkReal32 * AK_RESTRICT m_pfCFFbkMem;		// Unified feedback memory for CF banks
	AkUInt32 m_uCFTotalFbkMemSize;				// Size of unified CF feedback memory
	AkReal32 * m_pfCFYStart[NUMCOMBFILTERS];		// Output delay line start
	AkReal32 * m_pfCFYEnd[NUMCOMBFILTERS];			// Output delay line end
	AkReal32 * m_pfCFYPtr[NUMCOMBFILTERS];			// Delay line read/write pointer
	AkUInt32 m_uCFDelays[NUMCOMBFILTERS];		// Comb filter delay times in samples
	AkReal32 m_vCFFwdCoefs[NUMCOMBFILTERS];		// Feedforward filter coefficients (1-abs(g))
	AkReal32 m_vCFFbkCoefs[NUMCOMBFILTERS];		// Feedback filter coefficients (g)
	/////////////////////// COMB FILTER BANK END /////////////////////////

	/////////////////////// ALLPASS FILTER BANK /////////////////////////
	// Interleaved feedforward/feedback to reduce cache misses
	AkUInt32 m_uAPFDelays[NUMALLPASSFILTERS];		// Allpass filters delay lengths
	AkReal32 * m_pfAPFMem;			// Unified feedforward/feedback memory for APF 
	AkUInt32 m_uAPFTotalMemSize;	// Size of unified APF feedforward/feedback  memory
	AkReal32 * m_pfAPFStart[NUMALLPASSFILTERS];		// Output delay line start
	AkReal32 * m_pfAPFEnd[NUMALLPASSFILTERS];		// Output delay line end
	AkReal32 * m_pfAPFPtr[NUMALLPASSFILTERS];		// Output Delay line read/write pointer
	/////////////////////// ALLPASS FILTER BANK END /////////////////////////
};

AkForceInline void CAkReverbUnit::PrimeCombFiltersCurrent( )
{
#ifdef USEPREFETCH
	for ( unsigned int i = 0; i < NUMCOMBFILTERS; i++ )
		PrefetchFilterMem( m_pfCFYStart[i], (AkReal32 * AK_RESTRICT)m_pfCFYPtr[i], m_pfCFYEnd[i], m_uSampleRate*sizeof(AkReal32) );	
#endif
}

AkForceInline void CAkReverbUnit::PrimeAllPassFiltersCurrent( )
{
#ifdef USEPREFETCH
	for ( unsigned int i = 0; i < NUMALLPASSFILTERS; i++ )
		PrefetchFilterMem( m_pfAPFStart[i], (AkReal32 * AK_RESTRICT)m_pfAPFPtr[i], m_pfAPFEnd[i], m_uSampleRate*2*sizeof(AkReal32) );
#endif
}

AkForceInline void CAkReverbUnit::PrimeCombFiltersNext( bool in_bIsLastBuffer )
{
#ifdef USEPREFETCH
	if ( !in_bIsLastBuffer )
	{
		for ( unsigned int i = 0; i < NUMCOMBFILTERS; i++ )
		{
			AkReal32 * pfNextRoundPrefetechAdd = (AkReal32 * AK_RESTRICT) m_pfCFYPtr[i]+m_uMaxFramesPerBuffer;
			if ( pfNextRoundPrefetechAdd >= m_pfCFYEnd[i] )
				pfNextRoundPrefetechAdd = (AkReal32 * AK_RESTRICT)m_pfCFYStart[i];
			PrefetchFilterMem( m_pfCFYStart[i], pfNextRoundPrefetechAdd, m_pfCFYEnd[i], m_uMaxFramesPerBuffer*sizeof(AkReal32) );	
		}
	}
#endif
}

AkForceInline void CAkReverbUnit::PrimeAllPassFiltersNext( bool in_bIsLastBuffer )
{
#ifdef USEPREFETCH
	if ( !in_bIsLastBuffer )
	{
		for ( unsigned int i = 0; i < NUMALLPASSFILTERS; i++ )
		{
			AkReal32 * pfNextRoundPrefetechAdd = (AkReal32 * AK_RESTRICT) m_pfAPFPtr[i]+m_uMaxFramesPerBuffer*2;
			if ( pfNextRoundPrefetechAdd >= m_pfAPFEnd[i] )
				pfNextRoundPrefetechAdd = (AkReal32 * AK_RESTRICT)m_pfAPFStart[i];
			PrefetchFilterMem( m_pfAPFStart[i], pfNextRoundPrefetechAdd, m_pfAPFEnd[i], m_uMaxFramesPerBuffer*2*sizeof(AkReal32) );
		}
	}
#endif
}

AkForceInline void CAkReverbUnit::ProcessBuffer( AkReal32 * AK_RESTRICT io_pBufferIn, AkReal32 * AK_RESTRICT io_pBufferOut, AkUInt32 in_uFramesToProcess, bool in_bIsLastBuffer )
{
	////////////// COMB FILTER BANK /////////////
	PrimeCombFiltersNext( in_bIsLastBuffer );
	CombFilterProcessBufferReplace( io_pBufferIn, io_pBufferOut, 0, in_uFramesToProcess );
	for ( unsigned int i = 1; i < NUMCOMBFILTERS; i++ )
	{
		CombFilterProcessBufferAdd( io_pBufferIn, io_pBufferOut, i, in_uFramesToProcess );	
	}	
	////////////// COMB FILTER BANK /////////////

	////////////// ALLPASS FILTER BANK /////////////
	PrimeAllPassFiltersNext( in_bIsLastBuffer );
	AllpassFilterProcessBuffer( io_pBufferOut, 0, in_uFramesToProcess );
	AllpassFilterProcessBuffer( io_pBufferOut, 1, in_uFramesToProcess );
	AllpassFilterProcessBuffer( io_pBufferOut, 2, in_uFramesToProcess );
	AllpassFilterProcessBuffer( io_pBufferOut, 3, in_uFramesToProcess );
	////////////// ALLPASS FILTER BANK /////////////
}

AkForceInline void CAkReverbUnit::AllpassFilterProcessBuffer( AkReal32 * AK_RESTRICT in_pfInOut, AkUInt32 in_uIndex, AkUInt32 in_uFramesToProcessThisLoop )
{
	AkReal32 * AK_RESTRICT pfAPFMem = (AkReal32 * AK_RESTRICT ) m_pfAPFPtr[in_uIndex];
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
				pfAPFMem = (AkReal32 * AK_RESTRICT) m_pfAPFStart[in_uIndex];

			uFramesProcessed += uFramesBeforeWrap;
			uFramesBeforeWrap = AkMin( AkUInt32(pfAPFEnd-pfAPFMem)/2, in_uFramesToProcessThisLoop-uFramesProcessed );
		}
		m_pfAPFPtr[in_uIndex] = pfAPFMem;
	}
}

AkForceInline void CAkReverbUnit::CombFilterProcessBufferReplace( AkReal32 * AK_RESTRICT in_pfIn, AkReal32 * AK_RESTRICT in_pfOut, AkUInt32 in_uIndex, AkUInt32 in_uFramesToProcessThisLoop )
{
	const AkReal32 fCFFwdCoefs = m_vCFFwdCoefs[in_uIndex];
	const AkReal32 fCFFbkCoefs = m_vCFFbkCoefs[in_uIndex];
	AkReal32 * AK_RESTRICT pfCFMem = (AkReal32 * AK_RESTRICT) m_pfCFYPtr[in_uIndex];
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
			pfCFMem = (AkReal32 * AK_RESTRICT) m_pfCFYStart[in_uIndex];	

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

AkForceInline void CAkReverbUnit::CombFilterProcessBufferAdd( AkReal32 * AK_RESTRICT in_pfIn, AkReal32 * AK_RESTRICT in_pfOut, AkUInt32 in_uIndex, AkUInt32 in_uFramesToProcessThisLoop )
{
	AkReal32 fCFFwdCoefs = m_vCFFwdCoefs[in_uIndex];
	AkReal32 fCFFbkCoefs = m_vCFFbkCoefs[in_uIndex];
	AkReal32 * AK_RESTRICT pfCFMem = (AkReal32 * AK_RESTRICT) m_pfCFYPtr[in_uIndex];
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
		const AkReal32 * pfCFYEnd = m_pfCFYEnd[in_uIndex];
		for ( unsigned int j = 0; j < uFramesBeforeWrap; ++j )
		{
			*in_pfOut++ += CombFilterProcessFrame(in_pfIn,pfCFMem,fCFFwdCoefs,fCFFbkCoefs);
			++in_pfIn;
			++pfCFMem;
		} 

		if ( pfCFMem >= pfCFYEnd )
			pfCFMem = (AkReal32 * AK_RESTRICT) m_pfCFYStart[in_uIndex];
		
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

#endif // _AK_REVERBUNIT_H_
