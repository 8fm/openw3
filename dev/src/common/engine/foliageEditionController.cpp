/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "foliageEditionController.h"

#include "foliageScene.h"
#include "foliageBroker.h"
#include "foliageCell.h"
#include "foliageResourceLoader.h"
#include "foliageRenderCommandDispatcher.h"
#include "foliageResourceHandler.h"
#include "foliageInstance.h"
#include "foliageCellIterator.h"

#include "grassMask.h"
#include "clipMap.h"
#include "baseTree.h"

#include "../core/feedback.h"
#include "../core/diskFile.h"
#include "../core/directory.h"
#include "../core/resource.h"
#include "../physics/physicsWorld.h"
#include "foliageCollisionHandler.h"

IMPLEMENT_ENGINE_CLASS( SFoliageInstanceStatistics )

const Float GSearchRadiusMax = 8192.0f;
const Uint32 GMaxInstanceInsertion = 3000;

SFoliageInstanceCollection::SFoliageInstanceCollection( THandle< CSRTBaseTree > baseTree ) 
	: m_baseTree( baseTree ) 
{}

SFoliageInstanceCollection::~SFoliageInstanceCollection()
{}

SFoliageInstanceStatistics::SFoliageInstanceStatistics( CSRTBaseTree* baseTree, Uint32 instanceCount ) 
	: m_baseTree( baseTree ), m_instanceCount( instanceCount ) 
{}

class CFoliageEditionController::AddTransaction : public CFoliageEditionController::ITransaction
{
public:

	AddTransaction( const CFoliageResourceHandler * resourceHandler, const CSRTBaseTree * baseTree, const CellHandleContainer & cells, const FoliageInstanceContainer& instances, const Box& rect )
		:	ITransaction( rect ),
			m_resourceHandler( resourceHandler ),
			m_baseTree( baseTree )
	{
		for( auto cellIter = cells.Begin(), cellEnd = cells.End(); cellIter != cellEnd; ++cellIter )
		{
			FoliageInstanceContainer instancesPerCell;
			CFoliageResource * foliageResource = (*cellIter)->AcquireFoliageResource();

			if( foliageResource->MarkModified() )
			{
				for( auto instanceIter = instances.Begin(), instanceEnd = instances.End(); instanceIter != instanceEnd; ++instanceIter )
				{
					if( foliageResource->CanInsertInstance( *instanceIter ) )
					{
						instancesPerCell.PushBack( *instanceIter );
					}
				}

				if( !instancesPerCell.Empty() )
				{
					m_instances.PushBack( MakePair( *cellIter, instancesPerCell ) );
				}
			}
		}
	}

	virtual void Apply() final
	{
		for( auto iter = m_instances.Begin(), end = m_instances.End(); iter != end; ++iter )
		{
			CFoliageResource * resource = iter->m_first->AcquireFoliageResource();
			const FoliageInstanceContainer & container = iter->m_second;

			RED_FATAL_ASSERT( container.Size() <= GMaxInstanceInsertion, "Trying to insert %d instances in one stroke!", container.Size() );
				
			for( auto instanceIter = container.Begin(), instanceEnd = container.End(); instanceIter != instanceEnd; ++instanceIter )
			{
				resource->InsertInstance( m_baseTree, *instanceIter );
			}

			m_resourceHandler->AddInstances( m_baseTree, container, resource->GetGridBox() );
		}
	}

	virtual void Undo() final
	{
		for( auto iter = m_instances.Begin(), end = m_instances.End(); iter != end; ++iter )
		{
			CFoliageResource * resource = iter->m_first->AcquireFoliageResource();
			const FoliageInstanceContainer & container = iter->m_second;
			for( auto instanceIter = container.Begin(), instanceEnd = container.End(); instanceIter != instanceEnd; ++instanceIter )
			{
				resource->RemoveInstance( m_baseTree, *instanceIter );
			}

			m_resourceHandler->RemoveInstances( m_baseTree, resource->GetGridBox() );
			FoliageInstanceContainer remainingInstance;
			resource->GetInstancesFromArea( m_baseTree, resource->GetGridBox(), remainingInstance );
			m_resourceHandler->AddInstances(m_baseTree, remainingInstance, resource->GetGridBox() );
		}
	}

private:

	const CFoliageResourceHandler * m_resourceHandler;
	const CSRTBaseTree * m_baseTree;
	TDynArray< TPair< CellHandle, FoliageInstanceContainer > > m_instances;
};

