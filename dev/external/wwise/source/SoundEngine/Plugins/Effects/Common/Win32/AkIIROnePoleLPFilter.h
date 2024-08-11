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

//////////////////////////////////////////////////////////////////////
//
// AkIIROnePoleLPFilter.h
//
// 1st order IIR low pass filter class
// y[n] = (1-g) * x[n] + g * y[n - 1]
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKIIRONEPOLEFILTER_H_
#define _AKIIRONEPOLEFILTER_H_

#include "AkDSPUtils.h" // RemoveDenormal
#include <AK/Tools/Common/AkAssert.h>

class CAkOnePoleLPFilter
{
public:
	CAkOnePoleLPFilter()
	{
		m_fFwdCoef = 0.f;
		m_fFbkCoef = 0.f;
		m_fLPMem = 0.f;
	}
	~CAkOnePoleLPFilter() { }
	AkForceInline void ProcessBuffer( AkReal32 * AK_RESTRICT io_pfInOut, AkUInt32 in_uFramesToProcess );
	inline void SetCutoffFrequency( AkReal32 in_fCutFreq );
	inline void SetSampleRate( AkUInt32 in_uSampleRate );
	inline void Reset( );
	inline void AvoidDenormal();

private:
	AkUInt32 m_uSampleRate;
	AkReal32 m_fLPMem;				// first order feedback memories
	AkReal32 m_fFwdCoef;			// Feedforward normalization coefficient for LP filter
	AkReal32 m_fFbkCoef;			// Feedback coefficient for LP filter
};

AkForceInline void CAkOnePoleLPFilter::ProcessBuffer( AkReal32 * AK_RESTRICT io_pfInOut, AkUInt32 in_uFramesToProcess )
{
	// Filter memory
	AkReal32 fLPyn1 = m_fLPMem;

	// Coefs
	AkReal32 fLPb0 = m_fFwdCoef;
	AkReal32 fLPa1 = m_fFbkCoef;

	const AkReal32 * const fEnd = io_pfInOut + in_uFramesToProcess;
	while ( io_pfInOut < fEnd )
	{
		AkReal32 fXIn = *io_pfInOut;		
		AkReal32 fFbk = fLPyn1 * fLPa1;
		AkReal32 fOutLPF = fXIn * fLPb0;
		fOutLPF += fFbk;
		*io_pfInOut++ = fOutLPF;	
		fLPyn1 = fOutLPF; 
	}

	// Save filter memories
	m_fLPMem = fLPyn1;
}

inline void CAkOnePoleLPFilter::SetCutoffFrequency( AkReal32 in_fCutFreq )
{
	// Effectively bypass filter if cutoff frequency is >= Nyquist frequency
	if ( in_fCutFreq < (m_uSampleRate / 2.f) )
	{
		/*
		// Coefficient computation
		AkReal32 fNormCutFreq = 2.f * PI * in_fCutoffFrequency / in_fSampleRate;
		AkReal32 fTmp = (2.f - cos(fNormCutFreq));
		AkReal32 fCoef = fTmp - sqrt(fTmp * fTmp - 1.f);
		*/

		// Coefficient approximation
		AkReal32 fTmp = (1.f - 2.f * in_fCutFreq / m_uSampleRate);
		AkReal32 fCoef = fTmp * fTmp;
		m_fFwdCoef = 1.f - fCoef;
		m_fFbkCoef = fCoef;
		RemoveDenormal( m_fFwdCoef );
		RemoveDenormal( m_fFbkCoef );
	}
	else
	{
		// Passthrough filter
		m_fFwdCoef = 1.f;
		m_fFbkCoef = 0.f;
	}
}

inline void CAkOnePoleLPFilter::SetSampleRate( AkUInt32 in_uSampleRate )
{
	m_uSampleRate = in_uSampleRate;
}

inline void CAkOnePoleLPFilter::Reset( )
{
	m_fLPMem = 0.f;
}

inline void CAkOnePoleLPFilter::AvoidDenormal()
{
#ifdef AK_XBOX360
	RemoveDenormal( m_fLPMem );
#endif
}

#endif // _AKIIRONEPOLEFILTER_H_
