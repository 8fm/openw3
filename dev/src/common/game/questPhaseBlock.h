/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "questScopeBlock.h"

// Quest phase save mode
enum EQuestPhaseSaveMode
{
	QPSM_SaveBlocker,			//!< Game will not be saved if this phase is active
	QPSM_SavesAllowed,			//!< The phase can safely be saved
};

BEGIN_ENUM_RTTI( EQuestPhaseSaveMode );
	ENUM_OPTION( QPSM_SaveBlocker );
	ENUM_OPTION( QPSM_SavesAllowed );
END_ENUM_RTTI();


// This scoped block assures that additional set of layers
// is loaded before it starts executing the embedded graph
class CQuestPhaseBlock : public CQuestScopeBlock
{
	DECLARE_ENGINE_CLASS( CQuestPhaseBlock, CQuestScopeBlock, 0 )

protected:
	// instance data
	TInstanceVar< TGenericPtr >					i_loadingFence;
	TInstanceVar< Int32 >						i_saveLock;
	TInstanceVar< Bool >						i_isActive;
	TInstanceVar< Bool >						i_layersLoaded;
	TInstanceVar< Bool >						i_soundBanksRequested;

	// block data
	TDynArray< String >							m_layersToLoad;	//!< Layers we want this block to activate when it gets active
	Bool										m_isBlackscreenPhase;
	Float										m_blackscreenFadeDuration;
	EQuestPhaseSaveMode							m_saveMode;
	TDynArray< CName >							m_soundBanksDependency;
	CName										m_playGoChunk;
	Bool										m_purgeSavedData;

public:
	CQuestPhaseBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Rounded; }
	virtual Color GetTitleColor() const;
	virtual Color GetClientColor() const;
	virtual String GetBlockCategory() const { return TXT( "Complexity management" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// Restart block's contents
	virtual Bool CanActivateInputsOnLoad( CQuestGraphBlock::EState activationState ) const { return m_saveMode == QPSM_SavesAllowed && activationState != ST_ACTIVE; }

	// Get the assigned content chunk (affects all blocks that are inside)
	RED_INLINE CName GetContentChunk() const { return m_playGoChunk; }
	RED_INLINE Bool IsActivelyChangingLayers( const InstanceBuffer& data ) const { return data[ i_loadingFence ] != nullptr; }

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual Bool OnProcessActivation( InstanceBuffer& data ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;
};

BEGIN_CLASS_RTTI( CQuestPhaseBlock )
	PARENT_CLASS( CQuestScopeBlock )
	PROPERTY_CUSTOM_EDIT( m_layersToLoad, TXT( "Layers that should be loaded when the block gets activated" ), TXT("LayerGroupList") );
	PROPERTY_EDIT( m_isBlackscreenPhase, TXT( "Phase will be executed on blackscreen" ) );
	PROPERTY_EDIT( m_blackscreenFadeDuration, TXT( "Blackscreen fade duration" ) );
	PROPERTY_EDIT( m_saveMode, TXT( "Save mode" ) );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_soundBanksDependency, TXT( "" ), TXT( "SoundBankEditor" ) )
	PROPERTY_CUSTOM_EDIT( m_playGoChunk, TXT("PlayGo chunk"), TXT("PlayGoChunkSelector") );
	PROPERTY_EDIT( m_purgeSavedData, TXT("Forget all the persistent data from hidden layers") );
END_CLASS_RTTI()