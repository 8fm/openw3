/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "renderParticleEmitter.h"
#include "..\physics\physicsParticleWrapper.h"
#include "..\engine\evaluatorVector.h"

//////////////////////////////////////////////////////////////////////////
// Static particle update methods
//////////////////////////////////////////////////////////////////////////

void CRenderParticleEmitter::VelocityAbsolute( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Vector3* velocity = ( Vector3* )( particle + PARTICLE_VELOCITY );
	*velocity = LightEval::EvaluateVector( &updateData.m_velocityEval, normalizedLife );
}

void CRenderParticleEmitter::VelocityModulate( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Vector3* velocity = ( Vector3* )( particle + PARTICLE_VELOCITY );
	*velocity *= LightEval::EvaluateVector( &updateData.m_velocityEval, normalizedLife );
}

void CRenderParticleEmitter::AccelerationLocal( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	const Vector direction = LightEval::EvaluateVector( &updateData.m_accelerationDirEval, updateContext.m_emitterTime );
	const Float scale = LightEval::EvaluateScalar( &updateData.m_accelerationScaleEval, updateContext.m_emitterTime );

	const Matrix& localToWorld = updateContext.m_componentTransform;
	const Vector delta =  localToWorld.TransformVector( direction * scale * updateContext.m_timeDelta );

	Vector3* baseVelocity = ( Vector3* )( particle + PARTICLE_BASE_VELOCITY );
	Vector3* velocity = ( Vector3* )( particle + PARTICLE_VELOCITY );
	*velocity += delta;
	*baseVelocity += delta;
}

void CRenderParticleEmitter::AccelerationWorld( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	const Vector direction = LightEval::EvaluateVector( &updateData.m_accelerationDirEval, updateContext.m_emitterTime );
	const Float scale = LightEval::EvaluateScalar( &updateData.m_accelerationScaleEval, updateContext.m_emitterTime );

	const Vector delta = direction * scale * updateContext.m_timeDelta;

	Vector3* baseVelocity = ( Vector3* )( particle + PARTICLE_BASE_VELOCITY );
	Vector3* velocity = ( Vector3* )( particle + PARTICLE_VELOCITY );
	*velocity += delta;
	*baseVelocity += delta;
}

void CRenderParticleEmitter::ColorAbsolute( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Vector3* color = ( Vector3* )( particle + PARTICLE_COLOR );
	*color = LightEval::EvaluateVector( &updateData.m_colorEval, normalizedLife );
}

void CRenderParticleEmitter::ColorModulate( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Vector3* color = ( Vector3* )( particle + PARTICLE_COLOR );
	*color *= LightEval::EvaluateVector( &updateData.m_colorEval, normalizedLife );
}

void CRenderParticleEmitter::AlphaAbsolute( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Float* alpha = ( Float* )( particle + PARTICLE_ALPHA );
	*alpha = LightEval::EvaluateScalar( &updateData.m_alphaEval, normalizedLife );
}

void CRenderParticleEmitter::AlphaModulate( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Float* alpha = ( Float* )( particle + PARTICLE_ALPHA );
	*alpha *= LightEval::EvaluateScalar( &updateData.m_alphaEval, normalizedLife );
}

//////////////////////////////////////////////////////////////////////////
void CRenderParticleEmitter::RotationAbsolute_1A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Float* rotation = ( Float* )( particle + PARTICLE_ROTATION );
	*rotation = LightEval::EvaluateScalar( &updateData.m_RotEval, normalizedLife );
}

void CRenderParticleEmitter::RotationModulate_1A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Float* rotation = ( Float* )( particle + PARTICLE_ROTATION );
	*rotation *= LightEval::EvaluateScalar( &updateData.m_RotEval, normalizedLife );
}

void CRenderParticleEmitter::RotRateAbsolute_1A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Float* rotRate = ( Float* )( particle + PARTICLE_ROTATION_RATE );
	*rotRate = LightEval::EvaluateScalar( &updateData.m_RotRateEval, normalizedLife );
}

