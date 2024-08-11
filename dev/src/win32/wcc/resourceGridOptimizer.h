/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

namespace ResourceGrid
{
	class CGrid;

	/// Optimizer for the grid
	class COptimizer
	{
	public:
		COptimizer();

		// optimize resource grid by pulling resources upward
		// this limits the amount of stuff that is duplicated
		void OptimizeGrid( CGrid& grid );

	private:
		void OptimizeGridCell( CGrid& grid, CGridCell& cell, const THashSet< CResourceRef* >& resourcesAtParentCells );

		// stats
		Uint32		m_numResourcesRemovedUpStream;
		Uint32		m_numResourcesRemovedDownStream;
	};

}