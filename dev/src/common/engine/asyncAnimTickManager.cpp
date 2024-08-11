
#include "build.h"
#include "asyncAnimTickManager.h"
#include "cameraDirector.h"
#include "../core/taskManager.h"
#include "game.h"
#include "viewport.h"
#include "renderFrame.h"
#include "world.h"

Float CAsyncAnimTickManager::BUDGET = 40.f;
#ifdef _DEBUG
Float CAsyncAnimTickManager::MAX_BUCKET_TIME = 15.f;
#else
Float CAsyncAnimTickManager::MAX_BUCKET_TIME = 5.f;
#endif

CAsyncAnimTickManager::CAsyncAnimTickManager()
	: m_timeBudget( BUDGET )
	, m_waitingFlag( false )
	, m_syncUpdateTime( 0.f )
	, m_canceledObjects( 0 )
#ifndef NO_EDITOR
	, m_debugLastUpdatedObject( 0 )
#endif

{
	m_allAsyncPendingObjects.Reserve( 100 );
}

CAsyncAnimTickManager::~CAsyncAnimTickManager()
{
	m_lazyAsyncActions.Clear();
	m_lazySyncActions.Clear();

	m_syncList.Clear();

	WaitForBuckets();

	//for ( Uint32 i=0; i<ARRAY_COUNT(m_asyncArray); ++i )
	{
		TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& arr = m_asyncArray;//[ i ];
		arr.ClearPtr();
	}

	m_allAsyncPendingObjects.Clear();
}

void CAsyncAnimTickManager::UpdatePre( Float dt )
{
	PC_SCOPE( AsyncAnimTickManager_Pre );

	CTimeCounter timer;

	ASSERT( ::SIsMainThread() );

	// Process sync lazy actions
	{
		PC_SCOPE( ProcessSyncLazyActions );
		ProcessSyncLazyActions();
	}

	// Calc frustum
	{
		PC_SCOPE( CalcCameraFrustum );
		CalcCameraFrustum( m_frustum );
	}

	// Wait for async buckets
	{
		PC_SCOPE( WaitForBuckets );
		WaitForBuckets();
	}

	// Sync update
	{
		PC_SCOPE( UpdateSync );
		UpdateSync( dt );
	}

	m_syncUpdateTime = (Float)timer.GetTimePeriodMS();
}

void CAsyncAnimTickManager::UpdatePost( Float dt, Bool canUseJobs )
{
	PC_SCOPE( AsyncAnimTickManager_Post );

	CTimeCounter timer;

	ASSERT( ::SIsMainThread() );

	UpdateAsync( dt, canUseJobs );

	m_syncUpdateTime += (Float)timer.GetTimePeriodMS();
}

void CAsyncAnimTickManager::Add( IAnimAsyncTickable* obj )
{
	ASSERT( obj );
	ASSERT( ::SIsMainThread() );
	m_lazyAsyncActions.PushBack( AsyncLazyAction( LA_Add, obj ) );
}

void CAsyncAnimTickManager::Add( IAnimSyncTickable* obj )
{
	ASSERT( obj );
	ASSERT( ::SIsMainThread() );
	m_lazySyncActions.PushBack( SyncLazyAction( LA_Add, obj ) );
}

void CAsyncAnimTickManager::Remove( IAnimAsyncTickable* obj )
{
	ASSERT( obj );
	ASSERT( ::SIsMainThread() );

	if ( !RemoveFromLazyList( obj, LA_Add ) )
	{
		m_lazyAsyncActions.PushBack( AsyncLazyAction( LA_Remove, obj ) );

		RemoveFromBuckets( obj );
	}
}

void CAsyncAnimTickManager::Remove( IAnimSyncTickable* obj )
{
	ASSERT( obj );
	ASSERT( ::SIsMainThread() );

	if ( !RemoveFromLazyList( obj, LA_Add ) )
	{
		m_lazySyncActions.PushBack( SyncLazyAction( LA_Remove, obj ) );
	}
}

