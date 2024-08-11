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

// Direct form biquad filter y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
// To be used on mono signals, create as many instances as there are channels if need be

public:
#if !defined(__SPU__) || defined(AKENABLESPUBIQUADCOMPUTECOEFS)

	 void ComputeCoefs(	
		FilterType in_eFilterType, 
		const AkReal32 in_fSampleRate, 
		const AkReal32 in_fFreq, 
		const AkReal32 in_fGain = 0.f,
		const AkReal32 in_fQ = 1.f)
	{
// Maximum shelf slope
#define SHELFSLOPE 1.f 
#define SQRT2 1.41421356237309504880f

		AkReal32 fb0 = 0.f;
		AkReal32 fb1 = 0.f;
		AkReal32 fb2 = 0.f;	// Feed forward coefficients
		AkReal32 fa0 = 0.f;
		AkReal32 fa1 = 0.f;
		AkReal32 fa2 = 0.f;	// Feed back coefficients

		// Note: Q and "bandwidth" linked by:
		// 1/Q = 2*sinh(ln(2)/2*BandWidth*fOmega/sin(fOmega))

		AkReal32 fFrequency = in_fFreq;

		// Frequency must be less or equal to the half of the sample rate
		// 0.9 is there because of bilinear transform behavior at frequencies close to Nyquist
		const AkReal32 fMaxFrequency = in_fSampleRate * 0.5f * 0.9f;
		fFrequency = AkMin(fMaxFrequency, fFrequency);

		if (in_eFilterType == FilterType_LowPass)
			{
				// Butterworth low pass for flat pass band
				AkReal32 PiFcOSr	= PI * fFrequency / in_fSampleRate;
				AkReal32 fTanPiFcSr = tanf(PiFcOSr);
				AkReal32 fIntVal	= 1.0f / fTanPiFcSr;
				AkReal32 fRootTwoxIntVal	= SQRT2 * fIntVal;
				AkReal32 fSqIntVal =	fIntVal	* fIntVal;
				AkReal32 fOnePlusSqIntVal = 1.f + fSqIntVal;
				// Coefficient formulas
				fb0 = (1.0f / ( fRootTwoxIntVal + fOnePlusSqIntVal));
				fb1 = 2.f * fb0;
				fb2 = fb0;
				fa0 = 1.f;
				fa1 = fb1 * ( 1.0f - fSqIntVal);
				fa2 = ( fOnePlusSqIntVal - fRootTwoxIntVal) * fb0;
			}
		else if (in_eFilterType == FilterType_HighPass)
			{
				// Butterworth high pass for flat pass band
				AkReal32 PiFcOSr	= PI * fFrequency / in_fSampleRate;
				AkReal32 fTanPiFcSr = tanf(PiFcOSr);
				AkReal32 fRootTwoxIntVal	= SQRT2 * fTanPiFcSr;
				AkReal32 fSqIntVal =	fTanPiFcSr	* fTanPiFcSr;
				AkReal32 fOnePlusSqIntVal = 1.f + fSqIntVal;
				// Coefficient formulas
				fb0 = (1.0f / ( fRootTwoxIntVal + fOnePlusSqIntVal));
				fb1 = -2.f * fb0;
				fb2 = fb0;
				fa0 = 1.f;
				fa1 = -fb1 * ( fSqIntVal - 1.0f );
				fa2 = ( fOnePlusSqIntVal - fRootTwoxIntVal ) * fb0;
			}
		else if (in_eFilterType == FilterType_BandPass)
			{
				AkReal32 fOmega = 2.f * PI * fFrequency / in_fSampleRate;	// Normalized angular frequency
				AkReal32 fCosOmega = cosf(fOmega);											// Cos omega
				AkReal32 fSinOmega = sinf(fOmega);											// Sin omega
				// 0 dB peak (normalized passband gain)
				// alpha = sin(w0)/(2*Q)
				AkReal32 fAlpha = fSinOmega/(2.f*in_fQ);
				// Coefficient formulas
				fb0 = fAlpha;
				fb1 = 0.f;
				fb2 = -fAlpha;
				fa0 = 1.f + fAlpha;
				fa1 = -2.f*fCosOmega;
				fa2 = 1.f - fAlpha;    
			}
		else if (in_eFilterType == FilterType_Notch)
			{
				AkReal32 fOmega = 2.f * PI * fFrequency / in_fSampleRate;	// Normalized angular frequency
				AkReal32 fCosOmega = cosf(fOmega);											// Cos omega
				AkReal32 fSinOmega = sinf(fOmega);											// Sin omega
				// Normalized passband gain
				AkReal32 fAlpha = fSinOmega/(2.f*in_fQ);
				// Coefficient formulas
				fb0 = 1.f;
				fb1 = -2.f*fCosOmega;
				fb2 = 1.f;
				fa0 = 1.f + fAlpha;
				fa1 = fb1;
				fa2 = 1.f - fAlpha;
			}
		else if (in_eFilterType == FilterType_LowShelf)
			{
#ifdef AKSIMD_FAST_EXP_IMPLEMENTED
				AkReal32 fLinAmp = AKSIMD_POW10_SCALAR(in_fGain*0.025f);
#else
				AkReal32 fLinAmp = AkMath::FastPow10(in_fGain*0.025f);
#endif
				AkReal32 fOmega = 2.f * PI * fFrequency / in_fSampleRate;	// Normalized angular frequency
				AkReal32 fAlpha = sinf(fOmega)/2.f * sqrtf( (fLinAmp + 1.f/fLinAmp)*(1.f/SHELFSLOPE - 1.f) + 2.f );
				AkReal32 fCosOmega = cosf(fOmega);
				AkReal32 fLinAmpPlusOne = fLinAmp+1.f;
				AkReal32 fLinAmpMinusOne = fLinAmp-1.f;
				AkReal32 fTwoSqrtATimesAlpha = 2.f*sqrt(fLinAmp)*fAlpha;
				AkReal32 fLinAmpMinusOneTimesCosOmega = fLinAmpMinusOne*fCosOmega;
				AkReal32 fLinAmpPlusOneTimesCosOmega = fLinAmpPlusOne*fCosOmega;

				fb0 = fLinAmp*( fLinAmpPlusOne - fLinAmpMinusOneTimesCosOmega + fTwoSqrtATimesAlpha );
				fb1 = 2.f*fLinAmp*( fLinAmpMinusOne - fLinAmpPlusOneTimesCosOmega );
				fb2 = fLinAmp*( fLinAmpPlusOne - fLinAmpMinusOneTimesCosOmega - fTwoSqrtATimesAlpha );
				fa0 = fLinAmpPlusOne + fLinAmpMinusOneTimesCosOmega + fTwoSqrtATimesAlpha;
				fa1 = -2.f*( fLinAmpMinusOne + fLinAmpPlusOneTimesCosOmega );
				fa2 = fLinAmpPlusOne + fLinAmpMinusOneTimesCosOmega - fTwoSqrtATimesAlpha;
			}
		else if (in_eFilterType == FilterType_HighShelf)
			{
#ifdef AKSIMD_FAST_EXP_IMPLEMENTED
				AkReal32 fLinAmp = AKSIMD_POW10_SCALAR(in_fGain*0.025f);
#else
				AkReal32 fLinAmp = AkMath::FastPow10(in_fGain*0.025f);
#endif
				AkReal32 fOmega = 2.f * PI * fFrequency / in_fSampleRate;	// Normalized angular frequency
				AkReal32 fAlpha = sinf(fOmega)/2.f * sqrtf( (fLinAmp + 1.f/fLinAmp)*(1.f/SHELFSLOPE - 1.f) + 2.f );
				AkReal32 fCosOmega = cosf(fOmega);
				AkReal32 fLinAmpPlusOne = fLinAmp+1.f;
				AkReal32 fLinAmpMinusOne = fLinAmp-1.f;
				AkReal32 fTwoSqrtATimesAlpha = 2.f*sqrt(fLinAmp)*fAlpha;
				AkReal32 fLinAmpMinusOneTimesCosOmega = fLinAmpMinusOne*fCosOmega;
				AkReal32 fLinAmpPlusOneTimesCosOmega = fLinAmpPlusOne*fCosOmega;

				fb0 = fLinAmp*( fLinAmpPlusOne + fLinAmpMinusOneTimesCosOmega + fTwoSqrtATimesAlpha );
				fb1 = -2.f*fLinAmp*( fLinAmpMinusOne + fLinAmpPlusOneTimesCosOmega );
				fb2 = fLinAmp*( fLinAmpPlusOne + fLinAmpMinusOneTimesCosOmega - fTwoSqrtATimesAlpha );
				fa0 = fLinAmpPlusOne - fLinAmpMinusOneTimesCosOmega + fTwoSqrtATimesAlpha;
				fa1 = 2.f*( fLinAmpMinusOne - fLinAmpPlusOneTimesCosOmega );
				fa2 = fLinAmpPlusOne - fLinAmpMinusOneTimesCosOmega - fTwoSqrtATimesAlpha;
			}
		else if (in_eFilterType == FilterType_Peaking)
			{
				AkReal32 fOmega = 2.f * PI * fFrequency / in_fSampleRate;	// Normalized angular frequency
				AkReal32 fCosOmega = cosf(fOmega);											// Cos omega
#ifdef AKSIMD_FAST_EXP_IMPLEMENTED
				AkReal32 fLinAmp = AKSIMD_POW10_SCALAR(in_fGain*0.025f);
#else
				AkReal32 fLinAmp = AkMath::FastPow10(in_fGain*0.025f);
#endif
				// alpha = sin(w0)/(2*Q)
				AkReal32 fAlpha = sinf(fOmega)/(2.f*in_fQ);
				// Coefficient formulas
				fb0 = 1.f + fAlpha*fLinAmp;
				fb1 = -2.f*fCosOmega;
				fb2 = 1.f - fAlpha*fLinAmp;
				fa0 = 1.f + fAlpha/fLinAmp;
				fa1 = -2.f*fCosOmega;
				fa2 = 1.f - fAlpha/fLinAmp;
			}

		// Normalize all 6 coefficients into 5 and flip sign of recursive coefficients 
		// to add in difference equation instead
		// Note keep gain separate from fb0 coefficients to avoid 
		// recomputing coefficient on output volume changes
		SetCoefs(fb0/fa0, fb1/fa0, fb2/fa0, fa1/fa0, fa2/fa0);
	}
#endif

#if defined(AKSIMD_V2F32_SUPPORTED)

	
	void ProcessBuffer( AkReal32 * io_pBuffer, AkUInt32 in_uNumframes, AkUInt32 in_iChannel = 0 )
	{
		AkUInt32 uUnalignedStart = (AkUIntPtr)io_pBuffer & (AK_SIMD_ALIGNMENT-1);
		if (AK_EXPECT_FALSE(uUnalignedStart))
		{
			uUnalignedStart = (AK_SIMD_ALIGNMENT-uUnalignedStart)/ sizeof(AkReal32);
			uUnalignedStart = AkMin( uUnalignedStart, in_uNumframes );
			ProcessBufferScalar(io_pBuffer, uUnalignedStart, in_iChannel);
			io_pBuffer += uUnalignedStart;
			in_uNumframes -= uUnalignedStart;
		}

		AkUInt32 uUnalignedSize = in_uNumframes & 1;

		AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pBuffer;
		const AkReal32 * pfEnd = io_pBuffer + (in_uNumframes - uUnalignedSize);
		Memories Mems = GetMemories(in_iChannel);

		AKSIMD_V2F32 vX, vXPrev, vYPrev, vXSwap,  vY;
		vXPrev[1] = Mems.fFFwd1;
		vXPrev[0] = Mems.fFFwd2;
		vYPrev[1] = Mems.fFFbk1;
		vYPrev[0] = Mems.fFFbk2;
		while ( pfBuf < pfEnd )
		{
			vX = AKSIMD_LOAD_V2F32(pfBuf);
			vY = AKSIMD_MUL_V2F32(vX, m_Coefficients.vFirst);
			vY = AKSIMD_MADD_V2F32(vXPrev, m_Coefficients.vSecond, vY);
			vXPrev = AKSIMD_SWAP_V2F32(vXPrev);
			vY = AKSIMD_MADD_V2F32(vXPrev, m_Coefficients.vThird, vY);
			vXSwap = AKSIMD_SWAP_V2F32(vX);
			vY = AKSIMD_MADD_V2F32(vXSwap, m_Coefficients.vFourth, vY);
			vY = AKSIMD_MADD_V2F32(vYPrev, m_Coefficients.vFifth, vY);
			vYPrev = AKSIMD_SWAP_V2F32(vYPrev);				
			vY = AKSIMD_MADD_V2F32(vYPrev, m_Coefficients.vSixth, vY);
			
			vYPrev = vY;
			vXPrev = vX;

			AKSIMD_STORE_V2F32(pfBuf, vY);
			pfBuf += 2;
		}

		Mems.fFFwd1 = vXPrev[1];
		Mems.fFFwd2 = vXPrev[0];
		Mems.fFFbk1 = vYPrev[1];
		Mems.fFFbk2 = vYPrev[0];

		RemoveDenormal( Mems.fFFbk1 );
		RemoveDenormal( Mems.fFFbk2 );

		SetMemories( Mems, in_iChannel);

		if (AK_EXPECT_FALSE(uUnalignedSize))
			ProcessBufferScalar(pfBuf, uUnalignedSize, in_iChannel);
	}
#elif defined AKSIMD_V4F32_SUPPORTED && !defined __PPU__

	
	void ProcessBuffer( AkReal32 * io_pBuffer, AkUInt32 in_uNumframes, AkUInt32 in_iChannel = 0)
	{
		AkUInt32 uUnalignedStart = (AkUIntPtr)io_pBuffer & (AK_SIMD_ALIGNMENT-1);
		if (AK_EXPECT_FALSE(uUnalignedStart))
		{
			uUnalignedStart = (AK_SIMD_ALIGNMENT-uUnalignedStart)/ sizeof(AkReal32);
			uUnalignedStart = AkMin( uUnalignedStart, in_uNumframes );
			ProcessBufferScalar(io_pBuffer, uUnalignedStart, in_iChannel);
			io_pBuffer += uUnalignedStart;
			in_uNumframes -= uUnalignedStart;
		}

		AkUInt32 uUnalignedSize = in_uNumframes & 3;

		AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pBuffer;
		const AkReal32 * pfEnd = io_pBuffer + (in_uNumframes-uUnalignedSize);
		Memories Mems = GetMemories(in_iChannel);

		AKSIMD_V4F32 vY, vC, vD, vYPrev1, vYPrev2, vXPrev1, vXPrev2;

		vXPrev1 = AKSIMD_SET_V4F32(Mems.fFFwd1);
		vXPrev2 = AKSIMD_SET_V4F32(Mems.fFFwd2);
		vYPrev1 = AKSIMD_SET_V4F32(Mems.fFFbk1);
		vYPrev2 = AKSIMD_SET_V4F32(Mems.fFFbk2);

		/*
		The following code is the implementation of the biquad filter in direct form: 

		Y = x0*B0 + xm2*B2 + xm1*B1 + ym2*A2 + ym1*A1  (the "m" in var names stands for "minus".  "p" for plus.  So xm1 means the previous x, and yp1 means the next y.

		Since the output of Y+1 depends on Y+0, it follows that Y+1 depends on the same inputs as Y+0.  Given that, we can recursively replace Y+0 in the Y+1 formula:
		yp0 = xp0*B0 + xm2*B2 + xm1*B1 + ym2*A2 + ym1*A1
		yp1 = xp1*B0 + xm1*B2 + xp0*B1 + ym1*A2 + (xp0*B0 + xm2*B2 + xm1*B1 + ym2*A2 + ym1*A1)*A1	(Note the shifts in other inputs: xp0 became xp1, etc)
		
		Repeat the process for yp2 and yp3, regroup terms and simplify and you get:

				1		2				3						4										5											6						7							8	
		yp0 = xp0*B0 																					+ xm1*B1 								  + xm2*B2 				  + ym1*A1 						+ ym2*A2
		yp1 = xp1*B0 										   + xp0(B1+B0A1) 							+ xm1(B2+B1A1) 							  + xm2*B2A1 			  + ym1(A1A1+A2) 				+ ym2*A2A1
		yp2 = xp2*B0 				+ xp1(B1+B0A1) 			   + xp0(B2+B1A1+B0A1A1+B0A2) 				+ xm1(B2A1+B1A1A1+B1A2) 				  + xm2(B2A1A1+B2A2) 	  + ym1(A1A1A1+A2A1+A1A2) 		+ ym2(A2A1A1+A2A2)
		yp3 = xp3*B0 + xp2(B1+A1B0) + xp1(B2+A1B1+A1A1B0+A2B0) + xp0(A1B2+A1A1B1+A1A1A1B0+2A1A2B0+A2B1) + xm1(A1A1B2+A1A1A1B1+A1A2B1+A2B2+A1A2B1) + xm2(A1A1A1B2+2A1A2B2) + ym1(A1A1A1A1+3A1A1A2+A2A2) 	+ ym2(2A1A2A2+A1A1A1A2)

		The constants are computed from the ordinary coefficients in SetCoefs
		*/

		//Important performance note.  While very tempting to use MADD to compute and accumulate each terms of the formula, 
		//it is NOT optimal as the latency of the instruction when there are dependencies between the result and the next instruction
		//comes back to bite.  So, by dividing the MUL and ADD operation, they can be pipelined more easily by the processor.  
		//This saves A LOT of time, so it is worth being even less readable :(  The code assumes the Xbox latency for Add and Mul, which is 4 cycles (1 cycle throughput)

		while ( pfBuf < pfEnd )
		{
			vC = AKSIMD_LOAD_V4F32(pfBuf);		//Ready in 2 cycles if in cache
			AKSIMD_V4F32 term5 = AKSIMD_MUL_V4F32(vXPrev1, m_Coefficients.vXPrev1);	
			AKSIMD_V4F32 term6 = AKSIMD_MUL_V4F32(vXPrev2, m_Coefficients.vXPrev2);	
			AKSIMD_V4F32 term1 = AKSIMD_MUL_V4F32(vC, m_Coefficients.vFirst); 
			AKSIMD_V4F32 term7 = AKSIMD_MUL_V4F32(vYPrev1, m_Coefficients.vYPrev1);	
			AKSIMD_V4F32 term8 = AKSIMD_MUL_V4F32(vYPrev2, m_Coefficients.vYPrev2);	
			vXPrev1 = AKSIMD_SPLAT_V4F32(vC,3);
			vXPrev2 = AKSIMD_SPLAT_V4F32(vC,2);
			vD = AKSIMD_SPLAT_V4F32(vC, 1);
			vC = AKSIMD_SPLAT_V4F32(vC, 0);	
			term5 = AKSIMD_ADD_V4F32(term5, term6);	//Partial sum
			term7 = AKSIMD_ADD_V4F32(term7, term8);	//Partial sum
			AKSIMD_V4F32 term2 = AKSIMD_MUL_V4F32(vXPrev2, m_Coefficients.vSecond);	//XPrev2 contains x[2] at this point!
			AKSIMD_V4F32 term3 = AKSIMD_MUL_V4F32(vD, m_Coefficients.vThird);
			AKSIMD_V4F32 term4 = AKSIMD_MUL_V4F32(vC, m_Coefficients.vFourth);	
			vY = AKSIMD_ADD_V4F32(term5, term1);
			term7 = AKSIMD_ADD_V4F32(term7, term2);
			term3 = AKSIMD_ADD_V4F32(term3, term4);
			vY = AKSIMD_ADD_V4F32(vY, term7);	//Will wait 2 cycles because of term7
			vY = AKSIMD_ADD_V4F32(vY, term3);	//Will wait 3 cycles because of vY

			AKSIMD_STORE_V4F32(pfBuf, vY);
			vYPrev1 = AKSIMD_SPLAT_V4F32(vY,3);
			vYPrev2 = AKSIMD_SPLAT_V4F32(vY,2);
			pfBuf += 4;
		}

		Mems.fFFwd1 = AKSIMD_GETELEMENT_V4F32(vXPrev1,0);
		Mems.fFFwd2 = AKSIMD_GETELEMENT_V4F32(vXPrev2,0);
		Mems.fFFbk1 = AKSIMD_GETELEMENT_V4F32(vYPrev1,0);
		Mems.fFFbk2 = AKSIMD_GETELEMENT_V4F32(vYPrev2,0);

		RemoveDenormal( Mems.fFFbk1 );
		RemoveDenormal( Mems.fFFbk2 );

		SetMemories( Mems, in_iChannel );

		if (AK_EXPECT_FALSE(uUnalignedSize))
			ProcessBufferScalar(pfBuf, uUnalignedSize, in_iChannel);
	}
#else
	
	void ProcessBuffer( AkReal32 * io_pBuffer, AkUInt32 in_uNumframes, AkUInt32 in_iChannel = 0)
	{
		ProcessBufferScalar(io_pBuffer, in_uNumframes, in_iChannel);
	}
#endif
	
	void ProcessBufferScalar( AkReal32 * io_pBuffer, AkUInt32 in_uNumframes, AkUInt32 in_iChannel = 0)
	{
		AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pBuffer;
		const AkReal32 * pfEnd = io_pBuffer + in_uNumframes;
		Memories Mems = GetMemories(in_iChannel);

		while ( pfBuf < pfEnd )
		{
			// Feedforward part
			AkReal32 fOut = *pfBuf * m_Coefficients.fB0;
			Mems.fFFwd2 = Mems.fFFwd2 * m_Coefficients.fB2;
			fOut = fOut + Mems.fFFwd2;
			Mems.fFFwd2 = Mems.fFFwd1;
			Mems.fFFwd1 = Mems.fFFwd1 * m_Coefficients.fB1;
			fOut = fOut + Mems.fFFwd1;
			Mems.fFFwd1 = *pfBuf;

			// Feedback part
			Mems.fFFbk2 = Mems.fFFbk2 * m_Coefficients.fA2;
			fOut = fOut + Mems.fFFbk2;
			Mems.fFFbk2 = Mems.fFFbk1;
			Mems.fFFbk1 = Mems.fFFbk1 * m_Coefficients.fA1;
			fOut = fOut + Mems.fFFbk1;
			Mems.fFFbk1 = fOut;

			*pfBuf++ = fOut;		
		}

		RemoveDenormal( Mems.fFFbk1 );
		RemoveDenormal( Mems.fFFbk2 );

		SetMemories(Mems, in_iChannel);
	}
