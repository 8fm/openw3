/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/inGameConfigVarBasic.h"
#include "../engine/inGameConfigGroupBasic.h"

class CDLCManager;

// ---------------------------------------------------------

class CInGameConfigDLCGroup : public InGameConfig::CConfigGroupBasic
{
public:
	struct ConstructParams
	{
		InGameConfig::CConfigGroupBasic::ConstructParams basicParams;
		CDLCManager* dlcManager;
	};

	CInGameConfigDLCGroup( const ConstructParams& params );

	virtual Bool IsVisible() const override;
	virtual void ListConfigVars(TDynArray< InGameConfig::IConfigVar* >& output) override;
	virtual void ResetToDefault() override;
	virtual void Discard() override;

	void AddDLCVar( const CName& dlcName );
	Bool ContainsDLCVar( const CName& dlcName );

	static CInGameConfigDLCGroup* CreateDLCGroup( CDLCManager* dlcManager );

private:
	CDLCManager* m_dlcManager;
	TDynArray<InGameConfig::IConfigVar*> m_vars;

};

// ---------------------------------------------------------

class CInGameConfigDLCVar : public InGameConfig::CConfigVarBasic
{
public:
	struct ConstructParams
	{
		InGameConfig::CConfigVarBasic::ConstructParams basicParams;
		CDLCManager* dlcManager;
	};

	CInGameConfigDLCVar( const ConstructParams& params );

	virtual const String GetDisplayType() const override;
	virtual const String GetDisplayName() const override;
	virtual const InGameConfig::CConfigVarValue GetValue() const override;
	virtual void SetValue(const InGameConfig::CConfigVarValue value, const InGameConfig::EConfigVarSetType setType) override;
	virtual void ResetToDefault() override;

private:
	CDLCManager* m_dlcManager;

};

// ---------------------------------------------------------
