
#include "build.h"
#include "animationManager.h"

#include "../core/jobGenericJobs.h"

#include "skeletalAnimation.h"
#include "animationIterator.h"
#include "animationStreamingJob.h"
#include "game.h"
#include "renderFrame.h"
#include "materialInstance.h"
#include "baseEngine.h"
#include "poseProvider.h"

namespace
{
	void PlaceForBreakpoint()
	{
		Int32 i = 0;
		i++;
	}
}

#define PROG_ASSERT( expr ) \
	ASSERT( expr );\
	if ( !(expr) )\
{\
	LOG_ENGINE( TXT(#expr) );\
	PlaceForBreakpoint();\
}

static int CompareByLastTouchTime(const void *a, const void* b)
{
	CSkeletalAnimation* animA = * ( CSkeletalAnimation** ) a;
	CSkeletalAnimation* animB = * ( CSkeletalAnimation** ) b;
	if ( animA->GetLastTouchTime() > animB->GetLastTouchTime() ) return -1;
	if ( animA->GetLastTouchTime() < animB->GetLastTouchTime() ) return 1;
	return 0;
}

Red::Threads::CMutex AnimationManager::st_needsStreamingAnimsetsMutex;

AnimationManager* GAnimationManager = NULL;

AnimationManager::AnimationManager()
	: m_updating( false )
	, m_unloadAll( false )
	, m_currentSize( 0 )
	, m_maxSize( 150 * 1024 * 1024 )
	, m_asyncTickManager( NULL )
	, m_recalcCurrentSize( false )
{
	m_asyncTickManager = new CAsyncAnimTickManager();
}

AnimationManager::~AnimationManager()
{
	m_needsStreamingAnims.Reset();
	m_toUnloadAnims.Reset();
	m_cooldownedAnimsets.Clear();

RED_MESSAGE(  "Threads: Mutex lock added here." )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_needsStreamingAnimsetsMutex );
		m_needsStreamingAnimsets.ClearPtr();
	}
	m_loadingAnimsets.ClearPtr();

	m_loadingAnims.Clear();

	delete m_asyncTickManager;
	m_asyncTickManager = NULL;
}

Uint32 AnimationManager::GetMaxPoolSize() const
{
	return m_maxSize;
}

Uint32 AnimationManager::GetCurrentPoolSize() const
{
	return m_currentSize;
}

Uint32 AnimationManager::GetPosesPoolSize() const
{
	return 15 * 1024 * 1024;
}

const AnimationManager::AMStat& AnimationManager::GetGraphStat() const
{
	return m_graphsStat;
}

const AnimationManager::AMStat& AnimationManager::GetGraphInstanceStat() const
{
	return m_graphInstanceStat;
}

const AnimationManager::AMStat& AnimationManager::GetAnimStaticStat() const
{
	return m_animsStaticStat;
}

void AnimationManager::MarkToLoad( const CSkeletalAnimation* animation )
{
	if ( !m_updating )
	{
		PROG_ASSERT( !animation->IsLoaded() );
		PROG_ASSERT( !animation->HasStreamingPending() );

		m_needsStreamingAnims.AddAsync( const_cast< CSkeletalAnimation* >( animation ) );
	}
}

void AnimationManager::MarkToUnload( CSkeletalAnimation* animation )
{
	PROG_ASSERT( !m_updating );
	PROG_ASSERT( animation->IsLoaded() || animation->HasStreamingPending() );

	m_toUnloadAnims.AddAsync( animation );
}

void AnimationManager::MarkToUnloadAllAnimations()
{
	m_unloadAll = true;
}

void AnimationManager::OnAnimationCreated( CSkeletalAnimation* animation )
{
	//m_animsStaticStat.m_size += animation->GetStaticSize();
	//m_animsStaticStat.m_num++;
}

