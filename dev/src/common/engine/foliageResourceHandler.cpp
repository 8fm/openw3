/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "foliageResourceHandler.h"
#include "foliageResource.h"
#include "foliageCollisionHandler.h"
#include "foliageRenderCommandDispatcher.h"
#include "foliageInstance.h"
#include "renderer.h"
#include "renderSettings.h"
#include "baseTree.h"

const Int32 MaxFoliageInstancePerFrame = 99999;
Float GrassVisibilityDistanceSquared = 92.0f * 92.0f;

CFoliageResourceHandler::GrassProxy::GrassProxy( CFoliageResourceHandler * handler, const CFoliageResource * foliage )
	:	m_handler( handler ),
		m_foliage( foliage )
{
	m_handler->AddGrass(m_foliage);	
}

CFoliageResourceHandler::GrassProxy::~GrassProxy()
{
	m_handler->RemoveGrass(m_foliage);
}

CFoliageResourceHandler::CFoliageResourceHandler()
	:	m_collisionHandler( nullptr ),
		m_renderCommandDispatcher( nullptr ),
		m_currentGrassDistanceScaleParam( 1.0f )
{}

CFoliageResourceHandler::~CFoliageResourceHandler()
{}


void CFoliageResourceHandler::DisplayFoliage( const CFoliageResource * foliage )
{
	m_foliagePartitionLevelContainer.PushBack( MakePair( foliage, FoliageLodCount ) );
	m_grassContainer.PushBack( MakePair( foliage, WeakGrassHandle() ) );

	if( IsGrassVisible( foliage ) )
	{
		UpdateGrassVisibility();
	}

	UpdateTreeVisibility();
}

bool CFoliageResourceHandler::IsGrassVisible( const CFoliageResource * foliage ) const
{
	const Vector center = foliage->GetGridBox().CalcCenter();
	return Abs( m_currentPosition.DistanceSquaredTo2D( center ) ) < GrassVisibilityDistanceSquared;
}

void CFoliageResourceHandler::AddInstances( const CSRTBaseTree* baseTree, const SFoliageInstanceGroup::Instances& instances, const Box& box ) const
{
	if( !instances.Empty() )
	{
		m_renderCommandDispatcher->CreateSpeedTreeInstancesCommand( baseTree, instances, box );
	}
}

void CFoliageResourceHandler::AddDynamicInstances( const CSRTBaseTree* baseTree, const SFoliageInstanceGroup::Instances& instances, const Box& box ) const
{
	m_renderCommandDispatcher->CreateSpeedTreeDynamicInstancesCommand( baseTree, instances, box );
}

void CFoliageResourceHandler::AddInstances( const CSRTBaseTree * baseTree, const InstanceContainer& instances ) const
{
	AddInstances( baseTree, instances, Box::EMPTY );
}

void CFoliageResourceHandler::RemoveInstances( const CSRTBaseTree* baseTree, const Box& box ) const
{
	RemoveAllCollision( baseTree, box );
	m_renderCommandDispatcher->RemoveSpeedTreeInstancesCommand( baseTree, box );
}

void CFoliageResourceHandler::RemoveInstances( const CSRTBaseTree* baseTree, const Vector& position, Float radius ) const
{
	Box box( position, radius );
	RemoveAllCollision( baseTree, box );
	m_renderCommandDispatcher->RemoveSpeedTreeInstancesRadiusCommand( baseTree, position, radius );
}

void CFoliageResourceHandler::RemoveDynamicInstances( const CSRTBaseTree* baseTree, const Vector& position, Float radius ) const
{
	Box box( position, radius );
	RemoveAllCollision( baseTree, box );
	m_renderCommandDispatcher->RemoveSpeedTreeDynamicInstancesRadiusCommand( baseTree, position, radius );
}

void CFoliageResourceHandler::AddCollisions( const CSRTBaseTree* baseTree, const InstanceContainer& instances, const Box& box ) const
{
	AddAllCollision( baseTree, instances, box );
}

void CFoliageResourceHandler::AddAllCollision( const CSRTBaseTree* baseTree, const InstanceContainer& instances, const Box& box) const
{
	m_collisionHandler->AddAllCollision( baseTree, instances, box );
}

void CFoliageResourceHandler::AddCollisionsForInstance(const CSRTBaseTree* baseTree, const SFoliageInstance & newInstance)
{
	m_collisionHandler->AddCollisionsForInstance( baseTree, newInstance );
}

void CFoliageResourceHandler::HideFoliage( const CFoliageResource * foliage )
{
	PC_CHECKER_SCOPE( 0.001f, TXT("FOLIAGE"), TXT("Slow HideFoliage") );

	PendingInstancesContainer::iterator iter = 
		FindIf( m_pendingTreeInstanceContainer.Begin(), m_pendingTreeInstanceContainer.End(), [&] (const PendingInstances& value) { return value.m_first == foliage; } );

	if( iter != m_pendingTreeInstanceContainer.End() )
	{
		m_pendingTreeInstanceContainer.Erase( iter );
	}

	GrassContainer::iterator grassIter = 
		FindIf( m_grassContainer.Begin(), m_grassContainer.End(), [&] (const TPair< const CFoliageResource *, WeakGrassHandle >& value) { return value.m_first == foliage; } );

	if( grassIter != m_grassContainer.End() )
	{
		m_grassContainer.Erase( grassIter );
		UpdateGrassVisibility();
	}

	FoliagePartitionLevelContainer::iterator partitionIter =
		FindIf( m_foliagePartitionLevelContainer.Begin(), m_foliagePartitionLevelContainer.End(), [&] (const TPair< const CFoliageResource *, Int32 >& value) { return value.m_first == foliage; } );

	if( partitionIter != m_foliagePartitionLevelContainer.End() )
	{
		m_foliagePartitionLevelContainer.Erase( partitionIter );
	}

	const CFoliageResource::BaseTreeContainer & baseTreeContainer = foliage->GetAllBaseTree();
	const Box & box = foliage->GetGridBox();
	for( auto baseTreeIter = baseTreeContainer.Begin(), baseTreeEnd = baseTreeContainer.End(); baseTreeIter != baseTreeEnd; ++baseTreeIter )
	{
		const CSRTBaseTree* baseTree = baseTreeIter->Get(); 
		RemoveAllCollision( baseTree, box );
		RemoveAllInstances( baseTree, box );
	}
}

void CFoliageResourceHandler::RemoveAllInstances( const CSRTBaseTree* baseTree, const Box & box )
{
	m_pendingRemoveInstanceContainer.PushBack( MakePair( baseTree->AcquireRenderObject(), box ) );
}

void CFoliageResourceHandler::RemoveAllCollision( const CSRTBaseTree* baseTree, const Box & box ) const
{
	if( baseTree && !baseTree->IsGrassType() )
	{
		m_collisionHandler->RemoveAllCollision( baseTree, box );
	}
}

void CFoliageResourceHandler::Tick()
{
	if( Config::cvGrassDistanceScale.Get() != m_currentGrassDistanceScaleParam )
	{
		m_currentGrassDistanceScaleParam = Config::cvGrassDistanceScale.Get();
		GrassVisibilityDistanceSquared = 92.0f * Max(m_currentGrassDistanceScaleParam, 1.0f);
		GrassVisibilityDistanceSquared *= GrassVisibilityDistanceSquared;
	}

	SFoliageUpdateRequest updateRequest;

	if( !m_pendingTreeInstanceContainer.Empty() )
	{
		ProcessAllPendingInstances( m_pendingTreeInstanceContainer, updateRequest.addRequestContainer );
	}
	else if( !m_pendingGrassInstanceContainer.Empty() )
	{
		ProcessAllPendingInstances( m_pendingGrassInstanceContainer, updateRequest.addRequestContainer );
	}

	updateRequest.removeRequestContainer = std::move( m_pendingRemoveInstanceContainer );

	m_renderCommandDispatcher->UpdateSpeedTreeInstancesCommand( updateRequest ); 

	if( !m_pendingCollisionContainer.Empty() )
	{
		for( auto pendingIter = m_pendingCollisionContainer.Begin(), end = m_pendingCollisionContainer.End(); pendingIter != end; ++pendingIter )
		{
			const PendingInstances & pendingInstances = *pendingIter;
			const CFoliageResource::InstanceGroupContainer & instanceContainer = pendingInstances.m_second;
			for( auto groupIter = instanceContainer.Begin(), groupEnd = instanceContainer.End(); groupIter != groupEnd; ++groupIter )
			{
				const SFoliageInstanceGroup & group = * groupIter; 
				const CSRTBaseTree * baseTree = group.baseTree.Get();
				const FoliageInstanceContainer& instances = group.instances;
			
				AddAllCollision( baseTree, instances, pendingInstances.m_first->GetGridBox() );
			}
		}

		m_pendingCollisionContainer.ClearFast();
	}
}

void CFoliageResourceHandler::ProcessPendingInstances( PendingInstancesContainer & pendingInstanceContainer, FoliageAddInstancesContainer & output )
{
	output.Reserve( pendingInstanceContainer.Size() + output.Size() );

	Int32 instanceCount = 0;

	PendingInstances & pendingInstances = pendingInstanceContainer.Back();
	const CFoliageResource * foliageResource = pendingInstances.m_first;
	const Box & box = foliageResource->GetGridBox();
	CFoliageResource::InstanceGroupContainer & instanceContainer = pendingInstances.m_second;
	
	do 
	{
		SFoliageInstanceGroup group = instanceContainer.PopBackFast();
		const CSRTBaseTree * baseTree = group.baseTree.Get();
		const SFoliageInstanceGroup::Instances & instances = group.instances;

		FoliageTreeInstances treeInstances;
		treeInstances.tree = baseTree->AcquireRenderObject();
		treeInstances.instances = instances;
		treeInstances.box = box;

		output.PushBack( std::move( treeInstances ) );

		instanceCount += instances.Size();

	} while ( instanceCount < MaxFoliageInstancePerFrame && !instanceContainer.Empty() );

	if( instanceContainer.Empty() )
	{
		pendingInstanceContainer.PopBackFast();
	}
}

void CFoliageResourceHandler::ProcessAllPendingInstances( PendingInstancesContainer & pendingInstanceContainer, FoliageAddInstancesContainer & output )
{
	output.Reserve( pendingInstanceContainer.Size() + output.Size() );

	for( auto pendingIter = pendingInstanceContainer.Begin(), pendingEnd = pendingInstanceContainer.End(); pendingIter != pendingEnd; ++pendingIter )
	{
		const PendingInstances & pendingInstances = *pendingIter;
		const CFoliageResource::InstanceGroupContainer & instanceContainer = pendingInstances.m_second;
		const CFoliageResource * foliageResource = pendingInstances.m_first;
		const Box & box = foliageResource->GetInstancesBoundingBox();
		for( auto groupIter = instanceContainer.Begin(), groupEnd = instanceContainer.End(); groupIter != groupEnd; ++groupIter )
		{
			const SFoliageInstanceGroup & group = * groupIter; 
			const CSRTBaseTree * baseTree = group.baseTree.Get();
			const FoliageInstanceContainer& instances = group.instances;
			
			if( !instances.Empty() )
			{
				FoliageTreeInstances treeInstances;
				treeInstances.tree = baseTree->AcquireRenderObject();
				treeInstances.instances = instances;
				treeInstances.box = box;

				output.PushBack( std::move( treeInstances ) );
			}
		}
	}

	pendingInstanceContainer.ClearFast();
}

void CFoliageResourceHandler::AddGrass( const CFoliageResource * foliage )
{
	CFoliageResource::InstanceGroupContainer instanceContainer = foliage->GetAllGrassInstances();
	if( !instanceContainer.Empty() )
	{
		m_pendingGrassInstanceContainer.PushBack( PendingInstances( foliage, instanceContainer ) );
	}
}

void CFoliageResourceHandler::RemoveGrass( const CFoliageResource * foliage )
{
	PendingInstancesContainer::iterator iter = 
		FindIf( m_pendingGrassInstanceContainer.Begin(), m_pendingGrassInstanceContainer.End(), [&] (const PendingInstances& value) { return value.m_first == foliage; } );

	if( iter != m_pendingGrassInstanceContainer.End() )
	{
		m_pendingGrassInstanceContainer.Erase( iter );
	}

	CFoliageResource::BaseTreeContainer baseTreeContainer = foliage->GetAllBaseTree();
	const Box & box = foliage->GetGridBox();
	for( auto iter = baseTreeContainer.Begin(), end = baseTreeContainer.End(); iter != end; ++iter )
	{
		const CSRTBaseTree* baseTree = iter->Get(); 
		if( baseTree->IsGrassType() )
		{
			RemoveAllInstances( baseTree, box );
		}
	}
}

void CFoliageResourceHandler::UpdateCurrentPosition( const Vector & position )
{
	m_currentPosition = position;
	UpdateGrassVisibility();
	UpdateTreeVisibility();
}

void CFoliageResourceHandler::UpdateGrassVisibility()
{
	VisibleGrassContainer container;
	container.Reserve( m_visibleGrassContainer.Size() );
	for( auto iter = m_grassContainer.Begin(), end = m_grassContainer.End(); iter != end; ++iter )
	{
		const CFoliageResource * foliage = iter->m_first;;
		if( IsGrassVisible( foliage ) )
		{
			GrassHandle handle = iter->m_second.Lock();
			if( !handle )
			{
				handle.Reset( new GrassProxy( this, foliage ) );
				iter->m_second = handle;
			} 
			container.PushBack( handle );
		}
	}

	m_visibleGrassContainer = container;
}

