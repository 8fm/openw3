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
// AkSegmentInfoRepository.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_SEGMENT_INFO_REPOSITORY_H_
#define _AK_SEGMENT_INFO_REPOSITORY_H_

#include <AK/MusicEngine/Common/AkMusicEngine.h>
#include <AK/Tools/Common/AkLock.h>
#include "AkKeyArray.h"

struct AkSegmentInfoRecord
{
	AkSegmentInfo			segmentInfo;	// Segment information
	AkInt64					timeUpdated;	// Last time information was updated
};

class CAkSegmentInfoRepository
{
public:
	CAkSegmentInfoRepository(); //constructor
	~CAkSegmentInfoRepository(); //destructor

	void	 Init();
	void	 Term();

	//Public Methods
	AKRESULT CreateEntry( AkPlayingID in_PlayingID );
	void	 UpdateSegmentInfo( AkPlayingID in_PlayingID, const AkSegmentInfo & in_segmentInfo );
	AKRESULT GetSegmentInfo( AkPlayingID in_PlayingID, AkSegmentInfo & out_info, bool in_bExtrapolate );
	void	 RemoveEntry( AkPlayingID in_PlayingID );

private:
	CAkKeyArray<AkPlayingID, AkSegmentInfoRecord> m_mapSegmentInfo;

	CAkLock m_lock;
};

#endif //_AK_SEGMENT_INFO_REPOSITORY_H_