class CFoliageEditionController::RemoveTransaction : public CFoliageEditionController::ITransaction
{
public:

	RemoveTransaction( const CFoliageResourceHandler * resourceHandler, const CSRTBaseTree* baseTree, const CellHandleContainer & cells, const Vector& center, Float radius )
		:	ITransaction( Box( center, radius ) ),
			m_resourceHandler( resourceHandler ),
			m_baseTree( baseTree )
	{
		for( auto cellIter = cells.Begin(), cellEnd = cells.End(); cellIter != cellEnd; ++cellIter )
		{
			CFoliageResource * foliageResource = (*cellIter)->GetFoliageResource();
			if( foliageResource && foliageResource->MarkModified() )
			{
				FoliageInstanceContainer instancesPerCell;
				foliageResource->GetInstancesFromArea( m_baseTree, center, radius, instancesPerCell );

				if( !instancesPerCell.Empty() )
				{
					m_instances.PushBack( MakePair( *cellIter, instancesPerCell ) );
				}
			}
		}
	}

	virtual void Apply() final
	{
		for( auto iter = m_instances.Begin(), end = m_instances.End(); iter != end; ++iter )
		{
			CFoliageResource * resource = iter->m_first->GetFoliageResource();
			const FoliageInstanceContainer & container = iter->m_second;
			for( auto instanceIter = container.Begin(), instanceEnd = container.End(); instanceIter != instanceEnd; ++instanceIter )
			{
				resource->RemoveInstance( m_baseTree, *instanceIter );
			}

			m_resourceHandler->RemoveInstances( m_baseTree, resource->GetGridBox() );
			FoliageInstanceContainer remainingInstance;
			resource->GetInstancesFromArea( m_baseTree, resource->GetGridBox(), remainingInstance );
			m_resourceHandler->AddInstances(m_baseTree, remainingInstance, resource->GetGridBox() );
		}
	}

	virtual void Undo() final
	{
		for( auto iter = m_instances.Begin(), end = m_instances.End(); iter != end; ++iter )
		{
			CFoliageResource * resource = iter->m_first->GetFoliageResource();
			const FoliageInstanceContainer & container = iter->m_second;
			for( auto instanceIter = container.Begin(), instanceEnd = container.End(); instanceIter != instanceEnd; ++instanceIter )
			{
				resource->InsertInstance( m_baseTree, *instanceIter );
			}

			m_resourceHandler->AddInstances( m_baseTree, container, resource->GetGridBox() );
		}
	}

private:

	const CFoliageResourceHandler * m_resourceHandler;
	const CSRTBaseTree * m_baseTree;
	TDynArray< TPair< CellHandle, FoliageInstanceContainer > > m_instances;
};

CFoliageEditionController::CFoliageEditionController()
	:	m_foliageScene( nullptr ),
		m_renderCommandDispatcher( nullptr ),
		m_resourceHandler( nullptr ),
		m_collisionHandler( nullptr ),
		m_transaction( 0 )
{}

CFoliageEditionController::~CFoliageEditionController()
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().UnregisterListener( CNAME( PreReloadBaseTree ), this );
	SEvents::GetInstance().UnregisterListener( CNAME( ReloadBaseTree ), this );
#endif
}

void CFoliageEditionController::Setup( const SFoliageEditionControllerSetupParameter & param )
{
	m_worldDimension = param.worldDimension;
	m_cellDimension = param.cellDimension;
	m_foliageScene = param.foliageScene;
	m_renderCommandDispatcher = param.renderCommandDispatcher;
	m_resourceHandler = param.resourceHandler;
	m_collisionHandler = param.collisionHandler;
	m_foliageBroker = param.foliageBroker;

#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().RegisterListener( CNAME( PreReloadBaseTree ), this );
	SEvents::GetInstance().RegisterListener( CNAME( ReloadBaseTree ), this );
#endif
}

Bool CFoliageEditionController::AddInstances( const CSRTBaseTree* baseTree, const FoliageInstanceContainer& instances )
{
	Box worldRect = Box::EMPTY;

	for ( const SFoliageInstance& inst : instances )
	{
		worldRect.AddPoint( inst.GetPosition() );
	}

	CellHandleContainer cellContainer = m_foliageBroker->AcquireCells( worldRect );
	if ( AcquireResourcesAndAddToModified( cellContainer ) )
	{
		TransactionHandle transaction( new AddTransaction( m_resourceHandler, baseTree, cellContainer, instances, worldRect ) );
		transaction->Apply();
		m_transactionContainer.PushBack( MakePair( m_transaction, transaction ) );

		m_resourceHandler->AddCollisions(baseTree, instances, worldRect );

		return true;
	}
	else
	{
		return false;
	}
}

