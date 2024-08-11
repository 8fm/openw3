/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "questBlockWithScene.h"
#include "storyScene.h"
#include "storySceneComment.h"

class CStorySceneInput;

class CQuestContextDialogBlock : public CQuestGraphBlock, public IQuestBlockWithScene
{
	DECLARE_ENGINE_CLASS( CQuestContextDialogBlock, CQuestGraphBlock, 0 )

private:
	enum EActivationPhase
	{
		PHASE_NotActive		= 0,
		PHASE_LoadingScene	= 1,
		PHASE_Activating	= 2,
		PHASE_Active		= 3,
	};

	// instance data
	TInstanceVar< TGenericPtr >								i_playbackListener;
	TInstanceVar< Int32 >									i_activated;

	// block data
	TSoftHandle< CStoryScene >							m_scene;
	TSoftHandle< CStoryScene >							m_targetScene;

public:
	CQuestContextDialogBlock() { m_name = TXT("Context dialog"); }

	// !IQuestBlockWithScene interface
	virtual CStoryScene* GetTargetScene() const { return m_targetScene.Get(); }
	virtual CStoryScene* GetScene() const { return m_scene.Get(); }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void SetScene( CStoryScene* scene );
#endif

	//! CObject interface
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 172, 108, 129 ); }
	virtual String GetCaption() const;
	virtual String GetBlockCategory() const { return TXT( "Scenes" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

	Bool NeedsUpdate() const;

#endif

	//! Returns the choices the dialog this block contains can inject
	Bool GetChoices( THashMap< String, SSceneInjectedChoiceLineInfo >& outChoices ) const;
	Bool GetChoices( const TDynArray< String >& activeInputs, THashMap< String, SSceneInjectedChoiceLineInfo >& outChoices ) const;

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;

protected:
	void GetInputNames( InstanceBuffer& instanceData, TDynArray< String >& inputNames ) const;

private:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	void RebuildSceneInterfaceSockets();
#endif
	Bool GetInjectedDialogDescription( CStorySceneInput* input, SSceneInjectedChoiceLineInfo& injectedLineInfo ) const;
};

BEGIN_CLASS_RTTI( CQuestContextDialogBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_scene, TXT( "Scene with new context dialog options" ) )
	PROPERTY_EDIT( m_targetScene, TXT( "Scene to which the options will be injected" ) )
END_CLASS_RTTI()
