/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "resourceGrid.h"
#include "resourceGridOptimizer.h"

namespace ResourceGrid
{
	COptimizer::COptimizer()
		: m_numResourcesRemovedUpStream(0)
	{
	}

	void COptimizer::OptimizeGrid( CGrid& grid )
	{
		// reset stats
		m_numResourcesRemovedUpStream = 0;
		m_numResourcesRemovedDownStream = 0;

		// recurse and merge
		THashSet< CResourceRef* > rootResources;
		OptimizeGridCell( grid, *grid.GetRootCell(), rootResources );

		// report number of resources released on the upstream filtering
		LOG_WCC( TXT("Num resource filtered upstream: %d"), m_numResourcesRemovedUpStream );
		LOG_WCC( TXT("Num resource filtered downstream: %d"), m_numResourcesRemovedDownStream );
	}

	void COptimizer::OptimizeGridCell( CGrid& grid, CGridCell& cell, const THashSet< CResourceRef* >& resourcesAtParentCells )
	{
		// remove references to resources that are this node and are already at the parent node
		for ( auto* parentRes : resourcesAtParentCells )
		{
			if ( cell.RemoveResourceReference( parentRes ) )
				m_numResourcesRemovedDownStream += 1;
		}

		// add resources from this node to the general list
		THashSet< CResourceRef* > resourceAtThidNode( resourcesAtParentCells );
		for ( auto* res : cell.GetResources() )
		{
			resourceAtThidNode.Insert( res );
		}

		// recurse to children
		THashMap< CResourceRef*, Uint32 > resourceAtChildNodes;
		for ( Uint32 i=0; i<cell.GetNumChildren(); ++i )
		{
			CGridCell* childCell = cell.GetChild(i);
			if ( childCell )
			{
				OptimizeGridCell( grid, *childCell, resourceAtThidNode );

				// check the resources
				for ( CResourceRef* childRes : childCell->GetResources() )
				{
					Uint32* count = resourceAtChildNodes.FindPtr( childRes );
					if ( count != nullptr )
					{
						*count += 1;
					}
					else
					{
						resourceAtChildNodes.Set( childRes, 1 );
					}
				}
			}
		}

		// for every resources that is referenced in at least 3 child nodes promote it up
		for ( auto it : resourceAtChildNodes )
		{
			if ( it.m_second >= 3 )
			{
				CResourceRef* childRes = it.m_first;

				// remove resource from child node
				for ( Uint32 i=0; i<cell.GetNumChildren(); ++i )
				{
					CGridCell* childCell = cell.GetChild(i);
					childCell->RemoveResourceReference( childRes );
				}

				// add it to the parent node
				cell.AddResourceReference( childRes );
				m_numResourcesRemovedUpStream += 1;
			}
		}
	}

} // ResourceGrid

