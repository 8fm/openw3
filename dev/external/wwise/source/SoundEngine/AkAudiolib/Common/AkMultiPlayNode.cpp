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
// AkMultiPlayNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkMultiPlayNode.h"
#include "AkPBI.h"
#include "AkPathManager.h"

SafeContinuationList::SafeContinuationList( AkPBIParams& in_rPBIParams, CAkMultiPlayNode* in_pMultiPlayNode )
{
	m_spBackupContinuationList = NULL;
	if ( in_rPBIParams.eType != AkPBIParams::PBI )
	{
		AKASSERT( in_rPBIParams.pContinuousParams );
		if( in_rPBIParams.pContinuousParams->spContList )
		{
			m_spBackupContinuationList = in_rPBIParams.pContinuousParams->spContList;
			in_pMultiPlayNode->ContRefList( m_spBackupContinuationList );
		}
	}
}

AkContParamsAndPath::AkContParamsAndPath( ContParams* in_pFrom )
	:m_continuousParams( in_pFrom )
{
	if( g_pPathManager )
	{
		if( m_continuousParams.pPathInfo->pPBPath )
		{
			g_pPathManager->AddPotentialUser( m_continuousParams.pPathInfo->pPBPath );
		}
	}
}

AkContParamsAndPath::~AkContParamsAndPath()
{
	if( g_pPathManager )
	{
		if( m_continuousParams.pPathInfo->pPBPath )
		{
			g_pPathManager->RemovePotentialUser( m_continuousParams.pPathInfo->pPBPath );
		}
	}
}


CAkMultiPlayNode::CAkMultiPlayNode( AkUniqueID in_ulID )
:CAkContainerBase( in_ulID )
{
}

CAkMultiPlayNode::~CAkMultiPlayNode()
{
}

bool CAkMultiPlayNode::IsContinuousPlayback()
{
	// By default, nodes are supposed to be in non continuous playback.
	// overload this function if the inheriting node has the possibility to be virtual.
	return false;
}

AKRESULT CAkMultiPlayNode::Init()
{
	AKRESULT eResult = CAkContainerBase::Init();
	return eResult;
}

void CAkMultiPlayNode::Term()
{
	m_listContParameters.Term();
}

AKRESULT CAkMultiPlayNode::ContRefList( CAkContinuationList* in_pList )
{
	ContParam * pParam = m_listContParameters.Set( in_pList );
	if ( !pParam )
		return AK_Fail;

	if ( pParam->uRefCount == 0 )
		in_pList->AddRef();

	pParam->uRefCount += 1;

	return AK_Success;
}

void CAkMultiPlayNode::ContGetList( CAkContinuationList* in_pList, CAkSmartPtr<CAkContinuationList>& io_spList )
{
	ContParam * pParam = m_listContParameters.Exists( in_pList );
	if( pParam )
	{
		if( pParam->uRefCount <= 1 )
		{
			if( !IsContinuousPlayback() )
			{
				io_spList = pParam->pContList;
			}
			pParam->pContList->Release();
			ContParams::Iterator it;
			it.pItem = pParam;
			m_listContParameters.Erase( it );
		}
		else
		{
			pParam->uRefCount -= 1;
		}
	}
}

AKRESULT CAkMultiPlayNode::ContUnrefList( CAkContinuationList* in_pList )
{
	ContParam * pParam = m_listContParameters.Exists( in_pList );
	if( pParam )
	{
		if( pParam->uRefCount <= 1 )
		{
			pParam->pContList->Release();
			ContParams::Iterator it;
			it.pItem = pParam;
			m_listContParameters.Erase( it );
		}
		else
		{
			pParam->uRefCount -= 1;
		}
	}

	return AK_Success;
}


AKRESULT CAkMultiPlayNode::AddMultiplayItem( AkContParamsAndPath& in_rContParams, AkPBIParams& in_rParams, SafeContinuationList& in_rSafeContList /*warning, not necessarily the one in the in_rContParams*/ )
{
	CAkContinueListItem* pItem = in_rContParams.Get().spContList->m_listItems.AddLast();

	///////////////////////////////////////////
	// Crash for WG-15729 padded but not fixed
	// in_rParams.pContinuousParams->spContList should not be NULL, but apparently it happens..
	// WG-15729 will be still active, but at least Wwise app will not crash here.
	///////////////////////////////////////////
	//AK::Monitor::PostString( in_pzConverted, AK::Monitor::ErrorLevel_Message );
	if( pItem && !in_rParams.pContinuousParams->spContList )
	{
		// Log a message to inform the user of the known issue.
		AK::Monitor::PostString( AKTEXT("Playback failed, known issue WG-15729. Multiple simultaneous playback in multiple continuous mode currently unsupported."), AK::Monitor::ErrorLevel_Message );
	}
	///////////////////////////////////////////
	// End WG-15729 patch.
	///////////////////////////////////////////

	if ( pItem && in_rParams.pContinuousParams->spContList )
	{
		pItem->m_pAlternateContList = in_rSafeContList.Get();
		pItem->m_pMultiPlayNode = this;

		// The following line is there only for Wwise and is used only in the situation of a force next to play command
		// this is voluntarily not set in #ifndef AK_OPTIMIZED, but it could be
		if( !in_rParams.pContinuousParams->spContList->m_listItems.IsEmpty() )
		{
			pItem->m_LoopingInfo = in_rParams.pContinuousParams->spContList->m_listItems.Begin().pItem->m_LoopingInfo;
		}
		else
		{
			pItem->m_LoopingInfo.bIsEnabled = false;
			pItem->m_LoopingInfo.bIsInfinite = false;
			pItem->m_LoopingInfo.lLoopCount = 1;
		}

		if( pItem->m_pAlternateContList )
		{
			ContRefList( pItem->m_pAlternateContList );
		}
	}
	else
	{
		in_rContParams.Get().spContList = NULL;
		return AK_InsufficientMemory;
	}
	return AK_Success;
}

AKRESULT CAkMultiPlayNode::PlayAndContinueAlternateMultiPlay( AkPBIParams& in_rPBIParams )
{
	AKRESULT eResult = AK_Fail;

	AkContParamsAndPath continuousParams( in_rPBIParams.pContinuousParams );

    ContGetList( in_rPBIParams.pContinuousParams->spContList, continuousParams.Get().spContList );

	if( continuousParams.Get().spContList )
	{
		in_rPBIParams.pContinuousParams = &continuousParams.Get();
		eResult = PlayAndContinueAlternate( in_rPBIParams );
		if( eResult == AK_PartialSuccess )
		{
			eResult = AK_Success;
		}
	}

	return eResult;
}


