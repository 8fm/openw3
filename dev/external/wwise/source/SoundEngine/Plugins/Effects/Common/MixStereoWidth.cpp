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

// Simple mix of 2 signals for stereo width

#include "MixStereoWidth.h"
#include <AK/Tools/Common/AkAssert.h>
#include <math.h>


#define MAXSTEREOWIDTH (180.f)
#define HALFPOWERGAIN (0.707106f)
// LR mix based on spread factor (constant power)
// 0 Stereo width corresponds to half power from each unit mix in each channel
// MAXSTEREOWIDTH maps full power to each discrete signal with no contribution from the other on each side
#define COMPUTELRMIX( __PrevStereoWidth__, __TargetStereoWidth__ )								 \
	AkReal32 fPrevGain1 = ((__PrevStereoWidth__)/MAXSTEREOWIDTH)*(1.f-HALFPOWERGAIN)+HALFPOWERGAIN;		 \
	AkReal32 fOneMinuseSquaredGain = 1.f-fPrevGain1*fPrevGain1;									 \
	AkReal32 fPrevGain2;																		 \
	if ( fOneMinuseSquaredGain > 0.f )															 \
		fPrevGain2 = sqrt(fOneMinuseSquaredGain);												 \
	else																						 \
		fPrevGain2 = 0.f;																		 \
	const AkReal32 fGain1 = ((__TargetStereoWidth__)/MAXSTEREOWIDTH)*(1.f-HALFPOWERGAIN)+HALFPOWERGAIN;			 \
	fOneMinuseSquaredGain = 1.f-fGain1*fGain1;													 \
	AkReal32 fGain2;																			 \
	if ( fOneMinuseSquaredGain > 0.f )															 \
		fGain2 = sqrt(fOneMinuseSquaredGain);													 \
	else																						 \
		fGain2 = 0.f;																			 \


namespace DSP
{
	// In-place processing routine on both channels
	void MixStereoWidth(AkReal32 * AK_RESTRICT io_pfIn1Out, 
						AkReal32 * AK_RESTRICT io_pfIn2Out, 
						AkUInt32 in_uNumFrames,
						AkReal32 in_fPrevStereoWidth, 
						AkReal32 in_fStereoWidth )
	{
		COMPUTELRMIX( in_fPrevStereoWidth, in_fStereoWidth );

		if ( (fGain1 == fPrevGain1) && (fGain2 == fPrevGain2) )
		{
			// No need for interpolation
			const AkReal32 * const pfEnd = io_pfIn1Out + in_uNumFrames;
			while ( io_pfIn1Out < pfEnd )
			{
				AkReal32 fIn1 = *io_pfIn1Out;
				AkReal32 fIn2 = *io_pfIn2Out;

				*io_pfIn1Out = fGain1 * fIn1 + fGain2 * fIn2;
				++io_pfIn1Out;
				*io_pfIn2Out = fGain2 * fIn1 + fGain1 * fIn2;
				++io_pfIn2Out;
			}
		}
		else
		{
			// Interpolate gains toward target
			const AkReal32 fGain1Inc = (fGain1-fPrevGain1)/in_uNumFrames;
			const AkReal32 fGain2Inc = (fGain2-fPrevGain2)/in_uNumFrames;
			const AkReal32 * const pfEnd = io_pfIn1Out + in_uNumFrames;
			while ( io_pfIn1Out < pfEnd )
			{
				AkReal32 fIn1 = *io_pfIn1Out;
				AkReal32 fIn2 = *io_pfIn2Out;

				*io_pfIn1Out = fPrevGain1 * fIn1 + fPrevGain2 * fIn2;
				*io_pfIn2Out = fPrevGain2 * fIn1 + fPrevGain1 * fIn2;
				fPrevGain1 += fGain1Inc;
				fPrevGain2 += fGain2Inc;
				++io_pfIn1Out;
				++io_pfIn2Out;
			}
		}
	}

