#pragma once

class CQuestGraph;

class CQuestPhase : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CQuestPhase, CResource, "w2phase", "Phase" );

	friend class CQuestScopeBlock;

protected:

	CQuestGraph *m_graph;
	
	void SetGraph( CQuestGraph *graph );

public:
	CQuestPhase();
	RED_INLINE CQuestGraph *GetGraph() const { return m_graph; }

	// Removes unnecessary object from the resource for cooking purposes
	void CleanupSourceData();
};

BEGIN_CLASS_RTTI( CQuestPhase )
	PARENT_CLASS( CResource )
	PROPERTY( m_graph )
END_CLASS_RTTI()
