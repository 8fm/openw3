/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#ifndef _ENGINE_FOLIAGE_BROKER_H_
#define _ENGINE_FOLIAGE_BROKER_H_

#include "foliageForward.h"

#include "../core/scopedPtr.h"
#include "../core/uniquePtr.h"
#include "../core/math.h"

class IRenderScene;
class CPhysicsWorld;
class IFoliageRenderCommandDispatcher;

struct SFoliageBrokerSetupParameters
{
	Vector2 worldDimension;
	Vector2 cellDimension;
	Int32 visibilityDepth;
	CFoliageResourceHandler * resourceHandler;
	const IFoliageRenderCommandDispatcher * renderCommandfactory;
};

class CFoliageBroker
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:

	CFoliageBroker();
	RED_MOCKABLE ~CFoliageBroker();

	void Setup( const Vector2 & worldDimension, const Vector2 & cellDimension, Int32 visibilityDepth );

	void UpdateVisibileCells( const Vector2 & centerPosition );
	void PrefetchArea( const Box * boxArray, Uint32 boxCount );
	RED_MOCKABLE void PrefetchPosition( const Vector & position );

	RED_MOCKABLE void Tick();

	const CellHandleContainer & GetVisibleCells() const;
	RED_MOCKABLE CellHandleContainer AcquireCells( const Box & box );
	CellHandle AcquireCell( const Vector2 & position );
	CFoliageCellIterator GetCellIterator( const Box & box );

	Uint32 VisibleCellCount() const;
	Uint32 TotalCellCount() const;

	void ReduceVisibilityDepth();
	void IncreateVisibilityDepth();

	RED_MOCKABLE bool IsLoading() const;

	void Wait();

	const Vector & GetCurrentPosition() const;

	void SetInternalGrid( CFoliageGrid * grid );
	void SetInternalLoader( CFoliageResourceLoader * loader );
	
	void SetVisibilityDepth( Int32 depth );

private:
	
	void UpdateVisibileCells();

	Red::TScopedPtr< CFoliageGrid > m_grid;
	Red::TScopedPtr< CFoliageResourceLoader > m_loader;
	Vector m_currentPosition;
	CellHandleContainer m_visibleCells;
	CellHandleContainer m_prefetchArea;
	CellHandle m_currentCell;
	Int32 m_visibilityDepth;
};

Red::TUniquePtr< CFoliageBroker > CreateFoliageBroker( const SFoliageBrokerSetupParameters & param );

#endif
