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
// AkActionUseState.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_USE_STATE_H_
#define _ACTION_USE_STATE_H_

#include "AkAction.h"

class CAkActionUseState : public CAkAction
{
public:
	//Thread safe version of the constructor
	static CAkActionUseState* Create(AkActionType in_eActionType, AkUniqueID in_ulID = 0);

	//Set the Action type of the current action
	virtual void ActionType(AkActionType in_ActionType);

protected:
	//Constructor
	CAkActionUseState(AkActionType in_eActionType, AkUniqueID in_ulID);

	//Destructor
	virtual ~CAkActionUseState();

public:

	//Execute the Action
	//Must be called only by the audiothread
	//
	// Return - AKRESULT - AK_Success if all succeeded
	virtual AKRESULT Execute(
		AkPendingAction * in_pAction
		);

	// Allow to switch the state from Use stata to UnUseState
	void UseState(
		bool in_bUseState
		);
};
#endif
