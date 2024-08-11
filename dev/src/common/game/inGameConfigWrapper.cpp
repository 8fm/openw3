/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "inGameConfigWrapper.h"
#include "../engine/inGameConfigInterface.h"
#include "../engine/inGameConfig.h"

IMPLEMENT_ENGINE_CLASS( CInGameConfigWrapper );

const String CInGameConfigWrapper::GetConfigValue(CName& groupConfigId, CName& variableConfigId)
{
	auto var = GetConfigVar( groupConfigId, variableConfigId );
	if( var == nullptr )
		return TXT("");
	return var->GetValue().GetAsString();
}

const String CInGameConfigWrapper::GetConfigValueConst( CName groupConfigId, CName variableConfigId ) const
{
	CInGameConfigWrapper* castedThis = const_cast< CInGameConfigWrapper* >(this);
	return castedThis->GetConfigValue(groupConfigId, variableConfigId);
}

void CInGameConfigWrapper::SetConfigValue(CName& groupConfigId, CName& variableConfigId, String& value)
{
	auto var = GetConfigVar( groupConfigId, variableConfigId );
	if( var != nullptr )
	{
		InGameConfig::CConfigVarValue configNewValue( value );
		if( var->GetValue().GetAsString() != configNewValue.GetAsString() )
		{
			var->SetValue( configNewValue, InGameConfig::eConfigVarAccessType_UserAction );
		}
	}
}

InGameConfig::IConfigVar* CInGameConfigWrapper::GetConfigVar(CName& groupConfigid, CName& varConfigid)
{
	auto group = GetConfigGroup( groupConfigid );
	
	if( group != nullptr )
	{	
		TDynArray< InGameConfig::IConfigVar* > vars;
		group->ListConfigVars( vars );

		for( Uint32 i=0; i<vars.Size(); ++i )
		{
			if( vars[i]->GetConfigId() == varConfigid )
			{
				return vars[i];
			}
		}
	}

	RED_ASSERT( false, TXT("InGame Config: there is no variable with cname: %ls in group with cname: %ls"), varConfigid.AsChar(), groupConfigid.AsChar() );

	return nullptr;
}

InGameConfig::IConfigGroup* CInGameConfigWrapper::GetConfigGroup(CName& groupConfigid)
{
	TDynArray< InGameConfig::IConfigGroup* > groups;
	GInGameConfig::GetInstance().ListAllConfigGroups( groups );

	for( Uint32 i=0; i<groups.Size(); ++i )
	{
		if( groups[i]->GetConfigId() == groupConfigid )
		{
			return groups[i];
		}
	}

	RED_ASSERT( false, TXT("InGame Config: there is no group with cname: %ls"), groupConfigid.AsChar() );

	return nullptr;
}

void CInGameConfigWrapper::funcGetConfigValue(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	GET_PARAMETER( CName, varName, CName::NONE );
	FINISH_PARAMETERS;

	String resultStr = GetConfigValue( groupName, varName );

	RETURN_STRING( resultStr );
}

void CInGameConfigWrapper::funcSetConfigValue(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	GET_PARAMETER( CName, varName, CName::NONE );
	GET_PARAMETER( String, varValue, String::EMPTY );
	FINISH_PARAMETERS;

	SetConfigValue( groupName, varName, varValue );

	RETURN_VOID()
}

void CInGameConfigWrapper::funcGetConfigVarDisplayType(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	GET_PARAMETER( CName, varName, CName::NONE );
	FINISH_PARAMETERS;

	auto var = GetConfigVar( groupName, varName );
	if( var != nullptr )
	{
		TDynArray<String> typeSplitted = var->GetDisplayType().Split( TXT(";") );		// for options like SLIDER;min;max;resolution
		if( typeSplitted.Size() > 0 )
		{
			RETURN_STRING( typeSplitted[0] );		// first element is type itself
			return;
		}
	}

	RETURN_STRING( TXT("NONE") );
}

void CInGameConfigWrapper::funcGetConfigVarDisplayName(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	GET_PARAMETER( CName, varName, CName::NONE );
	FINISH_PARAMETERS;

	auto var = GetConfigVar( groupName, varName );
	if( var != nullptr )
	{
		RETURN_STRING( var->GetDisplayName() );
		return;
	}

	RETURN_STRING( TXT("NONE") );
}

