/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
class CSeatComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CSeatComponent, CComponent, 0 );
private:
	CAdvancedVehicleComponent*	m_vehicle;
	CPilotComponent*			m_rider;
	Bool						m_isPilot;
	CName						m_slotName;

public:
	CSeatComponent();

	void	OnAttached			( CWorld* world );
	void	OnMounted			( CPilotComponent* rider );
	void	OnDisMounted		( );

	RED_INLINE CAdvancedVehicleComponent* GetVehicle	( )		{ return m_vehicle; }
	RED_INLINE Bool						IsPilot		( )		{ return m_isPilot; }

private:
	void	GrabVehicleIfNeeded	( );

	void	funcOnMounted		( CScriptStackFrame& stack, void* result );
	void	funcOnDisMounted	( CScriptStackFrame& stack, void* result );
	void	funcIsPilot			( CScriptStackFrame& stack, void* result );
};


BEGIN_CLASS_RTTI( CSeatComponent );
	PARENT_CLASS( CComponent )
	PROPERTY_EDIT( m_isPilot			, TXT( "Is this seat the pilot seat" ) );
	PROPERTY_EDIT( m_slotName			, TXT( "Slot to attach this seat" ) );
	NATIVE_FUNCTION( "I_IsPilot"		, funcIsPilot );
	NATIVE_FUNCTION( "I_OnMounted"		, funcOnMounted );
	NATIVE_FUNCTION( "I_OnDisMounted"	, funcOnDisMounted );
END_CLASS_RTTI();
