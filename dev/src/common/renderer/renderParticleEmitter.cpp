/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderParticleEmitter.h"
#include "renderMesh.h"
#include "renderScene.h"
#include "../../common/engine/decalSpawner.h"
#include "..\physics\physicsParticleWrapper.h"
#include "../engine/mesh.h"
#include "../engine/evaluatorFloat.h"
#include "../engine/particleEmitter.h"
#include "../engine/baseEngine.h"
#include "../engine/particleDrawer.h"

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
extern Bool isAnselParticleAttach;
#endif // USE_ANSEL

CRenderDecalSpawner::CRenderDecalSpawner( const CDecalSpawner* decalSpawner )
	: m_materialDiffuseTexture( nullptr )
	, m_materialNormalTexture( nullptr )
	, m_diffuseRandomColor0( decalSpawner->m_material.m_diffuseRandomColor0 )
	, m_diffuseRandomColor1( decalSpawner->m_material.m_diffuseRandomColor1 )
	, m_materialSpecularColor( decalSpawner->m_material.m_specularColor )
	, m_materialSpecularScale( decalSpawner->m_material.m_specularScale )
	, m_materialSpecularBase( decalSpawner->m_material.m_specularBase )
	, m_materialSpecularity( decalSpawner->m_material.m_specularity )
	, m_farZ( decalSpawner->m_farZ )
	, m_nearZ( decalSpawner->m_nearZ )
	, m_depthFadePower( decalSpawner->m_depthFadePower )
	, m_normalFadeBias( decalSpawner->m_normalFadeBias )
	, m_normalFadeScale( decalSpawner->m_normalFadeScale )
	, m_doubleSided( decalSpawner->m_doubleSided )
	, m_projectionMode( decalSpawner->m_projectionMode )
	, m_decalFadeTime( decalSpawner->m_decalFadeTime )
	, m_decalFadeInTime( decalSpawner->m_decalFadeInTime )
	, m_projectOnlyOnStatic( decalSpawner->m_projectOnlyOnStatic)
	, m_startScale( decalSpawner->m_startScale)
	, m_scaleTime(decalSpawner->m_scaleTime)
	, m_useVerticalProjection(decalSpawner->m_useVerticalProjection)
	, m_spawnPriority( decalSpawner->m_spawnPriority )
	, m_size( nullptr )
	, m_decalLifetime( nullptr )
	, m_autoHideDistance( decalSpawner->m_autoHideDistance )
	, m_chance( decalSpawner->m_chance )
	, m_subUVType( decalSpawner->m_material.m_subUVType )
	, m_lastSpawnTime( 0.0f )
	, m_spawnFrequency( decalSpawner->m_spawnFrequency )
{
	ExtractRenderResource( decalSpawner->m_material.m_diffuseTexture.Get(), m_materialDiffuseTexture );
	ExtractRenderResource( decalSpawner->m_material.m_normalTexture.Get(), m_materialNormalTexture );

	const IEvaluatorFloat* size = decalSpawner->m_size;
	if ( size ) 
	{
		m_size = new SCurveEvaluatorScalar;
		size->GetApproximationSamples( m_size->m_samples );
	}

	const IEvaluatorFloat* lifetime = decalSpawner->m_decalLifetime;
	if ( lifetime ) 
	{
		m_decalLifetime = new SCurveEvaluatorScalar;
		lifetime->GetApproximationSamples( m_decalLifetime->m_samples );
	}
}

CRenderDecalSpawner::~CRenderDecalSpawner()
{
	SAFE_RELEASE( m_materialDiffuseTexture );
	SAFE_RELEASE( m_materialNormalTexture );

	delete m_size;
	delete m_decalLifetime;
}

void CRenderDecalSpawner::SpawnDecalForParticle( const Vector3* position, const Vector3* velocity, const Vector* cameraPosition  ) const
{
	// no way to determine up / forward vectors, just quit
	if ( velocity->SquareMag() < NumericLimits<Float>::Epsilon() )
	{
		return;
	}

	if ( m_materialDiffuseTexture == nullptr )
	{
		return;
	}
	
	{
		const Float randomValue = GetRenderer()->GetRandomNumberGenerator().Get< Float >();
		if( randomValue > m_chance )
		{
			return;
		}
	}

	{
		const Float chance = Config::cvDecalsChance.Get();
		const Float spawnDistanceCutoff = Config::cvDecalsSpawnDistanceCutoff.Get();
		const Float randomValue = GetRenderer()->GetRandomNumberGenerator().Get< Float >();
		if ( randomValue > chance || cameraPosition->DistanceSquaredTo( *position ) > spawnDistanceCutoff*spawnDistanceCutoff )
		{
			return;
		}
	}

	// Parameters normalization
	const Vector& origin = *position;

	Vector forward = Vector( 0.0f, 0.0f, -1.0f );

	if (!m_useVerticalProjection || ::Abs(velocity->Normalized().Z) > 1.0f-NumericLimits<Float>::Epsilon() )
	{
		forward = velocity->Normalized();
	}
	forward.W = 1.0f;
	
	Vector side = Vector(velocity->X, velocity->Y, 0.0f ).Normalized3();
	
	if (!m_useVerticalProjection || ::Abs(velocity->Normalized().Z) > 1.0f-NumericLimits<Float>::Epsilon() )
	{
		// Error when velocity is exactly 0,0,1 or 0,0,-1
		if( ::Abs( forward.Z ) < 1.0f-NumericLimits<Float>::Epsilon() )
		{
			side = Vector::Cross( forward, Vector( 0.0f, 0.0f, 1.0f ) ).Normalized3(); 
		}
		else
		{
			side = Vector( 0.0f , -forward.Z , 0.0f );
		}
	}

	Vector up = Vector::Cross( side, forward );

	CWorld* world = GGame->GetActiveWorld();
	if ( world == nullptr )
	{
		return;
	}

	IRenderScene* scene = world->GetRenderSceneEx();
	if ( scene == nullptr )
	{
		return;
	}

	SDynamicDecalInitInfo initInfo;
	Float size = LightEval::EvaluateScalar( m_size, 1.0f );

	initInfo.m_origin					= origin;
	initInfo.m_dirFront					= forward;
	initInfo.m_dirUp					= up;
	initInfo.m_width					= size;
	initInfo.m_height					= size;
	initInfo.m_farZ						= m_farZ;
	initInfo.m_nearZ					= m_nearZ;

	initInfo.m_depthFadePower			= m_depthFadePower;
	initInfo.m_normalFadeBias			= m_normalFadeBias;
	initInfo.m_normalFadeScale			= m_normalFadeScale;
	initInfo.m_doubleSided				= m_doubleSided;
	initInfo.m_projectionMode			= m_projectionMode;
	initInfo.m_projectOnlyOnStatic		= m_projectOnlyOnStatic;

	initInfo.m_timeToLive				= LightEval::EvaluateScalar( m_decalLifetime, 1.0f );
	initInfo.m_fadeTime					= m_decalFadeTime;
	initInfo.m_fadeInTime				= m_decalFadeInTime;
	initInfo.m_startScale				= m_startScale;
	initInfo.m_scaleTime				= m_scaleTime;
	initInfo.m_spawnPriority			= m_spawnPriority;

	initInfo.m_diffuseTexture			= m_materialDiffuseTexture;
	initInfo.m_normalTexture			= m_materialNormalTexture;
	initInfo.m_specularColor			= m_materialSpecularColor;
	initInfo.m_specularScale			= m_materialSpecularScale;
	initInfo.m_specularBase				= m_materialSpecularBase;
	initInfo.m_specularity				= m_materialSpecularity;
	initInfo.m_autoHideDistance			= m_autoHideDistance;

	initInfo.SetAtlasVector( m_subUVType );

	{
		const Float randomValue = GetRenderer()->GetRandomNumberGenerator().Get< Float >();
		initInfo.m_diffuseColor			= Color::Lerp( randomValue, m_diffuseRandomColor0 , m_diffuseRandomColor1 );
	}

	IRenderResource* decal = GetRenderer()->CreateDynamicDecal( initInfo );
	if ( decal != nullptr )
	{
		static_cast< CRenderSceneEx* >( scene )->QueueDynamicDecalSpawn( decal, initInfo.m_projectOnlyOnStatic );
		decal->Release();
	}
}

Bool CRenderDecalSpawner::AllowSpawnParticle()
{
	if( m_spawnFrequency < 0.05f ) return true;

	const Uint32 lastSpawnTime	= static_cast<Uint32>( m_lastSpawnTime / m_spawnFrequency );
	const Uint32 currentTime	= static_cast<Uint32>( static_cast<Float>(GEngine->GetRawEngineTime()) / m_spawnFrequency );

	return currentTime > lastSpawnTime;
}

void CRenderDecalSpawner::MarkSpawnTime()
{
	m_lastSpawnTime = static_cast<Float>(GEngine->GetRawEngineTime());
}

CRenderParticleEmitter::LOD::LOD()
	: m_birthRate( nullptr )
{
}

CRenderParticleEmitter::LOD::~LOD()
{
	if ( m_birthRate )
	{
		delete m_birthRate;
		m_birthRate = nullptr;
	}
}


CRenderParticleEmitter::CRenderParticleEmitter()
	: m_decalSpawner( nullptr )
	, m_collisionDecalSpawner( nullptr )
	, m_motionDecalSpawner( nullptr )
	, m_initializerFunctions( nullptr )
	, m_modifierFunctions( nullptr )
	, m_internalPriority( 0 )
#ifndef NO_EDITOR
	, m_uniqueId( -1 )
#endif
{

}

CRenderParticleEmitter::~CRenderParticleEmitter()
{
	for ( Uint32 i = 0; i<m_renderMeshes.Size(); ++i )
	{
		if ( m_renderMeshes[i] )
		{
			SAFE_RELEASE( m_renderMeshes[i] );
		}
	}

	if ( m_initializerFunctions )
	{
		delete [] m_initializerFunctions;
		m_initializerFunctions = NULL;
	}

	if ( m_modifierFunctions )
	{
		delete [] m_modifierFunctions;
		m_modifierFunctions = NULL;
	}

	if (m_decalSpawner)
	{
		delete m_decalSpawner;
	}

	if (m_collisionDecalSpawner)
	{
		delete m_collisionDecalSpawner;
	}

	if (m_motionDecalSpawner)
	{
		delete m_motionDecalSpawner;
	}
}