void AnimationManager::OnAnimationDestroyed( CSkeletalAnimation* animation )
{
	PROG_ASSERT( !m_updating );

	//m_animsStaticStat.m_size -= animation->GetStaticSize();
	//m_animsStaticStat.m_num--;
	//PROG_ASSERT( m_animsStaticStat.m_size >= 0 );
	//PROG_ASSERT( m_animsStaticStat.m_num >= 0 );

	Bool check = false;

	// Streaming list
	if ( animation->HasStreamingPending() )
	{
		animation->CancelStreaming();

		for ( Uint32 i=0; i<m_loadingAnims.Size(); ++i )
		{
			if ( m_loadingAnims[ i ] == animation )
			{
				if ( check )
				{
					HALT( "Error in AnimationManager::OnAnimationDestroyed" );
				}

				check = true;
				m_loadingAnims[ i ] = NULL;
			}
		}
	}
	check = false;

	// Needs streaming list
	Uint32 needsStreamingNum = m_needsStreamingAnims.Size();
	for ( Uint32 i=0; i<needsStreamingNum; ++i )
	{
		if ( m_needsStreamingAnims[ i ] == animation )
		{
			if ( check )
			{
				HALT( "Error in AnimationManager::OnAnimationDestroyed" );
			}

			check = true;

			VERIFY( m_needsStreamingAnims.ResetAt( i ) );
		}
	}
	check = false;

	// Unload list
	Uint32 toUnloadAnimsNum = m_toUnloadAnims.Size();
	for ( Uint32 i=0; i<toUnloadAnimsNum; ++i )
	{
		if ( m_toUnloadAnims[ i ] == animation )
		{
			if ( check )
			{
				HALT( "Error in AnimationManager::OnAnimationDestroyed" );
			}

			check = true;

			VERIFY( m_toUnloadAnims.ResetAt( i ) );
		}
	}
	check = false;

	m_recalcCurrentSize = true;
}

void AnimationManager::OnGraphLoaded( CBehaviorGraph* graph )
{
	
}

void AnimationManager::OnGraphUnloaded( CBehaviorGraph* graph )
{
	
}

void AnimationManager::OnGraphInstanceBinded( CBehaviorGraphInstance* instance )
{
	
}

void AnimationManager::OnGraphInstanceUnbinded( CBehaviorGraphInstance* instance )
{
	
}

void AnimationManager::BeginUpdate( Float dt )
{
	PROG_ASSERT( ::SIsMainThread() );
	m_updating = true;

	m_asyncTickManager->UpdatePre( dt );

	UpdateStreaming();

	UpdateCooldowns( dt );
}

void AnimationManager::UpdateStreaming()
{
#ifndef NO_ANIM_CACHE
	// 0. 
	if ( m_recalcCurrentSize )
	{
		RecalcCurrentSize();

		m_recalcCurrentSize = false;
	}

	PROG_ASSERT( m_currentSize <= m_maxSize );

	// 1. For debug, unload all animations
	if ( m_unloadAll )
	{
		Flush();

		UnloadAllAnimations();

		//PROG_ASSERT( m_currentSize == 0 );

		m_unloadAll = false;

		return;
	}

	// 2. Unload marked animations
	{
		const Uint32 toUnloadAnimsNum = m_toUnloadAnims.Size();
		for ( Uint32 i=0; i<toUnloadAnimsNum; ++i )
		{
			CSkeletalAnimation* anim = m_toUnloadAnims[ i ];
			if ( anim )
			{
				UnloadAnimation( anim );

				m_currentSize -= anim->GetSizeOfAnimBuffer();
				PROG_ASSERT( m_currentSize >= 0 );
			}
		}

		m_toUnloadAnims.Reset();
	}

	// 3. Evict logic
	{
		const Int32 toLoadDataSize = GetDataToLoadSize();
		if ( m_currentSize + toLoadDataSize > m_maxSize )
		{
			Int32 dataNeeded = m_currentSize + toLoadDataSize - m_maxSize;
			Evict( dataNeeded );
		}
	}

	// 4A. Start new streaming jobs for animsets
	{
RED_MESSAGE(  "Threads: Mutex lock added here." )
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_needsStreamingAnimsetsMutex );

		const Uint32 needsStreamingNum = m_needsStreamingAnimsets.Size();
		for ( Uint32 i=0; i<needsStreamingNum; ++i )
		{
			AnimsetLoader* animsetLoader = m_needsStreamingAnimsets[ i ];

			const Uint32 dataToLoad = animsetLoader->GetDataSizeToLoad();

			if ( m_currentSize + (Int32)dataToLoad < m_maxSize && animsetLoader->StartStreaming() )
			{
				m_currentSize += dataToLoad;

				PROG_ASSERT( m_currentSize <= m_maxSize );

				m_loadingAnimsets.PushBack( animsetLoader );

				m_needsStreamingAnimsets[ i ] = NULL;
			}

		}

		m_needsStreamingAnimsets.ClearPtr();
	}

	// 4B. Start new streaming jobs for animations
	{
		const Uint32 needsStreamingNum = m_needsStreamingAnims.Size();
		for ( Uint32 i=0; i<needsStreamingNum; ++i )
		{
			CSkeletalAnimation* anim = m_needsStreamingAnims[ i ];

			if ( anim && HasMemoryForAnimation( anim ) && CanLoadAnimation( anim ) && !IsAnimInLoadingAnimsets( anim ) && anim->StartStreaming() )
			{
				m_currentSize += anim->GetSizeOfAnimBuffer();

				PROG_ASSERT( m_currentSize <= m_maxSize );

				m_loadingAnims.PushBack( anim );
			}
		}

		m_needsStreamingAnims.Reset();
	}

	PROG_ASSERT( m_currentSize <= m_maxSize );

	// 5A. Update streaming for animsets
	{
		const Int32 loadingAnimsetsNum = m_loadingAnimsets.SizeInt();
		for ( Int32 i=loadingAnimsetsNum-1; i>=0; --i )
		{
			AnimsetLoader* animsetLoader = m_loadingAnimsets[ i ];
			if ( animsetLoader->UpdateStreaming() )
			{
				Uint32 toLoad = animsetLoader->GetDataSizeToLoad();
				Uint32 loaded = animsetLoader->GetDataSizeLoaded();

				if ( toLoad != loaded )
				{
					m_currentSize -= toLoad;
					m_currentSize += loaded;

					PROG_ASSERT( m_currentSize <= m_maxSize );
				}

				// Streaming is finished
				delete animsetLoader;

				m_loadingAnimsets.Erase( m_loadingAnimsets.Begin() + i );
			}
		}
	}

	// 5B. Update streaming for animations
	{
		const Int32 loadingAnimsNum = m_loadingAnims.SizeInt();
		for ( Int32 i=loadingAnimsNum-1; i>=0; --i )
		{
			CSkeletalAnimation* anim = m_loadingAnims[ i ];
			if ( anim && anim->HasStreamingPending() )
			{
				if ( anim->UpdateStreaming() )
				{
					// Send event
					anim->OnStreamed();

					// Check
					PROG_ASSERT( anim->IsLoaded() );

					// Streaming is finished
					m_loadingAnims.Erase( m_loadingAnims.Begin() + i );
				}
			}
			else
			{
				// Animation was deleted during streaming
				m_loadingAnims.Erase( m_loadingAnims.Begin() + i );
			}
		}
	}

	static Bool CHECK_ANIM_STREAMING = false;
	if ( CHECK_ANIM_STREAMING )
	{
RED_MESSAGE(  "Threads: Mutex lock added here." )
		st_needsStreamingAnimsetsMutex.Acquire();
		Uint32 needsStreamingAnimsetsSize = m_needsStreamingAnimsets.Size();
		st_needsStreamingAnimsetsMutex.Release();
		
		// Check
		if ( m_loadingAnimsets.Size() == 0 && needsStreamingAnimsetsSize == 0 )
		{
			Bool check = true;
			Uint32 size = 0;

			for ( AnimationIterator it; it; ++it )
			{
				CSkeletalAnimation* anim = (*it);

				PROG_ASSERT( anim->HasValidId() );

				if ( anim->HasStreamingPending() )
				{
					check = false;
					break;
				}

				if ( anim->IsLoaded() )
				{
					size += anim->GetSizeOfAnimBuffer();
				}
			}

			if ( check )
			{
				PROG_ASSERT( (Int32)size == m_currentSize );
			}
		}
	}
	
#endif
}

void AnimationManager::EndUpdate( Float dt )
{
	m_asyncTickManager->UpdatePost( dt, GGame->GetGameplayConfig().m_animationAsyncJobs );

	m_updating = false;
}

void AnimationManager::UpateForInactiveWorld( Float dt )
{
	BeginUpdate( dt );

	m_asyncTickManager->UpdatePost( dt, false );

	m_updating = false;
}

void AnimationManager::GenerateEditorFragments( CRenderFrame* frame )
{
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_Bg ) )
	{
		m_asyncTickManager->GenerateEditorFragments( frame );
	}
}

void AnimationManager::Flush()
{
	// TODO
	//...
}

Bool AnimationManager::HasMemoryForAnimation( CSkeletalAnimation* animation ) const
{
	return m_currentSize + (Int32)animation->GetSizeOfAnimBuffer() < m_maxSize;
}

Bool AnimationManager::CanLoadAnimation( CSkeletalAnimation* animation ) const
{
	return !animation->IsLoaded() && !animation->HasStreamingPending();
}

