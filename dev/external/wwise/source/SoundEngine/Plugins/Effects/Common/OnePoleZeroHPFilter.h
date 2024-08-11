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

// High pass (one moveable pole, one fixed zero at dc) filter
// y[n] = x[n] - x[n-1] + g * y[n - 1]
// Can be used as DC filter for low cutoff frequencies (~20-40Hz)
// To be used on mono signals, create as many instances as there are channels if need be

#ifndef _AKONEPOLEZEROHIGHPASSFILTER_H_
#define _AKONEPOLEZEROHIGHPASSFILTER_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "SPUInline.h"

namespace DSP
{

	class OnePoleZeroHPFilter
	{
	public:
		SPUInline OnePoleZeroHPFilter() 
			: m_fFFwd1( 0.f )
			, m_fFbk1( 0.f )
			, m_fA1( 0.f ){}

		SPUInline void ComputeCoefs( AkReal32 in_fCutFreq, AkUInt32 in_uSampleRate )
		{
			static const AkReal32 fPI = 3.1415926535f;
			m_fA1 = 1.f - (2.f * fPI * in_fCutFreq / in_uSampleRate);
		}

		SPUInline void Reset( )
		{
			m_fFFwd1 = m_fFbk1 = 0.f;
		}

		 SPUInline void ProcessBuffer( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames );

		 SPUInline void ProcessBufferWithGain( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames, AkReal32 in_fGain );

	protected:
		AkReal32 m_fFFwd1;	// first order feedforward
		AkReal32 m_fFbk1;	// first order feedback memories
		AkReal32 m_fA1;		// Feedback coefficient for HP filter
	};

} // namespace DSP

#endif // _AKONEPOLEZEROHIGHPASSFILTER_H_
