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
// AkActionDuck.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionDuck.h"
#include "AkAudioLibIndex.h"
#include "AkAudioMgr.h"
#include "AkBus.h"

CAkActionDuck::CAkActionDuck(AkActionType in_eActionType, AkUniqueID in_ulID)
: CAkAction(in_eActionType, in_ulID)
{
}

CAkActionDuck::~CAkActionDuck()
{
}

AKRESULT CAkActionDuck::Execute( AkPendingAction * in_pAction )
{
	AKASSERT(g_pIndex);

	CAkBus* pBus = static_cast<CAkBus*>( GetAndRefTarget() );
	if(pBus)
	{
		pBus->DuckNotif();
		pBus->Release();
	}

	return AK_Success;
}

CAkActionDuck* CAkActionDuck::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionDuck*	pActionDuck = AkNew(g_DefaultPoolId,CAkActionDuck(in_eActionType, in_ulID));
	if( pActionDuck )
	{
		if( pActionDuck->Init() != AK_Success )
		{
			pActionDuck->Release();
			pActionDuck = NULL;
		}
	}

	return pActionDuck;
}
