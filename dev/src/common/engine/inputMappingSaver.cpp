/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "inputMappingSaver.h"
#include "../core/configVarLegacyWrapper.h"

namespace Config
{
	TConfigVar<String> cvInputMappingSettingsSectionName( "InputSettings", "SettingsSectionName", TXT("InputSettings"), eConsoleVarFlag_ReadOnly );
}

CInputMappingActionStringBuilder::CInputMappingActionStringBuilder()
{
	Reset();
}

void CInputMappingActionStringBuilder::Reset()
{
	m_actionName = String::EMPTY;
	m_state = String::EMPTY;
	m_idleTime = 0.0f;
	m_reprocess = false;
}

String CInputMappingActionStringBuilder::Construct()
{
	String result;

	BeginConstruct(result);
	ConstructAction(result);
	ConstructState(result);
	ConstructActionValue(result);
	ConstructReprocess(result);
	EndConstruct(result);

	return result;
}

void CInputMappingActionStringBuilder::BeginConstruct(String& result)
{
	result += TXT("(");
}

void CInputMappingActionStringBuilder::EndConstruct(String& result)
{
	result += TXT(")");
}

void CInputMappingActionStringBuilder::ConstructAction(String &result)
{
	if( m_actionName != String::EMPTY )
	{
		result += TXT("Action=");
		result += m_actionName;
	}
	else
	{
		result += TXT("Action=Unknown");
	}
}

void CInputMappingActionStringBuilder::ConstructState(String &result)
{
	if( m_state != String::EMPTY )
	{
		result += TXT(",");
		result += TXT("State=");
		result += m_state;
	}
}

void CInputMappingActionStringBuilder::ConstructActionValue(String& result)
{
	if( m_state == TXT("Axis") )
	{
		ConstructAxisValue(result);
	}
	else
	{
		ConstructIdleTime(result);
	}
}

void CInputMappingActionStringBuilder::ConstructIdleTime(String& result)
{
	if( Red::Math::MAbs( m_idleTime ) > NumericLimits< Float >::Epsilon() )
	{
		result += TXT(",");
		result += TXT("IdleTime=");
		result += ToString( m_idleTime );
	}
}

void CInputMappingActionStringBuilder::ConstructAxisValue(String& result)
{
	if( Red::Math::MAbs( m_idleTime ) > NumericLimits< Float >::Epsilon() )
	{
		result += TXT(",");
		result += TXT("Value=");
		result += ToString( m_idleTime );
	}
}

void CInputMappingActionStringBuilder::ConstructReprocess(String& result)
{
	if( m_reprocess == true )
	{
		result += TXT(",");
		result += TXT("Reprocess");
	}
}

CInputMappingSaver::CInputMappingSaver(const String& filename, const String& fileAbsolutePath, const Int32 version)
{
	m_file = new Config::Legacy::CConfigLegacyFile( filename, fileAbsolutePath );
	Config::Legacy::CConfigLegacySection* settingsSection = m_file->GetSection( Config::cvInputMappingSettingsSectionName.Get(), true );
	settingsSection->WriteValue( TXT("Version"), ToString( version ), false );
}

CInputMappingSaver::~CInputMappingSaver()
{
	delete m_file;
}

void CInputMappingSaver::AddEntry(const CName& context, const EInputKey key, const String& action)
{
	Config::Legacy::CConfigLegacySection* contextSection = m_file->GetSection( context.AsString(), true );

	String keyStr = GetKeyAsString(key);
	contextSection->WriteValue( keyStr, action, true );
}

void CInputMappingSaver::Save()
{
	m_file->Write();
}

String CInputMappingSaver::GetKeyAsString(const EInputKey key)
{
	CEnum* keyEnum = SRTTI::GetInstance().FindEnum( CNAME( EInputKey ) );

	RED_FATAL_ASSERT( keyEnum != nullptr, "Key enum was not found" );

	CName keyName;
	Bool keyFound = keyEnum->FindName( (Int32)key, keyName );

	if( keyFound == false )		// Default fallback
	{
		keyName = CName(TXT("IK_None"));
	}

	return keyName.AsString();
}

void CInputMappingSaver::AddEmptyContext(const CName& context)
{
	m_file->GetSection( context.AsString(), true );
}
