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

#ifndef _AK_MULTIBANDEQ_H
#define _AK_MULTIBANDEQ_H

#ifndef __SPU__
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#endif

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "BiquadFilter.h"
#include <AK/Tools/Common/AkAssert.h>
#include "SPUInline.h"

namespace DSP
{
	class CAkMultiBandEQ
	{
	public:

#ifndef __SPU__

		CAkMultiBandEQ();
		AKRESULT Init( AK::IAkPluginMemAlloc * in_pAllocator, AkUInt16 in_uNumChannels, AkUInt16 in_uNumBands );
		void Term( AK::IAkPluginMemAlloc * in_pAllocator );
		void Reset();

		// Compute filter coefficients for a given band
		void SetCoefficients( 
			AkUInt32 in_uBand,
			AkUInt32 in_uSampleRate, 
			BiquadFilterMono::FilterType in_eCurve, 			
			AkReal32 in_fFreq, 
			AkReal32 in_fGain = 0.f, 
			AkReal32 in_fQ = 1.f );

		// Bypass/unbypass a given band
		void SetBandActive( AkUInt32 in_uBand, bool in_bActive );
#endif

#ifndef AK_PS3
		// Process all channels
		void ProcessBuffer(	AkAudioBuffer * io_pBuffer );
#endif

#ifdef __PPU__
		AkUInt32 GetFilters( DSP::BiquadFilterMono *& out_pFilters )
		{
			out_pFilters = m_pFilters;
			return AK_ALIGN_SIZE_FOR_DMA(m_uNumFilters*sizeof(DSP::BiquadFilterMono));
		}
#endif


#ifdef __SPU__
		// Process all channels, position independent code takes in local storage array of filter objects
		SPUInline void ProcessBuffer(			
				AkAudioBuffer * io_pBuffer, 
				DSP::BiquadFilterMono * pFilter );
#endif

	private:

		// Helper to process all channels
		SPUInline  void ProcessBufferInternal(	
				DSP::BiquadFilterMono * pFilter,
				AkAudioBuffer * io_pBuffer );

	private:
		
		DSP::BiquadFilterMono *	m_pFilters;
		AkUInt32 m_uNumFilters;
		AkUInt32 m_uEnabledBandMask;
		AkUInt16 m_uNumBands;
		AkUInt16 m_uNumChannels;
	};
} // namespace DSP
#endif // _AK_MULTIBANDEQ_H
