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

#ifndef _AKTIMEWINDOW_H_
#define _AKTIMEWINDOW_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#include <stdlib.h>
#include "SPUInline.h"

namespace DSP
{
	class CAkTimeWindow
	{
	public:
		enum WindowType
		{
			WINDOWTYPE_RECTANGULAR = 0,
			WINDOWTYPE_HAMMING,
			WINDOWTYPE_HANN,
			WINDOWTYPE_BLACKMAN
		};

#ifndef __SPU__

		CAkTimeWindow() : m_pfWindowData( NULL ), m_uWindowSize(0), m_fCummulativeSum(0.f) {}

		AKRESULT Init(	
			AK::IAkPluginMemAlloc *	in_pAllocator,
			AkUInt32				in_uWindowSize,
			WindowType				in_eWindowType,
			bool					in_bWeighthedOLAWin = false,
			bool					in_bZeroPhase = false );

		void Term( AK::IAkPluginMemAlloc * in_pAllocator );

		void Apply( AkReal32 * io_pfBuf, AkUInt32 in_uNumFrames, AkReal32 in_fWinGain = 1.f );
#endif

		SPUInline AkReal32 * Get( AkUInt32 * out_puDataSize = NULL )
		{
			if ( out_puDataSize )
				*out_puDataSize = AK_ALIGN_SIZE_FOR_DMA(m_uWindowSize/2*sizeof(AkReal32));
			return m_pfWindowData;
		}

		SPUInline AkReal32 GetCummulativeSum()
		{
			return m_fCummulativeSum;
		}

#ifndef __SPU__
	protected: 

		void RectangularWindow( bool in_bWeighthedOLAWin, bool in_bZeroPhase );
		void HammingWindow( bool in_bWeighthedOLAWin, bool in_bZeroPhase );
		void HannWindow( bool in_bWeighthedOLAWin, bool in_bZeroPhase );
		void BlackmanWindow( bool in_bWeighthedOLAWin, bool in_bZeroPhase );
#endif
		// May be called directly on SPU with local storage window pointer, otherwise use as internal processing handler
		SPUInline void Apply( 
			AkReal32 * io_pfBuf, 
			AkUInt32 in_uNumFrames, 
			AkReal32 * in_pfWindow );

		SPUInline void Apply( 
			AkReal32 * io_pfBuf, 
			AkUInt32 in_uNumFrames, 
			AkReal32 in_fWinGain,
			AkReal32 * in_pfWindow
			);
	
	protected:
	
		AkReal32 *	m_pfWindowData;
		AkUInt32	m_uWindowSize;
		AkReal32	m_fCummulativeSum;
	
	};

} // namespace DSP


#endif // _AKTIMEWINDOW_H_
