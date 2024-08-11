/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/engine/foliageResourceLoader.h"
#include "../../common/engine/foliageResourceHandler.h"
#include "../../common/core/resourcePaths.h"
#include "../../common/core/depot.h"
#include "../../common/core/resourceLoading.h"
#include "../../common/core/diskFile.h"
#include "foliageMocks.h"


using testing::Mock;
using testing::ReturnRef;
using testing::Return;
using testing::_;
using Red::Core::ResourceManagement::CResourcePaths;


class CResourcePathsMock : public Red::Core::ResourceManagement::CResourcePaths
{
public:
	MOCK_CONST_METHOD1( GetPath, const String& ( EPath path ) );
};

class CDepotMock : public CDepot
{
public:
	CDepotMock()
		:CDepot( TXT( "Root/" ) )
	{}
	MOCK_METHOD1( FileExist, bool ( const String& path ) );
	MOCK_CONST_METHOD1( FindFile, CDiskFile * ( const String& path )  );
};

class CDiskFileMock : public CDiskFile
{
public:

	//MOCK_METHOD1( BindResource, void ( CResource * resource ) );
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	MOCK_METHOD0( Add, Bool() );
#endif

};

class CResourceLoaderMock : public CResourceLoader
{
public:
	
	MOCK_METHOD5( Load, void ( CDiskFile* rootFile, CDiskFile** files, Uint32 numFiles, EResourceLoadingPriority priority, class IDependencyImportLoader* loader ) );

};

struct FoliageResourceLoaderFixture : public ::testing::Test
{
	static void SetUpTestCase()
	{
		ResourceMonitorStats::EnableMonitoring( false );
	}

	static void TearDownTestCase()
	{
		ResourceMonitorStats::EnableMonitoring( true );
	}

	virtual void SetUp()
	{
		loader.SetInternalResourcePaths( &pathMock );
		loader.SetInternalHandler( &handlerMock );
		loader.SetInternalDepot( &depotMock );
		loader.SetInternalResourceLoader( &resourceLoader );
	}

	CFoliageResourceLoader loader;
	CResourcePathsMock pathMock;
	CFoliageResourceHandlerMock handlerMock;
	CFoliageResourceMock resourceMock;
	CDepotMock depotMock;
	CResourceLoaderMock resourceLoader;
};

TEST_F( FoliageResourceLoaderFixture, GenerateFoliageFilename_return_correctly_formated_string )
{
	EXPECT_STREQ( TXT( "foliage_128.00_92.00.flyr" ), GenerateFoliageFilename( Vector2( 128.0f, 92.0f ) ).AsChar() );
	EXPECT_STREQ( TXT( "foliage_-64.00_-64.00.flyr" ), GenerateFoliageFilename( Vector2( -64.0f, -64.0f ) ).AsChar() );
}

TEST_F( FoliageResourceLoaderFixture, GetResourceHandle_return_correctly_initialized_handle )
{
	const Vector2 position( -64.0f, 64.0f );
	const String path = TXT( "MyFoliagePath//" );
	const String expectedFullPath = path + GenerateFoliageFilename( position );

	EXPECT_CALL( pathMock, GetPath( CResourcePaths::Path_FoliageSourceData ) )
		.Times( 1 )
		.WillOnce( ReturnRef( path ) );
	
	EXPECT_CALL( depotMock, FileExist( expectedFullPath ) )
		.Times( 1 )
		.WillOnce( Return( true ) );

	FoliageResourceHandle resource = loader.GetResourceHandle( position );

	ASSERT_TRUE( resource );
	EXPECT_STREQ( expectedFullPath.AsChar(), resource->GetPath().AsChar() );

}

TEST_F( FoliageResourceLoaderFixture, GetResourceHandle_return_invalid_handle_if_file_does_not_exist )
{
	const Vector2 position( -64.0f, 64.0f );
	const String path = TXT( "MyFoliagePath//" );
	const String expectedFullPath = path + GenerateFoliageFilename( position );

	EXPECT_CALL( pathMock, GetPath( CResourcePaths::Path_FoliageSourceData ) )
		.Times( 1 )
		.WillOnce( ReturnRef( path ) );

	EXPECT_CALL( depotMock, FileExist( expectedFullPath ) )
		.Times( 1 )
		.WillOnce( Return( false ) );

	FoliageResourceHandle resource = loader.GetResourceHandle( position );

	ASSERT_FALSE( resource );
}

TEST_F( FoliageResourceLoaderFixture, ResourceAcquired_forward_loaded_resource_to_handler )
{
	EXPECT_CALL( handlerMock, DisplayFoliage( &resourceMock ) )
		.Times( 1 );

	loader.ResourceAcquired( &resourceMock, Vector2() );

	EXPECT_TRUE( Mock::VerifyAndClearExpectations( &handlerMock ) );
}

