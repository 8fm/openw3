/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "particleModificatorCollision.h"
#include "particleEmitter.h"
#include "particleSystem.h"

IMPLEMENT_ENGINE_CLASS( CParticleModificatorCollision );

void CParticleModificatorCollision::SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const
{
	CParticleEmitter* emitter = GetEmitter();
	if( !emitter ) return;
	CParticleSystem* particleSystem = Cast< CParticleSystem >( emitter->GetParent() );
	if( !particleSystem ) return;

	const TDynArray< CParticleEmitter* >& emitters = particleSystem->GetEmitters();
	Uint32 emittersCount = emitters.Size();
	for( Uint32 i = 0; i != emittersCount; ++i )
	{
		if( emitters[ i ] == emitter )
		{
			data.m_collisionEmitterIndex = i;
		}
	}

	if( data.m_collisionEmitterIndex == -1 ) return;

	data.m_collisionMask = GPhysicEngine->GetCollisionTypeBit( m_triggeringCollisionGroupNames );
	if( data.m_collisionMask == 0  )
	{
		data.m_collisionMask = GPhysicEngine->GetWithAllCollisionMask(); 
	}
	data.m_collisionDynamicFriction = m_dynamicFriction;
	data.m_collisionStaticFriction = m_staticFriction;
	data.m_collisionRestition = m_restition;
	data.m_collisionVelocityDamp = m_velocityDamp;
	data.m_collisionDisableGravity = m_disableGravity;
	data.m_collisionUseGpuSimulationIfAvaible = m_useGpuSimulationIfAvaible;
	data.m_collisionRadius = m_radius;
	data.m_collisionKillWhenCollide = m_killWhenCollide;
}

Uint32 CParticleModificatorCollision::GetModificatorId() const
{
	return (Uint32)PMF_Collision;
}
