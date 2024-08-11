/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/scriptable.h"
#include "../engine/inGameConfigInterface.h"

class CInGameConfigWrapper : public IScriptable
{
public:
	DECLARE_RTTI_OBJECT_CLASS( CInGameConfigWrapper );

public:
	const String GetConfigValue( CName& groupConfigId, CName& variableConfigId );
	const String GetConfigValueConst( CName groupConfigId, CName variableConfigId ) const;
	void SetConfigValue( CName& groupConfigId, CName& variableConfigId, String& value );

private:
	InGameConfig::IConfigVar* GetConfigVar( CName& groupConfigid, CName& varConfigid );
	InGameConfig::IConfigGroup* GetConfigGroup( CName& groupConfigid );

	// Script functions
	void funcGetConfigValue( CScriptStackFrame& stack, void* result );
	void funcSetConfigValue( CScriptStackFrame& stack, void* result );

	void funcGetConfigVarDisplayType( CScriptStackFrame& stack, void* result );
	void funcGetConfigVarDisplayName( CScriptStackFrame& stack, void* result );
	void funcGetConfigVarOptionsNum( CScriptStackFrame& stack, void* result );
	void funcGetConfigOption( CScriptStackFrame& stack, void* result );

	void funcGetConfigGroupDisplayName( CScriptStackFrame& stack, void* result );
	void funcGetConfigGroupPresetsNum( CScriptStackFrame& stack, void* result );
	void funcGetConfigGroupPresetDisplayName( CScriptStackFrame& stack, void* result );
	void funcApplyConfigGroupPreset( CScriptStackFrame& stack, void* result );

	void funcGetVarnameByGroupName( CScriptStackFrame& stack, void* result );
	void funcGetVarsNumByGroupName( CScriptStackFrame& stack, void* result );

	void funcGetConfigGroupsNum( CScriptStackFrame& stack, void* result );
	void funcGetConfigGroupName( CScriptStackFrame& stack, void* result );

	void funcGetConfigVarsNum( CScriptStackFrame& stack, void* result );
	void funcGetConfigVarName( CScriptStackFrame& stack, void* result );
	void funcIsVarVisible( CScriptStackFrame& stack, void* result );

	void funcHasTag( CScriptStackFrame& stack, void* result );
	void funcIsTagActive( CScriptStackFrame& stack, void* result );
	void funcActivateScriptTag( CScriptStackFrame& stack, void* result );
	void funcDeactivateScriptTag( CScriptStackFrame& stack, void* result );

	void funcGroupHasTag( CScriptStackFrame& stack, void* result );
	void funcGroupIsVisible( CScriptStackFrame& stack, void* result );

	void funcResetGroupToDefaults( CScriptStackFrame& stack, void* result );

};
BEGIN_CLASS_RTTI( CInGameConfigWrapper );
	PARENT_CLASS( IScriptable );
	NATIVE_FUNCTION( "GetGroupDisplayName", funcGetConfigGroupDisplayName );
	NATIVE_FUNCTION( "GetGroupPresetsNum", funcGetConfigGroupPresetsNum );
	NATIVE_FUNCTION( "GetGroupPresetDisplayName", funcGetConfigGroupPresetDisplayName );
	NATIVE_FUNCTION( "ApplyGroupPreset", funcApplyConfigGroupPreset );

	NATIVE_FUNCTION( "GetVarDisplayType", funcGetConfigVarDisplayType );
	NATIVE_FUNCTION( "GetVarDisplayName", funcGetConfigVarDisplayName );
	NATIVE_FUNCTION( "GetVarOptionsNum", funcGetConfigVarOptionsNum );
	NATIVE_FUNCTION( "GetVarOption", funcGetConfigOption );
	NATIVE_FUNCTION( "GetVarValue", funcGetConfigValue );
	NATIVE_FUNCTION( "SetVarValue", funcSetConfigValue );
	NATIVE_FUNCTION( "GetVarNameByGroupName", funcGetVarnameByGroupName );
	NATIVE_FUNCTION( "GetVarsNumByGroupName", funcGetVarsNumByGroupName );

	NATIVE_FUNCTION( "GetGroupsNum", funcGetConfigGroupsNum );
	NATIVE_FUNCTION( "GetGroupName", funcGetConfigGroupName );
	NATIVE_FUNCTION( "GetVarsNum", funcGetConfigVarsNum );
	NATIVE_FUNCTION( "GetVarName", funcGetConfigVarName );
	NATIVE_FUNCTION( "IsVarVisible", funcIsVarVisible );
	
	NATIVE_FUNCTION( "DoVarHasTag", funcHasTag );
	NATIVE_FUNCTION( "IsTagActive", funcIsTagActive );
	NATIVE_FUNCTION( "ActivateScriptTag", funcActivateScriptTag );
	NATIVE_FUNCTION( "DeactivateScriptTag", funcDeactivateScriptTag );

	NATIVE_FUNCTION( "DoGroupHasTag", funcGroupHasTag );
	NATIVE_FUNCTION( "IsGroupVisible", funcGroupIsVisible );

	NATIVE_FUNCTION( "ResetGroupToDefaults", funcResetGroupToDefaults );

END_CLASS_RTTI();
