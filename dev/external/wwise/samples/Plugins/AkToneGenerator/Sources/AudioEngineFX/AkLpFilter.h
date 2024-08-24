//////////////////////////////////////////////////////////////////////
//
// AkLpFilter.h
//
// AudioKinetic 2nd order IIR Butterworth low pass filter class
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include <AK/SoundEngine/Common/AkTypes.h>

#ifndef _AKLPFILTER_H_
#define _AKLPFILTER_H_

class CAkLpFilter
{
public:
    // Constructor/destructor.
	CAkLpFilter();	 
	~CAkLpFilter();

    void SetCoefs( AkReal32 in_fCutFreq, AkReal32 in_fSampRate );
	inline AkReal32 ProcessSample( AkReal32 in_fInSample )
	{
		AkReal32 fCurOut = m_fB0 * in_fInSample + m_fB1 * m_fIn1 + m_fB2 * m_fIn2;
		fCurOut -= m_fA1 * m_fOut1 + m_fA2 * m_fOut2;
		m_fIn2 = m_fIn1;
		m_fIn1 = in_fInSample;
		m_fOut2 = m_fOut1;
		m_fOut1 = fCurOut;
		return fCurOut;
	}
	void Reset( );

private:
    // Filter coefficients
	AkReal32 m_fB0;
	AkReal32 m_fB1;
	AkReal32 m_fB2;
	AkReal32 m_fA1;
	AkReal32 m_fA2;

	// Filter memory
	AkReal32 m_fIn1;
	AkReal32 m_fIn2;
	AkReal32 m_fOut1;
	AkReal32 m_fOut2;
};

#endif



