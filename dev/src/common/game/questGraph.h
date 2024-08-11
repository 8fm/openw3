/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "../engine/graphContainer.h"

class CQuestStartBlock;
class CQuestScopeBlock;
class CQuestGraphInstance;
class CQuestPhaseIOBlock;

class CQuestGraph : public CObject, public IGraphContainer
{
	DECLARE_ENGINE_CLASS( CQuestGraph, CObject, 0 );

private:
	TDynArray< CGraphBlock* >			m_graphBlocks;			//!< Blocks in the graph
	Vector								m_backgroundOffset;		//!< Graph offset
	InstanceDataLayout					m_dataLayout;			//!< Layout of data in this graph
	Bool								m_sourceDataRemoved;	//!< Has the source data been cleaned up from this graph
	Bool								m_isTest;

public:
	CQuestGraph();
	~CQuestGraph();

	RED_INLINE Bool IsTest() const { return m_isTest; }
	// Creates an instance of the graph. An instance can be executed
	// from an arbitrary context and will operate on the shared graph
	// structure using separate data per each instance.
	virtual CQuestGraphInstance* CreateInstance( CObject* parent ) const;

#ifndef NO_EDITOR
	//! CObject interface
	virtual void OnSerialize( IFile& file );
#endif

	//! CObject interface
	virtual void OnPostLoad();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	// Removes unnecessary object from the resource for cooking purposes
	virtual void CleanupSourceData();

	typedef TDynArray< CQuestScopeBlock* > ScopeStack;

	struct GUIDCheckEntry
	{
		GUIDCheckEntry( CQuestGraphBlock* block, const ScopeStack& scope ) : m_block( block ), m_scope( scope ) {}
		CQuestGraphBlock* m_block;
		ScopeStack m_scope;
	};

	typedef THashMap< CGUID, TDynArray< GUIDCheckEntry > > DuplicateMap;

	// Checks the GUIDs of the aggregate blocks in search for duplicates
	void CheckGUIDs( DuplicateMap& duplicatedGUIDs, const ScopeStack& scope = ScopeStack() ) const;

	// Recursively finds a block of given guid. Returns nullptr if not found.
	CQuestGraphBlock* FindGuid( const CGUID& guid ) const;

	void GetAllGUIDs( TDynArray< CGUID >& allGUIDs ) const;

	const CQuestScopeBlock* GetScopeBlockWithEmbeddedPhase( const CQuestPhase* phase ) const;

#endif 

	//! IGraphContainer interface
	virtual Vector GraphGetBackgroundOffset() const;
	virtual void GraphSetBackgroundOffset( const Vector& offset );
	virtual CObject *GraphGetOwner();
	virtual TDynArray< CGraphBlock* >& GraphGetBlocks() { return m_graphBlocks; }
	virtual const TDynArray< CGraphBlock* >& GraphGetBlocks() const { return m_graphBlocks; }
	virtual Bool ModifyGraphStructure();
	virtual void GraphStructureModified();

	virtual void GetRewards( TDynArray< CName >& rewards ) const;

	void RepairDuplicateInputs( TDynArray< CQuestPhaseIOBlock* >& changed );

private:
	void CompileDataLayout();
};

BEGIN_CLASS_RTTI( CQuestGraph )
	PARENT_CLASS( CObject )
	PROPERTY( m_graphBlocks )
	PROPERTY( m_sourceDataRemoved )
	PROPERTY_EDIT( m_isTest, TXT( "Quest used for testing" ) )
END_CLASS_RTTI()

