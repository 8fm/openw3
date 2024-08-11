#pragma once

#include "../../common/game/questCondition.h"
#include "../../common/game/questGraphBlock.h"

// A block which can pause R6 Quest until condition is met
// Always use it with CQuestGraphR6QuestBlock (inside it's scope)
class CQuestGraphR6QuestPauseBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestGraphR6QuestPauseBlock, CQuestGraphBlock, 0 )

private:
	IQuestCondition*					m_condition; 

	// instance data
	TInstanceVar< TGenericPtr >			i_condition; 
	TInstanceVar< TGenericPtr >			i_listener;			// listener we need to inform about this block's deactivation
	TInstanceVar< TGenericPtr >			i_listenerData;		// instance data in context of which the listener operates

public:
	CQuestGraphR6QuestPauseBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const;
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Octagon; }
	virtual Color GetClientColor() const { return Color( 213, 163, 163 ); }
	virtual String GetBlockCategory() const { return TXT( "R6 Quest" ); }
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
	virtual void SaveGame( InstanceBuffer& data, IGameSaver* saver ) const override;
	virtual void LoadGame( InstanceBuffer& data, IGameLoader* loader ) const override;

	// ------------------------------------------------------------------------
	// Listener setter
	// ------------------------------------------------------------------------
	void AttachListener( InstanceBuffer& data, const CQuestGraphR6QuestBlock& listener, InstanceBuffer& listenerData ) const;
};

BEGIN_CLASS_RTTI( CQuestGraphR6QuestPauseBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_INLINED( m_condition, TXT( "Condition that needs to be met to unpause quest" ) )
END_CLASS_RTTI()

