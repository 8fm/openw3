#pragma once
#include "../engine/graphContainer.h"

struct SBrokenSceneGraphInfo;

/// Scene graph that represents the top level structure of connected sections and conditions
class CStorySceneGraph : public CObject, public IGraphContainer
{
	DECLARE_ENGINE_CLASS( CStorySceneGraph, CObject, 0 );

private:
	TDynArray< CGraphBlock* >	m_graphBlocks;			//!< Blocks in the graph
	Vector						m_backgroundOffset;		//!< Graph offset

public:
	CStorySceneGraph();

	void OnPreSave();

	//! Graph was loaded from disk
	virtual void OnPostLoad();

	virtual void GraphStructureModified();

public:
	// Graph interface
	virtual CObject *GraphGetOwner();
	virtual TDynArray< CGraphBlock* >& GraphGetBlocks() { return m_graphBlocks; }
	virtual const TDynArray< CGraphBlock* >& GraphGetBlocks() const { return m_graphBlocks; }
	virtual Vector GraphGetBackgroundOffset() const;
	virtual void GraphSetBackgroundOffset( const Vector& offset );

#ifndef NO_EDITOR
	Bool CheckConsistency( SBrokenSceneGraphInfo* graphInfo = NULL, Bool doNotShowInfoIfItsOK = true, Bool doNotShowInfoIfItsNotOK = true );
#endif //NO_EDITOR
};

BEGIN_CLASS_RTTI( CStorySceneGraph );
	PARENT_CLASS( CObject );
	PROPERTY( m_graphBlocks );
END_CLASS_RTTI();

