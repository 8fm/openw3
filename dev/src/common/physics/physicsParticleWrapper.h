/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../physics/physicsWrapper.h"
#include "../physics/physicsWrapperPool.h"

class CPhysicsParticleWrapper : public CPhysicsWrapperInterface
{
	friend class TWrappersPool< CPhysicsParticleWrapper, SWrapperContext >;

protected:
	const static Uint8 BufferingSize = 4;

	struct SParticleSystemData
	{
		void* m_system;

		Uint32 m_max;

		struct SInputBuffer
		{
			struct SInputRefresh
			{
				Uint32 m_index;
				Vector3 m_position;
				Vector3 m_velocity;
				SInputRefresh( Uint32 index, const Vector3 &position, const Vector3& velocity ) : m_index( index ), m_position( position ), m_velocity( velocity ) {}
			};
			TDynArray< SInputRefresh, MC_PhysicsEngine, MemoryPool_Physics > m_inputRefreshBuffer;

			struct SInputVelocity
			{
				Uint32 m_index;
				Vector3 m_velocity;
				SInputVelocity( Uint32 index, const Vector3& velocity ) : m_index( index ), m_velocity( velocity ) {}
			};
			TDynArray< SInputVelocity, MC_PhysicsEngine, MemoryPool_Physics > m_inputVelocityBuffer;

			Uint32 m_inputLoad;
			Red::Threads::CAtomic< Uint64 > m_tick;

			SInputBuffer() : m_inputLoad( 0 )
			{
				m_tick.SetValue( 0 );
			}
		};

		SInputBuffer m_inputBuffer[ BufferingSize ];
		SInputBuffer* m_currentInputBuffer;

		struct SOutputBuffer 
		{
			struct SOutputPositionVelocity 
			{
				Uint8 m_outputValid;
				Vector3 m_outputPositionsBuffer;
				Vector3 m_outputVelocityBuffer;
				SOutputPositionVelocity( Uint8 outputValid, const Vector3& outputPositionsBuffer, const Vector3& outputVelocityBuffer ) : m_outputValid( outputValid ), m_outputPositionsBuffer( outputPositionsBuffer ), m_outputVelocityBuffer( outputVelocityBuffer ) {}
				SOutputPositionVelocity( Uint8 outputValid ) : m_outputValid( outputValid ) {}
			};
			TDynArray< SOutputPositionVelocity > m_outputPositionVelocityBuffer;
			TDynArray< Uint8 > m_outputValidMap;
			Red::Threads::CAtomic< Uint64 > m_tick;

			SOutputBuffer()
			{
				m_tick.SetValue( 0 );
			}
		};
		SOutputBuffer m_outputBuffer[ BufferingSize ];
		SOutputBuffer* m_currentOutputBuffer;

		TDynArray< Vector3 > m_cachedCollisionPositionsBuffer;

		SParticleSystemData( Uint32 maxLimit ) 
			: m_system( 0 )
			, m_max( maxLimit )
			, m_currentInputBuffer( nullptr)
			, m_currentOutputBuffer( nullptr )
		{}

	};
	TDynArray< SParticleSystemData* >	m_systems;

protected:
	CPhysicsParticleWrapper( Uint32 particleSystemsMax, Uint32 visibiltyId );
	virtual ~CPhysicsParticleWrapper();

	Bool MakeReadyToDestroy( TDynArray< void* >* toRemove );
	void PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd );
	virtual void PostSimulation( Uint8 visibilityQueryResult );

	static Bool IsParticleGPUSimulationEnabled();

public:
	void AddRef() { m_ref.Increment(); }

	virtual void Release( Uint32 actorIndex = 0 );

	virtual CPhysicsWorld* GetPhysicsWorld();

	virtual Bool IsReady() const;

	Bool AddParticleSystem( Uint32 systemIndex, Uint32 maxLimit, CPhysicsEngine::CollisionMask collisionMask, Float dynamicFriction, Float staticFriction, Float restitution, Float velocityDamp, bool disableGravity, Float radius, bool gpuSimulation );

	Bool GetParticleOutput( Uint32 actorIndex, Uint32 shapeIndex, Vector3* position, Vector3* velocity, Bool* colliding = nullptr );

	Bool RefreshParticle( Uint32 systemIndex, Uint32 particleIndex, const Vector3& position, const Vector3& velocity );
	Bool ApplyVelocity( Uint32 systemIndex, Uint32 particleIndex, const Vector3& velocity );

	void FlushInputOutput( Uint32 actorIndex, Uint32 load );
	
	Bool PopCachedCollision( Uint32 systemIndex, Vector3* position = nullptr );
	Bool PushCachedCollision( Uint32 systemIndex, const Vector3& position );
	Uint32 GetCachedCollisionCount( Uint32 systemIndex );
};