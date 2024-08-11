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

#ifndef _AK_ACCUMULATOR_H_
#define _AK_ACCUMULATOR_H_

#include <AK/SoundEngine/Common/AkTypes.h>

namespace DSP
{

	// Out of place processing routine
	void Accumulate(	   
		AkReal32 * in_pfInput1, 
		AkReal32 * in_pfInput2, 
		AkReal32 * out_pfOutput, 
		AkUInt32 in_uNumFrames );

	// In-place processing routine ( io_pfIn1Out == Input1 and output buffer )
	void Accumulate(	   
		AkReal32 * io_pfIn1Out, 
		AkReal32 * in_pfInput2, 
		AkUInt32 in_uNumFrames );

} // namespace DSP

#endif // _AK_ACCUMULATOR_H_
