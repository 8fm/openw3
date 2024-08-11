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

#ifndef _AKBIQUADFILTER_H_
#define _AKBIQUADFILTER_H_

#include "stdafx.h"
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSimd.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include <string.h>
#include "AkDSPUtils.h"
#include <AK/Tools/Common/AkAssert.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkMath.h"
#if !defined(__SPU__) || defined(AKENABLESPUBIQUADCOMPUTECOEFS)
#include <math.h>
#endif
#include <AK/Tools/Common/AkObject.h>

extern AKSOUNDENGINE_API AkMemPoolId g_LEngineDefaultPoolId;

namespace DSP
{
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

	struct SingleChannelPolicy
	{
		AkForceInline bool Allocate(AkUInt32 in_uChannels) {return true;}

		AkForceInline void Free(){};

		AkForceInline Memories& Get(AkUInt32 in_uChannel) {return m_Memories;}

		AkForceInline void Reset() {AkZeroMemSmall(&m_Memories, sizeof(Memories));}

		AkForceInline bool IsInitialized(){return true;}

		Memories m_Memories;
	};

	struct MultiChannelPolicy
	{
		AkForceInline MultiChannelPolicy()
			: m_pMemories( NULL )
			, m_uSize( 0 )
		{
		}
		AkForceInline ~MultiChannelPolicy()
		{
			Free();
		}

		typedef Memories* MemoryType;
		AkForceInline bool Allocate(AkUInt32 in_uChannels) 
		{
#ifdef AK_PS3
			m_uSize = AK_ALIGN_SIZE_FOR_DMA( in_uChannels*sizeof(Memories) );
			m_pMemories = (Memories*) AkMalign( g_LEngineDefaultPoolId, m_uSize, 16 );
#else
			m_uSize = in_uChannels*sizeof(Memories);
			m_pMemories = (Memories*) AkAlloc( g_LEngineDefaultPoolId, m_uSize );
#endif
			if ( m_pMemories )
			{
				AkZeroMemSmall(m_pMemories, m_uSize);
				return true;
			}

			return false;
		};

		AkForceInline void Free()
		{
			if (m_pMemories)
			{
#ifdef AK_PS3
				AkFalign(g_LEngineDefaultPoolId, m_pMemories);
#else
				AkFree(g_LEngineDefaultPoolId, m_pMemories);
#endif		
				m_pMemories = NULL;
			}
		};

		AkForceInline Memories& Get(AkUInt32 in_uChannel) {return m_pMemories[in_uChannel];}

		AkForceInline void Reset() 
		{
			if ( m_pMemories )
				AkZeroMemSmall( m_pMemories, m_uSize );
		}

		AkForceInline bool IsInitialized(){return m_pMemories != NULL;}

		Memories* m_pMemories;
		AkUInt32 m_uSize;
	};

	template<class CHANNEL_POLICY>
	class BiquadFilter
	{
	public:

