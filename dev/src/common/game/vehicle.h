
#pragma once

#include "../engine/component.h"

class CVehicleComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CVehicleComponent, CComponent, 0 );

protected:
	Bool			m_isHorse;
	THandle<CActor>	m_user;

	Bool			m_commandToMountDelay;
public:
	CVehicleComponent() 
		: m_isHorse( false )
		, m_commandToMountDelay( false )
	{}

	RED_INLINE Bool IsHorse() { return m_isHorse; }

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

public:
    THandle<CActor> GetUser() const { return m_user; }
	Bool PlayAnimationOnVehicle( const CName& slotName, const CName& animationName, Float blendIn = 0.2f, Float blendOut = 0.2f );
	Bool StopAnimationOnVehicle( const CName& slotName );

	Bool IsPlayingAnimationOnVehicle( const CName& slotName ) const;

	virtual void OnDriverMount() {}

protected:
	void funcGetSlotTransform( CScriptStackFrame& stack, void* result );
	void funcPlaySlotAnimationAsync( CScriptStackFrame& stack, void* result );
	void funcPlaySlotAnimation( CScriptStackFrame& stack, void* result );
    void funcGetDeepDistance( CScriptStackFrame& stack, void* result );
	void funcSetCommandToMountDelayed( CScriptStackFrame& stack, void* result );
	void funcIsCommandToMountDelayed( CScriptStackFrame& stack, void* result );
	void funcOnDriverMount( CScriptStackFrame& stack, void* result );
	virtual void OnTick( Float timeDelta );
    virtual void MountedAfterSaveRestore(){};
};

BEGIN_CLASS_RTTI( CVehicleComponent );
	PARENT_CLASS( CComponent );
	PROPERTY( m_user )
	NATIVE_FUNCTION( "PlaySlotAnimation", funcPlaySlotAnimation );
	NATIVE_FUNCTION( "PlaySlotAnimationAsync", funcPlaySlotAnimationAsync );
	NATIVE_FUNCTION( "GetSlotTransform", funcGetSlotTransform );
    NATIVE_FUNCTION( "GetDeepDistance", funcGetDeepDistance );
	NATIVE_FUNCTION( "SetCommandToMountDelayed", funcSetCommandToMountDelayed );
	NATIVE_FUNCTION( "IsCommandToMountDelayed", funcIsCommandToMountDelayed );
	NATIVE_FUNCTION( "OnDriverMount", funcOnDriverMount );
END_CLASS_RTTI();
