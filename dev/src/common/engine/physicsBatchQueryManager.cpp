/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "component.h"
#include "physicsBatchQueryManager.h"
#include "../physics/physicsWorldPhysxImplBatchTrace.h"
#include "../physics/physicsWrapper.h"
#include "../physics/physicsWorldUtils.h"

#include "../core/scriptStackFrame.h"

IMPLEMENT_RTTI_CLASS( SRaycastHitResult );
IMPLEMENT_RTTI_CLASS( SSweepHitResult );

CJobPhysicsBatchQuery::TJobId	CJobPhysicsBatchQuery::s_nextJobId = CJobPhysicsBatchQuery::TJobId( 1 );

CJobPhysicsBatchQuery::CJobPhysicsBatchQuery( CPhysicsWorld* world )
	: m_world( world )
	, m_jobId( s_nextJobId++ )
	, m_creationTime( GGame->GetEngineTime() )
{
}

CJobPhysicsBatchQuery::TQueryIndex CJobPhysicsBatchQuery::SubmitRaycastQuery( const Vector& startPos, const Vector& direction, Float distance, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup, Uint16 collisionFlags, Uint16 queryFlags, void* userData )
{
	RaycastJobQuery query;
	query.m_startPos = startPos;
	query.m_direction = direction;
	query.m_distance = distance;
	query.m_collisionType = collisionType;
	query.m_collisionGroup = collisionGroup;
	query.m_collisionFlags = collisionFlags;
	query.m_flags = queryFlags;
	query.m_userData = userData;

	m_raycastQueries.PushBack( query );
	return static_cast< TQueryIndex >( m_raycastQueries.Size() - 1 );
}

CJobPhysicsBatchQuery::TQueryIndex CJobPhysicsBatchQuery::SubmitSweepQuery( const Vector& startPos, Float radius, const Vector& direction, Float distance, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup, Uint16 collisionFlags, Uint16 queryFlags, void* userData )
{
	SweepJobQuery query;
	query.m_startPos = startPos;
	query.m_radius = radius;
	query.m_direction = direction;
	query.m_distance = distance;
	query.m_collisionType = collisionType;
	query.m_collisionGroup = collisionGroup;
	query.m_collisionFlags = collisionFlags;
	query.m_flags = queryFlags;
	query.m_userData = userData;

	m_sweepQueries.PushBack( query );
	return static_cast< TQueryIndex >( m_sweepQueries.Size() - 1 );
}

void CJobPhysicsBatchQuery::Run()
{
	PC_SCOPE_PIX( CJobPhysicsBatchQuery_Run );

	CPhysicsWorldBatchQuery* batch = m_world->CreateBatchTrace( m_raycastQueries.Size(), MAX_RAYCAST_HITS, m_sweepQueries.Size(), MAX_SWEEP_HITS );
	for ( Uint32 i = 0, size = m_raycastQueries.Size(); i < size; ++i )
	{
		RaycastJobQuery& query = m_raycastQueries[i];
		SPhysicalFilterData filter( query.m_collisionType, query.m_collisionGroup, query.m_collisionFlags );
		batch->Raycast( query.m_startPos, query.m_direction, query.m_distance, filter, query.m_flags, reinterpret_cast< void* >( i ) );
		query.m_notProcessed = false;
	}

	for ( Uint32 i = 0, size = m_sweepQueries.Size(); i < size; ++i )
	{
		SweepJobQuery& query = m_sweepQueries[i];
		SPhysicalFilterData filter( query.m_collisionType, query.m_collisionGroup, query.m_collisionFlags );
		batch->Sweep( query.m_startPos, query.m_radius, query.m_direction, query.m_distance, filter, query.m_flags, reinterpret_cast< void* >( i ) );
		query.m_notProcessed = false;
	}

	Int8 processResult = batch->Process();

	if(processResult == -1)
	{
		for ( Uint32 i = 0, size = m_raycastQueries.Size(); i < size; ++i )
		{
			m_raycastQueries[i].m_notProcessed = true;
		}
		for ( Uint32 i = 0, size = m_sweepQueries.Size(); i < size; ++i )
		{
			m_sweepQueries[i].m_notProcessed = true;
		}
	}
	else
	{
		for ( Uint32 i = 0, size = batch->GetRaycastBufferUsedSize(); i < size; ++i )
		{
			Uint32 count = batch->GetRaycastResultHitCount( i );
			if( !count )
			{
				continue;
			}

			Uint32 queryIndex = ( Uint32 ) reinterpret_cast< uintptr_t >( batch->GetRaycastResultHitUserData( i ) );
			RaycastJobQuery& query = m_raycastQueries[queryIndex];

			for( Uint32 j = 0; j < count; ++j )
			{
				SRaycastHitResult result;

				if( batch->GetRaycastResultHitLocation( i, j, result.m_distance, result.m_position, result.m_normal ) )
				{
					CPhysicsWrapperInterface* wrapper = batch->GetRaycastResultHitWrapper( i, j );

					if ( wrapper != nullptr )
					{
						result.m_wrapper = wrapper;
						result.m_physicalMaterial = batch->GetRaycastResultHitPhysicalMaterial( i, j );

						SActorShapeIndex actorShapeIndex = batch->GetRaycastResultHitActorShapeIndex( i, j );
						result.m_actorShapeIndex = actorShapeIndex;

						CComponent* component = nullptr;
						wrapper->GetParent( component, static_cast< Uint32 >( actorShapeIndex.m_actorIndex ) );

						result.m_component = component;
					}

					query.m_results.PushBack( result );
				}
			}
		}

		for ( Uint32 i = 0, size = batch->GetSweepBufferUsedSize(); i < size; ++i )
		{
			Uint32 count = batch->GetSweepResultHitCount( i );
			if( !count )
			{
				continue;
			}

			Uint32 queryIndex = ( Uint32 ) reinterpret_cast< uintptr_t >( batch->GetSweepResultHitUserData( i ) );
			SweepJobQuery& query = m_sweepQueries[queryIndex];

			for( Uint32 j = 0; j < count; ++j )
			{
				SSweepHitResult result;

				if( batch->GetSweepResultHitLocation( i, j, result.m_distance, result.m_position, result.m_normal ) )
				{
					CPhysicsWrapperInterface* wrapper = batch->GetSweepResultHitWrapper( i, j );

					if ( wrapper != nullptr )
					{
						SActorShapeIndex actorShapeIndex = batch->GetRaycastResultHitActorShapeIndex( i, j );
						CComponent* component = nullptr;
						wrapper->GetParent( component, static_cast< Uint32 >( actorShapeIndex.m_actorIndex ) );

						result.m_component = component;
					}

					query.m_results.PushBack( result );
				}
			}
		}
	}
	delete batch;
}