Bool CFoliageEditionController::RemoveInstances( const CSRTBaseTree* baseTree, const Vector& center, Float radius )
{
	CellHandleContainer cellContainer = m_foliageBroker->AcquireCells( Box( center, radius ) );
	if ( AcquireResourcesAndAddToModified( cellContainer ) )
	{
		TransactionHandle transaction( new RemoveTransaction( m_resourceHandler, baseTree, cellContainer, center, radius ) );
		transaction->Apply();
		m_transactionContainer.PushBack( MakePair( m_transaction, transaction ) );

		return true;
	}
	else
	{
		return false;
	}
}

void CFoliageEditionController::UndoTransaction()
{
	auto iter = FindIf( 
		m_transactionContainer.Begin(), 
		m_transactionContainer.End(), 
		[=]( const TransactionPair & pair ){ return pair.m_first == m_transaction; } );

	Box totalAffectedRect = Box::EMPTY;

	if( iter != m_transactionContainer.End() )
	{
		for( ; iter != m_transactionContainer.End() && iter->m_first == m_transaction; ++iter )
		{
			iter->m_second->Undo();
			totalAffectedRect.AddBox( iter->m_second->affectedRect );
		}
	}

	AcquireResourcesAndAddToModified( totalAffectedRect );

	--m_transaction;
}

void CFoliageEditionController::RedoTransaction()
{
	++m_transaction;

	auto iter = FindIf( 
		m_transactionContainer.Begin(), 
		m_transactionContainer.End(), 
		[=]( const TransactionPair & pair ){ return pair.m_first == m_transaction; } );

	Box totalAffectedRect = Box::EMPTY;

	if( iter != m_transactionContainer.End() )
	{
		for( ; iter != m_transactionContainer.End() && iter->m_first == m_transaction; ++iter )
		{
			iter->m_second->Apply();
			totalAffectedRect.AddBox( iter->m_second->affectedRect );
		}
	}

	AcquireResourcesAndAddToModified( totalAffectedRect );
}

void CFoliageEditionController::FlushTransaction()
{
	m_transactionContainer.Erase( 
		RemoveIf( m_transactionContainer.Begin(), m_transactionContainer.End(), [=](const TransactionPair & pair){ return pair.m_first > m_transaction; } ),
		m_transactionContainer.End() );
}

void CFoliageEditionController::AddPreviewInstances( const CSRTBaseTree * baseTree, const FoliageInstanceContainer& instanceInfos, const Box& box )
{
	m_renderCommandDispatcher->CreateSpeedTreeInstancesCommand( baseTree, instanceInfos, box );
}

void CFoliageEditionController::RemovePreviewInstances( const CSRTBaseTree * baseTree, const Box& box )
{
	m_renderCommandDispatcher->RemoveSpeedTreeInstancesCommand( baseTree, box );
}

Bool CFoliageEditionController::GetClosestInstance( const Vector& position, CSRTBaseTree*& outTree, SFoliageInstance & outInstance )
{
	const Float searchRadius = 3.0f; // Could this be somewhere else ? 
	
	CSRTBaseTree* closestSRT = nullptr;
	Float closestDistance = FLT_MAX;
	SFoliageInstance closestInstance;

	CellHandleContainer cellContainer = m_foliageBroker->AcquireCells( Box( position, searchRadius ) );

	for( auto cellIter = cellContainer.Begin(), cellEnd = cellContainer.End(); cellIter != cellEnd; ++cellIter )
	{
		CFoliageResource * foliageResource = (*cellIter)->GetFoliageResource();
		if( foliageResource )
		{
			const CFoliageResource::BaseTreeContainer & baseTreeContainer = foliageResource->GetAllBaseTree();
			for( auto baseTreeIter = baseTreeContainer.Begin(), baseTreeEnd = baseTreeContainer.End(); baseTreeIter != baseTreeEnd; ++baseTreeIter )
			{
				FoliageInstanceContainer instances;
				GetInstancesFromArea( baseTreeIter->Get(), position, searchRadius, instances );
				for ( Uint32 i=0; i<instances.Size(); ++i )
				{
					const Float distance = position.DistanceSquaredTo2D( instances[i].GetPosition() );
					if ( distance < closestDistance )
					{
						closestSRT = baseTreeIter->Get();
						outInstance = instances[i];
						closestDistance = distance;
					}
				}
			}
		}
	}
	
	if( closestSRT )
	{
		outTree = closestSRT;
		return true;
	}

	return false;
}

