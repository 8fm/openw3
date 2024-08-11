
#include "build.h"
#include "questGraphR6QuestConditionBlock.h"
#include "questGraphR6QuestBlock.h"
#include "../../common/game/questGraph.h"
#include "../../common/core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CQuestGraphR6QuestConditionBlock );

CQuestGraphR6QuestConditionBlock::CQuestGraphR6QuestConditionBlock() 
{ 
	m_name = TXT("R6 Quest Condition"); 
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
Bool CQuestGraphR6QuestConditionBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const 
{ 
	return graph && graph->GetParent()->IsExactlyA< CQuestGraphR6QuestBlock >();
}
#endif

void CQuestGraphR6QuestConditionBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_listener;
	compiler << i_listenerData;
}

void CQuestGraphR6QuestConditionBlock::OnInitInstance( InstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );

	data[ i_listener ] = nullptr;
	data[ i_listenerData ] = nullptr;
}

void CQuestGraphR6QuestConditionBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );
}

void CQuestGraphR6QuestConditionBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );
}

void CQuestGraphR6QuestConditionBlock::OnScopeEndReached( InstanceBuffer& data, const CName& outSocket ) const
{
	TBaseClass::OnScopeEndReached( data, outSocket );

	const CQuestGraphR6QuestBlock* listener = reinterpret_cast< const CQuestGraphR6QuestBlock*>( data[ i_listener ] );

	RED_ASSERT( data[ i_listener ], TXT("There should always be R6 Quest block that listen us") );
	RED_ASSERT( IsBlockEnabled( data ) );
	
	InstanceBuffer* listenerData = reinterpret_cast< InstanceBuffer*>( data[ i_listenerData ] );
	data[ i_listener ] = nullptr;
	data[ i_listenerData ] = nullptr;

	listener->StartQuest( *listenerData, this );
}

void CQuestGraphR6QuestConditionBlock::OnDeactivate( InstanceBuffer& data ) const
{
	TBaseClass::OnDeactivate( data );
}

void CQuestGraphR6QuestConditionBlock::SaveGame( InstanceBuffer& data, IGameSaver* saver )
{
	TBaseClass::SaveGame( data, saver );
}

void CQuestGraphR6QuestConditionBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader )
{
	TBaseClass::LoadGame( data, loader );
}

void CQuestGraphR6QuestConditionBlock::AttachListener( InstanceBuffer& data, const CQuestGraphR6QuestBlock& listener, InstanceBuffer& listenerData ) const
{
	data[ i_listener ] = reinterpret_cast< TGenericPtr >(const_cast< CQuestGraphR6QuestBlock* >( &listener ));
	data[ i_listenerData ] = reinterpret_cast< TGenericPtr >( &listenerData );
}
