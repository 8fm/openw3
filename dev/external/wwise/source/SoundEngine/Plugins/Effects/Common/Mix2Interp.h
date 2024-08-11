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

#ifndef _AKMIX2INTERP_H_
#define _AKMIX2INTERP_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "SPUInline.h"

namespace DSP
{

	// Out of place processing routine
	SPUStatic void Mix2Interp(	   
		AkReal32 * AK_RESTRICT in_pfInput1, 
		AkReal32 * AK_RESTRICT in_pfInput2, 
		AkReal32 * AK_RESTRICT out_pfOutput, 
		AkReal32 in_fCurrentGain1, 
		AkReal32 in_fTargetGain1, 
		AkReal32 in_fCurrentGain2, 
		AkReal32 in_fTargetGain2, 
		AkUInt32 in_uNumFrames );

	// In-place processing routine ( io_pfIn1Out == Input1 and output buffer )
	SPUStatic void Mix2Interp(	   
		AkReal32 * AK_RESTRICT io_pfIn1Out, 
		AkReal32 * AK_RESTRICT in_pfInput2, 
		AkReal32 in_fCurrentGain1, 
		AkReal32 in_fTargetGain1, 
		AkReal32 in_fCurrentGain2, 
		AkReal32 in_fTargetGain2, 
		AkUInt32 in_uNumFrames );

	// In-place processing routine ( io_pfIn1Out == Input1 and output buffer ) 
	// Note: without vectorization constraints on uNumframes for non-performance critical applications
	// Assumes that gain is not changing over time
	SPUStatic void Mix2Interp (
		AkReal32 * AK_RESTRICT io_pfIn1Out, 
		AkReal32 * AK_RESTRICT in_pfInput2, 
		const AkReal32 in_fGain1, 
		const AkReal32 in_fGain2, 
		AkUInt32 in_uNumFrames );

} // namespace DSP

#endif // _AKMIX2INTERP_H_
