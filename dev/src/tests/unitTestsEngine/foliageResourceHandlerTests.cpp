/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/engine/foliageResourceHandler.h"

#include "foliageMocks.h"
#include "rendererMocks.h"

using testing::Mock;
using testing::ReturnRef;
using testing::Return;
using testing::_;

struct FoliageResourceHandlerFixture : ::testing::Test
{
	FoliageResourceHandlerFixture()
		:	dispatcherMock( new CFoliageRenderCommandDispatcherMock ),
			collisionHandlerMock( new testing::StrictMock< CFoliageCollisionHandlerMock > )
	{
		baseTreeHandle = BaseTreeHandle( &baseTreeMock );
	}

	~FoliageResourceHandlerFixture()
	{
		delete collisionHandlerMock;
	}

	static void SetUpTestCase()
	{
	}	

	static void TearDownTestCase()
	{
	}

	virtual void SetUp()
	{
		handler.SetInternalDispatcher( dispatcherMock.Get() );
		handler.SetInternalCollisionHandler( collisionHandlerMock );
	}

	void SetupPendingInstance( Int32 instanceCount, Int32 groupCount )
	{
		for( Int32 instanceIndex = 0; instanceIndex != instanceCount; ++instanceIndex )
		{
			foliageInstanceContainer.PushBack( SFoliageInstance() );
		}

		SFoliageInstanceGroup group;
		group.baseTree = baseTreeHandle; 
		group.instances = foliageInstanceContainer;

		for( Int32 groupIndex = 0; groupIndex != groupCount; ++groupIndex ) 
		{
			instanceContainer.PushBack( group );
		}

		EXPECT_CALL( resourceMock, GetAllTreeInstances() )
			.Times( 1 )
			.WillOnce( ReturnRef( instanceContainer ) );
	}

	void SetupDisplayMock( Int32 instanceCount, Int32 groupCount )
	{
		SetupPendingInstance( instanceCount, groupCount  );

		EXPECT_CALL( *collisionHandlerMock, AddAllCollision( baseTreeHandle.Get(), _ , _) )
			.Times( groupCount );

		EXPECT_CALL( *dispatcherMock, CreateSpeedTreeInstancesCommand( baseTreeHandle.Get(), _, _ ) )
			.Times( groupCount );
	}

	void SetupHideMock( Int32 baseTreeCount )
	{	
		for( Int32 baseTreeIndex = 0; baseTreeIndex != baseTreeCount; ++baseTreeIndex  )
		{
			baseTreeContainer.PushBack( baseTreeHandle );
		}
		
		EXPECT_CALL( resourceMock, GetAllBaseTree() )
			.Times( 1 )
			.WillOnce( ReturnRef( baseTreeContainer ) );

		EXPECT_CALL( *collisionHandlerMock, RemoveAllCollision( baseTreeHandle.Get(), _ ) )
			.Times( baseTreeCount );

		EXPECT_CALL( *dispatcherMock, RemoveSpeedTreeInstancesCommand( baseTreeHandle.Get(), _ ) )
			.Times( baseTreeCount );
	}

	CFoliageResourceMock resourceMock;
	Red::TScopedPtr< CFoliageRenderCommandDispatcherMock > dispatcherMock;
	testing::StrictMock< CFoliageCollisionHandlerMock > * collisionHandlerMock;
	CSRTBaseTreeMock baseTreeMock;
	BaseTreeHandle baseTreeHandle;

	FoliageInstanceContainer foliageInstanceContainer;
	CFoliageResource::InstanceGroupContainer instanceContainer;
	CFoliageResource::BaseTreeContainer baseTreeContainer;

	CFoliageResourceHandler handler;
};