Box CFoliageEditionController::GetCellExtend( const Vector& position ) const
{
	CellHandle cell = m_foliageBroker->AcquireCell( position );
	ASSERT( cell );
	return Box( cell->GetWorldCoordinate(), cell->GetWorldCoordinate() + m_cellDimension );
}

void CFoliageEditionController::GetInstancesFromArea( const CSRTBaseTree* baseTree, const Vector& center, Float radius, FoliageInstanceContainer& instances )
{
	const CellHandleContainer & cellContainer = m_foliageBroker->GetVisibleCells( );

	for ( auto cellIter = cellContainer.Begin(), cellEnd = cellContainer.End(); cellIter != cellEnd; ++cellIter )
	{
		if ( CFoliageResource * foliageResource = (*cellIter)->GetFoliageResource() )
		{
			foliageResource->GetInstancesFromArea( baseTree, center, radius, instances );
		}
	}
}

void CFoliageEditionController::GetInstancesFromArea( const CSRTBaseTree* baseTree, const Box& box, FoliageInstanceContainer& instances )
{
	const CellHandleContainer & cellContainer = m_foliageBroker->GetVisibleCells( );

	for ( auto cellIter = cellContainer.Begin(), cellEnd = cellContainer.End(); cellIter != cellEnd; ++cellIter )
	{
		if ( CFoliageResource * foliageResource = (*cellIter)->GetFoliageResource() )
		{
			foliageResource->GetInstancesFromArea( baseTree, box, instances );
		}
	}
}

void CFoliageEditionController::GetInstancesFromArea( const Vector& center, Float radius, TDynArray< SFoliageInstanceCollection >& instances )
{
	const CellHandleContainer & cellContainer = m_foliageBroker->GetVisibleCells( );

	for ( auto cellIter = cellContainer.Begin(), cellEnd = cellContainer.End(); cellIter != cellEnd; ++cellIter )
	{
		if ( CFoliageResource * foliageResource = (*cellIter)->GetFoliageResource() )
		{
			const CFoliageResource::BaseTreeContainer & baseTreeContainer = foliageResource->GetAllBaseTree();
			for( auto baseTreeIter = baseTreeContainer.Begin(), baseTreeEnd = baseTreeContainer.End(); baseTreeIter != baseTreeEnd; ++baseTreeIter )
			{
				SFoliageInstanceCollection collection( *baseTreeIter );
				foliageResource->GetInstancesFromArea( baseTreeIter->Get(), center, radius, collection.m_instances );
				if( !collection.m_instances.Empty() )
				{
					instances.PushBack( collection );
				}
			}
		}
	}
}

void CFoliageEditionController::GetInstancesFromArea( const Box& box, TDynArray< SFoliageInstanceCollection >& instances )
{
	const CellHandleContainer & cellContainer = m_foliageBroker->GetVisibleCells( );

	for ( auto cellIter = cellContainer.Begin(), cellEnd = cellContainer.End(); cellIter != cellEnd; ++cellIter )
	{
		if ( CFoliageResource * foliageResource = (*cellIter)->GetFoliageResource() )
		{
			const CFoliageResource::BaseTreeContainer & baseTreeContainer = foliageResource->GetAllBaseTree();
			for( auto baseTreeIter = baseTreeContainer.Begin(), baseTreeEnd = baseTreeContainer.End(); baseTreeIter != baseTreeEnd; ++baseTreeIter )
			{
				SFoliageInstanceCollection collection( *baseTreeIter );
				collection.m_cellHandle = *cellIter;
				foliageResource->GetInstancesFromArea( baseTreeIter->Get(), box, collection.m_instances );
				if( !collection.m_instances.Empty() )
				{
					instances.PushBack( collection );
				}
			}
		}
	}
}

