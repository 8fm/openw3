#pragma once

class CSwitch : public CGameplayEntity
{	
	DECLARE_ENGINE_CLASS( CSwitch, CGameplayEntity, 0 );

	// Variables
public:
	TDynArray< EntityHandle >	m_TargetEntities;
	Int32							m_SwitchId;


public:
	CSwitch();

private:
	// Script interface
	void funcCallEventSwitch( CScriptStackFrame& stack, void* result );
	void funcCallEventSwitchOn( CScriptStackFrame& stack, void* result );
	void funcCallEventSwitchOff( CScriptStackFrame& stack, void* result );
};


BEGIN_CLASS_RTTI( CSwitch )
	PARENT_CLASS( CGameplayEntity )	
	
	PROPERTY_EDIT( m_TargetEntities,	TXT("TargetEntities") );
	PROPERTY_CUSTOM_EDIT( m_SwitchId,	TXT("i_SwitchID")		, TXT("A manual ID for the switch") );
	NATIVE_FUNCTION( "CallEventSwitch"		, funcCallEventSwitch);
	NATIVE_FUNCTION( "CallEventSwitchOn"	, funcCallEventSwitchOn);
	NATIVE_FUNCTION( "CallEventSwitchOff"	, funcCallEventSwitchOff);
END_CLASS_RTTI()