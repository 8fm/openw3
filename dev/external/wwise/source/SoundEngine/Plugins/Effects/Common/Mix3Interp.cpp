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

// Simple mix of 3 signals with interpolating volume
// To be used on mono signals, create as many instances as there are channels if need be

#include "Mix3Interp.h"
#include <AK/Tools/Common/AkAssert.h>
#include <AK/SoundEngine/Common/AkSimd.h>

namespace DSP
{

	// Out of place processing routine
	void Mix3Interp( 
		AkReal32 * AK_RESTRICT in_pfInput1, 
		AkReal32 * AK_RESTRICT in_pfInput2, 
		AkReal32 * AK_RESTRICT in_pfInput3, 
		AkReal32 * AK_RESTRICT out_pfOutput, 
		AkReal32 in_fCurrentGain1, 
		AkReal32 in_fTargetGain1, 
		AkReal32 in_fCurrentGain2, 
		AkReal32 in_fTargetGain2,
		AkReal32 in_fCurrentGain3, 
		AkReal32 in_fTargetGain3,
		AkUInt32 in_uNumFrames )
	{
		// Friendly reminder for RESTRICT constraints
		AKASSERT( !((out_pfOutput == in_pfInput1) ||  (out_pfOutput == in_pfInput2)) );
		if ( (in_fTargetGain1 == in_fCurrentGain1) && 
			(in_fTargetGain2 == in_fCurrentGain2) &&
			(in_fTargetGain3 == in_fCurrentGain3) )
		{
			// No need for interpolation
			const AkReal32 * const pfEnd = out_pfOutput + in_uNumFrames;
#if defined(AKSIMD_V4F32_SUPPORTED)
			AKSIMD_V4F32 vInput1, vInput2, vInput3;
			AKSIMD_V4F32 vGain1 = AKSIMD_SET_V4F32(in_fCurrentGain1);
			AKSIMD_V4F32 vGain2 = AKSIMD_SET_V4F32(in_fCurrentGain2);
			AKSIMD_V4F32 vGain3 = AKSIMD_SET_V4F32(in_fCurrentGain3);

			while ( out_pfOutput < pfEnd )
			{
				vInput1 = AKSIMD_LOAD_V4F32(in_pfInput1);
				vInput2 = AKSIMD_LOAD_V4F32(in_pfInput2);
				vInput3 = AKSIMD_LOAD_V4F32(in_pfInput3);
				vInput1 = AKSIMD_MUL_V4F32(vInput1, vGain1);
				vInput2 = AKSIMD_MUL_V4F32(vInput2, vGain2);
				vInput3 = AKSIMD_MUL_V4F32(vInput3, vGain3);
				vInput1 = AKSIMD_ADD_V4F32(vInput1, vInput2);
				vInput1 = AKSIMD_ADD_V4F32(vInput1, vInput3);
				AKSIMD_STORE_V4F32(out_pfOutput, vInput1);
				in_pfInput1 += 4;
				in_pfInput2 += 4;
				in_pfInput3 += 4;
				out_pfOutput += 4;
			}
#elif defined(AKSIMD_V2F32_SUPPORTED)
			AKSIMD_V2F32 vInput1, vInput2, vInput3;
			AKSIMD_V2F32 vGain1 = AKSIMD_SET_V2F32(in_fCurrentGain1);
			AKSIMD_V2F32 vGain2 = AKSIMD_SET_V2F32(in_fCurrentGain2);
			AKSIMD_V2F32 vGain3 = AKSIMD_SET_V2F32(in_fCurrentGain3);

			while ( out_pfOutput < pfEnd )
			{
				vInput1 = AKSIMD_LOAD_V2F32(in_pfInput1);
				vInput2 = AKSIMD_LOAD_V2F32(in_pfInput2);
				vInput3 = AKSIMD_LOAD_V2F32(in_pfInput3);
				vInput1 = AKSIMD_MUL_V2F32(vInput1, vGain1);
				vInput2 = AKSIMD_MUL_V2F32(vInput2, vGain2);
				vInput3 = AKSIMD_MUL_V2F32(vInput3, vGain3);
				vInput1 = AKSIMD_ADD_V2F32(vInput1, vInput2);
				vInput1 = AKSIMD_ADD_V2F32(vInput1, vInput3);
				AKSIMD_STORE_V2F32(out_pfOutput, vInput1);
				in_pfInput1 +=2;
				in_pfInput2 +=2;
				in_pfInput3 +=2;
				out_pfOutput +=2;
			}
#else
			while ( out_pfOutput < pfEnd )
			{
				*out_pfOutput++ = in_fTargetGain1 * *in_pfInput1++ + in_fTargetGain2 * *in_pfInput2++ + in_fTargetGain3 * *in_pfInput3++;
			}
#endif
		}
		else
		{
			// Interpolate gains toward target
			const AkReal32 fGain1Inc = (in_fTargetGain1-in_fCurrentGain1)/in_uNumFrames;
			const AkReal32 fGain2Inc = (in_fTargetGain2-in_fCurrentGain2)/in_uNumFrames;
			const AkReal32 fGain3Inc = (in_fTargetGain3-in_fCurrentGain3)/in_uNumFrames;
			const AkReal32 * const pfEnd = out_pfOutput + in_uNumFrames;
			while ( out_pfOutput < pfEnd )
			{
				*out_pfOutput++ = in_fCurrentGain1 * *in_pfInput1++ + in_fCurrentGain2 * *in_pfInput2++ + in_fCurrentGain3 * *in_pfInput3++;
				in_fCurrentGain1 += fGain1Inc;
				in_fCurrentGain2 += fGain2Inc;
				in_fCurrentGain3 += fGain3Inc;
			}
		}
	}

