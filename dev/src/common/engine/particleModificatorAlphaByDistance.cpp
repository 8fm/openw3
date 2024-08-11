/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModificator.h"
#include "particleEmitter.h"
#include "evaluatorFloat.h"

/// Modify alpha of a particle by the particle - camera distance
class CParticleModificatorAlphaByDistance : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorAlphaByDistance, IParticleModificator, 0 );

protected:
	Float m_far;
	Float m_near;

public:
	CParticleModificatorAlphaByDistance()
		: m_near( 0.f )
		, m_far( 0.f )
	{
		m_editorName = TXT("Alpha by distance");
		m_editorGroup = TXT("Material");	
		m_requiredParticleField = PFS_Color;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const
	{
		data.m_farDistance = m_far;
		data.m_nearDistance = m_near;
	}

	//! Get mask flag of this modifier
	virtual Uint32 GetModificatorId() const 
	{
		if ( m_near >= m_far )
		{
			return 0;
		}
		return (Uint32)PMF_AlphaByDistance;
	}
};

BEGIN_CLASS_RTTI( CParticleModificatorAlphaByDistance );
PARENT_CLASS( IParticleModificator );
PROPERTY_EDIT( m_far, TXT("Blending out start distance") );
PROPERTY_EDIT( m_near, TXT("Blending out stop distance") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleModificatorAlphaByDistance );