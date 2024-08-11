/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

//////////////////////////////////////////////////////////////////////////////////////

#include "../physics/physicalCallbacks.h"
#include "physicsEngine.h"
#include "../physics/physicsWrapperPool.h"
#include "physicsWorldInvalidAreaCache.h"
#include "../physics/PhysicsWrappersDefinition.h"

struct SPhysicsContactInfo;
struct SPhysicsOverlapInfo;
class CPhysicsTraceCallback;
class ICustomTraceCallback;
class CRenderCamera;

enum ETraceReturnValue
{
	TRV_NoHit,								// Returned when no hits occured in the trace
	TRV_Hit,								// Returned when we recorded a hit in the trace. Number of hits passed in
											// the parameter for traces with multiple results
	TRV_ProcessedWhileFetch,				// Returned when trace wasn't fired because scene was in processing
	TRV_BufferOverflow,						// Returned when we recorded hits but trace didn't fully complete because of hit buffer overflow
	TRV_InvalidTransform					// Returned when the transform for a geometry test is invalid
};

#ifndef NO_EDITOR

struct SPhysicsDebugLine
{
	Vector	m_start;
	Vector	m_end;
	Color	m_color;

	SPhysicsDebugLine( Vector start, Vector end, Color color )
	{
		m_start = start;
		m_end = end;
		m_color = color;
	}
};

#endif

class IPhysicsWorldParentProvider
{
public:

	virtual ~IPhysicsWorldParentProvider() {}

	virtual const Vector& GetCameraPosition() const = 0;
	virtual const Vector& GetCameraForward() const = 0;
	virtual const Vector& GetCameraUp() const = 0;

	virtual Vector GetWindAtPoint( const Vector& point ) = 0;
	virtual void GetWindAtPoint( Uint32 elementsCount, void* inputPos, void* outputPos, size_t stride ) const = 0;

	virtual Bool IsWaterAvailable() const = 0;
	virtual Float GetWaterLevel( const Vector &point, Uint32 lodApproximation = 3, Float* heightDepth = 0 ) const = 0;
	virtual Bool GetWaterLevelBurst( Uint32 elementsCount, void* inputPos, void* outputPos, void* outputHeightDepth, size_t stride, Vector referencePosition, Bool useApproximation = true ) const = 0;

	virtual Bool GetVisibilityQueryResult( Uint32 elementsCount, Int16* indexes, void* inputPos, void* outputPos, size_t stride ) const = 0;

};

//////////////////////////////////////////////////////////////////////////////////////
/// Base class for physics world
class CPhysicsWorld
{
protected:
	friend class CPhysicsEngine;
	friend class CDebugWindowPhysicsArea;

	static CPhysicsWorld* m_top;
	CPhysicsWorld* m_next;

	Red::Threads::CAtomic< Int32 >	m_ref;
	IPhysicsWorldParentProvider*							m_worldParentProvider;
	Vector													m_upVector;						//!< World up vector in world space
	Vector													m_gravityVector;				//!< Gravity vector
	Float													m_currentDelta;
	Float													m_deltaAcumulator;
	Uint32													m_deltaAcumulatorFrameCount;
	Vector													m_position;						//!< Reference position for streaming and other stuff
	Bool													m_movementLocked;				//!< All movement is locked (used during loading screens)
	Bool													m_positionValid;				//!< Reference position was set at least once
	Bool													m_viewValid;					//!< Reference camera view matrix is valid
	Matrix													m_viewMatrix;					//!< Reference camera view matrix
	Bool													m_stuffAdded;
	Uint64*													m_area;
	Uint32													m_areaNumTilesPerEdge;
	Float													m_areaSize;
	Vector2													m_areaCornerPosition;	
	Uint32													m_clipMapSize;
	Uint32													m_areaRes;

	TDynArray< class CPhysicsContactListener* >				m_contactListeners;

	Red::Threads::CAtomic< class CPhysicsTileWrapper* >*	m_tilesCache;
	
	CPhysicsWorldInvalidAreaCache	m_invalidArea;

	struct SObjectHandleNameComponentHandle
	{
		THandle< IScriptable > m_eventReceiverObject;
		CName m_onEventName;
		THandle< IScriptable > m_otherObject;
		SActorShapeIndex m_index;

		SObjectHandleNameComponentHandle() {}
		SObjectHandleNameComponentHandle( const THandle< IScriptable >& receiverObject, const CName& onEventName, THandle< IScriptable > otherObject, const SActorShapeIndex& index ) : m_eventReceiverObject( receiverObject ), m_onEventName( onEventName ), m_otherObject( otherObject ), m_index( index ) {}
	};
	TDynArray< SObjectHandleNameComponentHandle > m_receivedEvents;

