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
// AkEvent.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _EVENT_H_
#define _EVENT_H_

#include <AK/Tools/Common/AkListBareLight.h>
#include "AkIndexable.h"
#include "AkPoolSizes.h"
#include <AK/SoundEngine/Common/AkQueryParameters.h>

class CAkAction;

//Event object
//The event mainly contains a set of actions to be executed
class CAkEvent : public CAkIndexable
{
	friend class CAkAudioMgr;
	friend class CAkActionEvent;
	friend class CAkBankMgr;

public:
	//Destructor
	virtual ~CAkEvent();

	//Thread safe version of the constructor
	static CAkEvent* Create(AkUniqueID in_ulID = 0);

	static CAkEvent* CreateNoIndex(AkUniqueID in_ulID = 0);

protected:
	//constructor
	CAkEvent(AkUniqueID in_ulID);

public:
	//Add an action at the end of the action list
	//
	//Return - AKRESULT - AK_Success if succeeded
	AKRESULT Add(
		AkUniqueID in_ulAction//Action to be added
		);

	//remove an action from the action list
	//
	//Return - AKRESULT - AK_Success if succeeded
	void Remove(
		AkUniqueID in_ulAction//Action ID to be removed
		);

	//clear the action list
	void Clear();

	void AddToIndex();
	void RemoveFromIndex();

	virtual AkUInt32 AddRef();
	virtual AkUInt32 Release();

	AKRESULT SetInitialValues( AkUInt8* pData, AkUInt32 ulDataSize );

	AKRESULT QueryAudioObjectIDs( AkUInt32& io_ruNumItems, AkObjectInfo* out_aObjectInfos );

	bool IsPrepared(){ return m_iPreparationCount != 0; }
	void IncrementPreparedCount(){ ++m_iPreparationCount; }
	void DecrementPreparedCount(){ --m_iPreparationCount; }
	void FlushPreparationCount() { m_iPreparationCount = 0; }

	AkUInt32 GetPreparationCount(){ return m_iPreparationCount; }
	
private:

	AKRESULT AddAfter( AkUniqueID in_ulAction, CAkAction*& io_pPrevious );

	typedef AkListBareLight<CAkAction> AkActionList;
	AkActionList m_actions;//Action list contained in the event

	AkUInt32 m_iPreparationCount;

};
#endif
