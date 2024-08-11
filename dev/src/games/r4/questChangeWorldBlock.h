/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Change world
class CQuestChangeWorldBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestChangeWorldBlock, CQuestGraphBlock, 0 )

private:

	// World change related

	String	m_worldFilePath;
	Int32	m_newWorld;
    String  m_loadingMovieName;
	TagList	m_targetTag;			// Where to teleport player after world change

	TInstanceVar< Bool >	i_wasChangeScheduled;

public:
	CQuestChangeWorldBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CObject interface
	virtual String GetCaption() const { return TXT("Change world"); }

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 71, 199, 255 ); }
	virtual String GetBlockCategory() const { return TXT( "Game systems control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

	virtual void OnExecute( InstanceBuffer& data ) const;

	virtual void OnDeactivate( InstanceBuffer& data ) const;

	// ------------------------------------------------------------------------
	// Saving the game state
	// ------------------------------------------------------------------------
	virtual void SaveGame( InstanceBuffer& data, IGameSaver* saver ) const;
	virtual void LoadGame( InstanceBuffer& data, IGameLoader* loader ) const;

	// Called when a data layout for the graph the block is a part of is being built.
	// A block should register all its instance variables with the specified compiler.
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	// Called when a graph instance is initialized. A block should 
	// initialize its instance data here
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
};

BEGIN_CLASS_RTTI( CQuestChangeWorldBlock )
	PARENT_CLASS( CQuestGraphBlock )

	PROPERTY_RO( m_worldFilePath, TXT( "New world to load" ) );
	PROPERTY_CUSTOM_EDIT( m_newWorld, TXT( "New world to load" ), TXT("ScriptedEnum_EAreaName" ) )
    PROPERTY_EDIT( m_loadingMovieName, TXT( "Name of the movie to play, while transitioning. Path relative to data\\movies\\" ) );
	PROPERTY_CUSTOM_EDIT( m_targetTag, TXT( "Target location tag" ), TXT( "TagListEditor" ) )
END_CLASS_RTTI()

