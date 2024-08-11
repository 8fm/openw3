#pragma once

#include "physicsWorld.h"
#include "physXEngine.h"

#ifdef USE_PHYSX

#include "../core/uniqueBuffer.h"

//////////////////////////////////////////////////////////////////////////////////////
/// Callback for traces
class CPhysicsTraceCallback : public physx::PxQueryFilterCallback
{
public:
	physx::PxSceneQueryHitType::Enum m_hitType;

	CPhysicsTraceCallback()
	{
	}

	virtual physx::PxQueryHitType::Enum preFilter( const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags )
	{
		const physx::PxFilterData& shapeFilterData = shape->getQueryFilterData();
		Uint64 type = ( ( Uint64& ) shapeFilterData.word0 ) & 0x0000FFFFFFFFFFFF;
		Uint64 include = ( ( Uint64& ) filterData.word0 ) & 0x0000FFFFFFFFFFFF;
		Uint64 exclude = ( Uint64& ) filterData.word2;

		if( exclude )
		{
			if( include & type && !( exclude & type) )
			{
				return m_hitType;
			}
		}
		else if( include & type )
		{
			return m_hitType;
		}

		return  physx::PxSceneQueryHitType::eNONE;
	}

	virtual physx::PxQueryHitType::Enum postFilter( const physx::PxFilterData& filterData, const physx::PxQueryHit& hit )
	{
		return m_hitType;
	}
};

class ICustomTraceCallback : public physx::PxQueryFilterCallback
{
public:
	ICustomTraceCallback(){}

	virtual physx::PxQueryHitType::Enum preFilter( const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags ) = 0;
	virtual physx::PxQueryHitType::Enum postFilter( const physx::PxFilterData& filterData, const physx::PxQueryHit& hit ) = 0;
};




//////////////////////////////////////////////////////////////////////////////////////
/// Wrapper class for physics world
class CPhysicsWorldPhysXImpl : public CPhysicsWorld, public physx::PxSimulationEventCallback, public physx::PxContactModifyCallback
{
	friend class CPhysXEngine;
	friend class CApexWrapper;
protected:

	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

	CPhysicsTraceCallback					m_singleTraceCallback;
	CPhysicsTraceCallback					m_multiTraceCallback;
	physx::PxScene*							m_scene;
	Red::Threads::CAtomic< Uint32 >			m_whileSceneProcess;
	Red::Threads::CAtomic< Uint32 >			m_tracesProcessing;
	Red::Threads::CAtomic< Uint32 >			m_waitingForTasksProcessing;
	class CPhysxActorsAllocateReleaseTask*	m_backgroundTask;
	class CAsyncFetchTask*					m_asyncFetchTask;
	Bool									m_resultFromFetch;
	physx::PxTask*							m_completionTask;

#ifndef NO_EDITOR
	Box										m_debugVisualisationBox;
	physx::PxDebugLine*						m_debugLineBuffer;
	Uint32									m_debugLineBufferSize;
	physx::PxDebugTriangle*					m_debugTriangleBuffer;
	Uint32									m_debugTriangleBufferSize;
	physx::PxSimulationStatistics			m_statistics;
#endif

#ifdef USE_APEX
	void* m_apexInterface; 
#endif

	Red::UniqueBuffer m_physxBuffer;

	CPhysicsWorldPhysXImpl( IPhysicsWorldParentProvider* parentProvider, Uint32 areaResolution, Float areaSize, const Vector2& areaCornerPosition, Uint32 clipSize, Uint32 tileRes );
	virtual ~CPhysicsWorldPhysXImpl();

public:	
	RED_INLINE physx::PxScene* GetPxScene() { return m_scene; };

	virtual bool Init( Bool criticalPriority, Bool allowGpu, Bool useMemorySettings ) override;
	void StartNextFrameSimulation( Float timeDelta, Float timeScale, Bool blankFrame ) override;
	void FetchCurrentFrameSimulation( Bool async );
	void CompleteCurrentFrameSimulation();

public:
	Bool IsSceneWhileProcessing() const;
	void SceneUsageAddRef() { m_tracesProcessing.Increment(); }
	void SceneUsageRemoveRef() { m_tracesProcessing.Decrement(); }

	template < typename T >
	T* PushCompletionTask()
	{
		RED_FATAL_ASSERT( !m_completionTask, "post task where already stored" );
		if( m_completionTask )
		{
			return nullptr;
		}
		T* result = new T( this );
		m_completionTask  = result;
		return result;
	}

	Bool PopCompletionTask();

	virtual class CPhysicsWorldBatchQuery* CreateBatchTrace( Uint32 raycastPrealocate, Uint32 maxRaycastHits = 1, Uint32 sweepPrealocate = 0, Uint32 maxSweepHits = 1 ); 

