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
// AkActionBypassFX.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_BYPASS_FX_H_
#define _ACTION_BYPASS_FX_H_

#include "AkActionExcept.h"
#include "AkRegisteredObj.h"

class CAkActionBypassFX : public CAkActionExcept
{
public:
	//Thread safe version of the constructor
	static CAkActionBypassFX* Create(AkActionType in_eActionType, AkUniqueID in_ulID = 0);

	void Bypass(
		const bool in_bIsBypass
		);

	void SetBypassTarget(
		bool in_bTargetAll, 
		AkUInt32 in_uTargetMask
		);

	virtual AKRESULT SetActionParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

protected:
	//Constructor
	CAkActionBypassFX(AkActionType in_eActionType, AkUniqueID in_ulID);

	//Destructor
	virtual ~CAkActionBypassFX();
public:

	//Execute the Action
	//Must be called only by the audiothread
	//
	// Return - AKRESULT - AK_Success if all succeeded
	virtual AKRESULT Execute(
		AkPendingAction * in_pAction
		);

private:

	void ResetBypassFxAllHelper( const AkListNode* in_pListID );
	void ResetBypassFXAEHelper( const AkListNode* in_pListID );

	bool m_bIsBypass;
	AkUInt32 m_uTargetMask;
};

#endif // _ACTION_BYPASS_FX_H_
