/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "questBlockWithScene.h"

class CQuestScriptedDialogBlock : public CQuestGraphBlock, public IQuestBlockWithScene
{
	DECLARE_ENGINE_CLASS( CQuestScriptedDialogBlock, CQuestGraphBlock, 0 );

private:
	// instance data
	TInstanceVar< TDynArray< TGenericPtr > >	i_players;

	TSoftHandle< CStoryScene >	m_scene;
	TagList						m_actorTags;

public:
	CQuestScriptedDialogBlock();

	// !IQuestBlockWithScene interface
	virtual CStoryScene* GetScene() const { return m_scene.Get(); }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void SetScene( CStoryScene* scene );

	//! CObject interface
	virtual void OnPropertyPostChange( IProperty* property );

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 255, 213, 164 ); }
	virtual String GetCaption() const;
	virtual String GetBlockCategory() const { return TXT( "Scenes" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

	Bool NeedsUpdate() const;

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;

};

BEGIN_CLASS_RTTI( CQuestScriptedDialogBlock );
	PARENT_CLASS( CQuestGraphBlock );
	PROPERTY_EDIT( m_scene, TXT( "Scene available for use from scripts" ) );
	PROPERTY_CUSTOM_EDIT( m_actorTags, TXT( "Tags of actor who can use scene" ), TXT( "TagListEditor" ) );
END_CLASS_RTTI();