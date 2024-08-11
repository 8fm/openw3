
#pragma once

#include "../core/threadSafeNodeList.h"
#include "asyncAnimTickManager.h"
#include "poseProviderRepository.h"

class CAnimAllocator;
class AnimsetLoader;
class CBehaviorGraphInstance;
class CRenderFrame;

/// Animation manager
class AnimationManager
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Animation, MC_Animation );

public:
	struct AMStat
	{
		Int32	m_size;
		Int32	m_num;

		AMStat() : m_size( 0 ), m_num( 0 ) {}
	};

	struct AnimsetStreamingCooldown
	{
		const CSkeletalAnimationSet*	m_set;
		Float							m_timer;

		RED_INLINE AnimsetStreamingCooldown() : m_set( NULL ), m_timer( 120.f ) {}
		
		RED_INLINE Bool Update( Float dt ) { m_timer -= dt; return m_timer > 0.f; }
	};

private:
	static Red::Threads::CMutex						st_needsStreamingAnimsetsMutex;

protected:
	ThreadSafeNodeList< 1024, CSkeletalAnimation >	m_needsStreamingAnims;
	ThreadSafeNodeList< 1024, CSkeletalAnimation >	m_toUnloadAnims;
	TDynArray< CSkeletalAnimation* >				m_loadingAnims;

	TDynArray< AnimsetLoader* >						m_needsStreamingAnimsets;
	TDynArray< AnimsetLoader* >						m_loadingAnimsets;
	TDynArray< AnimsetStreamingCooldown >			m_cooldownedAnimsets;

	Bool											m_updating;

	Int32											m_currentSize;
	Int32											m_maxSize;
	Bool											m_recalcCurrentSize;

	AMStat											m_animsStaticStat;
	AMStat											m_graphsStat;
	AMStat											m_graphInstanceStat;

	Bool											m_unloadAll;

	CAsyncAnimTickManager*							m_asyncTickManager;

	CPoseProviderRepository							m_poseProviderRepository;

public:
	AnimationManager();
	~AnimationManager();

	void BeginUpdate( Float dt );
	void EndUpdate( Float dt );

	void UpateForInactiveWorld( Float dt );

	void GenerateEditorFragments( CRenderFrame* frame );

	void Flush();

	Uint32 GetMaxPoolSize() const;
	Uint32 GetCurrentPoolSize() const;
	Uint32 GetPosesPoolSize() const;

	const AMStat& GetGraphStat() const;
	const AMStat& GetGraphInstanceStat() const;
	const AMStat& GetAnimStaticStat() const;

public:
	void PreloadAnimationsOnGameInit();
	void PreloadAnimationsOnGameStart();
	void PreloadAnimationsOnAutoLoadResource();

	// Use with care - this function is solution for problems with emulation
	void StreamAnimset( const CSkeletalAnimationSet* set );
	void UnstreamAnimset( const CSkeletalAnimationSet* set );
	void ForceAnimsetToStream( AnimsetLoader* loader );

public: // Anim alloc
	PoseProviderHandle AcquirePoseProvider( const CSkeleton* skeleton );
	void SanitizePoseProvider();

public: // Logic
	void MarkToLoad( const CSkeletalAnimation* animation );
	void MarkToUnload( CSkeletalAnimation* animation );

	void MarkToUnloadAllAnimations();

public: // Events
	void OnAnimationCreated( CSkeletalAnimation* animation );
	void OnAnimationDestroyed( CSkeletalAnimation* animation );

	void OnGraphLoaded( CBehaviorGraph* graph );
	void OnGraphUnloaded( CBehaviorGraph* graph );

	void OnGraphInstanceBinded( CBehaviorGraphInstance* instance );
	void OnGraphInstanceUnbinded( CBehaviorGraphInstance* instance );

public: // Async/Sync ticks
	void AddAsync( IAnimAsyncTickable* obj );
	void AddSync( IAnimSyncTickable* obj );

	void RemoveAsync( IAnimAsyncTickable* obj );
	void RemoveSync( IAnimSyncTickable* obj );

private: // This functions can be called only from Update function
	void UnloadAllAnimations();
	void UnloadAnimation( CSkeletalAnimation* animation );
	
	void UpdateStreaming();
	Bool Evict( Int32 dataNeeded );
	void RecalcCurrentSize();

	Bool IsNotInPersistentList( const CSkeletalAnimation* animation ) const;

	Int32 GetDataToLoadSize() const;
	Bool HasMemoryForAnimation( CSkeletalAnimation* animation ) const;
	Bool CanLoadAnimation( CSkeletalAnimation* animation ) const;

	Bool CanStreamAnimset( const CSkeletalAnimationSet* set, Float& cooldown ) const;
	Bool HasStreamingPending( const CSkeletalAnimationSet* set ) const;
	Bool IsAnimInLoadingAnimsets( const CSkeletalAnimation* animation ) const;
	void SetCooldownForAnimset( const CSkeletalAnimationSet* set );
	void UpdateCooldowns( Float dt );

#ifndef NO_DEBUG_PAGES
public: // Debug functions
	void Debug_AddPoolSize( Int32 valInMB );

	CAsyncAnimTickManager* Debug_GetAsynTickMgr();
#endif
};

/// Animation manager
extern AnimationManager* GAnimationManager;
