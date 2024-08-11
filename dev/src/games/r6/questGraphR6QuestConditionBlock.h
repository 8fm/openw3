#pragma once

#include "../../common/game/questScopeBlock.h"
#include "../../common/game/questTriggerCondition.h"
#include "idResource.h"

// A quest block which is equivalent to quest triggering condition.
// It's main job is to notify R6 Quest about condition meeting
// Always use it with CQuestGraphR6QuestBlock
class CQuestGraphR6QuestConditionBlock : public CQuestScopeBlock
{
	DECLARE_ENGINE_CLASS( CQuestGraphR6QuestConditionBlock, CQuestScopeBlock, 0 )

private:
	// instance data
	TInstanceVar< TGenericPtr >			i_listener;			//!< listener we need to inform about this block's deactivation
	TInstanceVar< TGenericPtr >			i_listenerData;		//!< instance data in context of which the listener operates

public:
	CQuestGraphR6QuestConditionBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const;
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Slanted; }
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

	// Restart block's contents (block is correctly loaded from savegame)
	virtual Bool CanActivateInputsOnLoad( CQuestGraphBlock::EState activationState ) const { return activationState != ST_ACTIVE; }

	// ------------------------------------------------------------------------
	// Scoped block related methods
	// ------------------------------------------------------------------------
	virtual void OnScopeEndReached( InstanceBuffer& data, const CName& outSocket) const;

	// ------------------------------------------------------------------------
	// Saving the game state
	// ------------------------------------------------------------------------
	virtual void SaveGame( InstanceBuffer& data, IGameSaver* saver );
	virtual void LoadGame( InstanceBuffer& data, IGameLoader* loader );

	// ------------------------------------------------------------------------
	// Listener setter
	// ------------------------------------------------------------------------
	void AttachListener( InstanceBuffer& data, const CQuestGraphR6QuestBlock& listener, InstanceBuffer& listenerData ) const;
};

BEGIN_CLASS_RTTI( CQuestGraphR6QuestConditionBlock )
	PARENT_CLASS( CQuestScopeBlock )
END_CLASS_RTTI()

