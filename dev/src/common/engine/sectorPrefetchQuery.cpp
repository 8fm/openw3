/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorPrefetchGrid.h"
#include "sectorPrefetchQuery.h"

//-----------------------------------------------------------------------------

CSectorPrefetchQueryResult::CSectorPrefetchQueryResult()
	: m_grid( nullptr )
	, m_numCells( 0 )
{
}

CSectorPrefetchQueryResult::CSectorPrefetchQueryResult( const CSectorPrefetchGrid* grid )
	: m_grid( grid )
	, m_numCells( 0 )
{
}

//-----------------------------------------------------------------------------
