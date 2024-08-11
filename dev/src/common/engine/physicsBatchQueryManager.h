/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../core/task.h"
#include "../core/sortedmap.h"

#include "../physics/physicsEngine.h"
#include "../physics/physicalCallbacks.h"

struct SRaycastHitResult
{
	DECLARE_RTTI_STRUCT( SRaycastHitResult );

	Vector	m_position;
	Vector	m_normal;
	Float	m_distance;

	CPhysicsWrapperInterface* m_wrapper;
	SPhysicalMaterial* m_physicalMaterial;
	SActorShapeIndex m_actorShapeIndex;

	THandle< CComponent >	m_component;
};

BEGIN_CLASS_RTTI( SRaycastHitResult )
	PROPERTY( m_position )
	PROPERTY( m_normal )
	PROPERTY( m_distance )
	PROPERTY( m_component )
END_CLASS_RTTI()

struct SSweepHitResult
{
	DECLARE_RTTI_STRUCT( SSweepHitResult );

	Vector	m_position;
	Vector	m_normal;
	Float	m_distance;

	THandle< CComponent >	m_component;
};

BEGIN_CLASS_RTTI( SSweepHitResult )
	PROPERTY( m_position )
	PROPERTY( m_normal )
	PROPERTY( m_distance )
	PROPERTY( m_component )
END_CLASS_RTTI()

#ifndef RED_FINAL_BUILD
struct SRaycastDebugData
{
	Vector				m_position;
	Vector				m_target;
	TDynArray< Vector >	m_hits;
};
#endif

class CJobPhysicsBatchQuery : public CTask
{
public:

	static const Uint32		MAX_RAYCAST_HITS	= 512;
	static const Uint32		MAX_SWEEP_HITS		= 128;

	typedef Int32 TJobId;
	typedef Int32 TQueryIndex;
	static const Int32 INVALID_JOB_ID = -1;
	static const Int32 INVALID_QUERY_INDEX = -1;

private:

	template < typename T >
	struct BaseJobQuery
	{
		Vector							m_startPos;
		Vector							m_direction;
		Float							m_distance;
		CPhysicsEngine::CollisionMask	m_collisionType;
		CPhysicsEngine::CollisionMask	m_collisionGroup;
		Uint16							m_collisionFlags;
		Uint16							m_flags;
		void*							m_userData;
		TDynArray< T >					m_results;
		Bool							m_notProcessed;
	};

	struct RaycastJobQuery : public BaseJobQuery< SRaycastHitResult >
	{
	};

	struct SweepJobQuery : public BaseJobQuery< SSweepHitResult >
	{
		Float	m_radius;
	};

	CPhysicsWorld* const			m_world;
	TJobId							m_jobId;
	EngineTime						m_creationTime;
	TDynArray< RaycastJobQuery >	m_raycastQueries;
	TDynArray< SweepJobQuery >		m_sweepQueries;

public:

	CJobPhysicsBatchQuery( CPhysicsWorld* world );

	RED_INLINE TJobId GetId() const
	{
		return m_jobId;
	}

	RED_INLINE const EngineTime& GetCreationTime() const
	{
		return m_creationTime;
	}

	RED_INLINE Uint32 GetTotalQueriesCount() const
	{
		return m_raycastQueries.Size() + m_sweepQueries.Size();
	}

	// RAYCAST

	RED_INLINE Uint32 GetRaycastQueriesCount() const
	{
		return m_raycastQueries.Size();
	}

	RED_INLINE void ReserveRaycastQueries( Uint32 count )
	{
		m_raycastQueries.Reserve( count );
	}
	
	RED_INLINE void* GetRaycastUserData( TQueryIndex i ) const
	{
		ASSERT( i != INVALID_QUERY_INDEX && static_cast< Uint32 >( i ) < m_raycastQueries.Size() );
		return m_raycastQueries[i].m_userData;
	}

	Bool GetRaycastResult( TQueryIndex i, TDynArray< SRaycastHitResult >& result ) const;
#ifndef RED_FINAL_BUILD
	void GetDebugData( TQueryIndex i, SRaycastDebugData& debugData ) const;
#endif

	TQueryIndex SubmitRaycastQuery( const Vector& startPos, const Vector& direction, Float distance, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup, Uint16 collisionFlags, Uint16 queryFlags, void* userData );

	// SWEEP

	RED_INLINE Uint32 GetSweepQueriesCount() const
	{
		return m_sweepQueries.Size();
	}

	RED_INLINE void ReserveSweepQueries( Uint32 count )
	{
		m_sweepQueries.Reserve( count );
	}

