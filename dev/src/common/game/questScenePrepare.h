/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "questGraphBlock.h"

class CQuestScenePrepareBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestScenePrepareBlock, CQuestGraphBlock, 0 );

protected:
	TDynArray< StorySceneDefinition >						m_storyScenes;
	
	TInstanceVar< TDynArray< TSoftHandle< CResource > > >	i_sceneResources;
	TInstanceVar< Bool >									i_resourcesCollected;
	TInstanceVar< Uint32 >									i_currentSceneIndex;


public:
	CQuestScenePrepareBlock() { m_name = TXT("Scene prepare"); }

	virtual void OnDeactivate( InstanceBuffer& data ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 79, 129, 189 ); }
	virtual String GetBlockCategory() const { return TXT( "Scenes" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

	virtual String GetCaption() const;
#endif


};

BEGIN_CLASS_RTTI( CQuestScenePrepareBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_storyScenes, TXT( "Story Scene resources to prepare" ) )
END_CLASS_RTTI();