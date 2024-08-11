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
// AkPreparationAware.h
// Basic interface and implementation for containers that are 
// switch/state dependent.
//
//////////////////////////////////////////////////////////////////////
#ifndef _PREPARATION_AWARE_H_
#define _PREPARATION_AWARE_H_

#include "AkSwitchAware.h"
#include <AK/Tools/Common/AkArray.h>

class CAkPreparedContent
{
public:
	CAkPreparedContent()
	{
	}

	~CAkPreparedContent()
	{
		m_PreparableContentList.Term();
	}

	bool IsIncluded( AkUInt32 in_GameSync )
	{
		return ( m_PreparableContentList.FindEx( in_GameSync ) != m_PreparableContentList.End() );
	}

	typedef AkArray<AkUInt32, AkUInt32, ArrayPoolDefault, 4> ContentList;
	ContentList m_PreparableContentList;
};

class CAkPreparationAware
{
public:
	//ListBare usage reserved
	CAkPreparationAware* pNextItemPrepare;

protected:
	AkUInt32 m_uPreparationCount;

public:

	CAkPreparationAware()
		:pNextItemPrepare( NULL )
		,m_uPreparationCount(0)
	{}

	virtual AKRESULT ModifyActiveState( 
		AkUInt32 in_stateID,
		bool in_bSupported
        ) = 0;

	CAkPreparedContent* GetPreparedContent( 
        AkUInt32 in_ulGroup, 
        AkGroupType in_eGroupType 
        );

protected:

	AKRESULT SubscribePrepare( 
        AkUInt32 in_ulGroup, 
        AkGroupType in_eGroupType 
        );
	
    void UnsubscribePrepare(
		AkUInt32 in_ulGroup, 
        AkGroupType in_eGroupType 
		);
};
#endif //_PREPARATION_AWARE_H_
