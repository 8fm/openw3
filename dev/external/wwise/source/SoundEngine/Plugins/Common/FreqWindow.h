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

#ifndef _AKFREQWINDOW_H_
#define _AKFREQWINDOW_H_

#ifndef __SPU__
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#endif

#include <AK/SoundEngine/Common/AkTypes.h>
#include "ak_fftr.h"
#include "SPUInline.h"

namespace DSP
{

typedef AkReal32 /*AkReal64*/ PhaseProcessingType; 
	
	struct AkPolar
	{
		AkReal32 fMag;
		AkReal32 fPhase;
	};

	namespace BUTTERFLYSET_NAMESPACE
	{

	class CAkFreqWindow
	{
	public:

#ifndef __SPU__
		CAkFreqWindow() : m_pfFreqData( NULL ), m_uFFTSize(0), m_bReady(false), m_bPolar(false) {}

		AKRESULT Alloc(	
			AK::IAkPluginMemAlloc *	in_pAllocator,
			AkUInt32 in_uFFTSize );

		void Free( AK::IAkPluginMemAlloc * in_pAllocator );
#ifndef __PPU__
		// Non PIC interfaces
		void Compute( 
			AkReal32 * in_pfTimeDomainWindow, 
			AkUInt32 in_uNumFrames, 
			ak_fftr_state * in_pFFTState );

		void ConvertToTimeDomain( 
			AkReal32 * out_pfTimeDomainBuffer, 
			AkUInt32 in_uNumFrames, 
			ak_fftr_state * in_pIFFTState );

		void CartToPol();
		void PolToCart();
		
		void ComputeVocoderSpectrum( 
			AkPolar * in_pPreviousFrame, 
			AkPolar * in_pNextFrame,
			PhaseProcessingType * io_pPreviousSynthPhases,
			AkUInt32 in_uHopSize,
			AkReal32 in_fInterpLoc,
			bool in_bInitPhases );
#endif // #ifndef __PPU__

#endif

		SPUInline ak_fft_cpx * Get( AkUInt32 * out_puDataSize = NULL )
		{
			if ( out_puDataSize )
				*out_puDataSize = AK_ALIGN_SIZE_FOR_DMA((m_uFFTSize/2+1)*sizeof(ak_fft_cpx));
			return m_pfFreqData;
		}

		SPUInline void SetReady( bool in_bReady )
		{
			m_bReady = in_bReady;
		}

		SPUInline bool IsReady()
		{
			return m_bReady;
		}

#ifndef __SPU__
	protected: 
#endif
#ifndef __PPU__
		// PIC interfaces, also used by non-PIC interfaces as low-level handlers
		// May be called directly on SPU with local storage window pointer, otherwise use as internal processing handler
		SPUInline void Compute( 
			AkReal32 * in_pfTimeDomainWindow, 
			AkUInt32 in_uNumFrames, 
			ak_fftr_state * in_pFFTState, 
			ak_fft_cpx * io_pfFreqData );

		SPUInline void ConvertToTimeDomain( 
			AkReal32 * out_pfTimeDomainBuffer, 
			AkUInt32 in_uNumFrames, 
			ak_fftr_state * in_pIFFTState,
			ak_fft_cpx * io_pfFreqData );

		SPUInline void CartToPol( ak_fft_cpx * io_pfFreqData );
		SPUInline void PolToCart( AkPolar * io_pfFreqData );

		SPUInline void ComputeVocoderSpectrum( 
			AkPolar * in_pPreviousFrame, 
			AkPolar * in_pNextFrame,
			PhaseProcessingType * io_pPreviousSynthPhases,
			AkUInt32 in_uHopSize,
			AkReal32 in_fInterpLoc,
			bool in_bInitPhases,
			AkPolar * io_pfFreqData );
#endif	
	protected:
	
		ak_fft_cpx *	m_pfFreqData;
		AkUInt32		m_uFFTSize;
		bool			m_bReady;
		bool			m_bPolar;
	};

	} // BUTTERFLYSET_NAMESPACE

} // namespace DSP


#endif // _AKTIMEWINDOW_H_
