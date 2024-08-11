/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleInitializer.h"
#include "particleEmitter.h"
#include "evaluatorFloat.h"


/// Initialize spawn position of a particle inside a sphere
class CParticleInitializerSpawnSphere : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerSpawnSphere, IParticleInitializer, 0 );

protected:
	IEvaluatorFloat*	m_outerRadius;		//!< Outer radius of a sphere
	IEvaluatorFloat*	m_innerRadius;		//!< Inner radius of a sphere
	Bool				m_surfaceOnly;		//!< Spawn only on the surface of the sphere
	Bool				m_spawnPositiveX;	//!< Spawn only in the positive X hemisphere
	Bool				m_spawnNegativeX;	//!< Spawn only in the negative X hemisphere
	Bool				m_spawnPositiveY;	//!< Spawn only in the positive Y hemisphere
	Bool				m_spawnNegativeY;	//!< Spawn only in the negative Y hemisphere
	Bool				m_spawnPositiveZ;	//!< Spawn only in the positive Z hemisphere
	Bool				m_spawnNegativeZ;	//!< Spawn only in the negative Z hemisphere
	Bool				m_velocity;			//!< Spawn with a velocity
	IEvaluatorFloat*	m_forceScale;		//!< Used with velocity to set the velocity force

public:
	CParticleInitializerSpawnSphere()
	{
		m_editorName = TXT("Spawn sphere");
		m_editorGroup = TXT("Location");
		m_outerRadius = new CEvaluatorFloatConst( this, 0.2f );
		m_innerRadius = new CEvaluatorFloatConst( this, 0.0f );
		m_spawnPositiveX = true;
		m_spawnNegativeX = true;
		m_spawnPositiveY = true;
		m_spawnNegativeY = true;
		m_spawnPositiveZ = true;
		m_spawnNegativeZ = true;
		m_velocity = false;
		m_forceScale = new CEvaluatorFloatConst( this, 1.0f );
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const 
	{
		data.m_spawn_CircleSphere_SurfaceOnly = m_surfaceOnly;

		// Fill direction samples
		if ( m_outerRadius )
		{
			m_outerRadius->GetApproximationSamples( data.m_spawnCircleOuterRadius.m_samples );
		}
		if ( m_innerRadius )
		{
			m_innerRadius->GetApproximationSamples( data.m_spawnCircleInnerRadius.m_samples );
		}

		// Fill the positive and negative X,Y,Z
		data.m_spawnSpherePositiveX = m_spawnPositiveX;
		data.m_spawnSphereNegativeX = m_spawnNegativeX;
		data.m_spawnSpherePositiveY = m_spawnPositiveY;
		data.m_spawnSphereNegativeY = m_spawnNegativeY;
		data.m_spawnSpherePositiveZ = m_spawnPositiveZ;
		data.m_spawnSphereNegativeZ = m_spawnNegativeZ;

		// Use velocity on spawn or not
		data.m_spawnSphereVelocity = m_velocity;
		if ( m_forceScale )
		{
			m_forceScale->GetApproximationSamples( data.m_targetForceScale.m_samples );
		}
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_outerRadius || !m_innerRadius )
		{
			return 0;
		}
		return (Uint32)PIF_SpawnSphere;
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerSpawnSphere );
	PARENT_CLASS( IParticleInitializer );
	PROPERTY_INLINED( m_innerRadius, TXT("Inner radius of the sphere") );
	PROPERTY_INLINED( m_outerRadius, TXT("Outer radius of the sphere") );
	PROPERTY_EDIT( m_surfaceOnly, TXT("Spawn only on the surface of the sphere") );
	PROPERTY_EDIT( m_spawnPositiveX, TXT("Spawn only in the positive X hemisphere") );
	PROPERTY_EDIT( m_spawnNegativeX, TXT("Spawn only in the negative X hemisphere") );
	PROPERTY_EDIT( m_spawnPositiveY, TXT("Spawn only in the positive Y hemisphere") );
	PROPERTY_EDIT( m_spawnNegativeY, TXT("Spawn only in the negative Y hemisphere") );
	PROPERTY_EDIT( m_spawnPositiveZ, TXT("Spawn only in the positive Z hemisphere") );
	PROPERTY_EDIT( m_spawnNegativeZ, TXT("Spawn only in the negative Z hemisphere") );
	PROPERTY_EDIT( m_velocity, TXT("Spawn with velocity") );
	PROPERTY_INLINED( m_forceScale, TXT("Velocity") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerSpawnSphere );