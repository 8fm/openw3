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
// AkEnvironmentsMgr.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ENVIRONMENTS_MGR_H_
#define _ENVIRONMENTS_MGR_H_

#include <AK/SoundEngine/Common/AkTypes.h>

#include "AkConversionTable.h"
#include "AkCommon.h"

class CAkEnvironmentsMgr
{
public:
	
	// also defined in IWProject.h
	enum eCurveXType { CurveObs, CurveOcc, MAX_CURVE_X_TYPES };
	enum eCurveYType { CurveVol, CurveLPF, MAX_CURVE_Y_TYPES };

	
public:
	AKRESULT Init();
	AKRESULT Term();

	// Obstruction/Occlusion curves methods
	AKRESULT SetObsOccCurve( eCurveXType in_x, eCurveYType in_y, unsigned long in_ulNbPoints, AkRTPCGraphPoint in_paPoints[], AkCurveScaling in_eScaling );
	AkForceInline void SetCurveEnabled( eCurveXType in_x, eCurveYType in_y, bool in_bEnable ) { m_bCurveEnabled[in_x][in_y] = in_bEnable; }
	AkForceInline bool IsCurveEnabled( eCurveXType in_x, eCurveYType in_y ) { return m_bCurveEnabled[in_x][in_y] && ConversionTable[in_x][in_y].m_pArrayGraphPoints; }

	AkForceInline AkReal32 GetCurveValue( eCurveXType in_x, eCurveYType in_y, AkReal32 in_value )
	{
		AKASSERT( in_x < MAX_CURVE_X_TYPES && in_y < MAX_CURVE_Y_TYPES && IsCurveEnabled( in_x, in_y ) );
		return ConversionTable[in_x][in_y].Convert( in_value );
	}

private:

	bool m_bCurveEnabled[MAX_CURVE_X_TYPES][MAX_CURVE_Y_TYPES];
	CAkConversionTable<AkRTPCGraphPoint, AkReal32>	ConversionTable[MAX_CURVE_X_TYPES][MAX_CURVE_Y_TYPES]; //we use this for our obs/occ curves!
};

extern CAkEnvironmentsMgr* g_pEnvironmentMgr;

#endif
