/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "list.h"
#include "objectGC.h"

/// Garbage collector for CObject
class CObjectGC_Legacy : public IObjectGCStategy
{
protected:
	TList< CObject* >					m_finalizationList;			// List of objects to finalize
	TDynArray<Bool, MC_Engine>			m_unreachables;
	TDynArray<CObject*, MC_Engine >		m_objectsResults;
	TDynArray<CObject*, MC_Engine>		m_toRemoveList;
	class CFullReachablityMarker*		m_marker;

public:
	CObjectGC_Legacy();
	~CObjectGC_Legacy();

	// Perform GC
	virtual void CollectGarbage( const Bool reportLeaks ) override;

private:

	void DiscardAllObjectToRemove();
};
