/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleDrawer.h"

IMPLEMENT_RTTI_ENUM( EParticleVertexDrawerType );
IMPLEMENT_ENGINE_CLASS( IParticleDrawer );
IMPLEMENT_ENGINE_CLASS( CParticleDrawerBillboard );
IMPLEMENT_ENGINE_CLASS( CParticleDrawerRain );
IMPLEMENT_ENGINE_CLASS( CParticleDrawerEmitterOrientation );
IMPLEMENT_ENGINE_CLASS( CParticleDrawerMotionBlur );
IMPLEMENT_ENGINE_CLASS( CParticleDrawerSphereAligned );
IMPLEMENT_RTTI_ENUM( EMeshParticleOrientationMode );
IMPLEMENT_ENGINE_CLASS( CParticleDrawerMesh );
IMPLEMENT_ENGINE_CLASS( CParticleDrawerFacingTrail );
IMPLEMENT_ENGINE_CLASS( CParticleDrawerTrail );
IMPLEMENT_ENGINE_CLASS( CParticleDrawerBeam );
IMPLEMENT_ENGINE_CLASS( CParticleDrawerScreen );

// See perforce history for old implementation!