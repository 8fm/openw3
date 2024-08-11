/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "triggerManager.h"
#include "component.h"

/// Activator of triggers
class CTriggerActivatorComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CTriggerActivatorComponent, CComponent, 0 );

protected:
	//! Radius of trigger activator
	Float						m_radius;

	//! Radius of trigger activator
	Float						m_height;

	//! Created activator object for trigger system
	ITriggerActivator*			m_activator;

	//! Channels on which we do the activation
	Uint32						m_channels;

	//! Maximum distance traveled witin one frame that is considered contiguous
	Float						m_maxContinousDistance;

	//! Enable contignous collision detection
	Bool						m_enableCCD;

public:
	CTriggerActivatorComponent();

	//! Attach to the world
	virtual void OnAttached( CWorld* world );

	//! Detached from world
	virtual void OnDetached( CWorld* world );

	//! Moved
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	//! Debug drawing
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	//! Properties changed
	virtual void OnPropertyPostChange( IProperty* property );

public:
	//! Change the radius of the trigger activator
	void SetRadius( const Float radius );

	//! Change the height of the trigger activator
	void SetHeight( const Float height );

	//! Add trigger channel to the trigger activator
	void AddTriggerChannel( const ETriggerChannel channel );

	//! Remove trigger channel from the trigger activator
	void RemoveTriggerChannel( const ETriggerChannel channel );	

	//! Check if activator is inside trigger
	Bool IsInsideTrigger( const class ITriggerObject* trigger ) const;

protected:
	void funcSetRadius( CScriptStackFrame& stack, void* result );
	void funcSetHeight( CScriptStackFrame& stack, void* result );
	void funcGetRadius( CScriptStackFrame& stack, void* result );
	void funcGetHeight( CScriptStackFrame& stack, void* result );
	void funcAddTriggerChannel( CScriptStackFrame& stack, void* result );
	void funcRemoveTriggerChannel( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CTriggerActivatorComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT_RANGE( m_radius, TXT("Radius of the activator"), 0.0f, 1000.0f );
	PROPERTY_EDIT_RANGE( m_height, TXT("Height of the activator"), 0.0f, 1000.0f );
	PROPERTY_BITFIELD_EDIT( m_channels, ETriggerChannel, TXT("Trigger channels") );
	PROPERTY_EDIT( m_enableCCD, TXT("Enable contignous collision detection (much slower)") );
	PROPERTY_EDIT( m_maxContinousDistance, TXT("Maximum distance traveled witin one frame that is considered contiguous") );
	NATIVE_FUNCTION( "SetRadius", funcSetRadius );
	NATIVE_FUNCTION( "SetHeight", funcSetHeight );
	NATIVE_FUNCTION( "GetRadius", funcGetRadius );
	NATIVE_FUNCTION( "GetHeight", funcGetHeight );
	NATIVE_FUNCTION( "AddTriggerChannel", funcAddTriggerChannel );
	NATIVE_FUNCTION( "RemoveTriggerChannel", funcRemoveTriggerChannel );
END_CLASS_RTTI();
