/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../../common/engine/entity.h"
#include "../../common/core/uniqueBuffer.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/jobSpawnEntity.h"
#include "../../common/engine/appearanceComponent.h"
#include "../../common/game/entityPoolSpawnJobFactory.h"
#include "../../common/game/entityPool.h"
#include "../../common/game/actor.h"
#include "../unitTestsFramework/attorney.h"

//////////////////////////////////////////////////////////////////////////

using ::testing::_;
using ::testing::Mock;
using ::testing::Return;
using ::testing::ReturnRef;

class CEntityMock : public CEntity
{
public:

	RED_DECLARE_UNIT_TEST_CLASS( CEntityMock );

	MOCK_METHOD0( OnPoolRequest, Bool() );
	MOCK_CONST_METHOD0( GetEntityTemplate, CEntityTemplate* () );
	MOCK_METHOD0( DestroyFromPool, void () );
};

class CActorMock : public CActor
{
public:
	RED_DECLARE_UNIT_TEST_CLASS( CActorMock );

	MOCK_METHOD0( DestroyFromPool, void () );
	MOCK_METHOD0( OnPoolRequest, Bool() );
};

class CAppearanceComponentMock : public CAppearanceComponent
{
public:

	RED_DECLARE_UNIT_TEST_CLASS( CAppearanceComponentMock );

	MOCK_CONST_METHOD0( GetAppearance, const CName& () );
};

class CEntityTemplateMock : public CEntityTemplate
{
public:

	RED_DECLARE_UNIT_TEST_CLASS( CEntityTemplateMock );
};

class CDynamicLayerMock : public CDynamicLayer
{
public:

	RED_DECLARE_UNIT_TEST_CLASS( CDynamicLayerMock );

	MOCK_METHOD1( DetachEntity, void ( CEntity* entity ) );
	MOCK_CONST_METHOD1( ValidateSpawnInfo, Bool ( const EntitySpawnInfo& info ) );
};

class CEntityPoolSpawnJobFactoryMock : public CEntityPoolSpawnJobFactory
{
public:

	MOCK_METHOD1( CreateSpawnJob, IJobEntitySpawn * ( SpawnInfo & info ) );
	MOCK_METHOD1( CreateCompositeSpawnJob, IJobEntitySpawn * ( SpawnInfoContainer & spawnInfoContainer ) );

private:
};

class CJobEntitySpawnMock : public IJobEntitySpawn 
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	CJobEntitySpawnMock()
		: IJobEntitySpawn( nullptr )
	{}

	MOCK_CONST_METHOD0( ValidateSpawn, Bool () );
	MOCK_METHOD0( LinkEntities, void () );
	MOCK_METHOD0( GetEntitiesCount, Uint32() );
	MOCK_METHOD1( GetSpawnedEntity, CEntity* ( Uint32 n ) );
	MOCK_METHOD0( Process, EJobResult () );
};

const CName UnkownAppearance( L"UnkownAppearance" );
const CName CorrectAppearance( L"CorrectAppearance" );

class EntityPoolMock : public CEntityPool
{
public:

	MOCK_METHOD0( UpdateEntitySpawnLimit, void() );
}; 

struct EntityPoolFixture : public ::testing::Test
{
	EntityPoolFixture()
		:	entityMock( new CEntityMock ),
			entityTemplateMock( new CEntityTemplateMock ),
			layerMock( new CDynamicLayerMock ),
			appearanceComponentMock( new CAppearanceComponentMock ),
			spawnJobFactoryMock( new CEntityPoolSpawnJobFactoryMock ),
			spawnJobMock( new CJobEntitySpawnMock )
	{
		pool.SetInternalDynamicLayer( layerMock.Get() );
		pool.SetInternalSpawnJobFactory( Red::TUniquePtr< CEntityPoolSpawnJobFactory >( spawnJobFactoryMock ) );
		
		entityMock->AddComponent( appearanceComponentMock.Get() );
	}

	~EntityPoolFixture()
	{
		spawnJobMock->Release();
	}

	void SetupReadyEntity()
	{
		entityMock->SetEntityFlag( EF_Poolable );
		EXPECT_CALL( *entityMock, OnPoolRequest() )
			.WillOnce( Return( true ) );

		EXPECT_CALL( *entityMock, GetEntityTemplate() )
			.WillRepeatedly( Return( entityTemplateMock.Get() ) );

		EXPECT_CALL( *layerMock, DetachEntity( entityMock.Get() ) )
			.Times( 1 );

		EXPECT_CALL( *appearanceComponentMock, GetAppearance() )
			.WillRepeatedly( ReturnRef( CorrectAppearance ) );

		pool.AddEntity( entityMock.Get() );
	}

