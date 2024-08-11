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
// AkDownmix.h
// 
// Downmix recipes.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>

// Downmix structures; downmix equations are implemented in constructor and stored in structure.
// They all concern full band channels only. LFE needs to be handled separately.
namespace AkDownmix
{
	// Computes downmix volumes for a given set of AkSpeakerVolumes according to AC3 standards.
	// AkSpeakerVolumes is platform specific, hence downmix rules are fixed for a given platform.
	static void ComputeVolumes( 
		AkSpeakerVolumes & in_volumes,
		AkUInt32 in_uOutputConfig,
		AkReal32 out_fOutputVolumes[]		// Array of anonymous channel levels.
		)
	{
		// LFE must be excluded by caller.
		AKASSERT( ( in_uOutputConfig & AK_SPEAKER_LOW_FREQUENCY ) == 0 );
		switch ( in_uOutputConfig )
		{
		case AK_SPEAKER_SETUP_MONO:

			/** Former mixdown recipe
			out_fOutputVolumes[AK_IDX_SETUP_1_CENTER] = sqrtf( 
				(in_volumes.fFrontLeft*in_volumes.fFrontLeft) 
				+ (in_volumes.fFrontRight*in_volumes.fFrontRight) 
			#ifdef AK_LFECENTER
				+ (in_volumes.fCenter*in_volumes.fCenter)
			#endif
			#ifdef AK_REARCHANNELS
				+ 0.5f*( (in_volumes.fRearRight*in_volumes.fRearRight)+(in_volumes.fRearLeft*in_volumes.fRearLeft) ) 
			#endif
			);
			**/

			/// AC3
			{
				AkReal32 fLeft = in_volumes.fFrontLeft;
				AkReal32 fRight = in_volumes.fFrontRight;
				#ifdef AK_LFECENTER
					fLeft += ONE_OVER_SQRT_OF_TWO * in_volumes.fCenter;
					fRight += ONE_OVER_SQRT_OF_TWO * in_volumes.fCenter;
				#endif
				#ifdef AK_REARCHANNELS
					fLeft += ONE_OVER_SQRT_OF_TWO * in_volumes.fRearLeft;
					fRight += ONE_OVER_SQRT_OF_TWO * in_volumes.fRearRight;
				#endif
				#ifdef AK_71AUDIO
					fLeft += ONE_OVER_SQRT_OF_TWO * in_volumes.fSideLeft;
					fRight += ONE_OVER_SQRT_OF_TWO * in_volumes.fSideRight;
				#endif
				// Note: stereo to mono mixdown needs to be scaled in order to ensure that 
				// constant power stereo and full scale mono (center) yield the same value.
				out_fOutputVolumes[AK_IDX_SETUP_1_CENTER] = ONE_OVER_SQRT_OF_TWO * ( fLeft + fRight );
			}
			break;

		case AK_SPEAKER_SETUP_STEREO:

			/** Former mixdown recipe
			out_fOutputVolumes[AK_IDX_SETUP_2_LEFT] = sqrtf( (in_volumes.fFrontLeft*in_volumes.fFrontLeft) 
			#ifdef AK_LFECENTER
				+ 0.5f*(in_volumes.fCenter*in_volumes.fCenter) 
			#endif
			#ifdef AK_REARCHANNELS
				+ 0.5f*(in_volumes.fRearLeft*in_volumes.fRearLeft)
			#endif
			#ifdef AK_71AUDIO
				+ 0.5f*(in_volumes.fSideLeft*in_volumes.fSideLeft)
			#endif
			);
			out_fOutputVolumes[AK_IDX_SETUP_2_RIGHT] = sqrtf( (in_volumes.fFrontRight*in_volumes.fFrontRight) 
			#ifdef AK_LFECENTER
				+ 0.5f*(in_volumes.fCenter*in_volumes.fCenter)
			#endif
			#ifdef AK_REARCHANNELS
				+ 0.5f*(in_volumes.fRearRight*in_volumes.fRearRight)
			#endif
			#ifdef AK_71AUDIO
				+ 0.5f*(in_volumes.fSideRight*in_volumes.fSideRight)
			#endif
			);
			**/
			
			/// AC3
			out_fOutputVolumes[AK_IDX_SETUP_2_LEFT] = in_volumes.fFrontLeft;
			out_fOutputVolumes[AK_IDX_SETUP_2_RIGHT] = in_volumes.fFrontRight;
			#ifdef AK_LFECENTER
				out_fOutputVolumes[AK_IDX_SETUP_2_LEFT] += ONE_OVER_SQRT_OF_TWO * in_volumes.fCenter;
				out_fOutputVolumes[AK_IDX_SETUP_2_RIGHT] += ONE_OVER_SQRT_OF_TWO * in_volumes.fCenter;
			#endif
			#ifdef AK_REARCHANNELS
				out_fOutputVolumes[AK_IDX_SETUP_2_LEFT] += ONE_OVER_SQRT_OF_TWO * in_volumes.fRearLeft;
				out_fOutputVolumes[AK_IDX_SETUP_2_RIGHT] += ONE_OVER_SQRT_OF_TWO * in_volumes.fRearRight;
			#endif
			#ifdef AK_71AUDIO
				out_fOutputVolumes[AK_IDX_SETUP_2_LEFT] += ONE_OVER_SQRT_OF_TWO * in_volumes.fSideLeft;
				out_fOutputVolumes[AK_IDX_SETUP_2_RIGHT] += ONE_OVER_SQRT_OF_TWO * in_volumes.fSideRight;
			#endif
			break;

#ifdef AK_LFECENTER
		case AK_SPEAKER_SETUP_3STEREO:
			/// AC3
			out_fOutputVolumes[AK_IDX_SETUP_3_LEFT] = in_volumes.fFrontLeft;
			out_fOutputVolumes[AK_IDX_SETUP_3_RIGHT] = in_volumes.fFrontRight;
			out_fOutputVolumes[AK_IDX_SETUP_3_CENTER] = in_volumes.fCenter;
			#ifdef AK_REARCHANNELS
				out_fOutputVolumes[AK_IDX_SETUP_3_LEFT] += ONE_OVER_SQRT_OF_TWO * in_volumes.fRearLeft;
				out_fOutputVolumes[AK_IDX_SETUP_3_RIGHT] += ONE_OVER_SQRT_OF_TWO * in_volumes.fRearRight;
			#endif
			#ifdef AK_71AUDIO
				out_fOutputVolumes[AK_IDX_SETUP_3_LEFT] += ONE_OVER_SQRT_OF_TWO * in_volumes.fSideLeft;
				out_fOutputVolumes[AK_IDX_SETUP_3_RIGHT] += ONE_OVER_SQRT_OF_TWO * in_volumes.fSideRight;
			#endif			
			break;
#endif // AK_LFECENTER

#ifdef AK_REARCHANNELS
		case AK_SPEAKER_SETUP_4:
			/** Former mixdown recipe
			out_fOutputVolumes[AK_IDX_SETUP_4_FRONTLEFT] = sqrtf( (in_volumes.fFrontLeft*in_volumes.fFrontLeft) 
			#ifdef AK_LFECENTER
				+ 0.5f*( in_volumes.fCenter*in_volumes.fCenter ) 
			#endif
			);
			out_fOutputVolumes[AK_IDX_SETUP_4_FRONTRIGHT] = sqrtf( (in_volumes.fFrontRight*in_volumes.fFrontRight) 
			#ifdef AK_LFECENTER
				+ 0.5f*( in_volumes.fCenter*in_volumes.fCenter ) 
			#endif
			);
			out_fOutputVolumes[AK_IDX_SETUP_4_REARLEFT] = in_volumes.fRearLeft;
			out_fOutputVolumes[AK_IDX_SETUP_4_REARRIGHT] = in_volumes.fRearRight;
			**/

			/// AC3
			out_fOutputVolumes[AK_IDX_SETUP_4_FRONTLEFT] = in_volumes.fFrontLeft;
			out_fOutputVolumes[AK_IDX_SETUP_4_FRONTRIGHT] = in_volumes.fFrontRight;
			#ifdef AK_LFECENTER
				out_fOutputVolumes[AK_IDX_SETUP_4_FRONTLEFT] += ONE_OVER_SQRT_OF_TWO * in_volumes.fCenter;
				out_fOutputVolumes[AK_IDX_SETUP_4_FRONTRIGHT] += ONE_OVER_SQRT_OF_TWO * in_volumes.fCenter;
			#endif
			out_fOutputVolumes[AK_IDX_SETUP_4_REARLEFT] = in_volumes.fRearLeft;
			out_fOutputVolumes[AK_IDX_SETUP_4_REARRIGHT] = in_volumes.fRearRight;
			// 7.x->4.x: Collapse surround channels together. 
			// We add them with no scaling. The rationale behind this is two-fold:
			// 1) There should be no precedence of side over back channels (and vice-versa),
			// 2) If a 4.x to stereo conversion occurs further down the chain, rear channels must not be attenuated twice.
			#ifdef AK_71AUDIO
				out_fOutputVolumes[AK_IDX_SETUP_4_REARLEFT] += in_volumes.fSideLeft;
				out_fOutputVolumes[AK_IDX_SETUP_4_REARRIGHT] += in_volumes.fSideRight;
			#endif

			break;
#endif // AK_REARCHANNELS

#if defined(AK_REARCHANNELS) && defined(AK_LFECENTER)
		case AK_SPEAKER_SETUP_5:
			out_fOutputVolumes[AK_IDX_SETUP_5_FRONTLEFT] = in_volumes.fFrontLeft;
			out_fOutputVolumes[AK_IDX_SETUP_5_FRONTRIGHT] = in_volumes.fFrontRight;
			out_fOutputVolumes[AK_IDX_SETUP_5_CENTER] = in_volumes.fCenter;
			out_fOutputVolumes[AK_IDX_SETUP_5_REARLEFT] = in_volumes.fRearLeft;
			out_fOutputVolumes[AK_IDX_SETUP_5_REARRIGHT] = in_volumes.fRearRight;
			// 7.x->5.x: Collapse surround channels together. See note in 4.x case above.
			#ifdef AK_71AUDIO
				out_fOutputVolumes[AK_IDX_SETUP_5_REARLEFT] += in_volumes.fSideLeft;
				out_fOutputVolumes[AK_IDX_SETUP_5_REARRIGHT] += in_volumes.fSideRight;
			#endif
			break;
#endif // defined(AK_REARCHANNELS) && defined(AK_LFECENTER)

#ifdef AK_71AUDIO
		case AK_SPEAKER_SETUP_6:
			out_fOutputVolumes[AK_IDX_SETUP_6_FRONTLEFT] = in_volumes.fFrontLeft + ONE_OVER_SQRT_OF_TWO * in_volumes.fCenter;
			out_fOutputVolumes[AK_IDX_SETUP_6_FRONTRIGHT] = in_volumes.fFrontRight + ONE_OVER_SQRT_OF_TWO * in_volumes.fCenter;
			out_fOutputVolumes[AK_IDX_SETUP_6_REARLEFT] = in_volumes.fRearLeft;
			out_fOutputVolumes[AK_IDX_SETUP_6_REARRIGHT] = in_volumes.fRearRight;
			out_fOutputVolumes[AK_IDX_SETUP_6_SIDELEFT] = in_volumes.fSideLeft;
			out_fOutputVolumes[AK_IDX_SETUP_6_SIDERIGHT] = in_volumes.fSideRight;
			break;
		case AK_SPEAKER_SETUP_7:
			out_fOutputVolumes[AK_IDX_SETUP_7_FRONTLEFT] = in_volumes.fFrontLeft;
			out_fOutputVolumes[AK_IDX_SETUP_7_FRONTRIGHT] = in_volumes.fFrontRight;
			out_fOutputVolumes[AK_IDX_SETUP_7_CENTER] = in_volumes.fCenter;
			out_fOutputVolumes[AK_IDX_SETUP_7_REARLEFT] = in_volumes.fRearLeft;
			out_fOutputVolumes[AK_IDX_SETUP_7_REARRIGHT] = in_volumes.fRearRight;
			out_fOutputVolumes[AK_IDX_SETUP_7_SIDELEFT] = in_volumes.fSideLeft;
			out_fOutputVolumes[AK_IDX_SETUP_7_SIDERIGHT] = in_volumes.fSideRight;
			break;
#endif // AK_71AUDIO

		default:
			AKASSERT( !"Unsupported channel configuration" );
		}
	}
}
