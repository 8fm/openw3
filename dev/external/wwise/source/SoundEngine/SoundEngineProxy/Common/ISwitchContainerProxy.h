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
#pragma once

#ifndef AK_OPTIMIZED

#include "IParameterNodeProxy.h"

#include "AkSwitchCntr.h"

class CAkSwitchCntr;


class ISwitchContainerProxy : virtual public IParameterNodeProxy
{
public:

	virtual void SetSwitchGroup( AkUInt32 in_ulGroup, AkGroupType in_eGroupType ) = 0;

	virtual void SetDefaultSwitch( AkUInt32 in_switch ) = 0;

	virtual void ClearSwitches() = 0;

	virtual void AddSwitch( AkSwitchStateID in_switch ) = 0;
	virtual void RemoveSwitch( AkSwitchStateID in_switch ) = 0;

	virtual void AddNodeInSwitch(
		AkUInt32			in_switch,
		AkUniqueID		in_nodeID
		) = 0;

	virtual void RemoveNodeFromSwitch(
		AkUInt32			in_switch,
		AkUniqueID		in_nodeID
		) = 0;

	virtual void SetContinuousValidation( bool in_bIsContinuousCheck ) = 0;

	virtual void SetContinuePlayback( AkUniqueID in_NodeID, bool in_bContinuePlayback ) = 0;
	virtual void SetFadeInTime( AkUniqueID in_NodeID, AkTimeMs in_time ) = 0;
	virtual void SetFadeOutTime( AkUniqueID in_NodeID, AkTimeMs in_time ) = 0;
	virtual void SetOnSwitchMode( AkUniqueID in_NodeID, AkOnSwitchMode in_bSwitchMode ) = 0;
	virtual void SetIsFirstOnly( AkUniqueID in_NodeID, bool in_bIsFirstOnly ) = 0;



	enum MethodIDs
	{
		MethodSetSwitchGroup = IParameterNodeProxy::LastMethodID,
		MethodSetDefaultSwitch,
		MethodClearSwitches,
		MethodAddSwitch,
		MethodRemoveSwitch,
		MethodAddNodeInSwitch,
		MethodRemoveNodeFromSwitch,
		MethodSetContinuousValidation,
		MethodSetContinuePlayback,
		MethodSetFadeInTime,
		MethodSetFadeOutTime,
		MethodSetOnSwitchMode,
		MethodSetIsFirstOnly,

		LastMethodID
	};
};

#endif // #ifndef AK_OPTIMIZED