void CRenderParticleEmitter::RotRateModulate_1A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Float* rotRate = ( Float* )( particle + PARTICLE_ROTATION_RATE );
	*rotRate *= LightEval::EvaluateScalar( &updateData.m_RotRateEval, normalizedLife );
}

void CRenderParticleEmitter::SizeAbsolute_2D( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Vector2* size = ( Vector2* )( particle + PARTICLE_SIZE );
	*size = LightEval::EvaluateVector( &updateData.m_sizeEval, normalizedLife );
}

void CRenderParticleEmitter::SizeModulate_2D( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Vector2* size = ( Vector2* )( particle + PARTICLE_SIZE );
	*size *= LightEval::EvaluateVector( &updateData.m_sizeEval, normalizedLife );
}

//////////////////////////////////////////////////////////////////////////
void CRenderParticleEmitter::RotationAbsolute_3A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Vector3* rot3D = ( Vector3* )( particle + PARTICLE_ROTATION3D );
	*rot3D = LightEval::EvaluateVector( &updateData.m_rotEval3D, normalizedLife );
}

void CRenderParticleEmitter::RotationModulate_3A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Vector3* rot3D = ( Vector3* )( particle + PARTICLE_ROTATION3D );
	*rot3D *= LightEval::EvaluateVector( &updateData.m_rotEval3D, normalizedLife );
}

void CRenderParticleEmitter::RotRateAbsolute_3A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Vector3* rotRate3D = ( Vector3* )( particle + PARTICLE_ROTATION_RATE3D );
	Vector3* rot3D = ( Vector3* )( particle + PARTICLE_ROTATION3D );
	*rotRate3D = LightEval::EvaluateVector( &updateData.m_rotRateEval3D, normalizedLife );
}

void CRenderParticleEmitter::RotRateModulate_3A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Vector3* rotRate3D = ( Vector3* )( particle + PARTICLE_ROTATION_RATE3D );
	Vector3* rot3D = ( Vector3* )( particle + PARTICLE_ROTATION3D );
	*rotRate3D *= LightEval::EvaluateVector( &updateData.m_rotRateEval3D, normalizedLife );
}

void CRenderParticleEmitter::SizeAbsolute_3D( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Vector3* size3D = ( Vector3* )( particle + PARTICLE_SIZE3D );
	*size3D = LightEval::EvaluateVector( &updateData.m_sizeEval3D, normalizedLife );
}

void CRenderParticleEmitter::SizeModulate_3D( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Vector3* size3D = ( Vector3* )( particle + PARTICLE_SIZE3D );
	*size3D *= LightEval::EvaluateVector( &updateData.m_sizeEval3D, normalizedLife );
}
//////////////////////////////////////////////////////////////////////////

void CRenderParticleEmitter::AnimationFrame_Speed( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Float* animFrame = ( Float* )( particle + PARTICLE_FRAME );
	Float* baseAnimFrame = ( Float* )( particle + PARTICLE_BASE_FRAME );

	const Float speed = LightEval::EvaluateScalar( &updateData.m_animFrameEval, updateContext.m_emitterTime );

	// A constant frames-per-second animation
	const Float animatedFrameDelta = speed * updateContext.m_timeDelta;
	*baseAnimFrame += animatedFrameDelta;
	*animFrame += animatedFrameDelta;		
}

void CRenderParticleEmitter::AnimationFrame_Life( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Float* animFrame = ( Float* )( particle + PARTICLE_FRAME );
	Float* life = ( Float* )( particle + PARTICLE_LIFE );
	Float* lifeSpanInv = ( Float* )( particle + PARTICLE_LIFE_SPAN_INV );

	const Float speed = LightEval::EvaluateScalar( &updateData.m_animFrameEval, updateContext.m_emitterTime );

	// A particle life time based animation
	const Float particleRelativeLife = ( *life ) * ( *lifeSpanInv );
	const Float animationFrame = particleRelativeLife * speed;
	*animFrame += animationFrame;
}

