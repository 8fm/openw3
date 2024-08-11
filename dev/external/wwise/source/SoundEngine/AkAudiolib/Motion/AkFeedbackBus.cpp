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
// AkFeedbackBus.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "AkFeedbackBus.h"
#include "AkSIS.h"
#include <AK/SoundEngine/Common/IAkRTPCSubscriber.h>

CAkFeedbackBus* CAkFeedbackBus::s_pMasterMotionBus = NULL;

CAkFeedbackBus::CAkFeedbackBus(AkUniqueID in_ulID) 
	:CAkBus(in_ulID)
{
	AkNew2( m_pFeedbackInfo, g_DefaultPoolId, AkFeedbackInfo, AkFeedbackInfo());
	if (s_pMasterMotionBus == NULL)
		s_pMasterMotionBus = this;
}

CAkFeedbackBus::~CAkFeedbackBus()
{
	if (s_pMasterMotionBus == this)
		s_pMasterMotionBus = NULL;
}

CAkFeedbackBus* CAkFeedbackBus::Create(AkUniqueID in_ulID)
{
	CAkFeedbackBus* pBus = AkNew( g_DefaultPoolId, CAkFeedbackBus( in_ulID ) );
	if( pBus )
	{
		if( pBus->Init() != AK_Success )
		{
			pBus->Release();
			pBus = NULL;
		}
	}
	return pBus;
}

CAkFeedbackBus* CAkFeedbackBus::GetMasterMotionBusAndAddRef()
{
	AkAutoLock<CAkLock> lock(g_pIndex->GetNodeLock( AkNodeType_Bus ));
	if (s_pMasterMotionBus != NULL)
		s_pMasterMotionBus->AddRef();

	return s_pMasterMotionBus;
}

CAkFeedbackBus* CAkFeedbackBus::ClearTempMasterBus()
{
	AkAutoLock<CAkLock> lock(g_pIndex->GetNodeLock( AkNodeType_Bus ));
	CAkFeedbackBus* pTemp = s_pMasterMotionBus;
	s_pMasterMotionBus = NULL;

	if (pTemp != NULL)
		pTemp->AddRef();
	return pTemp;
}

void CAkFeedbackBus::ResetMasterBus(CAkFeedbackBus* in_pBus)
{
	AkAutoLock<CAkLock> lock(g_pIndex->GetNodeLock( AkNodeType_Bus ));
	if (s_pMasterMotionBus == NULL && in_pBus != NULL)
		s_pMasterMotionBus = in_pBus;
}

AkNodeCategory CAkFeedbackBus::NodeCategory()
{
	return AkNodeCategory_FeedbackBus;
}

AKRESULT CAkFeedbackBus::AddChildInternal( CAkParameterNodeBase* pAudioNode )
{
	AKRESULT eResult = CanAddChild(pAudioNode);
	if(eResult == AK_Success)
	{
		CAkParameterNodeBase** ppNode = NULL;
		if( pAudioNode->IsBusCategory() )
		{
			ppNode = m_mapBusChildId.AddNoSetKey( pAudioNode->ID() );
		}
		else
		{
			ppNode = m_mapChildId.AddNoSetKey( pAudioNode->ID() );
		}
		if( !ppNode )
		{
			eResult = AK_Fail;
		}
		else
		{
			*ppNode = pAudioNode;
			pAudioNode->FeedbackParentBus(this);
			this->AddRef();
		}
	}

	pAudioNode->Release();

	return eResult;
}

void CAkFeedbackBus::RemoveChild( CAkParameterNodeBase* in_pChild )
{
	AKASSERT(in_pChild);

	if( in_pChild->FeedbackParentBus() == this )
	{
		in_pChild->FeedbackParentBus(NULL);
		if( in_pChild->IsBusCategory() )
		{
			m_mapBusChildId.Unset( in_pChild->ID() );
		}
		else
		{
			m_mapChildId.Unset( in_pChild->ID() );
		}
		this->Release();
	}
}


