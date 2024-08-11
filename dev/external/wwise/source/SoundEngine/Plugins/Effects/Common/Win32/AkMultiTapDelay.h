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
// AkMultiTapDelay.h
//
// Sums the output of an arbitrary number (M) of taps with specified gains and lengths
// y[n] = g1 * x[n - d1] + g2 * x[n - d2] + g3 * x[n - d3] + ... + gM * x[n - dM]
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKMULTITAPDELAY_H_
#define _AKMULTITAPDELAY_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/Tools/Common/AkAssert.h>
#include "AkReverbUnitCommon.h"

//#define MEMORYOUTPUT
#if defined(_DEBUG) && defined(AK_WIN) && defined(MEMORYOUTPUT)
#include "AKTrace.h"
#endif

#define NUM_TAPS  (6) // Including the implicit 0 time tap (read write ptr)
#define WRAPINDEX (NUM_TAPS-1)
static AkReal32 g_fTapTimes[NUM_TAPS-1] = { 0.0155f, 0.019f, 0.0215f, 0.05f, 0.0597f }; //These must be in increasing order
static AkReal32 g_fTapGains[NUM_TAPS-1] = { 0.818f, 0.635f, 0.719f, 0.267f, 0.242f };
#define TAP0GAIN (0.99f)

class CAkMultiTapDelay
{
public:

	CAkMultiTapDelay() : m_pfX( NULL ), m_pfXEnd( NULL )
	{
		for ( AkUInt32 i = 0; i < NUM_TAPS-1; ++i )
			m_pfTapPtrs[i] = NULL;
	}

	AKRESULT Init( AK::IAkPluginMemAlloc * in_pAllocator, AkUInt32 in_uSampleRate );
	void Term( AK::IAkPluginMemAlloc * in_pAllocator );
	void Reset( );
	AkForceInline void ProcessBuffer( AkReal32 * in_pInBuffer, AkReal32 * in_pOutBuffer, AkUInt32 in_uNumFrames );

private:
    
	AkReal32 * m_pfTapPtrs[NUM_TAPS-1];	// Tap lengths pointer array -> index 0 is max and 0 delay and write ptr
	AkUInt32 m_uDelayLineLength;					// Maximum length
	AkReal32 * m_pfX;					// Input delay line (start)
	AkReal32 * m_pfXEnd;				// Delay line back boundary
	AkUInt32 m_puTapDistances[NUM_TAPS-1];			// Tap delays array
	AkUInt32 m_uFramesBeforeNextWrap;				// Frames reamining before one of the tap (including write) has to wrap
	AkUInt32 m_uWrapDistanceIndex;					// Index into m_puTapDistances to determine next tap distance
};

#endif // _AKMULTITAPDELAY_H_


inline AKRESULT CAkMultiTapDelay::Init( AK::IAkPluginMemAlloc * in_pAllocator, AkUInt32 in_uSampleRate ) 
{
	AkUInt32 uTapDelays[NUM_TAPS]; 
	uTapDelays[0] = 0;
	for ( unsigned int i = 1; i < NUM_TAPS; i++ )
	{
		AkUInt32 uCurTapLength = MakePrime( static_cast<AkUInt32>( g_fTapTimes[i-1] * in_uSampleRate ) );
		uTapDelays[i] = uCurTapLength;
	}
	m_uDelayLineLength = uTapDelays[NUM_TAPS-1]; //Max delay length is larger tap

	m_pfX = (AkReal32 * )AK_PLUGIN_ALLOC( in_pAllocator, sizeof(AkReal32) * m_uDelayLineLength );	
	if ( m_pfX == NULL )
		return AK_InsufficientMemory;
	m_pfXEnd = m_pfX + m_uDelayLineLength;

// Print out total memory allocated to Wwise debug window
#if defined(_DEBUG) && defined(AK_WIN) && defined(MEMORYOUTPUT)
		AKTrace::FormatTrace( AKTrace::CategoryGeneral, L"Multi-tap delay: Total allocated memory: %d\n", sizeof(AkReal32) * m_uDelayLineLength );
#endif
	
	Reset();
	
	m_pfTapPtrs[0] = m_pfX;	// Tap0 is write ptr and r0 and r5
	for ( unsigned int i = 1; i < NUM_TAPS-1; i++ )
	{
		m_pfTapPtrs[i] = m_pfXEnd - uTapDelays[i];
	}

	for ( unsigned int i = 1; i < NUM_TAPS; i++ )
	{
		m_puTapDistances[i-1] = uTapDelays[i] - uTapDelays[i-1];
	}
	// Note: First tap is zero, wraps at the same time as max delay and write ptr

	m_uWrapDistanceIndex = 1;
	m_uFramesBeforeNextWrap = m_puTapDistances[0];

	return AK_Success;
}

inline void CAkMultiTapDelay::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if ( m_pfX != NULL )
	{
		AK_PLUGIN_FREE( in_pAllocator, m_pfX );
		m_pfX = NULL;
	}
}

