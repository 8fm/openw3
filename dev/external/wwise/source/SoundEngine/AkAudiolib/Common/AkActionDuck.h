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
// AkActionDuck.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_DUCK_H_
#define _ACTION_DUCK_H_

#include "AkAction.h"

class CAkActionDuck : public CAkAction
{

public:
	//Thread safe version of the constructor
	static CAkActionDuck* Create(AkActionType in_eActionType, AkUniqueID in_ulID = 0);

	virtual AKRESULT Execute(
		AkPendingAction * in_pAction
		);

	//Constructor
	CAkActionDuck(
		AkActionType in_eActionType,	//Type of action
		AkUniqueID in_ulID
		);

	//Destructor
	virtual ~CAkActionDuck();

protected:
	

};
#endif //_ACTION_DUCK_H_
