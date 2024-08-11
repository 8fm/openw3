/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/core/object.h"

class IRedTelemetryServiceInterface;

enum ER4TelemetryEvents
{
#	define EVENT( x, cat, name ) x,
#	include "r4Events.enum"
#	undef EVENT
	TE_COUNT
};

enum ER4CommonStats
{
#	define STAT( x, name ) x,
#	include "r4CommonStats.enum"
#	undef STAT

	CS_COUNT
};

BEGIN_ENUM_RTTI( ER4TelemetryEvents );

#	define EVENT( x, cat, name ) ENUM_OPTION( x );
#	include "r4Events.enum"
#	undef EVENT

END_ENUM_RTTI();

BEGIN_ENUM_RTTI( ER4CommonStats );

#	define STAT( x, name ) ENUM_OPTION( x );
#	include "r4CommonStats.enum"
#	undef STAT

END_ENUM_RTTI();

class CR4TelemetryScriptProxy : public CObject
{
	DECLARE_ENGINE_CLASS( CR4TelemetryScriptProxy, CObject, 0 );

public:

	void funcLog( CScriptStackFrame& stack, void* result );
	void funcLogWithLabel( CScriptStackFrame& stack, void* result );
	void funcLogWithValue( CScriptStackFrame& stack, void* result );
	void funcLogWithValueStr( CScriptStackFrame& stack, void* result );
	void funcLogWithLabelAndValue( CScriptStackFrame& stack, void* result );
	void funcLogWithLabelAndValueStr( CScriptStackFrame& stack, void* result );

	void funcSetCommonStatFlt( CScriptStackFrame& stack, void* result );
	void funcSetCommonStatI32( CScriptStackFrame& stack, void* result );

	void funcSetGameProgress( CScriptStackFrame& stack, void* result );

	void funcAddSessionTag( CScriptStackFrame& stack, void* result );
	void funcRemoveSessionTag( CScriptStackFrame& stack, void* result );

	void funcXDPPrintUserStats( CScriptStackFrame& stack, void* result );
	void funcXDPPrintUserAchievement( CScriptStackFrame& stack, void* result );

};

BEGIN_CLASS_RTTI( CR4TelemetryScriptProxy )
	PARENT_CLASS( CObject )
	NATIVE_FUNCTION( "LogWithName", funcLog );
	NATIVE_FUNCTION( "LogWithLabel", funcLogWithLabel );

	NATIVE_FUNCTION( "LogWithValue", funcLogWithValue );
	NATIVE_FUNCTION( "LogWithValueStr", funcLogWithValueStr );

	NATIVE_FUNCTION( "LogWithLabelAndValue", funcLogWithLabelAndValue );
	NATIVE_FUNCTION( "LogWithLabelAndValueStr", funcLogWithLabelAndValueStr );

	NATIVE_FUNCTION( "SetCommonStatFlt", funcSetCommonStatFlt );
	NATIVE_FUNCTION( "SetCommonStatI32", funcSetCommonStatI32 );

	NATIVE_FUNCTION( "SetGameProgress", funcSetGameProgress );

	NATIVE_FUNCTION( "AddSessionTag", funcAddSessionTag );
	NATIVE_FUNCTION( "RemoveSessionTag", funcRemoveSessionTag );

	NATIVE_FUNCTION( "XDPPrintUserStats", funcXDPPrintUserStats );
	NATIVE_FUNCTION( "XDPPrintUserAchievement", funcXDPPrintUserAchievement );
END_CLASS_RTTI();