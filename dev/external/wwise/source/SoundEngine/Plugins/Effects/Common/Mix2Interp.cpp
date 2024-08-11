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

// Simple mix of 2 signals with interpolating volume
// Can be used as wet/dry mix
// To be used on mono signals, create as many instances as there are channels if need be

#include "Mix2Interp.h"
#include <AK/Tools/Common/AkAssert.h>
#include <AK/SoundEngine/Common/AkSimd.h>

#if defined(AKSIMD_V2F32_SUPPORTED)
#define OOP_ITERATION \
	vIn1 = *pvInput1;	\
	vIn2 = *pvInput2;	\
	vOut = AKSIMD_MADD_V2F32( vfTargetGain1, vIn1, AKSIMD_MUL_V2F32( vfTargetGain2, vIn2 ) );	\
	AKSIMD_STORE_V2F32( (AkReal32*)pvOutput, vOut );	\
	pvInput1++;	\
	pvInput2++;	\
	pvOutput++;

#define IP_ITERATION \
	vIn1 = *pvIn1Out;	\
	vIn2 = *pvInput2;	\
	vOut = AKSIMD_MADD_V2F32( vfTargetGain1, vIn1, AKSIMD_MUL_V2F32( vfTargetGain2, vIn2 ) );	\
	AKSIMD_STORE_V2F32( (AkReal32*)pvIn1Out, vOut );	\
	pvInput2++;	\
	pvIn1Out++;
#else
#define OOP_ITERATION \
	vIn1 = AKSIMD_LOAD_V4F32( pvInput1 );	\
	vIn2 = AKSIMD_LOAD_V4F32( pvInput2 );	\
	vOut = AKSIMD_MADD_V4F32( vfTargetGain1, vIn1, AKSIMD_MUL_V4F32( vfTargetGain2, vIn2 ) );	\
	AKSIMD_STORE_V4F32( (AkReal32*)pvOutput, vOut );	\
	pvInput1++;	\
	pvInput2++;	\
	pvOutput++;

#define IP_ITERATION \
	vIn1 = AKSIMD_LOAD_V4F32( pvIn1Out );	\
	vIn2 = AKSIMD_LOAD_V4F32( pvInput2 );	\
	vOut = AKSIMD_MADD_V4F32( vfTargetGain1, vIn1, AKSIMD_MUL_V4F32( vfTargetGain2, vIn2 ) );	\
	AKSIMD_STORE_V4F32( pvIn1Out, vOut );	\
	pvInput2++;	\
	pvIn1Out++;
#endif


namespace DSP
{