Bool CJobPhysicsBatchQuery::GetRaycastResult( TQueryIndex i, TDynArray< SRaycastHitResult >& result ) const
{
	RED_ASSERT( i != INVALID_QUERY_INDEX && static_cast< Uint32 >( i ) < m_raycastQueries.Size() );
	RED_ASSERT( IsFinished() );


	result = m_raycastQueries[ i ].m_results;
	return !m_raycastQueries[ i ].m_notProcessed;
}

Bool CJobPhysicsBatchQuery::GetSweepResult( TQueryIndex i, TDynArray< SSweepHitResult >& result ) const
{
	RED_ASSERT( i != INVALID_QUERY_INDEX && static_cast< Uint32 >( i ) < m_sweepQueries.Size() );
	RED_ASSERT( IsFinished() );

	result = m_sweepQueries[ i ].m_results;
	return !m_sweepQueries[ i ].m_notProcessed;
}

#ifndef RED_FINAL_BUILD
void CJobPhysicsBatchQuery::GetDebugData( TQueryIndex i, SRaycastDebugData& debugData ) const
{
	RED_ASSERT( i != INVALID_QUERY_INDEX && static_cast< Uint32 >( i ) < m_raycastQueries.Size() );

	const RaycastJobQuery& query = m_raycastQueries [ i ];
	debugData.m_position = query.m_startPos;
	debugData.m_target = query.m_startPos + query.m_direction * query.m_distance;
	debugData.m_hits.Resize( query.m_results.Size() );
	for ( Uint32 i = 0; i < query.m_results.Size(); ++i )
	{
		debugData.m_hits[ i ] = query.m_results[ i ].m_position;
	}
}
#endif

//////////////////////////////////////////////////////////////////////////

CPhysicsBatchQueryManager::SQueryId	CPhysicsBatchQueryManager::SQueryId::INVALID	= CPhysicsBatchQueryManager::SQueryId();
const Uint32				CPhysicsBatchQueryManager::MIN_QUERIES_PER_JOB			= 10;
const Uint32				CPhysicsBatchQueryManager::MAX_QUERIES_PER_JOB			= NumericLimits<Uint32>::Max();
const Uint32				CPhysicsBatchQueryManager::MAX_JOBS_TO_RUN_PER_FRAME	= NumericLimits<Uint32>::Max();
const Float					CPhysicsBatchQueryManager::MAX_TIME_TO_SUBMIT_JOB		= 0.5f;
const Float					CPhysicsBatchQueryManager::JOB_TIME_OUT					= 1.0f; // including MAX_TIME_TO_SUBMIT_JOB

CPhysicsEngine::CollisionMask	CPhysicsBatchQueryManager::DEFAULT_COLLISION_MASK	= 0;
Uint16							CPhysicsBatchQueryManager::DEFAULT_QUERY_FLAGS		= 0;

CPhysicsBatchQueryManager::CPhysicsBatchQueryManager( CPhysicsWorld* world )
	: m_world( world )
	, m_currentJob( nullptr )
{
	DEFAULT_COLLISION_MASK = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );
	DEFAULT_QUERY_FLAGS = EBatchQueryQueryFlag::EQQF_IMPACT | EBatchQueryQueryFlag::EQQF_NORMAL | EBatchQueryQueryFlag::EQQF_DISTANCE;

	m_scriptAccessor = new CScriptBatchQueryAccessor;
	m_scriptAccessor->m_manager = this;
}