		// Out-of-place processing routine 
	void MixStereoWidth(AkReal32 * AK_RESTRICT in_pfIn1, 
						AkReal32 * AK_RESTRICT in_pfIn2, 
						AkReal32 * AK_RESTRICT out_pfOut1, 
						AkReal32 * AK_RESTRICT out_pfOut2, 
						AkUInt32 in_uNumFrames,
						AkReal32 in_fPrevStereoWidth, 
						AkReal32 in_fStereoWidth )
	{
		COMPUTELRMIX( in_fPrevStereoWidth, in_fStereoWidth );

		if ( (fGain1 == fPrevGain1) && (fGain2 == fPrevGain2) )
		{
			// No need for interpolation
			const AkReal32 * const pfEnd = in_pfIn1 + in_uNumFrames;
			while ( in_pfIn1 < pfEnd )
			{
				AkReal32 fIn1 = *in_pfIn1++;
				AkReal32 fIn2 = *in_pfIn2++;

				*out_pfOut1++ = fGain1 * fIn1 + fGain2 * fIn2;
				*out_pfOut2++ = fGain2 * fIn1 + fGain1 * fIn2;
			}
		}
		else
		{
			// Interpolate gains toward target
			const AkReal32 fGain1Inc = (fGain1-fPrevGain1)/in_uNumFrames;
			const AkReal32 fGain2Inc = (fGain2-fPrevGain2)/in_uNumFrames;
			const AkReal32 * const pfEnd = in_pfIn1 + in_uNumFrames;
			while ( in_pfIn1 < pfEnd )
			{
				AkReal32 fIn1 = *in_pfIn1++;
				AkReal32 fIn2 = *in_pfIn2++;

				*out_pfOut1++ = fPrevGain1 * fIn1 + fPrevGain2 * fIn2;
				*out_pfOut2++ = fPrevGain2 * fIn1 + fPrevGain1 * fIn2;
				fPrevGain1 += fGain1Inc;
				fPrevGain2 += fGain2Inc;
			}
		}
	}
	
	// In-place processing routine
	void MixStereoWidth(AkAudioBuffer *io_pBuffer, 
						AkReal32 in_fPrevStereoWidth, 
						AkReal32 in_fStereoWidth )
	{
		const AkUInt32 uNumFrames = io_pBuffer->uValidFrames;
		AkUInt32 uChannelMask = io_pBuffer->GetChannelMask();
		if ( uChannelMask & AK_SPEAKER_SETUP_2_0 )  
		{
			// Front pair
			AkReal32 * pfLeft = io_pBuffer->GetChannel( 0 );
			AkReal32 * pfRight = io_pBuffer->GetChannel( 1 );
			MixStereoWidth( pfLeft, pfRight, uNumFrames, in_fPrevStereoWidth, in_fStereoWidth );
		}

		if ( uChannelMask & AK_SPEAKER_SETUP_REAR )  
		{
			AkUInt32 uBackChannelIndex = 2;
			if ( uChannelMask & AK_SPEAKER_FRONT_CENTER )
				uBackChannelIndex++;
			// Back pair
			{
				AkReal32 * pfLeft = io_pBuffer->GetChannel( uBackChannelIndex );
				AkReal32 * pfRight = io_pBuffer->GetChannel( uBackChannelIndex+1 );
				MixStereoWidth( pfLeft, pfRight, uNumFrames, in_fPrevStereoWidth, in_fStereoWidth );
			}

			if ( uChannelMask & AK_SPEAKER_SETUP_SIDE )  
			{
				AkUInt32 uChannelIndex = uBackChannelIndex + 2;
				
				// Side pair
				AkReal32 * pfLeft = io_pBuffer->GetChannel( uChannelIndex );
				AkReal32 * pfRight = io_pBuffer->GetChannel( uChannelIndex+1 );
				MixStereoWidth( pfLeft, pfRight, uNumFrames, in_fPrevStereoWidth, in_fStereoWidth );
			}
		}
	}

} // namespace DSP
