#include "build.h"
#include "questGraphR6QuestBlock.h"
#include "questGraphR6QuestConditionBlock.h"
#include "questGraphR6QuestPauseBlock.h"

#include "../../common/game/questThread.h"
#include "../../common/game/questGraph.h"

#include "idResource.h"
#include "idSystem.h"
#include "idInstance.h"
#include "questGraphBlockInteractiveDialog.h"
#include "r6QuestSystem.h"

#include "companionComponent.h"

#include "../../common/core/instanceDataLayoutCompiler.h"
#include "../../common/core/gameSave.h"

IMPLEMENT_ENGINE_CLASS( CQuestGraphR6QuestBlock );
IMPLEMENT_RTTI_ENUM( EQuestType );
IMPLEMENT_RTTI_ENUM( EQuestState );

RED_DEFINE_STATIC_NAME( QuestState )

CQuestGraphR6QuestBlock::CQuestGraphR6QuestBlock() 
	: m_type( QT_Main )
	, m_unpausedDialog( nullptr )
{ 
	m_name = TXT("R6 Quest"); 
}

void CQuestGraphR6QuestBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_pauseQuestAreaTrigger;
	compiler << i_questState;
	compiler << i_questStateBeforePause;

	compiler << i_unpausedDialogRequest;
	compiler << i_outerPauseConditions;
}

void CQuestGraphR6QuestBlock::OnInitInstance( InstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );

	data[ i_pauseQuestAreaTrigger ] = nullptr;
	data[ i_questState ] = QS_Inactive;
	data[ i_questStateBeforePause ] = QS_Inactive;
	data[ i_unpausedDialogRequest ] = nullptr;
}

void CQuestGraphR6QuestBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	// create trigger to check if player is inside quest area
	CQuestInsideTriggerCondition* triggerCond = new CQuestInsideTriggerCondition();
	triggerCond->AddToRootSet();
	triggerCond->SetIsInside( false );
	triggerCond->SetTriggerTag( m_questAreaTag );
	data[ i_pauseQuestAreaTrigger ] = reinterpret_cast< TGenericPtr >( triggerCond );

	// quest is waiting for triggering condition to be met
	data[ i_questState ] = QS_ConditionChecking;

	// notify quest system about being activated
	CR6QuestSystem* questSystem = GCommonGame->GetSystem< CR6QuestSystem >();
	Bool activated = questSystem->ActivateQuest( *this );
	if ( !activated )
	{
		Block( data );
	}
}

Bool CQuestGraphR6QuestBlock::OnProcessActivation( InstanceBuffer& data ) const
{
	if ( !TBaseClass::OnProcessActivation( data ) )
	{
		return false;
	}

	RED_ASSERT( data[ i_thread ] );

	// Listen if any condition is met
	CQuestGraph* graph = const_cast< CQuestGraph* >( GetGraph() );
	TDynArray< CGraphBlock* >& graphBlocks = graph->GraphGetBlocks();
	CQuestGraphR6QuestConditionBlock* conditionBlock;
	CQuestGraphR6QuestPauseBlock* pauseBlock;
	Uint32 count = graphBlocks.Size();
	bool condBlockFound = false;
	for ( Uint32 i = 0; i < count; ++i )
	{
		if( graphBlocks[ i ] == nullptr )
			continue;

		if ( graphBlocks[ i ]->IsExactlyA< CQuestGraphR6QuestConditionBlock >() )
		{
			conditionBlock = SafeCast< CQuestGraphR6QuestConditionBlock >( graphBlocks[ i ] );

			conditionBlock->AttachListener( data[ i_thread ]->GetInstanceData(), *this, data );

			condBlockFound = true;
		}
		else if ( graphBlocks[ i ]->IsExactlyA< CQuestGraphR6QuestPauseBlock >() )
		{
			pauseBlock = SafeCast< CQuestGraphR6QuestPauseBlock >( graphBlocks[ i ] );

			pauseBlock->AttachListener( data[ i_thread ]->GetInstanceData(), *this, data );
		}
	}

	RED_ASSERT( condBlockFound, TXT("There is no R6 Quest Conditions in Quest") ); // there has to be at least one quest trigger 
	return true;
}