template< typename PARTICLE_TYPE >
void CRenderParticleEmitter::SpawnParticles( CParticleData< PARTICLE_TYPE >* data, const SSimulationContext& simulationContext, Uint32 count, Float startTime, Float spawnTimeRange ) const
{
	// See if we can use sub-frame spawning position :)
	const CSubframeInterpolator* subframeInterpolator = NULL;
	if ( m_useSubFrameEmission )
	{
		subframeInterpolator = simulationContext.m_interpolator;
	}

	const Uint32 lod = simulationContext.m_lod;

	RED_ASSERT( lod < m_lods.Size() );
	if ( !m_lods[lod].m_isEnabled )
	{
		return;
	}

	// Spawn particles
	CParticleBuffer< PARTICLE_TYPE >& particleBuffer = data->GetBuffer();
	Uint32 firstSpawned = particleBuffer.Alloc( count );
	if ( firstSpawned != IParticleBuffer::INVALID_PARTICLE_INDEX )
	{
		// Normalize spawn time
		const Float normalizedTimeDeltaSpawnStep = ( spawnTimeRange / GetEmitterDuration( lod ) ) / (Float) count;
		const Float normalizedEmitterTime = ::Clamp< Float >( startTime / GetEmitterDuration( lod ), 0.f, 1.f );

		// Setup initialization context
		ParticleInitializationContext initContext;
		initContext.m_wrapper = simulationContext.m_wrapper;
		initContext.m_baseVelocity = simulationContext.m_velocity;
		initContext.m_normalizedSpawnTime = normalizedEmitterTime;
		initContext.m_normalizedSpawnDelta = normalizedTimeDeltaSpawnStep;
		initContext.m_localToWorld = simulationContext.m_componentTransform;
		if ( m_keepSimulationLocal || m_vertexDrawerType == PVDT_Screen )
		{
			initContext.m_localToWorld.SetIdentity();
		}
		initContext.m_subframeInterpolator = subframeInterpolator;
		initContext.m_cameraPosition = simulationContext.m_cameraPosition;

		// Set the initial particle position
		if ( subframeInterpolator )
		{
			// Spawn all particles in the same spot
			const Float fracStep = 1.0f / ( Float ) count;
			for ( Uint32 i=0; i<count; i++ )
			{
				initContext.m_particleIndex = i + firstSpawned;
				const Float frac = ( i + 0.5f ) * fracStep;
				initContext.m_frac = frac;

				ASSERT( frac <= 1.00001f ); // this should not be 1.0f due to precision of floats and casting int to float

				char* spawnedParticle = ( char* )particleBuffer.GetParticleDataAt( i + firstSpawned );

				// Get main particle fields
				Vector3* pos = ( Vector3* ) ( spawnedParticle + PARTICLE_POSITION );
				Float* life = ( Float* ) ( spawnedParticle + PARTICLE_LIFE );

				// Interpolate spawn position
				Vector initPos;
				subframeInterpolator->InterpolatePosition( frac, initPos );
				*pos = initPos;

				if ( PARTICLE_TYPE::m_fieldSet & PFS_EmitterAxis )
				{
					// Interpolate axis
					Vector3* axis = ( Vector3* )( spawnedParticle + PARTICLE_EMITTER_AXIS );
					Vector emitterAxis;
					subframeInterpolator->InterpolateAxis( frac, emitterAxis );
					*axis = emitterAxis;
				}

				// Apply initializers
				for ( Uint32 j=0; j<m_numInitializers; ++j )
				{
					(*(m_initializerFunctions[j]))( spawnedParticle, m_updaterData, initContext, data->m_randGenerators[j] );
				}

				// Adjust life time
				*life += simulationContext.m_timeDelta * frac;

				if ( PARTICLE_TYPE::m_fieldSet & PFS_Velocity )
				{
					// Adjust position by applying sub frame velocity
					Vector3* baseVelocity = ( Vector3* )( spawnedParticle + PARTICLE_BASE_VELOCITY );
					const Vector step =  *baseVelocity * simulationContext.m_timeDelta * frac;
					*pos += step;
				}

				initContext.m_normalizedSpawnTime += initContext.m_normalizedSpawnDelta;
				initContext.m_normalizedSpawnTime = ::Clamp< Float >( initContext.m_normalizedSpawnTime, 0.f, 1.f );
			}
		}
		else
		{
			// Spawn all particles in the same spot
			const Vector initialPosition = initContext.m_localToWorld.GetTranslation();
			const Float fracStep = 1.0f / ( Float ) count;
			for ( Uint32 i=0; i<count; i++ )
			{
				initContext.m_particleIndex = i + firstSpawned;
				const Float frac = ( i + 0.5f ) * fracStep;
				initContext.m_frac = frac;

				char* spawnedParticle = ( char* )particleBuffer.GetParticleDataAt( i + firstSpawned );
				Vector3* pos = ( Vector3* ) spawnedParticle;

				*pos = initialPosition;

				if ( PARTICLE_TYPE::m_fieldSet & PFS_EmitterAxis )
				{
					// Interpolate axis
					Vector3* axis = ( Vector3* )( spawnedParticle + PARTICLE_EMITTER_AXIS );
					*axis = initContext.m_localToWorld.GetRow(2);
				}

				// Apply initializers
				for ( Uint32 j=0; j<m_numInitializers; ++j )
				{
					(*(m_initializerFunctions[j]))( spawnedParticle, m_updaterData, initContext, data->m_randGenerators[j] );
				}

				initContext.m_normalizedSpawnTime += initContext.m_normalizedSpawnDelta;
				initContext.m_normalizedSpawnTime = ::Clamp< Float >( initContext.m_normalizedSpawnTime, 0.f, 1.f );
			}
		}
		if( simulationContext.m_wrapper && m_updaterData.m_collisionEmitterIndex >= 0 )
		{
			for ( Uint32 i=0; i<count; i++ )
			{
				char* spawnedParticle = ( char* )particleBuffer.GetParticleDataAt( i + firstSpawned );
				Vector3* pos = ( Vector3* ) ( spawnedParticle + PARTICLE_POSITION );
				Vector3* velocity = ( Vector3* ) ( spawnedParticle + PARTICLE_BASE_VELOCITY );
				simulationContext.m_wrapper->RefreshParticle( m_updaterData.m_collisionEmitterIndex, i + firstSpawned, *pos, *velocity );
			}
		}
	}
}

Bool CRenderParticleEmitter::Simulate( IParticleData* data, const SSimulationContext& simulationContext, Bool onScreen ) const
{
	switch( GetParticleType() )
	{
	case PT_Simple:
		return PerformSimulation( ( CParticleData< SimpleParticle >* )( data ), simulationContext, onScreen );
		break;
	case PT_Trail:
		return PerformSimulation( ( CParticleData< TrailParticle >* )( data ), simulationContext, onScreen );
		break;
	case PT_FacingTrail:
		return PerformSimulation( ( CParticleData< FacingTrailParticle >* )( data ), simulationContext, onScreen );
		break;
	case PT_Mesh:
		return PerformSimulation( ( CParticleData< MeshParticle >* )( data ), simulationContext, onScreen );
		break;
	case PT_Beam:
		return PerformSimulation( ( CParticleData< BeamParticle >* )( data ), simulationContext, onScreen );
		break;
	default:
		ASSERT( 0 );
	};

	return false;
}

template< typename PARTICLE_TYPE >
Bool CRenderParticleEmitter::PerformSimulation( CParticleData< PARTICLE_TYPE >* data, const SSimulationContext& simulationContext, Bool onScreen ) const
{
	Float randomRepeat = 0.0f;
	Bool continuesSpawn = false;
	
	const Bool wasUpdatedThisFrame = data->WasUpdatedThisFrame();
	if ( !wasUpdatedThisFrame )
	{
		data->MarkAsUpdatedThisFrame();
	}

	const Uint32 lod = simulationContext.m_lod;
	RED_ASSERT( lod < m_lods.Size() );


	const LOD& emitterLod = m_lods[lod];


	const Bool performSimulation =
		!GIsRendererTakingUberScreenshot && GIsDuringUberSampleIsFirst &&
		GEngine->IsActiveSubsystem( ES_Particles ) && !GGame->IsPaused() &&
		!wasUpdatedThisFrame
#ifdef USE_ANSEL
		|| ( isAnselSessionActive && isAnselParticleAttach )
#endif // USE_ANSEL
		;

	// Update the cycle time
	Float prevTime = data->m_cycleTime;
	
	// Checking if there is a delay for the emitter or not
	if( GetEmitterDelay( lod ) != 0 )
	{
		if( emitterLod.m_emitterDelaySettings.m_useEmitterDelayRange && data->m_randomDelayTime == 0.0 )
		{
			randomRepeat = GEngine->GetRandomNumberGenerator().Get< Float >( GetEmitterDelayLow( lod ), GetEmitterDelay( lod ) );
			data->m_randomDelayTime = randomRepeat;
		}
		if( !emitterLod.m_emitterDelaySettings.m_useEmitterDelayRange )
		{
			data->m_randomDelayTime = GetEmitterDelay( lod );
		}
		// particle has a delay
		data->m_delayTimer += simulationContext.m_timeDelta;

		if ( performSimulation && GetEmitterDuration( lod ) > 0.0f && data->m_randomDelayTime <= data->m_delayTimer )
		{
			if( !data->m_delayedParticles )
			{
				data->m_delayedParticles = true;
				prevTime = 0.0f;
			}
			Int32 cycles = data->m_cycleCount;
			// If the delay is not 0 we need to not spawn anything until the time is right.
			data->m_cycleTime += simulationContext.m_timeDelta;

			Float timesWrappedAround = MFloor( data->m_cycleTime / GetEmitterDuration( lod ) );
			prevTime -= GetEmitterDuration( lod ) * timesWrappedAround;
			data->m_cycleTime -= GetEmitterDuration( lod ) * timesWrappedAround;
			data->m_cycleCount += ( Int32 )timesWrappedAround;
			if( cycles != data->m_cycleCount )
			{
				if( !emitterLod.m_emitterDelaySettings.m_useEmitterDelayOnce )
				{
					data->m_delayedParticles = false;
					data->m_delayTimer = 0.0f;
					if( emitterLod.m_emitterDelaySettings.m_useEmitterDelayRange )
					{
						data->m_randomDelayTime = 0.0;
					}
				}
			}

			// When one cycle has been run then we reset the emitterDelay. 
			ASSERT( data->m_cycleTime >= 0.0f && data->m_cycleTime < GetEmitterDuration( lod ), TXT("Particle data cycleTime is out of range. Wrap-around math broken?") );
		}
	}
	else if( performSimulation && GetEmitterDuration( lod ) > 0.0f )
	{
		// If the delay is not 0 we need to not spawn anything until the time is right.
		data->m_cycleTime += simulationContext.m_timeDelta;

		Float timesWrappedAround = MFloor( data->m_cycleTime / GetEmitterDuration( lod ) );
		prevTime -= GetEmitterDuration( lod ) * timesWrappedAround;
		data->m_cycleTime -= GetEmitterDuration( lod ) * timesWrappedAround;
		data->m_cycleCount += ( Int32 )timesWrappedAround;

		// When one cycle has been run then we reset the emitterDelay
		ASSERT( data->m_cycleTime >= 0.0f && data->m_cycleTime < GetEmitterDuration( lod ), TXT("Particle data cycleTime is out of range. Wrap-around math broken?") );
	}

	// We can spawn particles only if we have not exceeded the cycle count
	Bool canSpawnParticles = false;
	if ( m_emitterLoops < 1 || ( data->m_cycleCount < m_emitterLoops ) )
	{
		// We can still spawn particles
		canSpawnParticles = true;

		Uint32 collisionPositions = 0;
		if( simulationContext.m_wrapper && m_updaterData.m_collisionSpawnParentModuleIndex >= 0 )
		{
			collisionPositions = simulationContext.m_wrapper->GetCachedCollisionCount( m_updaterData.m_collisionSpawnParentModuleIndex );
		}
		
		if( collisionPositions )
		{
			float probability = m_updaterData.m_collisionSpawnProbability;
			if( probability < 1.0f )
			{
				float result = GEngine->GetRandomNumberGenerator().Get< Float >( 0, probability );
				Uint32 drop = collisionPositions - Uint32( collisionPositions * result );
				while( drop-- )
				{
					simulationContext.m_wrapper->PopCachedCollision( m_updaterData.m_collisionSpawnParentModuleIndex );
				}
			}
			collisionPositions = simulationContext.m_wrapper->GetCachedCollisionCount( m_updaterData.m_collisionSpawnParentModuleIndex );
			if( collisionPositions )
			{
				SpawnParticles( data, simulationContext, collisionPositions, prevTime, simulationContext.m_timeDelta );
			}
		}

		// Use bursts list to spawn particles
		if ( emitterLod.m_burstList.Size() && performSimulation )
		{
			// Emit particles from activated burst lists
			for ( Uint32 i=0; i<emitterLod.m_burstList.Size(); i++ )
			{
				const ParticleBurst& burst = emitterLod.m_burstList[i];
				
				if ( burst.m_spawnCount )
				{
					if( !emitterLod.m_emitterDurationSettings.m_useEmitterDurationRange )
					{ 
						if ( burst.m_burstTime >= prevTime && burst.m_burstTime < data->m_cycleTime && !data->m_bursted )
						{
							SpawnParticles( data, simulationContext, burst.m_spawnCount, prevTime, burst.m_spawnTimeRange );
							data->m_lastBurstTime = 0.0f;
														
							// Do not repeat the burst if we have just one burst
							// and its repeat time is less 0.0 (it should be equal 0.0 but less is due to default value all over artists data)
							if( burst.m_repeatTime < 0.0f && emitterLod.m_burstList.Size() == 1 ) 
								data->m_bursted = true;
						}
					}
					else
					{
						if( emitterLod.m_emitterDurationSettings.m_useEmitterDurationRange && !data->m_bursted )
						{
							randomRepeat = GEngine->GetRandomNumberGenerator().Get< Float >( GetEmitterDurationLow( lod ), GetEmitterDuration( lod ) );
							data->m_bursted = true;
							data->m_randomBurstTime = randomRepeat;
						}

						data->m_lastBurstTime += simulationContext.m_timeDelta;

						if ( data->m_randomBurstTime > 0.0f && data->m_randomBurstTime < data->m_lastBurstTime && data->m_bursted )
						{
							SpawnParticles( data, simulationContext, burst.m_spawnCount, data->m_cycleTime, burst.m_spawnTimeRange );
							data->m_lastBurstTime = 0.0f;
							data->m_bursted = false;
						}
					}
				}
			}
		}
		else if ( emitterLod.m_birthRate && performSimulation && ( data->m_randomDelayTime <= data->m_delayTimer ) )
		{	
			// Count how many particles to spawn
			const Float normalizedEmitterTime = ::Clamp< Float >( data->m_cycleTime / GetEmitterDuration( lod ), 0.f, 1.f );
			const Float birthRate = LightEval::EvaluateScalar( emitterLod.m_birthRate, normalizedEmitterTime ) * simulationContext.m_globalEmissionScale;
			if ( birthRate > 0.0f )
			{

					// This might change when doing this with delay.
					// Update spawn counter
					Float particlesToSpawnF = birthRate * simulationContext.m_timeDelta;

					if ( (m_vertexDrawerType == PVDT_Trail || m_vertexDrawerType == PVDT_FacingTrail) && simulationContext.m_interpolator && m_drawerData.trail.m_minSegmentsPer360Degrees > 0 )
					{
						const Float normalizedAngleChange = simulationContext.m_interpolator->GetAngleChange() * ( 0.5f / M_PI );
						particlesToSpawnF = Max( particlesToSpawnF, m_drawerData.trail.m_minSegmentsPer360Degrees * normalizedAngleChange );
					}

					data->m_spawnCounter += particlesToSpawnF;
					// Count how many (whole) particles we should spawn this frame
					const Uint32 particlesToSpawn = (Int32)data->m_spawnCounter;
					
					if ( particlesToSpawn )
					{
						data->m_spawnCounter -= ( Float )particlesToSpawn;
						// Spawn particles
						SpawnParticles( data, simulationContext, particlesToSpawn, prevTime, simulationContext.m_timeDelta );
					}	
				data->m_lastBurstTime += simulationContext.m_timeDelta;
			}
		}
	}

	// Get the particle buffer we are working on
	CParticleBuffer< PARTICLE_TYPE >& buffer = data->GetBuffer();
	Bool canGenerateVertexData = true;

	// Simulate particles, only if there is anything to simulate :)
	if ( buffer.GetNumParticles() > 0 )
	{
		// Setup modification context
		SParticleUpdateContext updateContext;
		updateContext.m_componentTransform	= simulationContext.m_componentTransform;
		updateContext.m_emitterTime			= performSimulation ? ::Clamp< Float >( prevTime / GetEmitterDuration( lod ), 0.f, 1.f ) : 0.0f;
		updateContext.m_effectAlpha			= simulationContext.m_effectAlpha;
		updateContext.m_effectSize			= simulationContext.m_effectSize;
		updateContext.m_timeDelta			= performSimulation ? simulationContext.m_timeDelta : 0.0f;
		updateContext.m_targetTranslation	= simulationContext.m_targetTranslation;
		updateContext.m_cameraPosition		= simulationContext.m_cameraPosition;
		updateContext.m_windVector			= m_useOnlyWindInfluence ? simulationContext.m_windVectorOnly * m_windInfluence : simulationContext.m_windVector * m_windInfluence;
		updateContext.m_wrapper				= simulationContext.m_wrapper;
		updateContext.m_lod					= lod;
		updateContext.m_localSimulation		= m_keepSimulationLocal;

		switch ( m_vertexDrawerType )
		{
		case PVDT_Billboard:
		case PVDT_SphereAligned:
		case PVDT_EmitterOrientation:
		case PVDT_VerticalFixed:
			canGenerateVertexData = UpdateParticles< PARTICLE_TYPE, ParticleSimpleVertex >( buffer, updateContext, onScreen );
			break;
		case PVDT_Screen:
			canGenerateVertexData = UpdateParticles< PARTICLE_TYPE, ParticleSimpleVertex >( buffer, updateContext, onScreen );
			CParticleVertexBuffer::BoundingBox() = Box( Vector(-1000,-1000,-1000), Vector(1000,1000,1000) );
			break;
		case PVDT_MotionBlurred:
			canGenerateVertexData = UpdateParticles< PARTICLE_TYPE, ParticleMotionVertex >( buffer, updateContext, onScreen );
			break;
		case PVDT_Rain:
			canGenerateVertexData = UpdateParticles< PARTICLE_TYPE, ParticleMotionVertex >( buffer, updateContext, onScreen );
			CParticleVertexBuffer::BoundingBox() = Box( Vector(-10000,-10000,-10000), Vector(10000,10000,10000) );
			break;
		case PVDT_Trail:
			canGenerateVertexData = UpdateParticles< PARTICLE_TYPE, ParticleTrailVertex >( buffer, updateContext, onScreen );
			break;
		case PVDT_FacingTrail:
			canGenerateVertexData = UpdateParticles< PARTICLE_TYPE, ParticleFacingTrail_Beam_Vertex >( buffer, updateContext, onScreen );
			break;
		case PVDT_Beam:
			canGenerateVertexData = UpdateParticles< PARTICLE_TYPE, ParticleFacingTrail_Beam_Vertex >( buffer, updateContext, onScreen );
			break;
		case PVDT_Mesh:
			canGenerateVertexData = UpdateParticles< PARTICLE_TYPE, void >( buffer, updateContext, onScreen );
			break;
		default: ASSERT( 0 );
		}
	}

	const Uint32 numParticles = buffer.GetNumParticles();

	if( simulationContext.m_wrapper && m_updaterData.m_collisionEmitterIndex >= 0 )
	{
		simulationContext.m_wrapper->FlushInputOutput( m_updaterData.m_collisionEmitterIndex, numParticles );
	}

	if( onScreen && canGenerateVertexData )
	{
		data->m_numVertices = CParticleVertexBuffer::GetBindInfo().m_numVertices;
		data->m_byteOffset = CParticleVertexBuffer::GetBindInfo().m_byteOffset;
	}
	else
	{
		data->m_numVertices = 0;
		data->m_byteOffset = 0;
	}

	// Keep alive until there's work to do
	const Bool emitterHasNotFinished = canSpawnParticles || ( numParticles > 0 );

	// Return status flag
	return emitterHasNotFinished;
}

template< typename PARTICLE_TYPE >
void CRenderParticleEmitter::PerformActions( char* particle , Uint32 actionsPerformed, const SParticleUpdateContext& updateContext ) const
{
	if( actionsPerformed == 0 ) return;

	if( actionsPerformed & PARTICLE_KILLED )
	{
		Float* life = ( Float* )( particle + PARTICLE_LIFE );
		Float* lifeSpanInv = ( Float* )( particle + PARTICLE_LIFE_SPAN_INV );
		*life = 1.0f / *lifeSpanInv;
	}
	
	CRenderDecalSpawner * spawner = nullptr;

	if( actionsPerformed & PARTICLE_DEATH )
	{
		spawner = m_decalSpawner;
	}
	else if( actionsPerformed & PARTICLE_MOVED )
	{
		spawner = m_motionDecalSpawner;
	}
	else if( actionsPerformed & PARTICLE_COLLIDED )
	{
		spawner = m_collisionDecalSpawner;
	}

	if( !spawner )
		return;

	if ( PARTICLE_TYPE::m_fieldSet & PFS_Velocity )
	{
		Vector3* position = ( Vector3* )( particle + PARTICLE_POSITION );
		Vector3* velocity = ( Vector3* )( particle + PARTICLE_VELOCITY );

		if( spawner->AllowSpawnParticle() )
		{
			spawner->SpawnDecalForParticle( position, velocity, &updateContext.m_cameraPosition );

			spawner->MarkSpawnTime();
		}

		
	}

}

template< typename PARTICLE_TYPE, typename PARTICLE_VERTEX_TYPE >
Bool CRenderParticleEmitter::UpdateParticles( CParticleBuffer< PARTICLE_TYPE >& buffer, const SParticleUpdateContext& updateContext, Bool onScreen ) const
{
	Int32 numParticles = buffer.GetNumParticles();
	Vector tempParticleSize; // helper value, used for bounding box update

	if( !CParticleVertexBuffer::ValidateAvailableSize<PARTICLE_VERTEX_TYPE>( numParticles ) )
	{
		RED_LOG( RED_LOG_CHANNEL( Particles ), TXT("There's not enough GPU memory to draw particles.") );
		return false;
	}

	// Map vertex buffer
	CParticleVertexBuffer::ResetBoundingBox();

	RED_ASSERT( updateContext.m_lod < m_lods.Size() );
	if ( m_lods[updateContext.m_lod].m_sortBackToFront )
	{
		buffer.SortBackToFront( updateContext.m_cameraPosition );
	}

	for ( Int32 i=0; i<numParticles; ++i )
	{
		char* particle = ( char* ) buffer.GetParticleDataAt( i );
		Uint32 actionsPerformed = 0;

		//////////////////////////////////////////////////////////////////////////
		// Decide on particle survival :)
		Float* lifeTime = ( Float* ) ( particle + PARTICLE_LIFE );
		Float* lifeSpanInv = ( Float* ) ( particle + PARTICLE_LIFE_SPAN_INV );
		ASSERT( *(Uint32*)lifeSpanInv );

		// Kill the particle if it's time come to pass
		Float lifeFrac = ( *lifeTime ) * ( *lifeSpanInv );
		if ( lifeFrac >= 1.0f )
		{
			PerformActions<PARTICLE_TYPE>( particle , PARTICLE_DEATH , updateContext );

			// Kill
			*lifeTime = 1.0f / ( *lifeSpanInv );
			*lifeSpanInv = 0.0f;

			Int32 previousIndex = buffer.GetNumParticles() - 1;
			buffer.Dealloc( i );

			if( updateContext.m_wrapper && m_updaterData.m_collisionEmitterIndex >= 0 )
			{
				Vector3 currentPos, velocity;
				if( previousIndex != i )
				{
					if( updateContext.m_wrapper->GetParticleOutput( m_updaterData.m_collisionEmitterIndex, previousIndex, &currentPos, &velocity ) )
					{
						updateContext.m_wrapper->RefreshParticle( m_updaterData.m_collisionEmitterIndex, i, currentPos, velocity );
					}
				}
			}

			i--;
			numParticles--;

			continue;
		}
		//////////////////////////////////////////////////////////////////////////

		Float nullAlpha = 1.0f;
		Float* alphaPtr = &nullAlpha;
		//////////////////////////////////////////////////////////////////////////
		// Particle is still alive, copy initial stuff
		if ( PARTICLE_TYPE::m_fieldSet & PFS_Color )
		{
			Vector3* color = ( Vector3* )( particle + PARTICLE_COLOR );
			Vector3* baseColor = ( Vector3* )( particle + PARTICLE_BASE_COLOR );
			*color = *baseColor;

			Float* alpha = ( Float* )( particle + PARTICLE_ALPHA );
			Float* baseAlpha = ( Float* )( particle + PARTICLE_BASE_ALPHA );
			*alpha = *baseAlpha;
			alphaPtr = alpha;
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Rotation2D )
		{
			Float* rotRate = ( Float* )( particle + PARTICLE_ROTATION_RATE );
			Float* baseRotRate = ( Float* )( particle + PARTICLE_BASE_ROTATION_RATE );
			*rotRate = *baseRotRate;
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Rotation3D )
		{
			Vector3* rotRate = ( Vector3* )( particle + PARTICLE_ROTATION_RATE3D );
			Vector3* baseRotRate = ( Vector3* )( particle + PARTICLE_BASE_ROTATION_RATE3D );
			*rotRate = *baseRotRate;
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Size2D )
		{
			Vector2* size = ( Vector2* )( particle + PARTICLE_SIZE );
			Vector2* baseSize = ( Vector2* )( particle + PARTICLE_BASE_SIZE );
			*size = *baseSize;
			*size *= updateContext.m_effectSize;
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Size3D )
		{
			Vector3* size = ( Vector3* )( particle + PARTICLE_SIZE3D );
			Vector3* baseSize = ( Vector3* )( particle + PARTICLE_BASE_SIZE3D );
			*size = *baseSize;
			*size *= updateContext.m_effectSize;
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Velocity )
		{
			Vector3* velocity = ( Vector3* )( particle + PARTICLE_VELOCITY );
			Vector3* baseVelocity = ( Vector3* )( particle + PARTICLE_BASE_VELOCITY );
			*velocity = *baseVelocity + updateContext.m_windVector;
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Frame )
		{
			Float* frame = ( Float* )( particle + PARTICLE_FRAME );
			Float* baseFrame = ( Float* )( particle + PARTICLE_BASE_FRAME );
			*frame = *baseFrame;
		}
		//////////////////////////////////////////////////////////////////////////
		// Apply modifiers
		const Float	particleNormalizedLife = ( *lifeTime ) * ( *lifeSpanInv );
		for ( Uint32 j=0; j<m_numModifiers; ++j )
		{
			(*(m_modifierFunctions[j]))( particle, m_updaterData, updateContext, particleNormalizedLife, i , actionsPerformed );
		}

		//////////////////////////////////////////////////////////////////////////
		// Update tickables
		if ( PARTICLE_TYPE::m_fieldSet & PFS_Velocity )
		{
			// Update position using velocity
			Vector3* velocity = ( Vector3* )( particle + PARTICLE_VELOCITY );
			Vector3* position = ( Vector3* )( particle + PARTICLE_POSITION );
			*position += *velocity * updateContext.m_timeDelta;
			actionsPerformed |= PARTICLE_MOVED;
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Rotation2D )
		{
			Float* rotation = ( Float* )( particle + PARTICLE_ROTATION );
			Float* rotationRate = ( Float* )( particle + PARTICLE_ROTATION_RATE );
			*rotation += *rotationRate * updateContext.m_timeDelta;
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Rotation3D )
		{
			Vector3* rotation = ( Vector3* )( particle + PARTICLE_ROTATION3D );
			Vector3* rotationRate = ( Vector3* )( particle + PARTICLE_ROTATION_RATE3D );
			*rotation += *rotationRate * updateContext.m_timeDelta;
		}

		//////////////////////////////////////////////////////////////////////////
		// Generate vertex data for particle
		// If Alpha is too low we will not generate anything to render
		if ( onScreen && *alphaPtr > 0.01f )
		{
			GenerateVertexData< PARTICLE_TYPE, PARTICLE_VERTEX_TYPE >( ( PARTICLE_TYPE* ) particle );
		}
		// Update particle life
		*lifeTime += updateContext.m_timeDelta;

		//////////////////////////////////////////////////////////////////////////
		// Throw it into bounding box
		Vector3* position = ( Vector3* )( particle + PARTICLE_POSITION ); // <- position->A will be read as the four component array, but it is safe due to particle structure
		if ( PARTICLE_TYPE::m_fieldSet & PFS_Size2D )
		{
			Vector2* size = ( Vector2* )( particle + PARTICLE_SIZE );
			Float sizeSpill = Red::Math::NumericalUtils::Max( size->X, size->Y );
			tempParticleSize.Set3( sizeSpill, sizeSpill, sizeSpill );
		}
		else if ( PARTICLE_TYPE::m_fieldSet & PFS_Size3D )
		{
			Vector3* size = ( Vector3* )( particle + PARTICLE_SIZE3D );
			tempParticleSize.Set3( size->A );
		}

		const Vector boxMin( position->X - tempParticleSize.X, position->Y - tempParticleSize.Y, position->Z - tempParticleSize.Z );
		const Vector boxMax( position->X + tempParticleSize.X, position->Y + tempParticleSize.Y, position->Z + tempParticleSize.Z );
		CParticleVertexBuffer::BoundingBox().AddPoint( boxMin );
		CParticleVertexBuffer::BoundingBox().AddPoint( boxMax );

		PerformActions<PARTICLE_TYPE>( particle , actionsPerformed , updateContext );
	}

	// lets be sure... culling assumes correct bounding boxes
	CParticleVertexBuffer::BoundingBox().Min.W = 1.0f;
	CParticleVertexBuffer::BoundingBox().Max.W = 1.0f;
	////// ----

	// Unmap vertex buffer
	if( onScreen )
	{
		CParticleVertexBuffer::MoveOffset< PARTICLE_VERTEX_TYPE >();
	}

	return true;
}

template<>
Bool CRenderParticleEmitter::UpdateParticles< TrailParticle, ParticleTrailVertex >( CParticleBuffer< TrailParticle >& buffer, const SParticleUpdateContext& updateContext, Bool onScreen ) const
{
	Float lastTexCoord = 0.0f;
	Float startTexCoord = 0.0f;
	Uint8 lastAlpha = 0;
	Float fullTrailLength = 0.0f;
	Vector3 lastPos, lastEmitterAxis;

	Int32 numParticles = buffer.GetNumParticles();

	if( !CParticleVertexBuffer::ValidateAvailableSize<ParticleTrailVertex>( numParticles ) )
	{
		RED_LOG( RED_LOG_CHANNEL( Particles ), TXT("There's not enough GPU memory to draw particles.") );
		return false;
	}

	// Initiate pointer to the first particle (oldest trail segment)
	TrailParticle* firstParticle = buffer.GetParticleAt( 0 );

	if ( firstParticle )
	{
		lastPos = firstParticle->m_position;
		lastEmitterAxis = firstParticle->m_emitterAxis;
		startTexCoord = firstParticle->m_prevTexCoord;
	}

	// Map vertex buffer
	CParticleVertexBuffer::ResetBoundingBox();

	// Trails need two iterations due to the tex coord computation
	for ( Int32 i=0; i<numParticles; ++i )
	{
		TrailParticle* particle = buffer.GetParticleAt( i );
		ASSERT( *(Uint32*)&particle->m_lifeSpanInv );

		Uint32 actionsPerformed = 0;

		// Kill the particle if it's time come to pass
		Float lifeFrac = particle->m_life * particle->m_lifeSpanInv;
		if ( lifeFrac >= 1.0f && i == 0 )
		{
			PerformActions<TrailParticle>( (char*)particle , PARTICLE_DEATH , updateContext );
			// Kill
			particle->m_life = 1.0f / particle->m_lifeSpanInv;
			particle->m_lifeSpanInv = 0.0f;

			// Update starting tex coord
			startTexCoord = particle->m_texCoord;

			Int32 previousIndex = buffer.GetNumParticles() - 1;
			buffer.Dealloc( i );

			if( updateContext.m_wrapper && m_updaterData.m_collisionEmitterIndex >= 0 )
			{
				Vector3 currentPos, velocity;
				if( previousIndex != i )
				{
					if( updateContext.m_wrapper->GetParticleOutput( m_updaterData.m_collisionEmitterIndex, previousIndex, &currentPos, &velocity ) )
					{
						updateContext.m_wrapper->RefreshParticle( m_updaterData.m_collisionEmitterIndex, i, currentPos, velocity );
					}
				}
			}

			--i;
			--numParticles;

			// Update first particle pointer. We assume that trail particles die in the FIFO way
			firstParticle = buffer.GetParticleAt( 0 );
			if ( firstParticle )
			{
				lastPos = firstParticle->m_position;
				lastEmitterAxis = firstParticle->m_emitterAxis;
			}

			continue;
		}

		const Float	particleNormalizedLife = particle->m_life * particle->m_lifeSpanInv;

		// Copy initial stuff
		particle->m_color = particle->m_baseColor;
		particle->m_alpha = particle->m_baseAlpha;
		particle->m_size = particle->m_baseSize;
		particle->m_size *= updateContext.m_effectSize;
		particle->m_frame = particle->m_baseFrame;
		particle->m_lastEmitterAxis = lastEmitterAxis;

		// What kind of action were performed via modifiers on particle
		Uint32 actionPerformed = 0;

		// Apply modifiers
		for ( Uint32 j=0; j<m_numModifiers; ++j )
		{
			(*(m_modifierFunctions[j]))( (char*)particle, m_updaterData, updateContext, particleNormalizedLife, i, actionPerformed );
		}

		// Update particle life
		particle->m_life += updateContext.m_timeDelta;

		// Increase full trail length for further tex coords processing
		fullTrailLength += (particle->m_position - lastPos).Mag();

		lastPos = particle->m_position;
		lastEmitterAxis = particle->m_emitterAxis;

		PerformActions<TrailParticle>( (char*)particle , actionsPerformed , updateContext );
	}

	if ( numParticles > 0 )
	{
		lastTexCoord = startTexCoord;

		Float texPerUnit = m_drawerData.trail.m_texturesPerUnit;
		if ( m_drawerData.trail.m_dynamicTexCoords )
		{
			if ( fullTrailLength>0.0f )
			{
				texPerUnit *= 1.0f / fullTrailLength;
			}
			else
			{
				texPerUnit *= 0.0f;
			}
			lastTexCoord = 0.0f;
		}

		// Generate vertex data along with tex coords update
		firstParticle = buffer.GetParticleAt( 0 );
		lastPos = firstParticle->m_position;
		lastAlpha = (Uint8) ::Clamp< Float >( firstParticle->m_alpha * 255.0f, 0.0f, 255.0f );
		for ( Int32 i=0; i<numParticles; ++i )
		{
			TrailParticle* particle = buffer.GetParticleAt( i );

			particle->m_prevTexCoord = lastTexCoord;
			Float step = 1.0f;
			if ( m_drawerData.trail.m_dynamicTexCoords )
			{
				step  = ( particle->m_position - lastPos ).Mag();
			}
			particle->m_texCoord = lastTexCoord + step * texPerUnit;
			lastTexCoord = particle->m_texCoord;
			lastPos = particle->m_position;

			Uint32 frontEdgeColor;
			Uint32 rearEdgeColor;

			Uint8 red = (Uint8) ::Clamp< Float >( particle->m_color.X * 255.0f, 0.0f, 255.0f );
			Uint8 green = (Uint8) ::Clamp< Float >( particle->m_color.Y * 255.0f, 0.0f, 255.0f );
			Uint8 blue = (Uint8) ::Clamp< Float >( particle->m_color.Z * 255.0f, 0.0f, 255.0f );
			Uint8 alpha = (Uint8) ::Clamp< Float >( particle->m_alpha * 255.0f, 0.0f, 255.0f );
			frontEdgeColor = (alpha<<24)|(red<<16)|(green<<8)|blue;
			lastAlpha = alpha;
			rearEdgeColor = frontEdgeColor & 0x00FFFFFF;
			rearEdgeColor = rearEdgeColor | ( lastAlpha<<24 );

			// Compute each vertex position
			// 0 -- 1
			// | \  |  
			// |  \ |
			// 3 -- 2

			const Float stretch = particle->m_size.X / 2.0f;

			const Vector vertex0Pos = particle->m_position + particle->m_emitterAxis * stretch;
			const Vector vertex1Pos = firstParticle->m_position + particle->m_lastEmitterAxis * stretch;
			const Vector vertex2Pos = firstParticle->m_position - particle->m_lastEmitterAxis * stretch;
			const Vector vertex3Pos = particle->m_position - particle->m_emitterAxis * stretch;
			const Float onef = 1.0f;
			const Float zerof = 0.0f;

			if( onScreen )
			{
				ParticleTrailVertex* vertex = NULL;
				// Vertex 0
				CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleTrailVertex >( vertex );
				COPY_FIELD_AS_INT32( vertex->m_position[0], vertex0Pos.A[0] );
				COPY_FIELD_AS_INT32( vertex->m_position[1], vertex0Pos.A[1] );
				COPY_FIELD_AS_INT32( vertex->m_position[2], vertex0Pos.A[2] );
				COPY_FIELD_AS_INT32( vertex->m_color, frontEdgeColor );
				COPY_FIELD_AS_INT32( vertex->m_uv[0], particle->m_texCoord );
				COPY_FIELD_AS_INT32( vertex->m_uv[1], zerof );
				COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );

				// Vertex 1
				CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleTrailVertex >( vertex );
				COPY_FIELD_AS_INT32( vertex->m_position[0], vertex1Pos.A[0] );
				COPY_FIELD_AS_INT32( vertex->m_position[1], vertex1Pos.A[1] );
				COPY_FIELD_AS_INT32( vertex->m_position[2], vertex1Pos.A[2] );
				COPY_FIELD_AS_INT32( vertex->m_color, rearEdgeColor );
				COPY_FIELD_AS_INT32( vertex->m_uv[0], particle->m_prevTexCoord );
				COPY_FIELD_AS_INT32( vertex->m_uv[1], zerof );
				COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );

				// Vertex 2
				CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleTrailVertex >( vertex );
				COPY_FIELD_AS_INT32( vertex->m_position[0], vertex2Pos.A[0] );
				COPY_FIELD_AS_INT32( vertex->m_position[1], vertex2Pos.A[1] );
				COPY_FIELD_AS_INT32( vertex->m_position[2], vertex2Pos.A[2] );
				COPY_FIELD_AS_INT32( vertex->m_color, rearEdgeColor );
				COPY_FIELD_AS_INT32( vertex->m_uv[0], particle->m_prevTexCoord );
				COPY_FIELD_AS_INT32( vertex->m_uv[1], onef );
				COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );

				// Vertex 3
				CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleTrailVertex >( vertex );
				COPY_FIELD_AS_INT32( vertex->m_position[0], vertex3Pos.A[0] );
				COPY_FIELD_AS_INT32( vertex->m_position[1], vertex3Pos.A[1] );
				COPY_FIELD_AS_INT32( vertex->m_position[2], vertex3Pos.A[2] );
				COPY_FIELD_AS_INT32( vertex->m_color, frontEdgeColor );
				COPY_FIELD_AS_INT32( vertex->m_uv[0], particle->m_texCoord );
				COPY_FIELD_AS_INT32( vertex->m_uv[1], onef );
				COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );
			}

			const Vector boxMin( particle->m_position.X - particle->m_size.X, particle->m_position.Y - particle->m_size.Y, particle->m_position.Z - particle->m_size.Y );
			const Vector boxMax( particle->m_position.X + particle->m_size.X, particle->m_position.Y + particle->m_size.Y, particle->m_position.Z + particle->m_size.Y );
			CParticleVertexBuffer::BoundingBox().AddPoint( boxMin );
			CParticleVertexBuffer::BoundingBox().AddPoint( boxMax );
			firstParticle = particle;
		}

		// lets be sure... culling assumes correct bounding boxes
		CParticleVertexBuffer::BoundingBox().Min.W = 1.0f;
		CParticleVertexBuffer::BoundingBox().Max.W = 1.0f;
		////// ----
	}

	// Unmap vertex buffer
	if( onScreen )
	{
		CParticleVertexBuffer::MoveOffset< ParticleTrailVertex >();
	}

	return true;
}

template<>
Bool CRenderParticleEmitter::UpdateParticles< FacingTrailParticle, ParticleFacingTrail_Beam_Vertex >( CParticleBuffer< FacingTrailParticle >& buffer, const SParticleUpdateContext& updateContext, Bool onScreen ) const
{
	Float lastTexCoord = 0.0f;
	Float startTexCoord = 0.0f;
	Uint8 lastAlpha = 0;
	Float fullTrailLength = 0.0f;
	Vector3 lastPos;

	Int32 numParticles = buffer.GetNumParticles();

	if( !CParticleVertexBuffer::ValidateAvailableSize<ParticleFacingTrail_Beam_Vertex>( numParticles ) )
	{
		RED_LOG( RED_LOG_CHANNEL( Particles ), TXT("There's not enough GPU memory to draw particles.") );
		return false;
	}

	// Get oldest trail segment
	FacingTrailParticle* firstParticle = buffer.GetParticleAt( 0 );
	if ( firstParticle )
	{
		// Initiate last position value with the first segment position
		lastPos = firstParticle->m_position;
		startTexCoord = firstParticle->m_prevTexCoord;
	}

	// Map vertex buffer
	CParticleVertexBuffer::ResetBoundingBox();

	// Trails need two iterations due to the tex coord computation
	for ( Int32 i=0; i<numParticles; ++i )
	{
		FacingTrailParticle* particle = buffer.GetParticleAt( i );
		ASSERT( *(Uint32*)&particle->m_lifeSpanInv );

		Uint32 actionsPerformed = 0;

		// Kill the particle if it's time come to pass
		Float lifeFrac = particle->m_life * particle->m_lifeSpanInv;
		if ( lifeFrac >= 1.0f && i == 0 ) // In trails, we only kill first particle
		{
			PerformActions<FacingTrailParticle>( (char*)particle , PARTICLE_DEATH , updateContext );

			// Kill
			particle->m_life = 1.0f / particle->m_lifeSpanInv;
			particle->m_lifeSpanInv = 0.0f;
			startTexCoord = particle->m_texCoord;

			Int32 previousIndex = buffer.GetNumParticles() - 1;
			buffer.Dealloc( i );

			if( updateContext.m_wrapper && m_updaterData.m_collisionEmitterIndex >= 0 )
			{
				Vector3 currentPos, velocity;
				if( previousIndex != i )
				{
					if( updateContext.m_wrapper->GetParticleOutput( m_updaterData.m_collisionEmitterIndex, previousIndex, &currentPos, &velocity ) )
					{
						updateContext.m_wrapper->RefreshParticle( m_updaterData.m_collisionEmitterIndex, i, currentPos, velocity );
					}
				}
			}

			--i;
			--numParticles;

			// The bottom assumes that trail particles die in the FIFO way
			firstParticle = buffer.GetParticleAt( 0 );
			if ( firstParticle )
			{
				lastPos = firstParticle->m_position;
			}

			continue;
		}

		const Float	particleNormalizedLife = particle->m_life * particle->m_lifeSpanInv;

		// Copy initial stuff
		particle->m_velocity = particle->m_baseVelocity + updateContext.m_windVector;
		particle->m_color = particle->m_baseColor;
		particle->m_alpha = particle->m_baseAlpha;
		particle->m_size = particle->m_baseSize;
		particle->m_size *= updateContext.m_effectSize;
		particle->m_frame = particle->m_baseFrame;

		// What kind of action were performed via modifiers on particle
		Uint32 actionPerformed = 0;

		// Apply modifiers
		for ( Uint32 j=0; j<m_numModifiers; ++j )
		{
			(*(m_modifierFunctions[j]))( (char*)particle, m_updaterData, updateContext, particleNormalizedLife, i, actionPerformed );
		}

		// Update particle life
		particle->m_life += updateContext.m_timeDelta;

		// Apply velocity
		particle->m_position += particle->m_velocity * updateContext.m_timeDelta;

		// Prepare full trail length value for the next iteration below
		fullTrailLength += (particle->m_position - lastPos).Mag();

		lastPos = particle->m_position;

		PerformActions<FacingTrailParticle>( (char*)particle , actionsPerformed , updateContext );
	}

	if ( numParticles > 0 )
	{
		// Start from the oldest particle tex coord
		lastTexCoord = startTexCoord;

		firstParticle = buffer.GetParticleAt( 0 );

		Vector3 n1Pos = firstParticle->m_position;	//  n-1 particle position
		Vector3 n2Pos = firstParticle->m_position;	//  n-2 particle position
		lastAlpha = (Uint8) ::Clamp< Float >( firstParticle->m_alpha * 255.0f, 0.0f, 255.0f );

		// Compute texture per unit value
		Float texPerUnit = m_drawerData.trail.m_texturesPerUnit;
		if ( m_drawerData.trail.m_dynamicTexCoords )
		{
			// We don't want distance based tiling 
			if ( fullTrailLength>0.0f )
			{
				texPerUnit *= 1.0f / fullTrailLength;
			}
			else
			{
				texPerUnit *= 0.0f;
			}
			lastTexCoord = 0.0f;
		}

		for ( Int32 i=0; i<numParticles; ++i )
		{
			FacingTrailParticle* particle = buffer.GetParticleAt( i );

			particle->m_prevTexCoord = lastTexCoord;
			Float step = 1.0f;
			if ( m_drawerData.trail.m_dynamicTexCoords )
			{
				step  = ( particle->m_position - n1Pos ).Mag();
			}
			particle->m_texCoord = Red::Math::MFract( lastTexCoord + step * texPerUnit );
			lastTexCoord = particle->m_texCoord;

			Uint32 frontEdgeColor;
			Uint32 rearEdgeColor;

			Uint8 red = (Uint8) ::Clamp< Float >( particle->m_color.X * 255.0f, 0.0f, 255.0f );
			Uint8 green = (Uint8) ::Clamp< Float >( particle->m_color.Y * 255.0f, 0.0f, 255.0f );
			Uint8 blue = (Uint8) ::Clamp< Float >( particle->m_color.Z * 255.0f, 0.0f, 255.0f );
			Uint8 alpha = (Uint8) ::Clamp< Float >( particle->m_alpha * 255.0f, 0.0f, 255.0f );
			frontEdgeColor = (alpha<<24)|(red<<16)|(green<<8)|blue;
			lastAlpha = alpha;
			rearEdgeColor = frontEdgeColor & 0x00FFFFFF;
			rearEdgeColor = rearEdgeColor | ( lastAlpha<<24 );

			// Compute each vertex position
			// 0 -- 1
			// | \  |  
			// |  \ |
			// 3 -- 2

			const Float stretch = particle->m_size.X / 2.0f;
			const Float stretchNeg = -stretch;
			const Float zerof = 0.0f;
			const Float onef = 1.0f;

			if( onScreen )
			{
				ParticleFacingTrail_Beam_Vertex* vertex = NULL;
				// Vertex 0
				CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleFacingTrail_Beam_Vertex >( vertex );
				COPY_FIELD_AS_INT32( vertex->m_position[0], particle->m_position.A[0] );
				COPY_FIELD_AS_INT32( vertex->m_position[1], particle->m_position.A[1] );
				COPY_FIELD_AS_INT32( vertex->m_position[2], particle->m_position.A[2] );
				COPY_FIELD_AS_INT32( vertex->m_prevPosition[0], n1Pos.A[0] );
				COPY_FIELD_AS_INT32( vertex->m_prevPosition[1], n1Pos.A[1] );
				COPY_FIELD_AS_INT32( vertex->m_prevPosition[2], n1Pos.A[2] );
				COPY_FIELD_AS_INT32( vertex->m_stretch, stretch );
				COPY_FIELD_AS_INT32( vertex->m_color, frontEdgeColor );
				COPY_FIELD_AS_INT32( vertex->m_uv[0], particle->m_texCoord );
				COPY_FIELD_AS_INT32( vertex->m_uv[1], zerof );
				COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );

				// Vertex 1
				CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleFacingTrail_Beam_Vertex >( vertex );
				COPY_FIELD_AS_INT32( vertex->m_position[0], n1Pos.A[0] );
				COPY_FIELD_AS_INT32( vertex->m_position[1], n1Pos.A[1] );
				COPY_FIELD_AS_INT32( vertex->m_position[2], n1Pos.A[2] );
				COPY_FIELD_AS_INT32( vertex->m_prevPosition[0], n2Pos.A[0] );
				COPY_FIELD_AS_INT32( vertex->m_prevPosition[1], n2Pos.A[1] );
				COPY_FIELD_AS_INT32( vertex->m_prevPosition[2], n2Pos.A[2] );
				COPY_FIELD_AS_INT32( vertex->m_stretch, stretch );
				COPY_FIELD_AS_INT32( vertex->m_color, rearEdgeColor );
				COPY_FIELD_AS_INT32( vertex->m_uv[0], particle->m_prevTexCoord );
				COPY_FIELD_AS_INT32( vertex->m_uv[1], zerof );
				COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );

				// Vertex 2
				CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleFacingTrail_Beam_Vertex >( vertex );
				COPY_FIELD_AS_INT32( vertex->m_position[0], n1Pos.A[0] );
				COPY_FIELD_AS_INT32( vertex->m_position[1], n1Pos.A[1] );
				COPY_FIELD_AS_INT32( vertex->m_position[2], n1Pos.A[2] );
				COPY_FIELD_AS_INT32( vertex->m_prevPosition[0], n2Pos.A[0] );
				COPY_FIELD_AS_INT32( vertex->m_prevPosition[1], n2Pos.A[1] );
				COPY_FIELD_AS_INT32( vertex->m_prevPosition[2], n2Pos.A[2] );
				COPY_FIELD_AS_INT32( vertex->m_stretch, stretchNeg );
				COPY_FIELD_AS_INT32( vertex->m_color, rearEdgeColor );
				COPY_FIELD_AS_INT32( vertex->m_uv[0], particle->m_prevTexCoord );
				COPY_FIELD_AS_INT32( vertex->m_uv[1], onef );
				COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );

				// Vertex 3
				CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleFacingTrail_Beam_Vertex >( vertex );
				COPY_FIELD_AS_INT32( vertex->m_position[0], particle->m_position.A[0] );
				COPY_FIELD_AS_INT32( vertex->m_position[1], particle->m_position.A[1] );
				COPY_FIELD_AS_INT32( vertex->m_position[2], particle->m_position.A[2] );
				COPY_FIELD_AS_INT32( vertex->m_prevPosition[0], n1Pos.A[0] );
				COPY_FIELD_AS_INT32( vertex->m_prevPosition[1], n1Pos.A[1] );
				COPY_FIELD_AS_INT32( vertex->m_prevPosition[2], n1Pos.A[2] );
				COPY_FIELD_AS_INT32( vertex->m_stretch, stretchNeg );
				COPY_FIELD_AS_INT32( vertex->m_color, frontEdgeColor );
				COPY_FIELD_AS_INT32( vertex->m_uv[0], particle->m_texCoord );
				COPY_FIELD_AS_INT32( vertex->m_uv[1], onef );
				COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );
			}

			const Vector boxMin( particle->m_position.X - particle->m_size.X, particle->m_position.Y - particle->m_size.Y, particle->m_position.Z - particle->m_size.Y );
			const Vector boxMax( particle->m_position.X + particle->m_size.X, particle->m_position.Y + particle->m_size.Y, particle->m_position.Z + particle->m_size.Y );
			CParticleVertexBuffer::BoundingBox().AddPoint( boxMin );
			CParticleVertexBuffer::BoundingBox().AddPoint( boxMax );

			n2Pos = n1Pos;
			n1Pos = particle->m_position;
		}

		// lets be sure... culling assumes correct bounding boxes
		CParticleVertexBuffer::BoundingBox().Min.W = 1.0f;
		CParticleVertexBuffer::BoundingBox().Max.W = 1.0f;
		////// ----
	}

	// Unmap vertex buffer
	if( onScreen )
	{
		CParticleVertexBuffer::MoveOffset< ParticleFacingTrail_Beam_Vertex >();
	}

	return true;
}

template<>
Bool CRenderParticleEmitter::UpdateParticles< BeamParticle, ParticleFacingTrail_Beam_Vertex >( CParticleBuffer< BeamParticle >& buffer, const SParticleUpdateContext& updateContext, Bool onScreen ) const
{
	Uint32 numParticles = buffer.GetNumParticles();
	const Uint32 numSegments = m_drawerData.beam.m_numSegments;
	if ( numSegments < 2 )
	{
		return false;
	}

	if( !CParticleVertexBuffer::ValidateAvailableSize<ParticleFacingTrail_Beam_Vertex>( numParticles ) )
	{
		RED_LOG( RED_LOG_CHANNEL( Particles ), TXT("There's not enough GPU memory to draw particles.") );
		return false;
	}

	Int32 beamsCount = numParticles / numSegments;

	const Vector emitterPos = updateContext.m_componentTransform.GetTranslation();

	// Generate vertex data along with tex coords update
	Vector3 n1Pos; 
	Vector3 n2Pos;
	Float texCoord, lastTexCoord;

	// Map vertex buffer
	CParticleVertexBuffer::ResetBoundingBox();

	for ( Int32 i=0; i<beamsCount; )
	{
		Int32 headParticleIndex = i * numSegments;
		Int32 tailParticleIndex = headParticleIndex + numSegments - 1;
		BeamParticle* headParticle = buffer.GetParticleAt( headParticleIndex );

		Uint32 actionsPerformed = 0;

		Float lifeFrac = headParticle->m_life * headParticle->m_lifeSpanInv;
		if ( lifeFrac >= 1.0f )
		{
			// Kill this beam, it's a pussy
			// Smite all particles in the segment at once, meaning the minion particles life (value) doesn't matter!
			for ( Int32 j = tailParticleIndex; j >= headParticleIndex; --j )
			{
				Int32 previousIndex = buffer.GetNumParticles() - 1;
				buffer.Dealloc( j );

				if( updateContext.m_wrapper && m_updaterData.m_collisionEmitterIndex >= 0 )
				{
					Vector3 currentPos, velocity;
					if( previousIndex != j )
					{
						if( updateContext.m_wrapper->GetParticleOutput( m_updaterData.m_collisionEmitterIndex, previousIndex, &currentPos, &velocity ) )
						{
							updateContext.m_wrapper->RefreshParticle( m_updaterData.m_collisionEmitterIndex, j, currentPos, velocity );
						}
					}
				}

			}

			// Update amount of beams we can draw
			numParticles = buffer.GetNumParticles();
			beamsCount = numParticles / numSegments;
		}
		else
		{
			// This beam is no pussy, draw it
			const Vector targetPos = updateContext.m_targetTranslation + Vector( headParticle->m_beamSpread );
			// Compute distance to target
			const Float targetDistance = targetPos.DistanceTo( emitterPos );
			// Compute segment length
			const Float segmentLength = targetDistance / ( numSegments - 1 );

			// Compute beam direction
			const Vector beamDirection = ( targetPos - emitterPos ).Normalized3();

			// Compute segment offset vector
			const Vector beamSegmentOffset = beamDirection * segmentLength;

			Int32 segmentIndex = 0;
			n1Pos = headParticle->m_position;
			n2Pos = headParticle->m_position;
			Uint8 lastAlpha = (Uint8) ::Clamp< Float >( headParticle->m_alpha * 255.0f, 0.0f, 255.0f );
			texCoord = 0.0f;
			lastTexCoord = 0.0f;

			for( Int32 j=headParticleIndex; j <= tailParticleIndex; ++j )
			{
				// Load particle
				BeamParticle* particle = buffer.GetParticleAt( j );
				ASSERT( *(Uint32*)&particle->m_lifeSpanInv );

				const Float	particleNormalizedLife = particle->m_life * particle->m_lifeSpanInv;

				// Copy initial stuff
				particle->m_velocity = particle->m_baseVelocity;
				particle->m_color = particle->m_baseColor;
				particle->m_alpha = particle->m_baseAlpha;
				particle->m_frame = particle->m_baseFrame;
				particle->m_size = particle->m_baseSize;

				// What kind of action were performed via modifiers on particle
				Uint32 actionPerformed = 0;

				// Apply modifiers
				for ( Uint32 k=0; k<m_numModifiers; ++k )
				{
					(*(m_modifierFunctions[k]))( ( char* )particle, m_updaterData, updateContext, particleNormalizedLife, i, actionPerformed );
				}

				// Update particle life
				particle->m_life += updateContext.m_timeDelta;

				particle->m_position = emitterPos + beamSegmentOffset * (Float)segmentIndex + particle->m_velocity * ( ( j == tailParticleIndex || j == headParticleIndex ) ? 0.0f : 1.0f );
				++segmentIndex;

				Float particleLength = ( particle->m_position - n1Pos ).Mag();
				texCoord = lastTexCoord + particleLength * m_drawerData.beam.m_texturesPerUnit;	

				Uint32 frontEdgeColor;
				Uint32 rearEdgeColor;

				Uint8 red = (Uint8) ::Clamp< Float >( particle->m_color.X * 255.0f, 0.0f, 255.0f );
				Uint8 green = (Uint8) ::Clamp< Float >( particle->m_color.Y * 255.0f, 0.0f, 255.0f );
				Uint8 blue = (Uint8) ::Clamp< Float >( particle->m_color.Z * 255.0f, 0.0f, 255.0f );
				Uint8 alpha = (Uint8) ::Clamp< Float >( particle->m_alpha * 255.0f, 0.0f, 255.0f );
				frontEdgeColor = (alpha<<24)|(red<<16)|(green<<8)|blue;
				lastAlpha = alpha;
				rearEdgeColor = frontEdgeColor & 0x00FFFFFF;
				rearEdgeColor = rearEdgeColor | ( lastAlpha<<24 );

				// Compute each vertex position
				// 0 -- 1
				// | \  |  
				// |  \ |
				// 3 -- 2

				const Float stretch = particle->m_size.X / 2.0f;
				const Float stretchNeg = -stretch;
				const Float zerof = 0.0f;
				const Float onef = 1.0f;

				if( onScreen )
				{
					ParticleFacingTrail_Beam_Vertex* vertex = NULL;
					// Vertex 0
					CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleFacingTrail_Beam_Vertex >( vertex );
					COPY_FIELD_AS_INT32( vertex->m_position[0], particle->m_position.A[0] );
					COPY_FIELD_AS_INT32( vertex->m_position[1], particle->m_position.A[1] );
					COPY_FIELD_AS_INT32( vertex->m_position[2], particle->m_position.A[2] );
					COPY_FIELD_AS_INT32( vertex->m_prevPosition[0], n1Pos.A[0] );
					COPY_FIELD_AS_INT32( vertex->m_prevPosition[1], n1Pos.A[1] );
					COPY_FIELD_AS_INT32( vertex->m_prevPosition[2], n1Pos.A[2] );
					COPY_FIELD_AS_INT32( vertex->m_stretch, stretch );
					COPY_FIELD_AS_INT32( vertex->m_color, frontEdgeColor );
					COPY_FIELD_AS_INT32( vertex->m_uv[0], texCoord );
					COPY_FIELD_AS_INT32( vertex->m_uv[1], zerof );
					COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );

					// Vertex 1
					CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleFacingTrail_Beam_Vertex >( vertex );
					COPY_FIELD_AS_INT32( vertex->m_position[0], n1Pos.A[0] );
					COPY_FIELD_AS_INT32( vertex->m_position[1], n1Pos.A[1] );
					COPY_FIELD_AS_INT32( vertex->m_position[2], n1Pos.A[2] );
					COPY_FIELD_AS_INT32( vertex->m_prevPosition[0], n2Pos.A[0] );
					COPY_FIELD_AS_INT32( vertex->m_prevPosition[1], n2Pos.A[1] );
					COPY_FIELD_AS_INT32( vertex->m_prevPosition[2], n2Pos.A[2] );
					COPY_FIELD_AS_INT32( vertex->m_stretch, stretch );
					COPY_FIELD_AS_INT32( vertex->m_color, rearEdgeColor );
					COPY_FIELD_AS_INT32( vertex->m_uv[0], lastTexCoord );
					COPY_FIELD_AS_INT32( vertex->m_uv[1], zerof );
					COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );

					// Vertex 2
					CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleFacingTrail_Beam_Vertex >( vertex );
					COPY_FIELD_AS_INT32( vertex->m_position[0], n1Pos.A[0] );
					COPY_FIELD_AS_INT32( vertex->m_position[1], n1Pos.A[1] );
					COPY_FIELD_AS_INT32( vertex->m_position[2], n1Pos.A[2] );
					COPY_FIELD_AS_INT32( vertex->m_prevPosition[0], n2Pos.A[0] );
					COPY_FIELD_AS_INT32( vertex->m_prevPosition[1], n2Pos.A[1] );
					COPY_FIELD_AS_INT32( vertex->m_prevPosition[2], n2Pos.A[2] );
					COPY_FIELD_AS_INT32( vertex->m_stretch, stretchNeg );
					COPY_FIELD_AS_INT32( vertex->m_color, rearEdgeColor );
					COPY_FIELD_AS_INT32( vertex->m_uv[0], lastTexCoord );
					COPY_FIELD_AS_INT32( vertex->m_uv[1], onef );
					COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );

					// Vertex 3
					CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleFacingTrail_Beam_Vertex >( vertex );
					COPY_FIELD_AS_INT32( vertex->m_position[0], particle->m_position.A[0] );
					COPY_FIELD_AS_INT32( vertex->m_position[1], particle->m_position.A[1] );
					COPY_FIELD_AS_INT32( vertex->m_position[2], particle->m_position.A[2] );
					COPY_FIELD_AS_INT32( vertex->m_prevPosition[0], n1Pos.A[0] );
					COPY_FIELD_AS_INT32( vertex->m_prevPosition[1], n1Pos.A[1] );
					COPY_FIELD_AS_INT32( vertex->m_prevPosition[2], n1Pos.A[2] );
					COPY_FIELD_AS_INT32( vertex->m_stretch, stretchNeg );
					COPY_FIELD_AS_INT32( vertex->m_color, frontEdgeColor );
					COPY_FIELD_AS_INT32( vertex->m_uv[0], texCoord );
					COPY_FIELD_AS_INT32( vertex->m_uv[1], onef );
					COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );
				}

				lastTexCoord = texCoord;

				const Vector boxMin( particle->m_position.X - stretch, particle->m_position.Y - stretch, particle->m_position.Z - stretch );
				const Vector boxMax( particle->m_position.X + stretch, particle->m_position.Y + stretch, particle->m_position.Z + stretch );
				CParticleVertexBuffer::BoundingBox().AddPoint( boxMin );
				CParticleVertexBuffer::BoundingBox().AddPoint( boxMax );

				n2Pos = n1Pos;
				n1Pos = particle->m_position;

				PerformActions<FacingTrailParticle>( (char*)particle , actionsPerformed , updateContext );
			}

			++i;
		}
	}	

	// lets be sure... culling assumes correct bounding boxes
	CParticleVertexBuffer::BoundingBox().Min.W = 1.0f;
	CParticleVertexBuffer::BoundingBox().Max.W = 1.0f;
	////// ----

	// Unmap vertex buffer
	if( onScreen )
	{
		CParticleVertexBuffer::MoveOffset< ParticleFacingTrail_Beam_Vertex >();
	}

	return true;
}

void CRenderParticleEmitter::CompileInitializersList()
{
	if ( m_initializerFunctions )
	{
		delete m_initializerFunctions;
		m_initializerFunctions = NULL;
	}
	m_initializerFunctions = new InitializerFunc[ m_numInitializers ];
	Uint32 funcIndex = 0;
	
	if ( m_initializerSetMask & PIF_Alpha )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitAlpha;
	}
	if ( m_initializerSetMask & PIF_AlphaRandom )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitAlphaRandom;
	}

	if ( m_initializerSetMask & PIF_Color )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitColor;
	}

	if ( m_initializerSetMask & PIF_ColorRandom )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitColorRandom;
	}

	if ( m_initializerSetMask & PIF_LifeTime )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitLifeTime;
	}
	if ( m_initializerSetMask & PIF_LifeTimeRandom )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitLifeTimeRandom;
	}

	if ( m_initializerSetMask & PIF_Position )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitPosition;
	}
	if ( m_initializerSetMask & PIF_PositionRandom )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitPositionRandom;
	}

	if ( m_initializerSetMask & PIF_Rotation )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitRotation;
	}
	if ( m_initializerSetMask & PIF_RotationRandom )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitRotationRandom;
	}

	if ( m_initializerSetMask & PIF_Rotation3D )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitRotation3D;
	}
	if ( m_initializerSetMask & PIF_Rotation3DRandom )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitRotation3DRandom;
	}

	if ( m_initializerSetMask & PIF_RotationRate )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitRotationRate;
	}
	if ( m_initializerSetMask & PIF_RotationRateRandom )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitRotationRateRandom;
	}

	if ( m_initializerSetMask & PIF_RotationRate3D )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitRotationRate3D;
	}
	if ( m_initializerSetMask & PIF_RotationRate3DRandom )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitRotationRate3DRandom;
	}

	if ( m_initializerSetMask & PIF_Size )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitSize;
	}
	if ( m_initializerSetMask & PIF_SizeRandom )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitSizeRandom;
	}

	if ( m_initializerSetMask & PIF_Size3D )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitSize3D;
	}
	if ( m_initializerSetMask & PIF_Size3DRandom )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitSize3DRandom;
	}

	if ( m_initializerSetMask & PIF_SpawnBox )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitSpawnBox;
	}

	if ( m_initializerSetMask & PIF_SpawnCircle )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitSpawnCircle;
	}

	if ( m_initializerSetMask & PIF_SpawnSphere )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitSpawnSphere;
	}

	if ( m_initializerSetMask & PIF_Velocity )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitVelocity;
	}
	if ( m_initializerSetMask & PIF_VelocityRandom )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitVelocityRandom;
	}

	if ( m_initializerSetMask & PIF_VelocityInherit )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitVelocityInherit;
	}
	if ( m_initializerSetMask & PIF_VelocityInheritRandom )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitVelocityInheritRandom;
	}

	if ( m_initializerSetMask & PIF_VelocitySpread )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitVelocitySpread;
	}

	if ( m_initializerSetMask & PIF_AnimationFrame )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitAnimationFrame;
	}
	if ( m_initializerSetMask & PIF_AnimationFrameRandom )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitAnimationFrameRandom;
	}

	if ( m_initializerSetMask & PIF_CollisionSpawn )
	{
		m_initializerFunctions[ funcIndex++ ] = &CRenderParticleEmitter::InitCollisionSpawn;
	}
}

void CRenderParticleEmitter::CompileModifiersList()
{
	if ( m_modifierFunctions )
	{
		delete m_modifierFunctions;
		m_modifierFunctions = NULL;
	}
	m_modifierFunctions = new ModifierFunc[ m_numModifiers ];
	Uint32 funcIndex = 0;

	if ( m_modifierSetMask & PMF_VelocityOverLifeAbsolute )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::VelocityAbsolute;
	}

	if ( m_modifierSetMask & PMF_VelocityOverLifeModulate )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::VelocityModulate;
	}

	if ( m_modifierSetMask & PMF_AccelerationLocal )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::AccelerationLocal;
	}

	if ( m_modifierSetMask & PMF_AccelerationWorld )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::AccelerationWorld;
	}

	if ( m_modifierSetMask & PMF_AlphaOverLifeAbsolute )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::AlphaAbsolute;
	}

	if ( m_modifierSetMask & PMF_AlphaOverLifeModulate )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::AlphaModulate;
	}

	if ( m_modifierSetMask & PMF_AlphaByDistance )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::AlphaDistance;
	}

	if ( m_modifierSetMask & PMF_ColorOverLifeAbsolute )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::ColorAbsolute;
	}

	if ( m_modifierSetMask & PMF_ColorOverLifeModulate )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::ColorModulate;
	}

	if ( m_modifierSetMask & PMF_1ARotationOverLifeAbsolute )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::RotationAbsolute_1A;
	}

	if ( m_modifierSetMask & PMF_1ARotationOverLifeModulate )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::RotationModulate_1A;
	}

	if ( m_modifierSetMask & PMF_1ARotRateOverLifeAbsolute )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::RotRateAbsolute_1A;
	}

	if ( m_modifierSetMask & PMF_1ARotRateOverLifeModulate )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::RotRateModulate_1A;
	}

	if ( m_modifierSetMask & PMF_3ARotationOverLifeAbsolute )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::RotationAbsolute_3A;
	}

	if ( m_modifierSetMask & PMF_3ARotationOverLifeModulate )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::RotationModulate_3A;
	}

	if ( m_modifierSetMask & PMF_3ARotRateOverLifeAbsolute )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::RotRateAbsolute_3A;
	}

	if ( m_modifierSetMask & PMF_3ARotRateOverLifeModulate )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::RotRateModulate_3A;
	}

	if ( m_modifierSetMask & PMF_2DSizeOverLifeAbsolute )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::SizeAbsolute_2D;
	}

	if ( m_modifierSetMask & PMF_2DSizeOverLifeModulate )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::SizeModulate_2D;
	}

	if ( m_modifierSetMask & PMF_3DSizeOverLifeAbsolute )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::SizeAbsolute_3D;
	}

	if ( m_modifierSetMask & PMF_3DSizeOverLifeModulate )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::SizeModulate_3D;
	}

	if ( m_modifierSetMask & PMF_AnimationFrameBySpeed )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::AnimationFrame_Speed;
	}

	if ( m_modifierSetMask & PMF_AnimationFrameByLife )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::AnimationFrame_Life;
	}

	if ( m_modifierSetMask & PMF_TargetNode )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::TargetNode;
	}

	if ( m_modifierSetMask & PMF_Target )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::TargetPosition;
	}

	if ( m_modifierSetMask & PMF_Turbulize )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::Turbulize;
	}

	if ( m_modifierSetMask & PMF_EffectAlpha )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::EffectAlpha;
	}

	if ( m_modifierSetMask & PMF_Collision )
	{
		m_modifierFunctions[ funcIndex++ ] = &CRenderParticleEmitter::Collision;
	}
}

IRenderResource* CRenderParticleEmitter::Create( const CParticleEmitter* particleEmitter )
{
	ASSERT( particleEmitter );

	IParticleDrawer* drawer = particleEmitter->GetParticleDrawer();
	if ( !drawer )
	{
		// No drawer, failed to create particle emitter
		return nullptr;
	}

	const Uint32 lodCount = particleEmitter->GetLODCount();
	RED_ASSERT( lodCount > 0, TXT("Particle emitter has 0 LODs") );
	if ( lodCount == 0 )
	{
		return nullptr;
	}

	CRenderParticleEmitter* rpe = new CRenderParticleEmitter;

	// Copy/initiate basic stuff
	{
		rpe->m_lods.Resize( lodCount );
		for ( Uint32 i = 0; i < lodCount; ++i )
		{
			const SParticleEmitterLODLevel& srcLod = particleEmitter->GetLOD( i );
			LOD& lod = rpe->m_lods[i];

			lod.m_emitterDurationSettings	= srcLod.m_emitterDurationSettings;
			lod.m_emitterDelaySettings		= srcLod.m_emitterDelaySettings;
			lod.m_burstList					= srcLod.m_burstList;
			lod.m_sortBackToFront			= srcLod.m_sortBackToFront;
			lod.m_isEnabled					= srcLod.m_isEnabled;


			const IEvaluatorFloat* birthRate = srcLod.m_birthRate;
			if ( birthRate ) 
			{
				// Drey todo: random
				lod.m_birthRate = new SCurveEvaluatorScalar;
				birthRate->GetApproximationSamples( lod.m_birthRate->m_samples );
			}
		}


		rpe->m_vertexDrawerType =								drawer->GetVertexDrawerType();
		rpe->m_maxParticles =									Min( particleEmitter->GetMaxParticles(), (Uint32)1000 );
		rpe->m_emitterLoops =									particleEmitter->GetEmitterLoops();
		rpe->m_keepSimulationLocal =							particleEmitter->IsKeepSimulationLocal();
		// DreyTODO: The IsKeepSimulationLocal is a solution, but should be at least suggested in editor
		rpe->m_useSubFrameEmission =							particleEmitter->IsUseSubFrameEmission() && !particleEmitter->IsKeepSimulationLocal();

		rpe->m_envColorGroup =									particleEmitter->GetEnvColorGroup();
		rpe->m_windInfluence =									particleEmitter->GetWindInfluence();
		rpe->m_useOnlyWindInfluence =							particleEmitter->GetUseOnlyWindInfluence();
		rpe->m_internalPriority =								particleEmitter->GetInternalPriority();

#ifndef NO_EDITOR
		rpe->SetUniqueId( particleEmitter->GetUniqueId() );
#endif

		rpe->m_particleType =					(ERenderParticleType)(Uint32) particleEmitter->GetParticleType();

		if (particleEmitter->GetDeathDecalSpawner() && particleEmitter->GetDeathDecalSpawner()->IsA(ClassID<CDecalSpawner>()))
		{
			rpe->m_decalSpawner = new CRenderDecalSpawner( (CDecalSpawner*) particleEmitter->GetDeathDecalSpawner() );
		}

		if (particleEmitter->GetCollisionSpawner() && particleEmitter->GetCollisionSpawner()->IsA(ClassID<CDecalSpawner>()))
		{
			rpe->m_collisionDecalSpawner = new CRenderDecalSpawner( (CDecalSpawner*) particleEmitter->GetCollisionSpawner() );
		}

		if (particleEmitter->GetMotionDecalSpawner() && particleEmitter->GetMotionDecalSpawner()->IsA(ClassID<CDecalSpawner>()))
		{
			rpe->m_motionDecalSpawner = new CRenderDecalSpawner( (CDecalSpawner*) particleEmitter->GetMotionDecalSpawner() );
		}
	}

	// Prepare simulation data approximations based on emitter modules set
	{
		particleEmitter->SetupSimulationFlags( rpe->m_modifierSetMask, rpe->m_initializerSetMask, rpe->m_numModifiers, rpe->m_numInitializers, rpe->m_seedsInitializers );

		rpe->CompileInitializersList();
		rpe->CompileModifiersList();

		particleEmitter->GenerateApproximatedUpdaterData( rpe->m_updaterData );

		// Copy drawer setup to the lightweight representation
		switch ( drawer->GetVertexDrawerType() )
		{
		case PVDT_Rain:		
			{
				CParticleDrawerRain* rainDrawer = Cast< CParticleDrawerRain >( drawer );
				ASSERT( rainDrawer );
				rpe->m_drawerData.motionBlur.m_stretchPerVelocity = rainDrawer->m_stretchPerVelocity;
				rpe->m_drawerData.motionBlur.m_oneOverBlendVelocityRange = rainDrawer->m_blendEndVelocity; // Drey TODO
			}
			break;
		case PVDT_MotionBlurred:
			{
				CParticleDrawerMotionBlur* motionDrawer = Cast< CParticleDrawerMotionBlur >( drawer );
				ASSERT( motionDrawer );
				rpe->m_drawerData.motionBlur.m_stretchPerVelocity = motionDrawer->m_stretchPerVelocity;
				rpe->m_drawerData.motionBlur.m_oneOverBlendVelocityRange = 1.0f; // Drey TODO
			}
			break;
		case PVDT_Trail:
		case PVDT_FacingTrail:
			{
				CParticleDrawerTrail* trailDrawer = Cast< CParticleDrawerTrail >( drawer );
				ASSERT( trailDrawer );
				rpe->m_drawerData.trail.m_dynamicTexCoords = trailDrawer->m_dynamicTexCoords;
				rpe->m_drawerData.trail.m_texturesPerUnit = trailDrawer->m_texturesPerUnit;
				rpe->m_drawerData.trail.m_minSegmentsPer360Degrees = (Float) Min( trailDrawer->m_minSegmentsPer360Degrees, 500 );
			}
			break;
		case PVDT_Mesh:
			{
				CParticleDrawerMesh* meshDrawer = Cast< CParticleDrawerMesh >( drawer );
				ASSERT( meshDrawer );

				// Collect render meshes
				const IParticleDrawer::TMeshList* meshes = meshDrawer->GetMeshes();
				if ( nullptr != meshes )
				{
					for ( Uint32 i = 0; i<meshes->Size(); ++i )
					{
						if ( (*meshes)[i].IsValid() )
						{
							CRenderMesh* renderMesh = ( CRenderMesh* )( (*meshes)[i]->GetRenderResource() );
							if ( renderMesh )
							{
								renderMesh->AddRef();
								rpe->m_renderMeshes.PushBack( renderMesh );
							}
						}
					}
				}

				rpe->m_drawerData.mesh.m_lightChannels = meshDrawer->m_lightChannels;
				rpe->m_drawerData.mesh.m_orientationMode = meshDrawer->m_orientationMode;
			}
			break;
		case PVDT_Beam:
			{
				CParticleDrawerBeam* beamDrawer = Cast< CParticleDrawerBeam >( drawer );
				ASSERT( beamDrawer );
				rpe->m_drawerData.beam.m_numSegments = beamDrawer->m_numSegments;
				rpe->m_drawerData.beam.m_texturesPerUnit = beamDrawer->m_texturesPerUnit;
			}
			break;
		}
	}

	return rpe;
}

#ifndef NO_EDITOR
Bool CRenderParticleEmitter::CheckBufferCompatibility( CRenderParticleEmitter* other )
{
	ASSERT( other );
	if ( m_particleType != other->GetParticleType() ||
		m_maxParticles != other->GetMaxParticles() )
	{
		// Not compatible
		return false;
	}

	// Compatible
	return true;
}
#endif

IRenderResource* CRenderInterface::CreateParticleEmitter( const CParticleEmitter* particleEmitter )
{
	return CRenderParticleEmitter::Create( particleEmitter );
}