		// Filter coefficients
#ifdef AK_WIIU
		struct Coefficients
#else
		AK_ALIGN_SIMD (struct) Coefficients
#endif
		{
#if defined(AKSIMD_V2F32_SUPPORTED)
			AKSIMD_V2F32 vFirst;
			AKSIMD_V2F32 vSecond;
			AKSIMD_V2F32 vThird;
			AKSIMD_V2F32 vFourth;
			AKSIMD_V2F32 vFifth;
			AKSIMD_V2F32 vSixth;
#elif defined AKSIMD_V4F32_SUPPORTED
			AKSIMD_V4F32 vFirst;
			AKSIMD_V4F32 vSecond;
			AKSIMD_V4F32 vThird;
			AKSIMD_V4F32 vFourth;
			AKSIMD_V4F32 vXPrev1;
			AKSIMD_V4F32 vXPrev2;
			AKSIMD_V4F32 vYPrev1;
			AKSIMD_V4F32 vYPrev2;
#endif	
			AkReal32 fB0;
			AkReal32 fB1;
			AkReal32 fB2;
			AkReal32 fA1;
			AkReal32 fA2;
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

		BiquadFilter()
		{
			SetCoefs(1.f, 0.f, 0.f, 0.f, 0.f);
		}

		AKRESULT Init(AkUInt32 in_iChannels)
		{
			if (m_Memories.Allocate(in_iChannels))
				return AK_Success;

			return AK_InsufficientMemory;
		}

		void Term()
		{
			m_Memories.Free();
		}

		AkForceInline bool IsInitialized()
		{
			return m_Memories.IsInitialized();
		}

		AkForceInline void Reset()
		{
			m_Memories.Reset();
		}

		void SetCoefs( 
			AkReal32 in_fB0,
			AkReal32 in_fB1,
			AkReal32 in_fB2,
			AkReal32 in_fA1,
			AkReal32 in_fA2 )
		{
			in_fA1 = -in_fA1;
			in_fA2 = -in_fA2;
			m_Coefficients.fB0 = in_fB0;
			m_Coefficients.fB1 = in_fB1;
			m_Coefficients.fB2 = in_fB2;
			m_Coefficients.fA1 = in_fA1;
			m_Coefficients.fA2 = in_fA2;
#if defined(AKSIMD_V2F32_SUPPORTED)
			m_Coefficients.vFirst = AKSIMD_SET_V2F32(in_fB0);

			m_Coefficients.vSecond[0] = in_fB2;
			m_Coefficients.vSecond[1] = in_fB2 + in_fB1*in_fA1;

			m_Coefficients.vThird[0] = in_fB1;
			m_Coefficients.vThird[1] = in_fB2*in_fA1;

			m_Coefficients.vFourth[0] = 0;
			m_Coefficients.vFourth[1] = in_fB1 + in_fB0*in_fA1;

			m_Coefficients.vFifth[0] = in_fA2;
			m_Coefficients.vFifth[1] = in_fA1*in_fA1 + in_fA2;

			m_Coefficients.vSixth[0] = in_fA1;
			m_Coefficients.vSixth[1] = in_fA1*in_fA2;
#elif defined AKSIMD_V4F32_SUPPORTED
			m_Coefficients.vFirst = AKSIMD_SET_V4F32(in_fB0);

			AkReal32 tmp[4];
			tmp[0] = 0.f;
			tmp[1] = 0.f; 
			tmp[2] = 0.f;
			tmp[3] = in_fB1 + in_fA1*in_fB0;
			memcpy(&m_Coefficients.vSecond, tmp, sizeof(AKSIMD_V4F32));

			tmp[0] = 0.f;
			tmp[1] = 0.f; 
			tmp[2] = tmp[3];
			tmp[3] = in_fB2 + tmp[3]*in_fA1 + in_fB0*in_fA2;
			memcpy(&m_Coefficients.vThird, tmp, sizeof(AKSIMD_V4F32));

			tmp[0] = 0.f;
			tmp[1] = tmp[2];
			tmp[2] = tmp[3];
			tmp[3] = tmp[3]*in_fA1 + in_fA1*in_fA2*in_fB0 + in_fA2*in_fB1;
			memcpy(&m_Coefficients.vFourth, tmp, sizeof(AKSIMD_V4F32));

			tmp[0] = in_fB1;
			tmp[1] = in_fB2 + in_fB1*in_fA1;
			tmp[2] = in_fA1*(in_fB2 + in_fB1*in_fA1) + in_fB1*in_fA2;
			tmp[3] = in_fA1*tmp[2] +in_fA2*in_fB2 + in_fB1*in_fA2*in_fA1;
			memcpy(&m_Coefficients.vXPrev1, tmp, sizeof(AKSIMD_V4F32));

			tmp[0] = in_fB2;
			tmp[1] = in_fB2*in_fA1;
			tmp[2] = in_fB2*in_fA1*in_fA1 + in_fB2*in_fA2;
			tmp[3] = in_fB2*in_fA1*in_fA1*in_fA1 + 2*in_fB2*in_fA2*in_fA1;
			memcpy(&m_Coefficients.vXPrev2, tmp, sizeof(AKSIMD_V4F32));

			tmp[0] = in_fA1;
			tmp[1] = in_fA1*in_fA1 + in_fA2;
			tmp[2] = in_fA1*in_fA1*in_fA1 + 2*in_fA2*in_fA1;
			tmp[3] = in_fA1*in_fA1*in_fA1*in_fA1 + 3*in_fA2*in_fA1*in_fA1 + in_fA2*in_fA2;
			memcpy(&m_Coefficients.vYPrev1, tmp, sizeof(AKSIMD_V4F32));

			tmp[0] = in_fA2;
			tmp[1] = in_fA2*in_fA1;
			tmp[2] = in_fA2*in_fA1*in_fA1 + in_fA2*in_fA2;
			tmp[3] = in_fA2*in_fA1*in_fA1*in_fA1 + 2*in_fA2*in_fA2*in_fA1;
			memcpy(&m_Coefficients.vYPrev2, tmp, sizeof(AKSIMD_V4F32));			
#endif
		}

		AkForceInline Coefficients & GetCoefs( )
		{
			return m_Coefficients;
		}

		AkForceInline void CopyCoefs( Coefficients & in_rCoefficients )
		{
			m_Coefficients = in_rCoefficients;
		}

		AkForceInline Memories & GetMemories(AkUInt32 in_iChannel = 0)
		{
			return m_Memories.Get(in_iChannel);
		}

		AkForceInline void SetMemories(Memories & in_rMemories, AkUInt32 in_iChannel = 0 )
		{
			m_Memories.Get(in_iChannel) = in_rMemories;
		}

#ifdef AK_PS3
		AkForceInline Memories* TransferMemories(Memories * in_pMemories)
		{
			Memories * pRet = m_Memories.m_pMemories;
			m_Memories.m_pMemories = in_pMemories;
			return pRet;
		}
#endif
	
		AkForceInline AkReal32 ProcessSample( Memories & io_rMemories, AkReal32 in_fIn )
		{
			// Feedforward part
			AkReal32 fOut = in_fIn * m_Coefficients.fB0;
			io_rMemories.fFFwd2 = io_rMemories.fFFwd2 * m_Coefficients.fB2;
			fOut = fOut + io_rMemories.fFFwd2;
			io_rMemories.fFFwd2 = io_rMemories.fFFwd1;
			io_rMemories.fFFwd1 = io_rMemories.fFFwd1 * m_Coefficients.fB1;
			fOut = fOut + io_rMemories.fFFwd1;
			io_rMemories.fFFwd1 = in_fIn;

			// Feedback part
			io_rMemories.fFFbk2 = io_rMemories.fFFbk2 * m_Coefficients.fA2;
			fOut = fOut + io_rMemories.fFFbk2;
			io_rMemories.fFFbk2 = io_rMemories.fFFbk1;
			io_rMemories.fFFbk1 = io_rMemories.fFFbk1 * m_Coefficients.fA1;
			fOut = fOut + io_rMemories.fFFbk1;
			io_rMemories.fFFbk1 = fOut;
			return fOut;
		}

	protected:
		
		Coefficients	m_Coefficients;	//Must be first for correct SIMD alignment!
		CHANNEL_POLICY	m_Memories;

#include "BiquadFilter.cpp"	//Inlined code.
	} AK_ALIGN_DMA;

	typedef BiquadFilter<SingleChannelPolicy> BiquadFilterMono;
	typedef BiquadFilter<MultiChannelPolicy> BiquadFilterMulti;

} // namespace DSP

#endif // _AKBIQUADFILTER_H_