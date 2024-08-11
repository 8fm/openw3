/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "renderParticleEmitter.h"
#include "renderParticleBuffer.h"
#include "..\physics\physicsParticleWrapper.h"
#include "..\engine\baseEngine.h"
#include "..\engine\evaluatorVector.h"

//////////////////////////////////////////////////////////////////////////
// Static particle initialization methods
//////////////////////////////////////////////////////////////////////////	
void CRenderParticleEmitter::InitAlpha( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Float* baseAlpha = ( Float* )( particle + PARTICLE_BASE_ALPHA );

	*baseAlpha = LightEval::EvaluateScalar( &updateData.m_alphaInit, initContext.m_normalizedSpawnTime );	
}

void CRenderParticleEmitter::InitAlphaRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Float* baseAlpha = ( Float* )( particle + PARTICLE_BASE_ALPHA );

	*baseAlpha = generator.Get< Float >( updateData.m_alphaInit.m_samples[0], updateData.m_alphaInit.m_samples[1] );
}

void CRenderParticleEmitter::InitColor( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Vector3* baseColor = ( Vector3* )( particle + PARTICLE_BASE_COLOR );

	*baseColor = LightEval::EvaluateVector( &updateData.m_colorInit, initContext.m_normalizedSpawnTime );
}

void CRenderParticleEmitter::InitColorRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Vector3* baseColor = ( Vector3* )( particle + PARTICLE_BASE_COLOR );

	*baseColor = GRandomVector3<FastRand>( updateData.m_colorInit.m_samples[0], updateData.m_colorInit.m_samples[1], generator );
}

void CRenderParticleEmitter::InitLifeTime( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Float* lifeTime = ( Float* )( particle + PARTICLE_LIFE );
	Float* lifeTimeSpanInv = ( Float* )( particle + PARTICLE_LIFE_SPAN_INV );

	const Float life = Max< Float >( 0.001f, LightEval::EvaluateScalar( &updateData.m_lifeTimeInit, initContext.m_normalizedSpawnTime ) );

	*lifeTime = 0.0f;
	*lifeTimeSpanInv = 1.0f / life;
}

void CRenderParticleEmitter::InitLifeTimeRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Float* lifeTime = ( Float* )( particle + PARTICLE_LIFE );
	Float* lifeTimeSpanInv = ( Float* )( particle + PARTICLE_LIFE_SPAN_INV );

	const Float life = Max< Float >(
		0.001f,
		updateData.m_lifeTimeInit.m_samples[0] + (updateData.m_lifeTimeInit.m_samples[1] - updateData.m_lifeTimeInit.m_samples[0]) * generator.Get< Float >()
		);

	*lifeTime = 0.0f;
	*lifeTimeSpanInv = 1.0f / life;
}

void CRenderParticleEmitter::InitPosition( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Vector3* position = ( Vector3* )( particle + PARTICLE_POSITION );

	Vector3 pos = LightEval::EvaluateVector( &updateData.m_positionInit, initContext.m_normalizedSpawnTime );
	pos = initContext.m_localToWorld.TransformPoint( pos );

	Vector3 partToCam = ( initContext.m_cameraPosition - pos ).Normalized3();
	*position = pos + partToCam * updateData.m_positionOffset;
}

void CRenderParticleEmitter::InitPositionRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Vector3* position = ( Vector3* )( particle + PARTICLE_POSITION );

	Vector3 pos = GRandomVector3<FastRand>( updateData.m_positionInit.m_samples[0], updateData.m_positionInit.m_samples[1], generator );
	pos = initContext.m_localToWorld.TransformPoint( pos );

	Vector3 partToCam = ( initContext.m_cameraPosition - pos ).Normalized3();
	*position = pos + partToCam * updateData.m_positionOffset;
}

void CRenderParticleEmitter::InitRotation( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Float* rotation = ( Float* )( particle + PARTICLE_ROTATION );
	const Float rot = LightEval::EvaluateScalar( &updateData.m_rotationInit, initContext.m_normalizedSpawnTime );

	*rotation = rot * 2.0f * M_PI;
}

void CRenderParticleEmitter::InitRotationRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Float* rotation = ( Float* )( particle + PARTICLE_ROTATION );
	const Float rot = updateData.m_rotationInit.m_samples[0] + (updateData.m_rotationInit.m_samples[1] - updateData.m_rotationInit.m_samples[0]) * generator.Get< Float >();

	*rotation = rot * 2.0f * M_PI;
}

void CRenderParticleEmitter::InitRotation3D( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Vector3* rotation3D = ( Vector3* )( particle + PARTICLE_ROTATION3D );
	const Vector3 rot3D = LightEval::EvaluateVector( &updateData.m_rotation3DInit, initContext.m_normalizedSpawnTime );

	*rotation3D = rot3D;
}

void CRenderParticleEmitter::InitRotation3DRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Vector3* rotation3D = ( Vector3* )( particle + PARTICLE_ROTATION3D );
	const Vector3 rot3D = GRandomVector3<FastRand>( updateData.m_rotation3DInit.m_samples[0], updateData.m_rotation3DInit.m_samples[1], generator );

	*rotation3D = rot3D;
}

void CRenderParticleEmitter::InitRotationRate( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Float* baseRotationRate = ( Float* )( particle + PARTICLE_BASE_ROTATION_RATE );
	const Float rr = LightEval::EvaluateScalar( &updateData.m_rotationRateInit, initContext.m_normalizedSpawnTime );

	*baseRotationRate = rr * 2.0f * M_PI;
}

void CRenderParticleEmitter::InitRotationRateRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Float* baseRotationRate = ( Float* )( particle + PARTICLE_BASE_ROTATION_RATE );
	const Float rr = updateData.m_rotationRateInit.m_samples[0] + (updateData.m_rotationRateInit.m_samples[1] - updateData.m_rotationRateInit.m_samples[0]) * generator.Get< Float >();

	*baseRotationRate = rr * 2.0f * M_PI;
}

void CRenderParticleEmitter::InitRotationRate3D( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Vector3* baseRotationRate3D = ( Vector3* )( particle + PARTICLE_BASE_ROTATION_RATE3D );
	const Vector3 rr = LightEval::EvaluateVector( &updateData.m_rotationRate3DInit, initContext.m_normalizedSpawnTime );

	*baseRotationRate3D = rr * 2.0f * M_PI;
}

void CRenderParticleEmitter::InitRotationRate3DRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Vector3* baseRotationRate3D = ( Vector3* )( particle + PARTICLE_BASE_ROTATION_RATE3D );
	const Vector3 rr = GRandomVector3<FastRand>( updateData.m_rotationRate3DInit.m_samples[0], updateData.m_rotationRate3DInit.m_samples[1], generator );

	*baseRotationRate3D = rr * 2.0f * M_PI;
}

void CRenderParticleEmitter::InitSize( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Vector2* baseSize = ( Vector2* )( particle + PARTICLE_BASE_SIZE );
	const Vector2 bs = LightEval::EvaluateVector( &updateData.m_sizeInit, initContext.m_normalizedSpawnTime );

	*baseSize = bs;
}

void CRenderParticleEmitter::InitSizeRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Vector2* baseSize = ( Vector2* )( particle + PARTICLE_BASE_SIZE );

	if ( updateData.m_sizeSpillFirstAxis )
	{
		const Float bs = updateData.m_sizeInit.m_samples[0].X + (updateData.m_sizeInit.m_samples[1].X - updateData.m_sizeInit.m_samples[0].X) * generator.Get< Float >();
		baseSize->X = bs;
		baseSize->Y = bs;
	}
	else
	{
		const Vector2 bs = GRandomVector3<FastRand>( updateData.m_sizeInit.m_samples[0], updateData.m_sizeInit.m_samples[1], generator ).AsVector2();
		*baseSize = bs;
	}
}

void CRenderParticleEmitter::InitSize3D( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Vector3* baseSize3D = ( Vector3* )( particle + PARTICLE_BASE_SIZE3D );
	const Vector3 bs3D = LightEval::EvaluateVector( &updateData.m_size3DInit, initContext.m_normalizedSpawnTime );

	*baseSize3D = bs3D;
}

void CRenderParticleEmitter::InitSize3DRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Vector3* baseSize3D = ( Vector3* )( particle + PARTICLE_BASE_SIZE3D );

	if ( updateData.m_sizeSpillFirstAxis )
	{
		const Float bs = generator.Get< Float >( updateData.m_size3DInit.m_samples[0].X, updateData.m_size3DInit.m_samples[1].X );
		baseSize3D->X = bs;
		baseSize3D->Y = bs;
		baseSize3D->Z = bs;
	}
	else
	{
		const Vector3 bs3D = GRandomVector3<FastRand>( updateData.m_size3DInit.m_samples[0], updateData.m_size3DInit.m_samples[1], generator );
		*baseSize3D = bs3D;
	}

}

void CRenderParticleEmitter::InitSpawnBox( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	// Get the velocity to set
	const Float frac = initContext.m_frac;

	Vector3* position = ( Vector3* )( particle + PARTICLE_POSITION );


	// Get the size of the spawn box
	const Vector3 extents = LightEval::EvaluateVector( &updateData.m_spawnBoxExtents, initContext.m_normalizedSpawnTime );

	// Calculate the deltas
	Float dx = generator.Get< Float >( -1.0f , 1.0f );
	Float dy = generator.Get< Float >( -1.0f , 1.0f );
	Float dz = generator.Get< Float >( -1.0f , 1.0f );

	// Calculate the position delta
	const Vector boxDelta( dx, dy, dz, 1.0f );
	Vector delta = boxDelta * extents;

	// Transfer to world space
	if ( !updateData.m_spawn_BoxCircle_WorldSpace )
	{
		if ( initContext.m_subframeInterpolator )
		{
			initContext.m_subframeInterpolator->InterpolateVector( frac, delta, delta );
		}
		else
		{
			delta = initContext.m_localToWorld.TransformVector( delta );
		}
	}

	// Move the particle
	*position += delta;
}
void CRenderParticleEmitter::InitSpawnCircle( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	// Get the velocity to set
	const Float frac = initContext.m_frac;

	Vector3* position = ( Vector3* )( particle + PARTICLE_POSITION );

	// Get the radius of the spawn sphere
	const Float outerRadius = LightEval::EvaluateScalar( &updateData.m_spawnCircleOuterRadius, initContext.m_normalizedSpawnTime );
	const Float innerRadius = LightEval::EvaluateScalar( &updateData.m_spawnCircleInnerRadius, initContext.m_normalizedSpawnTime );

	// Calculate the random values
	const Float angleTheta = generator.Get< Float >( 2.0f * M_PI );
	const Float radius = updateData.m_spawn_CircleSphere_SurfaceOnly ? outerRadius : generator.Get< Float >( innerRadius , outerRadius );

	// Calculate the spawn deltas
	const Float dx = cos( angleTheta ) * radius;
	const Float dy = sin( angleTheta ) * radius;
	const Float dz = 0.0f;

	// Move by circle radius
	Vector spawnDelta( dx, dy, dz, 0.0f );
	if ( updateData.m_spawnCircleRotation != EulerAngles::ZEROS )
	{
		spawnDelta = updateData.m_spawnCircleSpawnToLocal.TransformVector( spawnDelta );
	}

	// Move to world space
	if ( !updateData.m_spawn_BoxCircle_WorldSpace )
	{
		if ( initContext.m_subframeInterpolator )
		{
			initContext.m_subframeInterpolator->InterpolateVector( frac, spawnDelta, spawnDelta );
		}
		else
		{
			spawnDelta = initContext.m_localToWorld.TransformVector( spawnDelta );
		}
	}

	// Move the particle
	*position += spawnDelta;
}

void CRenderParticleEmitter::InitSpawnSphere( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	// Spawn in positive X,Y,Z values
	Bool spawnPositiveX = updateData.m_spawnSpherePositiveX;
	Bool spawnNegativeX = updateData.m_spawnSphereNegativeX;
	Bool spawnPositiveY = updateData.m_spawnSpherePositiveY;
	Bool spawnNegativeY = updateData.m_spawnSphereNegativeY;
	Bool spawnPositiveZ = updateData.m_spawnSpherePositiveZ;
	Bool spawnNegativeZ = updateData.m_spawnSphereNegativeZ;
	
	Vector3* position = ( Vector3* )( particle + PARTICLE_POSITION );
	Vector3 origPosition = *position;
	Vector3* velocity = ( Vector3* )( particle + PARTICLE_BASE_VELOCITY );
	// Get the radius of the spawn sphere
	const Float outerRadius =	LightEval::EvaluateScalar( &updateData.m_spawnCircleOuterRadius, initContext.m_normalizedSpawnTime );
	const Float innerRadius =	LightEval::EvaluateScalar( &updateData.m_spawnCircleInnerRadius, initContext.m_normalizedSpawnTime );
	//const Float forceScale =	LightEval::EvaluateScalar( &updateData.m_targetForceScale, initContext.m_normalizedSpawnTime );
	const Float forceScale =	generator.Get< Float >( updateData.m_targetForceScale.m_samples[0], updateData.m_targetForceScale.m_samples[1] );
	

	Bool circleNotOnSurfaceX = !spawnPositiveX && !spawnNegativeX;
	Bool circleNotOnSurfaceY = !spawnPositiveY && !spawnNegativeY;
	Bool circleNotOnSurfaceZ = !spawnPositiveZ && !spawnNegativeZ;

	Float dx = 0.0f;
	Float dy = 0.0f;
	Float dz = 0.0f;

	if ( circleNotOnSurfaceX || circleNotOnSurfaceY || circleNotOnSurfaceZ ) // 2D shape
	{
		const Float angleTheta = generator.Get< Float >( 2.0f * M_PI );
		const Float radius = updateData.m_spawn_CircleSphere_SurfaceOnly ? outerRadius : generator.Get< Float >( innerRadius , outerRadius );

		if ( circleNotOnSurfaceX )
		{
			if ( !circleNotOnSurfaceY )
				dy = cos( angleTheta ) * radius;

			if ( !circleNotOnSurfaceZ )
				dz = sin( angleTheta ) * radius;
		}
		
		if ( circleNotOnSurfaceY )
		{
			if ( !circleNotOnSurfaceX )
				dx = cos( angleTheta ) * radius;

			if ( !circleNotOnSurfaceZ )
				dz = sin( angleTheta ) * radius;
		}

		if ( circleNotOnSurfaceZ )
		{
			if ( !circleNotOnSurfaceX )
				dx = cos( angleTheta ) * radius;

			if ( !circleNotOnSurfaceY )
				dy = sin( angleTheta ) * radius;
		}
	}
	else // 3D shape
	{
		// Calculate the random values
		const Float anglePhi   = generator.Get< Float >( -M_PI / 2.0f, M_PI / 2.0f );
		const Float angleTheta = generator.Get< Float >( 2.0f * M_PI );
		const Float radius	   = updateData.m_spawn_CircleSphere_SurfaceOnly ? outerRadius : generator.Get< Float >( innerRadius , outerRadius );

		// Check the X positive and negative checks
		if ( spawnPositiveX || spawnNegativeX )
		{
			// Calculate the position delta
			dx = cos( angleTheta ) * cos( anglePhi ) * radius;	
		}
		// Check the Y positive and negative checks
		if ( spawnPositiveY || spawnNegativeY )
		{
			// Calculate the position delta
			dy = sin( angleTheta ) * cos( anglePhi ) * radius;
		}

		// Check the Z positive and negative checks
		if ( spawnPositiveZ || spawnNegativeZ )
		{
			// Calculate the position delta
			dz = sin( anglePhi ) * radius;	
		}
	}

	if( spawnPositiveX && !spawnNegativeX && dx < 0)
	{
		dx *= -1;
	}
	if( spawnNegativeX && !spawnPositiveX && dx > 0 )
	{
		dx *= -1;
	}	

	if( spawnPositiveY && !spawnNegativeY && dy < 0 )
	{
		dy *= -1;
	}
	if( spawnNegativeY && !spawnPositiveY && dy > 0 )
	{
		dy *= -1;
	}	

	if( spawnPositiveZ && !spawnNegativeZ && dz < 0 )
	{
		dz *= -1;
	}
	if( spawnNegativeZ && !spawnPositiveZ && dz > 0 )
	{
		dz *= -1;
	}	
	
	const Vector spawnDelta( dx, dy, dz, 0.0f );
	*position += spawnDelta;
	
	if ( updateData.m_spawnSphereVelocity )
	{	
 		Vector3 delta = *position - origPosition;
		Vector3 force = delta * forceScale;

		// Get the velocity of the particle
		*velocity += force;
	}
}

void CRenderParticleEmitter::InitVelocity( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	// Get the velocity to set
	const Float frac = initContext.m_frac;

	Vector3* baseVelocity = ( Vector3* )( particle + PARTICLE_BASE_VELOCITY );

	// Get the velocity of the particle
	Vector vel = LightEval::EvaluateVector( &updateData.m_velocityInit, initContext.m_normalizedSpawnTime );

	// Move it to world space if needed
	if ( !updateData.m_velocityInitWorldSpace )
	{
		if ( initContext.m_subframeInterpolator )
		{
			initContext.m_subframeInterpolator->InterpolateVector( frac, vel, vel );
		}
		else
		{
			vel = initContext.m_localToWorld.TransformVector( vel );
		}
	}

	// Set particle velocity ( additive )
	*baseVelocity += vel;
}

void CRenderParticleEmitter::InitVelocityRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	// Get the velocity to set
	const Float frac = initContext.m_frac;

	Vector3* baseVelocity = ( Vector3* )( particle + PARTICLE_BASE_VELOCITY );

	// Get the velocity of the particle
	Vector vel = GRandomVector3<FastRand>( updateData.m_velocityInit.m_samples[0], updateData.m_velocityInit.m_samples[1], generator );

	// Move it to world space if needed
	if ( !updateData.m_velocityInitWorldSpace )
	{
		if ( initContext.m_subframeInterpolator )
		{
			initContext.m_subframeInterpolator->InterpolateVector( frac, vel, vel );
		}
		else
		{
			vel = initContext.m_localToWorld.TransformVector( vel );
		}
	}

	// Set particle velocity ( additive )
	*baseVelocity += vel;
}

void CRenderParticleEmitter::InitVelocityInherit( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	// Get the velocity to set
	Vector3* baseVelocity = ( Vector3* )( particle + PARTICLE_BASE_VELOCITY );

	// Get the inherited velocity
	Vector vel = initContext.m_baseVelocity;

	// Set the particle initial velocity
	const Float scale = LightEval::EvaluateScalar( &updateData.m_velocityInheritInit, initContext.m_normalizedSpawnTime );
	*baseVelocity += vel * scale;
}

void CRenderParticleEmitter::InitVelocityInheritRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	// Get the velocity to set
	Vector3* baseVelocity = ( Vector3* )( particle + PARTICLE_BASE_VELOCITY );

	// Get the inherited velocity
	Vector vel = initContext.m_baseVelocity;

	// Set the particle initial velocity
	const Float scale = generator.Get< Float >( updateData.m_velocityInheritInit.m_samples[0], updateData.m_velocityInheritInit.m_samples[1] );
	*baseVelocity += vel * scale;
}

void CRenderParticleEmitter::InitVelocitySpread( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Vector3* baseVelocity = ( Vector3* )( particle + PARTICLE_BASE_VELOCITY );

	// Calculate the spread, absolute spread depends on the current velocity
	const Float currentVelocity = baseVelocity->Mag();
	const Float spreadValue = LightEval::EvaluateScalar( &updateData.m_velocitySpreadInit, initContext.m_normalizedSpawnTime ) * currentVelocity;
	const Vector spreadVector = GRandomVector3<FastRand>( Vector( -spreadValue, -spreadValue, -spreadValue, 0.0f ), Vector( spreadValue, spreadValue, spreadValue, 0.0f ), generator );

	// Change velocity
	if ( updateData.m_conserveVelocitySpreadMomentum )
	{
		// Conserve particle current momentum
		*baseVelocity += spreadVector;
		baseVelocity->Normalize();
		*baseVelocity *= currentVelocity;
	}
	else
	{
		// Just change velocity
		*baseVelocity += spreadVector;
	}
}

void CRenderParticleEmitter::InitAnimationFrame( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Float* baseFrame = ( Float* )( particle + PARTICLE_BASE_FRAME );

	*baseFrame = LightEval::EvaluateScalar( &updateData.m_animFrameInit, initContext.m_normalizedSpawnTime );
}

void CRenderParticleEmitter::InitAnimationFrameRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& initContext, Generator< FastRand >& generator )
{
	Float* baseFrame = ( Float* )( particle + PARTICLE_BASE_FRAME );

	*baseFrame = updateData.m_animFrameInit.m_samples[0] + (updateData.m_animFrameInit.m_samples[1] - updateData.m_animFrameInit.m_samples[0]) * generator.Get< Float >();
}

void CRenderParticleEmitter::InitCollisionSpawn( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator )
{
	if( !updateContext.m_wrapper || updateData.m_collisionSpawnParentModuleIndex < 0 ) return;

	Vector3* currentPos = ( Vector3* )( particle + PARTICLE_POSITION );

	updateContext.m_wrapper->PopCachedCollision( updateData.m_collisionSpawnParentModuleIndex, currentPos );
}