inline void CAkMultiTapDelay::Reset( )
{
	if ( m_pfX != NULL )
	{
		memset( m_pfX, 0, sizeof(AkReal32) * m_uDelayLineLength );
	}
}


AkForceInline void CAkMultiTapDelay::ProcessBuffer( AkReal32 * in_pInBuffer, AkReal32 * in_pOutBuffer, AkUInt32 in_uNumFrames )
{
	AKASSERT( in_uNumFrames > 0 );	
	AkReal32 * pfTapPtrs[NUM_TAPS-1];
	AkReal32 pfTapGains[NUM_TAPS-1];
	for ( unsigned int i = 0; i < NUM_TAPS-1; i++ )
	{
		pfTapPtrs[i] = m_pfTapPtrs[i];
		pfTapGains[i] = g_fTapGains[i];
	}

	if ( m_uFramesBeforeNextWrap > in_uNumFrames )
	{	
		// No wraps necessary
		unsigned int iFrame=in_uNumFrames;
		do
		{
			AkReal32 fTap5 = *(pfTapPtrs[0]) * pfTapGains[4];	 // tap 5 has maximum length read before overwriting value
			AkReal32 fIn = *in_pInBuffer;						 // input		
			++in_pInBuffer;
			*(pfTapPtrs[0]) = fIn;	// tap5 == tap0 == write ptr, Write input to delay line
			++(pfTapPtrs[0]);
			AkReal32 fTap0 = fIn * TAP0GAIN;					// tap 0 has 0 delay time

			AkReal32 fTap1 = *(pfTapPtrs[1]) * pfTapGains[0];		// Other taps in parallel
			AkReal32 fTap2 = *(pfTapPtrs[2]) * pfTapGains[1];	
			AkReal32 fTap3 = *(pfTapPtrs[3]) * pfTapGains[2];	
			AkReal32 fTap4 = *(pfTapPtrs[4]) * pfTapGains[3];

			AkReal32 fOut = fTap1 + fTap2 + fTap3;
			++(pfTapPtrs[1]);
			++(pfTapPtrs[2]);
			++(pfTapPtrs[3]);
			++(pfTapPtrs[4]);
			fOut += fTap4 + fTap5;

			*(in_pOutBuffer) = fOut;
			++in_pOutBuffer;
		} while(--iFrame > 0);
		m_uFramesBeforeNextWrap -= in_uNumFrames;
	}
	else
	{
		// Handle minimum wraps
		unsigned int uFramesProduced = 0;
		unsigned int uFramesToProcess = m_uFramesBeforeNextWrap;
		while ( uFramesProduced < in_uNumFrames )
		{
			unsigned int iFrame=uFramesToProcess;
			do
			{
				AkReal32 fTap5 = *(pfTapPtrs[0]) * pfTapGains[4];	 // tap 5 has maximum length read before overwriting value
				AkReal32 fIn = *in_pInBuffer;						 // input		
				++in_pInBuffer;
				*(pfTapPtrs[0]) = fIn;	// tap5 == tap0 == write ptr, Write input to delay line
				++(pfTapPtrs[0]);
				AkReal32 fTap0 = fIn * TAP0GAIN;					// tap 0 has 0 delay time

				AkReal32 fTap1 = *(pfTapPtrs[1]) * pfTapGains[0];		// Other taps in parallel
				AkReal32 fTap2 = *(pfTapPtrs[2]) * pfTapGains[1];	
				AkReal32 fTap3 = *(pfTapPtrs[3]) * pfTapGains[2];	
				AkReal32 fTap4 = *(pfTapPtrs[4]) * pfTapGains[3];

				AkReal32 fOut = fTap1 + fTap2 + fTap3;
				++(pfTapPtrs[1]);
				++(pfTapPtrs[2]);
				++(pfTapPtrs[3]);
				++(pfTapPtrs[4]);
				fOut += fTap4 + fTap5;

				*(in_pOutBuffer) = fOut;
				++in_pOutBuffer;
			} while(--iFrame > 0);
			uFramesProduced += uFramesToProcess;
			m_uFramesBeforeNextWrap -= uFramesToProcess;
			if ( m_uFramesBeforeNextWrap == 0 )
			{
				pfTapPtrs[m_uWrapDistanceIndex] = m_pfX;			
				m_uFramesBeforeNextWrap = m_puTapDistances[m_uWrapDistanceIndex];
				++m_uWrapDistanceIndex;
				if ( m_uWrapDistanceIndex == WRAPINDEX )
					m_uWrapDistanceIndex = 0;
			}
			uFramesToProcess = AkMin( m_uFramesBeforeNextWrap, in_uNumFrames-uFramesProduced );
		}
	}

	for ( unsigned int i = 0; i < NUM_TAPS-1; i++ )
	{
		 m_pfTapPtrs[i] = pfTapPtrs[i];
	}
}