CPhysicsBatchQueryManager::~CPhysicsBatchQueryManager()
{
	delete m_scriptAccessor;

	TJobsMap::iterator itEnd = m_jobsToRun.End();
	for ( TJobsMap::iterator it = m_jobsToRun.Begin(); it != itEnd; ++it )
	{
		it->m_second->Release();
	}
	m_jobsToRun.ClearFast();

	itEnd = m_jobsReady.End();
	for ( TJobsMap::iterator it = m_jobsReady.Begin(); it != itEnd; ++it )
	{
		it->m_second->Release();
	}
	m_jobsReady.ClearFast();

	if( m_currentJob )
	{
		m_currentJob->Release();
	}
}

CPhysicsBatchQueryManager::SQueryId CPhysicsBatchQueryManager::SubmitRaycastQuery( const Vector& startPos, const Vector& endPos, CPhysicsEngine::CollisionMask collisionType /*= DEFAULT_COLLISION_MASK*/, CPhysicsEngine::CollisionMask collisionGroup /*= 0*/, Uint16 collisionFlags /*= 0*/, Uint16 queryFlags /*= DEFAULT_QUERY_FLAGS*/, void* userData /*= nullptr*/ )
{
	Vector direction = endPos - startPos;
	Float distance = direction.Normalize3();

	return SubmitRaycastQuery( startPos, direction, distance, collisionType, collisionGroup, collisionFlags, queryFlags, userData );
}

CPhysicsBatchQueryManager::SQueryId CPhysicsBatchQueryManager::SubmitRaycastQuery( const Vector& startPos, const Vector& direction, Float distance, CPhysicsEngine::CollisionMask collisionType /*= DEFAULT_COLLISION_MASK*/, CPhysicsEngine::CollisionMask collisionGroup /*= 0*/, Uint16 collisionFlags /*= 0*/, Uint16 queryFlags /*= DEFAULT_QUERY_FLAGS*/, void* userData /*= nullptr*/ )
{
	PepareCurrentJob();
	TJobId jobId = m_currentJob->GetId();
	TQueryIndex queryIndex = m_currentJob->SubmitRaycastQuery( startPos, direction, distance, collisionType, collisionGroup, collisionFlags, queryFlags, userData );
	if ( m_currentJob->GetTotalQueriesCount() >= MAX_QUERIES_PER_JOB )
	{
		SubmitCurrentJob();
	}
	return SQueryId( jobId, queryIndex );
}

EBatchQueryState CPhysicsBatchQueryManager::GetRaycastQueryState( const SQueryId& queryId, TDynArray< SRaycastHitResult >& result ) const
{
	if ( !queryId.IsValid() )
	{
		return BQS_NotFound;
	}

	TJobId jobId = queryId.m_jobId;
	if ( m_currentJob != nullptr && jobId == m_currentJob->GetId() )
	{
		return BQS_NotReady;
	}

	if ( m_jobsToRun.KeyExist( jobId ) )
	{
		return BQS_NotReady;
	}

	TJobsMap::const_iterator it = m_jobsReady.Find( jobId );
	if ( it == m_jobsReady.End() )
	{
		return BQS_NotFound;
	}
	if ( !it->m_second->IsFinished() || !(it->m_second->GetRaycastResult( queryId.m_queryIndex, result )))
	{
		return BQS_NotReady;
	}
	
	
	return BQS_Processed;
}

void* CPhysicsBatchQueryManager::GetRaycastUserData( const SQueryId& queryId ) const
{
	if( queryId.IsValid() )
	{
		const TJobId jobId = queryId.m_jobId;
		TJobsMap::const_iterator it = m_jobsReady.Find( jobId );
		if( it != m_jobsReady.End() )
		{
			return it->m_second->GetRaycastUserData( queryId.m_queryIndex );
		}

		it = m_jobsToRun.Find( jobId );
		if( it != m_jobsToRun.End() )
		{
			return it->m_second->GetRaycastUserData( queryId.m_queryIndex );
		}

		if ( m_currentJob != nullptr && jobId == m_currentJob->GetId() )
		{
			return m_currentJob->GetRaycastUserData( queryId.m_queryIndex );
		}
	}

	return nullptr;
}

#ifndef RED_FINAL_BUILD
Bool CPhysicsBatchQueryManager::GetRaycastDebugData( const SQueryId& queryId, SRaycastDebugData& debugData ) const
{
	if ( queryId.IsValid() )
	{
		const TJobId jobId = queryId.m_jobId;
		TJobsMap::const_iterator it = m_jobsReady.Find( jobId );
		if( it != m_jobsReady.End() )
		{
			it->m_second->GetDebugData( queryId.m_queryIndex, debugData );
			return true;
		}

		it = m_jobsToRun.Find( jobId );
		if( it != m_jobsToRun.End() )
		{
			it->m_second->GetDebugData( queryId.m_queryIndex, debugData );
			return true;
		}

		if ( m_currentJob != nullptr && jobId == m_currentJob->GetId() )
		{
			m_currentJob->GetDebugData( queryId.m_queryIndex, debugData );
			return true;
		}
	}
	return false;
}
#endif

