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
// AkPBIAware.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkPBIAware.h"
#include "AudiolibDefs.h"
#include "AkContinuousPBI.h"

CAkPBIAware::~CAkPBIAware()
{
}

CAkPBI* CAkPBIAware::CreatePBI( CAkSoundBase*		in_pSound,
							    CAkSource*			in_pSource,
                                AkPBIParams&		in_rPBIParams,
                                const PriorityInfoCurrent& in_rPriority,
								CAkLimiter*					in_pAMLimiter,
							    CAkLimiter*					in_pBusLimiter
								) const
{
    AKASSERT( in_rPBIParams.eType == AkPBIParams::PBI );
#ifdef AK_MOTION
	return AkNew( RENDERER_DEFAULT_POOL_ID, CAkPBI( 
		in_pSound,
		in_pSource,
		in_rPBIParams.pGameObj,
		in_rPBIParams.userParams,
		in_rPBIParams.playHistory,
		AK_INVALID_SEQUENCE_ID,
		in_rPriority,
		in_rPBIParams.bTargetFeedback,
		0,
		in_pAMLimiter,
		in_pBusLimiter) );
#else // AK_MOTION
	return AkNew( RENDERER_DEFAULT_POOL_ID, CAkPBI( 
		in_pSound,
		in_pSource,
		in_rPBIParams.pGameObj,
		in_rPBIParams.userParams,
		in_rPBIParams.playHistory,
		AK_INVALID_SEQUENCE_ID,
		in_rPriority,
		0,
		in_pAMLimiter,
		in_pBusLimiter) );
#endif // AK_MOTION
}
