/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "..\..\common\engine\particleInitializerLifeTime.h"
#include "..\..\common\engine\particleSystem.h"
#include "..\..\common\engine\particleEmitter.h"

/// Factory for animation set
class CParticleSystemFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CParticleSystemFactory, IFactory, 0 );

public:
	CParticleSystemFactory();
	virtual CResource* DoCreate( const FactoryOptions& options );
};

DEFINE_SIMPLE_RTTI_CLASS(CParticleSystemFactory,IFactory);
IMPLEMENT_ENGINE_CLASS(CParticleSystemFactory);

CParticleSystemFactory::CParticleSystemFactory()
{
	m_resourceClass = ClassID< CParticleSystem >();
}

CResource* CParticleSystemFactory::DoCreate( const FactoryOptions& options )
{
	// Create the default system
	CParticleSystem* particleSystem = ::CreateObject< CParticleSystem >( options.m_parentObject );

	// Create default emitter
	CParticleEmitter* emitter = particleSystem->AddEmitter( ClassID< CParticleEmitter >(), TXT("Default Emitter") );
	if ( !emitter )
	{
		// We cannot create particle system without emitters
		particleSystem->Discard();
		return NULL;
	}

	// Add default modificator to the newly created emitter
	IParticleModule* module = emitter->AddModule( ClassID< CParticleInitializerLifeTime >() );

	// Done
	return particleSystem;
}
