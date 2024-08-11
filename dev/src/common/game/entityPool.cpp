/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "entityPool.h"
#include "entityPoolSpawnJobFactory.h"
#include "../engine/dynamicLayer.h"
#include "../engine/appearanceComponent.h"
#include "../engine/renderFrame.h"
#include "../core/dynarray.h"
#include "../redSystem/unitTestMode.h"

namespace Config
{
#ifdef RED_PLATFORM_CONSOLE
	TConfigVar< Int32, Validation::IntRange< 1, 1000 > >		cvEntityGlobalLimit( "Gameplay/EntityPool", "GlobalLimit", 100 );
	TConfigVar< Int32, Validation::IntRange< 1, 1000 > >		cvEntitySpawnedLimit( "Gameplay/EntityPool", "SpawnedLimit", 75 );
#else
	TConfigVar< Int32, Validation::IntRange< 1, 1000 > >		cvEntityGlobalLimit( "Gameplay/EntityPool", "GlobalLimit", 150 );
	TConfigVar< Int32, Validation::IntRange< 1, 1000 > >		cvEntitySpawnedLimit( "Gameplay/EntityPool", "SpawnedLimit", 130, eConsoleVarFlag_Save );
#endif
}

const Uint32 c_pendingEntityContainerReservedSize = 16;

struct CEntityPool::BucketFinder
{
	BucketFinder( const THandle< CEntityTemplate > & entityTemplate )
		: m_entityTemplate( entityTemplate )
	{}

	bool operator()( const BucketTableEntry & entry ) const
	{
		return entry.m_first == m_entityTemplate;
	}

	THandle< CEntityTemplate > m_entityTemplate;
};

CEntityPool::Bucket::Bucket( Bucket && value )
	: entryContainer( std::move( value.entryContainer ) )
{}

CEntityPool::Bucket & CEntityPool::Bucket::operator=( Bucket && other )
{
	if( this != &other )
	{
		entryContainer = std::move( other.entryContainer );
	}
	return *this;
}

CEntityPool::CEntityPool()
	:	m_layer( nullptr ),
		m_entityCount( 0 ),
		m_entitySpawnedCount( 0 ),
		m_maxEntityCount( Config::cvEntityGlobalLimit.Get() ),
		m_maxEntitySpawnCount( Config::cvEntitySpawnedLimit.Get() ),
		m_fastForwardMode( false ),
		m_entityAddedThisFrame( false )
{
	m_pendingEntityContainer.Reserve( c_pendingEntityContainerReservedSize );
	m_processEntityContainer.Reserve( c_pendingEntityContainerReservedSize );
	m_entityContainer.Reserve( m_maxEntityCount );
}

CEntityPool::~CEntityPool()
{}

void CEntityPool::AddEntity( CEntity * entity )
{
	RED_FATAL_ASSERT( entity != nullptr, "null entity can't be add to pool." );
	RED_FATAL_ASSERT( entity->CheckEntityFlag( EF_Poolable ), "entity must have Poolable flag.");

	if( !m_fastForwardMode )
	{
		Red::Threads::CScopedLock< LockType > scopedLock( m_lock );
		m_pendingEntityContainer.PushBackUnique( entity );
	}
	else
	{
		RED_FATAL_ASSERT( SIsMainThread(), "Entity can only be added to directly to pool from fast travel, and on main thread");
		TryAddingEntityToPool( entity );
	}
}

void CEntityPool::Update()
{
	RED_FATAL_ASSERT( SIsMainThread(), "Update function can only be called form Main thread." );

	UpdateEntitySpawnLimit();

	SetupEntityToProcess();

	for( auto iter = m_processEntityContainer.Begin(), end = m_processEntityContainer.End(); iter != end; ++iter )
	{
		if( iter->IsValid() )
		{
			TryAddingEntityToPool( *iter );
		}
		else
		{
			// There are no guaranty that CEntity will still be around.
			// If its gone, just ignore it. It won't be processed next Update.
		}
	}

	Shrink();

	DestroyPendingEntity();
}

void CEntityPool::SetupEntityToProcess()
{
	m_entityAddedThisFrame = false;

	Red::Threads::CScopedLock< LockType > scopedLock( m_lock );
	m_processEntityContainer.SwapWith( m_pendingEntityContainer );
	m_pendingEntityContainer.ClearFast();
}

