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
#ifndef PROXYCENTRAL_CONNECTED

#include "ParameterNodeProxyLocal.h"
#include "ISwitchContainerProxy.h"

class SwitchContainerProxyLocal : public ParameterNodeProxyLocal
								, virtual public ISwitchContainerProxy
{

public:

	SwitchContainerProxyLocal( AkUniqueID in_id );
	virtual ~SwitchContainerProxyLocal();

	virtual void SetSwitchGroup( AkUInt32 in_ulGroup, AkGroupType in_eGroupType );

	virtual void SetDefaultSwitch( AkUInt32 in_switch );

	virtual void ClearSwitches();

	virtual void AddSwitch( AkSwitchStateID in_switch );
	virtual void RemoveSwitch( AkSwitchStateID in_switch );

	virtual void AddNodeInSwitch(
		AkUInt32			in_switch,
		AkUniqueID		in_nodeID
		);

	virtual void RemoveNodeFromSwitch(
		AkUInt32			in_switch,
		AkUniqueID		in_nodeID
		);

	virtual void SetContinuousValidation( bool in_bIsContinuousCheck );

	virtual void SetContinuePlayback( AkUniqueID in_NodeID, bool in_bContinuePlayback );
	virtual void SetFadeInTime( AkUniqueID in_NodeID, AkTimeMs in_time );
	virtual void SetFadeOutTime( AkUniqueID in_NodeID, AkTimeMs in_time );
	virtual void SetOnSwitchMode( AkUniqueID in_NodeID, AkOnSwitchMode in_bSwitchMode );
	virtual void SetIsFirstOnly( AkUniqueID in_NodeID, bool in_bIsFirstOnly );
};
#endif
#endif // #ifndef AK_OPTIMIZED