Bool AnimationManager::IsAnimInLoadingAnimsets( const CSkeletalAnimation* animation ) const
{
	const Uint32 size = m_loadingAnimsets.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const AnimsetLoader* loader = m_loadingAnimsets[ i ];

		if ( loader->HasAnimationToLoad( animation ) )
		{
			return true;
		}
	}

	return false;
}

Bool AnimationManager::CanStreamAnimset( const CSkeletalAnimationSet* set, Float& cooldown ) const
{
	cooldown = 0.f;

	const Uint32 size = m_cooldownedAnimsets.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_cooldownedAnimsets[ i ].m_set == set )
		{
			cooldown = m_cooldownedAnimsets[ i ].m_timer;

			return false;
		}
	}

	return true;
}

void AnimationManager::SetCooldownForAnimset( const CSkeletalAnimationSet* set )
{
	Uint32 index = static_cast< Uint32 >( m_cooldownedAnimsets.Grow( 1 ) );
	AnimsetStreamingCooldown& cooldown = m_cooldownedAnimsets[ index ];
	cooldown.m_set = set;
	ASSERT( cooldown.m_timer > 0.f );
}

void AnimationManager::UpdateCooldowns( Float dt )
{
	for ( Int32 i=m_cooldownedAnimsets.SizeInt()-1; i>=0; --i )
	{
		if ( m_cooldownedAnimsets[ i ].Update( dt ) == false )
		{
			m_cooldownedAnimsets.RemoveAt( i );
		}
	}
}

Bool AnimationManager::HasStreamingPending( const CSkeletalAnimationSet* set ) const
{
	{
RED_MESSAGE(  "Threads: Mutex lock added here." )
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_needsStreamingAnimsetsMutex );

		const Uint32 size = m_needsStreamingAnimsets.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const AnimsetLoader* loader = m_needsStreamingAnimsets[ i ];
			if ( loader->CompareAnimset( set ) )
			{
				return true;
			}
		}
	}

	{
		const Uint32 size = m_loadingAnimsets.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const AnimsetLoader* loader = m_loadingAnimsets[ i ];
			if ( loader->CompareAnimset( set ) )
			{
				return true;
			}
		}
	}

	return false;
}

Bool AnimationManager::Evict( Int32 dataNeeded )
{
	// Get current engine time
	Uint64 currentTime = GEngine->GetCurrentEngineTick();

	//--- TEMP!!!
	static TDynArray< CSkeletalAnimation* > loadedAnims;
	loadedAnims.ClearFast();

	for ( AnimationIterator it( true ); it; ++it )
	{
		CSkeletalAnimation* anim = (*it);

		if ( currentTime - anim->GetLastTouchTime() > 30 )
		{
			loadedAnims.PushBack( anim );
		}
	}
	//---

	// Sort by last touch time
	qsort( loadedAnims.TypedData(), loadedAnims.Size(), sizeof( CSkeletalAnimation* ), &CompareByLastTouchTime );

	// Remove some old animations
	while( dataNeeded > 0 && loadedAnims.Size() > 0 )
	{
		CSkeletalAnimation* anim = loadedAnims.Back();

		if ( IsNotInPersistentList( anim ) )
		{
			Uint32 animSize = anim->GetSizeOfAnimBuffer();

			PROG_ASSERT( m_currentSize >= 0 );

			// Unload animation
			UnloadAnimation( anim );

			// Update sizes
			m_currentSize -= animSize;
			dataNeeded -= animSize;
		}

		// Remove animation from list
		loadedAnims.Erase( loadedAnims.End() - 1 );
	}

	return dataNeeded <= 0;
}

Bool AnimationManager::IsNotInPersistentList( const CSkeletalAnimation* animation ) const
{
	return true;
}

void AnimationManager::RecalcCurrentSize()
{
	Uint32 size = 0;

	// Animations
	for ( AnimationIterator it; it; ++it )
	{
		CSkeletalAnimation* anim = (*it);

		if ( anim && anim->IsLoaded() )
		{
			size += anim->GetSizeOfAnimBuffer();
		}
	}

	PROG_ASSERT( (Int32)size <= m_maxSize );

	// Loading animations
	const Uint32 animSize = m_loadingAnims.Size();
	for ( Uint32 i=0; i<animSize; ++i )
	{
		CSkeletalAnimation* anim = m_loadingAnims[ i ];

		if ( anim && anim->IsLoaded() )
		{
			size += anim->GetSizeOfAnimBuffer();
		}
	}

	PROG_ASSERT( (Int32)size <= m_maxSize );

	// Loading animsets
	const Uint32 animsetSize = m_loadingAnimsets.Size();
	for ( Uint32 i=0; i<animsetSize; ++i )
	{
		AnimsetLoader* animsetLoader = m_loadingAnimsets[ i ];

		size += animsetLoader->GetDataSizeToLoad();
	}

	PROG_ASSERT( (Int32)size <= m_maxSize );

	if ( (Int32)size != m_currentSize )
	{
		//LOG_ENGINE( TXT("AnimationManager Recalc size - old %d -> new %d"), m_currentSize, size );

		m_currentSize = size;
	}
}

