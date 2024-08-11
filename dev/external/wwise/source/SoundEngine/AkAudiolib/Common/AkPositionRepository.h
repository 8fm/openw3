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
// AkPositionRepository.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_POSITION_REPOSITORY_H_
#define _AK_POSITION_REPOSITORY_H_

#include "AkKeyArray.h"
#include <AK/Tools/Common/AkLock.h>

#define AK_POSITION_REPOSITORY_MIN_NUM_INSTANCES 8

struct AkPositionInfo
{
	AkBufferPosInformation	bufferPosInfo;	//Buffer position information
	AkInt64					timeUpdated;	//Last time information was updated
	void *					cookie;			//Unique caller ID to avoid clash between 2 sources
};

class CAkPositionRepository
{
public:
	//initialization
	AKRESULT Init();
	void Term();

	//Public Methods
	AKRESULT GetCurrPosition( AkPlayingID in_PlayingID, AkTimeMs* out_puPos, bool in_bExtrapolate );
	void UpdatePositionInfo( AkPlayingID in_PlayingID, AkBufferPosInformation* in_pPosInfo, void* in_cookie );
	void AddSource( AkPlayingID in_PlayingID, void * in_cookie );
	void RemoveSource( AkPlayingID in_PlayingID, void * in_cookie );
	void RemovePlayingID( AkPlayingID in_PlayingID );

	// Update the timer, should be called once per frame.
	inline void UpdateTime()
	{
		if( !m_mapPosInfo.IsEmpty() )
			AKPLATFORM::PerformanceCounter( &m_i64LastTimeUpdated );
	}

	// SetRate, mostly used for paused/resumed sounds.
	void SetRate( AkPlayingID in_PlayingID, AkReal32 in_fNewRate );

private:
	CAkKeyArray<AkPlayingID, AkPositionInfo> m_mapPosInfo;

	CAkLock m_lock;
	AkInt64 m_i64LastTimeUpdated;
};

extern CAkPositionRepository* g_pPositionRepository;

#endif //_AK_POSITION_REPOSITORY_H_
