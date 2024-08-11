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
// AkActionSetValue.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_SET_VALUE_H_
#define _ACTION_SET_VALUE_H_

#include "AkActionExcept.h"
#include "AkParameters.h"
#include "AkRegisteredObj.h"

class CAkParameterNodeBase;

class CAkActionSetValue : public CAkActionExcept
{

protected:
	//Constructor
	CAkActionSetValue(AkActionType in_eActionType, AkUniqueID in_ulID);

	//Destructor
	virtual ~CAkActionSetValue();

	virtual AKRESULT SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

protected:

	//Execute the Action
	//Must be called only by the audiothread
	//
	// Return - AKRESULT - AK_Success if all succeeded
	virtual AKRESULT Execute(
		AkPendingAction * in_pAction
		);

	// Sets the value on the Member element
	virtual void ExecSetValue(
		CAkParameterNodeBase* in_pNode	// Target Parameter Node
		) = 0;

	// Sets the value on the Object specified Member element
	virtual void ExecSetValue(
		CAkParameterNodeBase* in_pNode,	// Target Parameter Node
		CAkRegisteredObj * in_pGameObj	// Target Game Element
		) = 0;

	// Resets the value on the Member element
	virtual void ExecResetValue(
		CAkParameterNodeBase* in_pNode	// Target Parameter Node
		) = 0;

	// Resets the value on the Object specified Member element
	virtual void ExecResetValue(
		CAkParameterNodeBase* in_pNode,	// Target Parameter Node
		CAkRegisteredObj * in_pGameObj	// Target Game Object
		) = 0;

	// Resets the value on the Member element of both global and object specific
	virtual void ExecResetValueAll(
		CAkParameterNodeBase* in_pNode	// Target Parameter Node
		) = 0;

	// Resets the value on the Member element of both global and object specific
	virtual void ExecResetValueExcept(
		CAkParameterNodeBase* in_pNode	// Target Parameter Node
		) = 0;

	// Resets the value on the Member element of both global and object specific
	virtual void ExecResetValueExcept(
		CAkParameterNodeBase* in_pNode,	// Target Parameter Node
		CAkRegisteredObj * in_pGameObj	// Target Game Object
		) = 0;

private:

	void ResetAllHelper( const AkListNode* in_pListID );
	void ResetAEHelper( const AkListNode* in_pListID );
};
#endif
