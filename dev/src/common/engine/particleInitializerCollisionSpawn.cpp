/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "particleInitializerCollisionSpawn.h"
#include "particleEmitter.h"
#include "particleSystem.h"

IMPLEMENT_ENGINE_CLASS( CParticleInitializerCollisionSpawn );

void CParticleInitializerCollisionSpawn::SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const 
{
	CParticleEmitter* emitter = GetEmitter();
	if( !emitter ) return;
	CParticleSystem* particleSystem = Cast< CParticleSystem >( emitter->GetParent() );
	if( !particleSystem ) return;

	const TDynArray< CParticleEmitter* >& emitters = particleSystem->GetEmitters();
	Uint32 emittersCount = emitters.Size();
	for( Uint32 k = 0; k != emittersCount; ++k )
	{
		const IParticleModule* parentEmitter = emitters[ k ];

		if( m_parentEmitterName.AsString() == parentEmitter->GetEditorName() )
		{
			data.m_collisionSpawnParentModuleIndex = k;
			break;
		}
	}

	if( data.m_collisionSpawnParentModuleIndex == -1 ) return;

	data.m_collisionSpawnProbability = m_probability;

}

Uint32 CParticleInitializerCollisionSpawn::GetInitializerId() const
{
	return PIF_CollisionSpawn;
}