Bool CFoliageEditionController::ReplantInstance( const CSRTBaseTree* baseTree, const SFoliageInstance & oldInstance, const SFoliageInstance & newInstance )
{
	CellHandle oldCell = m_foliageBroker->AcquireCell( oldInstance.GetPosition().AsVector2() );
	CellHandle newCell = m_foliageBroker->AcquireCell( newInstance.GetPosition().AsVector2() );

	CFoliageResource * oldResource = oldCell->AcquireFoliageResource();
	CFoliageResource * newResource = newCell->AcquireFoliageResource();

	if( oldResource->MarkModified() && newResource->MarkModified() )
	{
		oldResource->RemoveInstance( baseTree, oldInstance );
		newResource->InsertInstance( baseTree, newInstance );

		Box box = Box::EMPTY;
		box.AddPoint( oldInstance.GetPosition() );
		box.AddPoint( newInstance.GetPosition() );
		RefreshVisibility( baseTree, box );

		m_editedCells.PushBackUnique( oldCell );
		m_editedCells.PushBackUnique( newCell );

		m_resourceHandler->AddCollisionsForInstance(baseTree, newInstance);

		return true;
	}

	return false;
}

void CFoliageEditionController::RefreshVisibility( const TDynArray< const CSRTBaseTree* >& baseTrees, const Box& box )
{
	for( auto iter = baseTrees.Begin(), end = baseTrees.End(); iter != end; ++iter )
	{
		RefreshVisibility( *iter, box );
	}
}

void CFoliageEditionController::RefreshVisibility( const CSRTBaseTree* baseTree, const Box& box )
{
	m_resourceHandler->RemoveInstances( baseTree, box );

	FoliageInstanceContainer instances;
	GetInstancesFromArea( baseTree, box, instances );

	m_resourceHandler->AddInstances( baseTree, instances );
}

Bool CFoliageEditionController::ResizeInstances( const CSRTBaseTree* baseTree, const Vector& center, Float radius, Float value, Bool shrink )
{
	FoliageInstanceContainer oldInstances;
	FoliageInstanceContainer newInstances;
	GetInstancesFromArea( baseTree, center, radius, oldInstances );

	if ( oldInstances.Empty() )
	{
		return true; // nothing to do
	}

	newInstances.Reserve( oldInstances.Size() );

	if ( AcquireResourcesAndAddToModified( Box( center, radius ) ) )
	{
		m_resourceHandler->RemoveInstances( baseTree, center, radius );

		const Float invDistance = 1.0f / radius;
		for( auto iter = oldInstances.Begin(), end = oldInstances.End(); iter != end; ++iter )
		{
			const SFoliageInstance & oldInstance = *iter;
			SFoliageInstance newInstance = *iter;

			const Vector& position = newInstance.GetPosition();
			const Float distance = position.DistanceTo2D( center ) * invDistance;

			if ( distance < 1.0f )
			{
				// Calculate brush intensity
				const Float intensity = 1.f + ( value * MSqrt( 1.f - distance ) );
				newInstance.MultiplySize( shrink ? 1.f / intensity : intensity );
			
				CellHandle cellHandle = m_foliageBroker->AcquireCell( position );
				CFoliageResource * foliageResource = cellHandle->GetFoliageResource();
				if( foliageResource )
				{
					foliageResource->RemoveInstance( baseTree, oldInstance );
					foliageResource->InsertInstance( baseTree, newInstance );
				}
			}

			newInstances.PushBack( newInstance );
		}

		m_resourceHandler->AddInstances( baseTree, newInstances );

		return true;
	}

	return false;
	
}

Bool CFoliageEditionController::PerformSilentCheckOutOnResource( CResource& res )
{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	if ( CDiskFile* file = res.GetFile() )
	{
		file->GetStatus(); // force to update the p4 status

		if ( file->IsLocal() || file->IsCheckedOut() )
		{
			return res.MarkModified();
		}
		else
		{
			if ( file->SilentCheckOut() )
			{
				return res.MarkModified();
			}
			else
			{
				return false;
			}
		}
	}
	else
#endif
	{
		return false;
	}
}

