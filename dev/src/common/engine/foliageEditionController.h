/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#ifndef _ENGINE_FOLIAGE_EDITION_CONTROLLER_H_
#define _ENGINE_FOLIAGE_EDITION_CONTROLLER_H_

#include "foliageForward.h"
#include "dynamicCollisionCollector.h"
#include "foliageResource.h"

#include "../core/uniquePtr.h"

class CSRTBaseTree;
class CClipMap;
class CFoliageInstancesBucket;
class IFoliageRenderCommandDispatcher;
class CFoliageResourceHandler;
class CFoliageCollisionHandler;
class CFoliageBroker;
class IRenderProxy;
class IRenderObject;
struct SFoliageRenderParams;
enum EFoliageVisualisationMode : Uint32;

extern const Uint32 GMaxInstanceInsertion;

struct SFoliageEditionControllerSetupParameter
{
	Vector2 worldDimension;
	Vector2 cellDimension;
	CFoliageScene * foliageScene;
	const IFoliageRenderCommandDispatcher * renderCommandDispatcher;
	CFoliageResourceHandler * resourceHandler; 
	CFoliageCollisionHandler * collisionHandler;
	Red::TSharedPtr< CFoliageBroker > foliageBroker;
};

// Helper container grouping instances with corresponding base tree
struct SFoliageInstanceCollection
{
	SFoliageInstanceCollection( THandle< CSRTBaseTree > baseTree );
	~SFoliageInstanceCollection();

	THandle< CSRTBaseTree > m_baseTree;
	FoliageInstanceContainer m_instances;
	CellHandle m_cellHandle;
};

struct SFoliageInstanceStatistics
{
	DECLARE_RTTI_STRUCT( SFoliageInstanceStatistics )

	SFoliageInstanceStatistics() {}
	SFoliageInstanceStatistics( CSRTBaseTree* baseTree, Uint32 instanceCount );

	THandle< CSRTBaseTree > m_baseTree;
	Uint32 m_instanceCount;
};

struct SFoliagePickShape
{
	THandle< CSRTBaseTree > m_baseTree;
	Cylinder m_shape;
	SFoliageInstance m_instance;
};

class CFoliageEditionController : Red::System::NonCopyable
#ifndef NO_EDITOR_EVENT_SYSTEM
	, IEdEventListener
