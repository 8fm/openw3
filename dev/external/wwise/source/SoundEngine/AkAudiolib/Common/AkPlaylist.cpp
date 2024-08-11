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
// AkPlayList.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkPlaylist.h"

CAkPlayListRandom::CAkPlayListRandom()
{
}
CAkPlayListRandom::~CAkPlayListRandom()
{
}

AKRESULT CAkPlayListRandom::Init()
{
	return AK_Success;
}

void CAkPlayListRandom::Destroy()
{
	m_PlayList.Term();
	AkDelete(g_DefaultPoolId,this);
}

AkUInt32 CAkPlayListRandom::Length()
{
	return m_PlayList.Length();
}

AkUniqueID CAkPlayListRandom::ID(AkUInt16 in_wPosition)
{
	return m_PlayList[in_wPosition].ulID;
}

AKRESULT CAkPlayListRandom::Add(AkUniqueID in_ulID, AkUInt32 in_weight/*=DEFAULT_RANDOM_WEIGHT*/)
{
	AKASSERT(in_weight);
	AkPlaylistItem * pItem = m_PlayList.AddLast();
	if ( !pItem )
		return AK_Fail;

	pItem->weight = in_weight;
	pItem->ulID = in_ulID;

	return AK_Success;
}

void CAkPlayListRandom::Remove(AkUniqueID in_ID)
{
	AkPlayListRdm::Iterator iter = m_PlayList.Begin();
	while( iter != m_PlayList.End() )
	{
		if((*iter).ulID == in_ID)
		{
			iter = m_PlayList.Erase( iter );
		}
		else
		{
			++iter;
		}
	}
}

void CAkPlayListRandom::RemoveAll()
{
	m_PlayList.RemoveAll();
}

void CAkPlayListRandom::SetWeight(AkUInt16 in_wPosition, AkUInt32 in_weight)
{
	AKASSERT(in_weight);
	m_PlayList[in_wPosition].weight = in_weight;
}

AkUInt32 CAkPlayListRandom::GetWeight(AkUInt16 in_wPosition)
{
	return m_PlayList[in_wPosition].weight;
}


AkUInt32 CAkPlayListRandom::CalculateTotalWeight()
{
	AkUInt32 ulTotalWeight = 0;
	for( AkPlayListRdm::Iterator iter = m_PlayList.Begin(); iter != m_PlayList.End(); ++iter )
	{
		ulTotalWeight += (*iter).weight;
	}
	return ulTotalWeight;
}

bool CAkPlayListRandom::Exists(AkUniqueID in_ID)
{
	for( AkPlayListRdm::Iterator iter = m_PlayList.Begin(); iter != m_PlayList.End(); ++iter )
	{
		if( (*iter).ulID == in_ID )
		{
			return true;
		}
	}
	return false;
}

bool CAkPlayListRandom::GetPosition( AkUniqueID in_ID, AkUInt16& out_rwPosition )
{
	AkUInt16 wPositionCount = 0;
	for( AkPlayListRdm::Iterator iter = m_PlayList.Begin(); iter != m_PlayList.End(); ++iter )
	{
		if( (*iter).ulID == in_ID )
		{
			out_rwPosition = wPositionCount;	
			return true;
		}
		++wPositionCount;
	}
	return false;
}
//////////////////////////////////////////////////////////////////////
///////////////////////// SEQUENCE ///////////////////////////////////
//////////////////////////////////////////////////////////////////////
CAkPlayListSequence::CAkPlayListSequence()
{
}
CAkPlayListSequence::~CAkPlayListSequence()
{
}

AKRESULT CAkPlayListSequence::Init()
{
	return AK_Success;
}
void CAkPlayListSequence::Destroy()
{
	m_PlayList.Term();
	AkDelete(g_DefaultPoolId,this);
}

AkUInt32 CAkPlayListSequence::Length()
{
	return m_PlayList.Length();
}

AkUniqueID CAkPlayListSequence::ID(AkUInt16 in_wPosition)
{
	return m_PlayList[in_wPosition];
}

AKRESULT CAkPlayListSequence::Add(AkUniqueID in_ulID, AkUInt32 /*in_weight=DEFAULT_RANDOM_WEIGHT*/)
{
	return m_PlayList.AddLast(in_ulID) ? AK_Success:AK_Fail;
}

void CAkPlayListSequence::Remove(AkUniqueID in_ulID)
{
	m_PlayList.Remove(in_ulID);
}

void CAkPlayListSequence::RemoveAll()
{
	m_PlayList.RemoveAll();
}

void CAkPlayListSequence::SetWeight(AkUInt16 /*in_wPosition*/, AkUInt32 /*in_weight*/)
{
}

bool CAkPlayListSequence::Exists(AkUniqueID in_ulID)
{
	if(m_PlayList.FindEx(in_ulID) != m_PlayList.End())
	{
		return true;
	}
	return false;
}

bool CAkPlayListSequence::GetPosition(AkUniqueID in_ID, AkUInt16& out_rwPosition)
{
	AkPlaylistSeq::Iterator it = m_PlayList.FindEx( in_ID );
	if ( it != m_PlayList.End() )
	{
		out_rwPosition = (AkUInt16) ( it.pItem - m_PlayList.Begin().pItem );
		return true;
	}

	return false;
}
