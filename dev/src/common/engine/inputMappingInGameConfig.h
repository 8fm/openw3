/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "inGameConfigVarBasic.h"
#include "inGameConfigGroupBasic.h"

class CInputMappingInGameConfigGroup : public InGameConfig::CConfigGroupBasic
{
public:
	struct ConstructParams
	{
		InGameConfig::CConfigGroupBasic::ConstructParams basicParams;
		TDynArray< InGameConfig::IConfigVar* > vars;
	};

	CInputMappingInGameConfigGroup( const ConstructParams& params );

	virtual void ListConfigVars(TDynArray< InGameConfig::IConfigVar* >& output);
	virtual void ResetToDefault();
	virtual void Discard();

private:
	TDynArray<InGameConfig::IConfigVar*> m_vars;

};

class CInputMappingInGameConfigVar : public InGameConfig::CConfigVarBasic
{
public:
	struct ConstructParams
	{
		InGameConfig::CConfigVarBasic::ConstructParams basicParams;
		Bool isPadInput;
		TDynArray<CName> actions;
	};

	CInputMappingInGameConfigVar( const ConstructParams& params );

	virtual const String GetDisplayType() const;
	virtual const InGameConfig::CConfigVarValue GetValue() const;
	virtual void SetValue(const InGameConfig::CConfigVarValue value, const InGameConfig::EConfigVarSetType setType);
	virtual void ResetToDefault();

private:
	const String GetKeysStringForPad() const;
	const String GetKeysStringForKeyboardMouse() const;

	RED_INLINE Bool IsPadKey( enum EInputKey key ) const { return ( key >= IK_Pad_First && key <= IK_Pad_Last ); }
	void SetPadValue(const InGameConfig::CConfigVarValue value);
	void SetKeyboardMouseValue(const InGameConfig::CConfigVarValue value);

private:
	Bool m_isPadInput;
	TDynArray<CName> m_actions;

};
