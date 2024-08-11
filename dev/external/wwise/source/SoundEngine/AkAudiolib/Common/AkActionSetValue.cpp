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
// AkActionSetValue.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionSetValue.h"
#include "AkParameterNode.h"
#include "AkAudioLibIndex.h"
#include "AkRegistryMgr.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkModifiers.h"
#include "AkRegisteredObj.h"

#include "AkAudioMgr.h"

CAkActionSetValue::CAkActionSetValue(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkActionExcept(in_eActionType, in_ulID)
{
}

CAkActionSetValue::~CAkActionSetValue()
{

}

AKRESULT CAkActionSetValue::Execute( AkPendingAction * in_pAction )
{
	AKASSERT(g_pIndex);
	AKASSERT(g_pRegistryMgr);

	CAkParameterNodeBase* pNode = NULL;

	switch(ActionType())
	{
		case AkActionType_SetVolume_O:
		case AkActionType_SetPitch_O:
		case AkActionType_Mute_O:
		case AkActionType_SetLPF_O:
			pNode = GetAndRefTarget();
			if(pNode)
			{
				ExecSetValue(pNode, in_pAction->GameObj());
				pNode->Release();
			}
			break;
		case AkActionType_SetVolume_M:
		case AkActionType_SetPitch_M:
		case AkActionType_Mute_M:
		case AkActionType_SetLPF_M:
		case AkActionType_SetBusVolume_M:
			pNode = GetAndRefTarget();
			if(pNode)
			{
				ExecSetValue(pNode);
				pNode->Release();
			}
			break;
		case AkActionType_ResetVolume_O:
		case AkActionType_ResetPitch_O:
		case AkActionType_Unmute_O:
		case AkActionType_ResetLPF_O:
			pNode = GetAndRefTarget();
			if(pNode)
			{
				ExecResetValue(pNode,in_pAction->GameObj());
				pNode->Release();
			}
			break;
		case AkActionType_ResetVolume_M:
		case AkActionType_ResetPitch_M:
		case AkActionType_Unmute_M:
		case AkActionType_ResetLPF_M:
		case AkActionType_ResetBusVolume_M:
			pNode = GetAndRefTarget();
			if(pNode)
			{
				ExecResetValue(pNode);
				pNode->Release();
			}
			break;
		case AkActionType_ResetVolume_ALL_O:
		case AkActionType_ResetPitch_ALL_O:
		case AkActionType_Unmute_ALL_O:
		case AkActionType_ResetLPF_ALL_O:
			{
				const AkListNode* pListID = in_pAction->GameObj()->GetModifiedElementList();
				if( pListID )
				{
					for( AkListNode::Iterator iter = pListID->Begin(); iter != pListID->End(); ++iter )
					{
						pNode = g_pIndex->GetNodePtrAndAddRef( *iter );
						if(pNode)
						{
							ExecResetValue( pNode, in_pAction->GameObj() );
							pNode->Release();
						}
					}
				}
				break;
			}
		case AkActionType_ResetVolume_ALL:
		case AkActionType_ResetPitch_ALL:
		case AkActionType_Unmute_ALL:
		case AkActionType_ResetLPF_ALL:
		case AkActionType_ResetBusVolume_ALL:
			{	
				ResetAllHelper( g_pRegistryMgr->GetModifiedElementList() );

				CAkRegistryMgr::AkMapRegisteredObj& regObjects = g_pRegistryMgr->GetRegisteredObjectList();
				for ( CAkRegistryMgr::AkMapRegisteredObj::Iterator iterObj = regObjects.Begin(); iterObj != regObjects.End(); ++iterObj )
				{
					ResetAllHelper( (*iterObj).item->GetModifiedElementList() );
				}
				break;
			}
		case AkActionType_ResetVolume_AE:
		case AkActionType_ResetPitch_AE:
		case AkActionType_Unmute_AE:
		case AkActionType_ResetLPF_AE:
		case AkActionType_ResetBusVolume_AE:
			{
				ResetAEHelper( g_pRegistryMgr->GetModifiedElementList() );

				CAkRegistryMgr::AkMapRegisteredObj& regObjects = g_pRegistryMgr->GetRegisteredObjectList();
				for ( CAkRegistryMgr::AkMapRegisteredObj::Iterator iterObj = regObjects.Begin(); iterObj != regObjects.End(); ++iterObj )
				{
					ResetAEHelper( (*iterObj).item->GetModifiedElementList() );
				}
				break;
			}
		case AkActionType_ResetVolume_AE_O:
		case AkActionType_ResetPitch_AE_O:
		case AkActionType_Unmute_AE_O:
		case AkActionType_ResetLPF_AE_O:
			{
				const AkListNode* pListID = in_pAction->GameObj()->GetModifiedElementList();
				if( pListID )
				{
					for( AkListNode::Iterator iter = pListID->Begin(); iter != pListID->End(); ++iter )
					{
						pNode = g_pIndex->GetNodePtrAndAddRef( *iter );
						if( pNode )
						{
							ExecResetValueExcept( pNode, in_pAction->GameObj() );
							pNode->Release();
						}
					}
				}
				break;
			}
		case AkActionType_SetGameParameter_O:
		case AkActionType_SetGameParameter:
			ExecSetValue(NULL, in_pAction->GameObj());
			break;
		case AkActionType_ResetGameParameter_O:
		case AkActionType_ResetGameParameter:
			ExecResetValue(NULL,in_pAction->GameObj());
			break;
		default:
			AKASSERT(!"Unknown or unsupported Action Type Requested");
			break;
	}
	return AK_Success;
}

void CAkActionSetValue::ResetAllHelper( const AkListNode* in_pListID )
{
	if( in_pListID )
	{
		for( AkListNode::Iterator iter = in_pListID->Begin(); iter != in_pListID->End(); ++iter )
		{
			CAkParameterNodeBase* pNode = g_pIndex->GetNodePtrAndAddRef( *iter );
			if( pNode )
			{
				ExecResetValueAll( pNode );
				pNode->Release();
			}
		}
	}
}

void CAkActionSetValue::ResetAEHelper( const AkListNode* in_pListID )
{
	if( in_pListID )
	{
		for( AkListNode::Iterator iter = in_pListID->Begin(); iter != in_pListID->End(); ++iter )
		{
			CAkParameterNodeBase* pNode = g_pIndex->GetNodePtrAndAddRef( *iter );
			if( pNode )
			{
				ExecResetValueExcept( pNode );
				pNode->Release();
			}
		}
	}
}

AKRESULT CAkActionSetValue::SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkUInt8 ucFadeCurveType = READBANKDATA(AkUInt8, io_rpData, io_rulDataSize );

	m_eFadeCurve = (AkCurveInterpolation)ucFadeCurveType;

	AKRESULT eResult = SetActionSpecificParams(io_rpData, io_rulDataSize);

	if( eResult == AK_Success )
		eResult = SetExceptParams( io_rpData, io_rulDataSize );

	return eResult;
}