void CEntityPool::TryAddingEntityToPool( CEntity * entity )
{
	if( CanEntityBeAddedToPool( entity ) ) 
	{
		AddEntityToPool( entity );
	}
	else
	{
		Red::Threads::CScopedLock< LockType > scopedLock( m_lock );
		m_pendingEntityContainer.PushBack( entity );
	}
}

Bool CEntityPool::CanEntityBeAddedToPool( CEntity * entity ) const
{
	return ( m_fastForwardMode || !m_entityAddedThisFrame ) && entity->OnPoolRequest(); // OnPoolRequest; this function name is confusing. It means Entity can be detached and added to pool to be reused
}

void CEntityPool::AddEntityToPool( CEntity * entity )
{
	PC_SCOPE_PIX( CEntityPool::AddEntityToPool );

	m_entityAddedThisFrame = true;
	
	m_layer->DetachEntity( entity );

	CActor * actor = Cast< CActor >( entity );
	if( actor && !actor->IsAlive() )
	{
		// Dead actor respawn not supported. Get rid of this entity.
		m_entityPendingDestruction.PushBack( actor );
	}
	else
	{
		AddEntityToBucket( entity );
		m_entityCount.Increment();
	}
}

void CEntityPool::AddEntityToBucket( CEntity * entity )
{
	const CAppearanceComponent* appComp = entity->FindComponent<CAppearanceComponent>();
	const CName appearance = appComp ? appComp->GetAppearance() : CName::NONE; 

	THandle< CEntityTemplate > entityTemplate = entity->GetEntityTemplate();
	RED_FATAL_ASSERT( entityTemplate, "Entity do not have access to EntityTemplate." );

	Red::Threads::CScopedLock< LockType > scopedLock( m_lock );
	Bucket & bucket = AcquireBucket( entityTemplate );
	bucket.entryContainer.PushBack( MakePair( entity, appearance ) );
	m_entityContainer.PushBack( entity );
}

CEntityPool::Bucket & CEntityPool::AcquireBucket( const THandle< CEntityTemplate > & entityTemplate )
{
	BucketContainer::iterator iter = FindIf( m_bucketContainer.Begin(), m_bucketContainer.End(), BucketFinder( entityTemplate ) );
	if( iter == m_bucketContainer.End() )
	{
		BucketTableEntry entry;
		entry.m_first = entityTemplate;
		m_bucketContainer.PushBack( std::move( entry ) );
		return m_bucketContainer.Back().m_second;
	}

	return iter->m_second;
}

IJobEntitySpawn * CEntityPool::SpawnEntity( EntitySpawnInfo && spawnInfo, SpawnType type )
{
	RED_FATAL_ASSERT( spawnInfo.m_template, "Can't spawn without an EntityTemplate." );

	if( CanEntityBeSpawned( spawnInfo ) )
	{
		spawnInfo.m_entityFlags |= EF_Poolable;

		Red::Threads::CScopedLock< LockType > scopedLock( m_lock );
		CEntity * entity = AcquireMatchingEntity( spawnInfo );
		if( entity )
		{
			// An entity as been remove from pool. 
			// lets make sure we don't hold hard reference on EntityTemplate if we dont need to.
			Sanitize(); 
		}

#ifndef NO_EDITOR
		if( type == SpawnType_Community )
		{
			m_communityEntitySpawnedCount.Increment();
		}
#endif
		m_entitySpawnedCount.Increment();

		auto pair = MakePair( entity, std::move( spawnInfo ) );
		return m_factory->CreateSpawnJob( pair );
	}

	return nullptr;
}

Bool CEntityPool::CanEntityBeSpawned( const EntitySpawnInfo & info ) const
{
	if( m_layer->ValidateSpawnInfo( info ) )
	{
		return m_entitySpawnedCount.GetValue() < m_maxEntitySpawnCount || info.m_importantEntity;
	}

	return false;
}

CEntity * CEntityPool::AcquireMatchingEntity( EntitySpawnInfo & spawnInfo )
{
	CEntity * entity = nullptr;
	BucketContainer::iterator bucketIter = FindIf( m_bucketContainer.Begin(), m_bucketContainer.End(), BucketFinder( spawnInfo.m_template ) );
	if( bucketIter != m_bucketContainer.End() )
	{
		Bucket & bucket = bucketIter->m_second;
		entity = AcquireMatchingEntityFromBucket( bucket, spawnInfo ); 
		if( !entity )
		{
			// No entity match request appearance. Get first available (also the oldest). Appearance will be patched in spawning job.
			entity = AcquireFirstEntityFromBucket( bucket, spawnInfo );
		}
	}

	return entity;
}

