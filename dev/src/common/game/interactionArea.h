/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#define PROFILE_INTERACTION

#include "optional.h"
#include "newNpcSensesManager.h"

struct SActivatorData
{
	SActivatorData();
	SActivatorData( CEntity* activator );
	
	Bool IsPlayer() const;
	Bool IsInCombat() const;
	Bool CanStartTalk() const;
	Bool IsUsingVehicle() const;
	Bool IsSwimming() const;
	Vector GetCenter() const;
	Vector GetLineOfSightPosition() const;
	
	RED_INLINE CEntity* GetEntity() const
	{
		return m_activator;
	}

private:

	CEntity*					m_activator;
	mutable Uint32				m_flags;

	enum EActivatorFlags
	{
		AF_IsPlayer,
		AF_IsInCombat,
		AF_CanStartTalk,
		AF_IsUsingVehicle,
		AF_IsSwimming,
		AF_Total,
	};

	Bool IsInitialized( EActivatorFlags flag ) const;
	Bool GetFlag( EActivatorFlags flag ) const;
	void SetFlag( EActivatorFlags flag, Bool f ) const;
};

// Component for handling interaction with user
class CInteractionAreaComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CInteractionAreaComponent, CComponent, 0 );

protected:
	Bool					m_isEnabled;			//!< Interaction is enabled
	Float					m_rangeMin;				//!< Minimum range
	Float					m_rangeMax;				//!< Maximum range
	Uint32					m_rangeAngle;			//!< Range angle
	Float					m_height;				//!< Interaction height	
	String					m_friendlyName;			//!< Friendly name
	Bool					m_isPlayerOnly;			//!< Player only
	Bool					m_manualTestingOnly;	//!< Manual testing only ( not checked by InteractionsManager )
	Bool					m_checkLineOfSight;		//!< To use this interaction there should be light of sight between player and interaction
	Float					m_alwaysVisibleRange;	//!< Interaction will be always visible (line of sight test) within this range
	Vector					m_lineOfSightOffset;	//!< Offset for calculacting line of sight test position (relative to interaction center)
	mutable Bool			m_performScriptedTest;	//!< Should this interaction area perform scripted activation test

	mutable const CFunction* m_scriptedTestFunction; //!< Cached pointer to scripted test function

	// line of sight cached data
	mutable Bool			m_lastLineOfSightResult;
	mutable EngineTime		m_lastLineOfSightTest;
	mutable VisibilityQueryId	m_lineOfSightQuery;

	// precalcs
	Float					m_rangeMinSquare;
	Float					m_rangeMaxSquare;
	Float					m_cosHalfRangeAngle;
	Float					m_alwaysVisibleRangeSqr;

	static const Float		s_activatorHeightTolerance;
	static const Float		s_lineOfSightTestInterval;
	static const Float		s_forceLineOfSightTestInterval;

public:
	//! Get minimum range
	RED_INLINE Float GetRangeMin() const { return m_rangeMin; }

	//! Get maximum range
	RED_INLINE Float GetRangeMax() const { return m_rangeMax; }

	//! Get range angle
	RED_INLINE Uint32 GetRangeAngle() const { return m_rangeAngle; }

	//! Get interaction height
	RED_INLINE Float GetHeight() const { return m_height; }

	//! Is this area activated and deactivated manually?
	RED_INLINE Bool IsManualTestingOnly() const { return m_manualTestingOnly; }

	//! Sets manual testing only
	RED_INLINE void SetManualTestingOnly( Bool flag ) { m_manualTestingOnly = true; }

	//! Is testing player only
	RED_INLINE Bool IsPlayerOnly() const { return m_isPlayerOnly; }

	//! Set check LOS
	RED_INLINE void SetCheckLineOfSight( Bool flag ) { m_checkLineOfSight = flag; }

	//! Is interaction enabled ?
	RED_INLINE Bool IsEnabled() const { return m_isEnabled; }

public:
	//! Default constructor
	CInteractionAreaComponent();

	//! Component was attached to world
	virtual void OnAttached( CWorld* world );

	//! Component was detached from world
	virtual void OnDetached( CWorld *world );

	//! Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

	//! Get fragments color
	virtual Color GetColor() const;

	//! Toggle on and off
	virtual void SetEnabled( Bool enabled );

	//! Set friendly name
	virtual void SetFriendlyName( String friendlyName ) { m_friendlyName = friendlyName; }

protected:
	
	//! Activation test - "cheap" part - should be performed first
	Bool ActivationFastTest( const SActivatorData& activatorData ) const;

	//! Activation test - "expensive" part - should be performed later)
	Bool ActivationSlowTest( const SActivatorData& activatorData ) const;

public:

	//! Activation test
	virtual Bool ActivationTest( CEntity* activator, const SActivatorData& activatorData ) const;

	//! Line of sight test between the activator and interactive entity
	virtual Bool LineOfSightTest( const Vector& activatorPos ) const;

protected:

	//! Compute line of sight test position based on activator's position
	// (don't add offset towards activator if its position is not specified - vector is zero)
	Vector GetLineOfSightTestPosition( const Vector& activatorPosition = Vector::ZEROS ) const;

	//! Called on after scripts reload
	void OnScriptReloaded() override;

	//! Cache scripted functions pointers
	void CacheScriptedFunctions() const;

	//! Recalculate useful geometry data
	void RecalculateGeometryData();

private:
	void funcGetRangeMin( CScriptStackFrame& stack, void* result );
	void funcGetRangeMax( CScriptStackFrame& stack, void* result );
	void funcSetRanges( CScriptStackFrame& stack, void* result );
	void funcSetRangeAngle( CScriptStackFrame& stack, void* result );
	void funcActivationTest( CScriptStackFrame& stack, void* result );
	void funcActivationPointTest( CScriptStackFrame& stack, void* result );
	void funcSetCheckLineOfSight( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CInteractionAreaComponent )
	PARENT_CLASS( CComponent )
	PROPERTY_EDIT( m_rangeMin, TXT("Activation range Min") );
	PROPERTY_EDIT( m_rangeMax, TXT("Activation range Max") );
	PROPERTY_EDIT( m_rangeAngle, TXT("Activation angle") );
	PROPERTY_EDIT_RANGE( m_height, TXT("Interaction height"), 0.f, NumericLimits<Float>::Max() );
	PROPERTY_EDIT( m_isPlayerOnly, TXT("Player Only") );
	PROPERTY_EDIT( m_isEnabled, TXT("Is Enabled") );
	//PROPERTY_EDIT( m_friendlyName, TXT("Friendly Name") );
	PROPERTY_EDIT( m_manualTestingOnly, TXT("Manual testing only") );
	PROPERTY_EDIT( m_checkLineOfSight, TXT("Check line of sight") );
	PROPERTY_EDIT( m_alwaysVisibleRange, TXT("Interaction will be always visible (line of sight test) within this range") );
	PROPERTY_EDIT( m_lineOfSightOffset, TXT("Offset for calculacting line of sight test position (relative to interaction center)") );
	PROPERTY_EDIT( m_performScriptedTest, TXT("Should this interaction area perform scripted activation test") );
	NATIVE_FUNCTION( "GetRangeMin", funcGetRangeMin );
	NATIVE_FUNCTION( "GetRangeMax", funcGetRangeMax );
	NATIVE_FUNCTION( "SetRanges", funcSetRanges );
	NATIVE_FUNCTION( "SetRangeAngle", funcSetRangeAngle );
	NATIVE_FUNCTION( "SetCheckLineOfSight", funcSetCheckLineOfSight );
END_CLASS_RTTI()