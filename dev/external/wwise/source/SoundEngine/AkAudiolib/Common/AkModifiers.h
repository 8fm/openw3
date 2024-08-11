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
// AkModifiers.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _MODIFIERS_H_
#define _MODIFIERS_H_

#include "AkRandom.h"
#include "AkParameters.h"

// Range modifier storage class

class RandomizerModifier
{
public:
	template <class T_Type>
	static T_Type GetMod( RANGED_MODIFIERS<T_Type>& in_rModifier )
	{
		T_Type result = 0; 
		T_Type mod = in_rModifier.m_max - in_rModifier.m_min;
		if( mod )
		{
			AkReal64 fZeroOneRandom = (AkReal64)AKRANDOM::AkRandom() / AKRANDOM::AK_RANDOM_MAX;
			result = (T_Type)( ( fZeroOneRandom * mod ) + 0.5 ); //+0.5 to round
		}
		return result + in_rModifier.m_min;
	}

	static AkReal32 GetMod( RANGED_MODIFIERS<AkReal32>& in_rModifier )
	{
		AkReal32 result = 0; 
		AkReal32 mod = in_rModifier.m_max - in_rModifier.m_min;
		if( mod )
		{
			AkReal64 fZeroOneRandom = (AkReal64)AKRANDOM::AkRandom() / AKRANDOM::AK_RANDOM_MAX;
			result = (AkReal32)( ( fZeroOneRandom * mod ));
		}
		return result + in_rModifier.m_min;
	}

	template <class T_Type>
	static inline T_Type GetModValue( RANGED_PARAMETER<T_Type>& in_rModifier )
	{
		return in_rModifier.m_base + GetMod( in_rModifier.m_mod );
	}

	template <class T_Type>
	static inline T_Type GetBaseValue( RANGED_PARAMETER<T_Type>& in_rModifier )
	{
		return in_rModifier.m_base;
	}

	template <class T_Type>
	static inline void SetModValue( RANGED_PARAMETER<T_Type>& in_rModifier, T_Type in_MidValue, T_Type in_min = 0, T_Type in_max = 0 )
	{
		in_rModifier.m_base = in_MidValue;
		in_rModifier.m_mod.m_min = in_min;
		in_rModifier.m_mod.m_max = in_max;
	}
};

#endif
