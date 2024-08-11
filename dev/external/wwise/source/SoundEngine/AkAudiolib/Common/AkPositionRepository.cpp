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
// AkPositionRepository.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkCommon.h"
#include "AkPositionRepository.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

//-----------------------------------------------------------------------------
// Defines.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialize CAkPositionRepository.
//
// Return: Ak_Success
//-----------------------------------------------------------------------------
AKRESULT CAkPositionRepository::Init()
{
	return m_mapPosInfo.Reserve( AK_POSITION_REPOSITORY_MIN_NUM_INSTANCES );
}

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminates CAkPositionRepository.
//-----------------------------------------------------------------------------
void CAkPositionRepository::Term()
{
	m_mapPosInfo.Term();
}

//-----------------------------------------------------------------------------
// Name: SetRate
// Desc: Update the last rate for a given ID, useful when sounds gets paused/resumed.
//
//-----------------------------------------------------------------------------
void CAkPositionRepository::SetRate( AkPlayingID in_PlayingID, AkReal32 in_fNewRate )
{
	AkPositionInfo* pPosInfo = m_mapPosInfo.Exists( in_PlayingID );
	if( pPosInfo )
	{
		AkAutoLock<CAkLock> gate(m_lock);
		pPosInfo->timeUpdated = m_i64LastTimeUpdated;
		pPosInfo->bufferPosInfo.fLastRate = in_fNewRate;
	}
}

//-----------------------------------------------------------------------------
// Name: AddSource
// Desc: Register a playing ID _with a cookie (source)_ for position information.
//-----------------------------------------------------------------------------
void CAkPositionRepository::AddSource( AkPlayingID in_PlayingID, void * in_cookie )
{
	if( m_mapPosInfo.Exists( in_PlayingID ) )
	{
		// There is already an entry for this playing ID. The caller must be another sound 
		// in the same event. Its position is ignored.
		return;
	}
	
	AkAutoLock<CAkLock> gate(m_lock);

	MapStruct<AkPlayingID, AkPositionInfo> * pNewEntry = m_mapPosInfo.AddLast();
	if ( pNewEntry )
	{
		pNewEntry->key = in_PlayingID;
		pNewEntry->item.cookie = in_cookie;

		// Initialize uSampleRate. As long as it is 1, the structure's content is not used/read.
		pNewEntry->item.bufferPosInfo.Clear();
	}
}

//-----------------------------------------------------------------------------
// Name: RemoveSource
// Desc: Unregister a playing ID for position information only if cookie (source)_ matches.
//-----------------------------------------------------------------------------
void CAkPositionRepository::RemoveSource( AkPlayingID in_PlayingID, void * in_cookie )
{
	CAkKeyArray<AkPlayingID, AkPositionInfo>::Iterator it = m_mapPosInfo.FindEx( in_PlayingID );
	if ( it != m_mapPosInfo.End() )
	{
		if ( (*it).item.cookie == in_cookie )
		{
			AkAutoLock<CAkLock> gate(m_lock);
			m_mapPosInfo.Erase( it );
		}
	}
}

//-----------------------------------------------------------------------------
// Name: UpdatePositionInfo
// Desc: Updates the position information associated with a PlayingID.
//-----------------------------------------------------------------------------
void CAkPositionRepository::UpdatePositionInfo( AkPlayingID in_PlayingID, AkBufferPosInformation* in_pPosInfo, void* in_cookie )
{
	AkPositionInfo* pPosInfo = m_mapPosInfo.Exists( in_PlayingID );
	
	AkAutoLock<CAkLock> gate(m_lock);

	if( !pPosInfo )
	{
		pPosInfo = m_mapPosInfo.Set( in_PlayingID );
		if( !pPosInfo )
			return;

		UpdateTime();
		pPosInfo->cookie = in_cookie;
	}	
	else if ( in_cookie != pPosInfo->cookie )
		return; //not the original caller
	
	pPosInfo->bufferPosInfo = *in_pPosInfo;
	pPosInfo->timeUpdated = m_i64LastTimeUpdated;
}

//-----------------------------------------------------------------------------
// Name: GetCurrPosition
// Desc: Returns the interpolated position associated with a PlayingID.
//
// Return: AK_Success, AK_Fail if structure doesn't exist yet
//-----------------------------------------------------------------------------
AKRESULT CAkPositionRepository::GetCurrPosition( AkPlayingID in_PlayingID, AkTimeMs* out_puPos, bool in_bExtrapolate )
{
	AkAutoLock<CAkLock> gate(m_lock);
	AkPositionInfo* pPosInfo = m_mapPosInfo.Exists( in_PlayingID );
	if( !pPosInfo 
		|| pPosInfo->bufferPosInfo.uSampleRate == 1 )
	{
		// If sample rate == 1, the entry has been added but no update has been performed yet. (see AkBufferPosInformation::Clear)
		*out_puPos = 0;
		return AK_Fail;
	}

	AkReal32 fPosition = (pPosInfo->bufferPosInfo.uStartPos*1000.f)/pPosInfo->bufferPosInfo.uSampleRate;
	AkUInt32 uEndFileTime = (AkUInt32)((pPosInfo->bufferPosInfo.uFileEnd*1000.f)/pPosInfo->bufferPosInfo.uSampleRate);

	if ( in_bExtrapolate )
	{
		//extrapolation using timer
		AkInt64 CurrTime;
		AKPLATFORM::PerformanceCounter( &CurrTime );
		AkReal32 fElapsed = AKPLATFORM::Elapsed( CurrTime, pPosInfo->timeUpdated );

		fPosition += (fElapsed * pPosInfo->bufferPosInfo.fLastRate);	
	}

	AkUInt32 uPosition = (AkUInt32)fPosition;

	*out_puPos = AkMin( uPosition, uEndFileTime );

	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: RemovePlayingID
// Desc: Removes the position information associated with a PlayingID.
//
// Return: AK_Success
//-----------------------------------------------------------------------------
void CAkPositionRepository::RemovePlayingID( AkPlayingID in_PlayingID )
{
	AkAutoLock<CAkLock> gate(m_lock);
	m_mapPosInfo.Unset( in_PlayingID );
}
