/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiAction.h"

#if 0 // this awaits IScriptable submit from dex, to be refactored after this submit

class CAIActionScripted : public CAIAction
{
	DECLARE_AI_ACTION_CLASS( CAIActionScripted, CAIAction )

protected:
	Bool m_ticked;

	// no handle to any kind of a component here; the idea is the action is scripted, so script side holds the eventual component handle

public:
	CAIActionScripted();	
	
protected:
	// CAIAction interface
	virtual Bool CanBeStartedOn( CComponent* component ) const	override;
	virtual EAIActionStatus StartOn( CComponent* component )	override;
	virtual EAIActionStatus Stop( EAIActionStatus newStatus )	override;
	virtual EAIActionStatus Tick( Float timeDelta )				override;
	virtual EAIActionStatus Reset()								override;
	virtual Bool ShouldBeTicked() const							override { return m_ticked; }

private:
	void funcStop( CScriptStackFrame& stack, void* result );
	void funcGetStatus( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CAIActionScripted )
	PARENT_CLASS( CAIAction )
	PROPERTY( m_ticked ) // set from script	

	NATIVE_FUNCTION( "Stop", funcStop )
	NATIVE_FUNCTION( "GetStatus", funcGetStatus )
END_CLASS_RTTI()

#endif