void CAsyncAnimTickManager::GenerateEditorFragments( CRenderFrame* frame )
{
#ifndef NO_EDITOR
	for ( Uint32 i=0; i<m_asyncArray.Size(); ++i )
	{
		Color color = Color( 0, 10, 0 );

		const Uint32 size = m_allAsyncPendingObjects.Size();
		for ( Uint32 j=0; j<size; ++j )
		{
			if ( m_asyncArray[ i ] == m_allAsyncPendingObjects[ j ] )
			{
				if ( j < m_debugLastUpdatedObject )
				{
					color = Color( 0, 255, 0 );
				}
			}
		}

		frame->AddDebugBox( m_asyncArray[ i ]->m_lastBox, Matrix::IDENTITY, color, false );
	}

	frame->AddDebugFrustum( m_debugCamera.GetScreenToWorld(), Color( 0, 0, 255 ), true, false, 1000.f );
#endif
}

void CAsyncAnimTickManager::ProcessSyncLazyActions()
{
	const Uint32 lazySyncSize = m_lazySyncActions.Size();
	for ( Uint32 i=0; i<lazySyncSize; ++i )
	{
		SyncLazyAction& action = m_lazySyncActions[ i ];

		if ( action.m_type == LA_Add )
		{
			ASSERT( !m_syncList.Exist( action.m_object ) );
			m_syncList.PushBack( action.m_object );
		}
		else
		{
			ASSERT( action.m_type == LA_Remove );
			VERIFY( m_syncList.Remove( action.m_object ) );
		}
	}
	m_lazySyncActions.ClearFast();
}

void CAsyncAnimTickManager::ProcessAsyncLazyActions()
{
	TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& list = m_asyncArray;
	for ( Int32 i=list.SizeInt()-1; i>=0; --i )
	{
		AsyncElem* e = list[ i ];

		if ( e->m_object == NULL )
		{
			delete e;

			list.Erase( list.Begin() + i );
		}
	}

	const Uint32 lazyAsyncSize = m_lazyAsyncActions.Size();
	for ( Uint32 i=0; i<lazyAsyncSize; ++i )
	{
		AsyncLazyAction& action = m_lazyAsyncActions[ i ];

		if ( action.m_type == LA_Add )
		{
			// Find list
			//EAsyncAnimPriority prio = action.m_prio;
			TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& list = m_asyncArray;//[ prio ];

			// Add to list
			AddToList( list, action.m_object );
		}
		else
		{
			ASSERT( action.m_type == LA_Remove );

			// Find list
			//EAsyncAnimPriority prio = action.m_prio;
			TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& list = m_asyncArray;

			// Check main list
			RemoveFromList( m_allAsyncPendingObjects, action.m_object );
			if( !RemoveAndDeleteFromList( list, action.m_object ) )
			{
				// AsyncElem.m_object can be NULL
				//ASSERT( 0 );
			}
		}
	}
	m_lazyAsyncActions.ClearFast();
}

void CAsyncAnimTickManager::UpdateSync( Float dt )
{
	TList< IAnimSyncTickable* >::iterator start = m_syncList.Begin();
	TList< IAnimSyncTickable* >::iterator end = m_syncList.End();

	for ( TList< IAnimSyncTickable* >::iterator it=start; it!=end; ++it )
	{
		IAnimSyncTickable* obj = *it;
		obj->DoSyncTick( dt );
	}
}

