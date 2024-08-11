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
// AkContainerBase.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkContainerBase.h"
#include "AkAudioLibIndex.h"
#include "AkContinuationList.h"
#include "PrivateStructures.h"
#include "AkMonitor.h"

CAkContainerBase::CAkContainerBase(AkUniqueID in_ulID)
:CAkActiveParent<CAkParameterNode>(in_ulID)
{
}

CAkContainerBase::~CAkContainerBase()
{
}

AKRESULT CAkContainerBase::CanAddChild( CAkParameterNodeBase * in_pAudioNode )
{
	AKASSERT(in_pAudioNode);

	AkNodeCategory eCategory = in_pAudioNode->NodeCategory();

	AKRESULT eResult = AK_Success;	
	if(Children() >= AK_MAX_NUM_CHILD)
	{
		MONITOR_ERRORMSG( AKTEXT("Too many children in one single container.") );
		eResult = AK_MaxReached;
	}
	else if(
		eCategory != AkNodeCategory_Sound 
		&& eCategory != AkNodeCategory_FeedbackNode
		&& eCategory != AkNodeCategory_RanSeqCntr
		&& eCategory != AkNodeCategory_SwitchCntr
		&& eCategory != AkNodeCategory_LayerCntr
		)
	{
		eResult = AK_NotCompatible;
	}
	else if(in_pAudioNode->Parent() != NULL)
	{
		eResult = AK_ChildAlreadyHasAParent;
	}
	else if(m_mapChildId.Exists(in_pAudioNode->ID()))
	{
		eResult = AK_AlreadyConnected;
	}
	else if(ID() == in_pAudioNode->ID())
	{
		eResult = AK_CannotAddItseflAsAChild;
	}
	return eResult;	
}