	RED_INLINE void* GetSweepUserData( TQueryIndex i ) const
	{
		ASSERT( i != INVALID_QUERY_INDEX && static_cast< Uint32 >( i ) < m_sweepQueries.Size() );
		return m_sweepQueries[i].m_userData;
	}

	Bool GetSweepResult( TQueryIndex i, TDynArray< SSweepHitResult >& result ) const;

	TQueryIndex SubmitSweepQuery( const Vector& startPos, Float radius, const Vector& direction, Float distance, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup, Uint16 collisionFlags, Uint16 queryFlags, void* userData );

protected:

	//! Process the job, is called from job thread
	virtual void Run();

	static TJobId s_nextJobId;

public:
#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const { return TXT("PhysicsBatchQuery"); }
	virtual Uint32 GetDebugColor() const { return Color::CYAN.ToUint32(); }
#endif

};

//////////////////////////////////////////////////////////////////////////

enum EBatchQueryState
{
	BQS_NotFound, // not found or timed out
	BQS_NotReady,
	BQS_Processed
};

BEGIN_ENUM_RTTI( EBatchQueryState )
	ENUM_OPTION( BQS_NotFound )
	ENUM_OPTION( BQS_NotReady )
	ENUM_OPTION( BQS_Processed )
END_ENUM_RTTI()

class CScriptBatchQueryAccessor;
class CPhysicsBatchQueryManager : public Red::System::NonCopyable
{
public:

	static CPhysicsEngine::CollisionMask	DEFAULT_COLLISION_MASK;
	static Uint16							DEFAULT_QUERY_FLAGS;

	typedef	CJobPhysicsBatchQuery::TJobId		TJobId;
	typedef	CJobPhysicsBatchQuery::TQueryIndex	TQueryIndex;

	struct SQueryId
	{
		TJobId		m_jobId;
		TQueryIndex	m_queryIndex;

		SQueryId()
			: m_jobId( CJobPhysicsBatchQuery::INVALID_JOB_ID )
			, m_queryIndex( CJobPhysicsBatchQuery::INVALID_QUERY_INDEX )
		{}

		SQueryId( TJobId jobId, TQueryIndex queryIndex )
			: m_jobId( jobId )
			, m_queryIndex( queryIndex )
		{}

		RED_FORCE_INLINE Bool IsValid() const
		{
			return !( m_jobId == CJobPhysicsBatchQuery::INVALID_JOB_ID || m_queryIndex == CJobPhysicsBatchQuery::INVALID_QUERY_INDEX );
		}

		static SQueryId	INVALID;
	};

	CPhysicsBatchQueryManager( CPhysicsWorld* world );
	~CPhysicsBatchQueryManager();

	void Tick( Float timeDelta );

	RED_INLINE CScriptBatchQueryAccessor* GetScriptAccessor() const { return m_scriptAccessor; }
	RED_INLINE CPhysicsWorld* GetPhysicsWorld() const { return m_world; }

	// RAYCAST
	SQueryId SubmitRaycastQuery( const Vector& startPos, const Vector& endPos, CPhysicsEngine::CollisionMask collisionType = DEFAULT_COLLISION_MASK, CPhysicsEngine::CollisionMask collisionGroup = 0, Uint16 collisionFlags = 0, Uint16 queryFlags = DEFAULT_QUERY_FLAGS, void* userData = nullptr );
	SQueryId SubmitRaycastQuery( const Vector& startPos, const Vector& direction, Float distance, CPhysicsEngine::CollisionMask collisionType = DEFAULT_COLLISION_MASK, CPhysicsEngine::CollisionMask collisionGroup = 0, Uint16 collisionFlags = 0, Uint16 queryFlags = DEFAULT_QUERY_FLAGS, void* userData = nullptr );

	EBatchQueryState	GetRaycastQueryState( const SQueryId& queryId, TDynArray< SRaycastHitResult >& result ) const;
	void*				GetRaycastUserData( const SQueryId& queryId ) const;
#ifndef RED_FINAL_BUILD
	Bool				GetRaycastDebugData( const SQueryId& queryId, SRaycastDebugData& debugData ) const;
#endif

	// SWEEP

	SQueryId SubmitSweepQuery( const Vector& startPos, Float radius, const Vector& endPos, CPhysicsEngine::CollisionMask collisionType = DEFAULT_COLLISION_MASK, CPhysicsEngine::CollisionMask collisionGroup = 0, Uint16 collisionFlags = 0, Uint16 queryFlags = DEFAULT_QUERY_FLAGS, void* userData = nullptr );
	SQueryId SubmitSweepQuery( const Vector& startPos, Float radius, const Vector& direction, Float distance, CPhysicsEngine::CollisionMask collisionType = DEFAULT_COLLISION_MASK, CPhysicsEngine::CollisionMask collisionGroup = 0, Uint16 collisionFlags = 0, Uint16 queryFlags = DEFAULT_QUERY_FLAGS, void* userData = nullptr );

