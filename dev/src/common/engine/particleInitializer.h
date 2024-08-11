/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../renderer/particleLayouts.h"
#include "particleModule.h"

enum EParticleType : Int32;

struct SParticleUpdaterData;

/// Initialize of particle
class IParticleInitializer : public IParticleModule
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IParticleInitializer, IParticleModule );

public:
	Int32  m_requiredParticleField;
	Uint32 m_seed;					//!< Seed value for evaluators

	/// Particle initialization context
	struct InitializationContext
	{
		const CParticleComponent*		m_component;			//!< Owning component
		Vector							m_baseVelocity;			//!< Base component velocity
		Matrix							m_localToWorld;			//!< Local to world matrix for emitter
		Float							m_normalizedSpawnTime;	//!< Normalized (0-1) time of the spawn
		Float							m_normalizedSpawnDelta;	//!< Normalized (0-1) spawn time step for each spawned particle
		Vector							m_cameraPosition;		//!< Camera position

		//! Fill to defaults
		RED_INLINE InitializationContext()
			: m_component( NULL )
			, m_normalizedSpawnTime( 0.0f )
			, m_normalizedSpawnDelta( 0.0f )
			, m_baseVelocity( Vector::ZEROS )
		{};
	};

public:
	IParticleInitializer() : m_requiredParticleField( 0xffffffff ) {}

	//! Initialize particles
	//virtual void InitParticles( CParticleBuffer& particlesBuffer, Uint32 firstIndex, Uint32 count, const InitializationContext& info ) const=0;

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const {}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const { return 0; }

	template< typename PARTICLE_TYPE >
	Bool SupportsParticleType() const
	{
		return ( PARTICLE_TYPE::m_fieldSet & m_requiredParticleField ) != 0;
	}

	// Called after object's property is changed in the editor
	virtual void OnPropertyPostChange( IProperty* property );
};

BEGIN_ABSTRACT_CLASS_RTTI( IParticleInitializer );
	PARENT_CLASS( IParticleModule );
	PROPERTY_EDIT( m_seed, TXT("Seed value for all evaluators.") );
END_CLASS_RTTI();