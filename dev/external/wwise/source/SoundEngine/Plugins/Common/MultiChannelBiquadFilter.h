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

#ifndef _AK_MULTICHANNELBIQUADFILTER_H_
#define _AK_MULTICHANNELBIQUADFILTER_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "AkDSPUtils.h"
#if !defined(__SPU__) || defined(AKENABLESPUBIQUADCOMPUTECOEFS)
#include <math.h>
#endif

namespace AK
{
	namespace DSP
	{

		template < AkUInt32 T_MAXNUMCHANNELS > 
		class MultiChannelBiquadFilter
		{
		public:

			// Filter coefficients
			struct Coefficients
			{
				AkReal32 fB0;
				AkReal32 fB1;
				AkReal32 fB2;
				AkReal32 fA1;
				AkReal32 fA2;

				Coefficients()
				{
					fB0 = 1.f;
					fB1 = 0.f;
					fB2 = 0.f;
					fA1 = 0.f;
					fA2 = 0.f;
				}
			};

			// Filter memories
			struct Memories
			{		
				AkReal32 fFFwd1;
				AkReal32 fFFwd2;
				AkReal32 fFFbk1;
				AkReal32 fFFbk2;

				Memories()
				{
					fFFwd1 = 0.f;
					fFFwd2 = 0.f;
					fFFbk1 = 0.f;
					fFFbk2 = 0.f;
				}
			};

			enum FilterType
			{
				FilterType_LowShelf = 0,
				FilterType_Peaking,
				FilterType_HighShelf,
				FilterType_LowPass,
				FilterType_HighPass,
				FilterType_BandPass,
				FilterType_Notch,
			};

#if !defined(__SPU__) || defined(AKENABLESPUBIQUADCOMPUTECOEFS)
			void ComputeCoefs(	
				FilterType in_eFilterType, 
				AkUInt32 in_uSampleRate, 
				AkReal32 in_fFreq, 
				AkReal32 in_fGain, /* = 0.f */ 
				AkReal32 in_fQ ) /* = 1.f */ 
			{
				static const AkReal32 SHELFSLOPE = 1.f; // Maximum shelf slope
				static const AkReal32 PI = 	3.14159265358979323846f;
				static const AkReal32 SQRT2 = 1.41421356237309504880f;

				AkReal32 fb0,fb1,fb2;	// Feed forward coefficients
				AkReal32 fa0,fa1,fa2;	// Feed back coefficients

				// Note: Q and "bandwidth" linked by:
				// 1/Q = 2*sinh(ln(2)/2*BandWidth*fOmega/sin(fOmega))

				AkReal32 fFrequency = in_fFreq;

				// Frequency must be less or equal to the half of the sample rate
				// 0.9 is there because of bilinear transform behavior at frequencies close to Nyquist
				const AkReal32 fMaxFrequency = in_uSampleRate * 0.5f * 0.9f;
				if ( fFrequency >= fMaxFrequency )
					fFrequency = fMaxFrequency; 

				switch ( in_eFilterType)
				{
				default:
				case FilterType_LowPass:
					{
						// Butterworth low pass for flat pass band
						AkReal32 PiFcOSr	= PI * fFrequency / in_uSampleRate;
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
						break;
					}
				case FilterType_HighPass:
					{
						// Butterworth high pass for flat pass band
						AkReal32 PiFcOSr	= PI * fFrequency / in_uSampleRate;
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
						break;
					}
				case FilterType_BandPass:
					{
						AkReal32 fOmega = 2.f * PI * fFrequency / in_uSampleRate;	// Normalized angular frequency
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
						break;
					}
				case FilterType_Notch:
					{
						AkReal32 fOmega = 2.f * PI * fFrequency / in_uSampleRate;	// Normalized angular frequency
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
						break;
					}
				case FilterType_LowShelf:
					{
						AkReal32 fLinAmp = powf(10.f,in_fGain*0.025f);
						AkReal32 fOmega = 2.f * PI * fFrequency / in_uSampleRate;	// Normalized angular frequency
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
						break;
					}
				case FilterType_HighShelf:
					{
						AkReal32 fLinAmp = powf(10.f,in_fGain*0.025f);
						AkReal32 fOmega = 2.f * PI * fFrequency / in_uSampleRate;	// Normalized angular frequency
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
						break;
					}
				case FilterType_Peaking:
					{
						AkReal32 fOmega = 2.f * PI * fFrequency / in_uSampleRate;	// Normalized angular frequency
						AkReal32 fCosOmega = cosf(fOmega);											// Cos omega
						AkReal32 fLinAmp = powf(10.f,in_fGain*0.025f);
						// alpha = sin(w0)/(2*Q)
						AkReal32 fAlpha = sinf(fOmega)/(2.f*in_fQ);
						// Coefficient formulas
						fb0 = 1.f + fAlpha*fLinAmp;
						fb1 = -2.f*fCosOmega;
						fb2 = 1.f - fAlpha*fLinAmp;
						fa0 = 1.f + fAlpha/fLinAmp;
						fa1 = -2.f*fCosOmega;
						fa2 = 1.f - fAlpha/fLinAmp;
						break;
					}
				}

				// Normalize all 6 coefficients into 5 and flip sign of recursive coefficients 
				// to add in difference equation instead
				// Note keep gain separate from fb0 coefficients to avoid 
				// recomputing coefficient on output volume changes
				m_Coefficients.fB0 = fb0/fa0;
				m_Coefficients.fB1 = fb1/fa0;
				m_Coefficients.fB2 = fb2/fa0;
				m_Coefficients.fA1 = -fa1/fa0;
				m_Coefficients.fA2 = -fa2/fa0;
			}

			void Reset()
			{
				for ( AkUInt32 i = 0; i < T_MAXNUMCHANNELS; i++ )
					m_Memories[i].fFFwd1 = m_Memories[i].fFFwd2 = m_Memories[i].fFFbk1 = m_Memories[i].fFFbk2 = 0.f;
			}
#endif

			void ProcessChannel( AkReal32 * io_pfBuffer, AkUInt32 in_uNumframes, AkUInt32 in_uChannelIndex )
			{
				AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pfBuffer;
				const AkReal32 * pfEnd = io_pfBuffer + in_uNumframes;
				Memories Mems = m_Memories[in_uChannelIndex];

				while ( pfBuf < pfEnd )
				{
					// Feedforward part
					AkReal32 fIn = *pfBuf;
					AkReal32 fOut = fIn * m_Coefficients.fB0;
					Mems.fFFwd2 = Mems.fFFwd2 * m_Coefficients.fB2;
					fOut = fOut + Mems.fFFwd2;
					Mems.fFFwd2 = Mems.fFFwd1;
					Mems.fFFwd1 = Mems.fFFwd1 * m_Coefficients.fB1;
					fOut = fOut + Mems.fFFwd1;
					Mems.fFFwd1 = fIn;
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

				m_Memories[in_uChannelIndex] = Mems;
			}

			void ProcessBuffer( AkAudioBuffer * io_pBuffer )
			{
				const AkUInt32 uNumChannels = io_pBuffer->NumChannels();
				const AkUInt32 uNumFrames = io_pBuffer->uValidFrames;
				for ( AkUInt32 i = 0; i < uNumChannels; i++ )
				{
					AkReal32 * pfChan = io_pBuffer->GetChannel(i);
					ProcessChannel( pfChan, uNumFrames, i );
				}
			}

		protected:

			Coefficients	m_Coefficients;
			Memories		m_Memories[T_MAXNUMCHANNELS];
		};

	} // namespace DSP
}

#endif // _AK_MULTICHANNELBIQUADFILTER_H_