void CAsyncAnimTickManager::WaitForBuckets()
{
	m_waitingFlag = false;
	m_canceledObjects = 0;

	Double maxTime = 32.33 - MAX_BUCKET_TIME;

	if ( GGame && GGame->IsActive() && m_frameTime.GetTimePeriodMS() < maxTime )
	{
		Bool hasPendingJobs = true;
		const Uint32 size = m_asyncBuckets.Size();

		while ( m_frameTime.GetTimePeriodMS() < maxTime && hasPendingJobs )
		{
			hasPendingJobs = false;

			for ( Uint32 i=0; i<size; ++i )
			{
				CAsyncAnimBucketJob* bucketJob = m_asyncBuckets[ i ];
				if ( !bucketJob->IsFinished() )
				{
					hasPendingJobs = true;
					break;
				}
			}
		}
	}

	const Uint32 size = m_asyncBuckets.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CAsyncAnimBucketJob* bucketJob = m_asyncBuckets[ i ];
		if ( !bucketJob->IsFinished() )
		{
			if ( !m_waitingFlag )
			{
				m_waitingTimer.ResetTimer();
				m_waitingFlag = true;
			}

			m_canceledObjects += bucketJob->GetNumberOfObjects();

			bucketJob->TryCancel();
		}

		bucketJob->Release();
	}

	if ( m_canceledObjects > 0 )
	{
		if ( CAsyncAnimBucketJob::IsAnyJobProcessing() )
		{
			while ( CAsyncAnimBucketJob::IsAnyJobProcessing() )
			{
				// Waiting...
			}
		}
	}

	m_asyncBuckets.ClearFast();

	if ( m_waitingFlag == true )
	{
		m_waitingTime = (Float)m_waitingTimer.GetTimePeriodMS();
	}
	else
	{
		m_waitingTime = 0.0;
	}

	m_frameTime.ResetTimer();
}

void CAsyncAnimTickManager::UpdateAsync( Float dt, Bool canUseJobs )
{
	// 0. Process async lazy actions
	{
		PC_SCOPE( ProcessAsyncLazyActions );
		ProcessAsyncLazyActions();
	}

	if ( !GGame->GetGameplayConfig().m_animationCanUseAsyncJobs )
	{
		return;
	}

	// 1. Back to default params
	m_timeBudget = BUDGET;
	m_allAsyncPendingObjects.ClearFast();

	
	// 2. Calc reference position
	Vector refPos;
#ifndef NO_EDITOR
	if ( GIsEditor && !GGame->IsActive() )
	{
		// Dead code. Will not compile and was not use for months. Still useful?
		//extern Vector CalcCameraPosFromEditorPreview();
		//refPos = CalcCameraPosFromEditorPreview();
	}
	else if ( GGame->GetActiveWorld() )
	{
		refPos = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
	}
	else
	{
		refPos.Set4(0, 0, 0, 1);
	}
#else
	if( !GGame->IsActive() )
	{
		refPos.Set4(0, 0, 0, 1);
	}
	else
	{
		refPos = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
	}
#endif


	// 3. Add all visible elements
	//const Uint32 size = ARRAY_COUNT( m_asyncArray );
	//for ( Uint32 i=0; i<size; ++i )
	{
		PC_SCOPE( CollectObjects );

		const TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& arr = m_asyncArray;//[ i ];

		const Uint32 arrSize = arr.Size();
		for ( Uint32 j=0; j<arrSize; ++j )
		{
			AsyncElem* e = arr[ j ];
			ASSERT( e );
			ASSERT( e->m_object );
			
			if ( m_frustum.TestBox( e->m_lastBox ) > 0 )
			{
				// Add to global list
				//ASSERT( !m_allAsyncPendingObjects.Exist( e ) );

				e->m_distToPlayer = e->m_lastBox.SquaredDistance( refPos );

				m_allAsyncPendingObjects.PushBack( e );
			}
		}
	}

	// Sometimes we can not use job - for example in editor previews
	// Check it now before heavy operations
	if ( !canUseJobs )
	{
		CFakeAsyncAnimBucketJob fake( dt, m_allAsyncPendingObjects );
		return;
	}


	// 4. Sort list
	{
		PC_SCOPE( SortObjects );
		qsort( m_allAsyncPendingObjects.TypedData(), m_allAsyncPendingObjects.Size(), sizeof( AsyncElem* ), &AsyncElem::CompareElems );
	}



	// 5. Create buckets
	if ( !m_allAsyncPendingObjects.Empty() )
	{
		PC_SCOPE( CreateBuckets );

		Uint32 bucketStart = 0;
		Uint32 bucketEnd = 0;
		Float bucketBudget = MAX_BUCKET_TIME;

		const Uint32 size = m_allAsyncPendingObjects.Size();
		Uint32 i = 0;
		while ( 1 )
		{
			AsyncElem* elemToAdd = m_allAsyncPendingObjects[ i ];
			const Float elemToAddTime = elemToAdd->m_lastTime;

			Bool createBucket = false;

			if ( bucketBudget > 0.f )
			{
				// Dec budgets
				bucketBudget -= elemToAddTime;
				m_timeBudget -= elemToAddTime;

				// Add to bucket
				bucketEnd = i + 1;

				if ( bucketBudget < 0.f )
				{
					createBucket = true;
				}
			}

			Bool end = m_timeBudget < 0.f || i + 1 == size;

			if ( createBucket || end )
			{
				ASSERT( bucketStart != bucketEnd );

				// Create job for prev bucket
				CAsyncAnimBucketJob *job = new ( CTask::Root ) CAsyncAnimBucketJob( dt, bucketStart, bucketEnd, m_allAsyncPendingObjects );
				GTaskManager->Issue( *job );

				// Add job to list
				m_asyncBuckets.PushBack( job );

				// Create new bucket
				bucketStart = i + 1;
				bucketEnd = i + 1;

				// Dec budgets
				bucketBudget = Max( bucketBudget + MAX_BUCKET_TIME, 0.01f );

				if ( end )
				{
#ifndef NO_EDITOR
					m_debugLastUpdatedObject = bucketEnd;
#endif

					// Sorry, no more fun
					return;
				}
			}

			i += 1;
		}

#ifndef NO_EDITOR
		m_debugLastUpdatedObject = bucketEnd;
#endif
	}
}

