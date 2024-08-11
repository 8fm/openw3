/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CGraphBlock;
class GraphBlockSpawnInfo;

enum EGraphLayerState
{
	GLS_Visible,
	GLS_Freeze,
	GLS_Hide
};

BEGIN_ENUM_RTTI( EGraphLayerState );
	ENUM_OPTION( GLS_Visible );
	ENUM_OPTION( GLS_Freeze );
	ENUM_OPTION( GLS_Hide );
END_ENUM_RTTI();

struct SGraphLayer 
{
	DECLARE_RTTI_STRUCT( SGraphLayer );

	EGraphLayerState	m_state;
	String				m_name;

	SGraphLayer() : m_state( GLS_Visible ) {}
	SGraphLayer( const String& name ) : m_state( GLS_Visible ), m_name( name ) {}
};

BEGIN_CLASS_RTTI( SGraphLayer );
	PROPERTY_EDIT( m_state, TXT("") );
	PROPERTY_EDIT( m_name, TXT("") );
END_CLASS_RTTI();


/// Graph container interface, we can edit it in editor
class IGraphContainer
{
public:
	virtual ~IGraphContainer() {};

public:
	//! Get object that owns the graph
	virtual CObject *GraphGetOwner()=0;

	//! Get list of blocks
	virtual TDynArray< CGraphBlock* >& GraphGetBlocks() = 0;
	virtual const TDynArray< CGraphBlock* >& GraphGetBlocks() const = 0;

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Structure of graph was modified
	virtual void GraphStructureModified();

	//! Can we modify the graph structure ?
	virtual Bool ModifyGraphStructure();

	//! Does this graph supports given class
	virtual Bool GraphSupportsBlockClass( CClass *blockClass ) const;

	//!! Get background offset
	virtual Vector GraphGetBackgroundOffset() const;

	//! Set background offset
	virtual void GraphSetBackgroundOffset( const Vector& offset );

	//! Create block and at it to graph
	virtual CGraphBlock* GraphCreateBlock( const GraphBlockSpawnInfo& info );

	//! Can remove block?
	virtual Bool GraphCanRemoveBlock( CGraphBlock *block ) const;

	//! Remove block from the graph, does not delete object
	virtual Bool GraphRemoveBlock( CGraphBlock *block );

	//! Paste blocks
	virtual Bool GraphPasteBlocks( const TDynArray< Uint8 >& data, TDynArray< CGraphBlock* >& pastedBlocks, Bool relativeSpawn, Bool atLeftUpper, const Vector& spawnPosition );

	//! Get layer
	virtual SGraphLayer* GetLayer( Uint32 num ) { return NULL; }

	//! Get layer num
	virtual Uint32 GetLayerNum() const { return 0; }

	//! Get layers in state
	virtual Uint32 GetLayersInStateFlag( EGraphLayerState state );

#endif
};