void CFoliageResourceHandler::UpdateTreeVisibility()
{
	PC_CHECKER_SCOPE( 0.001f, TXT("FOLIAGE"), TXT("Slow UpdateTreeVisibility") );

	for( auto iter = m_foliagePartitionLevelContainer.Begin(), end = m_foliagePartitionLevelContainer.End(); iter != end; ++iter )
	{
		const CFoliageResource * foliage = iter->m_first;
		const Int32 currentLod = iter->m_second;
		const Box & box = foliage->GetGridBox();
		const Vector center = box.CalcCenter();
		const Float distance = Abs( m_currentPosition.DistanceSquaredTo2D( center ) );
		Float lodStep = 64.0f;
		const Float lodOffset = 32.0f;

		for( Int32 lod = 0; lod != FoliageLodCount; ++lod )
		{
			const Float lodDistance = lodStep + lodOffset;
			const Float lodDistanceSqaured = lodDistance * lodDistance;

			if( distance < lodDistanceSqaured )
			{
				UpdateCurrentLod( foliage, currentLod, lod );
				iter->m_second = lod;
				break;
			}

			lodStep *= 2.0f;
		}
	}
}

void CFoliageResourceHandler::UpdateCurrentLod( const CFoliageResource * foliage, Int32 currentLod, Int32 lod )
{
	const CFoliageResource::InstanceGroupContainer & container = foliage->GetAllTreeInstances();

	if( currentLod < lod )
	{
		const Float minExtent = m_lodSetting.m_minTreeExtentPerLod[ lod ];
		const Box & box = foliage->GetGridBox();

		for( const SFoliageInstanceGroup& group : container )
		{
			const CSRTBaseTree * baseTree = group.baseTree;
			const Float baseTreeMaxExtent = baseTree->GetBBox().Max.Z;
			if( baseTreeMaxExtent < minExtent )
			{
				RemoveAllInstances( baseTree, box );
			}

			if( currentLod == 0 )
			{
				RemoveAllCollision( baseTree, box );
			}
		}
	}
	else if( currentLod > lod )
	{
		CFoliageResource::InstanceGroupContainer groupContainer;
		CFoliageResource::InstanceGroupContainer collisionGroupContainer;

		for( const SFoliageInstanceGroup& group : container )
		{
			const CSRTBaseTree * baseTree = group.baseTree;
			const Float baseTreeMaxExtent = baseTree->GetBBox().Max.Z;
			const Float minExtent = lod == 0 ? 0.0f : m_lodSetting.m_minTreeExtentPerLod[ lod ];
			const Float maxExtent = currentLod == 7 ? NumericLimits< Float >::Max() : m_lodSetting.m_minTreeExtentPerLod[ currentLod ];

			if( baseTreeMaxExtent > minExtent && baseTreeMaxExtent < maxExtent )
			{
				groupContainer.PushBack( group );
			}

			if( lod == 0 )
			{
				collisionGroupContainer.PushBack( group );
			}
		}

		if( !groupContainer.Empty() )
		{
			m_pendingTreeInstanceContainer.PushBack( PendingInstances( foliage, groupContainer ) );
		}

		if( !collisionGroupContainer.Empty() )
		{
			m_pendingCollisionContainer.PushBack( PendingInstances( foliage, collisionGroupContainer ) );
		}
	}
}

bool CFoliageResourceHandler::InstancesPendingInsertion() const
{
	return !m_pendingTreeInstanceContainer.Empty();
}

void CFoliageResourceHandler::Invalidate()
{
	FoliagePartitionLevelContainer allFoliageTile = m_foliagePartitionLevelContainer;
	for( auto iter : allFoliageTile )
	{
		HideFoliage( iter.m_first );
	}

	for( auto iter : allFoliageTile )
	{
		DisplayFoliage( iter.m_first );
	}

	UpdateGrassVisibility();
}

void CFoliageResourceHandler::Wait()
{
	SFoliageUpdateRequest updatRequest;

	ProcessAllPendingInstances( m_pendingTreeInstanceContainer, updatRequest.addRequestContainer );
	ProcessAllPendingInstances( m_pendingGrassInstanceContainer, updatRequest.addRequestContainer );
	updatRequest.removeRequestContainer = std::move( m_pendingRemoveInstanceContainer );

	m_renderCommandDispatcher->UpdateSpeedTreeInstancesCommand( updatRequest );
}

void CFoliageResourceHandler::SetLodSetting( const SFoliageLODSetting & setting )
{
	m_lodSetting = setting;
}

void CFoliageResourceHandler::SetInternalDispatcher( const IFoliageRenderCommandDispatcher * dispatcher )
{
	m_renderCommandDispatcher = dispatcher;
}

void CFoliageResourceHandler::SetInternalCollisionHandler( CFoliageCollisionHandler * collisionHandler )
{
	m_collisionHandler = collisionHandler;
}

Red::TUniquePtr< CFoliageResourceHandler > CreateFoliageResourceHandler( CFoliageCollisionHandler * collisionHandler, const IFoliageRenderCommandDispatcher * dispatcher )
{
	Red::TUniquePtr< CFoliageResourceHandler > handler( new CFoliageResourceHandler );
	handler->SetInternalDispatcher( dispatcher );
	handler->SetInternalCollisionHandler( collisionHandler );
	return handler;
}
