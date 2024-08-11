/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameTimeInterval.h"
#include "../core/scriptStackFrame.h"


IMPLEMENT_ENGINE_CLASS( GameTimeInterval );

static void funcGameTimeIntervalContainsTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( GameTime, gameTime, 0 );
	GET_PARAMETER( GameTimeInterval, gameTimeInterval, GameTimeInterval() );
	GET_PARAMETER_OPT( Bool, ignoreDays, false );
	FINISH_PARAMETERS;

	RETURN_BOOL( gameTimeInterval.DoesContainTime( gameTime, ignoreDays ) );
}

void ExportEngineGameTimeIntervalNatives()
{
	NATIVE_GLOBAL_FUNCTION("GameTimeIntervalContainsTime", funcGameTimeIntervalContainsTime );
}