void CQuestGraphR6QuestBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	CQuestInsideTriggerCondition* pauseTriggerCond = reinterpret_cast< CQuestInsideTriggerCondition* > ( data[ i_pauseQuestAreaTrigger ] );
	EIDPlayState state;
	SDialogStartRequest* info = nullptr;
	TDynArray< IQuestCondition* >& pauseConds = data[ i_outerPauseConditions ];

	EQuestState questState = data[ i_questState ];
	switch ( questState )
	{
	case QS_ConditionChecking:
		// just waiting for callback
		break;

	case QS_WaitingForTeam:
		// checking if all required members are in team
		if ( CheckTeam( data ) )
		{
			data[ i_thread ]->SetPaused( false );
			
			data[ i_questState ] = QS_Performing;

			// activate trigger to know if player is inside quest area
			pauseTriggerCond->Activate();
		}
		break;

	case QS_Performing:
		if ( pauseTriggerCond->IsFulfilled() )
		{
			// player left quest area, pause quest
			data[ i_questState ] = QS_PlayerOutsideArea;
			pauseTriggerCond->SetIsInside( true );
			data[ i_thread ]->SetPaused( true );
		}
		break;

	case QS_Paused:
		// Quest is paused by R6 Quest Block
		for ( Int32 i = pauseConds.Size() - 1; i >= 0 ; i-- )
		{
			if ( pauseConds[ i ]->IsFulfilled() )
			{
				pauseConds[ i ]->Deactivate();
				pauseConds.RemoveAt( i );
			}
		}
		if ( data[ i_outerPauseConditions ].Empty() )
		{
			data[ i_questState ] = data[ i_questStateBeforePause ];
			if ( data[ i_questState ] != QS_PlayerOutsideArea )
			{
				data[ i_thread ]->SetPaused( false );
			}
		}
		break;

	case QS_PlayerOutsideArea:
		// Quest is paused because player left quest area
		if ( pauseTriggerCond->IsFulfilled())
		{
			// player return to the quest area, continue quest
			pauseTriggerCond->SetIsInside( false );
			data[ i_thread ]->SetPaused( false );

			// play optional interactive dialog
			info = CQuestInteractiveDialogHelper::PlayDialogForPlayer( m_unpausedDialog );
			data[ i_unpausedDialogRequest ] = reinterpret_cast< TGenericPtr > ( info );

			data[ i_questState ] = QS_Unpausing;
		}
		break;

	case QS_Unpausing:

		info = reinterpret_cast< SDialogStartRequest* > ( data[ i_unpausedDialogRequest ] );
		state = CQuestInteractiveDialogHelper::IsDialogPlaying( info );
		if ( state == DIALOG_Finished || state == DIALOG_Error || state == DIALOG_Interrupted )
		{
			// once dialog is finished continue the quest
			data[ i_questState ] = QS_Performing;
		}
		break;

	case QS_Blocked:
	case QS_Inactive:
	default:
		return;
	}
}

void CQuestGraphR6QuestBlock::StartQuest( InstanceBuffer& data, const CQuestGraphR6QuestConditionBlock* metCond ) const
{
	RED_ASSERT( data[ i_thread ] );

	// now we will check the team
	RED_ASSERT( data[ i_questState ] == QS_ConditionChecking );
	data[ i_questState ] = QS_WaitingForTeam;
	data[ i_thread ]->SetPaused( true );

	// notify quest system about starting quest
	CR6QuestSystem* questSystem = GCommonGame->GetSystem< CR6QuestSystem >();
	questSystem->StartQuest( *this, data );
	
	// disable all other quest triggering conditions
	CQuestGraph* graph = const_cast< CQuestGraph* >( GetGraph() );
	TDynArray< CGraphBlock* >& graphBlocks = graph->GraphGetBlocks();
	CQuestGraphR6QuestConditionBlock* conditionBlock;
	Uint32 count = graphBlocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if( graphBlocks[ i ] == nullptr )
		{
			continue;
		}
		if ( metCond == graphBlocks[ i ] )
		{
			continue;
		}
		if ( graphBlocks[ i ]->IsExactlyA< CQuestGraphR6QuestConditionBlock >() )
		{
			conditionBlock = SafeCast< CQuestGraphR6QuestConditionBlock >( graphBlocks[ i ] );

			conditionBlock->Disable( data[ i_thread ]->GetInstanceData() );
		}
	}
}

