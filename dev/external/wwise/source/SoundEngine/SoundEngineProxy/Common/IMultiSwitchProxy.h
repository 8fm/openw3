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

#pragma once

#ifndef AK_OPTIMIZED

#include "IObjectProxy.h"
#include "AkParameters.h"
#include "AkPrivateTypes.h"

class IMultiSwitchProxy
{
public:
	virtual void SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 in_fMin, AkReal32 in_fMax ) = 0;
	virtual void SetAkProp( AkPropID in_eProp, AkInt32 in_iValue, AkInt32 in_iMin, AkInt32 in_iMax ) = 0;
	virtual void SetDecisionTree( void* in_pData, AkUInt32 in_uSize, AkUInt32 in_uDepth ) = 0;
	virtual void SetArguments( AkUInt32* in_pArgs, AkUInt8* in_pGroupTypes, AkUInt32 in_uNumArgs ) = 0;

	enum MethodIDs
	{
		MethodSetAkPropF = 1000,
		MethodSetAkPropI,
		MethodSetDecisionTree,
		MethodSetArguments,

		LastMethodID
	};
};

#endif // #ifndef AK_OPTIMIZED
