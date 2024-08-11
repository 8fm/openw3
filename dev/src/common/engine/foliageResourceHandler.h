/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef _ENGINE_FOLIAGE_RESOURCE_HANDLER_H_
#define _ENGINE_FOLIAGE_RESOURCE_HANDLER_H_

#include "../core/uniquePtr.h"
#include "../core/weakPtr.h"
#include "../core/sharedPtr.h"
#include "renderObjectPtr.h"
#include "foliageResource.h"
#include "foliageLodSetting.h"

class CFoliageInstancesData;
class IFoliageRenderCommandDispatcher;
class CFoliageCollisionHandler;

class CPhysicsWorld;
class CSRTBaseTree;

class CFoliageResourceHandler
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:

	typedef SFoliageInstanceGroup::Instances InstanceContainer;

	CFoliageResourceHandler();
	RED_MOCKABLE ~CFoliageResourceHandler();

	RED_MOCKABLE void DisplayFoliage( const CFoliageResource * foliage );
	RED_MOCKABLE void HideFoliage( const CFoliageResource * foliage );

	RED_MOCKABLE void Tick();
	RED_MOCKABLE bool InstancesPendingInsertion() const;

	RED_MOCKABLE void UpdateCurrentPosition( const Vector & position );

	void Wait();

	void AddInstances( const CSRTBaseTree * baseTree, const InstanceContainer& instances, const Box& box ) const;
	void AddDynamicInstances( const CSRTBaseTree * baseTree, const InstanceContainer& instances, const Box& box ) const;
	void AddInstances( const CSRTBaseTree * baseTree, const InstanceContainer& instances ) const;

	void RemoveInstances( const CSRTBaseTree* baseTree, const Box& box ) const;
	void RemoveDynamicInstances( const CSRTBaseTree* baseTree, const Vector& position, Float radius ) const;
	void RemoveInstances( const CSRTBaseTree* baseTree, const Vector& position, Float radius ) const;
	void AddCollisions( const CSRTBaseTree* baseTree, const InstanceContainer& instances, const Box& box ) const;
	void AddCollisionsForInstance(const CSRTBaseTree* baseTree, const SFoliageInstance & newInstance);

	void Invalidate();

	void SetLodSetting( const SFoliageLODSetting & setting );

	void SetInternalDispatcher( const IFoliageRenderCommandDispatcher * dispatcher );
	void SetInternalCollisionHandler( CFoliageCollisionHandler * collisionHandler );
	
private:

	class GrassProxy
	{
	public:
		GrassProxy( CFoliageResourceHandler * handler, const CFoliageResource * foliage );
		~GrassProxy();

	private:
		CFoliageResourceHandler * m_handler;
		const CFoliageResource * m_foliage;
	};

	typedef Red::TSharedPtr< GrassProxy > GrassHandle;
	typedef Red::TWeakPtr< GrassProxy > WeakGrassHandle;

	typedef TPair< const CFoliageResource *, CFoliageResource::InstanceGroupContainer> PendingInstances;
	typedef TDynArray< PendingInstances > PendingInstancesContainer;
	typedef TDynArray< GrassHandle > VisibleGrassContainer;
	typedef TDynArray< TPair< const CFoliageResource *, WeakGrassHandle > > GrassContainer;
	typedef TDynArray< TPair< const CFoliageResource *, Int32 > > FoliagePartitionLevelContainer;

	// Refactoring Note. This object handle Collision and Rendering. SRP not respected.
	// I was thinking of extracting both collision and rendering into their respective class that inherit from 
	// an interface that the CFoliageResourceHandler could work on an array of. 

	void RemoveAllInstances( const CSRTBaseTree* baseTree, const Box & box );

	void AddAllCollision( const CSRTBaseTree* baseTree, const InstanceContainer& instances, const Box& box ) const;
	void RemoveAllCollision( const CSRTBaseTree* baseTree, const Box & box ) const;

	void ProcessPendingInstances( PendingInstancesContainer & pendingInstanceContainer, FoliageAddInstancesContainer & output );
	void ProcessAllPendingInstances( PendingInstancesContainer & pendingInstanceContainer, FoliageAddInstancesContainer & output );

	void UpdateGrassVisibility();
	void UpdateTreeVisibility();

	void AddGrass( const CFoliageResource * foliage );
	void RemoveGrass( const CFoliageResource * foliage );

	void UpdateCurrentLod( const CFoliageResource * foliage, Int32 currentLod, Int32 lod );
	bool IsGrassVisible( const CFoliageResource * foliage ) const;

	CFoliageCollisionHandler * m_collisionHandler;
	const IFoliageRenderCommandDispatcher * m_renderCommandDispatcher;

	PendingInstancesContainer m_pendingTreeInstanceContainer;
	PendingInstancesContainer m_pendingGrassInstanceContainer;
	PendingInstancesContainer m_pendingCollisionContainer;
	FoliageRemoveInstancesContainer m_pendingRemoveInstanceContainer;

	GrassContainer m_grassContainer;
	VisibleGrassContainer m_visibleGrassContainer;

	FoliagePartitionLevelContainer m_foliagePartitionLevelContainer;

	Vector m_currentPosition;
	Float m_currentGrassDistanceScaleParam;

	SFoliageLODSetting m_lodSetting;
};

Red::TUniquePtr< CFoliageResourceHandler > CreateFoliageResourceHandler( CFoliageCollisionHandler * collisionHandler, const IFoliageRenderCommandDispatcher * dispatcher );

#endif
