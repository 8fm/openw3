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

#include "AkReverbUnit.h"
#include <AK/Tools/Common/AkAssert.h>
#include <math.h>

#ifdef AK_APPLE
#include <string.h>
#endif

//#define MEMORYOUTPUT
#if defined(_DEBUG) && defined(AK_WIN) && defined(MEMORYOUTPUT)
#include "AKTrace.h"
#endif

// Constructor.
CAkReverbUnit::CAkReverbUnit() 
	: m_pfCFFbkMem( NULL )
	, m_pfAPFMem( NULL )
{
	for ( AkUInt32 i = 0; i < NUMCOMBFILTERS; ++i )
	{
		m_pfCFYStart[i] = NULL;		
		m_pfCFYEnd[i] = NULL;			
		m_pfCFYPtr[i] = NULL;
	}
	for ( AkUInt32 i = 0; i < NUMALLPASSFILTERS; ++i )
	{
		m_pfAPFStart[i] = NULL;	
		m_pfAPFEnd[i] = NULL;	
		m_pfAPFPtr[i] = NULL;
	}
}

CAkReverbUnit::~CAkReverbUnit() 
{

}

// Initialization
AKRESULT CAkReverbUnit::Init(	AK::IAkPluginMemAlloc * in_pAllocator,		// Memory allocator interface.
								AkUInt32 in_uSampleRate,					// Sample rate
								AkUInt32 in_uMaxFramesPerBuffer )			// Max buffer size				
{
	m_uSampleRate = in_uSampleRate;
	m_uMaxFramesPerBuffer = in_uMaxFramesPerBuffer;

	/////////////////////// COMB FILTER BANK START /////////////////////////
	// Comb filter are used with summed channels, thus not dependent on number of channels
	// Compute delay lengths as a function of sampling frequency and ensure prime numbers
	AkUInt32 uTotalCFFbkMemFrames = 0;
	for ( unsigned int i = 0; i < NUMCOMBFILTERS; i++ )
	{	
		AkUInt32 uCurCFLength = static_cast<AkUInt32>( g_fCombDelayTimes[i] * m_uSampleRate );
		uCurCFLength = MakePrime( uCurCFLength );
		if ( uCurCFLength < m_uMaxFramesPerBuffer )	// We do not want to wrap more than once per buffer
			uCurCFLength = m_uMaxFramesPerBuffer;		 
		m_uCFDelays[i] = uCurCFLength;
		uTotalCFFbkMemFrames += uCurCFLength;
	}

#if defined(_DEBUG) && defined(AK_WIN) && defined(MEMORYOUTPUT)
	AkUInt32 uTotalMemoryAllocated = 0;
#endif

	// Allocate and initialize unified feedback memory space
	m_uCFTotalFbkMemSize = uTotalCFFbkMemFrames * sizeof(AkReal32);
	m_pfCFFbkMem = (AkReal32*)AK_PLUGIN_ALLOC( in_pAllocator, m_uCFTotalFbkMemSize );
	if ( m_pfCFFbkMem == NULL )
		return AK_InsufficientMemory;
	memset( m_pfCFFbkMem, 0, m_uCFTotalFbkMemSize );

#if defined(_DEBUG) && defined(AK_WIN) && defined(MEMORYOUTPUT)
		uTotalMemoryAllocated += m_uCFTotalFbkMemSize;
#endif

	// Setup NUMCOMBFILTERS parallel comb filters
	AkReal32 * pfStartCurrentCFMem = m_pfCFFbkMem;
	for ( unsigned int i = 0; i < NUMCOMBFILTERS; i++ )
	{	
		m_pfCFYStart[i] = pfStartCurrentCFMem;
		m_pfCFYPtr[i] = pfStartCurrentCFMem;
		m_pfCFYEnd[i] = pfStartCurrentCFMem + m_uCFDelays[i];
		pfStartCurrentCFMem += m_uCFDelays[i];
	}
	//////////////////////// COMB FILTER BANK END ////////////////////////

	/////////////////////// ALL PASS FILTER BANK START /////////////////////////
	// APF filter are used with summed channels, thus not dependent on number of channels
	// Compute delay lengths as a function of sampling frequency and ensure prime numbers
	AkUInt32 uTotalAPFMemFrames = 0;
	for ( unsigned int i = 0; i < NUMALLPASSFILTERS; i++ )
	{	
		AkUInt32 uCurAPFLength = static_cast<AkUInt32>( g_fAllPassDelayTimes[i] * m_uSampleRate );
		uCurAPFLength = MakePrime( uCurAPFLength ); 
		m_uAPFDelays[i] = uCurAPFLength;
		uTotalAPFMemFrames += uCurAPFLength;
	}

	// Allocate unified feedback memory space
	m_uAPFTotalMemSize = uTotalAPFMemFrames * 2 * sizeof(AkReal32);
	m_pfAPFMem = (AkReal32*)AK_PLUGIN_ALLOC( in_pAllocator, m_uAPFTotalMemSize );
	if ( m_pfAPFMem == NULL )
		return AK_InsufficientMemory;
	memset( m_pfAPFMem, 0, m_uAPFTotalMemSize );

#if defined(_DEBUG) && defined(AK_WIN) && defined(MEMORYOUTPUT)
		uTotalMemoryAllocated += m_uAPFTotalMemSize;
#endif

// Print out total memory allocated to Wwise debug window
#if defined(_DEBUG) && defined(AK_WIN) && defined(MEMORYOUTPUT)
		AKTrace::FormatTrace( AKTrace::CategoryGeneral, L"ReverbUnit: Total allocated memory: %d\n", uTotalMemoryAllocated );
#endif

	// Setup NUMALLPASSFILTERS parallel Allpass filters
	AkReal32 * pfStartCurrentAPFMem = m_pfAPFMem;
	for ( unsigned int i = 0; i < NUMALLPASSFILTERS; i++ )
	{	
		// Output delay line
		m_pfAPFStart[i] = pfStartCurrentAPFMem;
		m_pfAPFPtr[i] = pfStartCurrentAPFMem;
		m_pfAPFEnd[i] = pfStartCurrentAPFMem + m_uAPFDelays[i]*2;
		pfStartCurrentAPFMem += m_uAPFDelays[i]*2;
	}
	/////////////////////// ALL PASS FILTER BANK END /////////////////////////

	return AK_Success;
}

