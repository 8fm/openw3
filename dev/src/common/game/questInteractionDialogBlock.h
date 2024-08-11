/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "questBlockWithScene.h"
#include "storyScene.h"


class CQuestScenePlayer;

class CQuestInteractionDialogBlock : public CQuestGraphBlock, public IQuestBlockWithScene
{
	DECLARE_ENGINE_CLASS( CQuestInteractionDialogBlock, CQuestGraphBlock, 0 )

private:
	// instance data
	TInstanceVar< TDynArray< TGenericPtr > >					i_players;

	// block data
	TSoftHandle< CStoryScene >							m_scene;
	TagList												m_actorTags;
	Bool												m_interrupt;

public:
	CQuestInteractionDialogBlock() : m_interrupt( false ) { m_name = TXT("Interaction dialog"); }

	// !IQuestBlockWithScene interface
	virtual CStoryScene* GetScene() const { return m_scene.Get(); }
	const	TagList &	 GetActorTags()	const { return m_actorTags; }
#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void SetScene( CStoryScene* scene );

	//! CObject interface
	virtual void OnPropertyPostChange( IProperty* property );

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 162, 167, 89 ); }
	virtual String GetCaption() const;
	virtual String GetBlockCategory() const { return TXT( "Scenes" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

	Bool NeedsUpdate() const;

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;

private:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	void RebuildSceneInterfaceSockets();
#endif

	virtual void CollectContent( IQuestContentCollector& collector ) const override;
};

BEGIN_CLASS_RTTI( CQuestInteractionDialogBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_scene, TXT( "Scene with default interaction dialogs" ) );
	PROPERTY_CUSTOM_EDIT( m_actorTags, TXT( "Tags of the actors for whom the dialog will be played" ), TXT( "TagListEditor" ) );
	PROPERTY_EDIT( m_interrupt, TXT( "Interrupts all scheduled and playing quest scenes" ) );
END_CLASS_RTTI()