void CInGameConfigWrapper::funcGetConfigVarOptionsNum(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	GET_PARAMETER( CName, varName, CName::NONE );
	FINISH_PARAMETERS;

	TDynArray< InGameConfig::SConfigPresetOption > options;
	auto var = GetConfigVar( groupName, varName );
	if ( !var )
	{
		RETURN_INT( 0 );
		return;
	}

	var->ListOptions( options );

	if( options.Size() > 0 )
	{
		RETURN_INT( options.Size() );
		return;
	}
	
	TDynArray<String> typeSplitted = var->GetDisplayType().Split( TXT(";") );		// for options like SLIDER;min;max;resolution
	if( typeSplitted.Size() > 0 )
	{
		RETURN_INT( typeSplitted.Size()-1 );		// -1 because first element is type itself
		return;
	}

	RETURN_INT( 0 );
}

void CInGameConfigWrapper::funcGetConfigOption(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	GET_PARAMETER( CName, varName, CName::NONE );
	GET_PARAMETER( Int32, optionIdx, 0 );
	FINISH_PARAMETERS;

	TDynArray< InGameConfig::SConfigPresetOption > options;
	auto var = GetConfigVar( groupName, varName );
	if ( !var )
	{
		RETURN_STRING( TXT("") );
		return;
	}

	var->ListOptions( options );

	if( Uint32( optionIdx ) < options.Size() )
	{
		RETURN_STRING( options[optionIdx].displayName );
		return;
	}

	TDynArray<String> typeSplitted = var->GetDisplayType().Split( TXT(";") );		// for options like SLIDER;min;max;resolution
	if( Uint32( optionIdx+1 ) < typeSplitted.Size() )
	{
		RETURN_STRING( typeSplitted[optionIdx+1] );		// -1 because first element is type itself
		return;
	}

	RETURN_STRING( TXT("") );
}

void CInGameConfigWrapper::funcGetConfigGroupDisplayName(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	FINISH_PARAMETERS;

	auto group = GetConfigGroup( groupName );

	if( group != nullptr )
	{
		RETURN_STRING( group->GetDisplayName() );
		return;
	}
	
	RETURN_STRING( TXT("None") );
}

void CInGameConfigWrapper::funcGetConfigGroupPresetsNum(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	FINISH_PARAMETERS;

	auto group = GetConfigGroup( groupName );

	if( group != nullptr )
	{
		TDynArray< InGameConfig::SConfigPresetOption > presets;
		group->ListPresets( presets );

		RETURN_INT( presets.Size() );
		return;
	}

	RETURN_INT( 0 );
}

void CInGameConfigWrapper::funcGetConfigGroupPresetDisplayName(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	GET_PARAMETER( Int32, presetIdx, 0 );
	FINISH_PARAMETERS;

	auto group = GetConfigGroup( groupName );

	if( group != nullptr )
	{
		TDynArray< InGameConfig::SConfigPresetOption > presets;
		group->ListPresets( presets );

		RETURN_STRING( presets[presetIdx].displayName );
		return;
	}

	RETURN_STRING( TXT("None") );
}

void CInGameConfigWrapper::funcApplyConfigGroupPreset(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	GET_PARAMETER( Int32, presetIdx, 0 );
	FINISH_PARAMETERS;

	auto group = GetConfigGroup( groupName );

	if( group != nullptr )
	{
		if( group->GetActivePreset() != presetIdx )
		{
			group->ApplyPreset( presetIdx, InGameConfig::eConfigVarAccessType_UserAction );
		}
	}

	RETURN_VOID()
}

void CInGameConfigWrapper::funcGetConfigGroupsNum(CScriptStackFrame& stack, void* result)
{
	FINISH_PARAMETERS;

	TDynArray< InGameConfig::IConfigGroup* > groups;
	GInGameConfig::GetInstance().ListAllConfigGroups( groups );

	RETURN_INT( groups.Size() );
}

void CInGameConfigWrapper::funcGetConfigGroupName(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( Int32, groupIdx, 0 );
	FINISH_PARAMETERS;

	TDynArray< InGameConfig::IConfigGroup* > groups;
	GInGameConfig::GetInstance().ListAllConfigGroups( groups );

	RETURN_NAME( groups[groupIdx]->GetConfigId() );
}

void CInGameConfigWrapper::funcGetConfigVarsNum(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( Int32, groupIdx, 0 );
	FINISH_PARAMETERS;

	TDynArray< InGameConfig::IConfigGroup* > groups;
	GInGameConfig::GetInstance().ListAllConfigGroups( groups );

	if ( groupIdx < 0 || groupIdx >= groups.SizeInt() )
	{
		RETURN_INT( 0 );
		return;
	}

	TDynArray< InGameConfig::IConfigVar* > vars;
	groups[groupIdx]->ListConfigVars( vars );

	RETURN_INT( vars.Size() );
}

