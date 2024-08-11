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
// AkDelay.h
//
// AudioKinetic delay class
// y[n] = x[n - D] 
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKDELAY_H_
#define _AKDELAY_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/Tools/Common/AkAssert.h>
#include <AK/SoundEngine/Common/AkSimd.h>

//#define MEMORYOUTPUT 
#if defined(_DEBUG) && defined(AK_WIN) && defined(MEMORYOUTPUT)
#include "AKTrace.h"
#endif

class CAkDelay
{
public:

	CAkDelay() : m_pX( NULL ), m_pXPtr( NULL ), m_pXEnd( NULL ) {}

	inline AKRESULT Init( AK::IAkPluginMemAlloc * in_pAllocator, AkUInt32 in_uDelayLineLength );
	inline void Term( AK::IAkPluginMemAlloc * in_pAllocator );
	inline void Reset( );
	AkForceInline void ProcessBuffer( AkReal32 * AK_RESTRICT in_pInBuffer, AkReal32 * AK_RESTRICT in_pOutBuffer, AkUInt32 in_uNumFrames );
	inline void PrimeMemory( AkUInt32 uFrameOffset, AkUInt32 uPrefetchFrames );

private:
    
	AkUInt32 m_uDelayLineLength;	// Delay line length
	AkReal32 * m_pX;		// Input delay line (start)
	AkReal32 * m_pXPtr;		// Delay line read/write pointer
	AkReal32 * m_pXEnd;		// Delay line back boundary			
};

AkForceInline void CAkDelay::ProcessBuffer( AkReal32 * AK_RESTRICT in_pInBuffer, AkReal32 * AK_RESTRICT in_pOutBuffer, AkUInt32 in_uNumFrames )
{
	AkReal32 * AK_RESTRICT pXPtr = (AkReal32 * AK_RESTRICT) m_pXPtr;
	const AkReal32 * pEnd = m_pXEnd;
	unsigned int uFramesBeforeWrap = AkUInt32(pEnd - pXPtr);
	unsigned int uFramesToProcess = AkMin( uFramesBeforeWrap, in_uNumFrames );
	if ( uFramesToProcess == in_uNumFrames )
	{
		// Not wrapping this time
		unsigned int i=uFramesToProcess;
		while( i-- )
		{
			AkReal32 fXn = *pXPtr;			// Read output
			*pXPtr++ = *in_pInBuffer++;		// Write input to delay line
			*in_pOutBuffer++ = fXn;
		} 
	}
	else
	{
		// Minimum number of wraps
		unsigned int uFramesProduced = 0;
		while ( uFramesProduced < in_uNumFrames )
		{
			for ( unsigned int i = 0; i < uFramesToProcess; ++i )
			{
				AkReal32 fXn = *pXPtr;			// Read output
				*pXPtr++ = *in_pInBuffer++;		// Write input to delay line
				*in_pOutBuffer++ = fXn;
			}
			
			if ( pXPtr == pEnd )
				pXPtr = (AkReal32 * AK_RESTRICT) m_pX;

			uFramesProduced += uFramesBeforeWrap;
			uFramesBeforeWrap = AkUInt32(pEnd - pXPtr);
			uFramesToProcess = AkMin( uFramesBeforeWrap, in_uNumFrames-uFramesProduced );
		}
	}
	m_pXPtr = pXPtr;
}

inline AKRESULT CAkDelay::Init( AK::IAkPluginMemAlloc * in_pAllocator, AkUInt32 in_uDelayLineLength )
{
	m_uDelayLineLength = AkMax( in_uDelayLineLength, 1 ); //at least one sample delay
	m_pX = (AkReal32*)AK_PLUGIN_ALLOC( in_pAllocator, sizeof(AkReal32) * m_uDelayLineLength );
	if ( m_pX == NULL )
		return AK_InsufficientMemory;

// Print out total memory allocated to Wwise debug window
#if defined(_DEBUG) && defined(AK_WIN) && defined(MEMORYOUTPUT)
		AKTrace::FormatTrace( AKTrace::CategoryGeneral, L"Delay: Total allocated memory: %d\n", sizeof(AkReal32) * m_uDelayLineLength );
#endif

	m_pXPtr = m_pX;
	m_pXEnd = m_pX + m_uDelayLineLength;
	Reset();
	return AK_Success;
}

inline void CAkDelay::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if ( m_pX )
	{
		AK_PLUGIN_FREE( in_pAllocator, m_pX );
		m_pX = NULL;
	}
}

inline void CAkDelay::Reset( )
{
	if ( m_pX )
	{
		memset( m_pX, 0, sizeof(AkReal32) * m_uDelayLineLength );
	}
}

inline void CAkDelay::PrimeMemory( AkUInt32 uFrameOffset, AkUInt32 uPrefetchFrames )
{
	AkReal32 * AK_RESTRICT pfCur = (AkReal32 * AK_RESTRICT) m_pXPtr;
	pfCur += uFrameOffset;
	if ( pfCur >= m_pXEnd )
		pfCur -= m_uDelayLineLength;

	AkUInt32 uBytesToPrefetch = AkMin( uPrefetchFrames*sizeof(AkReal32), AKSIMD_ARCHMAXPREFETCHSIZE );
	AkUInt32 uBytesBeforeWrap = AkUInt32((AkUInt8*)m_pXEnd - (AkUInt8*)pfCur);
	AkUInt32 uPretechBlockSize = AkMin( uBytesBeforeWrap, uBytesToPrefetch );
	for( unsigned int i = 0; i < uPretechBlockSize; i += AKSIMD_ARCHCACHELINESIZE )	
	{
		AKSIMD_PREFETCHMEMORY(i,pfCur); 
	}

	if ( uPretechBlockSize < uBytesToPrefetch )
	{
		// Wrap and continue prefetch
		pfCur = (AkReal32 * AK_RESTRICT) m_pX;
		uBytesBeforeWrap = AkUInt32((AkUInt8*)m_pXEnd - (AkUInt8*)pfCur);
		uPretechBlockSize = AkMin( uBytesBeforeWrap, uBytesToPrefetch-uPretechBlockSize );
		for( unsigned int i = 0; i < uPretechBlockSize; i += AKSIMD_ARCHCACHELINESIZE )	
		{
			AKSIMD_PREFETCHMEMORY(i,pfCur);
		}
	}
}

#endif // _AKDELAY_H_
