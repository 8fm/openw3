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
// AkActionMute.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_MUTE_H_
#define _ACTION_MUTE_H_

#include "AkActionSetValue.h"

class CAkActionMute : public CAkActionSetValue
{
public:
	//Thread safe version of the constructor
	static CAkActionMute* Create(AkActionType in_eActionType, AkUniqueID in_ulID = 0);

	//Set the Action type of the current action
	virtual void ActionType(AkActionType in_ActionType);

protected:
	CAkActionMute(AkActionType in_eActionType, AkUniqueID in_ulID);
	virtual ~CAkActionMute();

protected:

	// Sets the value on the Member element
	virtual void ExecSetValue(
		CAkParameterNodeBase* in_pNode	// Target Parameter Node
		);

	// Sets the value on the Object specified Member element
	virtual void ExecSetValue(
		CAkParameterNodeBase* in_pNode,	// Target Parameter Node
		CAkRegisteredObj * in_pGameObj	// Target Game Element
		);

	// Resets the value on the Member element
	virtual void ExecResetValue(
		CAkParameterNodeBase* in_pNode	// Target Parameter Node
		);

	// Resets the value on the Object specified Member element
	virtual void ExecResetValue(
		CAkParameterNodeBase* in_pNode,	// Target Parameter Node
		CAkRegisteredObj * in_pGameObj		// Target Game Object
		);

	// Resets the value on the Member element of both global and object specific
	virtual void ExecResetValueAll(
		CAkParameterNodeBase* in_pNode	// Target Parameter Node
		);

	virtual void ExecResetValueExcept(
		CAkParameterNodeBase* in_pNode	// Target Parameter Node
		);

	virtual void ExecResetValueExcept(
		CAkParameterNodeBase* in_pNode,	// Target Parameter Node
		CAkRegisteredObj * in_pGameObj	// Target Game Object
		);


private:
	
};
#endif
