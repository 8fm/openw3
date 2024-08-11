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

#include "AkAction.h"
#include "ObjectProxyConnected.h"

class ActionProxyConnected : public ObjectProxyConnected
{
public:
	ActionProxyConnected( AkActionType in_eActionType, AkUniqueID in_uID );
	virtual ~ActionProxyConnected();

	virtual void HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	DECLARE_BASECLASS( ObjectProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSimpleProxyConnected : public ActionProxyConnected
{
public:
	AkForceInline ActionSimpleProxyConnected( AkActionType in_actionType, AkUniqueID in_id ) : ActionProxyConnected( in_actionType, in_id ){}

	DECLARE_BASECLASS( ActionProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionExceptProxyConnected : public ActionProxyConnected
{
public:
	AkForceInline ActionExceptProxyConnected( AkActionType in_actionType, AkUniqueID in_id ) : ActionProxyConnected( in_actionType, in_id ){}

	virtual void HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	DECLARE_BASECLASS( ActionProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionStopProxyConnected : public ActionExceptProxyConnected
{
public:
	AkForceInline ActionStopProxyConnected( AkActionType in_actionType, AkUniqueID in_id ) : ActionExceptProxyConnected( in_actionType, in_id ){}

	DECLARE_BASECLASS( ActionExceptProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionPauseProxyConnected : public ActionExceptProxyConnected
{
public:
	AkForceInline ActionPauseProxyConnected( AkActionType in_actionType, AkUniqueID in_id ) : ActionExceptProxyConnected( in_actionType, in_id ){}

	virtual void HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	DECLARE_BASECLASS( ActionExceptProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionResumeProxyConnected : public ActionExceptProxyConnected
{
public:
	AkForceInline ActionResumeProxyConnected( AkActionType in_actionType, AkUniqueID in_id ) : ActionExceptProxyConnected( in_actionType, in_id ){}

	virtual void HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	DECLARE_BASECLASS( ActionExceptProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionBreakProxyConnected : public ActionProxyConnected
{
public:
	AkForceInline ActionBreakProxyConnected( AkActionType in_actionType, AkUniqueID in_id ) : ActionProxyConnected( in_actionType, in_id ){}

	DECLARE_BASECLASS( ActionExceptProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionMuteProxyConnected : public ActionExceptProxyConnected
{
public:
	AkForceInline ActionMuteProxyConnected( AkActionType in_actionType, AkUniqueID in_id ) : ActionExceptProxyConnected( in_actionType, in_id ){}

	DECLARE_BASECLASS( ActionExceptProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetAkPropProxyConnected : public ActionExceptProxyConnected
{
public:
	AkForceInline ActionSetAkPropProxyConnected( AkActionType in_actionType, AkUniqueID in_id ) : ActionExceptProxyConnected( in_actionType, in_id ){}

	virtual void HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	DECLARE_BASECLASS( ActionExceptProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetStateProxyConnected : public ActionProxyConnected
{
public:
	AkForceInline ActionSetStateProxyConnected( AkActionType in_actionType, AkUniqueID in_id ) : ActionProxyConnected( in_actionType, in_id ){}

	virtual void HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	DECLARE_BASECLASS( ActionProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetSwitchProxyConnected : public ActionProxyConnected
{
public:
	AkForceInline ActionSetSwitchProxyConnected( AkActionType in_actionType, AkUniqueID in_id ) : ActionProxyConnected( in_actionType, in_id ){}

	virtual void HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	DECLARE_BASECLASS( ActionProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetGameParameterProxyConnected : public ActionExceptProxyConnected
{
public:
	AkForceInline ActionSetGameParameterProxyConnected( AkActionType in_actionType, AkUniqueID in_id ) : ActionExceptProxyConnected( in_actionType, in_id ){}

	virtual void HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	DECLARE_BASECLASS( ActionExceptProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionUseStateProxyConnected : public ActionProxyConnected
{
public:
	AkForceInline ActionUseStateProxyConnected( AkActionType in_actionType, AkUniqueID in_id ) : ActionProxyConnected( in_actionType, in_id ){}

	virtual void HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	DECLARE_BASECLASS( ActionProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionBypassFXProxyConnected : public ActionProxyConnected
{
public:
	AkForceInline ActionBypassFXProxyConnected( AkActionType in_actionType, AkUniqueID in_id ) : ActionProxyConnected( in_actionType, in_id ){}

	virtual void HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	DECLARE_BASECLASS( ActionProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSeekProxyConnected : public ActionProxyConnected
{
public:
	AkForceInline ActionSeekProxyConnected( AkActionType in_actionType, AkUniqueID in_id ) : ActionProxyConnected( in_actionType, in_id ){}

	virtual void HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	DECLARE_BASECLASS( ActionProxyConnected );
};

#endif // #ifndef AK_OPTIMIZED
