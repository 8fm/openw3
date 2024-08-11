/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _UNIT_TEST_ENGINE_FOLIAGE_MOCKS_H_
#define _UNIT_TEST_ENGINE_FOLIAGE_MOCKS_H_

#include "../../common/engine/foliageBroker.h"
#include "../../common/engine/foliageGrid.h"
#include "../../common/engine/foliageCell.h"
#include "../../common/engine/foliageResourceLoader.h"
#include "../../common/engine/foliageResourceHandler.h"
#include "../../common/engine/foliageResource.h"
#include "../../common/engine/foliageCollisionHandler.h"
#include "../../common/engine/foliageRenderCommandDispatcher.h"
#include "../../common/engine/foliageRenderSettings.h"
#include "../../common/engine/foliageInstance.h"
#include "../../common/core/objectMap.h"
#include "../../common/engine/baseTree.h"
#include "../../common/engine/grassCellMask.h"
#include "../../common/core/math.h"


class CFoliageBrokerMock : public CFoliageBroker
{
public:
	CFoliageBrokerMock();
	virtual ~CFoliageBrokerMock();

	MOCK_METHOD1( AcquireCells, CellHandleContainer ( const Box & ) );
	MOCK_CONST_METHOD0( IsLoading, bool() );
	MOCK_METHOD0( Tick, void() );
	MOCK_METHOD1( PrefetchPosition, void( const Vector & ) );
};

class CFoliageGridMock : public CFoliageGrid
{
public:
	CFoliageGridMock();
	virtual ~CFoliageGridMock();
	MOCK_METHOD2( AcquireVisibleCells, CellHandleContainer ( const Vector2 & position, Int32 depth ) );
	MOCK_METHOD1( AcquireCell, CellHandle ( const Vector2 & position ) );
};

class CFoliageCellMock : public CFoliageCell
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	CFoliageCellMock();
	virtual ~CFoliageCellMock();

	MOCK_METHOD0( Tick, void() );
	MOCK_METHOD0_NO_OVERRIDE( Die, void() );

	MOCK_CONST_METHOD0( IsResourceValid, bool() );
	MOCK_METHOD0( Wait, void() );
	MOCK_CONST_METHOD0( IsLoading, bool() );
	MOCK_CONST_METHOD0( GetPath, const String & () );
};

typedef Red::TSharedPtr< CFoliageCellMock > CellHandleMock;

class CFoliageResourceLoaderMock : public CFoliageResourceLoader
{
public:
	CFoliageResourceLoaderMock();
	virtual ~CFoliageResourceLoaderMock();

	MOCK_CONST_METHOD1( GetResourceHandle,  FoliageResourceHandle ( const Vector2 & position ) );
	MOCK_CONST_METHOD2( ResourceAcquired, void ( CFoliageResource * resource, const Vector2 & position ) );
	MOCK_CONST_METHOD1( ResourceReleased, void ( CFoliageResource * resource ) );
	MOCK_CONST_METHOD1( CreateResource, FoliageResourceHandle( const Vector2 & position ) );
	MOCK_METHOD1( PrefetchAllResource, void ( const CellHandleContainer & container ) );
};

class CFoliageResourceHandlerMock : public CFoliageResourceHandler
{
public:

	CFoliageResourceHandlerMock();
	virtual ~CFoliageResourceHandlerMock();

	MOCK_METHOD1( DisplayFoliage, void ( const CFoliageResource * foliage ) );
	MOCK_METHOD1( HideFoliage, void ( const CFoliageResource * foliage ) );
	MOCK_CONST_METHOD0( InstancesPendingInsertion, bool() );
	MOCK_METHOD0( Tick, void() );
	MOCK_METHOD1( UpdateCurrentPosition, void( const Vector & ) );
};

class CFoliageResourceMock : public CFoliageResource
{
public:

	CFoliageResourceMock();
	virtual ~CFoliageResourceMock();

	MOCK_CONST_METHOD0( GetAllTreeInstances, const InstanceGroupContainer & () );
	MOCK_CONST_METHOD0( GetAllBaseTree, const BaseTreeContainer & () );

	MOCK_METHOD0( AddToRootSet, void() );
	MOCK_METHOD0( RemoveFromRootSet, void() );
};

class CSRTBaseTreeMock : public CSRTBaseTree
{
public:
	CSRTBaseTreeMock();
	virtual ~CSRTBaseTreeMock();

	MOCK_CONST_METHOD0( GetRenderObject, RenderObjectHandle() );
};

class CFoliageCollisionHandlerMock : public CFoliageCollisionHandler
{
public:

	CFoliageCollisionHandlerMock();
	virtual ~CFoliageCollisionHandlerMock();

	MOCK_METHOD3( AddAllCollision, void (  const CSRTBaseTree* baseTree, const FoliageInstanceContainer& instances, const Box& box) );
	MOCK_METHOD2( RemoveAllCollision, void ( const CSRTBaseTree* baseTree, const Box & box ) );
};


class CFoliageRenderCommandDispatcherMock : public IFoliageRenderCommandDispatcher
{
public:

	CFoliageRenderCommandDispatcherMock();
	virtual ~CFoliageRenderCommandDispatcherMock();

	MOCK_CONST_METHOD1( UpdateSpeedTreeInstancesCommand, void ( SFoliageUpdateRequest & updateRequest ) );
	MOCK_CONST_METHOD3( CreateSpeedTreeInstancesCommand, void ( const CSRTBaseTree * tree, const FoliageInstanceContainer & instanceData, const Box& rect ) );
	MOCK_CONST_METHOD3( CreateSpeedTreeDynamicInstancesCommand, void ( const CSRTBaseTree * tree,  const FoliageInstanceContainer & instanceData, const Box& rect ) );
	MOCK_CONST_METHOD2( RemoveSpeedTreeInstancesCommand, void ( const CSRTBaseTree * tree, const Box& rect ) );
	MOCK_CONST_METHOD3( RemoveSpeedTreeInstancesRadiusCommand, void( const CSRTBaseTree * tree, const Vector3& position, Float radius ) );
	MOCK_CONST_METHOD3( RemoveSpeedTreeDynamicInstancesRadiusCommand, void ( const CSRTBaseTree * tree, const Vector3& position, Float radius ) );
	MOCK_CONST_METHOD2( UpdateGenericGrassMaskCommand, void( IRenderProxy * terrainProxy, CGenericGrassMask * grassMask ) );
	MOCK_CONST_METHOD1( UploadGrassOccurrenceMasks, void( const TDynArray< CGrassCellMask >& cellMasks ) );
	MOCK_CONST_METHOD0( RefreshGenericGrassCommand, void () );
	MOCK_CONST_METHOD2( UpdateGrassSetupCommand, void ( IRenderProxy * terrainProxy, IRenderObject * renderUpdateData ) );
	MOCK_CONST_METHOD1( UpdateDynamicGrassCollisionCommand, void ( const TDynArray< SDynamicCollider >& collisions ) );
	MOCK_CONST_METHOD1( SetDebugVisualisationModeCommand, void ( EFoliageVisualisationMode mode ) );

#ifndef NO_EDITOR
	MOCK_CONST_METHOD1( UpdateFoliageRenderParams, void ( const SFoliageRenderParams &p ) );
#endif 

	MOCK_METHOD3( SetTreeFadingReferencePoints, void ( const Vector& left, const Vector& right, const Vector& center ) );
};

namespace testing
{
	namespace internal 
	{
		template < >
		class BuiltInDefaultValue< CellHandle > 
		{
		public:
			static bool Exists() { return true; }
			static CellHandle Get() { return CellHandle(); }
		};
	}
}

#endif