void CAsyncAnimTickManager::AddToList( TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& list, IAnimAsyncTickable* object ) const
{
	AsyncElem* newElem = new AsyncElem();
	newElem->m_object = object;
	newElem->m_lastTime = 0.2f;
	newElem->m_priority = object->GetPriority();
	newElem->m_lastBox = object->GetBox();
	newElem->m_lastPos = object->GetBox().CalcCenter();

#ifndef NO_EDITOR
	Vector refPos;
	if ( GIsEditor && !GGame->IsActive() )
	{
		// Dead code. Will not compile and was not use for months. Still useful?
		//extern Vector CalcCameraPosFromEditorPreview();
		//refPos = CalcCameraPosFromEditorPreview();
	}
	else
	{
		refPos = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
	}
#else
	Vector refPos = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
#endif

	newElem->m_distToPlayer = newElem->m_lastPos.DistanceSquaredTo( refPos );

	ASSERT( !IsInAnyAsyncList( object ) );

	list.PushBack( newElem );
}

Bool CAsyncAnimTickManager::RemoveFromList( TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& list, const IAnimAsyncTickable* elem ) const
{
	const Uint32 size = list.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const AsyncElem* e = list[ i ];

		if ( e->m_object == elem )
		{
			list.RemoveAt( i );

			return true;
		}
	}

	return false;
}

Bool CAsyncAnimTickManager::RemoveFromLazyList( const IAnimAsyncTickable* elem, ELazyAction actionType )
{
	const Uint32 size = m_lazyAsyncActions.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		AsyncLazyAction& action = m_lazyAsyncActions[ i ];

		if ( action.m_object == elem && action.m_type == actionType )
		{
			m_lazyAsyncActions.RemoveAt( i );

			return true;
		}
	}
	return false;
}