	void SetupAndAddEntity( Uint32 count )
	{
		for( Uint32 index = 0; index != count; ++index )
		{
			Red::TUniquePtr< CEntityMock > mock( new CEntityMock );
			mock->SetEntityFlag( EF_Poolable );
		
			EXPECT_CALL( *mock, GetEntityTemplate() )
				.WillRepeatedly( Return( entityTemplateMock.Get() ) );

			EXPECT_CALL( *appearanceComponentMock, GetAppearance() )
				.WillRepeatedly( ReturnRef( CorrectAppearance ) );

			pool.ForceAddEntity( mock.Get() );
		
			entityMockContainer.PushBack( std::move( mock ) );
		}
	}

	void SetupNonReadyEntity()
	{
		entityMock->SetEntityFlag( EF_Poolable );
		EXPECT_CALL( *entityMock, OnPoolRequest() )
			.WillOnce( Return( false ) );
	
		pool.AddEntity( entityMock.Get() );
	}

	void SetupReadyForSpawn()
	{
		SetupReadyEntity();

		pool.Update();

		EXPECT_CALL( *layerMock, ValidateSpawnInfo( _ ) )
			.WillRepeatedly( Return( true ) );
	}

	Red::TScopedPtr< CEntityMock > entityMock;
	Red::TScopedPtr< CEntityTemplateMock > entityTemplateMock;
 	Red::TScopedPtr< CDynamicLayerMock > layerMock;
	Red::TScopedPtr< CAppearanceComponentMock > appearanceComponentMock;
	CEntityPoolSpawnJobFactoryMock * spawnJobFactoryMock;
	CJobEntitySpawnMock * spawnJobMock;

	TDynArray< Red::TUniquePtr< CEntityMock > > entityMockContainer;

	EntityPoolMock pool;
};


TEST_F( EntityPoolFixture, GetEntityCount_return_0_by_default )
{
	EXPECT_EQ( 0, pool.GetEntityCount() );
}

TEST_F( EntityPoolFixture, GetEntitySpawned_return_0_by_default )
{
	EXPECT_EQ( 0, pool.GetEntitySpawnedCount() );
}

#if 1 // those test are too slow. 
TEST_F( EntityPoolFixture, AddEntity_assert_if_entity_is_nullptr )
{
	EXPECT_DEATH_IF_SUPPORTED( pool.AddEntity( nullptr ),""  );
}

TEST_F( EntityPoolFixture, AddEntity_assert_if_entity_is_not_poolable )
{
	EXPECT_DEATH_IF_SUPPORTED( pool.AddEntity( entityMock.Get() ), ""  );
}

TEST_F( EntityPoolFixture, SpawnEntity_assert_if_no_template_provided )
{
	EntitySpawnInfo spawnInfo;
	EXPECT_DEATH_IF_SUPPORTED( pool.SpawnEntity( std::move( spawnInfo ), CEntityPool::SpawnType_Encounter ), ""  );
}
#endif

TEST_F( EntityPoolFixture, AddEntity_push_to_pending_list_to_be_processed_next_update )
{
	entityMock->SetEntityFlag( EF_Poolable );
	pool.AddEntity( entityMock.Get() );
	EXPECT_EQ( 1, pool.GetInternalPendingEntityCount() );
}

TEST_F( EntityPoolFixture, AddEntity_push_to_pending_list_only_once )
{
	entityMock->SetEntityFlag( EF_Poolable );
	pool.AddEntity( entityMock.Get() );
	pool.AddEntity( entityMock.Get() );
	EXPECT_EQ( 1, pool.GetInternalPendingEntityCount() );
}

TEST_F( EntityPoolFixture, Update_do_nothing_if_pool_empty_and_no_pending_entity )
{
	pool.Update();
}

TEST_F( EntityPoolFixture, Update_remove_pending_entity_if_it_is_not_valid_anymore )
{
	entityMock->SetEntityFlag( EF_Poolable );
	pool.AddEntity( entityMock.Get() );
	entityMock->Discard();
	pool.Update();
	EXPECT_EQ( 0, pool.GetEntityCount() );
	EXPECT_EQ( 0, pool.GetInternalPendingEntityCount() );
}

TEST_F( EntityPoolFixture, Update_do_not_add_entity_to_pool_if_not_ready_to_be_processed )
{
	SetupNonReadyEntity();
	pool.Update();
	EXPECT_EQ( 0, pool.GetEntityCount() );
	EXPECT_EQ( 1, pool.GetInternalPendingEntityCount() );
}

