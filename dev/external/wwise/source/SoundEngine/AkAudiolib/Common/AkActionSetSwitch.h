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
// AkActionSetSwitch.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_SET_SWITCH_H_
#define _ACTION_SET_SWITCH_H_

#include "AkAction.h"

class CAkActionSetSwitch : public CAkAction
{
public:
	// Thread safe version of the constructor
	static CAkActionSetSwitch* Create( AkActionType in_eActionType, AkUniqueID in_ulID = 0 );

	// Set the channel of the Action
	void SetSwitchGroup(
		const AkSwitchGroupID in_ulSwitchGroupID //SwitchGroup ID
		);

	// Set the Target switch of the Action
	void SetTargetSwitch(
		const AkSwitchStateID in_ulSwitchID //Target State
		);

	virtual AKRESULT SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

protected:
	// Constructor
	CAkActionSetSwitch( AkActionType in_eActionType, AkUniqueID in_ulID );

	// Destructor
	virtual ~CAkActionSetSwitch();

public:

	// Execute the Action
	// Must be called only by the audiothread
	//
	// Return - AKRESULT - AK_Success if all succeeded
	virtual AKRESULT Execute(
		AkPendingAction * in_pAction
		);

private:
	AkSwitchGroupID m_ulSwitchGroupID; // StateGroup ID
	AkSwitchStateID m_ulSwitchStateID; // Target State ID
};

#endif // _ACTION_SET_SWITCH_H_
