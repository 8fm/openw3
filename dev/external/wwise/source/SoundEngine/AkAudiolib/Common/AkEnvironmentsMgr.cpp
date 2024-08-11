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
// AkEnvironmentsMgr.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkEnvironmentsMgr.h"
#include "AkFxBase.h"
#include "AkLEngine.h"
#include "AkMonitor.h"
#include "AkAudioLibIndex.h"
#include "AkAuxBus.h"

//====================================================================================================
//====================================================================================================
AKRESULT CAkEnvironmentsMgr::Init()
{
	memset( m_bCurveEnabled, 0, sizeof( m_bCurveEnabled ) );

	return AK_Success;
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkEnvironmentsMgr::Term()
{
	for ( int i=0; i<MAX_CURVE_X_TYPES; ++i )
	{
		for( int j=0; j<MAX_CURVE_Y_TYPES; ++j )
		{
			ConversionTable[i][j].Unset();
		}
	}

	return AK_Success;
}

AKRESULT CAkEnvironmentsMgr::SetObsOccCurve( eCurveXType in_x, eCurveYType in_y, unsigned long in_ulNbPoints, AkRTPCGraphPoint in_paPoints[], AkCurveScaling in_eScaling )
{
	AKASSERT( in_x < MAX_CURVE_X_TYPES );
	AKASSERT( in_y < MAX_CURVE_Y_TYPES );
	AKRESULT eResult = ConversionTable[in_x][in_y].Set( in_paPoints, in_ulNbPoints, in_eScaling );
	if ( eResult == AK_Success && in_y == CurveVol )
		ConversionTable[in_x][in_y].Linearize();

	return eResult;
}