TEST_F( EntityPoolFixture, Update_do_not_add_dead_actor_and_push_to_destruction_list )
{
	Red::TUniquePtr< CActorMock > actorMock( new CActorMock );
	
	actorMock->SetAlive( false );
	actorMock->SetEntityFlag( EF_Poolable );

	EXPECT_CALL( *layerMock, DetachEntity( actorMock.Get() ) )
		.Times( 1 );

	EXPECT_CALL( *actorMock, OnPoolRequest() )
		.WillOnce( Return( true ) );
	
	pool.AddEntity( actorMock.Get() );
	pool.Update();

	EXPECT_EQ( 1, pool.GetInternalEntityPendingDestructionCount() );
	EXPECT_TRUE( Mock::VerifyAndClearExpectations( actorMock.Get() ) );
}

TEST_F( EntityPoolFixture, Update_detach_entity_if_ready )
{
	SetupReadyEntity();
	pool.Update();

	EXPECT_TRUE( Mock::VerifyAndClearExpectations( layerMock.Get() ) );
}

TEST_F( EntityPoolFixture, Update_add_entity_to_pool_if_ready_and_remove_from_pending_list )
{
	SetupReadyEntity();
	pool.Update();

	EXPECT_EQ( 1, pool.GetEntityCount() );
	EXPECT_EQ( 0, pool.GetInternalPendingEntityCount() );
}

TEST_F( EntityPoolFixture, Update_push_entity_to_correct_bucket )
{
	SetupReadyEntity();
	pool.Update();

	EXPECT_EQ( 1, pool.GetInternalBucketEntityCount( entityTemplateMock.Get() ) );
}

TEST_F( EntityPoolFixture, Update_push_old_entity_to_destruction_list_if_full )
{
  	pool.SetInternalMaxEntityCount( 3 );
	SetupAndAddEntity( 3 );
	SetupReadyEntity();
	
	pool.Update();

	EXPECT_EQ( 1, pool.GetInternalEntityPendingDestructionCount() );
	EXPECT_EQ( 3, pool.GetEntityCount() );
}

TEST_F( EntityPoolFixture, SpawnEntity_return_null_if_layer_is_not_happy )
{
	EXPECT_CALL( *layerMock, ValidateSpawnInfo( _ ) )
		.WillOnce( Return( false ) );

	EntitySpawnInfo spawnInfo;
	spawnInfo.m_template = entityTemplateMock.Get();
	IJobEntitySpawn * job = pool.SpawnEntity( std::move( spawnInfo ), CEntityPool::SpawnType_Encounter );

	EXPECT_FALSE( job );
}

TEST_F( EntityPoolFixture, SpawnEntity_return_null_if_max_spawn_is_busted )
{
	pool.SetInternalMaxEntitySpawnCount( 1 );
	SetupReadyForSpawn();

	EXPECT_CALL( *spawnJobFactoryMock, CreateSpawnJob( _ ) )
		.WillOnce( Return( spawnJobMock ) );

	EntitySpawnInfo spawnInfo1;
	spawnInfo1.m_template = entityTemplateMock.Get();
	EntitySpawnInfo spawnInfo2;
	spawnInfo2.m_template = entityTemplateMock.Get();

	IJobEntitySpawn * job1 = pool.SpawnEntity( std::move( spawnInfo1 ), CEntityPool::SpawnType_Encounter );
	IJobEntitySpawn * job2 = pool.SpawnEntity( std::move( spawnInfo2 ), CEntityPool::SpawnType_Encounter );

	EXPECT_TRUE( job1 != nullptr );
	EXPECT_TRUE( job2 == nullptr );
}

TEST_F( EntityPoolFixture, SpawnEntity_return_default_spawn_job_if_no_bucket )
{
	EXPECT_CALL( *layerMock, ValidateSpawnInfo( _ ) )
		.WillOnce( Return( true ) );

	EXPECT_CALL( *spawnJobFactoryMock, CreateSpawnJob( _ ) )
		.WillOnce( Return( spawnJobMock ) );

	EntitySpawnInfo spawnInfo;
	spawnInfo.m_template = entityTemplateMock.Get();

	IJobEntitySpawn * job = pool.SpawnEntity( std::move( spawnInfo ), CEntityPool::SpawnType_Encounter );

	EXPECT_TRUE( Mock::VerifyAndClearExpectations( spawnJobFactoryMock ) );
}

TEST_F( EntityPoolFixture, SpawnEntity_return_default_spawn_job_if_no_entity_with_correct_apperance )
{
	SetupReadyForSpawn();

	EXPECT_CALL( *spawnJobFactoryMock, CreateSpawnJob( _ ) )
		.WillOnce( Return( spawnJobMock ) );

	EntitySpawnInfo spawnInfo;
	spawnInfo.m_template = entityTemplateMock.Get();
	spawnInfo.m_appearances.PushBack( UnkownAppearance );

	IJobEntitySpawn * job = pool.SpawnEntity( std::move( spawnInfo ), CEntityPool::SpawnType_Encounter );

	EXPECT_TRUE( Mock::VerifyAndClearExpectations( spawnJobFactoryMock ) );
};