TEST_F( FoliageResourceLoaderFixture, ResourceRelease_forward_loaded_resource_to_handler )
{
	EXPECT_CALL( handlerMock, HideFoliage( &resourceMock ) )
		.Times( 1 );

	loader.ResourceReleased( &resourceMock );

	EXPECT_TRUE( Mock::VerifyAndClearExpectations( &handlerMock ) );
}

/* ctremblay FIX ME SoftHandle are too intrusive. 
TEST_F( FoliageResourceLoaderFixture, CreateResource_create_and_add_resource_to_depot )
{
	const Vector2 position( -64.0f, 64.0f );
	const String path = TXT( "MyFoliagePath//" );
	const String expectedFullPath = path + GenerateFoliageFilename( position );

	CDiskFileMock diskFileMock;

	EXPECT_CALL( pathMock, GetPath( CResourcePaths::Path_FoliageSourceData ) )
		.Times( 1 )
		.WillOnce( ReturnRef( path ) );

	EXPECT_CALL( depotMock, AddFile( expectedFullPath ) )
		.Times( 1 )
		.WillOnce( Return( &diskFileMock ) );
	
	EXPECT_CALL( diskFileMock, BindResource( _ ) ).Times( 1 );
	EXPECT_CALL( diskFileMock, Add() ).Times( 1 );   
	
	loader.CreateResource( position );

	EXPECT_TRUE( Mock::VerifyAndClearExpectations( &diskFileMock ) );
}
*/

TEST_F( FoliageResourceLoaderFixture, PrefetchAllResource_do_nothing_if_provided_container_is_empty )
{
	CellHandleContainer container;

	EXPECT_CALL( resourceLoader, Load( _, _, _, _, _ ) ).Times( 0 );
	loader.PrefetchAllResource( container );
}

TEST_F( FoliageResourceLoaderFixture, PrefetchAllResoure_do_nothing_if_cell_are_invalid )
{
	Red::TSharedPtr< CFoliageCellMock > cell1( new CFoliageCellMock );
	Red::TSharedPtr< CFoliageCellMock > cell2( new CFoliageCellMock );
	Red::TSharedPtr< CFoliageCellMock > cell3( new CFoliageCellMock );

	CellHandleContainer container;
	container.PushBack( cell1 );
	container.PushBack( cell2 );
	container.PushBack( cell3 );

	EXPECT_CALL( *cell1, IsResourceValid() ).WillOnce( Return( false ) );
	EXPECT_CALL( *cell2, IsResourceValid() ).WillOnce( Return( false ) );
	EXPECT_CALL( *cell3, IsResourceValid() ).WillOnce( Return( false ) );

	EXPECT_CALL( resourceLoader, Load( nullptr, _, 3, _, _ ) ).Times( 0 );

	loader.PrefetchAllResource( container );
}

TEST_F( FoliageResourceLoaderFixture, PrefetchAllResoure_forward_all_resolved_CDiskfile_to_ResourceLoader )
{
	Red::TSharedPtr< CFoliageCellMock > cell1( new CFoliageCellMock );
	Red::TSharedPtr< CFoliageCellMock > cell2( new CFoliageCellMock );
	Red::TSharedPtr< CFoliageCellMock > cell3( new CFoliageCellMock );

	CDiskFileMock entry1;
	CDiskFileMock entry2;
	CDiskFileMock entry3;

	CellHandleContainer container;
	container.PushBack( cell1 );
	container.PushBack( cell2 );
	container.PushBack( cell3 );

	const String cell1Path = TXT( "MyFoliagePath//foliage_cell_1.flyr" );
	const String cell2Path = TXT( "MyFoliagePath//foliage_cell_2.flyr" );
	const String cell3Path = TXT( "MyFoliagePath//foliage_cell_3.flyr" );

	EXPECT_CALL( *cell1, IsResourceValid() ).WillOnce( Return( true ) );
	EXPECT_CALL( *cell2, IsResourceValid() ).WillOnce( Return( true ) );
	EXPECT_CALL( *cell3, IsResourceValid() ).WillOnce( Return( true ) );

	EXPECT_CALL( *cell1, GetPath() ).WillOnce( ReturnRef( cell1Path ) );
	EXPECT_CALL( *cell2, GetPath() ).WillOnce( ReturnRef( cell2Path ) );
	EXPECT_CALL( *cell3, GetPath() ).WillOnce( ReturnRef( cell3Path ) );

	EXPECT_CALL( depotMock, FindFile( cell1Path ) ).WillOnce( Return( &entry1 ) );
	EXPECT_CALL( depotMock, FindFile( cell2Path ) ).WillOnce( Return( &entry2 ) );
	EXPECT_CALL( depotMock, FindFile( cell3Path ) ).WillOnce( Return( &entry3 ) );

	EXPECT_CALL( resourceLoader, Load( nullptr, _, 3, eResourceLoadingPriority_Normal, _ ) ).Times( 1 );

	loader.PrefetchAllResource( container );
}