	// Raycasts
	virtual ETraceReturnValue RayCastWithSingleResult( const Vector& from, const Vector& to, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo& outContactInfo ); 
	virtual ETraceReturnValue RayCastWithMultipleResults( const Vector& from, const Vector& to, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos ); 

	// Sweep tests
	virtual ETraceReturnValue SweepTestAnyResult( const Vector& from, const Vector& to, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude );
	virtual ETraceReturnValue SweepTestWithSingleResult( const Vector& from, const Vector& to, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo& outContactInfo, Bool precise = false );
	virtual ETraceReturnValue CapsuleSweepTestWithSingleResult( const Vector& from, const Vector& to, const Float radius, const Float height, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo& outContactInfo, Bool usePreciseSweep = false );
	virtual ETraceReturnValue BoxSweepTestWithSingleResult( const Vector& from, const Vector& to, const Vector3& halfExtents, const Matrix& rotation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo& contactInfo, Bool usePreciseSweep = false );
	virtual ETraceReturnValue SphereSweepTestWithMultipleResults( const Vector& from, const Vector& to, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos );
	virtual ETraceReturnValue SphereSweepTestWithCustomCallbackNoResults( const Vector& from, const Vector& to, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, ICustomTraceCallback* customCallback);
	virtual ETraceReturnValue CapsuleSweepTestWithMultipleResults( const Vector& from, const Vector& to, const Float radius, const Float height, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos, CPhysicsTraceCallback* traceCallback = nullptr, Bool usePreciseSweep = false );
	virtual ETraceReturnValue BoxSweepTestWithMultipleResults( const Vector& from, const Vector& to, const Vector3& halfExtents, const Matrix& rotation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos, CPhysicsTraceCallback* traceCallback = nullptr, Bool usePreciseSweep = false );

	// Overlap tests
	virtual ETraceReturnValue SphereOverlapWithAnyResult( const Vector& position, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo& outContactInfo ); 
	virtual ETraceReturnValue BoxOverlapWithAnyResult( const Vector& position, const Vector3& halfExtents, const Matrix& orientation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo& outContactInfo );
	virtual ETraceReturnValue BoxOverlapWithMultipleResults( const Vector& position, const Vector3& halfExtents, const Matrix& orientation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos ); 
	virtual ETraceReturnValue BoxOverlapWithWithCustomCallbackNoResults( const Vector& position, const Vector3& halfExtents, const Matrix& orientation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, ICustomTraceCallback* customCallback ); 
	virtual ETraceReturnValue CapsuleOverlapWithAnyResult( const Vector& position, const Float radius, const Float height, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo& outContactInfo );
	virtual ETraceReturnValue SphereOverlapWithMultipleResults( const Vector& position, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos ); 

#ifndef NO_EDITOR
	virtual void AddPlane( float height );
	physx::PxSimulationStatistics& GetStatistics() { return m_statistics; }

	virtual void SetDebuggingVisualizationBox( Box cullingBox );
	virtual void*	GetDebugTriangles( Uint32& trianglesBufferSize );
	virtual void*	GetDebugLines( Uint32& linesBufferSize );
	virtual void	GetDebugVisualizationForMaterials(const Vector position, TDynArray< SPhysicsDebugLine >& debugLines );
#endif
/*	template< class T1, class T2 >
	void DumpPhysicalComponentsNames( TWrappersPool< T1, T2 >* pool, Uint32& index );
	void DumpPhysicalComponentsNames( Uint32& index );
	static void DumpPhysicalComponentsNamesGlobaly();*/

	Red::Threads::CAtomic< Uint32 >* GetFetchStartSyncInterlock() { return &m_waitingForTasksProcessing; }

	void MarkSectorAsStuffAdded() { m_stuffAdded = true; }

private:
	void UpdateActiveBodies();

	virtual void TickRemovalWrappers( bool force, TDynArray< void* >* toRemove = nullptr ) override;
	virtual void PreSimulateWrappers( Float timeDelta, Bool blankFrame, TDynArray< void* >* toAdd = nullptr, TDynArray< void* >* toRemove = nullptr, TDynArray< void* >* toRelease = nullptr );
	virtual void PostSimulateWrappers();

	virtual void onContactModify(physx::PxContactModifyPair* const pairs, physx::PxU32 count);
	virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs);
	virtual void ProcessContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs);
	virtual void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count);
	virtual void ProcessTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count);
	virtual void onConstraintBreak(physx::PxConstraintInfo*, physx::PxU32);
	virtual void onWake(physx::PxActor** , physx::PxU32 );
	virtual void onSleep(physx::PxActor** , physx::PxU32 );
	virtual void SetWhileSceneProcessFlag( bool flag );

};

#endif
