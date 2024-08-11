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

// Implement constant power mixdown of a number of input channel.
// Center and LFE channel are adjustable but others are taken full scale.
// Center and LFE channel gains are not changing at runtime and thus do not require interpolation.
// Constant power implies sum_i=1_N(ChanGain^2) = 1
// E.g. using 5.1 configuration (CG = Constant power channel gain)
// CG^2 (L) + CG^2 (R) + (CenterLevel*CG)^2 (C) + (LFELevel*CG)^2 (LFE) + CG^2 (LS) + CG^2 (RS)

#include "ConstantPowerChannelMixdown.h"
#include <AK/Tools/Common/AkAssert.h>
#include <math.h>
#include <string.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include <AK/SoundEngine/Common/AkSimd.h>


namespace DSP
{		
	void ConstantPowerChannelMixdown(	
		AkAudioBuffer * in_pAudioBufferIn,
		AkUInt32 in_uNumFrames,
		AkUInt32 in_uFrameOffset,
		AkReal32 * out_pfBufferOut, 
		AkChannelMask in_uChannelMask, 
		AkReal32 in_fInCenterGain,
		AkReal32 in_fInLFEGain )
	{
		if ( in_fInLFEGain > 1.f )
			in_fInLFEGain = 1.f;
		if ( in_fInCenterGain > 1.f )
			in_fInCenterGain = 1.f;

		AkReal32 fUnscaled = 0.f;
		AkReal32 fCenter = 0.f;
		AkReal32 fLFE = 0.f;
		if ( in_uChannelMask & AK_SPEAKER_FRONT_LEFT )
			fUnscaled += 1.f;
		if ( in_uChannelMask & AK_SPEAKER_FRONT_RIGHT )
			fUnscaled += 1.f;
#ifdef AK_71AUDIO
		if ( in_uChannelMask & AK_SPEAKER_SIDE_LEFT )
			fUnscaled += 1.f;
		if ( in_uChannelMask & AK_SPEAKER_SIDE_RIGHT )
			fUnscaled += 1.f;
#endif
		if ( in_uChannelMask & AK_SPEAKER_BACK_LEFT )
			fUnscaled += 1.f;
		if ( in_uChannelMask & AK_SPEAKER_BACK_RIGHT )
			fUnscaled += 1.f;
		if ( in_uChannelMask & AK_SPEAKER_FRONT_CENTER )
			fCenter = in_fInCenterGain*in_fInCenterGain;
		if ( in_uChannelMask & AK_SPEAKER_LOW_FREQUENCY )
			fLFE = in_fInLFEGain*in_fInLFEGain;
		AkReal32 fChanGain = sqrt( 1.f / ( fUnscaled + fCenter + fLFE ) );

		AkUInt32 uNumFrames = in_uNumFrames;
		memset( out_pfBufferOut, 0, uNumFrames*sizeof(AkReal32) );
		AkUInt32 uRemainingChannelMask = in_uChannelMask;
		AkUInt32 uCurrentChanMask = 1;
		AkUInt32 iChanIndex = 0;
		while ( uRemainingChannelMask )
		{
			if ( (uRemainingChannelMask & uCurrentChanMask) || (uRemainingChannelMask == AK_SPEAKER_LOW_FREQUENCY) )
			{
				AkReal32 fCombinedGain = fChanGain;
				if ( uCurrentChanMask & AK_SPEAKER_FRONT_CENTER )
					fCombinedGain *= in_fInCenterGain;
				// Ensure LFE is processed last
				if ( uRemainingChannelMask == AK_SPEAKER_LOW_FREQUENCY )
				{
					uRemainingChannelMask = 0; // Finished after LFE
					fCombinedGain *= in_fInLFEGain;	
				}
				else if ( uCurrentChanMask & AK_SPEAKER_LOW_FREQUENCY )
				{
					uCurrentChanMask<<=1;
					continue;
				}	

				uNumFrames = in_uNumFrames;
#if defined(AK_XBOX360)
				AKASSERT( (uNumFrames % 4) == 0 );
				__vector4 * AK_RESTRICT pfInChan = (__vector4 *) ( in_pAudioBufferIn->GetChannel(iChanIndex) + in_uFrameOffset );
				__vector4 * AK_RESTRICT pfOutChan = (__vector4 *) ( out_pfBufferOut );
				__vector4 * AK_RESTRICT pfEnd = (__vector4 *) ( out_pfBufferOut + uNumFrames );
				__vector4 vfGain = __vspltw( __lvlx( &fCombinedGain, 0 ), 0 );

				while ( uNumFrames )
				{
					*pfOutChan++ += vfGain * *pfInChan++;
					uNumFrames -= 4;
				}
#elif defined(__SPU__)
				AKASSERT( (uNumFrames % 4) == 0 );
				vec_float4 * AK_RESTRICT pfInChan = (vec_float4 *) ( in_pAudioBufferIn->GetChannel(iChanIndex) + in_uFrameOffset );
				vec_float4 * AK_RESTRICT pfOutChan = (vec_float4 *) ( out_pfBufferOut );
				vec_float4 vfGain = spu_splats( fCombinedGain );

				while ( uNumFrames )
				{
					*pfOutChan++ += vfGain * *pfInChan++;
					uNumFrames -= 4;
				}
#elif defined(AKSIMD_V4F32_SUPPORTED)
				AKASSERT( (uNumFrames % 4) == 0 );
				AKSIMD_V4F32 * AK_RESTRICT pfInChan = (AKSIMD_V4F32 *) ( in_pAudioBufferIn->GetChannel(iChanIndex) + in_uFrameOffset );
				AKSIMD_V4F32 * AK_RESTRICT pfOutChan = (AKSIMD_V4F32 *) ( out_pfBufferOut );
				AKSIMD_V4F32 vfGain = AKSIMD_SET_V4F32(fCombinedGain);

				while ( uNumFrames )
				{
					*pfOutChan = AKSIMD_MADD_V4F32(*pfInChan, vfGain, *pfOutChan);
					pfInChan++;
					pfOutChan++;
					uNumFrames -= 4;
				}
#elif defined AKSIMD_V2F32_SUPPORTED
				AKASSERT( (uNumFrames % 4) == 0 );
				AKSIMD_V2F32 * AK_RESTRICT pfInChan = (AKSIMD_V2F32 *) ( in_pAudioBufferIn->GetChannel(iChanIndex) + in_uFrameOffset );
				AKSIMD_V2F32 * AK_RESTRICT pfOutChan = (AKSIMD_V2F32 *) ( out_pfBufferOut );
				AKSIMD_V2F32 vfGain = AKSIMD_SET_V2F32(fCombinedGain);

				while ( uNumFrames )
				{
					*pfOutChan = AKSIMD_MADD_V2F32(*pfInChan, vfGain, *pfOutChan);
					pfInChan++;
					pfOutChan++;

					*pfOutChan = AKSIMD_MADD_V2F32(*pfInChan, vfGain, *pfOutChan);
					pfInChan++;
					pfOutChan++;

					uNumFrames -= 4;
				}
#else
				AkReal32 * AK_RESTRICT pfInChan = in_pAudioBufferIn->GetChannel(iChanIndex) + in_uFrameOffset;
				AkReal32 * AK_RESTRICT pfOutChan = out_pfBufferOut;
				while ( uNumFrames-- )
				{
					*pfOutChan++ += fCombinedGain * *pfInChan++;
				}
#endif
				// Mark as done
				uRemainingChannelMask = uRemainingChannelMask & ~uCurrentChanMask;
				++iChanIndex;
			}
			uCurrentChanMask<<=1;
		}
	}

} // namespace DSP