#endif
{
public:

	CFoliageEditionController();
	~CFoliageEditionController();

	void Setup( const SFoliageEditionControllerSetupParameter & param );

	//! Returns the entire area covered by foliage cells
	Box GetWorldBox() const;

#ifndef	NO_RESOURCE_IMPORT
	void Save();
#endif
	static Bool PerformSilentCheckOutOnResource( CResource& res );

	Bool AddInstances( const CSRTBaseTree* baseTree, const FoliageInstanceContainer& instances );
	void AddPreviewInstances( const CSRTBaseTree * baseTree, const FoliageInstanceContainer& instanceInfos, const Box& box );
	void RemovePreviewInstances( const CSRTBaseTree * baseTree, const Box& box );
	Bool RemoveInstances( const CSRTBaseTree* baseTree, const Vector& center, Float radius );
	Bool ResizeInstances( const CSRTBaseTree* baseTree, const Vector& center, Float radius, Float value, Bool shrink );

	Bool GetClosestInstance( const Vector& position, CSRTBaseTree*& outTree, SFoliageInstance & outInstance );
	Box GetCellExtend( const Vector& position ) const;

	void GetInstancesFromArea( const CSRTBaseTree* baseTree, const Vector& center, Float radius, FoliageInstanceContainer& instances );
	void GetInstancesFromArea( const CSRTBaseTree* baseTree, const Box& box, FoliageInstanceContainer& instances );
	void GetInstancesFromArea( const Vector& center, Float radius, TDynArray< SFoliageInstanceCollection >& instances );
	void GetInstancesFromArea( const Box& box, TDynArray< SFoliageInstanceCollection >& instances );

	Bool ReplantInstance( const CSRTBaseTree* baseTree, const SFoliageInstance & oldInstance, const SFoliageInstance & newInstance );

	//! Rebuild the rendering wrappers for the instances from the given area
	void RefreshVisibility( const TDynArray< const CSRTBaseTree* >& baseTrees, const Box& box );

	//! Replaces all instances of one tree with another. If newBaseTree is nullptr removes all instances. Works also for currently unstreamed areas. Returns the number of instances replaced/removed.
	Uint32 ReplaceTree( const CSRTBaseTree* oldBaseTree, const CSRTBaseTree* newBaseTree, const Box& box, Bool showProgress = true, FoliageInstanceFilter filter = AllIntances, const Float* resetScale = nullptr );

	//! Gathers speed-tree usage statistics from the area (rounded up to the cell size). Works also for currently unstreamed areas. Returns the number of viewed cells.
	Uint32 GetStatisticsFromArea( const Box& box, TDynArray< SFoliageInstanceStatistics >& statistics, Bool showProgress = true );

	//! Gathers editor pick-shapes from the area.
	Uint32 GetPickShapesFromArea( const Box& boc, TDynArray< SFoliagePickShape >& pickShapes );

	void UpdateGrassMask( IRenderProxy * terrainProxy );
	void CreateGrassMask( const CWorld * world );
	void RefreshGrassMask();
	CGenericGrassMask * GetGrassMask();

	void UpdateGrassSetup( IRenderProxy * terrainProxy, IRenderObject * renderUpdateData );
	void UpdateDynamicGrassCollision( const TDynArray< SDynamicCollider >& collisions );

	void AcquireAndLoadFoliageAtArea( const Box& worldRect, CellHandleContainer& outCels );
	Bool AcquireResourcesAndAddToModified( const Box & worldRect );
	Bool CanModifyResources( const Box & worldRect ) const;

	void PushTransaction();
	void PullTransaction();
	Uint32 GetCurrentTransactionId() const;

	void UndoTransaction();
	void RedoTransaction();
	void FlushTransaction();

	void SetDebugVisualisationMode( EFoliageVisualisationMode mode );

	void ReduceVisibilityDepth();
	void IncreateVisibilityDepth();

	void WaitUntilAllFoliageResourceLoaded();

	Red::TSharedPtr< CFoliageBroker > GetFoliageBroker() const;

#ifndef NO_EDITOR
	void UpdateFoliageRenderParams( const SFoliageRenderParams &params );
#endif

#ifndef NO_EDITOR_EVENT_SYSTEM
	virtual void DispatchEditorEvent(const CName& name, IEdEventData* data);
#endif

private:
 	
	class ITransaction
	{
	public:
		ITransaction( const Box& worldRect ) : affectedRect( worldRect ) {}
		virtual ~ITransaction(){}
		virtual void Apply() = 0;
		virtual void Undo() = 0;

		Box affectedRect;
	};
	
	class AddTransaction;
	class RemoveTransaction;

	typedef Red::TSharedPtr< ITransaction > TransactionHandle;
	typedef TPair< Uint32, TransactionHandle > TransactionPair;
	typedef TDynArray< TransactionPair > TransactionContainer;

	void RefreshVisibility( const CSRTBaseTree* baseTree, const Box& box );
	Bool AcquireResourcesAndAddToModified( const CellHandleContainer& cellContainer );

	CFoliageScene * m_foliageScene;
	const IFoliageRenderCommandDispatcher * m_renderCommandDispatcher;
	CFoliageResourceHandler * m_resourceHandler; 
	CFoliageCollisionHandler * m_collisionHandler;
	Red::TSharedPtr< CFoliageBroker > m_foliageBroker;

	Uint32 m_transaction;
	TransactionContainer m_transactionContainer;

	CellHandleContainer m_editedCells;

	Vector2 m_worldDimension;
	Vector2 m_cellDimension;
};

Red::TUniquePtr< CFoliageEditionController > CreateFoliageEditionController( const SFoliageEditionControllerSetupParameter & param );

BEGIN_CLASS_RTTI( SFoliageInstanceStatistics )
	PROPERTY( m_baseTree )
	PROPERTY( m_instanceCount )
	END_CLASS_RTTI()


#endif
