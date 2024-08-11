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

#include "OnePoleZeroHPFilter.h"
#include "AkDSPUtils.h"

namespace DSP
{
	void OnePoleZeroHPFilter::ProcessBuffer( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames )
	{
		AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pfBuffer;
		const AkReal32 * const fEnd = pfBuf + in_uNumFrames;
		const AkReal32 fA1 = m_fA1;
		AkReal32 fFFwd1 = m_fFFwd1;
		AkReal32 fFbk1 = m_fFbk1;

		while ( pfBuf < fEnd )
		{
			AkReal32 fIn = *pfBuf;	

			AkReal32 fFbk = fA1 * fFbk1;
			AkReal32 fOut = fIn - fFFwd1;
			fFFwd1 = fIn;	// xn1 = xn for DC
			fOut += fFbk;	
			fFbk1 = fOut;	// yn1 = yn for DC		

			*pfBuf++ = fOut;
		}

		RemoveDenormal( fFbk1 );
		m_fFFwd1 = fFFwd1;
		m_fFbk1 = fFbk1;
	}

	//Gain is applied prior filtering.
	void OnePoleZeroHPFilter::ProcessBufferWithGain( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames, AkReal32 in_fGain )
	{
		AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pfBuffer;
		const AkReal32 * const fEnd = pfBuf + in_uNumFrames;
		const AkReal32 fA1 = m_fA1;
		AkReal32 fFFwd1 = m_fFFwd1;
		AkReal32 fFbk1 = m_fFbk1;

		while ( pfBuf < fEnd )
		{
			AkReal32 fIn = *pfBuf * in_fGain;	

			AkReal32 fFbk = fA1 * fFbk1;
			AkReal32 fOut = fIn - fFFwd1;
			fFFwd1 = fIn;	// xn1 = xn for DC
			fOut += fFbk;	
			fFbk1 = fOut;	// yn1 = yn for DC		

			*pfBuf++ = fOut;	
		}

		RemoveDenormal( fFbk1 );
		m_fFFwd1 = fFFwd1;
		m_fFbk1 = fFbk1;
	}

} // namespace DSP