void CQuestGraphR6QuestBlock::PauseQuest( InstanceBuffer& data, IQuestCondition* pauseCond ) const
{
	pauseCond->Activate();
	data[ i_outerPauseConditions ].PushBack( pauseCond );

	if ( data[ i_questState ] == QS_Paused )
	{
		return;
	}
	data [ i_questStateBeforePause ] = data[ i_questState ];

	data[ i_questState ] = QS_Paused;

	data[ i_thread ]->SetPaused( true );
}

Bool CQuestGraphR6QuestBlock::CheckTeam( InstanceBuffer& data ) const
{
	CR6Player* player = Cast < CR6Player > ( GCommonGame->GetPlayer() );

	CCompanionComponent* companion = player->FindComponent< CCompanionComponent >();
	RED_ASSERT( companion );
	if ( companion == nullptr )
	{
		return true;
	}

	for ( auto it = m_companion.Begin(); it != m_companion.End(); ++it)
	{
		if ( !companion->HasMember( *it ) )
		{
			return false;
		}
	}

	return true;
}

void CQuestGraphR6QuestBlock::OnDeactivate( InstanceBuffer& data ) const
{
	// remove trigger
	CQuestInsideTriggerCondition* pauseTriggerCond = reinterpret_cast< CQuestInsideTriggerCondition* > ( data[ i_pauseQuestAreaTrigger ] );
	pauseTriggerCond->Deactivate();
	pauseTriggerCond->RemoveFromRootSet();
	data[ i_pauseQuestAreaTrigger ] = nullptr;

	// notify quest system about quest deactivating
	CR6QuestSystem* questSystem = GCommonGame->GetSystem< CR6QuestSystem >();
	questSystem->DeactivateQuest( *this, data );

	data[ i_questState ] = QS_Inactive;

	// remove dialogs
	SDialogStartRequest *info = reinterpret_cast< SDialogStartRequest* > ( data[ i_unpausedDialogRequest ] );
	if ( info )
	{
		delete info;
	}
	data[ i_unpausedDialogRequest ] = nullptr;

	// remove pause conditions
	TDynArray< IQuestCondition* >& pauseConds = data[ i_outerPauseConditions ];
	for ( Int32 i = pauseConds.Size() - 1; i >= 0 ; i-- )
	{
		pauseConds[ i ]->Deactivate();
	}
	pauseConds.Clear();

	TBaseClass::OnDeactivate( data );
}

void CQuestGraphR6QuestBlock::SaveGame( InstanceBuffer& data, IGameSaver* saver ) const
{
	TBaseClass::SaveGame( data, saver );

	// Area trigger
	CQuestInsideTriggerCondition* pauseTriggerCond = reinterpret_cast< CQuestInsideTriggerCondition* > ( data[ i_pauseQuestAreaTrigger ] );
	pauseTriggerCond->SaveGame( saver );

	// State
	saver->WriteValue( CNAME(QuestState), data[ i_questState ] );
}

void CQuestGraphR6QuestBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader ) const
{
	TBaseClass::LoadGame( data, loader );

	// Area trigger
	CQuestInsideTriggerCondition* pauseTriggerCond = reinterpret_cast< CQuestInsideTriggerCondition* > ( data[ i_pauseQuestAreaTrigger ] );
	pauseTriggerCond->LoadGame( loader );

	// Reset unpaused dialog
	data [ i_unpausedDialogRequest ] = nullptr;

	// State
	loader->ReadValue( CNAME(QuestState), data[ i_questState ] );
}

void CQuestGraphR6QuestBlock::Unblock( InstanceBuffer& data ) const
{
	// currently only main quest can be blocked
	RED_ASSERT( m_type == QT_Main );
	RED_ASSERT( data[ i_questState ] == QS_Blocked );

	data[ i_questState ] = QS_ConditionChecking;

	data[ i_thread ]->SetPaused( false );
}

void CQuestGraphR6QuestBlock::Block( InstanceBuffer& data ) const
{
	// currently only main quest can be blocked, only when checking condition
	RED_ASSERT( m_type == QT_Main );
	RED_ASSERT( data[ i_questState ] == QS_ConditionChecking );

	data[ i_questState ] = QS_Blocked;

	data[ i_thread ]->SetPaused( true );
}