CPhysicsBatchQueryManager::SQueryId CPhysicsBatchQueryManager::SubmitSweepQuery( const Vector& startPos, Float radius, const Vector& endPos, CPhysicsEngine::CollisionMask collisionType /*= DEFAULT_COLLISION_MASK*/, CPhysicsEngine::CollisionMask collisionGroup /*= 0*/, Uint16 collisionFlags /*= 0*/, Uint16 queryFlags /*= DEFAULT_QUERY_FLAGS*/, void* userData /*= nullptr*/ )
{
	Vector direction = endPos - startPos;
	Float distance = direction.Normalize3();

	return SubmitSweepQuery( startPos, radius, direction, distance, collisionType, collisionGroup, collisionFlags, queryFlags, userData );
}

CPhysicsBatchQueryManager::SQueryId CPhysicsBatchQueryManager::SubmitSweepQuery( const Vector& startPos, Float radius, const Vector& direction, Float distance, CPhysicsEngine::CollisionMask collisionType /*= DEFAULT_COLLISION_MASK*/, CPhysicsEngine::CollisionMask collisionGroup /*= 0*/, Uint16 collisionFlags /*= 0*/, Uint16 queryFlags /*= DEFAULT_QUERY_FLAGS*/, void* userData /*= nullptr*/ )
{
	PepareCurrentJob();
	TJobId jobId = m_currentJob->GetId();
	TQueryIndex queryIndex = m_currentJob->SubmitSweepQuery( startPos, radius, direction, distance, collisionType, collisionGroup, collisionFlags, queryFlags, userData );
	if ( m_currentJob->GetTotalQueriesCount() >= MAX_QUERIES_PER_JOB )
	{
		SubmitCurrentJob();
	}
	return SQueryId( jobId, queryIndex );
}

EBatchQueryState CPhysicsBatchQueryManager::GetSweepQueryState( const SQueryId& queryId, TDynArray< SSweepHitResult >& result ) const
{
	if ( !queryId.IsValid() )
	{
		return BQS_NotFound;
	}

	TJobId jobId = queryId.m_jobId;
	if ( m_currentJob != nullptr && jobId == m_currentJob->GetId() )
	{
		return BQS_NotReady;
	}

	if ( m_jobsToRun.KeyExist( jobId ) )
	{
		return BQS_NotReady;
	}

	TJobsMap::const_iterator it = m_jobsReady.Find( jobId );
	if ( it == m_jobsReady.End() )
	{
		return BQS_NotFound;
	}
	if ( !it->m_second->IsFinished() || !it->m_second->GetSweepResult( queryId.m_queryIndex, result ))
	{
		return BQS_NotReady;
	}

	
	return BQS_Processed;
}

void* CPhysicsBatchQueryManager::GetSweepUserData( const SQueryId& queryId ) const
{
	if( queryId.IsValid() )
	{
		const TJobId jobId = queryId.m_jobId;
		TJobsMap::const_iterator it = m_jobsReady.Find( jobId );
		if( it != m_jobsReady.End() )
		{
			return it->m_second->GetSweepUserData( queryId.m_queryIndex );
		}

		it = m_jobsToRun.Find( jobId );
		if( it != m_jobsToRun.End() )
		{
			return it->m_second->GetSweepUserData( queryId.m_queryIndex );
		}

		if ( m_currentJob != nullptr && jobId == m_currentJob->GetId() )
		{
			return m_currentJob->GetSweepUserData( queryId.m_queryIndex );
		}
	}

	return nullptr;
}

void CPhysicsBatchQueryManager::Tick( Float timeDelta )
{
	SubmitCurrentJob();
	RunJobs();
	RemoveTimedOutJobs();
}

void CPhysicsBatchQueryManager::PepareCurrentJob()
{
	if ( m_currentJob == nullptr )
	{
		m_currentJob = new ( CTask::Root ) CJobPhysicsBatchQuery( m_world );
	}
}

void CPhysicsBatchQueryManager::SubmitCurrentJob()
{
	if ( m_currentJob == nullptr )
	{
		return;
	}
	// if job contains minimum number of queries
	// or contains any query but waited to long for submission
	
	// On gameplay request I'm removing the delay on job spawning so the result is the most up to date as possible, it shouldn't be a problem anyway 
	/*if ( m_currentJob->GetTotalQueriesCount() > MIN_QUERIES_PER_JOB ||
		( m_currentJob->GetTotalQueriesCount() > 0 && ( GGame->GetEngineTime() - m_currentJob->GetCreationTime() > MAX_TIME_TO_SUBMIT_JOB ) ) )
	{*/
		m_jobsToRun.Insert( m_currentJob->GetId(), m_currentJob );
		m_currentJob = nullptr;
	//}
}

