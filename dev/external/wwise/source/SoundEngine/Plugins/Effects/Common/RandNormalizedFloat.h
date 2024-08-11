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

#ifndef _AKRANDNORMFLOAT_H_
#define _AKRANDNORMFLOAT_H_

// Linear congruential method for generating pseudo-random number (float version)

#include <AK/SoundEngine/Common/AkTypes.h>

namespace DSP
{
	class RandNormFloat
	{
	public:
		void SetSeed( AkUInt32 in_uSeed )
		{
			uSeed = in_uSeed;
		}

		AkReal32 Tick( )
		{
			static const AkReal32 ONEOVERMAXRANDVAL	= ( 1.f / 0xFFFFFFFF ); // 2^32
			// Generate a (pseudo) random number (32 bits range)
			uSeed = (uSeed * 196314165) + 907633515;
			// Scale to normalized floating point range (0.f,~1.f)
			AkReal32 fRandVal = uSeed * ONEOVERMAXRANDVAL;
			// Output
			return fRandVal;
		}

	private:
		AkUInt32 uSeed;
	};
}

#endif // _AKRANDNORMFLOAT_H_