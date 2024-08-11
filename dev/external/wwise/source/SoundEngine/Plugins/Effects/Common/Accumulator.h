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

// Accumulate (+=) signal into output buffer
// To be used on mono signals, create as many instances as there are channels if need be

#ifndef _AKACCUMULATOR_H_
#define _AKACCUMULATOR_H_

#include <AK/SoundEngine/Common/AkTypes.h>

namespace DSP
{
	AkForceInline static void Accumulator(	AkReal32 * AK_RESTRICT in_pfInput, 
											AkReal32 * AK_RESTRICT io_pfOutput, 
											AkUInt32 in_uNumFrames )
	{
		const AkReal32 * const pfEnd = in_pfInput + in_uNumFrames;
		while ( in_pfInput < pfEnd )
		{
			*io_pfOutput++ += *in_pfInput++;
		}
	}

} // namespace DSP


#endif // _AKACCUMULATOR_H_
