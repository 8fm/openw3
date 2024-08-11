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

#include "stdafx.h" 
#include "AkSettings.h"
#include "AkMath.h"
#include <math.h>

// When the filter is in interpolating mode:
// Interpolation of the filter parameter is performed every LPFPARAMUPDATEPERIOD samples
// Every interpolation of the filter parameter requires to recompute filter coefficients

// When filter is bypassed:
// Clear filter state.

// Linear or non-linear mapping function of the LPF parameter to cutoff freq 
#define MAPPINGMODENONLINEAR

// Mapping function LPF -> CutFreq defines
#define SRCLOPASSMINFREQ 50.f		// Total frequency range
#define SRCLOPASSMAXFREQ 20000.f
#define LOWESTPARAMVAL 0.f			// LPF Parameter in [0.f,100.f] range
#define HIGHESTPARAMVAL 100.f	

// Non-linear mapping
#define BETAPARAM 1.09f							// Exponential base
#define PARAMTRANSPOINT 30.f					// Linear->Exp transition point (param)
#define RANGETRANSPOINT 7000.f					// Linear->Exp transition point (cutoff freq)

// Precomputed values
// RANGETRANSPOINT /  BETAPARAM^(HIGHESTPARAMVAL-PARAMTRANSPOINT)
#define NONLINSCALE 16.79744331443938f

// (SRCLOPASSMAXFREQ - RANGETRANSPOINT)
#define LINRANGEEND ( SRCLOPASSMAXFREQ - RANGETRANSPOINT )

#define FEEDBACKMINFREQ (1.f)		
#define FEEDBACKMAXFREQ (AK_FEEDBACK_SAMPLE_RATE * 0.5f)	// Max valid feedback frequency value

// Fill array of 4 LPF coefficients following Butterworth formula's (only if the filter return value is false), 

AkForceInline AkReal32 FastPowOfBeta(AkReal32 x)
{
	// Below -37 we get denormals. Below -38, x * SCALE is in the range of BIAS, resulting in a valid int and a NAN.
	AKASSERT( x > -37 );

	// Fast exp(x):
	static const AkReal32 LOG = 8.043231727f; //Log in base 1.09 of 2.
	static const AkReal32 SCALE = (AkUInt32)(1 << 23)/LOG;	// 2^23 / log_1.09(2)
	static const AkReal32 BIAS = (AkUInt32)(1 << 23) * 127.f;	// 127 * 2^23

	union IntOrFloat
	{
		AkReal32 f;
		AkUInt32 u;
	};

	// Coarse eval: push the input into the exponent part of the IEEE float.
	IntOrFloat iof;
	iof.u = (AkUInt32)( x * SCALE + BIAS );

	// Read back into floating point, the input IS exponentiated: iof has an exponent which 
	// corresponds to 2^(floor(x)), and the fractional part of x spills into the mantissa
	// and can be viewed as a linear interpolation between neighboring integer exponents.

	// At this point, the maximum error is ~6%. We keep the exponent aside,
	// and evaluate the mantissa against a 2nd order polynomial fitting of 2^(x-1) in the range [1,2[.
	// The resulting error is ~0.2%, at least as good as typical approximations of the 
	// exponential function implemented with intrinsics.

	IntOrFloat iofexp;
	iofexp.u = iof.u & 0xFF800000;	// Exponent only.
	iof.u &= 0x007FFFFF;			// Mantissa only.
	iof.u += (127 << 23);

	AkReal32 fInterp = 0.653043474544611f + iof.f * (2.08057721186389e-2f + iof.f * 3.25189772597707e-1f);

	AkReal32 fExp = iofexp.f * fInterp;

	return fExp;
}

AkReal32 EvalLPFCutoff( AkReal32 in_fCurrentLPFPar, bool in_bIsForFeedbackPipeline )
{
	AkReal32 fCutFreq;

#ifdef AK_MOTION
	// Non-linear mapping function with extended frequency range
	if ( !in_bIsForFeedbackPipeline )
	{
#endif		
		if ( in_fCurrentLPFPar < PARAMTRANSPOINT )
		{
			// Linear mapping function
			fCutFreq = ((LINRANGEEND/PARAMTRANSPOINT)*(PARAMTRANSPOINT-in_fCurrentLPFPar) + RANGETRANSPOINT);
		}
		else
		{
			// Exponential mapping function
			// powf( BETAPARAM, HIGHESTPARAMVAL-in_fCurrentLPFPar );
			fCutFreq = NONLINSCALE * FastPowOfBeta( HIGHESTPARAMVAL-in_fCurrentLPFPar );
		}
#ifdef AK_MOTION		
	}
	else
	{
		// Linear mapping function
		fCutFreq = FEEDBACKMAXFREQ - in_fCurrentLPFPar * ((FEEDBACKMAXFREQ - FEEDBACKMINFREQ) / HIGHESTPARAMVAL);
	}
#else
	AKASSERT(!in_bIsForFeedbackPipeline);
#endif 
	return fCutFreq;
}
