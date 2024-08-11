/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "inGameConfigDLC.h"
#include "dlcManager.h"
#include "../engine/inGameConfig.h"

// ---------------------------------------------------------

CInGameConfigDLCGroup::CInGameConfigDLCGroup(const ConstructParams& params)
	: InGameConfig::CConfigGroupBasic( params.basicParams )
	, m_dlcManager( params.dlcManager )
{
}

void CInGameConfigDLCGroup::ListConfigVars(TDynArray< InGameConfig::IConfigVar* >& output)
{
	output.PushBack( m_vars );
}

void CInGameConfigDLCGroup::ResetToDefault()
{
	/* Intentionally empty */
}

void CInGameConfigDLCGroup::Discard()
{
	m_vars.ClearPtr();
}

void CInGameConfigDLCGroup::AddDLCVar(const CName& dlcName)
{
	CInGameConfigDLCVar::ConstructParams params;
	params.basicParams.name = dlcName;
	params.basicParams.visibilityCondition = nullptr;
	params.dlcManager = m_dlcManager;

	CInGameConfigDLCVar* var = new CInGameConfigDLCVar( params );
	m_vars.PushBack(var);
}

CInGameConfigDLCGroup* CInGameConfigDLCGroup::CreateDLCGroup(CDLCManager* dlcManager)
{
	ConstructParams params;

	params.basicParams.name = CName(TXT("DLC"));
	params.basicParams.displayName = TXT("dlc");
	params.basicParams.visibilityCondition = nullptr;
	params.dlcManager = dlcManager;

	CInGameConfigDLCGroup* group = new CInGameConfigDLCGroup( params );

	return group;
}

Bool CInGameConfigDLCGroup::ContainsDLCVar(const CName& dlcName)
{
	for( InGameConfig::IConfigVar* var : m_vars )
	{
		if( var->GetConfigId() == dlcName )
		{
			return true;
		}
	}

	return false;
}

Bool CInGameConfigDLCGroup::IsVisible() const 
{
	return GInGameConfig::GetInstance().IsTagActive( CNAME(mainMenu) );
}

// ---------------------------------------------------------

CInGameConfigDLCVar::CInGameConfigDLCVar(const ConstructParams& params)
	: InGameConfig::CConfigVarBasic( params.basicParams )
	, m_dlcManager( params.dlcManager )
{
	/* Intentionally empty */
}

const String CInGameConfigDLCVar::GetDisplayType() const
{
	return TXT("TOGGLE");
}

const InGameConfig::CConfigVarValue CInGameConfigDLCVar::GetValue() const
{
	Bool result = m_dlcManager->IsDLCEnabled( m_name );
	return InGameConfig::CConfigVarValue( result );
}

void CInGameConfigDLCVar::SetValue(const InGameConfig::CConfigVarValue value, const InGameConfig::EConfigVarSetType setType)
{
	if( setType == InGameConfig::eConfigVarAccessType_UserAction )
	{
		Bool enableDlc = value.GetAsBool( false );
		m_dlcManager->EnableDLC( m_name, enableDlc );
	}
}

void CInGameConfigDLCVar::ResetToDefault()
{
	/* Intentionally empty */
}

const String CInGameConfigDLCVar::GetDisplayName() const
{
	return m_dlcManager->GetDLCName( m_name );
}