Uint32 CFoliageEditionController::ReplaceTree( const CSRTBaseTree* oldBaseTree, const CSRTBaseTree* newBaseTree, const Box& box, Bool showProgress, FoliageInstanceFilter filter, const Float* resetScale )
{
	Uint32 cellsProcessed = 0;

	if ( newBaseTree != nullptr )
	{
		ASSERT ( oldBaseTree->GetType() == newBaseTree->GetType(), TXT("Tree to replace has to be of the same type") );

		if ( oldBaseTree->GetType() != newBaseTree->GetType() )
		{
			return 0;
		}
	}

	Uint32 replacedInstancesCount = 0;

	if ( showProgress )
	{
		GFeedback->BeginTask( TXT("Replacing tree type"), false );
	}

	for ( CFoliageCellIterator cell = m_foliageBroker->GetCellIterator( box ); cell; ++cell )
	{
		if ( cell->IsResourceValid() )
		{
			cell->Wait(); // wait for resource to load
			CFoliageResource* res = cell->GetFoliageResource();
			FoliageInstanceContainer outInstances;
			res->GetInstancesFromArea( oldBaseTree, box, outInstances );

			// If new base tree is null, it means we are removing tree instances, 
			// and since there might be resources that are not represented by any instances,
			// we want to remove them from the resource container anyway
			if ( ( newBaseTree == nullptr || !outInstances.Empty() ) && PerformSilentCheckOutOnResource( *res ) )
			{
				Uint32 instancesReplacedHere = res->ReplaceBaseTree( oldBaseTree, newBaseTree, filter, resetScale );

				if ( instancesReplacedHere != 0 )
				{
					m_editedCells.PushBackUnique( cell.Get() );
				}

				replacedInstancesCount += instancesReplacedHere;
			}
		}

		GFeedback->UpdateTaskProgress( ++cellsProcessed, cell.GetTotalCellCount() );
	}

	RefreshVisibility( oldBaseTree, box );

	if ( newBaseTree )
	{
		RefreshVisibility( newBaseTree, box );
	}

	m_resourceHandler->Invalidate();

	if ( showProgress )
	{
		GFeedback->EndTask();
	}

	return replacedInstancesCount;
}

namespace
{
	void ExtractStatistics( const CFoliageResource::InstanceGroupContainer & container, TDynArray< SFoliageInstanceStatistics >& statistics )
	{
		for( auto iter = container.Begin(), end = container.End(); iter != end; ++iter )
		{
			const SFoliageInstanceGroup & group = *iter;

			CSRTBaseTree * baseTree = group.baseTree.Get();
			const Uint32 count = group.instances.Size();
			auto statsIt = 
				FindIf( statistics.Begin(), statistics.End(),
				[ baseTree ]( const SFoliageInstanceStatistics& stats ) { return baseTree == stats.m_baseTree.Get(); }
			);

			if ( statsIt != statistics.End() )
			{
				statsIt->m_instanceCount += count;
			}
			else
			{
				statistics.PushBack( SFoliageInstanceStatistics( baseTree, count ) );
			}
		}
	}

	void ExtractPickShapes( const CFoliageResource::InstanceGroupContainer & container, TDynArray< SFoliagePickShape >& pickShapes )
	{
		for ( const SFoliageInstanceGroup& group : container )
		{
			Vector baseSize = group.baseTree->GetBBox().CalcSize();

			for ( const SFoliageInstance& instance : group.instances )
			{
				SFoliagePickShape shape;
				shape.m_baseTree = group.baseTree;
				shape.m_instance = instance;
				shape.m_shape = Cylinder( instance.GetPosition(), 
										  instance.GetPosition() + Vector3( 0., 0., baseSize.Z * instance.GetScale() ),
										  Min( baseSize.X, baseSize.Y ) * 0.3f * instance.GetScale() );
				pickShapes.PushBack( shape );
			}
		}
	}

}

Uint32 CFoliageEditionController::GetStatisticsFromArea( const Box& box, TDynArray< SFoliageInstanceStatistics >& statistics, Bool showProgress )
{
	if ( showProgress )
	{
		GFeedback->BeginTask( TXT("Gathering statistics"), false );
	}

	Uint32 cellCount = 0;

	for ( CFoliageCellIterator cell = m_foliageBroker->GetCellIterator( box ); cell; ++cell )
	{
		++cellCount;

		if ( cell->IsResourceValid() )
		{
			cell->Wait(); // wait for resource to load
			if ( CFoliageResource* res = cell->GetFoliageResource() )
			{
				ExtractStatistics( res->GetAllTreeInstances(), statistics );
				ExtractStatistics( res->GetAllGrassInstances(), statistics );
			}
		}
	}

	if ( showProgress )
	{
		GFeedback->EndTask();
	}

	return cellCount;
}

