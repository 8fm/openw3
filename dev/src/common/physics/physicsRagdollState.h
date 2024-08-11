#pragma once

#include "physicalCollision.h"

struct SRagdollResourcePartState
{
	CName m_name;
	Bool m_kinematic;

	SRagdollResourcePartState() : m_name(), m_kinematic( false ) {}
	SRagdollResourcePartState( const CName& name, Bool kinematic ) : m_name( name ), m_kinematic( kinematic ) {}
};

struct SPhysicsRagdollState
{
	Float				m_densityScaler;
	Float				m_windScaler;
	Float				m_autoStopDelay;			// auto-stop
	Float				m_autoStopTime;
	Float				m_autoStopSpeed;
	Bool				m_resetDampingAfterStop;
	Bool				m_forceWakeUpOnAttach;

	CPhysicalCollision	m_customDynamicGroup;

	Bool				m_disableConstrainsTwistAxis;
	Bool				m_disableConstrainsSwing1Axis;
	Bool				m_disableConstrainsSwing2Axis;
	Float				m_jointBounce;
	Float				m_modifyTwistLower;
	Float				m_modifyTwistUpper;
	Float				m_modifySwingY;
	Float				m_modifySwingZ;
	Int32				m_projectionIterations;

	TDynArray< SRagdollResourcePartState >	m_states;

	SPhysicsRagdollState() :  m_densityScaler( 1.0f )
							, m_autoStopDelay( 0.0f )
							, m_autoStopTime( 0.0f )
							, m_autoStopSpeed( 0.0f )
							, m_resetDampingAfterStop( true )
							, m_forceWakeUpOnAttach( false )
							, m_customDynamicGroup()
							, m_windScaler( 1.0f )
							, m_disableConstrainsTwistAxis( false )
							, m_disableConstrainsSwing1Axis( false )
							, m_disableConstrainsSwing2Axis( false )
							, m_jointBounce( -1.f )
							, m_modifyTwistLower( 0.0f )
							, m_modifyTwistUpper( 0.0f )
							, m_modifySwingY( 0.0f )
							, m_modifySwingZ( 0.0f )
							, m_projectionIterations( -1 ) {}
};
