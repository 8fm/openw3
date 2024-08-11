/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../core/scriptStackFrame.h"
#include "../core/scriptingSystem.h"
#include "gameTimeManager.h"

/// Event in game time
class CScriptTimeEvent : public ITimeEvent
{
protected:
	String m_functionWithParams;
	
public:
	CScriptTimeEvent( const THandle<IScriptable>& context, GameTime date, GameTime period, Int32 limit, Bool relative, String functionWithParams )
		: ITimeEvent( context, date, period, limit, relative )
		, m_functionWithParams( functionWithParams )
	{
	}
	
	//! Fire event
	virtual void OnEvent( CNode* caller, const CName& eventName, void* params )
	{
		if ( m_functionWithParams.Empty() )
			return;

		GScriptingSystem->CallLocalFunction( GetOwner().Get(), m_functionWithParams, true );
	}
};

void funcScheduleTimeEvent( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<IScriptable>, object, NULL );
	GET_PARAMETER( String, functionWithParams, String::EMPTY );
	GET_PARAMETER( GameTime, date, GameTime() );
	GET_PARAMETER_OPT( Bool, relative, false );
	GET_PARAMETER_OPT( GameTime, period, GameTime() );
	GET_PARAMETER_OPT( Int32, limit, 0 );
	FINISH_PARAMETERS;

	if ( functionWithParams.Empty() )
		return;
	
	GGame->GetTimeManager()->AddEvent( new CScriptTimeEvent( object, date, period, limit, relative, functionWithParams ) );
}
