/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "guiObject.h"
#include "../engine/graphContainer.h"
#include "../engine/graphBlock.h"

//////////////////////////////////////////////////////////////////////////
// IGuiResourceBlock
//////////////////////////////////////////////////////////////////////////
class IGuiResourceBlock : public CGraphBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IGuiResourceBlock, CGraphBlock );

public:
	virtual void					OnPropertyPostChange( IProperty* property );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! Rebuild sockets and connections
	virtual void OnRebuildSockets();

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;

	//! Get client color
	virtual Color GetClientColor() const;
#endif

	//! Get block caption, usually block name
	virtual String GetCaption() const;
};

//////////////////////////////////////////////////////////////////////////
// IGuiResource
//////////////////////////////////////////////////////////////////////////
class IGuiResource : public CResource, public IGraphContainer
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IGuiResource, CResource );

private:
	TDynArray< CGraphBlock* >						m_resourceBlocks;			// Blocks in the graph
	Vector											m_backgroundOffset;			// Graph offset

public:
	RED_INLINE const TDynArray< CGraphBlock* >&	GetResourceBlocks() const { return m_resourceBlocks; }

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual CClass*									GetResourceBlockClass() const { return nullptr; }
#endif

	//! IGraphContainer interface
	virtual Vector									GraphGetBackgroundOffset() const;
	virtual void									GraphSetBackgroundOffset( const Vector& offset );
	virtual CObject *								GraphGetOwner();
	virtual TDynArray< CGraphBlock* >&				GraphGetBlocks() { return m_resourceBlocks; }
	virtual const TDynArray< CGraphBlock* >&		GraphGetBlocks() const { return m_resourceBlocks; }
	virtual void									GraphStructureModified();

public:
	//! CObject interface
	virtual void									OnPostLoad();

	// Mark resource we are in as modified
	virtual Bool									MarkModified();

	virtual void									OnPropertyPostChange( IProperty* property );
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( IGuiResourceBlock, CGraphBlock );

BEGIN_ABSTRACT_CLASS_RTTI( IGuiResource );
PARENT_CLASS( CResource );
	PROPERTY( m_resourceBlocks );
END_CLASS_RTTI();


