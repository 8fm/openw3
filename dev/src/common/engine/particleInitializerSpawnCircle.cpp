/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "effectDummyPoint.h"
#include "particleInitializer.h"
#include "particleEmitter.h"
#include "evaluatorFloat.h"

/// Initialize spawn position of a particle inside a circle
class CParticleInitializerSpawnCircle : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerSpawnCircle, IParticleInitializer, 0 );

protected:
	IEvaluatorFloat*	m_outerRadius;		//!< Outer radius of a circle
	IEvaluatorFloat*	m_innerRadius;		//!< Inner radius of a circle
	Bool				m_surfaceOnly;		//!< Spawn only on the surface of the circle
	Bool				m_worldSpace;		//!< Spawn in the world space
	EulerAngles			m_rotation;			//!< Circle rotation
	Matrix				m_spawnToLocal;		//!< Spawn space

public:
	CParticleInitializerSpawnCircle()
		: m_worldSpace( true )
		, m_surfaceOnly( false )
	{
		m_spawnToLocal = Matrix::IDENTITY;
		m_editorName = TXT("Spawn circle");
		m_editorGroup = TXT("Deprecated");
		m_outerRadius = new CEvaluatorFloatConst( this, 0.2f );
		m_innerRadius = new CEvaluatorFloatConst( this, 0.0f );
		m_requiredParticleField = 0xffffffff;
	}

	virtual void OnPropertyPostChange( IProperty* property )
	{
		TBaseClass::OnPropertyPostChange( property );

		// Update rotation matrix
		if ( property->GetName() == TXT("rotation") )
		{
			m_spawnToLocal = m_rotation.ToMatrix();
		}
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const 
	{
		data.m_spawn_BoxCircle_WorldSpace = m_worldSpace;
		data.m_spawnCircleSpawnToLocal = m_spawnToLocal;
		data.m_spawnCircleRotation = m_rotation;
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
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_outerRadius || !m_innerRadius )
		{
			return 0;
		}
		return (Uint32)PIF_SpawnCircle;
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerSpawnCircle );
	PARENT_CLASS( IParticleInitializer );
	PROPERTY_INLINED( m_innerRadius, TXT("Inner radius of the circle") );
	PROPERTY_INLINED( m_outerRadius, TXT("Outer radius of the circle") );
	PROPERTY_EDIT( m_surfaceOnly, TXT("Spawn only on the surface of the circle") );
	PROPERTY_EDIT( m_worldSpace, TXT("Spawn in the world space") );
	PROPERTY( m_spawnToLocal );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerSpawnCircle );