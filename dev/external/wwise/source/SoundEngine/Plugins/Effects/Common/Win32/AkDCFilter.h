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
// AkDCFilter.h
//
// DC high pass (one pole filter)
// y[n] = x[n] - x[n-1] + g * y[n - 1]
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKDCFILTER_H_
#define _AKDCFILTER_H_

#include "AkDSPUtils.h" // RemoveDenormal

static const AkReal32 DCFILTERCUTOFFFREQ = 10.f;
static const AkReal32 PI = 3.1415926535f;

class CAkDCFilter
{
public:
	AkForceInline void ProcessBuffer( AkReal32 * AK_RESTRICT io_pfInOut, AkUInt32 in_uFramesToProcess );
	inline void SetCutoffFrequency( AkReal32 in_fCutFreq );
	inline void SetSampleRate( AkUInt32 in_uSampleRate );
	inline void Reset( );
	inline void AvoidDenormal();

private:
	AkUInt32 m_uSampleRate; // Sample rate
	AkReal32 m_fDCFwdMem;	// first order feedforward
	AkReal32 m_fDCFbkMem;	// first order feedback memories
	AkReal32 m_fDCCoef;		// Feedback coefficient for HP filter
};

AkForceInline void CAkDCFilter::ProcessBuffer( AkReal32 * AK_RESTRICT io_pfInOut, AkUInt32 in_uFramesToProcess )
{
	// Filter memory
	AkReal32 fDCxn1 = m_fDCFwdMem;	// xn1 DC
	AkReal32 fDCyn1 = m_fDCFbkMem;	// yn1 DC

	// Coefs
	AkReal32 fDCa1 = m_fDCCoef;	// a1 DC

	const AkReal32 * const fEnd = io_pfInOut + in_uFramesToProcess;
	while ( io_pfInOut < fEnd )
	{
		AkReal32 fXIn = *io_pfInOut;		
		AkReal32 fFbk = fDCa1 * fDCyn1;
		AkReal32 fOut = fXIn - fDCxn1;
		fDCxn1 = fXIn;	// xn1 = xn for DC
		fOut += fFbk;
		*io_pfInOut++ = fOut;	
		fDCyn1 = fOut;	// yn1 = yn for DC		
	}

	// Save filter memories
	m_fDCFwdMem = fDCxn1;	// xn1 DC
	m_fDCFbkMem = fDCyn1;	// yn1 DC
}

inline void CAkDCFilter::SetCutoffFrequency( AkReal32 in_fCutFreq )
{
	m_fDCCoef = 1.f - (2.f * PI * in_fCutFreq / m_uSampleRate);
	Reset();
}

inline void CAkDCFilter::SetSampleRate( AkUInt32 in_uSampleRate )
{
	m_uSampleRate = in_uSampleRate;
}

inline void CAkDCFilter::Reset( )
{
	m_fDCFwdMem = 0.f;
	m_fDCFbkMem = 0.f;
}

inline void CAkDCFilter::AvoidDenormal()
{
#ifdef AK_XBOX360
	RemoveDenormal( m_fDCFbkMem );
#endif
}

#endif // _AKDCFILTER_H_
