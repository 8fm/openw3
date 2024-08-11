
#include "build.h"

static void funcResetAnimatedSlideSettings( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SAnimatedSlideSettings, settings, SAnimatedSlideSettings() );
	FINISH_PARAMETERS;

	settings = SAnimatedSlideSettings();
}

static void funcResetActionMatchToSettings( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SActionMatchToSettings, settings, SActionMatchToSettings() );
	FINISH_PARAMETERS;

	settings = SActionMatchToSettings();
}

static void funcSetActionMatchToTarget_StaticPoint( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SActionMatchToTarget, target, SActionMatchToTarget() );
	GET_PARAMETER( Vector, point, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Float, yaw, 0.f );
	GET_PARAMETER( Bool, position, true );
	GET_PARAMETER( Bool, rotation, true );
	FINISH_PARAMETERS;

	target = SActionMatchToTarget();
	target.Set( point, yaw, position, rotation );
}

void ExportAnimationNatives()
{
	extern void ExportEngineAnimationNatives();
	ExportEngineAnimationNatives();

	NATIVE_GLOBAL_FUNCTION( "ResetAnimatedSlideSettings", funcResetAnimatedSlideSettings );
	NATIVE_GLOBAL_FUNCTION( "ResetActionMatchToSettings", funcResetActionMatchToSettings );
	NATIVE_GLOBAL_FUNCTION( "SetActionMatchToTarget_StaticPoint", funcSetActionMatchToTarget_StaticPoint );
}