void CPhysicsBatchQueryManager::RunJobs()
{
	Uint32 runJobs = 0;
	TJobsMap::iterator it = m_jobsToRun.Begin();
	while ( it != m_jobsToRun.End() && runJobs++ < MAX_JOBS_TO_RUN_PER_FRAME )
	{
		GTaskManager->Issue( *it->m_second );
		m_jobsReady.Insert( it->m_first, it->m_second );
		m_jobsToRun.Erase( it );
		it = m_jobsToRun.Begin();
	}
}

void CPhysicsBatchQueryManager::RemoveTimedOutJobs()
{
	TDynArray< TJobId > idsToRemove;
	EngineTime now = GGame->GetEngineTime();

	TJobsMap::iterator itEnd = m_jobsReady.End();
	for ( TJobsMap::iterator it = m_jobsReady.Begin(); it != itEnd; ++it )
	{
		if ( now - it->m_second->GetCreationTime() > JOB_TIME_OUT  )
		{
			idsToRemove.PushBack( it->m_first );
		}
	}

	TDynArray< TJobId >::iterator jtEnd = idsToRemove.End();
	for ( TDynArray< TJobId >::iterator jt = idsToRemove.Begin(); jt != jtEnd; ++jt )
	{
		itEnd = m_jobsReady.Find( *jt );
		if ( itEnd != m_jobsReady.End() )
		{
			itEnd->m_second->Release();
			m_jobsReady.Erase( itEnd );
		}
	}

	idsToRemove.ClearFast();
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EBatchQueryState );
IMPLEMENT_RTTI_CLASS( SScriptRaycastId );
IMPLEMENT_RTTI_CLASS( SScriptSweepId );
IMPLEMENT_ENGINE_CLASS( CScriptBatchQueryAccessor );

extern Bool GLatentFunctionStart;

//////////////////////////////////////////////////////////////////////////

Bool CScriptBatchQueryAccessor::RayCastSync( Vector position, Vector target, const CPhysicsEngine::CollisionMask& mask, TDynArray< SRaycastHitResult > & hitResults ) const
{
	static const CPhysicsEngine::CollisionMask charMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Character ) );

	CPhysicsWorld* physicsWorld = m_manager->GetPhysicsWorld();
	if ( physicsWorld == nullptr )
	{
		return false;
	}

	const Uint32 maxResults = 20;
	SPhysicsContactInfo infos[ maxResults ];
	Uint32 hits = 0;

	// disable character controller checking
	CPhysicsEngine::CollisionMask collisionGroups = mask & (~charMask);

	const ETraceReturnValue retVal = physicsWorld->RayCastWithMultipleResults( position, target, collisionGroups, 0, infos, hits, maxResults );
	if ( retVal == TRV_BufferOverflow || retVal == TRV_InvalidTransform || retVal == TRV_ProcessedWhileFetch )
	{
		return false;
	}

	for ( Uint32 i = 0; i < hits; ++i )
	{
		SPhysicsContactInfo& contactInfo = infos[ i ];
		SRaycastHitResult hitResult;

		hitResult.m_position = contactInfo.m_position;
		hitResult.m_normal = contactInfo.m_normal;
		hitResult.m_distance = contactInfo.m_distance;
		CComponent* component = nullptr;
		if ( contactInfo.m_userDataA != nullptr )
		{
			contactInfo.m_userDataA->GetParent( component, contactInfo.m_rigidBodyIndexA.m_actorIndex );
		}
		hitResult.m_component = component;
		hitResults.PushBack( hitResult );
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

void CScriptBatchQueryAccessor::funcRayCast( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, startPos, Vector::ZEROS );
	GET_PARAMETER( Vector, endPos, Vector::ZEROS );
	GET_PARAMETER_REF( TDynArray< SRaycastHitResult>, hitresult, TDynArray< SRaycastHitResult>() );
	GET_PARAMETER_OPT( TDynArray< CName >, collisionTypeNames, TDynArray< CName >() );
	GET_PARAMETER_OPT( Uint32, queryFlags, CPhysicsBatchQueryManager::DEFAULT_QUERY_FLAGS );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	SQueryId queryId;

	if ( GLatentFunctionStart )
	{
		CPhysicsEngine::CollisionMask mask = 0;

		if( collisionTypeNames.Empty() )
		{
			mask = CPhysicsBatchQueryManager::DEFAULT_COLLISION_MASK;
		}
		else
		{
			for( Uint32 i = 0, size = collisionTypeNames.Size(); i < size; ++i )
			{
				mask |= GPhysicEngine->GetCollisionTypeBit( collisionTypeNames[i] );
			}
		}

		queryId = m_manager->SubmitRaycastQuery( startPos, endPos, mask, queryFlags );

		ASSERT( !m_latentScriptQueries.KeyExist( stack.m_thread->GetID() ) );
		m_latentScriptQueries.Insert( stack.m_thread->GetID(), queryId );

		stack.m_thread->ForceYield();
		return;
	}

	m_latentScriptQueries.Find( stack.m_thread->GetID(), queryId );

	EBatchQueryState state = m_manager->GetRaycastQueryState( queryId, hitresult );
	if( state == BQS_Processed )
	{
		m_latentScriptQueries.Erase( stack.m_thread->GetID() );
		RETURN_BOOL( true );
		return;
	}

	if( state == BQS_NotReady )
	{
		stack.m_thread->ForceYield();
		return;
	}

	m_latentScriptQueries.Erase( stack.m_thread->GetID() );
	RETURN_BOOL( false );
}

