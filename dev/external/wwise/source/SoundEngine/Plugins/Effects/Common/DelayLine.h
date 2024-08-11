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

// Simple delay y[n] = x[n - D] 
// Works for delays smaller than number of frames to process (wraps multiple times) if used as below:
// Non PIC usage 
// -> ProcessBuffer( pInOutData, uFramesToProcess )
// PIC usage for delay line > uFramesToProcess 
// -> DMA delay line from current position single DMA sufficient for less than 4096 frames
// -> CurrentPos is the start of first DMA + Offset because DMA was 16 bytes aligned
// -> Wrap pos is the second DMA buffer received (address aligned so no offset to handle)
// PIC usage for delay line <= uFramesToProcess 
// -> DMA delay line from its start (address aligned so no offset to handle)
// -> CurrentPos is the start of first DMA + uCurOffset (no DMA offset necessary)
// -> Wrap pos is same DMA buffer (address aligned so no offset to handle)

// Also supports single sample process

#ifndef _AKDELAYLINE_H_
#define _AKDELAYLINE_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "SPUInline.h"
#ifndef __SPU__
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#else
#include "AkUnalignedDataPS3.h"
#endif
#include <AK/SoundEngine/Common/AkSimd.h>

namespace DSP
{

	class DelayLine
	{
	public:

#ifndef __SPU__

		DelayLine() : 
			uDelayLineLength( 0 ), 
			pfDelay( NULL ), 
#ifdef AK_WIIU
			m_pUnalignedDelay(NULL),
			m_pCache(NULL),
			m_pUncachedData(NULL),
			m_uCachedBlocks(0),
			m_uCacheWriteOffset(0),
			m_uLastPart(0),
#endif
			uCurOffset( 0 )
			{}

		AKRESULT Init( AK::IAkPluginMemAlloc * in_pAllocator, AkUInt32 in_uDelayLineLength );
		void Term( AK::IAkPluginMemAlloc * in_pAllocator );
		void Reset( );
		void ProcessBuffer( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames ); // In-place
		void ProcessBuffer(	AkReal32 * in_pfInBuffer, 
			AkReal32 * out_pfOutBuffer, 
			AkUInt32 in_uNumFrames ); // Out-of-place

		AkReal32 ProcessSample( AkReal32 in_fIn )
		{
			AkReal32 fDelay = pfDelay[uCurOffset];		
			pfDelay[uCurOffset] = in_fIn;
			++uCurOffset;
			if ( uCurOffset == uDelayLineLength )
				uCurOffset = 0;
			return fDelay;
		}

		AkReal32 ReadSample( )
		{
			return pfDelay[uCurOffset];
		}

		void WriteSample( AkReal32 in_fIn )
		{
			pfDelay[uCurOffset] = in_fIn;
			++uCurOffset;
			if ( uCurOffset == uDelayLineLength )
				uCurOffset = 0;
		}

		void WriteSampleNoWrapCheck( AkReal32 in_fIn )
		{
			pfDelay[uCurOffset] = in_fIn;
			++uCurOffset;
		}

		AkReal32 ReadSample( AkUInt32 in_curOffset )
		{
			return pfDelay[in_curOffset];
		}

		void WriteSample( AkReal32 in_fIn, AkUInt32 & in_curOffset )
		{
			pfDelay[in_curOffset] = in_fIn;
			++in_curOffset;
			if ( in_curOffset == uDelayLineLength )
				in_curOffset = 0;
		}

		void WriteSampleNoWrapCheck( AkReal32 in_fIn, AkUInt32 & in_curOffset )
		{
			pfDelay[in_curOffset] = in_fIn;
			++in_curOffset;
		}

#ifdef AK_WIIU
		AkReal32 *m_pUnalignedDelay;
		AkReal32 *m_pCache;
		AkReal32 *m_pUncachedData;
		AkUInt32 m_uCachedBlocks;
		AkUInt32 m_uCacheWriteOffset;
		AkUInt32 m_uLastPart;

		bool InitCache(AkUInt32 in_uSamples);
		void TermCache()
		{
			if (m_pCache != NULL)
			{
				//Reset the data pointer until next time
				pfDelay = m_pUncachedData;
				LCDealloc(m_pCache);
				m_pCache = NULL;
			}
		}	

		void CacheNextBlock()
		{
			if (m_pCache && m_uLastPart)
			{
				m_uCacheWriteOffset = 0;
				m_uCachedBlocks = m_uLastPart / CACHE_BLOCK_SIZE;
				DCFlushRange(m_pUncachedData, m_uLastPart);
				LCLoadDMABlocks(m_pCache, m_pUncachedData, m_uCachedBlocks);
				LCWaitDMAQueue(0);	//Just in case, LCLoadDMABlocks is blocking for now.				

				//Setup the pfDelay ptr at the right place in the CACHED memory
				//The rest of the class will then behave normally.
				pfDelay = m_pCache;
			}
		}

		//Flush the new delay data from locked cache
		void WriteDelayData()
		{
			if (m_pCache != NULL)
			{
				LCStoreDMABlocks((void*)AK_WIIU_ALIGN_DOWN_CACHE(m_pUncachedData + m_uCacheWriteOffset), m_pCache, m_uCachedBlocks);
				LCWaitDMAQueue(0); //Just in case, LCLoadDMABlocks is blocking for now.
			}
		}
#endif

