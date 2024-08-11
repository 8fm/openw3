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
// AkSink.h
//
// Platform dependent part
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "AkParameters.h"
#include "AkCommon.h"
#include "PlatformAudiolibDefs.h" // FT
#include "AkLEngineStructs.h"
#include "AkFileParserBase.h"
#include "AkCaptureMgr.h"
#include "AudioLibDefs.h"

#include "AkRuntimeEnvironmentMgr.h"
#include <AK/Tools/Common/AkListBare.h>

class CAkMergingSink;

#define AK_XAUDIO2

#ifndef AK_USE_METRO_API
	#define AK_DIRECTSOUND
#endif

struct IDirectSound8;
struct IXAudio2;

namespace AK
{
	class IAkStdStream;
};

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

struct BufferUpdateParams
{
	AkUInt32	ulOffset;
	AkUInt32	ulBytes;
	void*		pvAudioPtr1;
	AkUInt32	ulAudioBytes1;
	void*		pvAudioPtr2;
	AkUInt32	ulAudioBytes2;
	AkUInt32	ulFlags;
};

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

class CAkSink
{
public:
	virtual ~CAkSink() {}

	static CAkSink * Create( AkOutputSettings & in_settings, AkSinkType in_eType, AkUInt32 in_uInstance );

	virtual AKRESULT Play() = 0;
	virtual void Term();

	AkForceInline AkPipelineBufferBase &NextWriteBuffer()
	{
		m_MasterOut.uValidFrames = 0;
		return m_MasterOut;
	}

	AkForceInline AkChannelMask GetSpeakerConfig() { return m_SpeakersConfig; }

	virtual DWORD GetThreadWaitTime() = 0;
	virtual bool IsStarved() = 0;
	virtual void ResetStarved() = 0;

	virtual AKRESULT IsDataNeeded( AkUInt32 & out_uBuffersNeeded ) = 0;

	virtual AKRESULT PassData() = 0;

	virtual AKRESULT PassSilence() = 0;

	virtual IXAudio2* GetWwiseXAudio2Interface() { return NULL; }
	virtual IDirectSound8 * GetDirectSoundInstance() { return NULL; }

	void StartOutputCapture(const AkOSChar* in_CaptureFileName);
	void StopOutputCapture();
#ifndef AK_OPTIMIZED
	AkSinkStats m_stats;
#endif

	static void RerouteMerges(CAkSink* in_pNewSinks);
	virtual void FinishMix();

	AkForceInline AkSinkType GetType() {return m_eType;}

protected:
	CAkSink(AkSinkType in_eType);
	static CAkSink * CreateDummy( AkChannelMask in_uChannelMask );
	static CAkSink * CreateInternal( AkOutputSettings & in_settings, AkSinkType in_eType, AkUInt32 in_uInstance );

	AKRESULT AllocBuffer();

#ifndef AK_OPTIMIZED
	void UpdateProfileData( AkReal32 * in_pfSamples, AkUInt32 in_uNumSamples );
	void UpdateProfileSilence( AkUInt32 in_uNumSamples );

#endif
	typedef AkListBare<CAkMergingSink, AkListBareNextItem<CAkMergingSink>, AkCountPolicyWithCount> ListOfMergingSinks;
	static ListOfMergingSinks s_MergingSinks;
	bool m_bWaitForMerge;
	AkCaptureFile* m_pCapture;

	AkChannelMask m_SpeakersConfig;				// speakers config
	AkPipelineBufferBase m_MasterOut;
	AkSinkType				m_eType;
};

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//
// In the event that there is no sound card or driver present, the application will revert to creating
// a dummy sink
//
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
class CAkSinkDummy : public CAkSink
{
	DWORD m_Timer;
	DWORD m_dwMSPerBuffer;

public:
	CAkSinkDummy() : CAkSink( AkSink_Dummy) {};
	~CAkSinkDummy(){};

	// Used by Create 
	AKRESULT Init(AkChannelMask in_uChannels);	
	
	// CAkSink overrides
	virtual AKRESULT Play();
	virtual void Term();

	virtual DWORD GetThreadWaitTime();
	virtual bool IsStarved();
	virtual void ResetStarved();

	virtual AKRESULT IsDataNeeded( AkUInt32 & out_uBuffersNeeded );

	virtual AKRESULT PassData();

	virtual AKRESULT PassSilence();

	AkUInt16 m_usBlockAlign;
};

class CAkMergingSink : public CAkSinkDummy
{
public:
	CAkMergingSink() : CAkSinkDummy() , m_pTarget (NULL), pNextItem(NULL) {};
	AKRESULT Init(AkChannelMask in_uChannels, CAkSink* in_pTarget, AkPipelineBufferBase* in_pBuffer);
	virtual void Term();
	virtual AKRESULT PassData();
	virtual AKRESULT PassSilence();

	void ReplaceSink(CAkSink* in_pTarget, AkPipelineBufferBase* in_pBuffer);

	CAkMergingSink* pNextItem;
private:
	AkPipelineBufferBase *m_pMixingBuffer;
	CAkSink* m_pTarget;
};

//----------------------------------------------------------------------------------------------------

#if defined AK_CPU_X86 || defined AK_CPU_X86_64

#include <emmintrin.h>