Int32 AnimationManager::GetDataToLoadSize() const
{
	Int32 size = 0;

	const Uint32 needsStreamingNum = m_needsStreamingAnims.Size();
	for ( Uint32 i=0; i<needsStreamingNum; ++i )
	{
		CSkeletalAnimation* anim = m_needsStreamingAnims[ i ];
		if ( anim && CanLoadAnimation( anim ) )
		{
			size += (Int32)anim->GetSizeOfAnimBuffer();
		}
	}

RED_MESSAGE(  "Threads: Mutex lock added here." )
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_needsStreamingAnimsetsMutex );

	const Uint32 needsStreamingAnimsetNum = m_needsStreamingAnimsets.Size();
	for ( Uint32 i=0; i<needsStreamingAnimsetNum; ++i )
	{
		AnimsetLoader* animsetLoader = m_needsStreamingAnimsets[ i ];
		
		size += animsetLoader->GetDataSizeToLoad();
	}

	return size;
}

void AnimationManager::UnloadAllAnimations()
{
	for ( AnimationIterator it( true ); it; ++it )
	{
		CSkeletalAnimation* anim = (*it);
		if ( anim )
		{
			UnloadAnimation( anim );

			m_currentSize -= anim->GetSizeOfAnimBuffer();
			PROG_ASSERT( m_currentSize >= 0 );
		}
		else
		{
			PROG_ASSERT( anim );
		}
	}
}

void AnimationManager::UnloadAnimation( CSkeletalAnimation* animation )
{
	if ( animation->HasStreamingPending() )
	{
		animation->CancelStreaming();

		PROG_ASSERT( !animation->IsLoaded() );
	}
	else
	{
		// the whole AnimationManager code path looks broken
		//animation->Unload();
	}
}

PoseProviderHandle AnimationManager::AcquirePoseProvider( const CSkeleton* skeleton )
{
	return m_poseProviderRepository.AcquireProvider( skeleton );
}

void AnimationManager::SanitizePoseProvider()
{
	m_poseProviderRepository.Sanitize();
}


//////////////////////////////////////////////////////////////////////////

void AnimationManager::AddAsync( IAnimAsyncTickable* obj )
{
	m_asyncTickManager->Add( obj );
}

void AnimationManager::AddSync( IAnimSyncTickable* obj )
{
	m_asyncTickManager->Add( obj );
}

void AnimationManager::RemoveAsync( IAnimAsyncTickable* obj )
{
	m_asyncTickManager->Remove( obj );
}

void AnimationManager::RemoveSync( IAnimSyncTickable* obj )
{
	m_asyncTickManager->Remove( obj );
}

//////////////////////////////////////////////////////////////////////////

#define NO_PRELOAD_ANIMS
#define NO_USE_COOLDOWN_FOR_PRELOAD_ANIMS

class CAnimsetPreloadJob : public CJobLoadResource
{
public:
	CAnimsetPreloadJob( const String& resourceToLoad ) : CJobLoadResource( resourceToLoad, JP_Immediate ) {}
	virtual ~CAnimsetPreloadJob() { }
	virtual EJobResult Process()
	{
		EJobResult result = CJobLoadResource::Process();
		if ( m_loadedResource )
		{
			m_loadedResource->ForceFullyLoad();
		}
		CSkeletalAnimationSet* set = Cast< CSkeletalAnimationSet >( m_loadedResource.Get() );
		if ( set )
		{
			AnimsetLoader* loader = new AnimsetLoader( set, true );
			if ( loader->IsEmpty() )
			{
				delete loader;
			}
			else
			{
				GAnimationManager->ForceAnimsetToStream( loader );
			}
		}
		else
		{
			LOG_ENGINE( TXT("Preload animset ERROR - Invalid animset path:") );
			LOG_ENGINE( m_resourceToLoad.AsChar() );
		}
		return result;
	}

};

#define LOAD_ANIMSET( filePath )													\
{																					\
	CAnimsetPreloadJob* token = new CAnimsetPreloadJob( filePath		 );			\
	SJobManager::GetInstance().Issue( token );										\
	token->Release();																\
}

void AnimationManager::PreloadAnimationsOnGameInit()
{
	PROG_ASSERT( ::SIsMainThread() );
	PROG_ASSERT( !m_updating );

#if !defined NO_ANIM_CACHE && !defined NO_PRELOAD_ANIMS

	//LOG_ENGINE( TXT("PreloadAnimationsOnGameInit") );

	// Camera
	LOAD_ANIMSET( TXT("characters/templates/camera/camera_animations.w2anims") );
	LOAD_ANIMSET( TXT("characters/templates/camera/fistfight.w2anims") );
	LOAD_ANIMSET( TXT("characters/templates/camera/obstacle.w2anims") );

	// Witcher
	LOAD_ANIMSET( TXT("characters/templates/witcher/animation/exploration_animset.w2anims") );
	LOAD_ANIMSET( TXT("characters/templates/witcher/animation/weapon_draw.w2anims") );
	LOAD_ANIMSET( TXT("characters/templates/witcher/animation/witcher_steel.w2anims") );
	LOAD_ANIMSET( TXT("characters/templates/witcher/animation/witcher_stealth.w2anims") );

	// Npcs
	LOAD_ANIMSET( TXT("characters/templates/man/animation/exploration.w2anims") );
	LOAD_ANIMSET( TXT("characters/templates/woman/animation/exploration.w2anims") );
	LOAD_ANIMSET( TXT("characters/templates/dwarf/animation/exploration.w2anims") );
	LOAD_ANIMSET( TXT("characters/templates/child/animation/exploration.w2anims") );

#endif
}

void AnimationManager::PreloadAnimationsOnGameStart()
{
	PROG_ASSERT( ::SIsMainThread() );
	PROG_ASSERT( !m_updating );

	// Check loading jobs and fill animations before loading entities - we don't want to mark loading animations again
#if !defined NO_ANIM_CACHE && !defined NO_PRELOAD_ANIMS
	UpdateStreaming();
#endif
}

void AnimationManager::StreamAnimset( const CSkeletalAnimationSet* set )
{
	PROG_ASSERT( ::SIsMainThread() );
	PROG_ASSERT( !m_updating );

#ifndef NO_USE_COOLDOWN_FOR_PRELOAD_ANIMS
	Float cooldown = 0.f;
	if ( !CanStreamAnimset( set, cooldown ) )
	{
		LOG_ENGINE( TXT("===> Couldn't stream animset '%ls' - cooldown %1.2f"), set->GetDepotPath().AsChar(), cooldown );
		return;
	}
#endif

	if ( HasStreamingPending( set ) )
	{
		return;
	}

#if !defined NO_ANIM_CACHE && !defined NO_PRELOAD_ANIMS
	AnimsetLoader* loader = new AnimsetLoader( set );
	if ( loader->IsEmpty() )
	{
		delete loader;
	}
	else
	{
		Float size = loader->GetDataSizeToLoad() * ( 1.f / ( 1024.f * 1024.f ) );
		LOG_ENGINE( TXT("===> Stream animset '%ls' - %1.3f [MB]"), set->GetDepotPath().AsChar(), size );

		ForceAnimsetToStream( loader );

#ifndef NO_USE_COOLDOWN_FOR_PRELOAD_ANIMS
		SetCooldownForAnimset( set );
#endif
	}
#endif
}

void AnimationManager::UnstreamAnimset( const CSkeletalAnimationSet* set )
{
	// Usun persisten animations liste zeby bufor mogl je wyrzucic
	ASSERT( 0 );

	// UWAGA moze byc case Enter State leave enter => Load.... Unload, Load to samo, czyli spr czy jest w liscie m_needsStreamingAnimsets i usun wtedy
}

void AnimationManager::ForceAnimsetToStream( AnimsetLoader* loader )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_needsStreamingAnimsetsMutex );

	m_needsStreamingAnimsets.PushBack( loader );
}

//////////////////////////////////////////////////////////////////////////

#ifndef NO_DEBUG_PAGES

void AnimationManager::Debug_AddPoolSize( Int32 valInMB )
{
	const Uint32 mb = 1024 * 1024;
	m_maxSize = Max( m_maxSize + valInMB * mb, mb );
}

CAsyncAnimTickManager* AnimationManager::Debug_GetAsynTickMgr()
{
	return m_asyncTickManager;
}

#endif

//#pragma optimize("",on)
