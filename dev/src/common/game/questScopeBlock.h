/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "questGraphBlock.h"


class CQuestPhase;
class TQuestThread;

#define LATENT_QUEST_SCOPE_ENABLED
//#define DEBUGGING_LATENT_QUEST_SCOPE

// This block activates a separate quest phase. Phases allow to organize
// quests into hierarchical structures with many sub-quests.
class CQuestScopeBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestScopeBlock, CQuestGraphBlock, 0 )

protected:
	// instance data
	TInstanceVar< CQuestThread* >					i_thread;			//!< thread running this phase instance
	TInstanceVar< CQuestThread* >					i_parentThread;		//!< parent of the thread running this instance
	TInstanceVar< TDynArray< CName > >				i_inputNames;		//!< inputs to activate
	TInstanceVar< Bool >							i_activated;		//!< is block already active
	TInstanceVar< Bool >							i_phaseRefAdded;	//!< has block added phase ref for this instance?

	// block data
	THandle< CQuestPhase >							m_phase;			//!< Link to a quest phase resource
	CQuestGraph*									m_embeddedGraph;	//!< An embedded graph on which the block keeps its data in case no phase resource is linked

#ifdef LATENT_QUEST_SCOPE_ENABLED
	// Cooked version
	mutable Int32									m_phaseRefs;		//!< Number of threads with this block active
	TSoftHandle< CQuestPhase >						m_phaseHandle;		//!< Handle to the quest phase
#endif

	String											m_requiredWorld;

public:
	CQuestScopeBlock();

	//! CObject interface
	virtual void OnPostLoad();

#ifdef LATENT_QUEST_SCOPE_ENABLED
	// Prepare for cooking
#ifndef NO_RESOURCE_COOKING
	virtual void OnCook( class ICookerFramework& cooker ) override;
#endif

	// Load block data
	virtual Bool LoadAsync() const;
	const String& GetPhaseResource() const { return m_phaseHandle.GetPath(); }
	Int32 GetPhaseRefs() const { return m_phaseRefs; }
#endif

	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return false; }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual void SortOutputBlocks();

#endif

	// New methods
	CQuestPhase *GetPhase() { return m_phase.Get(); }
	CQuestGraph *GetGraph();
	const CQuestGraph *GetGraph() const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	void ConvertToResource();
	void EmbedGraphFromResource();
	Bool IsEmbeddedGraph() const { return m_phase.Get() == NULL; }
	Bool IsResource() const { return m_phase.IsValid(); }
	virtual void OnPasted( Bool wasCopied );
#endif

	static CQuestScopeBlock *GetParentScopeBlock( const CQuestGraphBlock *block );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual Bool NeedsUpdate() const;

	// Tells whether the graph can be expelled to an external resource or not
	virtual Bool CanConvertToResource() { return true; }

#endif

	// Can the thread spawned from this block block saves ?
	virtual Bool CanSpawnedThreadBlockSaves() const { return true; }

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;
	// Called before the block the block functionality activation
	// in order to give it some time to initialize itself
	virtual Bool OnProcessActivation( InstanceBuffer& data ) const;

	// ------------------------------------------------------------------------
	// Saving the game state
	// ------------------------------------------------------------------------
	virtual void LoadGame( InstanceBuffer& data, IGameLoader* loader ) const;

	// IScopeEndListener implementation
	virtual void OnScopeEndReached( InstanceBuffer& data, const CName& outSocket) const;

protected:
	//! Returns the thread responsible for running this scope block.
	CQuestThread* GetThread( InstanceBuffer& data ) const;

private:
	void UpdateGraph() const;
	void ActivateSubgraph( InstanceBuffer& data ) const;

public:
	RED_INLINE const String& GetRequiredWorld() const { return m_requiredWorld; }
};

BEGIN_CLASS_RTTI( CQuestScopeBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_phase, TXT("Quest phase stored in a separate resource file") )
	PROPERTY( m_embeddedGraph )
#ifdef LATENT_QUEST_SCOPE_ENABLED
	PROPERTY( m_phaseHandle )
#endif
	PROPERTY_CUSTOM_EDIT( m_requiredWorld, TXT( "World required for processing of this scope" ), TXT( "WorldSelectionQuestBinding" ) );
END_CLASS_RTTI()