static void FloatsToShortsSSE2( AkInt16 * dst, float * src, int nFloats )
{
	// This routine peaks at about 2.1 gigabytes / second on a P4 3 GHz. Unrolling helps a bit on the P4,
	// makes a bigger difference on Athlon64.

	AKASSERT( !( nFloats % 16 ) ); // operates on 16 floats at a time

	__m128 * pmIn = (__m128 *) src;
	__m128 * pmEnd = (__m128 *) ( src + nFloats );
	__m128i * pmOut = (__m128i *) dst;

	__m128 mMul = _mm_set_ps1( AUDIOSAMPLE_SHORT_MAX ); // duplicate multiplier factor 4x
	while ( pmIn < pmEnd )
	{
		__m128 mTmp1 = _mm_mul_ps( pmIn[ 0 ], mMul );
		__m128 mTmp2 = _mm_mul_ps( pmIn[ 1 ], mMul );
		__m128 mTmp3 = _mm_mul_ps( pmIn[ 2 ], mMul );
		__m128 mTmp4 = _mm_mul_ps( pmIn[ 3 ], mMul );
		
		// manually in-lined version of _mm_cvtps_epi32, because inlining doesn't actually work !

		__m128i mShorts1 = _mm_packs_epi32(_mm_cvtps_epi32(mTmp1), _mm_cvtps_epi32(mTmp2));
		_mm_storeu_si128(pmOut, mShorts1);

		__m128i mShorts2 = _mm_packs_epi32(_mm_cvtps_epi32(mTmp3), _mm_cvtps_epi32(mTmp4));
		_mm_storeu_si128(pmOut + 1, mShorts2);

		pmIn += 4;
		pmOut += 2;
	}
}

#ifdef AK_CPU_X86
static void FloatsToShortsMMX( AkInt16 * dst, float * src, int nFloats )
{
	// This routine peaks at about 2.1 gigabytes / second on a P4 3 GHz. Unrolling helps a bit on the P4,
	// makes a bigger difference on Athlon64.

	AKASSERT( !( nFloats % 16 ) ); // operates on 16 floats at a time

	__m128 * pmIn = (__m128 *) src;
	__m128 * pmEnd = (__m128 *) ( src + nFloats );
	__m64 * pmOut = (__m64 *) dst;

	__m128 mMul = _mm_set_ps1( AUDIOSAMPLE_SHORT_MAX ); // duplicate multiplier factor 4x
	while ( pmIn < pmEnd )
	{
		__m128 mTmp1 = _mm_mul_ps( pmIn[ 0 ], mMul );
		__m128 mTmp2 = _mm_mul_ps( pmIn[ 1 ], mMul );
		__m128 mTmp3 = _mm_mul_ps( pmIn[ 2 ], mMul );
		__m128 mTmp4 = _mm_mul_ps( pmIn[ 3 ], mMul );
		
		// manually in-lined version of _mm_cvtps_pi16, because inlining doesn't actually work !

		__m64 mShorts1 = _mm_packs_pi32(_mm_cvtps_pi32(mTmp1), _mm_cvtps_pi32(_mm_movehl_ps(mTmp1, mTmp1)));
		_mm_stream_pi( pmOut, mShorts1 );

		__m64 mShorts2 = _mm_packs_pi32(_mm_cvtps_pi32(mTmp2), _mm_cvtps_pi32(_mm_movehl_ps(mTmp2, mTmp2)));
		_mm_stream_pi( pmOut + 1, mShorts2 );

		__m64 mShorts3 = _mm_packs_pi32(_mm_cvtps_pi32(mTmp3), _mm_cvtps_pi32(_mm_movehl_ps(mTmp3, mTmp3)));
		_mm_stream_pi( pmOut + 2, mShorts3 );

		__m64 mShorts4 = _mm_packs_pi32(_mm_cvtps_pi32(mTmp4), _mm_cvtps_pi32(_mm_movehl_ps(mTmp4, mTmp4)));
		_mm_stream_pi( pmOut + 3, mShorts4 );

		pmIn += 4;
		pmOut += 4;
	}
	_mm_empty();
}
#endif

static AkForceInline void FloatsToShorts( AkInt16 * dst, float * src, int nFloats )
{
#ifdef AK_CPU_X86
		if (AK::AkRuntimeEnvironmentMgr::Instance()->GetSIMDSupport(AK::AK_SIMD_SSE2))
#endif
		{
			FloatsToShortsSSE2((AkInt16 *) dst, (float *) src, nFloats );
		}
#ifdef AK_CPU_X86
		else
		{
			FloatsToShortsMMX( (AkInt16 *) dst, (float *) src, nFloats );
		}
#endif
}

#else

static AkForceInline void FloatsToShorts( AkInt16 * dst, float * src, int nFloats )
{
	do
	{
		AkReal32 fSample = *src;

		if ( fSample > AUDIOSAMPLE_FLOAT_MAX )
			fSample = AUDIOSAMPLE_FLOAT_MAX;
		else if ( fSample < AUDIOSAMPLE_FLOAT_MIN )
			fSample = AUDIOSAMPLE_FLOAT_MIN;

		*dst = (AkInt16) ( fSample * AUDIOSAMPLE_SHORT_MAX );

		src += 1;
		dst += 1;
	}
	while( --nFloats );
}

#endif

extern CAkSink* g_pAkSink;
