/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "foliageBroker.h"
#include "foliageGrid.h"
#include "foliageCell.h"
#include "foliageResourceLoader.h"
#include "foliageResourceHandler.h"
#include "foliageCellIterator.h"

#include "../core/uniquePtr.h"

const Int32 MaxVisibilityDepth = 20;
const Int32 MinVisibilityDepth = 0;

CFoliageBroker::CFoliageBroker()
	:	m_loader( nullptr ),
		m_visibilityDepth( 0 )
{}

CFoliageBroker::~CFoliageBroker()
{}

void CFoliageBroker::Setup( const Vector2 & worldDimension, const Vector2 & cellDimension, Int32 visibilityDepth )
{
	m_visibilityDepth = visibilityDepth;

	SFoliageGridSetupParameters param = 
	{ 
		worldDimension, 
		cellDimension, 
		m_loader.Get() 
	};

	m_grid.Reset( CreateFoliageGrid( param ).Release() );
}

void CFoliageBroker::UpdateVisibileCells( const Vector2 & centerPosition )
{
	m_currentPosition = centerPosition;
	UpdateVisibileCells();
}

void CFoliageBroker::UpdateVisibileCells()
{
	m_currentCell = m_grid->AcquireCell( m_currentPosition );
	m_visibleCells = m_grid->AcquireVisibleCells( m_currentPosition, m_visibilityDepth );
}

void CFoliageBroker::Tick()
{
	for( Int32 index = 0, end = m_visibleCells.Size(); index != end; ++index )
	{
		m_visibleCells[ index ]->Tick();
	}
}

void CFoliageBroker::PrefetchArea( const Box * boxArray, Uint32 boxCount )
{
	CellHandleContainer prefetchAreaContainer;
	for( Uint32 index = 0; index != boxCount; ++index )
	{
		const Box & box = boxArray[ index ];
		prefetchAreaContainer.PushBack( AcquireCells( box ) );
	}

	m_prefetchArea = std::move( prefetchAreaContainer );

	for( Int32 index = 0, end = m_prefetchArea.Size(); index != end; ++index )
	{
		m_prefetchArea[ index ]->Tick();
	}
}

void CFoliageBroker::PrefetchPosition( const Vector & position )
{
	UpdateVisibileCells( position );
	m_loader->PrefetchAllResource( m_visibleCells );
}

Uint32 CFoliageBroker::VisibleCellCount() const
{
	return m_visibleCells.Size();
}

Uint32 CFoliageBroker::TotalCellCount() const
{
	return m_grid->GetCellCountX() * m_grid->GetCellCountY();
}

const CellHandleContainer & CFoliageBroker::GetVisibleCells() const
{
	return m_visibleCells;
}

CellHandleContainer CFoliageBroker::AcquireCells( const Box & box )
{
	return m_grid->AcquireCells( box );
}

CellHandle CFoliageBroker::AcquireCell( const Vector2& position )
{
	return m_grid->AcquireCell( position );
}

CFoliageCellIterator CFoliageBroker::GetCellIterator( const Box & box )
{
	return CFoliageCellIterator( m_grid.Get(), box );
}

void CFoliageBroker::ReduceVisibilityDepth()
{
	m_visibilityDepth = Max( MinVisibilityDepth, m_visibilityDepth - 1 );
	UpdateVisibileCells();
}

void CFoliageBroker::IncreateVisibilityDepth()
{
	m_visibilityDepth = Min( MaxVisibilityDepth, m_visibilityDepth + 1 );
	UpdateVisibileCells();
}

void CFoliageBroker::Wait()
{
	Tick();

	for( Int32 index = 0, end = m_visibleCells.Size(); index != end; ++index )
	{
		m_visibleCells[ index ]->Wait();
	}
}

bool CFoliageBroker::IsLoading() const
{
	for( Int32 index = 0, end = m_visibleCells.Size(); index != end; ++index )
	{
		if( m_visibleCells[ index ]->IsLoading() )
		{
			return true;
		}
	}

	return false;
}

void CFoliageBroker::SetInternalGrid( CFoliageGrid * grid )
{
	m_grid.Reset( grid );
}

void CFoliageBroker::SetInternalLoader( CFoliageResourceLoader * loader )
{
	m_loader.Reset( loader );
}

void CFoliageBroker::SetVisibilityDepth( Int32 depth )
{
	m_visibilityDepth = depth;
}

const Vector & CFoliageBroker::GetCurrentPosition() const
{
	return m_currentPosition;
}

Red::TUniquePtr< CFoliageBroker > CreateFoliageBroker( const SFoliageBrokerSetupParameters & param )
{
	Red::TUniquePtr< CFoliageBroker > broker( new CFoliageBroker );
	Red::TUniquePtr< CFoliageResourceLoader > loader = CreateFoliageResourceLoader( param.resourceHandler );
	broker->SetInternalLoader( loader.Release() );
	broker->Setup( param.worldDimension, param.cellDimension, param.visibilityDepth );
	return broker;
}
