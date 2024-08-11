
#include "build.h"
#include "scriptEngineAnimationNatives.h"
#include "animatedComponentScripts.h"
#include "../core/scriptStackFrame.h"
#include "skeletalAnimationEntry.h"


static void funcResetAnimatedComponentSyncSettings( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SAnimatedComponentSyncSettings, settings, SAnimatedComponentSyncSettings() );
	FINISH_PARAMETERS;

	settings = SAnimatedComponentSyncSettings();
}

static void funcResetAnimatedComponentSlotAnimationSettings( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SAnimatedComponentSlotAnimationSettings, settings, SAnimatedComponentSlotAnimationSettings() );
	FINISH_PARAMETERS;

	settings = SAnimatedComponentSlotAnimationSettings();
}

static void funcGetAnimNameFromEventAnimInfo( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimationEventAnimInfo, eventAnimInfo, SAnimationEventAnimInfo() );
	FINISH_PARAMETERS;

	if ( eventAnimInfo.m_animation &&
		 eventAnimInfo.m_animation->GetAnimation() )
	{
		RETURN_NAME( eventAnimInfo.m_animation->GetAnimation()->GetName() );
	}
	else
	{
		RETURN_NAME( CName::NONE );
	}
}

static void funcGetLocalAnimTimeFromEventAnimInfo( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimationEventAnimInfo, eventAnimInfo, SAnimationEventAnimInfo() );
	FINISH_PARAMETERS;

	RETURN_FLOAT( eventAnimInfo.m_localTime );
}

static void funcGetEventEndsAtTimeFromEventAnimInfo( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimationEventAnimInfo, eventAnimInfo, SAnimationEventAnimInfo() );
	FINISH_PARAMETERS;

	RETURN_FLOAT( eventAnimInfo.m_eventEndsAtTime );
}

static void funcGetEventDurationFromEventAnimInfo( CObject* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimationEventAnimInfo, eventAnimInfo, SAnimationEventAnimInfo() );
	FINISH_PARAMETERS;

	RETURN_FLOAT( eventAnimInfo.m_eventDuration );
}

void ExportEngineAnimationNatives()
{
	NATIVE_GLOBAL_FUNCTION( "ResetAnimatedComponentSyncSettings", funcResetAnimatedComponentSyncSettings );
	NATIVE_GLOBAL_FUNCTION( "ResetAnimatedComponentSlotAnimationSettings", funcResetAnimatedComponentSlotAnimationSettings );
	NATIVE_GLOBAL_FUNCTION( "GetAnimNameFromEventAnimInfo", funcGetAnimNameFromEventAnimInfo );
	NATIVE_GLOBAL_FUNCTION( "GetLocalAnimTimeFromEventAnimInfo", funcGetLocalAnimTimeFromEventAnimInfo );
	NATIVE_GLOBAL_FUNCTION( "GetEventEndsAtTimeFromEventAnimInfo", funcGetEventEndsAtTimeFromEventAnimInfo );
	NATIVE_GLOBAL_FUNCTION( "GetEventDurationFromEventAnimInfo", funcGetEventDurationFromEventAnimInfo );
}
