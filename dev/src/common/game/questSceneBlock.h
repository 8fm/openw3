/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "questBlockWithScene.h"
#include "storySceneForcingMode.h"

class CQuestScenePlayer;

// Quest scene save mode
enum EQuestSceneSaveMode
{
	QSSM_SaveBlocker,			//!< Game will not be saved if this scene is played
	QSSM_Restart,				//!< Restart scene after loading
	QSSM_Skip,					//!< Skip scene after loading
};

BEGIN_ENUM_RTTI( EQuestSceneSaveMode );
	ENUM_OPTION( QSSM_SaveBlocker );
	ENUM_OPTION( QSSM_Restart );
	ENUM_OPTION( QSSM_Skip );
END_ENUM_RTTI();


// A quest block capable of playing a story scene
class CQuestSceneBlock : public CQuestGraphBlock, public IQuestBlockWithScene
{
	DECLARE_ENGINE_CLASS( CQuestSceneBlock, CQuestGraphBlock, 0 )

private:
	// instance data
	TInstanceVar< TGenericPtr >							i_player;

	// block data
	TSoftHandle< CStoryScene >							m_scene;
	EStorySceneForcingMode								m_forcingMode;
	Bool												m_forceSpawnedActors; // TODO: remove when version VER_FORCING_MODE_MAY_BE_DEFINED_FOR_QUEST_SCENES is outdated
	Bool												m_interrupt;
	Bool												m_shouldFadeOnLoading;
	TDynArray< CName >									m_shouldFadeOnLoading_NamesCooked;
	TDynArray< Bool >									m_shouldFadeOnLoading_ValuesCooked;

	// save system
	EQuestSceneSaveMode									m_saveMode;
	CName												m_saveSkipOutputNode;
	CName												m_playGoChunk;

public:
	CQuestSceneBlock();

	// !IQuestBlockWithScene interface
	virtual CStoryScene* GetScene() const { return m_scene.Get(); }
	virtual void SetScene( CStoryScene* scene );
	Bool HasLoadedSection( const InstanceBuffer& data ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );

	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 79, 129, 189 ); }
	virtual String GetBlockCategory() const { return TXT( "Scenes" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

	virtual void SortOutputBlocks();

	Bool NeedsUpdate() const;

	void CookSceneData();
#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

	virtual void OnExecute( InstanceBuffer& data ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;

	// ------------------------------------------------------------------------
	// Saving the game state
	// ------------------------------------------------------------------------
	virtual void SaveGame( InstanceBuffer& data, IGameSaver* saver );
	virtual void LoadGame( InstanceBuffer& data, IGameLoader* loader );

private:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	void RebuildSceneInterfaceSockets();

	//! Removes unnecessary object from the resource for cooking purposes
	virtual void CleanupSourceData() override;
#endif

	// Action that tries to activate a scene 
	void ActivateScene( InstanceBuffer& data ) const;

	// Action that waits for an active scene to end
	void WaitForSceneToEnd( InstanceBuffer& data ) const;

	// Check what type of input should be played
	Bool ShouldFadeOnLoading_Cooked( const CName& inputName ) const;

	// Collect resources
	virtual void CollectContent( IQuestContentCollector& collector ) const override;
};

BEGIN_CLASS_RTTI( CQuestSceneBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_scene, TXT( "Story scene file" ) )
	PROPERTY_EDIT( m_forcingMode, TXT("How the scene conditions should be forced") )
	PROPERTY( m_forceSpawnedActors )
	PROPERTY_EDIT( m_interrupt, TXT( "Interrupts all scheduled and playing quest scenes" ) )
	PROPERTY_EDIT( m_shouldFadeOnLoading, TXT( "" ) )
	PROPERTY_RO( m_shouldFadeOnLoading_NamesCooked, TXT( "" ) )
	PROPERTY_RO( m_shouldFadeOnLoading_ValuesCooked, TXT( "" ) )
	PROPERTY( m_saveMode )
	PROPERTY( m_saveSkipOutputNode );
	PROPERTY_CUSTOM_EDIT( m_playGoChunk, TXT("PlayGo chunk"), TXT("PlayGoChunkSelector") );
END_CLASS_RTTI()

