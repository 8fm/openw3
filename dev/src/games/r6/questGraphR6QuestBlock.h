#pragma once

#include "../../common/game/questScopeBlock.h"
#include "../../common/game/questTriggerCondition.h"
#include "idResource.h"

class CQuestGraphR6QuestConditionBlock;

enum EQuestType
{
	QT_Main,
	QT_Side,
	QT_Living,
	QT_Fedex,
};

BEGIN_ENUM_RTTI( EQuestType );
ENUM_OPTION( QT_Main );
ENUM_OPTION( QT_Side );
ENUM_OPTION( QT_Living );
ENUM_OPTION( QT_Fedex );
END_ENUM_RTTI();

enum EQuestState
{
	QS_ConditionChecking, // Block is in this state as long as one of the R6QuestConditionBlock is done.
	QS_WaitingForTeam,	// Checks if all required members are chosen by the player
	QS_Performing, // Quest is active
	QS_Paused, // R6 Pause Block is active, so quest is paused until pause condition is met
	QS_PlayerOutsideArea, // Player is outside of the quest area, so quest is paused (case similar to above)
	QS_Unpausing, // Player came back to quest area and optional interactive dialog is playing
	QS_Inactive, // Quest isn't active
	QS_Blocked, // Quest is blocked by other (main) quest
};

BEGIN_ENUM_RTTI( EQuestState );
	ENUM_OPTION( QS_ConditionChecking );
	ENUM_OPTION( QS_WaitingForTeam );
	ENUM_OPTION( QS_Performing );
	ENUM_OPTION( QS_Paused );
	ENUM_OPTION( QS_PlayerOutsideArea );
	ENUM_OPTION( QS_Unpausing );
	ENUM_OPTION( QS_Inactive );
	ENUM_OPTION( QS_Blocked );
END_ENUM_RTTI();

// A quest block which is equivalent to quest 
// This block is used with CQuestGraphR6QuestConditionBlock (which should be inside it's scope), 
// there has to be at least one quest trigger 
class CQuestGraphR6QuestBlock : public CQuestScopeBlock
{
	DECLARE_ENGINE_CLASS( CQuestGraphR6QuestBlock, CQuestScopeBlock, 0 )

private:
	// properties data
	EQuestType							m_type;
	TDynArray< CName >					m_companion;
	CName								m_questAreaTag;
	TSoftHandle< CInteractiveDialog >	m_unpausedDialog;
	

	// instance data
	TInstanceVar< EQuestState >		i_questState;
	TInstanceVar< EQuestState >		i_questStateBeforePause;
	TInstanceVar< TGenericPtr >		i_pauseQuestAreaTrigger;
	TInstanceVar< TGenericPtr >		i_unpausedDialogRequest;
	TInstanceVar< TDynArray< IQuestCondition* > > i_outerPauseConditions;

public:
	CQuestGraphR6QuestBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual EGraphBlockShape GetBlockShape() const override { return GBS_Rounded; }
	virtual Color GetClientColor() const override { return Color( 213, 163, 163 ); }
	virtual String GetBlockCategory() const override { return TXT( "R6 Quest" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const override { return true; }
#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const override;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const override;
	virtual Bool OnProcessActivation( InstanceBuffer& data ) const override;

	virtual void OnExecute( InstanceBuffer& data ) const override;
	virtual void OnDeactivate( InstanceBuffer& data ) const override;

	// Restart block's contents (block is correctly loaded from savegame)
	virtual Bool CanActivateInputsOnLoad( CQuestGraphBlock::EState activationState ) const { return activationState != ST_ACTIVE; }

	// ------------------------------------------------------------------------
	// Saving the game state
	// ------------------------------------------------------------------------
	virtual void SaveGame( InstanceBuffer& data, IGameSaver* saver ) const override;
	virtual void LoadGame( InstanceBuffer& data, IGameLoader* loader ) const override;
	
	// ------------------------------------------------------------------------
	// (Un)blocking methods
	// ------------------------------------------------------------------------
	void Unblock( InstanceBuffer& data) const;
	void Block( InstanceBuffer& data) const;

	// ------------------------------------------------------------------------
	// Starting quest (one of the start condition was met)
	// ------------------------------------------------------------------------
	void StartQuest( InstanceBuffer& data, const CQuestGraphR6QuestConditionBlock* metCond ) const;

	// ------------------------------------------------------------------------
	// Pausing quest until condition is met (quest pause block was activated)
	// ------------------------------------------------------------------------
	void PauseQuest( InstanceBuffer& data, IQuestCondition* pauseCond ) const;

	RED_INLINE EQuestType GetType() const { return m_type; }

private:
	Bool CheckTeam( InstanceBuffer& data ) const;
};

BEGIN_CLASS_RTTI( CQuestGraphR6QuestBlock )
	PARENT_CLASS( CQuestScopeBlock )
	PROPERTY_EDIT( m_type, TXT( "Quest type" ) );
	PROPERTY_EDIT( m_companion, TXT( "Required members" ) );
	PROPERTY_EDIT( m_questAreaTag, TXT( "Quest area tag" ) );
	PROPERTY_EDIT_NAME( m_unpausedDialog, TXT("i_unpauseDialog"), TXT( "Dialog to play when coming back to quest area" ) );
END_CLASS_RTTI()

