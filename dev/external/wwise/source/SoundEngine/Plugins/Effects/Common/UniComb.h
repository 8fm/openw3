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
//
// ------------------------------------------------------------------------
// Unicomb - uses an interpolated delay with a feedback and feedfwd element.
// ------------------------------------------------------------------------
//
//                                          |--------------|
//								|-----------| x fFfwdGain  |-----|
//								|			|--------------|     |
//		|-----|   x[n]   |-----------|                        |-----|  y[n]
// ---->|  +  |----------|  Z[-D-d]  |------------------------|  +  |------>
//		|-----|  		 |-----------|						  |-----|
//		   |				    | 				    				          
//		   |                    |
//         |              |--------------|
//	       |--------------| x fFBackGain |
//		 				  |--------------|
//
//
//
#ifndef _AKUNICOMB_H_
#define _AKUNICOMB_H_

#ifndef __SPU__
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#else
#include <AK/Plugin/PluginServices/PS3/SPUServices.h>
#endif

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Tools/Common/AkAssert.h>
#include "SPUInline.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
// This needs to be the maximum buffer size of all supported platforms (but not more in order to allow local better storage optimizations on PS3)
// Using this rather than the actual platform MaxFrames allows more consistency between platforms 
#define MAX_SUPPORTED_BUFFER_MAXFRAMES (1024)

namespace DSP
{

	class UniComb
	{
	public:
#ifndef __SPU__

		UniComb() : 
					m_uDelayLength( 0 ),
					m_uMaxModWidth( 0 ),
					m_uAllocatedDelayLength( 0 ),
					m_pfDelay( 0 ),
					m_uWritePos( 0 ),
					m_fPrevFeedbackGain( 0.f ),
					m_fPrevFeedforwardGain( 0.f ),
					m_fPrevDryGain( 0.f ),
					m_fFeedbackGain( 0.f ),
					m_fFeedforwardGain( 0.f ),
					m_fDryGain( 0.f )
		{}

		AKRESULT Init(	AK::IAkPluginMemAlloc * in_pAllocator, 
						AkUInt32 in_uDelayLength,
						AkUInt32 in_uMaxBufferLength,
						AkReal32 in_fFeedbackGain,	  
						AkReal32 in_fFeedforwardGain, 
						AkReal32 in_fDryGain,
						AkReal32 in_fMaxModDepth );

		void Term( AK::IAkPluginMemAlloc * in_pAllocator );

		void Reset( );

		void SetParams(		AkReal32 in_fFeedbackGain,
							AkReal32 in_fFeedforwardGain,
							AkReal32 in_fDryGain,
							AkReal32 in_fMaxModDepth );

#endif // #ifndef __SPU__

#ifndef AK_PS3
		// Process delay buffer in place.
		void ProcessBuffer( 
			AkReal32 * io_pfBuffer, 
			AkUInt32 in_uNumFrames, 
			AkReal32 * in_pfLFOBuf  ); 
#endif

#ifdef __SPU__
	// Process delay buffer in place. Can call PIC optimized routines when possible.
	void ProcessBuffer(	
		AkReal32 * io_pfBuffer, 
		AkUInt32 in_uNumFrames, 
		AkReal32 * in_pfLFOBuf,
		AkReal32 * in_pfScratchMem,
		AkUInt32 in_uDMATag );
#endif

#ifdef AK_PS3
		SPUInline bool CanUseLocalStorageOptimAlgo( )
		{
			// Determine whether read pointer can reach location written previously in the same buffer
			if ( (m_uDelayLength - m_uMaxModWidth) > MAX_SUPPORTED_BUFFER_MAXFRAMES )
				return true;
			else
				return false;
		}

		SPUInline AkUInt32 GetScratchSize()
		{
			if ( CanUseLocalStorageOptimAlgo() )
			{
				// Use local storage size optimized algorithm using discrete region for read and write location (which do not overlap)
				// Worst case scenario: Knowing depth, compute maximum positive/negative excursion around nominal position
				// Allocate temporary space to stream write location (in_uMaxFrames)
				// Nominal read pointer will advance (in_uMaxFrames). Use maximum negative excursion before first nominal location
				// and maximum positive excursion past the nominal position as delimiters for possible modulated read position location.
				AkUInt32 uReadMemSize = AK_ALIGN_SIZE_FOR_DMA( (MAX_SUPPORTED_BUFFER_MAXFRAMES+2*m_uMaxModWidth)*sizeof(AkReal32) );
				AkUInt32 uWriteMemSize = AK_ALIGN_SIZE_FOR_DMA( MAX_SUPPORTED_BUFFER_MAXFRAMES * sizeof(AkReal32) );
				return uReadMemSize + uWriteMemSize;
			}
			else
			{
				// Cannot use local storage size optimization, use whole delay line
				return AK_ALIGN_SIZE_FOR_DMA( m_uAllocatedDelayLength * sizeof(AkReal32) );
			}
		}
#endif // #ifdef AK_PS3


