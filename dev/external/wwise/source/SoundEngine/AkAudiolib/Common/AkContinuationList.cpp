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
// AkContinuationList.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkContinuationList.h"
#include "AkRanSeqCntr.h"
#include "AkSwitchCntr.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

CAkContinueListItem::CAkContinueListItem()
:m_pContainer( NULL )
,m_pContainerInfo( NULL )
,m_pMultiPlayNode( NULL )
,m_pAlternateContList( NULL )
{
}

CAkContinueListItem::~CAkContinueListItem()
{
	if(m_pContainerInfo)
	{
		m_pContainerInfo->Destroy();
		m_pContainerInfo = NULL;
	}
}

const CAkContinueListItem& CAkContinueListItem::operator=(const CAkContinueListItem & in_listItem)
{
	if (&in_listItem != this)
	{
		m_pContainer = in_listItem.m_pContainer;

		if (in_listItem.m_pContainerInfo)
			m_pContainerInfo = in_listItem.m_pContainerInfo->Clone( (AkUInt16)m_pContainer->GetPlaylistLength() );
		else
			m_pContainerInfo = NULL;

		m_LoopingInfo = in_listItem.m_LoopingInfo;
		m_pMultiPlayNode = in_listItem.m_pMultiPlayNode;
		m_pAlternateContList = in_listItem.m_pAlternateContList;
	}
	return *this;
}

void CAkContinuationList::Term()
{
	for ( AkContinueListItem::Iterator iter = m_listItems.Begin(); iter != m_listItems.End(); ++iter )
	{
		CAkContinueListItem & item = *iter;

		if( item.m_pMultiPlayNode && item.m_pAlternateContList )
		{
			//Unref removes the ref and destroy the list if comes to zero
			item.m_pMultiPlayNode->ContUnrefList( item.m_pAlternateContList );
		}
	}

	m_listItems.Term();
}

CAkContinuationList::CAkContinuationList()
	: m_iRefCount( 1 )
{
}

CAkContinuationList* CAkContinuationList::Create()
{
	return AkNew( g_DefaultPoolId, CAkContinuationList );
}

void CAkContinuationList::AddRef()
{ 
	++m_iRefCount; 
}
void CAkContinuationList::Release()
{
	if( --m_iRefCount == 0 )
	{
		Term();
		AkDelete( g_DefaultPoolId, this );
	}
}
