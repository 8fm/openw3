/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleInitializer.h"

IMPLEMENT_ENGINE_CLASS( IParticleInitializer );

void IParticleInitializer::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// DREY TODO: recompile emitter
}
