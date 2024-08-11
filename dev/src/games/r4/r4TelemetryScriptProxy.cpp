/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/redTelemetryServiceInterface.h"

#include "r4Telemetry.h"
#include "r4TelemetryScriptProxy.h"

IMPLEMENT_RTTI_ENUM( ER4CommonStats );
IMPLEMENT_RTTI_ENUM( ER4TelemetryEvents );
IMPLEMENT_ENGINE_CLASS( CR4TelemetryScriptProxy );

void CR4TelemetryScriptProxy::funcLog( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ER4TelemetryEvents, r4event, TE_UNKNOWN );
	FINISH_PARAMETERS;

#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
		telemetrySystem->Log( r4event );
#endif // NO_TELEMETYRY
}

void CR4TelemetryScriptProxy::funcLogWithLabel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ER4TelemetryEvents, r4event, TE_UNKNOWN );
	GET_PARAMETER( String, label, TXT( "" ) );
	FINISH_PARAMETERS;

#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
		telemetrySystem->LogL( r4event, label );
#endif // NO_TELEMETRY
}

void CR4TelemetryScriptProxy::funcLogWithValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ER4TelemetryEvents, r4event, TE_UNKNOWN );
	GET_PARAMETER( Int32, val, 0 );
	FINISH_PARAMETERS;

#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
		telemetrySystem->LogV( r4event, val );
#endif // NO_TELEMETRY
}

void CR4TelemetryScriptProxy::funcLogWithValueStr( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ER4TelemetryEvents, r4event, TE_UNKNOWN );
	GET_PARAMETER( String, val, TXT( "" ) );
	FINISH_PARAMETERS;

#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
		telemetrySystem->LogV( r4event, val );
#endif // NO_TELEMETRY
}

void CR4TelemetryScriptProxy::funcLogWithLabelAndValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ER4TelemetryEvents, r4event, TE_UNKNOWN );
	GET_PARAMETER( String, label, TXT( "" ) );
	GET_PARAMETER( Int32, val, 0 );
	FINISH_PARAMETERS;

#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
		telemetrySystem->LogVL( r4event, val, label );
#endif // NO_TELEMETRY
}

void CR4TelemetryScriptProxy::funcLogWithLabelAndValueStr( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ER4TelemetryEvents, r4event, TE_UNKNOWN );
	GET_PARAMETER( String, label, TXT( "" ) );
	GET_PARAMETER( String, val, TXT( "" ) );
	FINISH_PARAMETERS;

#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
		telemetrySystem->LogVL( r4event, val, label );
#endif // NO_TELEMETRY
}


void CR4TelemetryScriptProxy::funcSetCommonStatFlt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ER4CommonStats, r4event, CS_UNKNOWN );
	GET_PARAMETER( Float, val, 0.0f );
	FINISH_PARAMETERS;

#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
		telemetrySystem->SetStatValue( r4event, val );
#endif // NO_TELEMETRY
}

void CR4TelemetryScriptProxy::funcSetGameProgress( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, perc, 0.0f );
	FINISH_PARAMETERS;

#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
	{
		telemetrySystem->SetStatValue( CS_GAME_PROGRESS, perc );
		telemetrySystem->Log( TE_SYS_GAME_PROGRESS );
	}
#endif // NO_TELEMETRY
}

void CR4TelemetryScriptProxy::funcSetCommonStatI32( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ER4CommonStats, r4event, CS_UNKNOWN );
	GET_PARAMETER( Int32, val, 0 );
	FINISH_PARAMETERS;

#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
		telemetrySystem->SetStatValue( r4event, val );
#endif // NO_TELEMETRY
}

void CR4TelemetryScriptProxy::funcAddSessionTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, tag, TXT( "" ) );
	FINISH_PARAMETERS;

#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
		telemetrySystem->AddSessionTag(tag);
#endif // NO_TELEMETRY
}

void CR4TelemetryScriptProxy::funcRemoveSessionTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, tag, TXT( "" ) );
	FINISH_PARAMETERS;

#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
		telemetrySystem->RemoveSessionTag(tag);
#endif // NO_TELEMETRY
}

void CR4TelemetryScriptProxy::funcXDPPrintUserStats( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, stat, TXT( "" ) );
	FINISH_PARAMETERS;
	GUserProfileManager->PrintUserStats( stat );
}

void CR4TelemetryScriptProxy::funcXDPPrintUserAchievement( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, achievement, TXT( "" ) );
	FINISH_PARAMETERS;
	GUserProfileManager->PrintUserAchievement( achievement );
}