	CPhysicsWorld( IPhysicsWorldParentProvider* parentProvider, Uint32 areaNumTilesPerEdge, Float areaSize, const Vector2& areaCornerPosition, Uint32 clipSize, Uint32 areaRes );
	virtual ~CPhysicsWorld();

	Bool IsToDelete() const { return m_worldParentProvider == 0; }

	template< typename T1, typename T2, typename T3 > 
	RED_FORCE_INLINE void TickToRemove( TWrappersPool< T1, T2 >* pool, T3* toRelease, Uint32 limiter )
	{
		Uint8* wrapperFrontPtr = ( Uint8* ) pool->GetWrapperFront();
		Uint64 wrapperSize = pool->GetWrapperSize();
		T2* context = pool->GetContextFront();
		pool->GetWrappersRemoveQue().Swap();
		Int32 currentIndex = pool->GetWrappersRemoveQue().Get();
		while( currentIndex != -1 )
		{
			T1* wrapper = ( T1* ) ( currentIndex * wrapperSize + wrapperFrontPtr );
			if( wrapper->m_poolIndex >= 0 )
			{
				if( !limiter )
				{
					pool->PushWrapperToRemove( wrapper );
				}
				else
				{
					if ( !wrapper->MakeReadyToDestroy( toRelease ) )
					{
						pool->PushWrapperToRemove( wrapper );
					}
					else
					{
						T2* position = context + currentIndex;
						position->m_desiredDistanceSquared = -1.0f;
						pool->Destroy( wrapper );
					}
					--limiter;
				}
			}
			else
			{
				int a = 0;
			}
			currentIndex = pool->GetWrappersRemoveQue().Get();
		}
	}

	template< typename T1, typename T2, typename T3 > 
	RED_FORCE_INLINE Bool TickContext( TWrappersPool< T1, T2 >* pool, const Vector& viewPortPosition, Float timeDelta, T3* visibilityQueryDataProvider )
	{
		T2* context = pool->GetContextFront();
		pool->GetWrappersDirtyQue().Swap();
		Int32 currentDirtyIndex = pool->GetWrappersDirtyQue().Get();
		while( currentDirtyIndex != -1 )
		{
			T2* dirtyContext = context + currentDirtyIndex;
			dirtyContext->m_requestProcessingFlag = true;
			currentDirtyIndex = pool->GetWrappersDirtyQue().Get();
		}

		float x = viewPortPosition.X;
		float y = viewPortPosition.Y;
		TDynArray< Int16 >& que = pool->GetWrappersSimulationQue();
		que.ClearFast();
		Uint32 maxIndex = pool->GetWrapperMaxIndex();
		for( Uint32 i = 0; i != maxIndex; ++i )
		{
			if( context->m_desiredDistanceSquared >= 0 )
			{
				Bool shouldProcess = context->m_requestProcessingFlag;
				context->m_requestProcessingFlag = false;
				shouldProcess |= context->m_resultDistanceSquared <= context->m_desiredDistanceSquared;
				Float pos = context->m_x - x;
				context->m_resultDistanceSquared = pos * pos;
				pos = context->m_y - y;
				context->m_resultDistanceSquared += pos * pos;
				shouldProcess |= context->m_resultDistanceSquared <= context->m_desiredDistanceSquared;
				if( shouldProcess )
				{
					que.PushBack( i );
				}
			}
			++context;
		}
		context = pool->GetContextFront();
		visibilityQueryDataProvider->GetVisibilityQueryResult( que.Size(), que.TypedData(), &context->m_visibilityQueryId, &context->m_visibilityQueryResult, sizeof( T2 ) );
		return !que.Empty();
	}

	template< typename T1, typename T2, typename T3 > 
	RED_FORCE_INLINE void TickPreSimulation( TWrappersPool< T1, T2 >* pool, const Vector& viewPortPosition, Float timeDelta, Uint64 tickMarker, T3* toAdd, T3* toRemove, T3* toRelease, Bool allowAdd )
	{
		Uint16 simulationCount = pool->GetWrappersSimulationQue().Size();
		if( !simulationCount ) return;
		T2* context = pool->GetContextFront();
		Uint8* wrapperFrontPtr = ( Uint8* ) pool->GetWrapperFront();
		Uint64 wrapperSize = pool->GetWrapperSize();
		Int16* simulationIndex = &pool->GetWrappersSimulationQue()[ 0 ];
		for( Uint32 i = 0; i != simulationCount; ++i )
		{
			T1* currentWrapper = ( T1* ) ( *simulationIndex * wrapperSize + wrapperFrontPtr );
			T2* currentContext = context + *simulationIndex;
			currentWrapper->PreSimulation( currentContext, timeDelta, tickMarker, toAdd, toRemove, toRelease, allowAdd );
			++simulationIndex;
		}
	}