	// In-place processing routine ( io_pfIn1Out == Input1 and output buffer )
	void Mix3Interp( 
		AkReal32 * AK_RESTRICT io_pfIn1Out, 
		AkReal32 * AK_RESTRICT in_pfInput2,
		AkReal32 * AK_RESTRICT in_pfInput3,
		AkReal32 in_fCurrentGain1, 
		AkReal32 in_fTargetGain1, 
		AkReal32 in_fCurrentGain2, 
		AkReal32 in_fTargetGain2, 
		AkReal32 in_fCurrentGain3, 
		AkReal32 in_fTargetGain3, 
		AkUInt32 in_uNumFrames )
	{
		if ( (in_fTargetGain1 == in_fCurrentGain1) && 
			(in_fTargetGain2 == in_fCurrentGain2) &&
			(in_fTargetGain3 == in_fCurrentGain3) )
		{
			// No need for interpolation

			const AkReal32 * const pfEnd = io_pfIn1Out + in_uNumFrames;
#if defined(AKSIMD_V4F32_SUPPORTED)
			AKSIMD_V4F32 vInput1, vInput2, vInput3;
			AKSIMD_V4F32 vGain1 = AKSIMD_SET_V4F32(in_fCurrentGain1);
			AKSIMD_V4F32 vGain2 = AKSIMD_SET_V4F32(in_fCurrentGain2);
			AKSIMD_V4F32 vGain3 = AKSIMD_SET_V4F32(in_fCurrentGain3);

			while ( io_pfIn1Out < pfEnd )
			{
				vInput1 = AKSIMD_LOAD_V4F32(io_pfIn1Out);
				vInput2 = AKSIMD_LOAD_V4F32(in_pfInput2);
				vInput3 = AKSIMD_LOAD_V4F32(in_pfInput3);
				vInput1 = AKSIMD_MUL_V4F32(vInput1, vGain1);
				vInput2 = AKSIMD_MUL_V4F32(vInput2, vGain2);
				vInput3 = AKSIMD_MUL_V4F32(vInput3, vGain3);
				vInput1 = AKSIMD_ADD_V4F32(vInput1, vInput2);
				vInput1 = AKSIMD_ADD_V4F32(vInput1, vInput3);
				AKSIMD_STORE_V4F32(io_pfIn1Out, vInput1);
				in_pfInput2 += 4;
				in_pfInput3 += 4;
				io_pfIn1Out += 4;
			}
#elif defined(AKSIMD_V2F32_SUPPORTED)
			AKSIMD_V2F32 vInput1, vInput2, vInput3;
			AKSIMD_V2F32 vGain1 = AKSIMD_SET_V2F32(in_fCurrentGain1);
			AKSIMD_V2F32 vGain2 = AKSIMD_SET_V2F32(in_fCurrentGain2);
			AKSIMD_V2F32 vGain3 = AKSIMD_SET_V2F32(in_fCurrentGain3);

			while ( io_pfIn1Out < pfEnd )
			{
				vInput1 = AKSIMD_LOAD_V2F32(io_pfIn1Out);
				vInput2 = AKSIMD_LOAD_V2F32(in_pfInput2);
				vInput3 = AKSIMD_LOAD_V2F32(in_pfInput3);
				vInput1 = AKSIMD_MUL_V2F32(vInput1, vGain1);
				vInput2 = AKSIMD_MUL_V2F32(vInput2, vGain2);
				vInput3 = AKSIMD_MUL_V2F32(vInput3, vGain3);
				vInput1 = AKSIMD_ADD_V2F32(vInput1, vInput2);
				vInput1 = AKSIMD_ADD_V2F32(vInput1, vInput3);
				AKSIMD_STORE_V2F32(io_pfIn1Out, vInput1);
				in_pfInput2 +=2 ;
				in_pfInput3 +=2 ;
				io_pfIn1Out +=2 ;
			}
#else
			while ( io_pfIn1Out < pfEnd )
			{
				*io_pfIn1Out = in_fTargetGain1 * *io_pfIn1Out + in_fTargetGain2 * *in_pfInput2++ + in_fTargetGain3 * *in_pfInput3++;
				++io_pfIn1Out;
			}
#endif
		}
		else
		{
			// Interpolate gains toward target
			const AkReal32 fGain1Inc = (in_fTargetGain1-in_fCurrentGain1)/in_uNumFrames;
			const AkReal32 fGain2Inc = (in_fTargetGain2-in_fCurrentGain2)/in_uNumFrames;
			const AkReal32 fGain3Inc = (in_fTargetGain3-in_fCurrentGain3)/in_uNumFrames;
			const AkReal32 * const pfEnd = io_pfIn1Out + in_uNumFrames;
			while ( io_pfIn1Out < pfEnd )
			{
				*io_pfIn1Out = in_fCurrentGain1 * *io_pfIn1Out + in_fCurrentGain2 * *in_pfInput2++ + in_fCurrentGain3 * *in_pfInput3++;
				in_fCurrentGain1 += fGain1Inc;
				in_fCurrentGain2 += fGain2Inc;
				in_fCurrentGain3 += fGain3Inc;
				++io_pfIn1Out;
			}
		}
	}

} // namespace DSP
