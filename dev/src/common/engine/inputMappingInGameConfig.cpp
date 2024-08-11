/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "inputMappingInGameConfig.h"
#include "game.h"
#include "inputManager.h"

CInputMappingInGameConfigGroup::CInputMappingInGameConfigGroup( const ConstructParams& params )
	: InGameConfig::CConfigGroupBasic( params.basicParams )
	, m_vars( params.vars )
{
	/* Intentionall empty */
}

void CInputMappingInGameConfigGroup::ListConfigVars(TDynArray< InGameConfig::IConfigVar* >& output)
{
	output.PushBack( m_vars );
}

void CInputMappingInGameConfigGroup::ResetToDefault()
{
	GGame->GetInputManager()->ReloadSettings();
}

void CInputMappingInGameConfigGroup::Discard()
{
	InGameConfig::CConfigGroupBasic::Discard();
	m_vars.ClearPtr();
}

CInputMappingInGameConfigVar::CInputMappingInGameConfigVar(const ConstructParams& params)
	: InGameConfig::CConfigVarBasic( params.basicParams )
	, m_isPadInput( params.isPadInput )
	, m_actions( params.actions )
{
	/* Intentionally empty */
}

const String CInputMappingInGameConfigVar::GetDisplayType() const
{
	if( m_isPadInput == true )
	{
		return TXT("INPUTPAD");
	}

	return TXT("INPUTPC");
}

const InGameConfig::CConfigVarValue CInputMappingInGameConfigVar::GetValue() const
{	
	String resultStr;

	if( m_isPadInput == true )
	{
		resultStr = GetKeysStringForPad();
	}
	else
	{
		resultStr = GetKeysStringForKeyboardMouse();
	}

	return InGameConfig::CConfigVarValue( resultStr );
}

void CInputMappingInGameConfigVar::SetValue(const InGameConfig::CConfigVarValue value, const InGameConfig::EConfigVarSetType setType)
{
	if( m_isPadInput == true )
	{
		SetPadValue( value );
	}
	else
	{
		SetKeyboardMouseValue( value );
	}
}

void CInputMappingInGameConfigVar::ResetToDefault()
{
	/* Intentionally empty */
}

const String CInputMappingInGameConfigVar::GetKeysStringForPad() const
{
	TDynArray< EInputKey > keys;
	if( m_actions.Size() > 0 )		// Check just fist action for that config - assuming all actions in one group have the same bindings
	{
		GGame->GetInputManager()->GetPadKeysForAction( m_actions[0], keys );
	}

	CEnum* keyEnum = SRTTI::GetInstance().FindEnum( CNAME( EInputKey ) );

	for( EInputKey key : keys )
	{
		if( IsPadKey( key ) == true )
		{
			CName keyName;
			keyEnum->FindName( (Int32)key, keyName );
			return keyName.AsString();
		}
	}

	return TXT("IK_None");
}

const String CInputMappingInGameConfigVar::GetKeysStringForKeyboardMouse() const
{
	TDynArray< EInputKey > keys;
	if( m_actions.Size() > 0 )		// Check just fist action for that config - assuming all actions in one group have the same bindings
	{
		GGame->GetInputManager()->GetPCKeysForAction( m_actions[0], keys );
	}

	CEnum* keyEnum = SRTTI::GetInstance().FindEnum( CNAME( EInputKey ) );

	Int32 keyCounter = 0;

	String result;

	for( EInputKey key : keys )
	{
		if( keyCounter > 1 )
		{
			break;
		}

		if( IsPadKey( key ) == false )
		{
			if( keyCounter > 0 )
			{
				result += TXT(";");
			}

			CName keyName;
			keyEnum->FindName( (Int32)key, keyName );
			result += keyName.AsString();
			keyCounter++;
		}
	}

	if( keyCounter == 0 )
	{
		result = TXT("IK_None;IK_None");
	}
	else if( keyCounter == 1 )
	{
		result += TXT(";IK_None");
	}

	return result;
}

void CInputMappingInGameConfigVar::SetPadValue(const InGameConfig::CConfigVarValue value)
{
	String valueStr = value.GetAsString();

	CEnum* keyEnum = SRTTI::GetInstance().FindEnum( CNAME( EInputKey ) );

	EInputKey key = IK_None;
	keyEnum->FindValue( CName( valueStr ), (Int32&)key );

	for( CName action : m_actions )
	{
		GGame->GetInputManager()->SetPadActionBinding( action, key );
	}
}

void CInputMappingInGameConfigVar::SetKeyboardMouseValue(const InGameConfig::CConfigVarValue value)
{
	String valueStr = value.GetAsString();

	String mainKeyStr = valueStr.StringBefore( TXT(";") );
	String alternativeKeyStr = valueStr.StringAfter( TXT(";") );

	CEnum* keyEnum = SRTTI::GetInstance().FindEnum( CNAME( EInputKey ) );

	EInputKey mainKey = IK_None;
	keyEnum->FindValue( CName( mainKeyStr ), (Int32&)mainKey );

	EInputKey alternativeKey = IK_None;
	keyEnum->FindValue( CName( alternativeKeyStr ), (Int32&)alternativeKey );

	for( CName action : m_actions )
	{
		GGame->GetInputManager()->SetKeyboardMouseActionBinding( action, mainKey, alternativeKey );
	}
}
