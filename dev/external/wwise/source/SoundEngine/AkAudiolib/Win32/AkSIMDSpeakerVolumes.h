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

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSimd.h>
#include "AkMath.h"

struct AkSIMDSpeakerVolumes
{
	union
	{
		AKSIMD_V4F32 vector[2];

		AkSpeakerVolumes volumes;

		AkReal32 aVolumes[8];
	};

	AkSIMDSpeakerVolumes()
	{
	}

	AkSIMDSpeakerVolumes( const AkReal32 fVolume)
	{
		Set(fVolume);
	}
	
	AkSIMDSpeakerVolumes( const AkSpeakerVolumes sVolumes)
		:volumes(sVolumes)
	{
	}

	AkForceInline AkSIMDSpeakerVolumes & operator=( const AkSIMDSpeakerVolumes& in_vol )
	{
		vector[0] = in_vol.vector[0];
		vector[1] = in_vol.vector[1];

		return *this;
	}

	AkForceInline void CopyTo( AkSpeakerVolumes & out_vol ) const
	{
		out_vol = *((AkSpeakerVolumes *) this );
	}

	AkForceInline void CopyTo( AkSIMDSpeakerVolumes & out_vol ) const
	{
		AKSIMD_STOREU_V4F32( (float*)&out_vol, vector[0] );
		AKSIMD_STOREU_V4F32( (float*)&out_vol + 4, vector[1] );		
	}

	AkForceInline AkSIMDSpeakerVolumes & operator=( const AkSpeakerVolumes& in_vol )
	{
		*((AkSpeakerVolumes *) this ) = in_vol;
		
#ifndef AK_71AUDIO
		AKSIMD_GETELEMENT_V4F32( vector[1], 2 ) = 0;
		AKSIMD_GETELEMENT_V4F32( vector[1], 3 ) = 0;
#endif

		return *this;
	}

	AkForceInline void Set( const AkReal32 in_fVol )
	{
		AKSIMD_V4F32 vTmp = AKSIMD_LOAD1_V4F32( in_fVol ); 
		vector[0] = vTmp;
		vector[1] = vTmp;
	}

	AkForceInline void Zero()
	{
		AKSIMD_V4F32 vTmp = AKSIMD_SETZERO_V4F32(); 
		vector[0] = vTmp;
		vector[1] = vTmp;
	}

	AkForceInline void Add( const AkReal32 in_fVol )
	{
		AKSIMD_V4F32 vTmp = AKSIMD_LOAD1_V4F32( in_fVol ); 
		vector[0] = AKSIMD_ADD_V4F32( vector[0], vTmp );
		vector[1] = AKSIMD_ADD_V4F32( vector[1], vTmp );
	}

	AkForceInline void Add( const AkSIMDSpeakerVolumes & in_vol )
	{
		vector[0] = AKSIMD_ADD_V4F32( vector[0], in_vol.vector[0] );
		vector[1] = AKSIMD_ADD_V4F32( vector[1], in_vol.vector[1] );
	}

	AkForceInline void Mul( const AkReal32 in_fVol )
	{
		AKSIMD_V4F32 vTmp = AKSIMD_LOAD1_V4F32( in_fVol ); 
		vector[0] = AKSIMD_MUL_V4F32( vector[0], vTmp );
		vector[1] = AKSIMD_MUL_V4F32( vector[1], vTmp );
	}

	AkForceInline void Mul( const AkSIMDSpeakerVolumes & in_vol )
	{
		vector[0] = AKSIMD_MUL_V4F32( vector[0], in_vol.vector[0] );
		vector[1] = AKSIMD_MUL_V4F32( vector[1], in_vol.vector[1] );
	}

	AkForceInline void Max( const AkSIMDSpeakerVolumes & in_vol )
	{
		vector[0] = AKSIMD_MAX_V4F32( vector[0], in_vol.vector[0] );
		vector[1] = AKSIMD_MAX_V4F32( vector[1], in_vol.vector[1] );
	}
	
	AkForceInline void Min( const AkSIMDSpeakerVolumes & in_vol )
	{
		vector[0] = AKSIMD_MIN_V4F32( vector[0], in_vol.vector[0] );
		vector[1] = AKSIMD_MIN_V4F32( vector[1], in_vol.vector[1] );
	}

	AkForceInline void Sqrt()
	{
		vector[0] = AKSIMD_SQRT_V4F32( vector[0] );
		vector[1] = AKSIMD_SQRT_V4F32( vector[1] );
	}

	AkForceInline void dBToLin()
	{
		volumes.fFrontLeft = AkMath::dBToLin( volumes.fFrontLeft );
		volumes.fFrontRight = AkMath::dBToLin( volumes.fFrontRight );
		volumes.fCenter = AkMath::dBToLin( volumes.fCenter );
		volumes.fLfe = AkMath::dBToLin( volumes.fLfe );
		volumes.fRearLeft = AkMath::dBToLin( volumes.fRearLeft );
		volumes.fRearRight = AkMath::dBToLin( volumes.fRearRight );
#ifdef AK_71AUDIO
		volumes.fSideLeft = AkMath::dBToLin( volumes.fSideLeft );
		volumes.fSideRight = AkMath::dBToLin( volumes.fSideRight );
#endif
	}

	AkForceInline void FastLinTodB()
	{
		volumes.fFrontLeft = AkMath::FastLinTodB( volumes.fFrontLeft );
		volumes.fFrontRight = AkMath::FastLinTodB( volumes.fFrontRight );
		volumes.fCenter = AkMath::FastLinTodB( volumes.fCenter );
		volumes.fLfe = AkMath::FastLinTodB( volumes.fLfe );
		volumes.fRearLeft = AkMath::FastLinTodB( volumes.fRearLeft );
		volumes.fRearRight = AkMath::FastLinTodB( volumes.fRearRight );
#ifdef AK_71AUDIO
		volumes.fSideLeft = AkMath::FastLinTodB( volumes.fSideLeft );
		volumes.fSideRight = AkMath::FastLinTodB( volumes.fSideRight );
#endif
	}

#if defined AK_CPU_X86 || defined AK_CPU_X86_64

	AkForceInline bool IsLessOrEqual( const float * in_pThreshold )
	{
		__m128 vThreshold = _mm_load_ps1( in_pThreshold ); 
		__m128 vResult0 = _mm_cmple_ps( vector[0], vThreshold );
		__m128 vResult1 = _mm_cmple_ps( vector[1], vThreshold );
		int result0 = _mm_movemask_ps( vResult0 ); 
		int result1 = _mm_movemask_ps( vResult1 ); 
#ifdef AK_71AUDIO
		return ((result0 == 0xf) & (result1 ==0xf)) != 0; // entire 4-bit mask is set when ALL are less than
#else
		return ((result0 == 0xf) & ((result1 & 3)==3)) != 0; // entire 4-bit mask is set when ALL are less than
#endif
	}

#else

	AkForceInline bool IsLessOrEqual( const float * in_pThreshold )
	{
		return ( volumes.fFrontLeft <= *in_pThreshold )
			&& ( volumes.fFrontRight <= *in_pThreshold )
			&& ( volumes.fCenter <= *in_pThreshold )
			&& ( volumes.fLfe <= *in_pThreshold )
			&& ( volumes.fRearLeft <= *in_pThreshold )
			&& ( volumes.fRearRight <= *in_pThreshold )
#ifdef AK_71AUDIO
			&& ( volumes.fSideLeft <= *in_pThreshold )
			&& ( volumes.fSideRight <= *in_pThreshold )
#endif
			;
	}

#endif

};

