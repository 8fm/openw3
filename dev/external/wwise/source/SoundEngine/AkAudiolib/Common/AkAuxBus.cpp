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
// AkAuxBus.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkAuxBus.h"

CAkAuxBus::CAkAuxBus(AkUniqueID in_ulID)
:CAkBus(in_ulID)
{
}

CAkAuxBus::~CAkAuxBus()
{
}

AKRESULT CAkAuxBus::Init()
{
	AKRESULT eResult = CAkBus::Init();

	return eResult;
}

CAkAuxBus* CAkAuxBus::Create( AkUniqueID in_ulID )
{
	CAkAuxBus* pAuxBus = AkNew( g_DefaultPoolId, CAkAuxBus( in_ulID ) );
	if( pAuxBus )
	{
		if( pAuxBus->Init() != AK_Success )
		{
			pAuxBus->Release();
			pAuxBus = NULL;
		}
	}

	return pAuxBus;
}

AkNodeCategory CAkAuxBus::NodeCategory()
{
	return AkNodeCategory_AuxBus;
}
