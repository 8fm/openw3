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
// AkActorMixer.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkActorMixer.h"
#include "AkAudioLibIndex.h"

CAkActorMixer::CAkActorMixer(AkUniqueID in_ulID)
:CAkActiveParent<CAkParameterNode>(in_ulID)
{
}

CAkActorMixer::~CAkActorMixer()
{
}

CAkActorMixer* CAkActorMixer::Create(AkUniqueID in_ulID)
{
	CAkActorMixer* pActorMixer = AkNew(g_DefaultPoolId,CAkActorMixer(in_ulID));
	if( pActorMixer )
	{
		if( pActorMixer->Init() != AK_Success )
		{
			pActorMixer->Release();
			pActorMixer = NULL;
		}
	}
	return pActorMixer;
}

AkNodeCategory CAkActorMixer::NodeCategory()
{
	return AkNodeCategory_ActorMixer;
}

AKRESULT CAkActorMixer::CanAddChild( CAkParameterNodeBase * in_pAudioNode )
{
	AKASSERT(in_pAudioNode);

	AKRESULT eResult = AK_Success;	
	if(in_pAudioNode->Parent() != NULL)
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

AKRESULT CAkActorMixer::PlayInternal( AkPBIParams& )
{
	AKASSERT(!"Shouldn't be called on an Actor mixer"); 
	return AK_NotImplemented;
}

AKRESULT CAkActorMixer::SetInitialValues(AkUInt8* in_pData, AkUInt32 in_ulDataSize)
{
	AKRESULT eResult = AK_Success;

	//Read ID
	// We don't care about the ID, just skip it
	SKIPBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	//ReadParameterNode
	eResult = SetNodeBaseParams(in_pData, in_ulDataSize, false);

	if( eResult == AK_Success )
	{
		eResult = SetChildren( in_pData, in_ulDataSize );
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}