Uint32 CFoliageEditionController::GetPickShapesFromArea( const Box& box, TDynArray< SFoliagePickShape >& pickShapes )
{
	Uint32 cellCount = 0;

	for ( CFoliageCellIterator cell = m_foliageBroker->GetCellIterator( box ); cell; ++cell )
	{
		++cellCount;

		if ( cell->IsResourceValid() )
		{
			cell->Wait(); // wait for resource to load
			if ( CFoliageResource* res = cell->GetFoliageResource() )
			{
				ExtractPickShapes( res->GetAllTreeInstances(), pickShapes );
				ExtractPickShapes( res->GetAllGrassInstances(), pickShapes );
			}
		}
	}

	return cellCount;
}

void CFoliageEditionController::UpdateGrassMask( IRenderProxy * terrainProxy )
{
	m_foliageScene->UploadGenericGrassData( terrainProxy );
}

void CFoliageEditionController::CreateGrassMask( const CWorld * world )
{
	if( !m_foliageScene->GetInternalGrassMask() )
	{
		CGenericGrassMask * grassMask = CreateObject< CGenericGrassMask >( m_foliageScene );
		m_foliageScene->SetGenericGrassMask( grassMask );

#ifndef NO_RESOURCE_IMPORT
		const String worldFileNameNoExt = world->GetFile()->GetFileName().StringBefore( TXT(".") );
		const String grassMaskFileName = String::Printf( TXT("%s.%s"), worldFileNameNoExt.AsChar(), CGenericGrassMask::GetFileExtension() );
		CDirectory* worldDirectory = world->GetFile()->GetDirectory();

		grassMask->SaveAs( worldDirectory, grassMaskFileName );
#endif // !NO_RESOURCE_IMPORT
	}
}

void CFoliageEditionController::RefreshGrassMask()
{
	m_renderCommandDispatcher->RefreshGenericGrassCommand();
}

CGenericGrassMask * CFoliageEditionController::GetGrassMask()
{
	return m_foliageScene->GetInternalGrassMask();
}

void CFoliageEditionController::AcquireAndLoadFoliageAtArea( const Box& worldRect, CellHandleContainer& outCels )
{
	outCels = m_foliageBroker->AcquireCells( worldRect );

	for ( auto it = outCels.Begin(), end = outCels.End(); it != end; ++it )
	{
		CFoliageCell* foliageCel = it->Get();
		if ( !foliageCel->IsResourceValid() )
		{
			continue;
		}
		foliageCel->Wait();
	}
}

Bool CFoliageEditionController::AcquireResourcesAndAddToModified( const CellHandleContainer& cellContainer )
{
	Bool success = true;
	for ( const CellHandle & cell : cellContainer )
	{
		CFoliageResource * foliageResource = cell->AcquireFoliageResource();
		
		if ( !foliageResource->MarkModified() )
		{
			return false;
		}
	}

	m_editedCells.PushBackUnique( cellContainer );
	return true;
}

Bool CFoliageEditionController::AcquireResourcesAndAddToModified( const Box & worldRect )
{
	CellHandleContainer cellContainer = m_foliageBroker->AcquireCells( worldRect );
	return AcquireResourcesAndAddToModified( cellContainer );
}

Bool CFoliageEditionController::CanModifyResources( const Box & worldRect ) const
{
	CellHandleContainer cellContainer = m_foliageBroker->AcquireCells( worldRect );
	for ( const CellHandle& cell : cellContainer )
	{
		if ( !cell->GetFoliageResource() || !cell->GetFoliageResource()->CanModify() )
		{
			return false;
		}
	}

	return true;
}

#ifndef NO_RESOURCE_IMPORT
void CFoliageEditionController::Save()
{
	CGenericGrassMask * grassMask = m_foliageScene->GetInternalGrassMask();
	if ( grassMask && grassMask->IsModified() )
	{
		grassMask->Save();
	}

	for( auto cellIter = m_editedCells.Begin(), end = m_editedCells.End(); cellIter != end; ++cellIter )
	{
		CFoliageResource * resource = (*cellIter)->GetFoliageResource();
		if( resource && resource->IsModified() )
		{
			resource->Save();
		}
	}

	m_editedCells.Clear();
}
#endif // !NO_RESOURCE_IMPORT

void CFoliageEditionController::SetDebugVisualisationMode( EFoliageVisualisationMode mode )
{
	m_renderCommandDispatcher->SetDebugVisualisationModeCommand( mode );
	
}

