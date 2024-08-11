/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "particleModificator.h"
#include "../physics/physicsEngine.h"

/// Kill particles that collided
class CParticleModificatorCollision : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorCollision, IParticleModificator, 0 );

protected:
	TDynArray< CName > m_triggeringCollisionGroupNames;
	Float m_dynamicFriction;
	Float m_staticFriction;
	Float m_restition;
	Float m_velocityDamp;
	Bool m_disableGravity;
	Bool m_useGpuSimulationIfAvaible;
	Float m_radius;
	Bool m_killWhenCollide;

public:
	CParticleModificatorCollision()
		: m_dynamicFriction( 0.0500000007f )
		, m_staticFriction( 0.0f )
		, m_restition( 0.0f )
		, m_velocityDamp( 0.0f )
		, m_disableGravity( false )
		, m_useGpuSimulationIfAvaible( true )
		, m_radius( 0.01f )
		, m_killWhenCollide( false )
	{
		m_editorName = TXT("Collision");
		m_editorGroup = TXT("Physics");
		m_requiredParticleField = PFS_Velocity;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const;
	
	//! Get mask flag of this modifier
	virtual Uint32 GetModificatorId() const;
};

BEGIN_CLASS_RTTI( CParticleModificatorCollision );
	PARENT_CLASS( IParticleModificator );
	PROPERTY_CUSTOM_EDIT( m_triggeringCollisionGroupNames, TXT( "Defines which collision groups will trigger" ), TXT("PhysicalCollisionGroupSelector") );
	PROPERTY_EDIT( m_dynamicFriction, TXT("Dynamic friction") );
	PROPERTY_EDIT( m_staticFriction, TXT("Static friction") );
	PROPERTY_EDIT( m_restition, TXT("Restitution") );
	PROPERTY_EDIT( m_velocityDamp, TXT("Velocity clamp") );
	PROPERTY_EDIT( m_disableGravity, TXT("Velocity clamp") );
	PROPERTY_EDIT( m_radius, TXT("Particle radius") );
	PROPERTY_EDIT( m_killWhenCollide, TXT("Remove particle on collision") );
	PROPERTY_EDIT_NAME( m_useGpuSimulationIfAvaible, TXT( "Use Gpu Simulation If Avaible" ), TXT( "" ) )
END_CLASS_RTTI();
