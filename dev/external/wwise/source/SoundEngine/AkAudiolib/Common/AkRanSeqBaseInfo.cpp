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
// AkRanSeqBaseInfo.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkRanSeqBaseInfo.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkRandom.h"

#define MAX_LISTAVOID_SIZE	4

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

void CAkRandomInfo::Destroy()
{
	AkDelete(g_DefaultPoolId,this);
}

CAkRandomInfo::CAkRandomInfo(AkUInt16 in_wItemCount)
		:m_ulTotalWeight(DEFAULT_RANDOM_WEIGHT * in_wItemCount)
		,m_ulRemainingWeight(DEFAULT_RANDOM_WEIGHT * in_wItemCount)
		,m_wRemainingItemsToPlay(in_wItemCount)
		,m_wCounter(in_wItemCount)
		,m_pcArrayBeenPlayedFlag(NULL)
		,m_pcArrayBlockedFlag(NULL)
{
}

AKRESULT CAkRandomInfo::Init(AkUInt16 in_wAvoidRepeatCount)
{
	size_t AllocSize = (m_wCounter + 7) / 8;
	AKASSERT(AllocSize);

	// Allocate m_pcArrayBeenPlayedFlag and m_pcArrayBlockedFlag together

	m_pcArrayBeenPlayedFlag = (char*)AkAlloc( g_DefaultPoolId, AkUInt32( AllocSize ) * 2 );
	if( !m_pcArrayBeenPlayedFlag )
		return AK_Fail;

    m_pcArrayBlockedFlag = m_pcArrayBeenPlayedFlag + AllocSize;

	// Memset both arrays at once
	memset( m_pcArrayBeenPlayedFlag, 0, (AkUInt32)AllocSize * 2 );

	return m_listAvoid.Reserve(AkMin(in_wAvoidRepeatCount,MAX_LISTAVOID_SIZE));
}

CAkRandomInfo::~CAkRandomInfo()
{
	// This frees up m_pcArrayBlockedFlag as well
	if(m_pcArrayBeenPlayedFlag)
	{
		AkFree(g_DefaultPoolId,m_pcArrayBeenPlayedFlag); 
	}

	m_listAvoid.Term();
}

// Returns a reference to a new object allocated therein, which is a copy of this. NULL if failed.
CAkContainerBaseInfo * CAkRandomInfo::Clone( AkUInt16 in_wItemCount )
{
	if( in_wItemCount == 0 )
		return NULL;

	CAkRandomInfo * pClone = AkNew( g_DefaultPoolId, CAkRandomInfo( in_wItemCount ) );
	if ( pClone )
	{
		pClone->m_ulTotalWeight			= m_ulTotalWeight;
		pClone->m_ulRemainingWeight		= m_ulRemainingWeight;
		pClone->m_wRemainingItemsToPlay	= m_wRemainingItemsToPlay;
		pClone->m_wCounter				= m_wCounter;

		if ( m_pcArrayBeenPlayedFlag )
		{
			size_t AllocSize = (in_wItemCount + 7) / 8;
			AKASSERT(AllocSize);

			// Allocate m_pcArrayBeenPlayedFlag and m_pcArrayBlockedFlag together

			pClone->m_pcArrayBeenPlayedFlag = (char*)AkAlloc( g_DefaultPoolId, AkUInt32( AllocSize ) * 2 );
			if( !pClone->m_pcArrayBeenPlayedFlag )
			{
				pClone->Destroy();
				return NULL;
			}

			pClone->m_pcArrayBlockedFlag = pClone->m_pcArrayBeenPlayedFlag + AllocSize;
			memcpy( pClone->m_pcArrayBeenPlayedFlag, m_pcArrayBeenPlayedFlag, AkUInt32( AllocSize ) * 2 );
		}
		
		pClone->m_listAvoid.Reserve(m_listAvoid.Length());

		AKRESULT eResult = AK_Success;
		AkAvoidList::Iterator it = m_listAvoid.Begin();
		while ( it != m_listAvoid.End() )
		{
			if ( !pClone->m_listAvoid.AddLast( (*it) ) )
			{
				eResult = AK_Fail;
				break;
			}
			++it;
		}

		if ( eResult != AK_Success )
		{
			pClone->Destroy();
			return NULL;
		}
	}

	return pClone;
}

void CAkRandomInfo::FlagSetPlayed(AkUInt16 in_wPosition)
{
	m_pcArrayBeenPlayedFlag[in_wPosition/8] |= (1 << (in_wPosition%8));
}

void CAkRandomInfo::FlagUnSetPlayed(AkUInt16 in_wPosition)
{
	m_pcArrayBeenPlayedFlag[in_wPosition/8] &= (~(1 << (in_wPosition%8)));
}

bool CAkRandomInfo::IsFlagSetPlayed(AkUInt16 in_wPosition) const
{
	return ((m_pcArrayBeenPlayedFlag[in_wPosition/8] & (1 << (in_wPosition%8))) != 0);
}

void CAkRandomInfo::ResetFlagsPlayed(size_t in_PlaylistSize)
{
	size_t AllocSize = (in_PlaylistSize + 7) / 8;
	AKASSERT(AllocSize);
	memset(m_pcArrayBeenPlayedFlag,0,(AkUInt32)AllocSize*sizeof(char));
	m_wRemainingItemsToPlay = (AkUInt16)in_PlaylistSize;
}

void CAkRandomInfo::FlagAsBlocked(AkUInt16 in_wPosition)
{
	m_pcArrayBlockedFlag[in_wPosition/8] |= (1 << (in_wPosition%8));
}

void CAkRandomInfo::FlagAsUnBlocked(AkUInt16 in_wPosition)
{
	m_pcArrayBlockedFlag[in_wPosition/8] &= (~(1 << (in_wPosition%8)));
}

bool CAkRandomInfo::IsFlagBlocked(AkUInt16 in_wPosition) const
{
	return ((m_pcArrayBlockedFlag[in_wPosition/8] & (1 << (in_wPosition%8))) != 0);
}

AkContainerMode CAkRandomInfo::Type()
{
	return ContainerMode_Random;
}

AkInt CAkRandomInfo::GetRandomValue()
{
	// AkRandom() is returning a 15 bits random value, I need way more precise randomization system.
	AkInt randValue = AKRANDOM::AkRandom_30_bits();//returns 0 to AK_RANDOM_MAX_30_BITS
	return randValue % m_ulRemainingWeight;
}
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

CAkSequenceInfo::CAkSequenceInfo()
:m_bIsForward(true)
,m_i16LastPositionChosen(-1)
{
}

CAkSequenceInfo::~CAkSequenceInfo()
{
}

void CAkSequenceInfo::Destroy()
{
	AkDelete(g_DefaultPoolId,this);
}

AkContainerMode CAkSequenceInfo::Type()
{
	return ContainerMode_Sequence;
}

// Returns a reference to a new object allocated therein, which is a copy of this. NULL if failed.
CAkContainerBaseInfo * CAkSequenceInfo::Clone( AkUInt16 )
{
	CAkSequenceInfo * pClone = AkNew( g_DefaultPoolId, CAkSequenceInfo() );
	if ( pClone )
	{
		pClone->m_bIsForward			= m_bIsForward;
		pClone->m_i16LastPositionChosen	= m_i16LastPositionChosen;
	}
	return pClone;
}