void CScriptBatchQueryAccessor::funcRayCastSync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, startPos, Vector::ZEROS );
	GET_PARAMETER( Vector, endPos, Vector::ZEROS );
	GET_PARAMETER_REF( TDynArray< SRaycastHitResult >, hitResult, TDynArray< SRaycastHitResult>() );
	GET_PARAMETER_OPT( TDynArray< CName >, collisionTypeNames, TDynArray< CName >() );
	FINISH_PARAMETERS;

	CPhysicsEngine::CollisionMask mask = 0;
	if ( collisionTypeNames.Empty() )
	{
		mask = CPhysicsBatchQueryManager::DEFAULT_COLLISION_MASK;
	}
	else
	{
		for ( Uint32 i = 0, size = collisionTypeNames.Size(); i < size; ++i )
		{
			mask |= GPhysicEngine->GetCollisionTypeBit( collisionTypeNames[i] );
		}
	}

	Bool res = RayCastSync( startPos, endPos, mask, hitResult );
	RETURN_BOOL( res );
}

void CScriptBatchQueryAccessor::funcRayCastAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, startPos, Vector::ZEROS );
	GET_PARAMETER( Vector, endPos, Vector::EY );
	GET_PARAMETER_OPT( TDynArray< CName >, collisionTypeNames, TDynArray< CName >() );
	GET_PARAMETER_OPT( Uint32, queryFlags, CPhysicsBatchQueryManager::DEFAULT_QUERY_FLAGS );
	FINISH_PARAMETERS;

	CPhysicsEngine::CollisionMask mask = 0;

	if( collisionTypeNames.Empty() )
	{
		mask = CPhysicsBatchQueryManager::DEFAULT_COLLISION_MASK;
	}
	else
	{
		for( Uint32 i = 0, size = collisionTypeNames.Size(); i < size; ++i )
		{
			mask |= GPhysicEngine->GetCollisionTypeBit( collisionTypeNames[i] );
		}
	}

	SQueryId queryId = m_manager->SubmitRaycastQuery( startPos, endPos, mask, queryFlags );

	SScriptRaycastId raycastId;
	raycastId.m_queryId = queryId;

	RETURN_STRUCT( SScriptRaycastId, raycastId );
}

void CScriptBatchQueryAccessor::funcRayCastDir( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, startPos, Vector::ZEROS );
	GET_PARAMETER( Vector, direction, Vector::EY );
	GET_PARAMETER( Float, distance, 1.f );
	GET_PARAMETER_REF( TDynArray< SRaycastHitResult>, hitresult, TDynArray< SRaycastHitResult>() );
	GET_PARAMETER_OPT( TDynArray< CName >, collisionTypeNames, TDynArray< CName >() );
	GET_PARAMETER_OPT( Uint32, queryFlags, CPhysicsBatchQueryManager::DEFAULT_QUERY_FLAGS );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	SQueryId queryId;

	if ( GLatentFunctionStart )
	{
		CPhysicsEngine::CollisionMask mask = 0;

		if( collisionTypeNames.Empty() )
		{
			mask = CPhysicsBatchQueryManager::DEFAULT_COLLISION_MASK;
		}
		else
		{
			for( Uint32 i = 0, size = collisionTypeNames.Size(); i < size; ++i )
			{
				mask |= GPhysicEngine->GetCollisionTypeBit( collisionTypeNames[i] );
			}
		}

		queryId = m_manager->SubmitRaycastQuery( startPos, direction, distance, mask, queryFlags );

		ASSERT( !m_latentScriptQueries.KeyExist( stack.m_thread->GetID() ) );
		m_latentScriptQueries.Insert( stack.m_thread->GetID(), queryId );

		stack.m_thread->ForceYield();
		return;
	}

	m_latentScriptQueries.Find( stack.m_thread->GetID(), queryId );

	EBatchQueryState state = m_manager->GetRaycastQueryState( queryId, hitresult );
	if( state == BQS_Processed )
	{
		m_latentScriptQueries.Erase( stack.m_thread->GetID() );
		RETURN_BOOL( true );
		return;
	}

	if( state == BQS_NotReady )
	{
		stack.m_thread->ForceYield();
		return;
	}

	m_latentScriptQueries.Erase( stack.m_thread->GetID() );
	RETURN_BOOL( false );
}