		void WrapDelayLine()
		{
			if ( uCurOffset == uDelayLineLength )
			{
#ifdef AK_WIIU
				WriteDelayData();
				CacheNextBlock();				
#endif
				uCurOffset = 0;
			}
		}

#if defined(AKSIMD_V2F32_SUPPORTED)
		void ReadSamples( AKSIMD_V2F32 &out_Low, AKSIMD_V2F32 &out_High )
		{
			out_Low = AKSIMD_LOAD_V2F32( pfDelay + uCurOffset );
			out_High = AKSIMD_LOAD_V2F32( pfDelay + uCurOffset + 2 );
		}
#elif defined(AKSIMD_V4F32_SUPPORTED) && !defined(AK_PS3)
		void ReadSamples( AKSIMD_V4F32 &out )
		{
			out = AKSIMD_LOADU_V4F32( pfDelay + uCurOffset );
		}
#endif

#if defined(AKSIMD_V2F32_SUPPORTED)
		void WriteSamplesNoWrapCheck(AKSIMD_V2F32 in_vfIn)
		{
			AKSIMD_STORE_V2F32( pfDelay + uCurOffset, in_vfIn );
			uCurOffset += 2;
		}
#elif defined(AKSIMD_V4F32_SUPPORTED) && !defined(AK_PS3)
		void WriteSamplesNoWrapCheck(AKSIMD_V4F32 in_vfIn)
		{
			AKSIMD_STOREU_V4F32( pfDelay + uCurOffset, in_vfIn );
			uCurOffset += 4;
		}
#endif

#ifdef __PPU__
		AkUInt32 GetScratchMemorySizeRequired( AkUInt32 in_uNumFrames );
#endif

#else
		// Position independent code routine
		void ProcessBuffer(	
			AkReal32 * io_pfBuffer, 
			AkUInt32 in_uNumFrames, 
			AkReal32 * in_pfDelayCurPos, 
			AkReal32 * in_pfDelayWrapPos ); // In-place

		void ProcessBuffer(	
			AkReal32 * in_pfInBuffer, 
			AkReal32 * out_pfOutBuffer, 
			AkUInt32 in_uNumFrames, 
			AkReal32 * in_pfDelayCurPos, 
			AkReal32 * in_pfDelayWrapPos ); // Out-of-place

		void GetDelayMemory(	
			AkReal32 *& out_pfCurPos,
			AkReal32 *& out_pfWrapPos,
			AkUInt32 & out_ruCurOffsetBefore,
			AkReal32 *& io_pfScratchMemLoc,
			AkUInt32 in_uNumFrames,
			AkUInt32 in_uDMATag );

		void PutDelayMemory(	
			AkUInt32 in_uCurOffsetBefore,
			AkReal32 *& io_pfScratchMemLoc,
			AkUInt32 in_uNumFrames,
			AkUInt32 in_uDMATag );

		AkReal32 ProcessSample(	
			AkReal32 in_fIn,
			AkReal32 *& io_pfDelayCurPos, 
			AkReal32 * in_pfDelayWrapPos )
		{
			AkReal32 fDelay = *io_pfDelayCurPos;		
			*io_pfDelayCurPos++ = in_fIn;
			++uCurOffset;
			if ( uCurOffset == uDelayLineLength )
				io_pfDelayCurPos = in_pfDelayWrapPos;
			return fDelay;
		}

		AkReal32 ReadSample( AkReal32 * in_pfDelayCurPos )
		{
			return *in_pfDelayCurPos;
		}

		vec_float4 ReadSamples( AkReal32 * in_pfDelayCurPos )
		{
			return AKSIMD_ReadUnalignedVector<vec_float4>( in_pfDelayCurPos );
		}

		void WriteSample(	
			AkReal32 in_fIn,
			AkReal32 *& io_pfDelayCurPos, 
			AkReal32 * in_pfDelayWrapPos )
		{
			*io_pfDelayCurPos++ = in_fIn;
			++uCurOffset;
			if ( uCurOffset == uDelayLineLength )
			{
				io_pfDelayCurPos = in_pfDelayWrapPos;
				uCurOffset = 0;
			}
		}

		void WriteSampleNoWrapCheck(	
			AkReal32 in_fIn,
			AkReal32 *& io_pfDelayCurPos )
		{
			*io_pfDelayCurPos++ = in_fIn;
			++uCurOffset;
		}

		void WriteSamplesNoWrapCheck(	
			vec_float4 in_vfIn,
			AkReal32 *& io_pfDelayCurPos )
		{
			AKSIMD_StoreUnalignedVector( io_pfDelayCurPos, in_vfIn );
			io_pfDelayCurPos += 4;
			uCurOffset += 4;
		}
#endif

		AkUInt32 GetDelayLength( )
		{
			return uDelayLineLength;
		}

//	protected:

		AkUInt32 uDelayLineLength;		// DelayLine line length
		AkReal32 * pfDelay;				// DelayLine line start position
		AkUInt32 uCurOffset;			// DelayLine line read/write position	
	};

} // namespace DSP

#endif // _AKDELAYLINE_H_