void CInGameConfigWrapper::funcGetConfigVarName(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( Int32, groupIdx, 0 );
	GET_PARAMETER( Int32, varIdx, 0 );
	FINISH_PARAMETERS;

	TDynArray< InGameConfig::IConfigGroup* > groups;
	GInGameConfig::GetInstance().ListAllConfigGroups( groups );

	if ( groupIdx < 0 || groupIdx >= groups.SizeInt() )
	{
		RETURN_NAME( CName::NONE );
		return;
	}

	TDynArray< InGameConfig::IConfigVar* > vars;
	groups[groupIdx]->ListConfigVars( vars );

	RETURN_NAME( vars[varIdx]->GetConfigId() );
}

void CInGameConfigWrapper::funcGetVarnameByGroupName(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	GET_PARAMETER( Int32, varIdx, 0 );
	FINISH_PARAMETERS;

	auto group = GetConfigGroup( groupName );

	if( group != nullptr )
	{
		TDynArray< InGameConfig::IConfigVar* > vars;
		group->ListConfigVars( vars );

		if( Uint32(varIdx) < vars.Size() )
		{
			RETURN_NAME( vars[varIdx]->GetConfigId() );
			return;
		}
	}

	RETURN_NAME( CName::NONE );
}

void CInGameConfigWrapper::funcGetVarsNumByGroupName(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	FINISH_PARAMETERS;

	auto group = GetConfigGroup( groupName );

	if( group != nullptr )
	{
		TDynArray< InGameConfig::IConfigVar* > vars;
		group->ListConfigVars( vars );

		RETURN_INT( vars.Size() );
		return;
	}

	RETURN_INT( 0 );
}

void CInGameConfigWrapper::funcHasTag(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	GET_PARAMETER( CName, varName, CName::NONE );
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	Bool funcResult = false;
	auto var = GetConfigVar( groupName, varName );

	if( var != nullptr )
	{
		funcResult = var->HasTag( tag );
	}

	RETURN_BOOL( funcResult );
}

void CInGameConfigWrapper::funcIsTagActive(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	Bool funcResult = GInGameConfig::GetInstance().IsTagActive( tag );

	RETURN_BOOL( funcResult );
}

void CInGameConfigWrapper::funcIsVarVisible(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	GET_PARAMETER( CName, varName, CName::NONE );
	FINISH_PARAMETERS;

	Bool funcResult = false;

	auto var = GetConfigVar( groupName, varName );

	if( var != nullptr )
	{
		funcResult = var->IsVisible();
	}

	RETURN_BOOL( funcResult );
}

void CInGameConfigWrapper::funcActivateScriptTag(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	//RED_ASSERT( tag.AsString().BeginsWith(TXT("#")), TXT("InGameConfig: tag \'%ls\' is invalid. Tags from scripts must have \'#\' prefix."), tag.AsChar() );

	GInGameConfig::GetInstance().ActivateTag( tag );

	RETURN_VOID();
}

void CInGameConfigWrapper::funcDeactivateScriptTag(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	//RED_ASSERT( tag.AsString().BeginsWith(TXT("#")), TXT("InGameConfig: tag \'%ls\' is invalid. Tags from scripts must have \'#\' prefix."), tag.AsChar() );

	GInGameConfig::GetInstance().DeactivateTag( tag );

	RETURN_VOID();
}

void CInGameConfigWrapper::funcGroupHasTag(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	Bool funcResult = false;
	auto group = GetConfigGroup( groupName );

	if( group != nullptr )
	{
		funcResult = group->HasTag( tag );
	}

	RETURN_BOOL( funcResult );
}

void CInGameConfigWrapper::funcGroupIsVisible(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	FINISH_PARAMETERS;

	Bool funcResult = false;
	auto group = GetConfigGroup( groupName );

	if( group != nullptr )
	{
		funcResult = group->IsVisible();
	}

	RETURN_BOOL( funcResult );
}

void CInGameConfigWrapper::funcResetGroupToDefaults( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, groupName, CName::NONE );
	FINISH_PARAMETERS;

	InGameConfig::IConfigGroup* group = GetConfigGroup( groupName );

	if( group )
	{
		group->ResetToDefault();
	}

	RETURN_VOID();
}
