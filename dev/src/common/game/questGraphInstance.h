/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once


// A quest graph instance allows to run algorithms on a
// stateless instance of a quest graph using a dedicated
// data buffer, which creates a statefull graph execution
// context.
class CQuestGraphInstance : public CObject
{
	DECLARE_ENGINE_CLASS( CQuestGraphInstance, CObject, 0 );

private:
	const CQuestGraph*							m_parentGraph;
	TDynArray< CQuestGraphBlock* >			m_graphBlocks;
	InstanceBuffer*								m_data;				//!< Runtime data for graph

public:
	CQuestGraphInstance();
	~CQuestGraphInstance();

	// Returns the parent graph this is an instance of
	RED_INLINE const CQuestGraph* GetParentGraph() const { return m_parentGraph; }

	// Binds a graph to this instance
	void Bind( const CQuestGraph& parentGraph, const TDynArray< CGraphBlock* >& graphBlocks, const InstanceDataLayout& layout );

	// Unbinds this instance from the shared graph.
	void Unbind();

	// Returns the data that can be used to work with the graph.
	RED_INLINE InstanceBuffer& GetInstanceData() { return *m_data; }

	// Returns an instance of a block that marks the start of the graph.
	const CQuestGraphBlock* GetStartBlock( const CName& name ) const;

	// Finds a block using a GUID number assigned to it as a search key.
	const CQuestGraphBlock* FindBlockByGUID( const CGUID& guid ) const;
};

BEGIN_CLASS_RTTI( CQuestGraphInstance )
	PARENT_CLASS( CObject )
END_CLASS_RTTI()
