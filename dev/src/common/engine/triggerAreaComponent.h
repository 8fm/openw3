/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "triggerManager.h"
#include "areaComponent.h"

class ITriggerAreaListener
{
public:

	virtual void OnAreaEnter( CTriggerAreaComponent* area, CComponent* activator ) { }
	virtual void OnAreaExit( CTriggerAreaComponent* area, CComponent* activator ) { }
};

/// Trigger component
class CTriggerAreaComponent : public CAreaComponent, public ITriggerCallback
{
	DECLARE_ENGINE_CLASS( CTriggerAreaComponent, CAreaComponent, 0 );

private:
	//! Registered trigger object
	class ITriggerObject* m_triggerObject;

protected:
	//! Trigger channels that will activate this area 
	Uint32 m_includedChannels;
	Uint32 m_excludedChannels;

	//! Is the trigger enabled (general flag)
	Bool m_isEnabled;			

	//! Enable CCD tests for trigger overlap (slower)
	Bool m_enableCCD;

	//! If player is teleported inside both triggers, in what order should they trigger? Lower = sooner
	Uint32 m_triggerPriority;

	//! Listeners notified on enter/exit
	TDynArray< ITriggerAreaListener* >	m_listeners;

public:

	//! Get trigger object
	RED_INLINE const class ITriggerObject* GetTrigggerObject() const { return m_triggerObject; }

	//! Get trigger priority
	RED_INLINE Uint32 GetTriggerPriority() const { return m_triggerPriority; }

	//! Get CCD
	Bool GetCCD() const { return m_enableCCD; }

public:
	CTriggerAreaComponent();
	~CTriggerAreaComponent();

	// Change trigger masks
	void SetChannelMask( const Uint32 includedChannels, const Uint32 excludedChannels );

	// Remove a channel from include mask
	void RemoveIncludedChannel( const ETriggerChannel channel );

	// Add a channel to include mask
	void AddIncludedChannel( const ETriggerChannel channel );

	// Remove a channel from exclude mask
	void RemoveExcludedChannel( const ETriggerChannel channel );

	// Add a channel to exclude mask
	void AddExcludedChannel( const ETriggerChannel channel );

	// Get contour line width
	virtual Float CalcLineWidth() const;

	// Test if given point is inside the area - uses better implementation for triggers
	virtual Bool TestPointOverlap( const Vector& worldPoint ) const;

	// Test if given box is inside the area - uses better implementation for triggers
	virtual Bool TestBoxOverlap( const Box& box ) const;

	// Trace a line segment
	virtual Bool TestBoxTrace( const Vector& worldStart, const Vector& worldEnd, const Vector& extents, Float& outEntryTime, Float& outExitTime ) const;

public:
	// Update transform
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

	// Called when object is serialized
	virtual void OnSerialize( IFile& file );

#ifndef NO_DATA_VALIDATION
	// Check data, can be called either for node on the level or for node in the template
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif

public:
	// Is trigger area enabled ?
	virtual Bool IsEnabled() const;

	// Toggle trigger area
	virtual void SetEnabled( Bool enabled );

public:
	// Something have entered zone
	virtual void EnteredArea( CComponent* component );

	// Something have exited zone
	virtual void ExitedArea( CComponent* component );

	// Add listener notified on enter/exit
	Bool AddListener( ITriggerAreaListener* listener );

	// Remove listener notifed on enter/exit
	Bool RemoveListener( ITriggerAreaListener* listener );

	// Getters for include and exclude channels
	ETriggerChannel GetIncludedChannels() const { return static_cast< ETriggerChannel >( m_includedChannels ); }
	ETriggerChannel GetExludedChannels() const  { return static_cast< ETriggerChannel >( m_excludedChannels ); }

protected:
	// Update attachment to world
	void ConditionalAttachToWorld( CWorld* world );

	// Converts old m_prioryty property to new m_triggerPriority
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );

	// Property changed
	virtual void OnPropertyPostChange( CProperty* prop );

	// Compiled shape of this area has changed
	virtual void OnAreaShapeChanged();

	// ITriggerCallback interface
	virtual void OnActivatorEntered( const class ITriggerObject* object, const class ITriggerActivator* activator );
	virtual void OnActivatorExited( const class ITriggerObject* object, const class ITriggerActivator* activator );

protected:
	void funcSetChannelMask( CScriptStackFrame& stack, void* result );
	void funcAddIncludedChannel( CScriptStackFrame& stack, void* result );
	void funcRemoveIncludedChannel( CScriptStackFrame& stack, void* result );
	void funcAddExcludedChannel( CScriptStackFrame& stack, void* result );
	void funcRemoveExcludedChannel( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CTriggerAreaComponent );
	PARENT_CLASS( CAreaComponent );
	PROPERTY_EDIT( m_isEnabled, TXT("Is Enabled") );
	PROPERTY_BITFIELD_EDIT( m_includedChannels, ETriggerChannel, TXT("Included trigger channels") );
	PROPERTY_BITFIELD_EDIT( m_excludedChannels, ETriggerChannel, TXT("Excluded trigger channels") );
	PROPERTY_EDIT( m_triggerPriority, TXT( "If player is teleported inside both triggers, in what order should they trigger? Lower = sooner" ) );
	PROPERTY_EDIT( m_enableCCD, TXT( "Use slower but more acculare collision detection for slower frame rates and/or faster movement" ) );
	NATIVE_FUNCTION( "SetChannelMask", funcSetChannelMask );
	NATIVE_FUNCTION( "AddIncludedChannel", funcAddIncludedChannel );
	NATIVE_FUNCTION( "RemoveIncludedChannel", funcRemoveIncludedChannel );
	NATIVE_FUNCTION( "AddExcludedChannel", funcAddExcludedChannel );
	NATIVE_FUNCTION( "RemoveExcludedChannel", funcRemoveExcludedChannel );
END_CLASS_RTTI();


