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
// AkPreparationAware.cpp
// Basic interface and implementation for containers that are 
// switch/state dependent.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkPreparationAware.h"
#include "AkStateMgr.h"
#include "AkMonitor.h"

CAkPreparedContent* CAkPreparationAware::GetPreparedContent( 
        AkUInt32 in_ulGroup, 
        AkGroupType in_eGroupType 
        )
{
	CAkStateMgr::PreparationStateItem* pPreparationStateItem = g_pStateMgr->GetPreparationItem( in_ulGroup, in_eGroupType );
	if( pPreparationStateItem )
		return pPreparationStateItem->GetPreparedcontent();

	return NULL;
}

AKRESULT CAkPreparationAware::SubscribePrepare( 
        AkUInt32 in_ulGroup, 
        AkGroupType in_eGroupType 
        )
{
	CAkStateMgr::PreparationStateItem* pPreparationStateItem = g_pStateMgr->GetPreparationItem( in_ulGroup, in_eGroupType );
	if( pPreparationStateItem )
	{
		pPreparationStateItem->Add( this );// add will not add if already enlisted.
		return AK_Success;
	}
	else
	{
		MONITOR_ERRORMSG( AKTEXT("Insufficient memory can cause sounds to not be loaded") );
		return AK_Fail;
	}
}
	
void CAkPreparationAware::UnsubscribePrepare(
		AkUInt32 in_ulGroup, 
        AkGroupType in_eGroupType 
		)
{
	CAkStateMgr::PreparationStateItem* pPreparationStateItem = g_pStateMgr->GetPreparationItem( in_ulGroup, in_eGroupType );
	if( pPreparationStateItem )
	{
		pPreparationStateItem->Remove( this );
	}
}