TEST_F( EntityPoolFixture, SpawnEntity_return_pool_spawn_job_if_entity_with_correct_apperance )
{
	SetupReadyForSpawn();

	EXPECT_CALL( *spawnJobFactoryMock, CreateSpawnJob( _ ) )
		.WillOnce( Return( spawnJobMock ) );

	EntitySpawnInfo spawnInfo;
	spawnInfo.m_template = entityTemplateMock.Get();
	spawnInfo.m_appearances.PushBack( CorrectAppearance );

	IJobEntitySpawn * job = pool.SpawnEntity( std::move( spawnInfo ), CEntityPool::SpawnType_Encounter );

	EXPECT_TRUE( Mock::VerifyAndClearExpectations( spawnJobFactoryMock ) );
};

TEST_F( EntityPoolFixture, SpawnEntity_clean_up_empty_bucket_after_being_done )
{
	SetupReadyForSpawn();

	EXPECT_CALL( *spawnJobFactoryMock, CreateSpawnJob( _ ) )
		.WillOnce( Return( spawnJobMock ) );

	EntitySpawnInfo spawnInfo;
	spawnInfo.m_template = entityTemplateMock.Get();
	spawnInfo.m_appearances.PushBack( CorrectAppearance );

	pool.SpawnEntity( std::move( spawnInfo ), CEntityPool::SpawnType_Encounter );

	EXPECT_EQ( 0, pool.GetInternalBucketEntityCount( entityTemplateMock.Get() ) );
}

TEST_F( EntityPoolFixture, SpawnEntities_return_null_if_one_spawnInfo_do_not_pass_layer_validation )
{
	TDynArray< EntitySpawnInfo > spawnInfos;
	spawnInfos.Resize( 3 );

	EXPECT_CALL( *layerMock, ValidateSpawnInfo( _ ) )
		.WillOnce( Return( true ) )
		.WillOnce( Return( true ) )
		.WillOnce( Return( false ) );

	IJobEntitySpawn * job = pool.SpawnEntities( std::move( spawnInfos ), CEntityPool::SpawnType_Encounter );
	EXPECT_FALSE( job );
}

TEST_F( EntityPoolFixture, SpawnEntities_return_correct_job )
{
	SetupReadyForSpawn();

	TDynArray< EntitySpawnInfo > spawnInfos;
	spawnInfos.Resize( 3 );
	spawnInfos[ 0 ].m_template = entityTemplateMock.Get();
	spawnInfos[ 1 ].m_template = entityTemplateMock.Get();
	spawnInfos[ 2 ].m_template = entityTemplateMock.Get();

	EXPECT_CALL( *spawnJobFactoryMock, CreateCompositeSpawnJob( _ ) )
		.WillOnce( Return( spawnJobMock ) );

	IJobEntitySpawn * job = pool.SpawnEntities( std::move( spawnInfos ), CEntityPool::SpawnType_Encounter );

	EXPECT_TRUE( Mock::VerifyAndClearExpectations( spawnJobFactoryMock ) );
}

TEST_F( EntityPoolFixture, SpawnEntities_return_null_if_max_spawn_is_busted )
{
	SetupReadyForSpawn();

	pool.SetInternalMaxEntitySpawnCount( 0 );

	TDynArray< EntitySpawnInfo > spawnInfos;
	spawnInfos.Resize( 3 );
	spawnInfos[ 0 ].m_template = entityTemplateMock.Get();
	spawnInfos[ 1 ].m_template = entityTemplateMock.Get();
	spawnInfos[ 2 ].m_template = entityTemplateMock.Get();

	IJobEntitySpawn * job = pool.SpawnEntities( std::move( spawnInfos ), CEntityPool::SpawnType_Encounter );

	EXPECT_TRUE( job == nullptr );
}

TEST_F( EntityPoolFixture, Flush_remove_and_delete_all_entity )
{
	pool.SetInternalMaxEntityCount( 3 );
	SetupAndAddEntity( 3 );

	pool.Update();

	EXPECT_CALL( *entityMockContainer[0], DestroyFromPool() ).Times( 1 );
	EXPECT_CALL( *entityMockContainer[1], DestroyFromPool() ).Times( 1 );
	EXPECT_CALL( *entityMockContainer[2], DestroyFromPool() ).Times( 1 );

	pool.Flush();

	EXPECT_EQ( 0, pool.GetEntityCount() );
}

TEST_F( EntityPoolFixture, Update_destoy_only_one_entity_per_call )
{
	pool.SetInternalMaxEntityCount( 1 );
	SetupAndAddEntity( 3 );

	EXPECT_CALL( *entityMockContainer[0], DestroyFromPool() ).Times( 0 );
	EXPECT_CALL( *entityMockContainer[1], DestroyFromPool() ).Times( 1 );

	pool.Update();
}
