//////////////////////////////////////////////////////////////////////
//
// AkLpFilter.h
//
// AudioKinetic 2nd order IIR Butterworth low pass filter class
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkLpFilter.h"
#include <math.h>

#define PI					(3.1415926535897932384626433832795f)
#define ROOTTWO				(1.4142135623730950488016887242097f)

CAkLpFilter::CAkLpFilter( )
{
	// Initialize filter coefficients
	m_fB0 = 0.f;
	m_fB1 = 0.f;
	m_fB2 = 0.f;
	m_fA1 = 0.f;
	m_fA2 = 0.f;

	Reset();
}

CAkLpFilter::~CAkLpFilter( )
{

}

void CAkLpFilter::SetCoefs( AkReal32 in_fCutFreq, AkReal32 in_fSampRate )
{

	// Butterworth Low-pass filter computations (pre-computed using Bilinear transform)
	AkReal32 PiFcOSr = PI * in_fCutFreq / in_fSampRate;
	AkReal32 fTanPiFcSr = tan(PiFcOSr);
	AkReal32 fIntVal = 1.f / fTanPiFcSr;
	AkReal32 fRootTwoxIntVal = ROOTTWO * fIntVal;
	AkReal32 fSqIntVal = fIntVal * fIntVal;

	m_fB0 = (1.f / ( 1.f + fRootTwoxIntVal + fSqIntVal));
	m_fB1 = m_fB0 + m_fB0;
	m_fB2 = m_fB0;
	m_fA1 = 2.f * ( 1.f - fSqIntVal) * m_fB0;
	m_fA2 = ( 1.f - fRootTwoxIntVal + fSqIntVal) * m_fB0;
}

void CAkLpFilter::Reset( )
{
	m_fIn1 = 0.f;
	m_fIn2 = 0.f;
	m_fOut1 = 0.f;
	m_fOut2 = 0.f;
}
