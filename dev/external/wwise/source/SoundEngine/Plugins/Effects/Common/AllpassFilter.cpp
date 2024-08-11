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

// Allpass filter with feedforward and feedback memories interleaved
// y[n] = g * x[n] + x[n - D] - g * y[n - D]
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


#include "AllPassFilter.h"
#ifdef __SPU__
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
	AKRESULT AllpassFilter::Init( AK::IAkPluginMemAlloc * in_pAllocator, AkUInt32 in_uDelayLineLength, AkReal32 in_fG )
	{
		//at least one sample delay
		if ( in_uDelayLineLength < 1 )
			in_uDelayLineLength = 1;
		uDelayLineLength = in_uDelayLineLength; 
		pfDelay = (AkReal32*)AK_PLUGIN_ALLOC( in_pAllocator, AK_ALIGN_SIZE_FOR_DMA( 2 * sizeof(AkReal32) * uDelayLineLength ) );
		if ( pfDelay == NULL )
			return AK_InsufficientMemory;

		uCurOffset = 0;
		SetGain( in_fG );
		return AK_Success;
	}

	void AllpassFilter::Term( AK::IAkPluginMemAlloc * in_pAllocator )
	{
		if ( pfDelay )
		{
			AK_PLUGIN_FREE( in_pAllocator, pfDelay );
			pfDelay = NULL;
		}
		uDelayLineLength = 0;
	}

	void AllpassFilter::Reset( )
	{
		if ( pfDelay )
		{
			memset( pfDelay, 0, 2 * sizeof(AkReal32) * uDelayLineLength );
		}
	}

	// Process delay buffer in place when access to direct memory is ok (not PIC)
	void AllpassFilter::ProcessBuffer( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames )
	{
		AkReal32 * AK_RESTRICT pfDelayPtr = (AkReal32 * AK_RESTRICT) &pfDelay[uCurOffset*2];
		AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pfBuffer;
		AkUInt32 uFramesBeforeWrap = uDelayLineLength - uCurOffset;
		if ( uFramesBeforeWrap > in_uNumFrames )
		{
			// Not wrapping this time
			AkUInt32 i = in_uNumFrames;
#if defined(AK_XBOX360)
			static const AkReal32 fAntiDenormal = 1e-18f;
			while( i-- )
			{
				AkReal32 fXn = *pfBuf;
				AkReal32 fFwdMem = *pfDelayPtr;		
				*pfDelayPtr++ = fXn;
				AkReal32 fFbkMem = *pfDelayPtr;
				AkReal32 fYn = fG * ( fXn - fFbkMem) + fFwdMem;
				fYn += fAntiDenormal;
				fYn -= fAntiDenormal;
				*pfDelayPtr++ = fYn;
				*pfBuf++ = fYn;
			} 
#else
			while ( i-- )
			{
				AkReal32 fXn = *pfBuf;
				AkReal32 fFwdMem = *pfDelayPtr;		
				*pfDelayPtr++ = fXn;
				AkReal32 fFbkMem = *pfDelayPtr;
				AkReal32 fYn = fG * ( fXn - fFbkMem) + fFwdMem;
				*pfDelayPtr++ = fYn;
				*pfBuf++ = fYn;
			} 
#endif
			uCurOffset += in_uNumFrames;
			AKASSERT( uCurOffset <= uDelayLineLength );
		}
		else
		{
			// Minimum number of wraps
			AkUInt32 uFramesRemainingToProcess = in_uNumFrames;
			do
			{
				AkUInt32 uFramesToProcess = AkMin(uFramesRemainingToProcess, uFramesBeforeWrap);
#if defined(AK_XBOX360)
				static const AkReal32 fAntiDenormal = 1e-18f;
				for ( AkUInt32 i = 0; i < uFramesToProcess; ++i )
				{
					AkReal32 fXn = *pfBuf;
					AkReal32 fFwdMem = *pfDelayPtr;		
					*pfDelayPtr++ = fXn;
					AkReal32 fFbkMem = *pfDelayPtr;
					AkReal32 fYn = fG * ( fXn - fFbkMem) + fFwdMem;
					fYn += fAntiDenormal;
					fYn -= fAntiDenormal;
					*pfDelayPtr++ = fYn;
					*pfBuf++ = fYn;
				}
#else
				for ( AkUInt32 i = 0; i < uFramesToProcess; ++i )
				{
					AkReal32 fXn = *pfBuf;
					AkReal32 fFwdMem = *pfDelayPtr;		
					*pfDelayPtr++ = fXn;
					AkReal32 fFbkMem = *pfDelayPtr;
					AkReal32 fYn = fG * ( fXn - fFbkMem) + fFwdMem;
					*pfDelayPtr++ = fYn;
					*pfBuf++ = fYn;
				}
#endif
				uCurOffset += uFramesToProcess;
				AKASSERT( uCurOffset <= uDelayLineLength );

				if ( uCurOffset == uDelayLineLength )
				{
					pfDelayPtr = (AkReal32 * AK_RESTRICT) pfDelay;
					uCurOffset = 0;
				}

				uFramesRemainingToProcess -= uFramesToProcess;
				uFramesBeforeWrap = uDelayLineLength - uCurOffset;
			}
			while ( uFramesRemainingToProcess );
		}
	}

	// Process delay buffer out-of-place when access to direct memory is ok (not PIC)
	void AllpassFilter::ProcessBuffer(	AkReal32 * in_pfInBuffer, 
		AkReal32 * out_pfOutBuffer, 
		AkUInt32 in_uNumFrames )
	{
		AkReal32 * AK_RESTRICT pfDelayPtr = (AkReal32 * AK_RESTRICT) &pfDelay[uCurOffset*2];
		AkReal32 * AK_RESTRICT pfBufIn = (AkReal32 * AK_RESTRICT) in_pfInBuffer;
		AkReal32 * AK_RESTRICT pfBufOut = (AkReal32 * AK_RESTRICT) out_pfOutBuffer;
		AkUInt32 uFramesBeforeWrap = uDelayLineLength - uCurOffset;
		if ( uFramesBeforeWrap > in_uNumFrames )
		{
			// Not wrapping this time
			AkUInt32 i = in_uNumFrames;
			while ( i-- )
			{
				AkReal32 fXn = *pfBufIn++;
				AkReal32 fFwdMem = *pfDelayPtr;		
				*pfDelayPtr++ = fXn;
				AkReal32 fFbkMem = *pfDelayPtr;
				AkReal32 fYn = fG * ( fXn - fFbkMem) + fFwdMem;
				*pfDelayPtr++ = fYn;
				*pfBufOut++ = fYn;
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
					AkReal32 fXn = *pfBufIn++;
					AkReal32 fFwdMem = *pfDelayPtr;		
					*pfDelayPtr++ = fXn;
					AkReal32 fFbkMem = *pfDelayPtr;
					AkReal32 fYn = fG * ( fXn - fFbkMem) + fFwdMem;
					*pfDelayPtr++ = fYn;
					*pfBufOut++ = fYn;
				}
				uCurOffset += uFramesToProcess;
				AKASSERT( uCurOffset <= uDelayLineLength );

				if ( uCurOffset == uDelayLineLength )
				{
					pfDelayPtr = (AkReal32 * AK_RESTRICT) pfDelay;
					uCurOffset = 0;
				}

				uFramesRemainingToProcess -= uFramesToProcess;
				uFramesBeforeWrap = uDelayLineLength - uCurOffset;
			}
		}
	}