CEntity * CEntityPool::AcquireMatchingEntityFromBucket( Bucket & bucket, EntitySpawnInfo & spawnInfo )
{
	Bucket::EntryContainer & entryContainer = bucket.entryContainer;

	if( !entryContainer.Empty() )
	{
		const auto & apperances = spawnInfo.m_appearances;
		for( auto apperanceIter = apperances.Begin(), apperanceEnd = apperances.End(); apperanceIter != apperanceEnd; ++apperanceIter  )
		{
			const auto predicate = [apperanceIter]( const Bucket::Entry & entry ){ return entry.m_second == *apperanceIter; };
			const auto entryIter = FindIf( entryContainer.Begin(), entryContainer.End(), predicate );
			if( entryIter != entryContainer.End() )
			{
				CEntity * entity = entryIter->m_first;
				const CName appearance = entryIter->m_second;

				// We have chosen an explicit appearance, so make sure to spawn this one
				spawnInfo.m_appearances.ClearFast();
				spawnInfo.m_appearances.PushBack( appearance );

				entryContainer.EraseFast( entryIter );
				m_entityContainer.RemoveFast( entity ); 

				m_entityCount.Decrement();

				return entity;
			}
		}
	}
	
	return nullptr;
}

CEntity * CEntityPool::AcquireFirstEntityFromBucket( Bucket & bucket, EntitySpawnInfo & spawnInfo )
{
	Bucket::EntryContainer & entryContainer = bucket.entryContainer;

	if( !entryContainer.Empty() )
	{
		Bucket::Entry entry = entryContainer.PopBack();
		spawnInfo.m_appearances.ResizeFast( 1 );
		m_entityContainer.RemoveFast( entry.m_first ); 
		m_entityCount.Decrement();
		return entry.m_first;
	}

	return nullptr;

}

IJobEntitySpawn * CEntityPool::SpawnEntities( TDynArray< EntitySpawnInfo > && spawnInfos, SpawnType type )
{
	if( CanEntitiesBeSpawned( spawnInfos ) )
	{
		CEntityPoolSpawnJobFactory::SpawnInfoContainer container;
		container.Resize( spawnInfos.Size() );

		Red::Threads::CScopedLock< LockType > scopedLock( m_lock );

		for( Uint32 index = 0, count = container.Size(); index != count; ++index )
		{
			CEntityPoolSpawnJobFactory::SpawnInfo & entry = container[ index ];
			entry.m_first = AcquireMatchingEntity( spawnInfos[ index ]  );
			entry.m_second = std::move( spawnInfos[ index ] );
			entry.m_second.m_entityFlags |= EF_Poolable;
			m_entitySpawnedCount.Increment();
		}

		// Average case would be that Entity were removed from at least one bucket.
		// To be safe, Sanitize.
		Sanitize();

		return m_factory->CreateCompositeSpawnJob( container ); // Better name ?
	}

	return nullptr;
}

Bool CEntityPool::CanEntitiesBeSpawned( const TDynArray< EntitySpawnInfo > & spawnInfos ) const
{
	Bool important = false;
	for( auto iter = spawnInfos.Begin(), end = spawnInfos.End(); iter != end; ++iter )
	{
		if( !m_layer->ValidateSpawnInfo( *iter ) )
		{
			return false;
		}

		important |= iter->m_importantEntity;
	}

	return important || ( GetEntitySpawnedCount() + static_cast< Int32 >( spawnInfos.Size() ) < GetMaxEntitySpawnCount() );
}

CEntity * CEntityPool::SpawnEntity_Sync( EntitySpawnInfo & info, SpawnType type )
{
	// Old comment: "Not using pool since we assume its loading a game so nothing should be in pool yet"
	info.m_entityFlags |= EF_Poolable;
	CEntity* entity = m_layer->CreateEntitySync( info );
	
	if( entity )
	{
		m_entitySpawnedCount.Increment();
	}

	return entity;
}