	EBatchQueryState	GetSweepQueryState( const SQueryId& queryId, TDynArray< SSweepHitResult >& result ) const;
	void*				GetSweepUserData( const SQueryId& queryId ) const;
	//Bool				GetSweepDebugData( const SQueryId& queryId, SSweepDebugData& debugData ) const;

private:

	void PepareCurrentJob();
	void SubmitCurrentJob();
	void RunJobs();
	void RemoveTimedOutJobs();

	static const Uint32						MIN_QUERIES_PER_JOB;
	static const Uint32						MAX_QUERIES_PER_JOB;
	static const Uint32						MAX_JOBS_TO_RUN_PER_FRAME;
	static const Float						MAX_TIME_TO_SUBMIT_JOB;
	static const Float						JOB_TIME_OUT;

	CPhysicsWorld* const					m_world;

	typedef TSortedMap< TJobId, CJobPhysicsBatchQuery* > TJobsMap;

	TJobsMap								m_jobsToRun;
	TJobsMap								m_jobsReady;
	CJobPhysicsBatchQuery*					m_currentJob;

	CScriptBatchQueryAccessor*				m_scriptAccessor;
};

struct SScriptRaycastId
{
	DECLARE_RTTI_STRUCT( SScriptRaycastId );

	CPhysicsBatchQueryManager::SQueryId m_queryId;
};

BEGIN_CLASS_RTTI( SScriptRaycastId )
END_CLASS_RTTI()

struct SScriptSweepId
{
	DECLARE_RTTI_STRUCT( SScriptSweepId );

	CPhysicsBatchQueryManager::SQueryId m_queryId;
};

BEGIN_CLASS_RTTI( SScriptSweepId )
END_CLASS_RTTI()

class CScriptBatchQueryAccessor : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( CScriptBatchQueryAccessor )

	friend class CPhysicsBatchQueryManager;

	typedef CPhysicsBatchQueryManager::SQueryId SQueryId;

private:
	CPhysicsBatchQueryManager*	m_manager;

	TSortedMap< Uint32, CPhysicsBatchQueryManager::SQueryId > m_latentScriptQueries;

	Bool RayCastSync( Vector startPos, Vector endPos, const CPhysicsEngine::CollisionMask& mask, TDynArray< SRaycastHitResult > & hitResults ) const;

	void funcRayCast( CScriptStackFrame& stack, void* result );
	void funcRayCastSync( CScriptStackFrame& stack, void* result );
	void funcRayCastAsync( CScriptStackFrame& stack, void* result );
	void funcRayCastDir( CScriptStackFrame& stack, void* result );
	void funcRayCastDirSync( CScriptStackFrame& stack, void* result );
	void funcRayCastDirAsync( CScriptStackFrame& stack, void* result );
	void funcGetRayCastState( CScriptStackFrame& stack, void* result );

	void funcSweep( CScriptStackFrame& stack, void* result );
	void funcSweepDir( CScriptStackFrame& stack, void* result );
	void funcSweepAsync( CScriptStackFrame& stack, void* result );
	void funcSweepDirAsync( CScriptStackFrame& stack, void* result );
	void funcGetSweepState( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CScriptBatchQueryAccessor )
	PARENT_CLASS( IScriptable )
	NATIVE_FUNCTION( "RayCast", funcRayCast )
	NATIVE_FUNCTION( "RayCastSync", funcRayCastSync )
	NATIVE_FUNCTION( "RayCastAsync", funcRayCastAsync )
	NATIVE_FUNCTION( "RayCastDir", funcRayCastDir )
	NATIVE_FUNCTION( "RayCastDirSync", funcRayCastDir )
	NATIVE_FUNCTION( "RayCastDirAsync", funcRayCastDirAsync )
	NATIVE_FUNCTION( "GetRayCastState", funcGetRayCastState )
	NATIVE_FUNCTION( "Sweep", funcSweep )
	NATIVE_FUNCTION( "SweepAsync", funcSweepAsync )
	NATIVE_FUNCTION( "SweepDir", funcSweepDir )
	NATIVE_FUNCTION( "SweepDirAsync", funcSweepDirAsync )
	NATIVE_FUNCTION( "GetSweepState", funcGetSweepState )
END_CLASS_RTTI()