	template< typename T1, typename T2 >
	RED_FORCE_INLINE void TickPostSimulation( TWrappersPool< T1, T2 >* pool )
	{
		Uint16 simulationCount = pool->GetWrappersSimulationQue().Size();
		if( !simulationCount ) return;
		T2* context = pool->GetContextFront();
		Uint8* wrapperFrontPtr = ( Uint8* ) pool->GetWrapperFront();
		Uint64 wrapperSize = pool->GetWrapperSize();
		Int16* simulationIndex = &pool->GetWrappersSimulationQue()[ 0 ];
		for( Uint32 i = 0; i != simulationCount; ++i )
		{
			T1* currentWrapper = ( T1* ) ( *simulationIndex * wrapperSize + wrapperFrontPtr );
			T2* currentContext = context + *simulationIndex;
			currentWrapper->PostSimulation( currentContext->m_visibilityQueryResult );
			++simulationIndex;
		}
	}

	char m_wrapperPools[ EPhysicsWrappers::EPW_MAX ];

//this is temporary place to hold somewhere controllers manager, which wont be used soon
	void* m_custom;
//this is temporary place to hold somewhere controllers manager, which wont be used soon

public:
	template< class T1,	class T2 >
	TWrappersPool< T1, T2 >* GetWrappersPool();

public:
	// Raycasts
	virtual ETraceReturnValue RayCastWithSingleResult( const Vector& from, const Vector& to, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo& outContactInfo ) { return TRV_NoHit; }
	virtual ETraceReturnValue RayCastWithMultipleResults( const Vector& from, const Vector& to, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos ) { return TRV_NoHit; }
	
	// Sweep tests
	virtual ETraceReturnValue SweepTestAnyResult( const Vector& from, const Vector& to, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude ) { return TRV_NoHit; }
	virtual ETraceReturnValue SweepTestWithSingleResult( const Vector& from, const Vector& to, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo& outContactInfo, Bool precise = false ) { return TRV_NoHit; }
	virtual ETraceReturnValue CapsuleSweepTestWithSingleResult( const Vector& from, const Vector& to, const Float radius, const Float height, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo& outContactInfo, Bool usePreciseSweep = false ) { return TRV_NoHit; }
	virtual ETraceReturnValue BoxSweepTestWithSingleResult( const Vector& from, const Vector& to, const Vector3& halfExtents, const Matrix& rotation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo& contactInfo, Bool usePreciseSweep = false ) { return TRV_NoHit; }
	virtual ETraceReturnValue SphereSweepTestWithMultipleResults( const Vector& from, const Vector& to, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos ) { return TRV_NoHit; }
	// This test requires an ICustomTraceCallback that processes hits on the fly, without reporting them to be stored and returned. Returns TRV_Hit if all went fine, TRV_ProcessedWhileFetch means a trace wasn't completed.
	virtual ETraceReturnValue SphereSweepTestWithCustomCallbackNoResults( const Vector& from, const Vector& to, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, ICustomTraceCallback* customCallback) { return TRV_NoHit; }	
	virtual ETraceReturnValue CapsuleSweepTestWithMultipleResults( const Vector& from, const Vector& to, const Float radius, const Float height, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos, CPhysicsTraceCallback* traceCallback = nullptr, Bool usePerciseSweep = false ) { return TRV_NoHit; }
	virtual ETraceReturnValue BoxSweepTestWithMultipleResults( const Vector& from, const Vector& to, const Vector3& halfExtents, const Matrix& rotation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos, CPhysicsTraceCallback* traceCallback = nullptr, Bool usePreciseSweep = false ) { return TRV_NoHit; }

	
	// Overlaps
	virtual ETraceReturnValue SphereOverlapWithAnyResult( const Vector& position, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo& outContactInfo ) { return TRV_NoHit; }
	virtual ETraceReturnValue BoxOverlapWithAnyResult( const Vector& position, const Vector3& halfExtents, const Matrix& orientation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo& outContactInfo ) { return TRV_NoHit; }
	virtual ETraceReturnValue BoxOverlapWithMultipleResults( const Vector& position, const Vector3& halfExtents, const Matrix& orientation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos ) { return TRV_NoHit; }
	virtual ETraceReturnValue BoxOverlapWithWithCustomCallbackNoResults( const Vector& position, const Vector3& halfExtents, const Matrix& orientation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, ICustomTraceCallback* customCallback ) { return TRV_NoHit; }
	virtual ETraceReturnValue CapsuleOverlapWithAnyResult( const Vector& position, const Float radius, const Float height, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo& outContactInfo ) { return TRV_NoHit; }
	virtual ETraceReturnValue SphereOverlapWithMultipleResults( const Vector& position, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos ) { return TRV_NoHit; }

