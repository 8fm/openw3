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

#include "DelayLine.h"
#ifdef __SPU__
#include "AkUnalignedDataPS3.h"
#include <AK/Plugin/PluginServices/PS3/MultiCoreServices.h>
#include <AK/Plugin/PluginServices/PS3/SPUServices.h>
#else
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <string.h>
#endif
#include <AK/Tools/Common/AkAssert.h>

namespace DSP
{

#ifndef __SPU__
	AKRESULT DelayLine::Init( AK::IAkPluginMemAlloc * in_pAllocator, AkUInt32 in_uDelayLineLength )
	{
		//at least four sample delay
		if ( in_uDelayLineLength < 4 )
			in_uDelayLineLength = 4;
		uDelayLineLength = in_uDelayLineLength; 
		AkUInt32 uSize = sizeof(AkReal32) * ((uDelayLineLength + 3) & ~3); // Round size to make sure unaligned simd accesses don't corrupt memory
#ifdef AK_WIIU
		uSize = AK_WIIU_ALIGN_UP_CACHE(uSize+LL_CACHE_FETCH_SIZE);	//Allow some "unalignment" at the begining and the end
		m_pCache = NULL;
		m_pUncachedData = NULL;
		m_pUnalignedDelay = NULL;
#endif
		pfDelay = (AkReal32*)AK_PLUGIN_ALLOC( in_pAllocator, AK_ALIGN_SIZE_FOR_DMA(uSize));
		if ( pfDelay == NULL )
			return AK_InsufficientMemory;

#ifdef AK_WIIU
		m_pUnalignedDelay = pfDelay;
		pfDelay = (AkReal32*)AK_WIIU_ALIGN_UP_CACHE(m_pUnalignedDelay);
#endif

		uCurOffset = 0;
		return AK_Success;
	}

	void DelayLine::Term( AK::IAkPluginMemAlloc * in_pAllocator )
	{
#ifdef AK_WIIU
		pfDelay = m_pUnalignedDelay;
		m_pUnalignedDelay = NULL;
#endif	
		if ( pfDelay )
		{
			AK_PLUGIN_FREE( in_pAllocator, pfDelay );
			pfDelay = NULL;
		}
		uDelayLineLength = 0;
	}

	void DelayLine::Reset( )
	{
		if ( pfDelay )
		{
			AkZeroMemLarge( pfDelay, AK_ALIGN_SIZE_FOR_DMA( sizeof(AkReal32) * uDelayLineLength ) );
		}
	}

#if defined(AKSIMD_V4F32_SUPPORTED) && !defined(AK_PS3)

	// Process delay buffer in place when access to direct memory is ok (not PIC)
	void DelayLine::ProcessBuffer( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames )
	{
		AkReal32 * AK_RESTRICT pfDelayPtr = (AkReal32 * AK_RESTRICT) &pfDelay[uCurOffset];
		AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pfBuffer;

		while ( in_uNumFrames )
		{
			AkUInt32 uFramesBeforeWrap = uDelayLineLength - uCurOffset;
			AkUInt32 uFramesToProcess = AkMin( in_uNumFrames, uFramesBeforeWrap );
			AkUInt32 uNumSimdIter = uFramesToProcess >> 2; // divide by 4
			AkUInt32 uNumScalarIter = uFramesToProcess & 3; // modulo 4

			for ( AkUInt32 i = 0; i < uNumSimdIter; ++i )
			{
				AKSIMD_V4F32 vfDelay = AKSIMD_LOADU_V4F32( pfDelayPtr );
				AKSIMD_V4F32 vfBuf = AKSIMD_LOADU_V4F32( pfBuf );
				AKSIMD_STOREU_V4F32( pfDelayPtr, vfBuf );
				AKSIMD_STOREU_V4F32( pfBuf, vfDelay );
				pfDelayPtr += 4;
				pfBuf += 4;
			}

			for ( AkUInt32 i = 0; i < uNumScalarIter; ++i )
			{
				AkReal32 fDelay = *pfDelayPtr;
				*pfDelayPtr++ = *pfBuf;
				*pfBuf++ = fDelay;
			}

			uCurOffset += uFramesToProcess;
			AKASSERT( uCurOffset <= uDelayLineLength );

			if ( uCurOffset == uDelayLineLength )
			{
				pfDelayPtr = (AkReal32 * AK_RESTRICT) pfDelay;
				uCurOffset = 0;
			}

			in_uNumFrames -= uFramesToProcess;
		}
	}

