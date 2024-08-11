/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

class BehaviorGraphNodeIterator
{
	const CBehaviorGraph*				m_graph;
	
	// TODO: Zrobic bez dynamicznej alokacji!
	// To jest placeholder!
	TDynArray< CBehaviorGraphNode* >	m_nodes;
	Uint32								m_index;

public:
	RED_INLINE BehaviorGraphNodeIterator( const CBehaviorGraph* graph )
		: m_graph( graph )
	{
		m_graph->GetAllNodes( m_nodes );
		m_index = 0;
	}

	RED_INLINE void operator++()
	{
		Next();
	}

	RED_INLINE operator Bool () const
	{
		return m_index < m_nodes.Size();
	}

	RED_INLINE CBehaviorGraphNode* operator * ()
	{
		return m_nodes[ m_index ];
	}

private:
	void Next()
	{
		m_index++;
	}
};