Bool CAsyncAnimTickManager::RemoveFromLazyList( const IAnimSyncTickable* elem, ELazyAction actionType )
{
	const Uint32 size = m_lazySyncActions.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		SyncLazyAction& action = m_lazySyncActions[ i ];

		if ( action.m_object == elem && action.m_type == actionType )
		{
			m_lazySyncActions.RemoveAt( i );

			return true;
		}
	}
	return false;
}

Bool CAsyncAnimTickManager::RemoveAndDeleteFromList( TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& list, const IAnimAsyncTickable* elem ) const
{
	const Uint32 size = list.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const AsyncElem* e = list[ i ];

		if ( e->m_object == elem )
		{
			list.RemoveAt( i );
			ASSERT( !IsInAnyAsyncList( elem ) );

			delete e;

			return true;
		}
	}

	ASSERT( !IsInAnyAsyncList( elem ) );

	return false;
}

Bool CAsyncAnimTickManager::IsInAnyAsyncList( const IAnimAsyncTickable* elem ) const
{
	//for ( Uint32 i=0; i<ARRAY_COUNT( m_asyncArray ); ++i )
	{
		const TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& arr = m_asyncArray;//[ i ];

		for ( Uint32 j=0; j<arr.Size(); ++j )
		{
			const AsyncElem* e = arr[ j ];
			if ( e->m_object == elem )
			{
				return true;
			}
		}
	}
	return false;
}

void CAsyncAnimTickManager::RemoveFromBuckets( IAnimAsyncTickable* obj )
{
	const TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& arr = m_asyncArray;
	for ( Uint32 j=0; j<arr.Size(); ++j )
	{
		AsyncElem* e = arr[ j ];
		if ( e->m_object == obj )
		{
			e->m_object = NULL;
			return;
		}
	}
}

void CAsyncAnimTickManager::CalcRenderCamera( CRenderCamera& camera ) const
{
	if ( !GGame->IsActive() )
	{
#ifndef NO_EDITOR
		if ( GIsEditor )
		{
			// Dead code. Will not compile and was not use for months. Still useful?
			//extern void CalcRenderCameraFromEditorPreview( CRenderCamera& camera );
			//CalcRenderCameraFromEditorPreview( camera );
		}
		else
		{
			//ASSERT( 0 );
			return;
		}
#else
		//ASSERT( 0 ); no active camera in menu
		return;
#endif
	}
	else if( GGame->GetActiveWorld() )
	{
		Uint32 viewWidth = GGame->GetViewport()->GetWidth();
		Uint32 viewHeight = GGame->GetViewport()->GetHeight();

		GGame->GetActiveWorld()->GetCameraDirector()->OnSetupCamera( camera, viewWidth, viewHeight );
	}
}

void CAsyncAnimTickManager::CalcCameraFrustum( CFrustum& frustum ) const
{
#ifndef NO_EDITOR
	if ( GGame->GetGameplayConfig().m_animationAsyncJobsUpdateFrustum )
	{
		CRenderCamera& camera = const_cast< CRenderCamera& >( m_debugCamera );
		CalcRenderCamera( camera );
		frustum = CFrustum( camera.GetWorldToScreen() );
	}
#else
	if( GGame->GetActiveWorld() )
	{
		GGame->GetActiveWorld()->GetCameraDirector()->CalcCameraFrustum( frustum );
	}
#endif
}

//////////////////////////////////////////////////////////////////////////

Red::Threads::CAtomic< Int32 > CAsyncAnimTickManager::CAsyncAnimBucketJob::m_counter( 0 );

CAsyncAnimTickManager::CAsyncAnimBucketJob::CAsyncAnimBucketJob( Float dt, Uint32 start, Uint32 end, const TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& list ) 
	: m_start( start )
	, m_end( end )
	, m_list( list )
	, m_dt( dt )
{
}

CAsyncAnimTickManager::CAsyncAnimBucketJob::~CAsyncAnimBucketJob() 
{

}

Bool CAsyncAnimTickManager::CAsyncAnimBucketJob::IsAnyJobProcessing()
{
	return m_counter.GetValue() > 0;
}

Uint32 CAsyncAnimTickManager::CAsyncAnimBucketJob::GetNumberOfObjects() const 
{ 
	return m_end - m_start; 
}

Float CAsyncAnimTickManager::CAsyncAnimBucketJob::GetLastDutation() const 
{
	Float time = 0.f;

	for ( Uint32 i=m_start; i<m_end; ++i )
	{
		AsyncElem* elem = m_list[ i ];
		time += elem->m_lastTime;
	}

	return time;
}

Float CAsyncAnimTickManager::CAsyncAnimBucketJob::GetMaxElemTime() const
{
	Float time = 0.f;

	for ( Uint32 i=m_start; i<m_end; ++i )
	{
		AsyncElem* elem = m_list[ i ];
		time = Max( time, elem->m_lastTime );
	}

	return time;
}

void CAsyncAnimTickManager::CAsyncAnimBucketJob::Run()
{
	PC_SCOPE_PIX( CAsyncAnimBucketJob );

	m_counter.Increment();

	for ( Uint32 i=m_start; i<m_end; ++i )
	{
		CTimeCounter timer;

		AsyncElem* elem = m_list[ i ];

		// PTom TODO:
		// elem->m_object->DoAsyncTick( m_dt ) and elem->m_object->GetBox() should be one function

		if ( elem->m_object )
		{
			elem->m_object->DoAsyncTick( m_dt );
		}

		if ( elem->m_object )
		{
			elem->m_lastBox = elem->m_object->GetBox();
		}
		
		elem->m_lastPos = elem->m_lastBox.CalcCenter();
		elem->m_lastTime = (Float)timer.GetTimePeriodMS();
	}

	m_counter.Decrement();

	ASSERT( m_counter.GetValue() >= 0 );
}

CAsyncAnimTickManager::CFakeAsyncAnimBucketJob::CFakeAsyncAnimBucketJob( Float dt, const TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& list )
{
	const Uint32 size = list.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CTimeCounter timer;

		AsyncElem* elem = list[ i ];

		elem->m_object->DoAsyncTick( dt );

		elem->m_lastBox = elem->m_object->GetBox();
		elem->m_lastPos = elem->m_object->GetBox().CalcCenter();
		elem->m_lastTime = (Float)timer.GetTimePeriodMS();
	}
}

//////////////////////////////////////////////////////////////////////////

#ifndef NO_DEBUG_PAGES

void CAsyncAnimTickManager::Debug_GetInfo( CAsyncAnimTickManager::SDebugInfo& info ) const
{
	info.m_currTimeBudget = m_timeBudget;
	info.m_timeBudget = BUDGET;
	info.m_syncTime = m_syncUpdateTime;
	info.m_waitingTime = m_waitingFlag ? m_waitingTime : 0.f;
	info.m_bucketBudget = MAX_BUCKET_TIME;
	info.m_syncNum = m_syncList.Size();

	info.m_asyncNum = 0;
	info.m_currTime = 0;
	info.m_bucketsNum = m_asyncBuckets.Size();
	info.m_canceledObjects = m_canceledObjects;

	for ( Uint32 i=0; i<m_asyncBuckets.Size(); ++i )
	{
		const CAsyncAnimBucketJob* bucketJob = m_asyncBuckets[ i ];

		Uint32 s = bucketJob->GetNumberOfObjects();
		Float t = bucketJob->GetLastDutation();
		Float m = bucketJob->GetMaxElemTime();

		info.m_bucketsInfo.PushBack( SDebugInfo::SBucketDebugInfo( s, t, m ) );

		info.m_asyncNum += s;
		info.m_currTime += t;
	}

	info.m_asyncRestNum = m_allAsyncPendingObjects.Size() - info.m_asyncNum;
}

#endif