void CFoliageEditionController::UpdateGrassSetup( IRenderProxy * terrainProxy, IRenderObject * renderUpdateData )
{
	m_renderCommandDispatcher->UpdateGrassSetupCommand( terrainProxy, renderUpdateData );
}

void CFoliageEditionController::UpdateDynamicGrassCollision( const TDynArray< SDynamicCollider >& collisions )
{
	m_renderCommandDispatcher->UpdateDynamicGrassCollisionCommand( collisions );
}

void CFoliageEditionController::PushTransaction()
{
	++m_transaction;
}

void CFoliageEditionController::PullTransaction()
{
	--m_transaction;
}

Uint32 CFoliageEditionController::GetCurrentTransactionId() const
{
	return m_transaction;
}

void CFoliageEditionController::ReduceVisibilityDepth()
{
	m_foliageBroker->ReduceVisibilityDepth();
}

void CFoliageEditionController::IncreateVisibilityDepth()
{
	m_foliageBroker->IncreateVisibilityDepth();
}

void CFoliageEditionController::WaitUntilAllFoliageResourceLoaded()
{
	m_foliageBroker->Wait();
	m_resourceHandler->Wait();
}

Red::TSharedPtr< CFoliageBroker > CFoliageEditionController::GetFoliageBroker() const
{
	return m_foliageBroker;
}

Box CFoliageEditionController::GetWorldBox() const
{
	Vector2 minPosition( -m_worldDimension.X / 2, -m_worldDimension.Y / 2 );
	Vector2 maxPosition(  m_worldDimension.X / 2, m_worldDimension.Y / 2 );
	return Box( minPosition, maxPosition );
}

Red::TUniquePtr< CFoliageEditionController > CreateFoliageEditionController( const SFoliageEditionControllerSetupParameter & param )
{
	Red::TUniquePtr< CFoliageEditionController > controller( new CFoliageEditionController );
	controller->Setup( param );
	return controller;
}

#ifndef NO_EDITOR
void CFoliageEditionController::UpdateFoliageRenderParams( const SFoliageRenderParams &params )
{
	WaitUntilAllFoliageResourceLoaded();
	m_renderCommandDispatcher->UpdateFoliageRenderParams( params );
}
#endif

#ifndef NO_EDITOR_EVENT_SYSTEM
void CFoliageEditionController::DispatchEditorEvent(const CName& name, IEdEventData* data)
{
	if ( name == CNAME( PreReloadBaseTree ) || name == CNAME( ReloadBaseTree ) )
	{
		const CReloadBaseTreeInfo& reloadInfo = GetEventData< CReloadBaseTreeInfo >( data );
		CSRTBaseTree* resource = Cast< CSRTBaseTree >( reloadInfo.m_resourceToReload );
		Box box = GetWorldBox();

		const CellHandleContainer & cellContainer = m_foliageBroker->GetVisibleCells();
		CFilePath reloadedTreePath( resource->GetDepotPath() );

		TDynArray< CSRTBaseTree* > uniqueBaseTrees;
		for ( auto cellIter = cellContainer.Begin(), cellEnd = cellContainer.End(); cellIter != cellEnd; ++cellIter )
		{
			if ( CFoliageResource * foliageResource = (*cellIter)->GetFoliageResource() )
			{
				const CFoliageResource::BaseTreeContainer& trees = foliageResource->GetAllBaseTree();
				for ( Uint32 i = 0; i < trees.Size(); ++i )
				{
					CSRTBaseTree* currTree = trees[i].Get();
					CFilePath currTreePath( currTree->GetDepotPath() );
					if ( currTree && currTree->IsGrassType() == resource->IsGrassType() && currTreePath.GetPathString() == reloadedTreePath.GetPathString() )
					{
						uniqueBaseTrees.PushBackUnique( currTree );
					}
				}
			}
		}

		if ( name == CNAME( PreReloadBaseTree ) )
		{
			for ( CSRTBaseTree* tree : uniqueBaseTrees )
			{
				m_resourceHandler->RemoveInstances( tree, box );
				tree->ReleaseTextures();
			}
		}
		else
		{
			for ( CSRTBaseTree* tree : uniqueBaseTrees )
			{
				tree->CreateRenderObject();
			}

			FoliageInstanceContainer instances;
			GetInstancesFromArea( resource, box, instances );
			m_resourceHandler->AddInstances( resource, instances );
			m_resourceHandler->Invalidate();
		}
	}
}
#endif

