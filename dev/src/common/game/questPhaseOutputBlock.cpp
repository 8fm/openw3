#include "build.h"
#include "questGraphSocket.h"
#include "questScopeBlock.h"
#include "questPhaseOutputBlock.h"
#include "questThread.h"
#include "questsSystem.h"
#include "questGraph.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestPhaseOutputBlock )

CQuestPhaseOutputBlock::CQuestPhaseOutputBlock()
{
	m_name = TXT( "Out" );
	m_socketID = CNAME( Out );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestPhaseOutputBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Space ), LSD_Input, LSP_Left ) );
}

Bool CQuestPhaseOutputBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const
{
	return graph && ( graph->GetParent()->IsExactlyA< CQuestPhase >() || graph->GetParent()->IsA< CQuestScopeBlock >() );
}

#endif

void CQuestPhaseOutputBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_listener;
	compiler << i_listenerData;
}

void CQuestPhaseOutputBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );
	instanceData[ i_listener ] = NULL;
	instanceData[ i_listenerData ] = NULL;
}

void CQuestPhaseOutputBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	if ( data[ i_listener ] != NULL )
	{
		InstanceBuffer* listenerData = reinterpret_cast< InstanceBuffer*>( data[ i_listenerData ] );
		CQuestScopeBlock* listener = data[ i_listener ];
		data[ i_listener ] = NULL;
		data[ i_listenerData ] = NULL;

		listener->OnScopeEndReached( *listenerData, m_socketID );
	}
}

void CQuestPhaseOutputBlock::AttachListener( InstanceBuffer& data, 
											 const CQuestScopeBlock& listener, 
											 InstanceBuffer& listenerData ) const
{
	data[ i_listener ] = const_cast< CQuestScopeBlock* >( &listener );
	data[ i_listenerData ] = reinterpret_cast< TGenericPtr >( &listenerData );
}