AKRESULT CAkFeedbackBus::CanAddChild( CAkParameterNodeBase * in_pAudioNode )
{
	AKRESULT eResult = AK_Success;
	bool bConnectingBus = in_pAudioNode->IsBusCategory();
	if( !bConnectingBus && m_mapChildId.Exists( in_pAudioNode->ID()) )
	{
		eResult = AK_AlreadyConnected;
	}
	else if( bConnectingBus && m_mapBusChildId.Exists( in_pAudioNode->ID()) )
	{
		eResult = AK_AlreadyConnected;
	}
	else if( bConnectingBus && ID() == in_pAudioNode->ID() )
	{
		eResult = AK_CannotAddItseflAsAChild;
	}
	return eResult;	
}

// Get the compounded feedback parameters.  There is currenly only the volume.
AKRESULT CAkFeedbackBus::GetFeedbackParameters( AkFeedbackParams &io_Params, CAkSource *in_pSource, CAkRegisteredObj * in_GameObjPtr, bool in_bDoBusCheck /*= true*/ )
{
	AkUInt32 paramSelect = PT_Volume + PT_Pitch;
	AKASSERT(m_pFeedbackInfo);

	io_Params.m_NewVolume += GetDuckedVolume( AkPropID_Volume );

	AkSoundParams l_StateParams;
	l_StateParams.Clear();  // Reset all to base values
	GetAudioStateParams( l_StateParams, paramSelect );

	io_Params.m_NewVolume		+= l_StateParams.Volume;
	io_Params.m_MotionBusPitch	+= l_StateParams.Pitch;

	if(paramSelect & PT_Volume)
	{
		GetPropAndRTPC( io_Params.m_NewVolume, AkPropID_Volume, NULL );
	}
	if(paramSelect & PT_Pitch)
	{
		GetPropAndRTPC( io_Params.m_MotionBusPitch, AkPropID_Pitch, NULL );
	}
	if( m_pGlobalSIS )
	{
		ApplySIS( *m_pGlobalSIS, AkPropID_Volume, io_Params.m_NewVolume );
		ApplySIS( *m_pGlobalSIS, AkPropID_Pitch, io_Params.m_MotionBusPitch );
	}
	
	//Recurse in the parent busses.  There can only be one parent!
	CAkFeedbackBus* pParentBus = FeedbackParentBus();
	if(pParentBus != NULL)
		pParentBus->GetFeedbackParameters(io_Params, in_pSource, in_GameObjPtr, false);

	return AK_Success;
}

void CAkFeedbackBus::Notification(
		AkRTPC_ParameterID in_ParamID, 
		AkReal32 in_fValue,				// Param variation
		CAkRegisteredObj * in_pGameObj,	// Target Game Object
		void* in_pExceptArray
		)
{
	if ( in_ParamID == RTPC_Volume )
		in_ParamID = RTPC_FeedbackVolume;
	else if ( in_ParamID == RTPC_Pitch )
		in_ParamID = RTPC_FeedbackPitch;

	CAkBus::Notification( in_ParamID, in_fValue, in_pGameObj, in_pExceptArray );
}

void CAkFeedbackBus::ParamNotification( NotifParams& in_rParams )
{
	AKASSERT( in_rParams.pGameObj == NULL );

	// Note: master bus and bus volumes are applied lower in the hierarchy when the is no effect, 
	// otherwise they are applied at the proper level to avoid having pre-effect volumes
	
	//TODO TODO Enable this code when bus effects will be supported.
	/*in_rParams.bIsFromBus = true;
	if( (in_rParams.eType == RTPC_Volume) && IsMixingBus() )
	{
		if ( IsMasterBus() )
			CAkLEngine::SetMasterBusVolume( in_rParams.fValue );
		else
			CAkLEngine::SetBusVolume( ID(), in_rParams.fValue );
	}
	*/

	CAkBus::ParamNotification(in_rParams);
}
