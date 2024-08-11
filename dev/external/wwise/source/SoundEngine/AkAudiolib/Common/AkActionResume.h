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
// AkActionResume.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_RESUME_H_
#define _ACTION_RESUME_H_

#include "AkActionActive.h"

class CAkParameterNodeBase;

class CAkActionResume : public CAkActionActive
{
public:
	//Thread safe version of the constructor
	static CAkActionResume* Create(AkActionType in_eActionType, AkUniqueID in_ulID = 0);

	//Execute the Action
	//Must be called only by the audiothread
	//
	// Return - AKRESULT - AK_Success if all succeeded
	virtual AKRESULT Execute(
		AkPendingAction * in_pAction
		);

	//Set the Action type of the current action
	virtual void ActionType(AkActionType in_ActionType);

	bool IsMasterResume(){ return m_bIsMasterResume; }

	void IsMasterResume( bool in_bIsMasterResume ){ m_bIsMasterResume = in_bIsMasterResume; }

	virtual AKRESULT SetActionSpecificParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

protected:
	CAkActionResume(AkActionType in_eActionType, AkUniqueID in_ulID);
	virtual ~CAkActionResume();
};
#endif