void CEntityPool::Shrink()
{
	if( GetEntityCount() > m_maxEntityCount )
	{
		// Pool is full, Let's get rid of old entity.
		Red::Threads::CScopedLock< LockType > scopedLock( m_lock );
		Int32 entityToRemove = GetEntityCount() - m_maxEntityCount;
		if( entityToRemove > 0 )
		{
			m_entityPendingDestruction.Reserve( m_entityPendingDestruction.Size() + entityToRemove );

			RemoveDanglingBucket( entityToRemove );

			for( Int32 index = 0; index != entityToRemove; ++index )
			{
				CEntity * entity = m_entityContainer[ index ];
				RemoveEntityFromPool( entity );
				m_entityPendingDestruction.PushBack( entity );
			}

			m_entityContainer.Erase( m_entityContainer.Begin(), m_entityContainer.Begin() + entityToRemove );
			m_entityCount.SetValue( m_entityContainer.Size() );

			Sanitize();
		}
		else
		{
			// Edge condition. Entity were remove from pool between "if" check and the lock acquire.
			// Nothing to do here anymore, move along !
		}
	}
}

void CEntityPool::RemoveDanglingBucket( Int32 & entityToRemove )
{
	for( auto iter = m_bucketContainer.Begin(), end = m_bucketContainer.End(); iter != end && entityToRemove != 0; ++iter )
	{
		Bucket & bucket = iter->m_second;
		if( bucket.entryContainer.Size() == 1 )
		{
			CEntity * entity = bucket.entryContainer.PopBack().m_first;
			m_entityContainer.Remove( entity );
			m_entityPendingDestruction.PushBack( entity );
			--entityToRemove;
		}
	}
}

void CEntityPool::RemoveEntityFromPool( CEntity * entity )
{
	CEntityTemplate * entityTemplate = entity->GetEntityTemplate();

	Bucket & bucket = AcquireBucket( entityTemplate );
	Bucket::EntryContainer & entryContainer = bucket.entryContainer;
	auto entryIter = FindIf( entryContainer.Begin(), entryContainer.End(), [entity]( const Bucket::Entry & entry ){ return entry.m_first == entity; } );
	if( entryIter != entryContainer.End() )
	{
		entryContainer.EraseFast( entryIter );
	}
}

void CEntityPool::DestroyPendingEntity()
{
	// If An entity was added to pool this frame, skip destruction this frame. It is too expensive. Else only destroy one.
	if( !m_entityAddedThisFrame && !m_entityPendingDestruction.Empty() ) 
	{
		PC_SCOPE_PIX( CEntityPool::DestroyPendingEntity );

		THandle< CEntity > entity = m_entityPendingDestruction.PopBack();
		if( entity.IsValid() )
		{
			entity->DestroyFromPool();
		}
	}
}

void CEntityPool::DestroyAllEntity( EntityContainer & container )
{
	for( auto entityIter = container.Begin(), entityEnd = container.End(); entityIter != entityEnd; ++entityIter )
	{
		// I move all the old code from DestroyEntity function to CEntity. 
		// CEntity should be in charge of its own destruction sequence.
		// Also, I can now Unit Test this!
		THandle< CEntity > entity = *entityIter;
		if( entity.IsValid() )
		{
			entity->DestroyFromPool();
		}
	}
}

void CEntityPool::Sanitize() // This function is not public and, in theory, buckets are already locked. No need to relock
{
	auto predicate = []( const BucketTableEntry & bucket ){ return bucket.m_second.entryContainer.Empty(); };
	m_bucketContainer.Erase(
		RemoveIf( m_bucketContainer.Begin(), m_bucketContainer.End(), predicate ),
		m_bucketContainer.End() );
}

void CEntityPool::Shutdown()
{
	RED_FATAL_ASSERT( SIsMainThread(), "Shutdown function can only be called form Main thread." );
	Flush();
}

void CEntityPool::SignalEntityUnspawn( CEntity * entity )
{
#ifndef NO_EDITOR
	{
		Red::Threads::CScopedLock< LockType > scopedLock( m_lock );
		if( m_communityEntity.Erase( entity ) )
		{
			m_communityEntitySpawnedCount.Decrement();
		}
	}
#endif

	m_entitySpawnedCount.Decrement();
}

void CEntityPool::RegisterCommunityEntity( CEntity * entity )
{
	// Terrible ... Only use for metrics.
#ifndef NO_EDITOR
	Red::Threads::CScopedLock< LockType > scopedLock( m_lock );
	m_communityEntity.Insert( entity );
#endif
	
}

void CEntityPool::CollectForGC( IFile & gc )
{
	Red::Threads::CScopedLock< LockType > scopedLock( m_lock );
	gc << m_bucketContainer;
}