// Termination
void CAkReverbUnit::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if ( m_pfCFFbkMem != NULL )
	{
		AK_PLUGIN_FREE( in_pAllocator, m_pfCFFbkMem );
		m_pfCFFbkMem = NULL;
	}

	if ( m_pfAPFMem != NULL )
	{
		AK_PLUGIN_FREE( in_pAllocator, m_pfAPFMem );
		m_pfAPFMem = NULL;
	}
}

// Reset
void CAkReverbUnit::Reset( )
{
	// Reset all filters
	if ( m_pfCFFbkMem != NULL )
	{
		memset( m_pfCFFbkMem, 0, m_uCFTotalFbkMemSize );
	}

	if ( m_pfAPFMem != NULL )
	{
		memset( m_pfAPFMem, 0, m_uAPFTotalMemSize );
	}
}

// SetReverbTime
void CAkReverbUnit::SetReverbTime( AkReal32 in_fReverbTime )
{
	// Compute all comb filters coefficients
	AkReal32 fReverbTimeFrames = in_fReverbTime * m_uSampleRate;
	for ( unsigned int i = 0; i < NUMCOMBFILTERS; i++ )
	{
		// g = 0.001^(loop time / reverb time )
		// 0.001 = -60 dB (T60)
		AkReal32 fCombCoef = powf( 0.001f, (m_uCFDelays[i] / fReverbTimeFrames) );
		AKASSERT( fCombCoef >= 0.f && fCombCoef <= 1.f );
		m_vCFFwdCoefs[i] = 1.f - fCombCoef;
		m_vCFFbkCoefs[i] = fCombCoef;
	}
}
