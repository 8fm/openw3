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
// AkActionSetState.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_SET_STATE_H_
#define _ACTION_SET_STATE_H_

#include "AkAction.h"

class CAkActionSetState : public CAkAction
{
public:
	//Thread safe version of the constructor
	static CAkActionSetState* Create(AkActionType in_eActionType, AkUniqueID in_ulID = 0);

	// Set the channel of the Action
	void SetStateGroup(
		const AkStateGroupID in_ulStateGroupID //StateGroup ID
		);

	// Set the Target state of the Action
	void SetTargetState(
		const AkStateID in_ulStateID //Target State
		);

#ifndef AK_OPTIMIZED
	// Set to skip transition or not
	void SetSkipTransition(
		bool in_bSkipTransition // Skip transition and go directly to state ?
		);
#endif

	virtual AKRESULT SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

protected:
	//Constructor
	CAkActionSetState(AkActionType in_eActionType, AkUniqueID in_ulID);

	//Destructor
	virtual ~CAkActionSetState();

public:

	//Execute the Action
	//Must be called only by the audiothread
	//
	// Return - AKRESULT - AK_Success if all succeeded
	virtual AKRESULT Execute(
		AkPendingAction * in_pAction
		);

private:
	AkStateGroupID m_ulStateGroupID;// StateGroup ID
	AkStateID m_ulTargetStateID; //Target State ID

#ifndef AK_OPTIMIZED
	bool m_bSkipTransition;
#endif
};
#endif
