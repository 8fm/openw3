
#pragma once

#include "animTickableInterface.h"
#include "renderCamera.h"
#include "../core/task.h"
#include "../core/profiler.h"
#include "../core/frustum.h"
#include "../core/list.h"

class CRenderFrame;

class CAsyncAnimTickManager
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_Animation, MC_Animation, __alignof( CAsyncAnimTickManager ) );

	enum ELazyAction
	{
		LA_Add,
		LA_Remove,
	};

	struct AsyncLazyAction
	{
		ELazyAction				m_type;
		IAnimAsyncTickable*		m_object;
		EAsyncAnimPriority		m_prio;

		AsyncLazyAction() : m_type( LA_Add ), m_object( NULL ), m_prio( AAP_Low ) {}
		AsyncLazyAction( ELazyAction type, IAnimAsyncTickable* obj ) : m_type( type ), m_object( obj ) { m_prio = m_object->GetPriority(); }
	};

	struct SyncLazyAction
	{
		ELazyAction				m_type;
		IAnimSyncTickable*		m_object;

		SyncLazyAction() : m_type( LA_Add ), m_object( NULL ) {}
		SyncLazyAction( ELazyAction type, IAnimSyncTickable* obj ) : m_type( type ), m_object( obj ) {}
	};

	struct AsyncElem
	{
		DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Animation );

		IAnimAsyncTickable*		m_object;
		EAsyncAnimPriority		m_priority;
		Float					m_lastTime;
		Box						m_lastBox;
		Vector					m_lastPos;
		Float					m_distToPlayer;

		static int CompareElems( const void*a, const void* b )
		{
			AsyncElem* infoA = * ( AsyncElem** ) a;
			AsyncElem* infoB = * ( AsyncElem** ) b;
			if ( infoA->m_distToPlayer < infoB->m_distToPlayer ) return -1;
			if ( infoA->m_distToPlayer > infoB->m_distToPlayer ) return 1;
			return 0;
		}
	};

	class CAsyncAnimBucketJob;

private:
	static Float								BUDGET;	
	static Float								MAX_BUCKET_TIME;

	Float										m_timeBudget;
	Float										m_syncUpdateTime;
	CTimeCounter								m_waitingTimer;
	Float										m_waitingTime;
	Bool										m_waitingFlag;
	CTimeCounter								m_frameTime;
	Uint32										m_canceledObjects;

	CFrustum									m_frustum;

	TDynArray< AsyncLazyAction, MC_Animation, MemoryPool_Animation >		m_lazyAsyncActions;
	TDynArray< SyncLazyAction, MC_Animation, MemoryPool_Animation >			m_lazySyncActions;

	TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >				m_allAsyncPendingObjects;
	TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >				m_asyncArray;
	TDynArray< CAsyncAnimBucketJob*, MC_Animation, MemoryPool_Animation >	m_asyncBuckets;
	TList< IAnimSyncTickable* >						m_syncList;

#ifndef NO_EDITOR
	Uint32										m_debugLastUpdatedObject;
	CRenderCamera								m_debugCamera;
#endif

public:
	CAsyncAnimTickManager();
	~CAsyncAnimTickManager();

	void UpdatePre( Float dt );
	void UpdatePost( Float dt, Bool canUseJobs );

	void Add( IAnimAsyncTickable* obj );
	void Add( IAnimSyncTickable* obj );

	void Remove( IAnimAsyncTickable* obj );
	void Remove( IAnimSyncTickable* obj );

	void GenerateEditorFragments( CRenderFrame* frame );

private:
	void ProcessSyncLazyActions();
	void ProcessAsyncLazyActions();

	void UpdateSync( Float dt );
	void UpdateAsync( Float dt, Bool canUseJobs );

	void WaitForBuckets();

	void CalcCameraFrustum( CFrustum& f ) const;
	void CalcRenderCamera( CRenderCamera& cam ) const;

	void AddToList( TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& list, IAnimAsyncTickable* elem ) const;
	Bool RemoveFromList( TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& list, const IAnimAsyncTickable* elem ) const;
	Bool RemoveAndDeleteFromList( TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& list, const IAnimAsyncTickable* elem ) const;
	Bool IsInAnyAsyncList( const IAnimAsyncTickable* elem ) const;

	Bool RemoveFromLazyList( const IAnimAsyncTickable* elem, ELazyAction action );
	Bool RemoveFromLazyList( const IAnimSyncTickable* elem, ELazyAction action );

	void RemoveFromBuckets( IAnimAsyncTickable* obj );

private:
	class CAsyncAnimBucketJob : public CTask
	{
	private:
		Float		m_dt;
		Uint32		m_start;
		Uint32		m_end;

		const TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >&	m_list;

		static Red::Threads::CAtomic< Int32 >			m_counter;

	public:
		CAsyncAnimBucketJob( Float dt, Uint32 start, Uint32 end, const TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& list );
		virtual ~CAsyncAnimBucketJob();

		Uint32 GetNumberOfObjects() const;
		Float GetLastDutation() const;
		Float GetMaxElemTime() const;

		static Bool IsAnyJobProcessing();

	protected:
		void Run() override;

#ifndef NO_DEBUG_PAGES
		virtual const Char* GetDebugName() const { return TXT("AsyncUpdateAnimationBucket"); }
		virtual Uint32 GetDebugColor() const { return COLOR_UINT32( 255, 255, 255 ); }
#endif
	};

	class CFakeAsyncAnimBucketJob
	{
	public:
		CFakeAsyncAnimBucketJob( Float dt, const TDynArray< AsyncElem*, MC_Animation, MemoryPool_Animation >& list );
	};

#ifndef NO_DEBUG_PAGES
public:
	struct SDebugInfo
	{
		Float		m_timeBudget;
		Float		m_currTimeBudget;
		Float		m_syncTime;
		Float		m_currTime;
		Float		m_waitingTime;
		Float		m_bucketBudget;

		Uint32		m_syncNum;
		Uint32		m_asyncNum;
		Uint32		m_asyncRestNum;
		Uint32		m_bucketsNum;

		Uint32		m_canceledObjects;

		struct SBucketDebugInfo
		{
			Uint32	m_size;
			Float	m_restTime;
			Float	m_maxTime;

			SBucketDebugInfo( Uint32 s, Float time, Float max ) : m_size( s ), m_restTime( time ), m_maxTime( max ) {}
		};

		TDynArray< SBucketDebugInfo, MC_Debug >	m_bucketsInfo;
	};

	void Debug_GetInfo( SDebugInfo& info ) const;
#endif
};
