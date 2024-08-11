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
// AkRandom.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _RANDOM_H_
#define _RANDOM_H_

/////////////////////////////////////////////////////////
// NOTE:
// Might have to implement the AkRandom on each platform
/////////////////////////////////////////////////////////

#include <time.h>
#include <stdlib.h>

namespace AKRANDOM
{	
	static const AkInt AK_RANDOM_MAX = 0x7FFF;
	static const AkInt AK_RANDOM_MAX_30_BITS = 0x3FFFFFFF;

#if defined AK_WIN
	//Since the Windows version is what is used in DLLs, use the real CRT.  It is the same maths as below.
	inline void AkRandomInit(AkUInt32 in_Seed = 0)
	{
		if (in_Seed == 0)
			srand((AkUInt32)time(NULL));
		else
			srand(in_Seed);
	}

	inline AkInt AkRandom()
	{
		return rand();
	}
#else
	extern AkUInt32 g_uiRandom;
	
	inline void AkRandomInit(AkUInt32 in_Seed = 0)
	{
		if (in_Seed == 0)
		{
#ifndef AK_3DS
			g_uiRandom = (AkUInt32)time( NULL );
#else
		    nn::fnd::DateTime now = nn::fnd::DateTime::GetNow();
			g_uiRandom = (AkUInt32)now.GetMilliSecond();
#endif
		}
		else
			g_uiRandom = in_Seed;
	}

	inline AkInt AkRandom()
	{
		//Taken from rand.c in the Windows CRT.
		return ((g_uiRandom = g_uiRandom * 214013L	+ 2531011L) >> 16) & AK_RANDOM_MAX;
	}
#endif

	inline AkInt AkRandom_30_bits()
	{
		AkInt randValue = AKRANDOM::AkRandom();//returns 0 to RAND_MAX (32767)
		randValue = randValue << 15;
		randValue += AKRANDOM::AkRandom();
		return randValue;// return value from 0 to AK_RANDOM_MAX_30_BITS
	}
}

#endif
