/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "particleInitializer.h"

class CParticleComponent;

/// Modificator of existing particle stream
class IParticleModificator : public IParticleInitializer
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IParticleModificator, IParticleInitializer );

public:
	/// Particle update context
	struct UpdateContext
	{
		const CParticleComponent*		m_component;		//!< Owning component
		Float							m_emitterTime;		//!< Current emitter time
		Float							m_effectAlpha;		//!< Current effect time
		Float							m_timeDelta;		//!< Time step
		const CNode*					m_targetNode;		//!< Target node for some particles

		//! Fill to defaults
		RED_INLINE UpdateContext()
			: m_component( NULL )
			, m_emitterTime( 0.0f )
			, m_effectAlpha( 1.0f )
			, m_timeDelta( 0.0f )
			, m_targetNode( NULL )
		{};
	};

public:
	IParticleModificator() {}

	//! Particle modifier usually do not initialize particles
	//virtual void InitParticles( IParticleBuffer& particlesBuffer, Uint32 firstIndex, Uint32 count, const InitializationContext& info ) const;

	//! Update particles, no particles are allowed to be killed
	//virtual void UpdateParticles( CParticleBuffer& particlesBuffer, const UpdateContext& updateContext ) const=0;

	//! Get mask flag of this modifier
	virtual Uint32 GetModificatorId() const { return 0; }
};

BEGIN_ABSTRACT_CLASS_RTTI( IParticleModificator );
	PARENT_CLASS( IParticleInitializer );
END_CLASS_RTTI();