	// Process delay buffer out-of-place when access to direct memory is ok (not PIC)
	void DelayLine::ProcessBuffer(	
		AkReal32 * in_pfInBuffer, 
		AkReal32 * out_pfOutBuffer, 
		AkUInt32 in_uNumFrames )
	{
		AkReal32 * AK_RESTRICT pfDelayPtr = (AkReal32 * AK_RESTRICT) &pfDelay[uCurOffset];
		AkReal32 * AK_RESTRICT pfInBuf = (AkReal32 * AK_RESTRICT) in_pfInBuffer;
		AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) out_pfOutBuffer;

		while ( in_uNumFrames )
		{
			AkUInt32 uFramesBeforeWrap = uDelayLineLength - uCurOffset;
			AkUInt32 uFramesToProcess = AkMin( in_uNumFrames, uFramesBeforeWrap );
			AkUInt32 uNumSimdIter = uFramesToProcess >> 2;
			AkUInt32 uNumScalarIter = uFramesToProcess & 3;

			for ( AkUInt32 i = 0; i < uNumSimdIter; ++i )
			{
				AKSIMD_V4F32 vfDelay = AKSIMD_LOADU_V4F32( pfDelayPtr );
				AKSIMD_V4F32 vfBuf = AKSIMD_LOADU_V4F32( pfInBuf );
				AKSIMD_STOREU_V4F32( pfDelayPtr, vfBuf );
				AKSIMD_STOREU_V4F32( pfOutBuf, vfDelay );
				pfDelayPtr += 4;
				pfInBuf += 4;
				pfOutBuf += 4;
			}

			for ( AkUInt32 i = 0; i < uNumScalarIter; ++i )
			{
				AkReal32 fDelay = *pfDelayPtr;
				*pfDelayPtr++ = *pfInBuf++;
				*pfOutBuf++ = fDelay;
			}

			uCurOffset += uFramesToProcess;
			AKASSERT( uCurOffset <= uDelayLineLength );

			if ( uCurOffset == uDelayLineLength )
			{
				pfDelayPtr = (AkReal32 * AK_RESTRICT) pfDelay;
				uCurOffset = 0;
			}

			in_uNumFrames -= uFramesToProcess;
		}
	}