	virtual class CPhysicsWorldBatchQuery* CreateBatchTrace( Uint32 raycastPrealocate, Uint32 maxRaycastHits = 1, Uint32 sweepPrealocate = 0, Uint32 maxSweepHits = 1 ) { return 0; }

public:

	virtual bool Init( Bool criticalPriority, Bool allowGpu, Bool useMemorySettings ) { return false; }
	virtual void StartNextFrameSimulation( Float timeDelta, Float timeScale, Bool blankFrame ) {}
	virtual void FetchCurrentFrameSimulation( Bool async ) {}
	virtual void CompleteCurrentFrameSimulation() {}
	virtual void TickRemovalWrappers( bool force, TDynArray< void* >* toRemove = nullptr );
	virtual void PreSimulateWrappers( Float timeDelta, Bool blankFrame, TDynArray< void* >* toAdd = nullptr, TDynArray< void* >* toRemove = nullptr, TDynArray< void* >* toRelease = nullptr );
	virtual void PostSimulateWrappers();
	virtual Bool HasWrappers();

	virtual void SetWhileSceneProcessFlag( bool flag ) {}

	// setup reference position for streaming and other calculations
	void SetReferencePosition( const Vector& position );

	// setup reference position for streaming and other calculations
	const Vector& GetReferencePosition() { return m_position; }

	// setup reference view (for APEX)
	void SetRenderingView( const Matrix& viewMatrix );

	void AddRef();
	virtual CPhysicsWorld* ReleaseRef();

    RED_INLINE const Vector& GetUpVector() const { return m_upVector; }
    RED_INLINE const Vector& GetGravityVector() const { return m_gravityVector; }

	RED_INLINE Float GetCurrentDelta() const { return m_currentDelta; }

	RED_INLINE CPhysicsWorldInvalidAreaCache& GetInvalidAreaCache() { return m_invalidArea; }
	RED_INLINE const CPhysicsWorldInvalidAreaCache& GetInvalidAreaCache() const { return m_invalidArea; }

#ifndef NO_EDITOR
	virtual void AddPlane( float height ) {}

	virtual void	SetDebuggingVisualizationBox( Box cullingBox ) {}
	virtual void*	GetDebugTriangles( Uint32& trianglesBufferSize ) { return nullptr; }
	virtual void*	GetDebugLines( Uint32& linesBufferSize ) { return nullptr; }
	virtual void	GetDebugVisualizationForMaterials(const Vector position, TDynArray< SPhysicsDebugLine >& debugLines ) {}
#endif

	void PushContactListener( class CPhysicsContactListener* contactListener ) { m_contactListeners.PushBack( contactListener ); }

	void PushScriptEvent( const THandle< IScriptable >& object, const CName& onEventName, THandle< IScriptable > otherObject, const SActorShapeIndex& index )
	{
		RED_ASSERT( object, TXT( "This may potentially lead to memory corruption" ) );
		m_receivedEvents.PushBack( SObjectHandleNameComponentHandle( object, onEventName, otherObject, index ) );
	}

	void SendReceivedScriptEvents();

	Bool IsAvaible( const Vector& pos, Bool checkInvalidAreas = true );

	Bool IsPositionInside( const Vector2& pos );

	Red::Threads::CAtomic< class CPhysicsTileWrapper* >* GetTerrainTileWrapperAtomic( const Vector2& pos );

	class CPhysicsTileWrapper* GetTerrainTileWrapper( const Vector2& pos );

	virtual const Vector& GetEyePosition() { return m_position; }

	IPhysicsWorldParentProvider* GetWorldParentProvider() const { return m_worldParentProvider; }

	Bool MarkSectorAsUnready( const Vector& pos );
	virtual void MarkSectorAsStuffAdded() {}

	// disable/enable global movement lock (mosltly for character controllers)
	// this affects values returned by IsTerrainPresentAt
	void ToggleMovementLock( const Bool isLocked );

	// check if all of the pending actors/collisions were created
	Bool IsAllPendingCollisionLoadedAndCreated() { return !m_stuffAdded;	}

protected:
	void CacheInvalidAreas();
};

////////////////////////////////////////////////////////////////////////////////////////////////

