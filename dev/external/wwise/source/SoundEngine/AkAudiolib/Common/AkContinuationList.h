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
// AkContinuationList.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _CONTINUATION_LIST_H_
#define _CONTINUATION_LIST_H_

#include "AkParameters.h"
#include <AK/Tools/Common/AkArray.h>
#include "AkPoolSizes.h"
#include <AK/Tools/Common/AkSmartPtr.h>

class CAkRanSeqCntr;
class CAkMultiPlayNode;
class CAkContinuationList;
class CAkContainerBaseInfo;

// This is the number of buffers we wait in a continuation list before
// attempting to play the next item when an item fails. The goal is to
// avoid making lots of very rapid attempts that keep failing.
#if defined(AK_WII_FAMILY_HW) | defined(AK_3DS)
#define AK_WAIT_BUFFERS_AFTER_PLAY_FAILED		(80)
#else
#define AK_WAIT_BUFFERS_AFTER_PLAY_FAILED		(10)
#endif

class CAkContinueListItem
{
public:
	//Constructor
	CAkContinueListItem();

	//Destructor
	~CAkContinueListItem();

	const CAkContinueListItem& operator=(const CAkContinueListItem & in_listItem);

// Members are public since they all have to be used by everybody
public:
	CAkSmartPtr<CAkRanSeqCntr> m_pContainer;			// Pointer to the container
	CAkContainerBaseInfo* m_pContainerInfo;	// Container info
											// Owned by this class
	AkLoop m_LoopingInfo;					// Looping info
	CAkSmartPtr<CAkMultiPlayNode> m_pMultiPlayNode;
	CAkContinuationList* m_pAlternateContList;
};

class CAkContinuationList
{
protected:
	CAkContinuationList();

public:
	//MUST Assign the returned value of create into a CAkSmartPtr.
	static CAkContinuationList* Create();

	typedef AkArray<CAkContinueListItem, const CAkContinueListItem&, ArrayPoolDefault, 2> AkContinueListItem;
	AkContinueListItem m_listItems;
	void AddRef();
	void Release();

private:
	void Term();
	AkInt32 m_iRefCount;
};

#endif
