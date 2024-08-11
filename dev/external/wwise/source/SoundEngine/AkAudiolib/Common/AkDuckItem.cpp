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
// AkDuckItem.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkDuckItem.h"
#include "AkTransition.h"
#include "AkTransitionManager.h"
#include "AkBus.h"
#include "AkMonitor.h"

AKRESULT CAkDuckItem::Term()
{
	AKASSERT(g_pTransitionManager);
	if(m_pvVolumeTransition)	{
		
		g_pTransitionManager->RemoveTransitionUser(m_pvVolumeTransition, this);
		m_pvVolumeTransition = NULL;
	}
	return AK_Success;
}


AKRESULT CAkDuckItem::Init(CAkBus* in_pBusNode)
{
	AKASSERT(in_pBusNode);
	m_pvVolumeTransition = NULL;
	m_pBusNode = in_pBusNode;
	m_EffectiveVolumeOffset = 0;
	return AK_Success;
}

void CAkDuckItem::TransUpdateValue(AkIntPtr in_target, AkReal32 in_fValue, bool in_bIsTerminated)
{
	AkPropID eTargetProp = (AkPropID) in_target;

	AkReal32 fPreviousVolume = m_pBusNode->GetDuckedVolume( eTargetProp );

	// set the new value
	m_EffectiveVolumeOffset = in_fValue;

	AkReal32 fNextVolume = m_pBusNode->GetDuckedVolume( eTargetProp );

	AkReal32 fNotified = fNextVolume - fPreviousVolume;

	if(in_bIsTerminated)
	{
		m_pBusNode->CheckDuck();
		m_pvVolumeTransition = NULL;
	}

	if( fNotified != 0.0f )
	{
		m_pBusNode->Notification( g_AkPropRTPCID[in_target], fNotified );
	}
}
