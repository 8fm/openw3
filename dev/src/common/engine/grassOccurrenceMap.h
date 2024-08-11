/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../core/object.h"
#include "../core/uniquePtr.h"
#include "grassCellMask.h"

class CGrassOccurrenceMap : public CObject
{
	DECLARE_ENGINE_CLASS( CGrassOccurrenceMap, CObject, 0 )
public:
	CGrassOccurrenceMap();

	void SetCellMasks( const TDynArray< CGrassCellMask >& cellMasks );
	TDynArray< CGrassCellMask >& GetCellMasks() { return m_cellMasks; }

private:
	TDynArray< CGrassCellMask >	m_cellMasks;	//!< A cells mask for each automatically populated grass type
};

#ifndef NO_RESOURCE_COOKING
Red::TUniquePtr< CGrassOccurrenceMap > CreateGrassOccurenceMap( const CClipMap * clipMap );
#endif

BEGIN_CLASS_RTTI( CGrassOccurrenceMap );
	PARENT_CLASS( CObject );
	PROPERTY( m_cellMasks );
END_CLASS_RTTI();
