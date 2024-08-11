
#include "build.h"
#include "questGraphR6QuestPauseBlock.h"
#include "questGraphR6QuestBlock.h"
#include "../../common/game/questGraphSocket.h"
#include "../../common/game/questGraph.h"
#include "../../common/core/instanceDataLayoutCompiler.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestGraphR6QuestPauseBlock );

CQuestGraphR6QuestPauseBlock::CQuestGraphR6QuestPauseBlock() 
{ 
	m_name = TXT("R6 Quest Pause"); 
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestGraphR6QuestPauseBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

Bool CQuestGraphR6QuestPauseBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const 
{ 
	return graph && graph->GetParent()->IsExactlyA< CQuestGraphR6QuestBlock >();
}

#endif

void CQuestGraphR6QuestPauseBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_listener;
	compiler << i_listenerData;
	compiler << i_condition;
}

void CQuestGraphR6QuestPauseBlock::OnInitInstance( InstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );

	data[ i_listener ] = nullptr;
	data[ i_listenerData ] = nullptr;
	data[ i_condition ] = nullptr;
}

void CQuestGraphR6QuestPauseBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	RED_ASSERT( m_condition, TXT( "Quest will never be unocked" ) );

	IQuestCondition* condition = Cast< IQuestCondition >( m_condition->Clone( const_cast< CQuestGraphR6QuestPauseBlock* >( this ) ) );
	condition->AddToRootSet();
	data[ i_condition ] = reinterpret_cast< TGenericPtr >( condition );

}

void CQuestGraphR6QuestPauseBlock::OnExecute( InstanceBuffer& data ) const
{
	const CQuestGraphR6QuestBlock* listener = reinterpret_cast< const CQuestGraphR6QuestBlock*>( data[ i_listener ] );

	RED_ASSERT( IsBlockEnabled( data ) );

	if ( data[ i_listener ] != nullptr )
	{
		// In first tick pause quest
		InstanceBuffer* listenerData = reinterpret_cast< InstanceBuffer*>( data[ i_listenerData ] );
		data[ i_listener ] = nullptr;
		data[ i_listenerData ] = nullptr;

		IQuestCondition* condition = reinterpret_cast< IQuestCondition* > ( data[ i_condition ] );

		listener->PauseQuest( *listenerData, condition );
	}
	else
	{
		// second tick (when unpaused) - end
		ActivateOutput( data, CNAME( Out ) );
	}
}

void CQuestGraphR6QuestPauseBlock::OnDeactivate( InstanceBuffer& data ) const
{
	IQuestCondition* condition = reinterpret_cast< IQuestCondition* > ( data[ i_condition ] );
	condition->RemoveFromRootSet();
	data[ i_condition ] = nullptr;
}

void CQuestGraphR6QuestPauseBlock::AttachListener( InstanceBuffer& data, const CQuestGraphR6QuestBlock& listener, InstanceBuffer& listenerData ) const
{
	data[ i_listener ] = reinterpret_cast< TGenericPtr >(const_cast< CQuestGraphR6QuestBlock* >( &listener ));
	data[ i_listenerData ] = reinterpret_cast< TGenericPtr >( &listenerData );
}

void CQuestGraphR6QuestPauseBlock::SaveGame( InstanceBuffer& data, IGameSaver* saver ) const
{
	TBaseClass::SaveGame( data, saver );

	IQuestCondition* condition = reinterpret_cast< IQuestCondition* > ( data[ i_condition ] );
	condition->SaveGame( saver );
}

void CQuestGraphR6QuestPauseBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader ) const
{
	TBaseClass::LoadGame( data, loader );

	IQuestCondition* condition = reinterpret_cast< IQuestCondition* > ( data[ i_condition ] );
	condition->LoadGame( loader );
}