	// Out of place processing routine
	void Mix2Interp(	   
		AkReal32 * AK_RESTRICT in_pfInput1, 
		AkReal32 * AK_RESTRICT in_pfInput2, 
		AkReal32 * AK_RESTRICT out_pfOutput, 
		AkReal32 in_fCurrentGain1, 
		AkReal32 in_fTargetGain1, 
		AkReal32 in_fCurrentGain2, 
		AkReal32 in_fTargetGain2, 
		AkUInt32 in_uNumFrames )
	{
		// Friendly reminder for RESTRICT constraints
		AKASSERT( !((out_pfOutput == in_pfInput1) ||  (out_pfOutput == in_pfInput2)) );
		if ( (in_fTargetGain1 == in_fCurrentGain1) && (in_fTargetGain2 == in_fCurrentGain2) )
		{
#if defined(AKSIMD_V2F32_SUPPORTED)
			// No need for interpolation. Process 8 iterations of 2 samples each loop to optimize.
			AKASSERT( (in_uNumFrames % 16) == 0 );
			AKSIMD_V2F32 * AK_RESTRICT pvInput1 = (AKSIMD_V2F32 * AK_RESTRICT) in_pfInput1;
			AKSIMD_V2F32 * AK_RESTRICT pvInput2 = (AKSIMD_V2F32 * AK_RESTRICT) in_pfInput2;
			AKSIMD_V2F32 * AK_RESTRICT pvOutput = (AKSIMD_V2F32 * AK_RESTRICT) out_pfOutput;
			const AKSIMD_V2F32 * pvEnd = (AKSIMD_V2F32 *) ( out_pfOutput + in_uNumFrames );
			const AKSIMD_V2F32 vfTargetGain1 = AKSIMD_SET_V2F32( in_fTargetGain1 );
			const AKSIMD_V2F32 vfTargetGain2 = AKSIMD_SET_V2F32( in_fTargetGain2 );

			AKSIMD_V2F32 vIn1, vIn2, vOut;
			while ( pvOutput < pvEnd )
			{
				OOP_ITERATION;
				OOP_ITERATION;
				OOP_ITERATION;
				OOP_ITERATION;
				OOP_ITERATION;
				OOP_ITERATION;
				OOP_ITERATION;
				OOP_ITERATION;
			}
#else
			// No need for interpolation.  Process 8 iterations of 4 samples each loop to optimize.
			AKASSERT( (in_uNumFrames % 32) == 0 );
			AKSIMD_V4F32 * AK_RESTRICT pvInput1 = (AKSIMD_V4F32 * AK_RESTRICT) in_pfInput1;
			AKSIMD_V4F32 * AK_RESTRICT pvInput2 = (AKSIMD_V4F32 * AK_RESTRICT) in_pfInput2;
			AKSIMD_V4F32 * AK_RESTRICT pvOutput = (AKSIMD_V4F32 * AK_RESTRICT) out_pfOutput;
			const AKSIMD_V4F32 * pvEnd = (AKSIMD_V4F32 *) ( out_pfOutput + in_uNumFrames );
			const AKSIMD_V4F32 vfTargetGain1 = AKSIMD_LOAD1_V4F32( in_fTargetGain1 );
			const AKSIMD_V4F32 vfTargetGain2 = AKSIMD_LOAD1_V4F32( in_fTargetGain2 );

			AKSIMD_V4F32 vIn1, vIn2, vOut;
			while ( pvOutput < pvEnd )
			{
				OOP_ITERATION;
				OOP_ITERATION;
				OOP_ITERATION;
				OOP_ITERATION;				
				OOP_ITERATION;
				OOP_ITERATION;
				OOP_ITERATION;
				OOP_ITERATION;
			}
#endif
		}
		else
		{
			// Interpolate gains toward target
			const AkReal32 fGain1Inc = (in_fTargetGain1-in_fCurrentGain1)/in_uNumFrames;
			const AkReal32 fGain2Inc = (in_fTargetGain2-in_fCurrentGain2)/in_uNumFrames;
			const AkReal32 * const pfEnd = out_pfOutput + in_uNumFrames;
			while ( out_pfOutput < pfEnd )
			{
				*out_pfOutput++ = in_fCurrentGain1 * *in_pfInput1++ + in_fCurrentGain2 * *in_pfInput2++;
				in_fCurrentGain1 += fGain1Inc;
				in_fCurrentGain2 += fGain2Inc;
			}
		}
	}