#else // defined(AKSIMD_V4F32_SUPPORTED) && !defined(AK_PS3)

	// Process delay buffer in place when access to direct memory is ok (not PIC)
	void DelayLine::ProcessBuffer( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames )
	{
#ifdef AK_WIIU
		InitCache(in_uNumFrames);
#endif

		AkReal32 * AK_RESTRICT pfDelayPtr = (AkReal32 * AK_RESTRICT) &pfDelay[uCurOffset];
		AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pfBuffer;
		AkUInt32 uFramesBeforeWrap = uDelayLineLength - uCurOffset;
		if ( uFramesBeforeWrap > in_uNumFrames )
		{
			// Not wrapping this time
			AkUInt32 i = in_uNumFrames;
			while ( i-- )
			{
				AkReal32 fDelay = *pfDelayPtr;		
				*pfDelayPtr++ = *pfBuf;			
				*pfBuf++ = fDelay;
			}
			uCurOffset += in_uNumFrames;
			AKASSERT( uCurOffset <= uDelayLineLength );
		}
		else
		{
			// Minimum number of wraps
			AkUInt32 uFramesRemainingToProcess = in_uNumFrames;
			while ( uFramesRemainingToProcess )
			{
				AkUInt32 uFramesToProcess = AkMin(uFramesRemainingToProcess,uFramesBeforeWrap);
				for ( AkUInt32 i = 0; i < uFramesToProcess; ++i )
				{
					AkReal32 fDelay = *pfDelayPtr;
					*pfDelayPtr++ = *pfBuf;
					*pfBuf++ = fDelay;
				}

				uCurOffset += uFramesToProcess;
				AKASSERT( uCurOffset <= uDelayLineLength );

				if ( uCurOffset == uDelayLineLength )
				{
#ifdef AK_WIIU
					WrapDelayLine();
#endif
					pfDelayPtr = (AkReal32 * AK_RESTRICT) pfDelay;
					uCurOffset = 0;
				}

				uFramesRemainingToProcess -= uFramesToProcess;
				uFramesBeforeWrap = uDelayLineLength - uCurOffset;
			}
		}

#ifdef AK_WIIU
		WriteDelayData();
		TermCache();
#endif
	}

	// Process delay buffer out-of-place when access to direct memory is ok (not PIC)
	void DelayLine::ProcessBuffer(	
		AkReal32 * in_pfInBuffer, 
		AkReal32 * out_pfOutBuffer, 
		AkUInt32 in_uNumFrames )
	{

#ifdef AK_WIIU
		InitCache(in_uNumFrames);
#endif

		AkReal32 * AK_RESTRICT pfDelayPtr = (AkReal32 * AK_RESTRICT) &pfDelay[uCurOffset];
		AkReal32 * AK_RESTRICT pfBufIn = (AkReal32 * AK_RESTRICT) in_pfInBuffer;
		AkReal32 * AK_RESTRICT pfBufOut = (AkReal32 * AK_RESTRICT) out_pfOutBuffer;
		AkUInt32 uFramesBeforeWrap = uDelayLineLength - uCurOffset;
		if ( uFramesBeforeWrap > in_uNumFrames )
		{
			// Not wrapping this time
			AkUInt32 i = in_uNumFrames;
			while ( i-- )
			{
				AkReal32 fDelay = *pfDelayPtr;
				*pfDelayPtr++ = *pfBufIn++;	
				*pfBufOut++ = fDelay;
			}
			uCurOffset += in_uNumFrames;
			AKASSERT( uCurOffset <= uDelayLineLength );
		}
		else
		{
			// Minimum number of wraps
			AkUInt32 uFramesRemainingToProcess = in_uNumFrames;
			while ( uFramesRemainingToProcess )
			{
				AkUInt32 uFramesToProcess = AkMin(uFramesRemainingToProcess,uFramesBeforeWrap);
				for ( AkUInt32 i = 0; i < uFramesToProcess; ++i )
				{
					AkReal32 fDelay = *pfDelayPtr;
					*pfDelayPtr++ = *pfBufIn++;
					*pfBufOut++ = fDelay;
				}
				uCurOffset += uFramesToProcess;
				AKASSERT( uCurOffset <= uDelayLineLength );

				if ( uCurOffset == uDelayLineLength )
				{
#ifdef AK_WIIU
					WrapDelayLine();
#endif
					pfDelayPtr = (AkReal32 * AK_RESTRICT) pfDelay;
					uCurOffset = 0;
				}

				uFramesRemainingToProcess -= uFramesToProcess;
				uFramesBeforeWrap = uDelayLineLength - uCurOffset;
			}
		}

#ifdef AK_WIIU
		WriteDelayData();
		TermCache();
#endif
	}

#endif // defined(AKSIMD_V4F32_SUPPORTED)