void CScriptBatchQueryAccessor::funcRayCastDirSync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, startPos, Vector::ZEROS );
	GET_PARAMETER( Vector, direction, Vector::ZEROS );
	GET_PARAMETER( Float, distance, 0.0f );
	GET_PARAMETER_REF( TDynArray< SRaycastHitResult >, hitResult, TDynArray< SRaycastHitResult>() );
	GET_PARAMETER_OPT( TDynArray< CName >, collisionTypeNames, TDynArray< CName >() );
	GET_PARAMETER_OPT( Uint32, queryFlags, CPhysicsBatchQueryManager::DEFAULT_QUERY_FLAGS );
	FINISH_PARAMETERS;

	CPhysicsEngine::CollisionMask mask = 0;
	if ( collisionTypeNames.Empty() )
	{
		mask = CPhysicsBatchQueryManager::DEFAULT_COLLISION_MASK;
	}
	else
	{
		for ( Uint32 i = 0, size = collisionTypeNames.Size(); i < size; ++i )
		{
			mask |= GPhysicEngine->GetCollisionTypeBit( collisionTypeNames[i] );
		}
	}

	const Vector endPos = startPos + direction * distance;
	Bool res = RayCastSync( startPos, endPos, mask, hitResult );
	RETURN_BOOL( res );
}

void CScriptBatchQueryAccessor::funcRayCastDirAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, startPos, Vector::ZEROS );
	GET_PARAMETER( Vector, direction, Vector::EY );
	GET_PARAMETER( Float, distance, 1.f );
	GET_PARAMETER_OPT( TDynArray< CName >, collisionTypeNames, TDynArray< CName >() );
	GET_PARAMETER_OPT( Uint32, queryFlags, CPhysicsBatchQueryManager::DEFAULT_QUERY_FLAGS );
	FINISH_PARAMETERS;

	CPhysicsEngine::CollisionMask mask = 0;

	if( collisionTypeNames.Empty() )
	{
		mask = CPhysicsBatchQueryManager::DEFAULT_COLLISION_MASK;
	}
	else
	{
		for( Uint32 i = 0, size = collisionTypeNames.Size(); i < size; ++i )
		{
			mask |= GPhysicEngine->GetCollisionTypeBit( collisionTypeNames[i] );
		}
	}

	SQueryId queryId = m_manager->SubmitRaycastQuery( startPos, direction, distance, mask, queryFlags );

	SScriptRaycastId raycastId;
	raycastId.m_queryId = queryId;

	RETURN_STRUCT( SScriptRaycastId, raycastId );
}

void CScriptBatchQueryAccessor::funcGetRayCastState( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SScriptRaycastId, id, SScriptRaycastId() );
	GET_PARAMETER_REF( TDynArray< SRaycastHitResult>, hitresult, TDynArray< SRaycastHitResult>() );
	FINISH_PARAMETERS;

	EBatchQueryState state = m_manager->GetRaycastQueryState( id.m_queryId, hitresult );

	RETURN_ENUM( state );
}

void CScriptBatchQueryAccessor::funcSweep( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, startPos, Vector::ZEROS );
	GET_PARAMETER( Vector, endPos, Vector::ZEROS );
	GET_PARAMETER( Float, radius, 0.5f );
	GET_PARAMETER_REF( TDynArray< SSweepHitResult>, hitresult, TDynArray< SSweepHitResult>() );
	GET_PARAMETER_OPT( TDynArray< CName >, collisionTypeNames, TDynArray< CName >() );
	GET_PARAMETER_OPT( Uint32, queryFlags, CPhysicsBatchQueryManager::DEFAULT_QUERY_FLAGS );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	SQueryId queryId;

	if ( GLatentFunctionStart )
	{
		CPhysicsEngine::CollisionMask mask = 0;

		if( collisionTypeNames.Empty() )
		{
			mask = CPhysicsBatchQueryManager::DEFAULT_COLLISION_MASK;
		}
		else
		{
			for( Uint32 i = 0, size = collisionTypeNames.Size(); i < size; ++i )
			{
				mask |= GPhysicEngine->GetCollisionTypeBit( collisionTypeNames[i] );
			}
		}

		queryId = m_manager->SubmitSweepQuery( startPos, radius, endPos, mask, queryFlags );

		ASSERT( !m_latentScriptQueries.KeyExist( stack.m_thread->GetID() ) );
		m_latentScriptQueries.Insert( stack.m_thread->GetID(), queryId );

		stack.m_thread->ForceYield();
		return;
	}

	m_latentScriptQueries.Find( stack.m_thread->GetID(), queryId );

	EBatchQueryState state = m_manager->GetSweepQueryState( queryId, hitresult );
	if( state == BQS_Processed )
	{
		m_latentScriptQueries.Erase( stack.m_thread->GetID() );
		RETURN_BOOL( true );
		return;
	}

	if( state == BQS_NotReady )
	{
		stack.m_thread->ForceYield();
		return;
	}

	m_latentScriptQueries.Erase( stack.m_thread->GetID() );
	RETURN_BOOL( false );
}

