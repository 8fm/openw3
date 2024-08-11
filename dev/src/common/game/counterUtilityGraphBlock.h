#pragma once

class CCounterUtilityGraphBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CCounterUtilityGraphBlock, CQuestGraphBlock, 0 )

public:

	CCounterUtilityGraphBlock();
	virtual ~CCounterUtilityGraphBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetBlockCategory() const { return TXT( "Utility" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return false; } // OBSOLETE
	virtual Color GetClientColor() const { return Color( 200, 200, 200 ); }

	//! CGraphBlock interface
	virtual void OnRebuildSockets();

#endif

	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

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

private:
	Int32 m_target;

	TInstanceVar< Int32 > i_count;
};

BEGIN_CLASS_RTTI( CCounterUtilityGraphBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_target, TXT( "Target Value - The output is triggered when count matches target" ) )
END_CLASS_RTTI()