void CEntityPool::Flush()
{
	EntityContainer entityToRemoveContainer;

	{
		Red::Threads::CScopedLock< LockType > scopedLock( m_lock );
		m_entityContainer.SwapWith( entityToRemoveContainer );
		m_bucketContainer.ClearFast();
		m_entityCount.SetValue( 0 );
	}

	DestroyAllEntity( entityToRemoveContainer );
	DestroyAllEntity( m_entityPendingDestruction );
}

void CEntityPool::StartFastForward()
{
	m_fastForwardMode = true;
}

void CEntityPool::StopFastForward()
{
	m_fastForwardMode = false;
	Flush(); // Fast forward completed, release all entity. Average case is that we don't need them anymore.
}

void CEntityPool::AddDebugScreenText( CRenderFrame * frame )
{
#ifndef RED_FINAL_BUILD
#ifndef NO_EDITOR
	frame->AddDebugScreenText( 250, 120, String::Printf( TXT("Encounter Actors Spawned: %i"), GetEntitySpawnedCount() - m_communityEntitySpawnedCount.GetValue() ), 0, false, Color::LIGHT_BLUE, Color(0,0,0) );
	frame->AddDebugScreenText( 250, 130, String::Printf( TXT("Community Actors Spawned: %i"), m_communityEntitySpawnedCount.GetValue() ), 0, false, Color::LIGHT_BLUE, Color(0,0,0) );
#endif
	frame->AddDebugScreenText( 250, 140, String::Printf( TXT("Total Actors Spawned: %i"), GetEntitySpawnedCount() ), 0, false, Color::RED, Color(0,0,0) );
#endif
}

Int32 CEntityPool::GetEntityCount() const
{
	return m_entityCount.GetValue();
}

Int32 CEntityPool::GetMaxEntityCount() const
{
	return m_maxEntityCount;
}

Int32 CEntityPool::GetEntitySpawnedCount() const
{
	return m_entitySpawnedCount.GetValue();
}

Int32 CEntityPool::GetMaxEntitySpawnCount() const
{
	return m_maxEntitySpawnCount; 
}

Bool CEntityPool::IsSpawnLimitExceeded() const
{
	return m_entitySpawnedCount.GetValue() > m_maxEntitySpawnCount;
}

Uint32 CEntityPool::GetInternalPendingEntityCount() const
{
	return m_pendingEntityContainer.Size();
}

Uint32 CEntityPool::GetInternalBucketEntityCount( CEntityTemplate * key ) const
{
	BucketContainer::const_iterator iter = FindIf( m_bucketContainer.Begin(), m_bucketContainer.End(), BucketFinder( key ) );
	return iter != m_bucketContainer.End() ? iter->m_second.entryContainer.Size() : 0;
}

Uint32 CEntityPool::GetInternalEntityPendingDestructionCount() const
{
	return m_entityPendingDestruction.Size();
}

void CEntityPool::SetInternalMaxEntityCount( Int32 value )
{
	m_maxEntityCount = value;
}

void CEntityPool::SetInternalMaxEntitySpawnCount( Int32 value )
{
	m_maxEntitySpawnCount = value;
}

void CEntityPool::SetInternalDynamicLayer( CDynamicLayer * layer )
{
	m_layer = layer;
}

void CEntityPool::SetInternalSpawnJobFactory( Red::TUniquePtr< CEntityPoolSpawnJobFactory > factory )
{
	m_factory = std::move( factory );
}

void CEntityPool::ForceAddEntity( CEntity * entity )
{
	RED_FATAL_ASSERT( Red::System::UnitTestMode(), "UnitTest only function! Use AddEntity." );
	AddEntityToBucket( entity );
	m_entityCount.Increment();
}

void CEntityPool::UpdateEntitySpawnLimit()
{
	Int32 entitySpawnedLimit = Config::cvEntitySpawnedLimit.Get();
	if( m_maxEntitySpawnCount != entitySpawnedLimit )
	{
		m_maxEntitySpawnCount = entitySpawnedLimit;
	}
}

Red::TUniquePtr< CEntityPool > CreateEntityPool( const CWorld * world )
{
	RED_FATAL_ASSERT( world, "Can't create CEntityPool with null world" );
	CDynamicLayer * layer = world->GetDynamicLayer();
	Red::TUniquePtr< CEntityPool > pool( new CEntityPool );
	Red::TUniquePtr< CEntityPoolSpawnJobFactory > factory = CreateEntityPoolSpawnJobFactory( layer );
	pool->SetInternalSpawnJobFactory( std::move( factory ) );
	pool->SetInternalDynamicLayer( world->GetDynamicLayer() );
	return pool;
}