void CRenderParticleEmitter::Turbulize( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Float* turbulenceCounter = ( Float* )( particle + PARTICLE_TURBULENCE_COUNTER );
	Vector3* velocity = ( Vector3* )( particle + PARTICLE_VELOCITY );
	Vector3* baseVelocity = ( Vector3* )( particle + PARTICLE_BASE_VELOCITY );
	Vector3* turbulenceScale = ( Vector3* )( particle + PARTICLE_TURBULENCE );
	Float* life = ( Float* )( particle + PARTICLE_LIFE );
	Float* lifeSpanInv = ( Float* )( particle + PARTICLE_LIFE_SPAN_INV );

	Float timelifeLimit = 1.0f;
	if( updateData.m_turbulenceTimeLifeLimit.m_samples.Size() )
	{
		timelifeLimit = LightEval::EvaluateScalar( &updateData.m_turbulenceTimeLifeLimit, normalizedLife );
	}

	if ( *turbulenceCounter > updateData.m_turbulenceNoiseInterval )
	{
		// Time to add new turbulence
		// Calculate normalized life of particle
		//const Float particleNormalizedLife = (*life) * (*lifeSpanInv);

		// Evaluate new turbulence
		Vector turbulence = GRandomVector( updateData.m_turbulenceScale.m_samples[0], updateData.m_turbulenceScale.m_samples[1] );
		*turbulenceCounter = 0.0f;

		turbulence *= timelifeLimit;
		//if ( m_absolute )
		//{
		//particle->m_baseVelocity = particle->m_turbulence;
		//}
		//else
		//{
		*baseVelocity += turbulence;
		//}

		*turbulenceScale = turbulence;
	}
	*turbulenceCounter += updateContext.m_timeDelta;

	const Float turbulenceRatio = ( Red::Math::NumericalUtils::Min( 1.0f, Red::Math::NumericalUtils::Max( 0.0f, *turbulenceCounter / updateData.m_turbulenceDuration ) ) );

	//if ( m_absolute )
	//{
	//	particle->m_velocity = particle->m_baseVelocity * ( 1.0f - turbulenceRatio ) + particle->m_turbulence * turbulenceRatio;
	//}
	//else
	//{

	*velocity += *turbulenceScale * turbulenceRatio * timelifeLimit;

	//}
}

void CRenderParticleEmitter::TargetNode( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	// Get the alpha of the particle
	const Float forceScale = LightEval::EvaluateScalar( &updateData.m_targetForceScale, updateContext.m_emitterTime );
	const Float killRadius = LightEval::EvaluateScalar( &updateData.m_targetKillRadius, updateContext.m_emitterTime );

	//// Move target position to world space
	//if ( !updateContext.m_targetNode )
	//{
	//	return;
	//}

	const Vector3& worldPosition = updateContext.m_targetTranslation;

	Vector3* particlePosition = ( Vector3* )( particle + PARTICLE_POSITION );

	// Accelerate particle
	Vector3 delta = worldPosition - *particlePosition;
	Float distance = delta.Normalize();
	Vector3 force = delta * ( forceScale / ( distance/*distance*/ ) );

	// Limit force
	const Float maxForceSq = updateData.m_targetMaxForce * updateData.m_targetMaxForce;
	if ( force.SquareMag() > maxForceSq )
	{
		force.Normalize();
		force *= updateData.m_targetMaxForce;
	}

	// Accelerate particle
	Vector3* baseVelocity = ( Vector3* )( particle + PARTICLE_BASE_VELOCITY );
	Vector3* velocity = ( Vector3* )( particle + PARTICLE_VELOCITY );

	*baseVelocity += force * updateContext.m_timeDelta;
	*velocity += force * updateContext.m_timeDelta;

	// Kill particle
	if ( killRadius > 0.0f )
	{
		// Kill particle if to close to the target
		if ( distance <= killRadius )
		{
			actionPerformed |= PARTICLE_KILLED;
		}
	}
}