#ifdef __PPU__
	AkUInt32 DelayLine::GetScratchMemorySizeRequired( AkUInt32 in_uNumFrames )
	{
		AKASSERT( in_uNumFrames % 4 == 0 );
		if ( uDelayLineLength )
		{
			if ( uDelayLineLength <= in_uNumFrames+4 )
				return AK_ALIGN_SIZE_FOR_DMA( uDelayLineLength*sizeof(AkReal32) );
			else
			{
				// Some waste due to 2 memory transfer alignment
				return AK_ALIGN_SIZE_FOR_DMA( (in_uNumFrames+3*2)*sizeof(AkReal32) );
			}
		}
		else
		{
			return 0;
		}
	}
#endif // __PPU__

#else // #ifndef __SPU__

	void DelayLine::GetDelayMemory(	
		AkReal32 *& out_pfCurPos,
		AkReal32 *& out_pfWrapPos,
		AkUInt32 & out_ruCurOffsetBefore,
		AkReal32 *& io_pfScratchMemLoc,
		AkUInt32 in_uNumFrames,
		AkUInt32 in_uDMATag )
	{
		out_ruCurOffsetBefore = 0;
		if ( uDelayLineLength )
		{
			AKASSERT( ((AkUInt32)io_pfScratchMemLoc & 0xF) == 0 );
			out_ruCurOffsetBefore = uCurOffset;
			// WG-17055 Consider delay lines that are almost the number of frames to be processed this iteration
			// as a 'small' delay line, to avoid producing bytes in the padding region that will never make it to main memory
			if ( uDelayLineLength <= in_uNumFrames + 4 )
			{
				// PIC usage for delay line <= uFramesToProcess 
				// -> DMA delay line from its start (address aligned so no offset to handle)

				// -> CurrentPos is the start of first DMA + uCurOffset (no DMA offset necessary)
				// -> Wrap pos is the start of the same DMA buffer (address aligned so no offset to handle)		
				AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( uDelayLineLength*sizeof(AkReal32) );
				AKASSERT( uDMASize <= 16*1024 );
				AkDmaGet(	
					"GetShortDelay",
					io_pfScratchMemLoc,
					(std::uint64_t)pfDelay,
					uDMASize,
					in_uDMATag,0,0);
				//AkDmaWait(1<<in_uDMATag); // FTDB
				out_pfCurPos = io_pfScratchMemLoc + uCurOffset;
				out_pfWrapPos = io_pfScratchMemLoc;
				io_pfScratchMemLoc += uDMASize/sizeof(AkReal32); 
			}
			else
			{
				// PIC usage for delay line > uFramesToProcess 
				// -> DMA delay line from current position single DMA sufficient for less than 4096 frames
				// -> CurrentPos is the start of first DMA + Offset because DMA was 16 bytes aligned
				// -> Wrap pos is the second DMA buffer received (address aligned so no offset to handle)
				AkUInt32 uFramesBeforeWrap = uDelayLineLength-uCurOffset;
				AkUInt32 uNumFirstDMAFrames = uFramesBeforeWrap;
				if ( uNumFirstDMAFrames > in_uNumFrames )
					uNumFirstDMAFrames = in_uNumFrames;
				AkReal32 * pfCurDelayPos = pfDelay + uCurOffset;
				AkReal32 * pfCurDelayPosDMA = (AkReal32 *) ( (AkUInt32) pfCurDelayPos & ~0xf ); // DMA must be 16-aligned
				AkUInt32 uDelayDMAOffset = (AkUInt32)pfCurDelayPos & 0xF;
				AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( uNumFirstDMAFrames*sizeof(AkReal32) + uDelayDMAOffset );
				AKASSERT( uDMASize <= 16*1024 );
				AkDmaGet(	
					"GetLongDelay1",
					io_pfScratchMemLoc,
					(std::uint64_t)pfCurDelayPosDMA,
					uDMASize,
					in_uDMATag,0,0);
				//AkDmaWait(1<<in_uDMATag); // FTDB
				out_pfCurPos = io_pfScratchMemLoc + uDelayDMAOffset/sizeof(AkReal32);
				io_pfScratchMemLoc += uDMASize/sizeof(AkReal32); 

				if ( uFramesBeforeWrap >= in_uNumFrames )
				{
					// Delay will not wrap this execution, single DMA sufficient
					out_pfWrapPos = NULL;
				}
				else
				{
					AkUInt32 uFramesAfterWrap = in_uNumFrames - uFramesBeforeWrap;
					// Need a second (address aligned) DMA to handle wrap
					uDMASize = AK_ALIGN_SIZE_FOR_DMA( uFramesAfterWrap*sizeof(AkReal32) );
					AKASSERT( uDMASize <= 16*1024 );
					AkDmaGet(	
						"GetLongDelay2",
						io_pfScratchMemLoc,
						(std::uint64_t)pfDelay,
						uDMASize,
						in_uDMATag,0,0);
					//AkDmaWait(1<<in_uDMATag); // FTDB
					out_pfWrapPos = io_pfScratchMemLoc;
					io_pfScratchMemLoc += uDMASize/sizeof(AkReal32); 
				}
			}
		}
	}

	void DelayLine::PutDelayMemory(	
		AkUInt32 in_uCurOffsetBefore,
		AkReal32 *& io_pfScratchMemLoc,
		AkUInt32 in_uNumFrames,
		AkUInt32 in_uDMATag )
	{
		if ( uDelayLineLength )
		{
			AKASSERT( ((AkUInt32)io_pfScratchMemLoc & 0xF) == 0 );
			// WG-17055 Consider delay lines that are almost the number of frames to be processed this iteration
			// as a 'small' delay line, to avoid producing bytes in the padding region that will never make it to main memory
			if ( uDelayLineLength <= in_uNumFrames + 4 )
			{
				// Single DMA necessary
				AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( uDelayLineLength*sizeof(AkReal32) );
				AKASSERT( uDMASize <= 16*1024 );
				AkDmaPut(	
					"PutShortDelay",
					io_pfScratchMemLoc,
					(std::uint64_t)pfDelay,
					uDMASize,
					in_uDMATag,0,0);
				//AkDmaWait(1<<in_uDMATag); // FTDB
				io_pfScratchMemLoc += uDMASize/sizeof(AkReal32); 
			}
			else
			{
				// PIC usage for delay line > uFramesToProcess 
				// -> DMA delay line from current position single DMA sufficient for less than 4096 frames
				// -> CurrentPos is the start of first DMA + Offset because DMA was 16 bytes aligned
				// -> Wrap pos is the second DMA buffer received (address aligned so no offset to handle)
				AkUInt32 uFramesBeforeWrap = uDelayLineLength-in_uCurOffsetBefore;
				AkUInt32 uNumFirstDMAFrames = uFramesBeforeWrap;
				if ( uNumFirstDMAFrames > in_uNumFrames )
					uNumFirstDMAFrames = in_uNumFrames;
				AkReal32 * pfCurDelayPos = pfDelay + in_uCurOffsetBefore;
				AkReal32 * pfCurDelayPosDMA = (AkReal32 *) ( (AkUInt32) pfCurDelayPos & ~0xf ); // DMA must be 16-aligned
				AkUInt32 uDelayDMAOffset = (AkUInt32)pfCurDelayPos & 0xF;
				AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( uNumFirstDMAFrames*sizeof(AkReal32) + uDelayDMAOffset );
				AKASSERT( uDMASize <= 16*1024 );
				AkDmaPut(	
					"PutLongDelay1",
					io_pfScratchMemLoc,
					(std::uint64_t)pfCurDelayPosDMA,
					uDMASize,
					in_uDMATag,0,0);
				//AkDmaWait(1<<in_uDMATag); // FTDB
				io_pfScratchMemLoc += uDMASize/sizeof(AkReal32); 

				if ( uFramesBeforeWrap < in_uNumFrames )
				{
					AkUInt32 uFramesAfterWrap = in_uNumFrames - uFramesBeforeWrap;
					// Need a second (address aligned) DMA to handle wrap
					uDMASize = AK_ALIGN_SIZE_FOR_DMA( uFramesAfterWrap*sizeof(AkReal32) );
					AKASSERT( uDMASize <= 16*1024 );
					AkDmaPut(	
						"PutLongDelay2",
						io_pfScratchMemLoc,
						(std::uint64_t)pfDelay,
						uDMASize,
						in_uDMATag,0,0);
					//AkDmaWait(1<<in_uDMATag); // FTDB
					io_pfScratchMemLoc += uDMASize/sizeof(AkReal32); 
				}
			}
		}
	}

	// Supports PIC
	// Process delay buffer in-place, starting from start position untill end position, wraps to 
	// (a possibly discontiguous) memory location given by WrapPos when the end is reached
	void DelayLine::ProcessBuffer(	
		AkReal32 * io_pfBuffer, 
		AkUInt32 in_uNumFrames, 
		AkReal32 * in_pfDelayCurPos, 
		AkReal32 * in_pfDelayWrapPos )
	{
		AkReal32 * AK_RESTRICT pfDelayPtr = (AkReal32 * AK_RESTRICT) in_pfDelayCurPos;
		AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pfBuffer;
		AkUInt32 uFramesBeforeWrap = uDelayLineLength-uCurOffset;
		if ( uFramesBeforeWrap > in_uNumFrames )
		{
			// Not wrapping this time
			AkUInt32 i = in_uNumFrames;
			while ( i >= 4 )
			{
				vec_float4 vfDelay = AKSIMD_ReadUnalignedVector<vec_float4>( pfDelayPtr );
				vec_float4 vfIn = AKSIMD_ReadUnalignedVector<vec_float4>( pfBuf );
				AKSIMD_StoreUnalignedVector( pfDelayPtr, vfIn );
				AKSIMD_StoreUnalignedVector( pfBuf, vfDelay );
				pfDelayPtr += 4;
				pfBuf += 4;
				i -= 4;
			}
			while(i > 0)
			{
				AkReal32 fDelay = *pfDelayPtr;
				*pfDelayPtr++ = *pfBuf;
				*pfBuf++ = fDelay;
				--i;
			}
			uCurOffset += in_uNumFrames;
			AKASSERT( uCurOffset >= 0 && uCurOffset <= uDelayLineLength );
		}
		else
		{
			// Minimum number of wraps
			AkUInt32 uFramesRemainingToProcess = in_uNumFrames;
			while ( uFramesRemainingToProcess )
			{
				AkUInt32 uFramesToProcess = (uFramesRemainingToProcess < uFramesBeforeWrap) 
					? uFramesRemainingToProcess :  uFramesBeforeWrap;
				AkUInt32 i = uFramesToProcess;
				while ( i >= 4 )
				{
					vec_float4 vfDelay = AKSIMD_ReadUnalignedVector<vec_float4>( pfDelayPtr );
					vec_float4 vfIn = AKSIMD_ReadUnalignedVector<vec_float4>( pfBuf );
					AKSIMD_StoreUnalignedVector( pfDelayPtr, vfIn );
					AKSIMD_StoreUnalignedVector( pfBuf, vfDelay );
					pfDelayPtr += 4;
					pfBuf += 4;
					i -= 4;
				}
				while(i > 0)
				{
					AkReal32 fDelay = *pfDelayPtr;
					*pfDelayPtr++ = *pfBuf;
					*pfBuf++ = fDelay;
					--i;
				}
				uCurOffset += uFramesToProcess;
				AKASSERT( uCurOffset >= 0 && uCurOffset <= uDelayLineLength );

				if ( uCurOffset == uDelayLineLength )
				{
					pfDelayPtr = (AkReal32 * AK_RESTRICT) in_pfDelayWrapPos;
					uCurOffset = 0;
				}

				uFramesRemainingToProcess -= uFramesToProcess;
				uFramesBeforeWrap = uDelayLineLength-uCurOffset;
			}
		}
	}

	// Supports PIC
	// Process delay buffer out-of-place, starting from start position untill end position, wraps to 
	// (a possibly discontiguous) memory location given by WrapPos when the end is reached
	void DelayLine::ProcessBuffer(	
		AkReal32 * in_pfInBuffer, 
		AkReal32 * out_pfOutBuffer, 
		AkUInt32 in_uNumFrames, 
		AkReal32 * in_pfDelayCurPos, 
		AkReal32 * in_pfDelayWrapPos )
	{
		AkReal32 * AK_RESTRICT pfDelayPtr = (AkReal32 * AK_RESTRICT) in_pfDelayCurPos;
		AkReal32 * AK_RESTRICT pfBufIn = (AkReal32 * AK_RESTRICT) in_pfInBuffer;
		AkReal32 * AK_RESTRICT pfBufOut = (AkReal32 * AK_RESTRICT) out_pfOutBuffer;
		AkUInt32 uFramesBeforeWrap = uDelayLineLength-uCurOffset;
		if ( uFramesBeforeWrap > in_uNumFrames )
		{
			// Not wrapping this time
			AkUInt32 i = in_uNumFrames;
			while( i-- > 0 )
			{
				AkReal32 fDelay = *pfDelayPtr;
				*pfDelayPtr++ = *pfBufIn++;
				*pfBufOut++ = fDelay;
			}
			uCurOffset += in_uNumFrames;
			AKASSERT( uCurOffset >= 0 && uCurOffset <= uDelayLineLength );
		}
		else
		{
			// Minimum number of wraps
			AkUInt32 uFramesRemainingToProcess = in_uNumFrames;
			while ( uFramesRemainingToProcess )
			{
				AkUInt32 uFramesToProcess = (uFramesRemainingToProcess < uFramesBeforeWrap) 
					? uFramesRemainingToProcess :  uFramesBeforeWrap;
				for ( AkUInt32 i = 0; i < uFramesToProcess; ++i )
				{
					AkReal32 fDelay = *pfDelayPtr;
					*pfDelayPtr++ = *pfBufIn++;
					*pfBufOut++ = fDelay;
				}
				uCurOffset += uFramesToProcess;
				AKASSERT( uCurOffset >= 0 && uCurOffset <= uDelayLineLength );

				if ( uCurOffset == uDelayLineLength )
				{
					pfDelayPtr = (AkReal32 * AK_RESTRICT) in_pfDelayWrapPos;
					uCurOffset = 0;
				}

				uFramesRemainingToProcess -= uFramesToProcess;
				uFramesBeforeWrap = uDelayLineLength-uCurOffset;
			}
		}
	}