	// In-place processing routine ( io_pfIn1Out == Input1 and output buffer )
	void Mix2Interp( 
		AkReal32 * AK_RESTRICT io_pfIn1Out, 
		AkReal32 * AK_RESTRICT in_pfInput2, 
		AkReal32 in_fCurrentGain1, 
		AkReal32 in_fTargetGain1, 
		AkReal32 in_fCurrentGain2, 
		AkReal32 in_fTargetGain2, 
		AkUInt32 in_uNumFrames )
	{
		if ( (in_fTargetGain1 == in_fCurrentGain1) && (in_fTargetGain2 == in_fCurrentGain2) )
		{
#if defined(AKSIMD_V2F32_SUPPORTED)
			// No need for interpolation
			AKASSERT( (in_uNumFrames % 16) == 0 );
			AKSIMD_V2F32 * AK_RESTRICT pvIn1Out = (AKSIMD_V2F32 * AK_RESTRICT) io_pfIn1Out;
			AKSIMD_V2F32 * AK_RESTRICT pvInput2 = (AKSIMD_V2F32 * AK_RESTRICT) in_pfInput2;
			const AKSIMD_V2F32 * pvEnd = (AKSIMD_V2F32 *) ( io_pfIn1Out + in_uNumFrames );
			const AKSIMD_V2F32 vfTargetGain1 = AKSIMD_SET_V2F32( in_fTargetGain1 );
			const AKSIMD_V2F32 vfTargetGain2 = AKSIMD_SET_V2F32( in_fTargetGain2 );

			AKSIMD_V2F32 vIn1, vIn2, vOut;
			while ( pvIn1Out < pvEnd )
			{
				IP_ITERATION;
				IP_ITERATION;
				IP_ITERATION;
				IP_ITERATION;
				IP_ITERATION;
				IP_ITERATION;
				IP_ITERATION;
				IP_ITERATION;
			}
#else
			// No need for interpolation. Process 8 iterations of 4 samples each loop to optimize.
			AKASSERT( (in_uNumFrames % 32) == 0 );	
			AKSIMD_V4F32 * AK_RESTRICT pvIn1Out = (AKSIMD_V4F32 * AK_RESTRICT) io_pfIn1Out;
			AKSIMD_V4F32 * AK_RESTRICT pvInput2 = (AKSIMD_V4F32 * AK_RESTRICT) in_pfInput2;
			const AKSIMD_V4F32 * pvEnd = (AKSIMD_V4F32 *) ( io_pfIn1Out + in_uNumFrames );
			const AKSIMD_V4F32 vfTargetGain1 = AKSIMD_LOAD1_V4F32( in_fTargetGain1 );
			const AKSIMD_V4F32 vfTargetGain2 = AKSIMD_LOAD1_V4F32( in_fTargetGain2 );
			
			AKSIMD_V4F32 vIn1, vIn2, vOut;
			while ( pvIn1Out < pvEnd )
			{
				IP_ITERATION;
				IP_ITERATION;
				IP_ITERATION;
				IP_ITERATION;
				IP_ITERATION;
				IP_ITERATION;
				IP_ITERATION;
				IP_ITERATION;
			}
#endif
		}
		else
		{
			// Interpolate gains toward target
			const AkReal32 fGain1Inc = (in_fTargetGain1-in_fCurrentGain1)/in_uNumFrames;
			const AkReal32 fGain2Inc = (in_fTargetGain2-in_fCurrentGain2)/in_uNumFrames;
			const AkReal32 * const pfEnd = io_pfIn1Out + in_uNumFrames;
			while ( io_pfIn1Out < pfEnd )
			{
				*io_pfIn1Out = in_fCurrentGain1 * *io_pfIn1Out + in_fCurrentGain2 * *in_pfInput2++;
				in_fCurrentGain1 += fGain1Inc;
				in_fCurrentGain2 += fGain2Inc;
				++io_pfIn1Out;
			}
		}
	}

	// In-place processing routine ( io_pfIn1Out == Input1 and output buffer ) 
	// Note: without vectorization constraints on uNumframes for non-performance critical applications
	// Assumes that gain is not changing over time
	void Mix2Interp (
		AkReal32 * AK_RESTRICT io_pfIn1Out, 
		AkReal32 * AK_RESTRICT in_pfInput2, 
		const AkReal32 in_fGain1, 
		const AkReal32 in_fGain2, 
		AkUInt32 in_uNumFrames )
	{
		const AkReal32 * const pfEnd = io_pfIn1Out + in_uNumFrames;
		while ( io_pfIn1Out < pfEnd )
		{
			*io_pfIn1Out = in_fGain1 * *io_pfIn1Out + in_fGain2 * *in_pfInput2++;
			++io_pfIn1Out;
		}
	}


} // namespace DSP
