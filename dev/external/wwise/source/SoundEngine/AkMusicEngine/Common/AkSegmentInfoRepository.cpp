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
// AkSegmentInfoRepository.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkKeyArray.h"
#include "AkSegmentInfoRepository.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkSettings.h"

//-----------------------------------------------------------------------------
// Defines.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Name: CAkPositionRepository
// Desc: Constructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSegmentInfoRepository::CAkSegmentInfoRepository()
{
}

//-----------------------------------------------------------------------------
// Name: ~CAkPositionRepository
// Desc: Destructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSegmentInfoRepository::~CAkSegmentInfoRepository()
{
}

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialize CAkSegmentInfoRepository.
//
// Return: Ak_Success
//-----------------------------------------------------------------------------
void CAkSegmentInfoRepository::Init()
{
}

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminates CAkSegmentInfoRepository.
//
// Return: Ak_Success
//-----------------------------------------------------------------------------
void CAkSegmentInfoRepository::Term()
{
	m_mapSegmentInfo.Term();
}

//-----------------------------------------------------------------------------
// Name: CreateEntry
// Desc: Create an entry for the specified playing ID.
//
// Return:	AK_Success
//			AK_Fail if a slot can't be allocated
//-----------------------------------------------------------------------------
AKRESULT CAkSegmentInfoRepository::CreateEntry( AkPlayingID in_PlayingID )
{
	AkAutoLock<CAkLock> gate( m_lock );

	// Note: (WG-17527) It may already exists if more than one PLAY action on a music node
	// exists in an event posted with flag AK_EnableGetMusicPlayPosition. In such a case, it doesn't matter:
	// we cannot index 2 context with 1 playing ID. The simplest is to replace the former entry with this one. It 
	//AKASSERT( !m_mapSegmentInfo.Exists( in_PlayingID ) 
	//		|| !"Playing ID already exists" );

	AkSegmentInfoRecord * pNewRecord = m_mapSegmentInfo.Set( in_PlayingID );

	if ( pNewRecord )
	{
		// Initialize Info structure with zeros.
		pNewRecord->segmentInfo.iCurrentPosition	= 0;
		pNewRecord->segmentInfo.iPreEntryDuration	= 0;
		pNewRecord->segmentInfo.iActiveDuration		= 0;
		pNewRecord->segmentInfo.iPostExitDuration	= 0;
		pNewRecord->segmentInfo.iRemainingLookAheadTime = 0;
		AKPLATFORM::PerformanceCounter( &pNewRecord->timeUpdated );
		return AK_Success;
	}

	return AK_Fail;
}

//-----------------------------------------------------------------------------
// Name: UpdateSegmentInfo
// Desc: Updates the segment information associated with a PlayingID.
//-----------------------------------------------------------------------------
void CAkSegmentInfoRepository::UpdateSegmentInfo( AkPlayingID in_PlayingID, const AkSegmentInfo & in_segmentInfo )
{
	AkAutoLock<CAkLock> gate( m_lock );

	AkSegmentInfoRecord * pRecord = m_mapSegmentInfo.Exists( in_PlayingID );
	if ( pRecord )
	{
		pRecord->segmentInfo = in_segmentInfo;
		AKPLATFORM::PerformanceCounter( &pRecord->timeUpdated );
	}
}

//-----------------------------------------------------------------------------
// Name: GetSegmentInfo
// Desc: Returns the segment info with a PlayingID. Performs interpolation to
//		return the most accurate info possible.
//
// Return: AK_Success, AK_Fail if structure doesn't exist yet
//-----------------------------------------------------------------------------
AKRESULT CAkSegmentInfoRepository::GetSegmentInfo( AkPlayingID in_PlayingID, AkSegmentInfo & out_info, bool in_bExtrapolate )
{
	AkAutoLock<CAkLock> gate(m_lock);
	AkSegmentInfoRecord * pRecord = m_mapSegmentInfo.Exists( in_PlayingID );
	if ( !pRecord )
		return AK_Fail;

	out_info = pRecord->segmentInfo;

	// Interpolate position since last update if the active segment is not nothing
	if ( in_bExtrapolate
		&& ( out_info.iActiveDuration > 0 
			|| out_info.iPreEntryDuration > 0
			|| out_info.iPostExitDuration > 0 ) )
	{
		AkInt64 currTime;
		AKPLATFORM::PerformanceCounter( &currTime );
		AkReal32 fElapsed = AKPLATFORM::Elapsed( currTime, pRecord->timeUpdated );

		out_info.iCurrentPosition += (AkTimeMs)fElapsed;
	}

	// NOTE: Do not clamp end of segment. Firstly, because we don't know if it is the Exit Cue or Post-Exit region,
	// and secondly, because it gives the user a clue that we might actually be a few samples within the next active segment.
	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: RemoveEntry
// Desc: Removes the position information associated with a PlayingID.
//
// Return: AK_Success
//-----------------------------------------------------------------------------
void CAkSegmentInfoRepository::RemoveEntry( AkPlayingID in_PlayingID )
{
	AkAutoLock<CAkLock> gate(m_lock);
	m_mapSegmentInfo.Unset( in_PlayingID );
}