#ifdef __PPU__
	AkUInt32 AllpassFilter::GetScratchMemorySizeRequired( AkUInt32 in_uNumFrames )
	{
		if ( uDelayLineLength )
		{
			if ( in_uNumFrames <= uDelayLineLength )
				return AK_ALIGN_SIZE_FOR_DMA( uDelayLineLength*sizeof(AkReal32)*2 );
			else
				return (in_uNumFrames+3*4)*sizeof(AkReal32)*2;
		}
		else
		{
			return 0;
		}
	}
#endif // __PPU__

#else // #ifndef __SPU__

	void AllpassFilter::GetDelayMemory(	
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
			if ( uDelayLineLength <= in_uNumFrames )
			{
				// PIC usage for delay line <= uFramesToProcess 
				// -> DMA delay line from its start (address aligned so no offset to handle)
				// -> CurrentPos is the start of first DMA + uCurOffset (no DMA offset necessary)
				// -> Wrap pos is the start of the same DMA buffer (address aligned so no offset to handle)		
				AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( uDelayLineLength*sizeof(AkReal32)*2 );
				AKASSERT( uDMASize <= 16*1024 );
				AkDmaGet(	
					"GetShortDelay",
					io_pfScratchMemLoc,
					(std::uint64_t)pfDelay,
					uDMASize,
					in_uDMATag,0,0);
				AkDmaWait(1<<in_uDMATag); // FTDB
				out_pfCurPos = io_pfScratchMemLoc + uCurOffset*2;
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
				AkReal32 * pfCurDelayPos = pfDelay + uCurOffset*2;
				AkReal32 * pfCurDelayPosDMA = (AkReal32 *) ( (AkUInt32) pfCurDelayPos & ~0xf ); // DMA must be 16-aligned
				AkUInt32 uDelayDMAOffset = (AkUInt32)pfCurDelayPos & 0xF;
				AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( uNumFirstDMAFrames*sizeof(AkReal32)*2 + uDelayDMAOffset );
				AKASSERT( uDMASize <= 16*1024 );
				AkDmaGet(	
					"GetLongDelay1",
					io_pfScratchMemLoc,
					(std::uint64_t)pfCurDelayPosDMA,
					uDMASize,
					in_uDMATag,0,0);
				AkDmaWait(1<<in_uDMATag); // FTDB
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
					uDMASize = AK_ALIGN_SIZE_FOR_DMA( uFramesAfterWrap*sizeof(AkReal32)*2 );
					AKASSERT( uDMASize <= 16*1024 );
					AkDmaGet(	
						"GetLongDelay2",
						io_pfScratchMemLoc,
						(std::uint64_t)pfDelay,
						uDMASize,
						in_uDMATag,0,0);
					AkDmaWait(1<<in_uDMATag); // FTDB
					out_pfWrapPos = io_pfScratchMemLoc;
					io_pfScratchMemLoc += uDMASize/sizeof(AkReal32); 
				}
			}
		}
	}

	void AllpassFilter::PutDelayMemory(	
		AkUInt32 in_uCurOffsetBefore, 
		AkReal32 *& io_pfScratchMemLoc,
		AkUInt32 in_uNumFrames,
		AkUInt32 in_uDMATag )
	{
		if ( uDelayLineLength )
		{
			AKASSERT( ((AkUInt32)io_pfScratchMemLoc & 0xF) == 0 );
			if ( uDelayLineLength <= in_uNumFrames )
			{
				// Single DMA necessary
				AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( uDelayLineLength*sizeof(AkReal32)*2 );
				AKASSERT( uDMASize <= 16*1024 );
				AkDmaPut(	
					"PutShortDelay",
					io_pfScratchMemLoc,
					(std::uint64_t)pfDelay,
					uDMASize,
					in_uDMATag,0,0);
				AkDmaWait(1<<in_uDMATag); // FTDB
				io_pfScratchMemLoc += uDMASize/sizeof(AkReal32); 
			}
			else
			{
				// PIC usage for delay line > uFramesToProcess 
				// -> DMA delay line from current position single DMA sufficient for less than 4096 frames
				// -> CurrentPos is the start of first DMA + Offset because DMA was 16 bytes aligned
				// -> Wrap pos is the second DMA buffer received (address aligned so no offset to handle)
				AkUInt32 uNumFirstDMAFrames = uDelayLineLength-in_uCurOffsetBefore;
				if ( uNumFirstDMAFrames > in_uNumFrames )
					uNumFirstDMAFrames = in_uNumFrames;
				AkReal32 * pfCurDelayPos = pfDelay + in_uCurOffsetBefore*2;
				AkReal32 * pfCurDelayPosDMA = (AkReal32 *) ( (AkUInt32) pfCurDelayPos & ~0xf ); // DMA must be 16-aligned
				AkUInt32 uDelayDMAOffset = (AkUInt32)pfCurDelayPos & 0xF;
				AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( uNumFirstDMAFrames*sizeof(AkReal32)*2 + uDelayDMAOffset );
				AKASSERT( uDMASize <= 16*1024 );
				AkDmaPut(	
					"PutLongDelay1",
					io_pfScratchMemLoc,
					(std::uint64_t)pfCurDelayPosDMA,
					uDMASize,
					in_uDMATag,0,0);
				AkDmaWait(1<<in_uDMATag); // FTDB
				io_pfScratchMemLoc += uDMASize/sizeof(AkReal32); 

				if ( uNumFirstDMAFrames < in_uNumFrames )
				{
					AkUInt32 uFramesAfterWrap = in_uNumFrames - uNumFirstDMAFrames;
					// Need a second (address aligned) DMA to handle wrap
					uDMASize = AK_ALIGN_SIZE_FOR_DMA( uFramesAfterWrap*sizeof(AkReal32)*2 );
					AKASSERT( uDMASize <= 16*1024 );
					AkDmaPut(	
						"PutLongDelay2",
						io_pfScratchMemLoc,
						(std::uint64_t)pfDelay,
						uDMASize,
						in_uDMATag,0,0);
					AkDmaWait(1<<in_uDMATag); // FTDB
					io_pfScratchMemLoc += uDMASize/sizeof(AkReal32); 
				}
			}
		}
	}


	// Supports PIC
	// Process delay buffer in-place, starting from start position untill end position, wraps to 
	// (a possibly discontiguous) memory location given by WrapPos when the end is reached
	void AllpassFilter::ProcessBuffer(	
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
			while ( i-- )
			{
				AkReal32 fXn = *pfBuf;
				AkReal32 fFwdMem = *pfDelayPtr;		
				*pfDelayPtr++ = fXn;
				AkReal32 fFbkMem = *pfDelayPtr;
				AkReal32 fYn = fG * ( fXn - fFbkMem) + fFwdMem;
				*pfDelayPtr++ = fYn;
				*pfBuf++ = fYn;
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
					AkReal32 fXn = *pfBuf;
					AkReal32 fFwdMem = *pfDelayPtr;		
					*pfDelayPtr++ = fXn;
					AkReal32 fFbkMem = *pfDelayPtr;
					AkReal32 fYn = fG * ( fXn - fFbkMem) + fFwdMem;
					*pfDelayPtr++ = fYn;
					*pfBuf++ = fYn;
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
	void AllpassFilter::ProcessBuffer(	
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
			while ( i-- )
			{
				AkReal32 fXn = *pfBufIn++;
				AkReal32 fFwdMem = *pfDelayPtr;		
				*pfDelayPtr++ = fXn;
				AkReal32 fFbkMem = *pfDelayPtr;
				AkReal32 fYn = fG * ( fXn - fFbkMem) + fFwdMem;
				*pfDelayPtr++ = fYn;
				*pfBufOut++ = fYn;
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
					AkReal32 fXn = *pfBufIn++;
					AkReal32 fFwdMem = *pfDelayPtr;		
					*pfDelayPtr++ = fXn;
					AkReal32 fFbkMem = *pfDelayPtr;
					AkReal32 fYn = fG * ( fXn - fFbkMem) + fFwdMem;
					*pfDelayPtr++ = fYn;
					*pfBufOut++ = fYn;
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

}