#endif // #ifndef __SPU__

#ifdef AK_WIIU
	bool DelayLine::InitCache(AkUInt32 in_uSamples)
	{
		m_pUncachedData = pfDelay;
		m_uLastPart = 0;

		//Check if the delay line fits entirely in the block.
		if (uDelayLineLength <= in_uSamples)
		{
			AkUInt32 uSize = uDelayLineLength * sizeof(AkReal32);
			uSize = AK_WIIU_ALIGN_UP_CACHE(uSize);

			//Allocate minimum
			m_pCache = (AkReal32 *)LCAlloc(AK_WIIU_ROUNDUP_LCALLOC_SIZE(uSize));
			if (!m_pCache)
				return false;
			
			//Load all blocks.  
			//TODO TODO: When Nintendo finally resolve their DMA vs code execution concurrency problem, we will need to split and pipeline the blocks loading
			//pfDelay should already be aligned during the allocation
			AKASSERT(pfDelay == (AkReal32*)AK_WIIU_ALIGN_DOWN_CACHE(pfDelay));
			m_uCachedBlocks = uSize / CACHE_BLOCK_SIZE;
			DCFlushRange(pfDelay, uSize);
			LCLoadDMABlocks(m_pCache, pfDelay, m_uCachedBlocks);
			
			//Setup the pfDelay ptr at the right place in the CACHED memory
			//The rest of the class will then behave normally.
			pfDelay = m_pCache; 
			m_uCacheWriteOffset = 0;
		}
		else
		{
			//It doesn't fit.  
			//We can have 2 situations: 
			//A) the next 256 samples are contiguous
			//B) the next 256 samples are split because of the end of the delay line buffer.
			AkUInt32 uDelayRemaining = uDelayLineLength - uCurOffset;	
			AkUIntPtr pAlignedData = AK_WIIU_ALIGN_DOWN_CACHE(pfDelay + uCurOffset);
			AkUInt32 uPrologue = (AkUIntPtr)(pfDelay + uCurOffset) - pAlignedData;

			//The SOURCE data must be aligned on 64 bytes.  
			if (in_uSamples <= uDelayRemaining)
			{
				//Case A, everything is contiguous.
				//The size to transfer must be aligned to 64 bytes too
				AkUInt32 uSize = AK_WIIU_ALIGN_UP_CACHE(pfDelay + uCurOffset + in_uSamples) - pAlignedData;
				AKASSERT(uSize % LL_CACHE_FETCH_SIZE == 0);	

				//Allocate minimum
				m_pCache = (AkReal32 *)LCAlloc(AK_WIIU_ROUNDUP_LCALLOC_SIZE(uSize));
				if (!m_pCache)
					return false;

				//Load all blocks.  
				//TODO TODO: When Nintendo finally resolve their DMA vs code execution concurrency problem, we will need to split and pipeline the blocks loading
				m_uCachedBlocks = uSize / CACHE_BLOCK_SIZE;
				DCFlushRange((void*)pAlignedData, uSize);
				LCLoadDMABlocks(m_pCache, (void*)pAlignedData, m_uCachedBlocks);
			}
			else
			{
				//Case B, the data is split in two.
				//In this case, reserve the largest area needed between the first part and the second part.  Not the whole thing.  
				//The algorithms need to take in account the pointer wraparound anyway, so we'll fetch the data needed at that point.
				AkUInt32 uSizePart1 = AK_WIIU_ALIGN_UP_CACHE(pfDelay + uDelayLineLength) - pAlignedData;
				m_uLastPart = AK_WIIU_ALIGN_UP_CACHE(in_uSamples - uDelayRemaining) * sizeof(AkReal32);

				//Allocate minimum
				m_pCache = (AkReal32 *)LCAlloc(AK_WIIU_ROUNDUP_LCALLOC_SIZE(AkMax(uSizePart1, m_uLastPart)));
				if (!m_pCache)
					return false;

				//Load all blocks of the first part
				//TODO TODO: When Nintendo finally resolve their DMA vs code execution concurrency problem, we will need to split and pipeline the blocks loading
				m_uCachedBlocks = uSizePart1 / CACHE_BLOCK_SIZE;
				DCFlushRange((void*)pAlignedData, uSizePart1);				
				LCLoadDMABlocks(m_pCache, (void*)pAlignedData, m_uCachedBlocks);
			}

			//Setup the pfDelay ptr at the right place RELATIVE TO CACHED memory.  
			//That means pfDelay[0] is in all likelihood OUTSIDE the cached memory.
			//The rest of the class will then behave normally.
			AkReal32* pAlignedAreaOfInterest = m_pCache + uPrologue/sizeof(AkReal32);  //This is the part that needs to be aligned.
			pfDelay = pAlignedAreaOfInterest - uCurOffset;
			m_uCacheWriteOffset = uCurOffset;
		}

		LCWaitDMAQueue(0); //Just in case, LCLoadDMABlocks is blocking for now.
		
		return true;
	}
#endif
}