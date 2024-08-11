/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModificator.h"
#include "particleEmitter.h"

/// Modify alpha of a particle over the particle lifetime
class CParticleModificatorAlphaOverEffect : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorAlphaOverEffect, IParticleModificator, 0 );

public:
	CParticleModificatorAlphaOverEffect()
	{
		m_editorName = TXT("Effect alpha");
		m_editorGroup = TXT("Material");
		m_requiredParticleField = PFS_Color;
	}

	//! Get mask flag of this modifier
	virtual Uint32 GetModificatorId() const 
	{
		return (Uint32)PMF_EffectAlpha;
	}
};

BEGIN_CLASS_RTTI( CParticleModificatorAlphaOverEffect );
PARENT_CLASS( IParticleModificator );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleModificatorAlphaOverEffect );