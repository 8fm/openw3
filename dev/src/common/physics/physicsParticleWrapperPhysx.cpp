#include "build.h"
#include "../physics/physXEngine.h"
#include "physicsParticleWrapper.h"
#include "../physics/physicsWorldPhysXImpl.h"
#include "../physics/physicsSettings.h"
#include "../physics/PhysicsWrappersDefinition.h"

#ifdef USE_PHYSX
using namespace physx;
#endif

DECLARE_PHYSICS_WRAPPER(CPhysicsParticleWrapper,EPW_Partice,false,true)

CPhysicsParticleWrapper::CPhysicsParticleWrapper( Uint32 particleSystemsMax, Uint32 visibiltyId )
	: CPhysicsWrapperInterface()
{
#ifdef USE_PHYSX
	GetParentProvider()->GetPhysicsWorld( m_world );

	PxPhysics* physics = GPhysXEngine->GetPxPhysics();

	m_systems.Resize( particleSystemsMax );
	Red::System::MemorySet( m_systems.TypedData(), 0, sizeof( SParticleSystemData* ) * particleSystemsMax );

	const Vector& pos = GetParentProvider()->GetLocalToWorld().GetTranslation();
	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsParticleWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	position->m_x = pos.X;
	position->m_y = pos.Y;
	position->m_resultDistanceSquared = FLT_MAX;
	position->m_visibilityQueryId = visibiltyId;

#endif
}

CPhysicsParticleWrapper::~CPhysicsParticleWrapper()
{
#ifdef USE_PHYSX
	Bool result = true;
#ifdef USE_PHYSX
	Uint32 systemsCount = m_systems.Size();
	for ( Uint16 i = 0; i < systemsCount; i++ )
	{
		SParticleSystemData* particleSystemData = m_systems[ i ];
		if( !particleSystemData ) continue;
		if( particleSystemData->m_system ) ( ( PxParticleSystem* )particleSystemData->m_system )->release();
		particleSystemData->m_system = nullptr;
		particleSystemData->~SParticleSystemData();
		RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysicsEngine, particleSystemData );
	}
	m_systems.Clear();
#endif
#endif
}

Bool CPhysicsParticleWrapper::MakeReadyToDestroy( TDynArray< void* >* toRemove )
{
	Bool result = true;
#ifdef USE_PHYSX
	Uint32 systemsCount = m_systems.Size();
	for( Uint32 i = 0; i != systemsCount; ++i )
	{
		SParticleSystemData* particleSystemData = m_systems[ i ];
		if( !particleSystemData ) continue;
		if( !particleSystemData->m_system ) continue;
		if( ( ( PxParticleSystem* )particleSystemData->m_system )->getScene() )
		{
			toRemove->PushBack( particleSystemData->m_system );
			result = false;
		}
	}
#endif
	return result;
}

void CPhysicsParticleWrapper::PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd )
{
#ifdef USE_PHYSX
	PC_SCOPE_PHYSICS(CPhysicsParticleWrapper PreSimulation )

	PxScene* scene = static_cast< CPhysicsWorldPhysXImpl* >( m_world )->GetPxScene();

	Bool attached = false;

	Uint32 systemsCount = m_systems.Size();
	for ( Uint16 i = 0; i < systemsCount; i++ )
	{
		if( SParticleSystemData* particleSystemData = m_systems[ i ] )
		{
			PxParticleSystem* system = ( PxParticleSystem* ) particleSystemData->m_system;

			if( !system ) continue;

			if( system->getScene() )
			{
				attached = true;
				break;
			}
		}
	}
	const float distanceFromViewportSquared = simulationContext->m_resultDistanceSquared;

	float distanceLimit = SPhysicsSettings::m_particleSimulationDistanceLimit * SPhysicsSettings::m_particleSimulationDistanceLimit;
	if( attached && distanceFromViewportSquared > distanceLimit )
	{
		for( Uint32 i = 0; i != systemsCount; ++i )
		{
			SParticleSystemData* particleSystemData = m_systems[ i ];
			if( !particleSystemData ) continue;
			PxParticleSystem* system = ( PxParticleSystem* ) particleSystemData->m_system;
			if( !system ) continue;
			scene->removeActor( *system );
		}
	}
	else if( !attached && distanceFromViewportSquared < distanceLimit && m_ref.GetValue() > 0 )
	{
		for( Uint32 i = 0; i != systemsCount; ++i )
		{
			SParticleSystemData* particleSystemData = m_systems[ i ];
			if( !particleSystemData ) continue;
			PxParticleSystem* system = ( PxParticleSystem* ) particleSystemData->m_system;
			if( !system ) continue;
			scene->addActor( *system );
		}
	}
	if( !attached ) return;

	for( Uint32 j = 0; j != systemsCount; ++j )
	{
		SParticleSystemData* particleSystemData = m_systems[ j ];
		if( !particleSystemData ) continue;

		PxParticleSystem* particleSystem = ( PxParticleSystem* ) particleSystemData->m_system;
		if( !particleSystem ) continue;

/*		Bool isEnabled = particleSystem->getParticleBaseFlags() & PxParticleBaseFlag::eENABLED;

		const ERenderVisibilityResult visibilityQueryResult = ( const ERenderVisibilityResult ) simulationContext->m_visibilityQueryResult;
		Bool shouldBeEnabled = visibilityQueryResult != ERenderVisibilityResult::RVR_NotVisible;
		if( isEnabled && !shouldBeEnabled )
		{
			particleSystem->setParticleBaseFlag( PxParticleBaseFlag::eENABLED, false );
		}
		else if( !isEnabled && shouldBeEnabled )
		{
			particleSystem->setParticleBaseFlag( PxParticleBaseFlag::eENABLED, true );
		}*/

		SParticleSystemData::SInputBuffer* inputBuffer = nullptr;

		Uint64 currentTick = 0;

		for( Uint8 i = 0; i != BufferingSize; ++i )
		{
			SParticleSystemData::SInputBuffer& input = particleSystemData->m_inputBuffer[ i ];
			Uint64 tick = input.m_tick.GetValue();
			if( !tick || tick <= currentTick ) continue;
			currentTick = tick;
			inputBuffer = &input;
		}

		if( !inputBuffer )
		{
			continue;
		}

		for( Uint8 i = 0; i != BufferingSize; ++i )
		{
			SParticleSystemData::SInputBuffer* input = &particleSystemData->m_inputBuffer[ i ];
			Uint64 tick = input->m_tick.GetValue();
			if( tick && tick < currentTick )
			{
				input->m_tick.SetValue( 0 );
			}
		}

		if( PxParticleReadData* read = particleSystem->lockParticleReadData( PxDataAccessFlag::eREADABLE ) )
		{
			Uint32 validMaxIndex = read->validParticleRange;
			Uint32 validCount = validMaxIndex + 1;
			static TDynArray< PxU8 > validBuffer;
			validBuffer.ResizeFast( validCount / 8 + 1 );
			Red::MemoryCopy( validBuffer.TypedData(), read->validParticleBitmap, validCount / 8 + 1 );
			read->unlock();

			for( Uint32 i = 0; i != inputBuffer->m_inputRefreshBuffer.Size(); ++i )
			{
				Uint32 index = inputBuffer->m_inputRefreshBuffer[ i ].m_index;
				PxStrideIterator< PxU32 > indexesInput( PxStrideIterator< PxU32 >( &index, sizeof( SParticleSystemData::SInputBuffer::SInputRefresh ) ) );
				PxStrideIterator< PxVec3 > positionsInputBuffer( PxStrideIterator< PxVec3 >( ( PxVec3* ) &inputBuffer->m_inputRefreshBuffer[ i ].m_position, sizeof( SParticleSystemData::SInputBuffer::SInputRefresh ) ) );
				PxStrideIterator< PxVec3 > velocitiesInputBuffer( PxStrideIterator< PxVec3 >( ( PxVec3* ) &inputBuffer->m_inputRefreshBuffer[ i ].m_velocity, sizeof( SParticleSystemData::SInputBuffer::SInputRefresh ) ) );
				Bool isToCreate = true;
				if( index < validMaxIndex )
				{
					Uint8 validnes = validBuffer.TypedData()[ index / 8 ];
					Uint8 subindex = index % 8;
					Uint8 valid = ( validnes >> subindex ) & 0x1;
					if( valid )
					{
						isToCreate = false;
					}
				}
					
				if( isToCreate )
				{
					PxParticleCreationData particleCreationData;
					particleCreationData.numParticles = 1;
					particleCreationData.indexBuffer = indexesInput;
					particleCreationData.positionBuffer = positionsInputBuffer;
					particleCreationData.velocityBuffer = velocitiesInputBuffer;
					
//					LOG_ENGINE( TXT( "particle add %i %i"), index, particleSystem );

					particleSystem->createParticles( particleCreationData );

				}
				else
				{
					particleSystem->setPositions( 1, indexesInput, positionsInputBuffer );
					particleSystem->setVelocities( 1, indexesInput, velocitiesInputBuffer );
				}
			}
			inputBuffer->m_inputRefreshBuffer.ClearFast();

			if( inputBuffer->m_inputLoad < validMaxIndex )
			{
				static TDynArray< PxU32 > indexesToRemoveLocalBuffer;
				indexesToRemoveLocalBuffer.ClearFast();
				while( inputBuffer->m_inputLoad < validCount - 1 )
				{
					Uint8 validnes = validBuffer.TypedData()[ validCount / 8 ] ;
					Uint8 subindex = validCount % 8;
					Uint8 valid = ( validnes >> subindex ) & 0x1;
					if( valid )
					{
						indexesToRemoveLocalBuffer.PushBack( validCount );
//						LOG_ENGINE( TXT( "particle remove %i %i"), validCount, particleSystem );
					}
					--validCount;
				}

				if( Uint32 size = indexesToRemoveLocalBuffer.Size() )
				{
					PxStrideIterator< PxU32 > indexesInput( PxStrideIterator< PxU32 >( indexesToRemoveLocalBuffer.TypedData(), sizeof( Uint32 ) ) );
					particleSystem->releaseParticles( size, indexesInput );
				}

			}

			if( inputBuffer->m_inputVelocityBuffer.Size() )
			{
				for( Uint32 i = inputBuffer->m_inputVelocityBuffer.Size() - 1; i > 0; --i )
				{
					Uint32 index = inputBuffer->m_inputVelocityBuffer[ i ].m_index;

					if( index >= validCount )
					{
						inputBuffer->m_inputVelocityBuffer.RemoveAtFast( i );
						continue;
					}
				}

				if( Uint32 count = inputBuffer->m_inputVelocityBuffer.Size() )
				{
					PxStrideIterator< PxU32 > indexesInput( PxStrideIterator< PxU32 >( &inputBuffer->m_inputVelocityBuffer.TypedData()->m_index, sizeof( SParticleSystemData::SInputBuffer::SInputVelocity ) ) );
					PxStrideIterator< PxVec3 > velocitiesInputBuffer( PxStrideIterator< PxVec3 >( ( PxVec3* ) &inputBuffer->m_inputVelocityBuffer.TypedData()->m_velocity, sizeof( SParticleSystemData::SInputBuffer::SInputVelocity ) ) );
					particleSystem->addForces( count, indexesInput, velocitiesInputBuffer, PxForceMode::eVELOCITY_CHANGE );

				}

			}
			inputBuffer->m_inputVelocityBuffer.ClearFast();
			inputBuffer->m_tick.SetValue( 0 );

		}
	}

#endif
}

void CPhysicsParticleWrapper::PostSimulation( Uint8 visibilityQueryResult )
{
#ifdef USE_PHYSX
	PC_SCOPE_PHYSICS(CPhysicsParticleWrapper PostSimulation )

	Uint32 systemsCount = m_systems.Size();
	for ( Uint16 i = 0; i < systemsCount; i++ )
	{
		SParticleSystemData* particleSystemData = m_systems[ i ];
		if( !particleSystemData ) continue;

		PxParticleSystem* system = ( PxParticleSystem* ) particleSystemData->m_system;

		if( !system ) continue;

		if( !system->getScene() )
		{
			return;
		}
	}

	if( m_ref.GetValue() <= 0 ) return;

	for ( Uint16 i = 0; i < systemsCount; i++ )
	{
		SParticleSystemData* particleSystemData = m_systems[ i ];
		if( !particleSystemData ) continue;

		PxParticleSystem* particleSystem = ( PxParticleSystem* )particleSystemData->m_system;
		if( !particleSystem ) continue;

		SParticleSystemData::SOutputBuffer* output = nullptr;
		for( Uint8 i = 0; i != BufferingSize; ++i )
		{
			if( particleSystemData->m_outputBuffer[ i ].m_tick.GetValue() ) continue;
			output = &particleSystemData->m_outputBuffer[ i ];
			break;
		}

		if( output == nullptr ) continue;

		PxParticleReadData* read = particleSystem->lockParticleReadData( PxDataAccessFlag::eREADABLE );
		if( !read || !read->nbValidParticles )
		{
			read->unlock();
			continue;
		}

		Uint32 validParticleRange = read->validParticleRange;
		output->m_outputValidMap.ResizeFast( validParticleRange / 8 + 1 );
		Red::System::MemoryCopy( output->m_outputValidMap.TypedData(), read->validParticleBitmap, validParticleRange / 8 + 1 );

		output->m_outputPositionVelocityBuffer.ClearFast();

		PxStrideIterator<const PxParticleFlags> flagsIt(read->flagsBuffer);
		PxStrideIterator<const PxVec3> posIt(read->positionBuffer);
		PxStrideIterator<const PxVec3> velIt(read->velocityBuffer);

		for (unsigned i = 0; i < validParticleRange; ++i, ++flagsIt, ++posIt, ++velIt)
		{
/*			if( *flagsIt & ( PxParticleFlag::eSPATIAL_DATA_STRUCTURE_OVERFLOW ) )
			{
				int a = 0;
			}*/
			Uint8 flags = *flagsIt & ( PxParticleFlag::eCOLLISION_WITH_STATIC | PxParticleFlag::eCOLLISION_WITH_DYNAMIC );
			output->m_outputPositionVelocityBuffer.PushBack( SParticleSystemData::SOutputBuffer::SOutputPositionVelocity( flags, Vector3( posIt->x, posIt->y, posIt->z ), Vector3( velIt->x, velIt->y, velIt->z ) ) );
		}

		read->unlock();

		for( Uint8 i = 0; i != BufferingSize; ++i )
		{
			SParticleSystemData* particleSystemData = m_systems[ i ];
			if( !particleSystemData ) continue;
			output->m_tick.SetValue( Red::System::Clock::GetInstance().GetTimer().GetTicks() );
			break;
		}
	}

#endif
}

Bool CPhysicsParticleWrapper::IsParticleGPUSimulationEnabled()
{
#ifdef USE_PHYSX
#ifndef RED_FINAL_BUILD
	if( SPhysicsSettings::m_dontCreateParticlesOnGPU ) return false;
#endif
#ifdef USE_PHYSX_GPU
	if( !GPhysXEngine->GetCudaContext() ) return false;
#endif
#endif
	return true;
}

void CPhysicsParticleWrapper::Release( Uint32 actorIndex )
{
	RED_ASSERT( m_ref.GetValue() > 0 )
	if( !m_ref.Decrement() )
	{
		m_world->GetWrappersPool< CPhysicsParticleWrapper, SWrapperContext >()->PushWrapperToRemove( this );
	}
}

CPhysicsWorld* CPhysicsParticleWrapper::GetPhysicsWorld()
{
	return m_world;
}

Bool CPhysicsParticleWrapper::IsReady() const
{
#ifndef USE_PHYSX
	return false;
#else
	if( m_systems.Empty() ) return false;
	Uint32 systemsCount = m_systems.Size();
	for ( Uint16 i = 0; i < systemsCount; i++ )
	{
		SParticleSystemData* particleSystemData = m_systems[ i ];
		if( !particleSystemData ) continue;

		PxParticleSystem* system = ( PxParticleSystem* ) particleSystemData->m_system;

		if( !system ) continue;

		return system->getScene() != 0;
	}
	return false;
#endif
}

Bool CPhysicsParticleWrapper::GetParticleOutput( Uint32 actorIndex, Uint32 shapeIndex, Vector3* position, Vector3* velocity, Bool* colliding )
{
	if( m_systems.Size() <= actorIndex ) return false;
	SParticleSystemData* particleSystemData = m_systems[ actorIndex ];
	if( !particleSystemData ) return false;
#ifdef USE_PHYSX
	PxParticleSystem* particleSystem = ( PxParticleSystem* ) particleSystemData->m_system;
	if( !particleSystem ) return false;

	if( particleSystemData->m_currentOutputBuffer == nullptr )
	{
		Uint64 currentTick = 0;
		for( Uint8 i = 0; i != BufferingSize; ++i )
		{
			SParticleSystemData::SOutputBuffer& output = particleSystemData->m_outputBuffer[ i ];
			Uint64 tick = output.m_tick.GetValue();
			if( !tick || tick <= currentTick ) continue;
			currentTick = tick;
			particleSystemData->m_currentOutputBuffer = &output;
		}
		if( particleSystemData->m_currentOutputBuffer == nullptr )
		{
			return false;
		}
		for( Uint8 i = 0; i != BufferingSize; ++i )
		{
			SParticleSystemData::SOutputBuffer& output = particleSystemData->m_outputBuffer[ i ];
			Uint64 tick = output.m_tick.GetValue();
			if( tick && tick < currentTick )
			{
				output.m_tick.SetValue( 0 );
			}
		}
	}


	SParticleSystemData::SOutputBuffer* outputBuffer = particleSystemData->m_currentOutputBuffer;

	Uint32 max = particleSystemData->m_max;
	if( max <= shapeIndex ) return false;
	if( outputBuffer->m_outputPositionVelocityBuffer.Size() <= shapeIndex ) return false;

	Uint8 validnes = outputBuffer->m_outputValidMap[ actorIndex / 8 ];
	Uint8 subindex = actorIndex % 8;
	Uint8 valid = ( validnes >> subindex ) & 0x1;
	if( !valid )
	{
		return false;
	}

	SParticleSystemData::SOutputBuffer::SOutputPositionVelocity& buffer = outputBuffer->m_outputPositionVelocityBuffer[ shapeIndex ];
	*position = buffer.m_outputPositionsBuffer;
	*velocity = buffer.m_outputVelocityBuffer;
	if( colliding )
	{
		*colliding = buffer.m_outputValid > 1;
	}
	
#endif
	return true;
}

Bool CPhysicsParticleWrapper::AddParticleSystem( Uint32 systemIndex, Uint32 maxLimit, CPhysicsEngine::CollisionMask collisionMask, Float dynamicFriction, Float staticFriction, Float restitution, Float velocityDamp, bool disableGravity, Float radius, bool gpuSimulation )
{
#ifdef USE_PHYSX
	if( m_systems.Size() <= systemIndex  ) return false;

	SParticleSystemData* particleSystemData = m_systems[ systemIndex ];
	if( particleSystemData ) return false;

	m_systems[ systemIndex ] = ( SParticleSystemData* ) RED_MEMORY_ALLOCATE( MemoryPool_Physics, MC_PhysicsEngine, sizeof( SParticleSystemData ) + sizeof( Vector3 ) * ( maxLimit - 1 ) );
	particleSystemData = m_systems[ systemIndex ];
	::new( ( SParticleSystemData* ) particleSystemData ) SParticleSystemData( maxLimit );

	Bool perParticleRestOffset = false;

	PxParticleSystem* system = GPhysXEngine->GetPxPhysics()->createParticleSystem( maxLimit, perParticleRestOffset );
	if( !system ) return false;

#ifndef RED_FINAL_BUILD
	if( !SPhysicsSettings::m_dontCreateParticlesOnGPU )
#endif
	{
		if( gpuSimulation && IsParticleGPUSimulationEnabled() )
		{
//			system->setParticleBaseFlag( PxParticleBaseFlag::eGPU, gpuSimulation );
		}
	}

	SPhysicalFilterData collision( 0, collisionMask );
	system->setSimulationFilterData( collision.m_data );

	system->userData = this;
	particleSystemData->m_system = system;
	particleSystemData->m_max = maxLimit;

	system->setDynamicFriction( dynamicFriction );
	system->setStaticFriction( staticFriction );
	system->setRestitution( restitution );
	system->setDamping( velocityDamp );
	system->setMaxMotionDistance( 10.0f );

//	float grid = system->getGridSize();
	system->setGridSize( SPhysicsSettings::m_particleCellSize );
	
	system->setParticleBaseFlag( PxParticleBaseFlag::eCOLLISION_WITH_DYNAMIC_ACTORS, true );
	system->setParticleBaseFlag( PxParticleBaseFlag::ePER_PARTICLE_COLLISION_CACHE_HINT, true );
	
//	system->setParticleReadDataFlag( PxParticleReadDataFlag::ePOSITION_BUFFER, true );
	system->setParticleReadDataFlag( PxParticleReadDataFlag::eVELOCITY_BUFFER, true );
//	system->setParticleReadDataFlag( PxParticleReadDataFlag::eREST_OFFSET_BUFFER, true );

	system->setActorFlag( physx::PxActorFlag::eDISABLE_GRAVITY, disableGravity );
	system->setRestOffset( radius );
	system->setContactOffset( radius + 0.001f );
//	system->setParticleBaseFlag( PxParticleBaseFlag::eENABLED, false );

#endif
	return true;
}

Bool CPhysicsParticleWrapper::RefreshParticle( Uint32 systemIndex, Uint32 particleIndex, const Vector3& position, const Vector3& velocity )
{
	if( m_systems.Size() <= systemIndex ) return false;

	SParticleSystemData* particleSystemData = m_systems[ systemIndex ];
	if( !particleSystemData ) return false;

#ifdef USE_PHYSX
	PxParticleSystem* particleSystem = ( PxParticleSystem* ) particleSystemData->m_system;
	if( !particleSystem ) return false;

	Uint32 max = particleSystemData->m_max;
	if( max <= particleIndex ) return false;

	if( particleSystemData->m_currentInputBuffer == nullptr )
	{
		for( Uint8 i = 0; i != BufferingSize; ++i )
		{
			if( !particleSystemData->m_inputBuffer[ i ].m_tick.GetValue() )
			{
				particleSystemData->m_currentInputBuffer = &particleSystemData->m_inputBuffer[ i ];
				particleSystemData->m_currentInputBuffer->m_inputRefreshBuffer.ClearFast();

			}

			if( !particleSystemData->m_currentInputBuffer ) continue;
			break;
		}
		if( particleSystemData->m_currentInputBuffer == nullptr )
		{
			return false;
		}
	}

	SParticleSystemData::SInputBuffer* inputBuffer = particleSystemData->m_currentInputBuffer;
	if( inputBuffer->m_inputRefreshBuffer.Size() < max )
	{
		inputBuffer->m_inputRefreshBuffer.PushBack( SParticleSystemData::SInputBuffer::SInputRefresh( particleIndex, position, velocity ) );
	}
#endif
	return true;
}

Bool CPhysicsParticleWrapper::ApplyVelocity( Uint32 systemIndex, Uint32 particleIndex, const Vector3& velocity )
{
	if( m_systems.Size() <= systemIndex ) return false;
	SParticleSystemData* particleSystemData = m_systems[ systemIndex ];
	if( !particleSystemData ) return false;
#ifdef USE_PHYSX
	PxParticleSystem* particleSystem = ( PxParticleSystem* ) particleSystemData->m_system;
	if( !particleSystem ) return false;

	Uint32 max = particleSystemData->m_max;
	if( max <= particleIndex ) return false;

	if( particleSystemData->m_currentInputBuffer == nullptr )
	{
		for( Uint8 i = 0; i != BufferingSize; ++i )
		{
			if( !particleSystemData->m_inputBuffer[ i ].m_tick.GetValue() )
			{
				particleSystemData->m_currentInputBuffer = &particleSystemData->m_inputBuffer[ i ];
				particleSystemData->m_currentInputBuffer->m_inputVelocityBuffer.ClearFast();
			}

			if( !particleSystemData->m_currentInputBuffer ) continue;
			break;
		}
		if( particleSystemData->m_currentInputBuffer == nullptr )
		{
			return false;
		}
	}

	SParticleSystemData::SInputBuffer* inputBuffer = particleSystemData->m_currentInputBuffer;
	if( inputBuffer->m_inputVelocityBuffer.Size() < max )
	{
		inputBuffer->m_inputVelocityBuffer.PushBack( SParticleSystemData::SInputBuffer::SInputVelocity( particleIndex, velocity ) );
	}
#endif
	return true;
}

void CPhysicsParticleWrapper::FlushInputOutput( Uint32 actorIndex, Uint32 load )
{
	if( m_systems.Size() <= actorIndex ) return;
	SParticleSystemData* particleSystemData = m_systems[ actorIndex ];
	if( !particleSystemData ) return;

	if( particleSystemData->m_currentOutputBuffer )
	{
		particleSystemData->m_currentOutputBuffer = nullptr;
	}

	if( !particleSystemData->m_currentInputBuffer ) return;

	SParticleSystemData::SInputBuffer* buffer = particleSystemData->m_currentInputBuffer;
	particleSystemData->m_currentInputBuffer = nullptr;
	buffer->m_inputLoad = load;
	buffer->m_tick.SetValue( Red::System::Clock::GetInstance().GetTimer().GetTicks() );
}

Bool CPhysicsParticleWrapper::PopCachedCollision( Uint32 systemIndex, Vector3* position )
{
	if( m_systems.Size() <= systemIndex ) return false;
	SParticleSystemData* particleSystemData = m_systems[ systemIndex ];
	if( !particleSystemData ) return false;

#ifdef USE_PHYSX
	PxParticleSystem* particleSystem = ( PxParticleSystem* ) particleSystemData->m_system;
	if( !particleSystem ) return false;

	if( !particleSystemData->m_cachedCollisionPositionsBuffer.Size() )
	{
		return false;
	}

	if( position )
	{
		*position = particleSystemData->m_cachedCollisionPositionsBuffer.PopBackFast();
	}
	
#endif
	return true;
}

Bool CPhysicsParticleWrapper::PushCachedCollision( Uint32 systemIndex, const Vector3& position )
{
	if( m_systems.Size() <= systemIndex ) return false;
	SParticleSystemData* particleSystemData = m_systems[ systemIndex ];
	if( !particleSystemData ) return false;
#ifdef USE_PHYSX
	PxParticleSystem* particleSystem = ( PxParticleSystem* ) particleSystemData->m_system;
	if( !particleSystem ) return false;

	Uint32 size = particleSystemData->m_cachedCollisionPositionsBuffer.Size();
	if( size == particleSystemData->m_max )
	{
		return false;
	}
	
	Uint32 index = 0;
	if( size ) 
	{
		static CStandardRand randomGenerator;
		index = randomGenerator.Get< Uint32 >( 0, size );
	}
	if( index < size )
	{
		Vector previousPosition = particleSystemData->m_cachedCollisionPositionsBuffer[ index ];
		particleSystemData->m_cachedCollisionPositionsBuffer[ index ] = position;
		particleSystemData->m_cachedCollisionPositionsBuffer.PushBack( previousPosition );
	}
	else
	{
		particleSystemData->m_cachedCollisionPositionsBuffer.PushBack( position );
	}
#endif
	return true;
}

Uint32 CPhysicsParticleWrapper::GetCachedCollisionCount( Uint32 systemIndex )
{
	if( m_systems.Size() <= systemIndex ) return 0;
	SParticleSystemData* particleSystemData = m_systems[ systemIndex ];
	if( !particleSystemData ) return false;
#ifdef USE_PHYSX
	PxParticleSystem* particleSystem = ( PxParticleSystem* ) particleSystemData->m_system;
	if( !particleSystem ) return 0;

	return particleSystemData->m_cachedCollisionPositionsBuffer.Size();
#else
	return 0;
#endif
}