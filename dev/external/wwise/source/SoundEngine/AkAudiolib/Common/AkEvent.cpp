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
// AkEvent.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkEvent.h"
#include "AkAction.h"
#include "AkAudioLibIndex.h"
#include "AkAudioMgr.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkParameterNodeBase.h"

CAkEvent::CAkEvent(AkUniqueID in_ulID)
:CAkIndexable(in_ulID)
,m_iPreparationCount(0)
{
}

CAkEvent::~CAkEvent()
{
	Clear();
	m_actions.Term();
	AKASSERT( m_iPreparationCount == 0 );
}

CAkEvent* CAkEvent::Create(AkUniqueID in_ulID)
{
	CAkEvent* pEvent = AkNew( g_DefaultPoolId, CAkEvent(in_ulID) );
	if( pEvent )
	{
		pEvent->AddToIndex();
	}
	return pEvent;
}

CAkEvent* CAkEvent::CreateNoIndex(AkUniqueID in_ulID)
{
	return AkNew( g_DefaultPoolId, CAkEvent(in_ulID) );
}

void CAkEvent::AddToIndex()
{
	AKASSERT( g_pIndex );
	AKASSERT( ID() != AK_INVALID_UNIQUE_ID );
	g_pIndex->m_idxEvents.SetIDToPtr( this );
}

void CAkEvent::RemoveFromIndex()
{
	AKASSERT(g_pIndex);
	AKASSERT( ID() != AK_INVALID_UNIQUE_ID );
	g_pIndex->m_idxEvents.RemoveID(ID());
}

AkUInt32 CAkEvent::AddRef() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxEvents.GetLock() ); 
    return ++m_lRef; 
} 

AkUInt32 CAkEvent::Release() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxEvents.GetLock() ); 
    AkInt32 lRef = --m_lRef; 
    AKASSERT( lRef >= 0 ); 
    if ( !lRef ) 
    { 
		RemoveFromIndex();
		AkDelete( g_DefaultPoolId, this ); 
    } 
    return lRef; 
}

AKRESULT CAkEvent::Add( AkUniqueID in_ulAction )
{
	//Find the last action, we must insert at the end.  (Yup, this is inefficient, but we only have small numbers of actions)
	AkActionList::Iterator itLast = m_actions.Begin();
	while(itLast != m_actions.End() && itLast.pItem->pNextLightItem != NULL)
		++itLast;

	return AddAfter( in_ulAction, itLast.pItem );
}

AKRESULT CAkEvent::AddAfter( AkUniqueID in_ulAction, CAkAction*& io_pPrevious )
{
	if( in_ulAction == AK_INVALID_UNIQUE_ID )
	{
		return AK_InvalidID;
	}

	CAkAction* pAction = g_pIndex->m_idxActions.GetPtrAndAddRef(in_ulAction);
	if ( !pAction )
		return AK_Fail;

	AKASSERT( m_actions.FindEx(pAction).pItem == NULL );
	m_actions.AddItem(pAction, NULL, io_pPrevious);
	io_pPrevious = pAction;

	return AK_Success;
}

void CAkEvent::Remove( AkUniqueID in_ulAction )
{
	CAkAction* pAction = g_pIndex->m_idxActions.GetPtrAndAddRef(in_ulAction);
	if ( pAction )
	{
	    if ( m_actions.Remove( pAction ) == AK_Success )
			pAction->Release();
	
		pAction->Release();
	}
}

void CAkEvent::Clear()
{
	while(!m_actions.IsEmpty())
	{
		CAkAction *pAction = m_actions.First();
		m_actions.RemoveFirst();
		pAction->Release();
	}
}

AKRESULT CAkEvent::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize)
{
	// Read ID

	// We don't care about the ID, just skip it
	SKIPBANKDATA(AkUInt32, in_pData, in_ulDataSize );

	// Read Action List Size
	AkUInt32 ulActionListSize = READBANKDATA(AkUInt32, in_pData, in_ulDataSize );

	AKRESULT eResult = AK_Success;
	CAkAction *pLast = NULL;
	for(AkUInt32 i = 0; i < ulActionListSize; ++i)
	{
		AkUInt32 ulActionID = READBANKDATA(AkUInt32, in_pData, in_ulDataSize );

		eResult = AddAfter(ulActionID, pLast);
		if(eResult != AK_Success)
		{
			break;
		}
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

AKRESULT CAkEvent::QueryAudioObjectIDs( AkUInt32& io_ruNumItems, AkObjectInfo* out_aObjectInfos )
{
	AkUInt32 uIndex_Out = 0;
	AkUInt32 uDepth = 0;
	CAkEvent::AkActionList::Iterator iter = m_actions.Begin();
	while( iter != m_actions.End() )
	{
		AkActionType aType = (*iter)->ActionType();
		if( aType != AkActionType_Play )
		{
			++iter;
			continue;
		}

		CAkParameterNodeBase * pObj = (*iter)->GetAndRefTarget();
		if( !pObj )
		{
			++iter;
			continue;
		}

		if( io_ruNumItems == 0 )
			uIndex_Out++;
		else
		{
			out_aObjectInfos[uIndex_Out].objID = pObj->ID();
			out_aObjectInfos[uIndex_Out].parentID = pObj->Parent() ? pObj->Parent()->ID() : AK_INVALID_UNIQUE_ID;
			out_aObjectInfos[uIndex_Out].iDepth = uDepth;
			uIndex_Out++;
			if( uIndex_Out == io_ruNumItems )
			{
				pObj->Release();
				break; //exit loop
			}
		}

		//now query all children of this object
		pObj->GetChildren( io_ruNumItems, out_aObjectInfos, uIndex_Out, uDepth + 1 );
		pObj->Release();
		if( uIndex_Out == io_ruNumItems )
		{
			break; //exit loop
		}

		++iter;
	}

	AKRESULT eResult = AK_Success;
	if( io_ruNumItems == 0 )
		eResult = AK_PartialSuccess;

	io_ruNumItems = uIndex_Out;
	return eResult;
}