void CScriptBatchQueryAccessor::funcSweepAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, startPos, Vector::ZEROS );
	GET_PARAMETER( Vector, endPos, Vector::EY );
	GET_PARAMETER( Float, radius, 0.5f );
	GET_PARAMETER_OPT( TDynArray< CName >, collisionTypeNames, TDynArray< CName >() );
	GET_PARAMETER_OPT( Uint32, queryFlags, CPhysicsBatchQueryManager::DEFAULT_QUERY_FLAGS );
	FINISH_PARAMETERS;

	CPhysicsEngine::CollisionMask mask = 0;

	if( collisionTypeNames.Empty() )
	{
		mask = CPhysicsBatchQueryManager::DEFAULT_COLLISION_MASK;
	}
	else
	{
		for( Uint32 i = 0, size = collisionTypeNames.Size(); i < size; ++i )
		{
			mask |= GPhysicEngine->GetCollisionTypeBit( collisionTypeNames[i] );
		}
	}

	SQueryId queryId = m_manager->SubmitSweepQuery( startPos, radius, endPos, mask, queryFlags );

	SScriptSweepId sweepId;
	sweepId.m_queryId = queryId;

	RETURN_STRUCT( SScriptSweepId, sweepId );
}

void CScriptBatchQueryAccessor::funcSweepDir( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, startPos, Vector::ZEROS );
	GET_PARAMETER( Vector, direction, Vector::EY );
	GET_PARAMETER( Float, radius, 0.5f );
	GET_PARAMETER( Float, distance, 1.f );
	GET_PARAMETER_REF( TDynArray< SSweepHitResult>, hitresult, TDynArray< SSweepHitResult>() );
	GET_PARAMETER_OPT( TDynArray< CName >, collisionTypeNames, TDynArray< CName >() );
	GET_PARAMETER_OPT( Uint32, queryFlags, CPhysicsBatchQueryManager::DEFAULT_QUERY_FLAGS );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	SQueryId queryId;

	if ( GLatentFunctionStart )
	{
		CPhysicsEngine::CollisionMask mask = 0;

		if( collisionTypeNames.Empty() )
		{
			mask = CPhysicsBatchQueryManager::DEFAULT_COLLISION_MASK;
		}
		else
		{
			for( Uint32 i = 0, size = collisionTypeNames.Size(); i < size; ++i )
			{
				mask |= GPhysicEngine->GetCollisionTypeBit( collisionTypeNames[i] );
			}
		}

		queryId = m_manager->SubmitSweepQuery( startPos, radius, direction, distance, mask, queryFlags );

		ASSERT( !m_latentScriptQueries.KeyExist( stack.m_thread->GetID() ) );
		m_latentScriptQueries.Insert( stack.m_thread->GetID(), queryId );

		stack.m_thread->ForceYield();
		return;
	}

	m_latentScriptQueries.Find( stack.m_thread->GetID(), queryId );

	EBatchQueryState state = m_manager->GetSweepQueryState( queryId, hitresult );
	if( state == BQS_Processed )
	{
		m_latentScriptQueries.Erase( stack.m_thread->GetID() );
		RETURN_BOOL( true );
		return;
	}

	if( state == BQS_NotReady )
	{
		stack.m_thread->ForceYield();
		return;
	}

	m_latentScriptQueries.Erase( stack.m_thread->GetID() );
	RETURN_BOOL( false );
}

void CScriptBatchQueryAccessor::funcSweepDirAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, startPos, Vector::ZEROS );
	GET_PARAMETER( Vector, direction, Vector::EY );
	GET_PARAMETER( Float, radius, 0.5f );
	GET_PARAMETER( Float, distance, 1.f );
	GET_PARAMETER_OPT( TDynArray< CName >, collisionTypeNames, TDynArray< CName >() );
	GET_PARAMETER_OPT( Uint32, queryFlags, CPhysicsBatchQueryManager::DEFAULT_QUERY_FLAGS );
	FINISH_PARAMETERS;

	CPhysicsEngine::CollisionMask mask = 0;

	if( collisionTypeNames.Empty() )
	{
		mask = CPhysicsBatchQueryManager::DEFAULT_COLLISION_MASK;
	}
	else
	{
		for( Uint32 i = 0, size = collisionTypeNames.Size(); i < size; ++i )
		{
			mask |= GPhysicEngine->GetCollisionTypeBit( collisionTypeNames[i] );
		}
	}

	SQueryId queryId = m_manager->SubmitSweepQuery( startPos, radius, direction, distance, mask, queryFlags );

	SScriptSweepId sweepId;
	sweepId.m_queryId = queryId;

	RETURN_STRUCT( SScriptSweepId, sweepId );
}

void CScriptBatchQueryAccessor::funcGetSweepState( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SScriptSweepId, id, SScriptSweepId() );
	GET_PARAMETER_REF( TDynArray< SSweepHitResult>, hitresult, TDynArray< SSweepHitResult>() );
	FINISH_PARAMETERS;

	EBatchQueryState state = m_manager->GetSweepQueryState( id.m_queryId, hitresult );

	RETURN_ENUM( state );
}