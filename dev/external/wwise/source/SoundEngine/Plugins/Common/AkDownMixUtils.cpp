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
// Constant power implies sum_i=1_N(ChanGain^2) = 1
// E.g. using 5.1 configuration (CG = Constant power channel gain)
// CG^2 (L) + CG^2 (R) + (CenterLevel*CG)^2 (C) + (LFELevel*CG)^2 (LFE) + CG^2 (LS) + CG^2 (RS)

#include "AkDownMixUtils.h"
#include <AK/Tools/Common/AkAssert.h>
#include <math.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>

namespace DSP
{		
	void AkDownMix(	AkAudioBuffer * in_pBuffer, AkAudioBuffer * out_pDownMixBuffer, 
					AkReal32 in_fPrevInputCenterLevel, AkReal32 in_fInputCenterLevel, 
					AkReal32 in_fPrevInputLFELevel , AkReal32 in_fInputLFELevel )
	{
		// TODO: Make this code more flexible and consider output channel mask to take the right down-mixing decisions
		AkChannelMask uInChannelMask = in_pBuffer->GetChannelMask();
		AkUInt32 uNumFullBandChannels = AK::GetNumChannels( uInChannelMask & ~AK_SPEAKER_LOW_FREQUENCY );
		AkUInt32 uNumDownMixChannels = AkMin( uNumFullBandChannels, 2 );
		const AkUInt32 uNumFrames = in_pBuffer->uValidFrames;

		if (	uInChannelMask == AK_SPEAKER_SETUP_0POINT1 ||
				uInChannelMask == AK_SPEAKER_SETUP_MONO ||
				uInChannelMask == AK_SPEAKER_SETUP_STEREO )
		{
			// No need to compute volumes, just copy input channels to output buffer
			AkUInt32 uInNumChannels = AK::GetNumChannels( uInChannelMask );
			for ( AkUInt32 i = 0; i< uInNumChannels; i++ )
				AKPLATFORM::AkMemCpy( out_pDownMixBuffer->GetChannel(i), in_pBuffer->GetChannel(i), uNumFrames*sizeof(float) );
		}
		else
		{
			// Compute channel gains
			AkReal32 fUnscaled = (AkReal32)AK::GetNumChannels( uInChannelMask & AK_SPEAKER_SETUP_4_0 );
			AkReal32 fPrevCenter, fCenter;
			fPrevCenter = fCenter = 0.f;
			AkReal32 fPrevLFE, fLFE;
			fPrevLFE = fLFE = 0.f;
			if ( uInChannelMask & AK_SPEAKER_FRONT_CENTER )
			{
				if ( (uInChannelMask & AK_SPEAKER_SETUP_3_0) == AK_SPEAKER_SETUP_3_0 )
				{
					fPrevCenter = in_fPrevInputCenterLevel*in_fPrevInputCenterLevel;
					fCenter = in_fInputCenterLevel*in_fInputCenterLevel;
				}
				else
				{
					fUnscaled += 1.f;
				}
			}
			if ( uInChannelMask & AK_SPEAKER_LOW_FREQUENCY )
			{
				fPrevLFE = in_fPrevInputLFELevel*in_fPrevInputLFELevel;
				fLFE = in_fInputLFELevel*in_fInputLFELevel;
			}
			AkReal32 fPrevChanGain = sqrt( 1.f / ( fUnscaled + fPrevCenter + fPrevLFE ) );
			AkReal32 fChanGain = sqrt( 1.f / ( fUnscaled + fCenter + fLFE ) );

			const AkReal32 fChanGainInc = (fChanGain-fPrevChanGain)/uNumFrames;
			const AkReal32 fCenterGainInc = (fCenter-fPrevCenter)/uNumFrames;
			const AkReal32 fLFEGainInc = (fLFE-fPrevLFE)/uNumFrames;

			// TODO: SIMD vectortize all data loops

			// Start by copying L (and R) channels into mixdown buffer with their channel gains
			for ( AkUInt32 i = 0; i < uNumDownMixChannels; i++ )
			{
				AkReal32 * AK_RESTRICT pIn = (AkReal32 * AK_RESTRICT)in_pBuffer->GetChannel(i);
				AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)out_pDownMixBuffer->GetChannel(i);
				AkReal32 fCurChanGain = fPrevChanGain;
				for ( AkUInt32 i = 0; i < uNumFrames; i++ )
				{
					*pOut++ = fCurChanGain * *pIn++;
					fCurChanGain += fChanGainInc;
				}
			}

			// Mix in true center (i.e. not mono) channel in Left and Right channels
			if ( (uInChannelMask & AK_SPEAKER_SETUP_3_0) == AK_SPEAKER_SETUP_3_0 )
			{		
				for ( AkUInt32 i = 0; i < 2; i++ )
				{
					AkReal32 * AK_RESTRICT pIn = (AkReal32 * AK_RESTRICT)in_pBuffer->GetChannel(2);
					AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)out_pDownMixBuffer->GetChannel(i);
					AkReal32 fCurCenterGain = fPrevCenter;	
					for ( AkUInt32 i = 0; i < uNumFrames; i++ )
					{
						*pOut++ += fCurCenterGain * *pIn++;
						fCurCenterGain += fCenterGainInc;
					}
				}
			}

			// Mix in back channels
			if ( uInChannelMask & AK_SPEAKER_SETUP_REAR )
			{
				AKASSERT( uNumDownMixChannels == 2 );
				AkUInt32 uRearChannelOffset = uInChannelMask & AK_SPEAKER_FRONT_CENTER ? 3 : 2;
				for ( AkUInt32 i = 0; i < 2; i++ )
				{
					AkReal32 * AK_RESTRICT pIn = (AkReal32 * AK_RESTRICT)in_pBuffer->GetChannel(i+uRearChannelOffset);
					AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)out_pDownMixBuffer->GetChannel(i);
					AkReal32 fCurChanGain = fPrevChanGain;
					for ( AkUInt32 i = 0; i < uNumFrames; i++ )
					{
						*pOut++ += fCurChanGain * *pIn++;
						fCurChanGain += fChanGainInc;
					}
				}

				// Mix in side channels
				if ( uInChannelMask & AK_SPEAKER_SETUP_SIDE )
				{
					AkUInt32 uChannelOffset = uRearChannelOffset + 2;
					for ( AkUInt32 i = 0; i < 2; i++ )
					{
						AkReal32 * AK_RESTRICT pIn = (AkReal32 * AK_RESTRICT)in_pBuffer->GetChannel(i+uChannelOffset);
						AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)out_pDownMixBuffer->GetChannel(i);
						AkReal32 fCurChanGain = fPrevChanGain;
						for ( AkUInt32 i = 0; i < uNumFrames; i++ )
						{
							*pOut++ += fCurChanGain * *pIn++;
							fCurChanGain += fChanGainInc;
						}
					}
				}
			}

			// Mix in LFE channel
			if ( uInChannelMask & AK_SPEAKER_LOW_FREQUENCY )
			{
				// Mix in LFE in every down-mix channel at its own level. 	
				for ( AkUInt32 i = 0; i < uNumDownMixChannels; i++ )
				{
					AkReal32 * AK_RESTRICT pLFE = (AkReal32 * AK_RESTRICT)in_pBuffer->GetLFE();	
					AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)out_pDownMixBuffer->GetChannel(i);
					AkReal32 fCurLFEGain = fPrevLFE;
					for ( AkUInt32 i = 0; i < uNumFrames; i++ )
					{
						*pOut++ += fCurLFEGain * *pLFE++;
						fCurLFEGain += fLFEGainInc;
					}
				}	
			}
		}
	}

} // namespace DSP