	protected:

	// Process in-place without LFO modulation. 
	void ProcessBufferNoLFO(	
		AkReal32 * io_pfBuffer, 
		AkUInt32 in_uNumFrames, 
		AkReal32 * io_pfDelay // Scratch mem on SPU
#ifdef __SPU__
		, AkUInt32 in_uDMATag
#endif
		);

#ifdef __SPU__
	// Process in-place without LFO modulation.
	// This routine is optimized for large delays and assumes that read regions and writing regions do not overlap. 
	void ProcessBufferNoLFO_PIC(	
		AkReal32 * io_pfBuffer, 
		AkUInt32 in_uNumFrames,
		AkReal32 * in_pfScratchMem,
		AkUInt32 in_uDMATag );
#endif

	// Process modulated delay buffer in place. 
	void ProcessBufferLFO(	
		AkReal32 * io_pfBuffer, 
		AkUInt32 in_uNumFrames, 
		AkReal32 * AK_RESTRICT in_pfLFOBuf,
		AkReal32 * io_pfDelay // Scratch mem on SPU
#ifdef __SPU__
		, AkUInt32 in_uDMATag
#endif
									);

#ifdef __SPU__
	// Process modulated delay buffer in place. 
	void ProcessBufferLFO_PIC(	
		AkReal32 * io_pfBuffer, 
		AkUInt32 in_uNumFrames, 
		AkReal32 * AK_RESTRICT in_pfLFOBuf, 
		AkReal32 * in_pfScratchMem, 
		AkUInt32 in_uDMATag );

	SPUInline void GetFullDelayMem( AkUInt8 * out_pDelayMem, AkUInt32 in_uDMATag )
	{
		AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( m_uAllocatedDelayLength * sizeof(AkReal32) );
		AkUInt32 uScheduledDMASize = 0;
		while ( uScheduledDMASize < uDMASize )
		{
			AkUInt32 uTransferSize = AkMin(uDMASize-uScheduledDMASize,16*1024);
			AkDmaGet( "UniComb::Delay",	out_pDelayMem+uScheduledDMASize, (std::uint64_t)m_pfDelay+uScheduledDMASize, uTransferSize, in_uDMATag, 0 , 0 );
			uScheduledDMASize += uTransferSize;
		}
		AkDmaWait(1<<in_uDMATag);
	}

	SPUInline void PutFullDelayMem( AkUInt8 * in_pDelayMem, AkUInt32 in_uDMATag )
	{
		AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( m_uAllocatedDelayLength * sizeof(AkReal32) );
		AkUInt32 uScheduledDMASize = 0;
		while ( uScheduledDMASize < uDMASize )
		{
			AkUInt32 uTransferSize = AkMin(uDMASize-uScheduledDMASize,16*1024);
			AkDmaPut( "UniComb::Delay",in_pDelayMem+uScheduledDMASize, (std::uint64_t)m_pfDelay+uScheduledDMASize, uTransferSize, in_uDMATag, 0 , 0 );
			uScheduledDMASize += uTransferSize;
		}
		AkDmaWait(1<<in_uDMATag);
	}

	SPUInline void GetRWDelayMem( 
		AkReal32 * in_pfScratchMem, 
		AkUInt32 in_uDMATag, 
		AkReal32 *& out_pfReadDelayMem, 
		AkReal32 *& out_pfWriteDelayMem,
		AkUInt32 & out_uReadPosOffset,
		AkUInt32 & out_uWritePosOffset );

	SPUInline void PutRWDelayMem( 
		AkReal32 * in_pfScratchMem, 
		AkUInt32 in_uDMATag );

#endif // #ifdef __SPU__

		SPUInline void InterpolationRampsDone()
		{
			m_fPrevFeedbackGain    =	m_fFeedbackGain;
			m_fPrevFeedforwardGain =	m_fFeedforwardGain;
			m_fPrevDryGain         =	m_fDryGain;
		}

	protected:

		AkUInt32 m_uDelayLength;			// InterpolatedDelayLine line nominal length
		AkUInt32 m_uMaxModWidth;			// Maximum expected variation of the delay length
		AkUInt32 m_uAllocatedDelayLength;	// InterpolatedDelayLine line total allocated length
		AkReal32 *m_pfDelay;				// InterpolatedDelayLine line start position

		AkUInt32 m_uWritePos;				// Current write position in the delay line.

		AkReal32 m_fPrevFeedbackGain;
		AkReal32 m_fPrevFeedforwardGain;
		AkReal32 m_fPrevDryGain;

		AkReal32 m_fFeedbackGain;
		AkReal32 m_fFeedforwardGain;
		AkReal32 m_fDryGain;

	};

} // namespace DSP

#endif // _AKUNICOMB_H_