void CRenderParticleEmitter::TargetPosition( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	// Get the alpha of the particle
	const Vector position = LightEval::EvaluateVector( &updateData.m_targetPosition, updateContext.m_emitterTime );
	const Float forceScale = LightEval::EvaluateScalar( &updateData.m_targetForceScale, updateContext.m_emitterTime );
	const Float killRadius = LightEval::EvaluateScalar( &updateData.m_targetKillRadius, updateContext.m_emitterTime );

	// Move target position to world space
	const Vector3 worldPosition = updateContext.m_componentTransform.TransformPoint( position );

	Vector3* particlePosition = ( Vector3* )( particle + PARTICLE_POSITION );

	// Accelerate particles
	Vector3 delta = worldPosition - *particlePosition;
	Float distance = delta.Normalize();
	Vector3 force = delta * ( forceScale / ( distance/*distance*/ ) );

	// Limit force
	const Float maxForceSq = updateData.m_targetMaxForce * updateData.m_targetMaxForce;
	if ( force.SquareMag() > maxForceSq )
	{
		force.Normalize();
		force *= updateData.m_targetMaxForce;
	}

	// Accelerate particle
	Vector3* baseVelocity = ( Vector3* )( particle + PARTICLE_BASE_VELOCITY );
	Vector3* velocity = ( Vector3* )( particle + PARTICLE_VELOCITY );

	*baseVelocity += force * updateContext.m_timeDelta;
	*velocity += force * updateContext.m_timeDelta;

	// Kill particle
	if ( killRadius > 0.0f )
	{
		// Kill particle if to close to the target
		if ( distance <= killRadius )
		{
			actionPerformed |= PARTICLE_KILLED;
		}
	}
}

void CRenderParticleEmitter::EffectAlpha( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	Float* alpha = ( Float* )( particle + PARTICLE_ALPHA );
	*alpha *= updateContext.m_effectAlpha;
}

void CRenderParticleEmitter::Collision( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed )
{
	if( !updateContext.m_wrapper || updateData.m_collisionEmitterIndex < 0 ) return;

	Vector3* currentPos = ( Vector3* )( particle + PARTICLE_POSITION );
	Vector3* velocity = ( Vector3* )( particle + PARTICLE_VELOCITY );
	Vector3* turbulenceScale = ( Vector3* )( particle + PARTICLE_TURBULENCE );

	Bool colliding = false;
	updateContext.m_wrapper->GetParticleOutput( updateData.m_collisionEmitterIndex, particleIndex, currentPos, velocity, &colliding );
	if( colliding )
	{
		actionPerformed |= PARTICLE_COLLIDED;

		updateContext.m_wrapper->PushCachedCollision( updateData.m_collisionEmitterIndex, *currentPos );

		if( updateData.m_collisionKillWhenCollide )
		{
			actionPerformed |= PARTICLE_KILLED;
		}
	}

	if( updateData.m_turbulenceNoiseInterval > 0.0f )
	{
		float result = Red::Math::NumericalUtils::Min( 1.0f, ( ( updateData.m_turbulenceNoiseInterval * updateContext.m_timeDelta ) / updateData.m_turbulenceDuration ) * 10.0f );

		Vector3 vec = *turbulenceScale * result;

		updateContext.m_wrapper->ApplyVelocity( updateData.m_collisionEmitterIndex, particleIndex, vec );
	}

}

void CRenderParticleEmitter::AlphaDistance( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex, Uint32& actionPerformed )
{
	Float* alpha = ( Float* )( particle + PARTICLE_ALPHA );
	Vector3* currentPos = ( Vector3* )( particle + PARTICLE_POSITION );

	Vector3 delta = *currentPos - updateContext.m_cameraPosition;
	if ( updateContext.m_localSimulation )
	{
		delta += updateContext.m_componentTransform.GetTranslation();
	}

	Float cameraDist = delta.Mag();
	Float normalizedDist = ( cameraDist - updateData.m_nearDistance ) / ( updateData.m_farDistance - updateData.m_nearDistance );
	*alpha *= ::Clamp< Float >( normalizedDist, 0.f, 1.f );
}