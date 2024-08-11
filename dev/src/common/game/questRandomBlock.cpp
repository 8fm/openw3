/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "questRandomBlock.h"
#include "questGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CQuestRandomBlock );

void CQuestRandomBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{	
	Int32 cChoice = 0; 	
	if( m_randomOutputs.Size() > 1 )
	{
		cChoice = GEngine->GetRandomNumberGenerator().Get< Int32 >( m_randomOutputs.Size() );
		if( cChoice == data[ i_lastChoice ] )
		{		
			cChoice = ( cChoice + 1 ) % m_randomOutputs.Size();
		}
	}
	data[ i_lastChoice ] = cChoice;

	ActivateOutput( data, m_randomOutputs[ cChoice ] );
}

void CQuestRandomBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );
	instanceData[ i_lastChoice ] = -1;
}

void CQuestRandomBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_lastChoice;
}
#ifndef NO_EDITOR_GRAPH_SUPPORT
void CQuestRandomBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	if ( m_randomOutputs.Empty() == true )
	{
		m_randomOutputs.PushBack( CNAME( Random1 ) );
	}

	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );

	for ( Uint32 i = 0; i < m_randomOutputs.Size(); ++i )
	{
		CreateSocket( CQuestGraphSocketSpawnInfo( m_randomOutputs[ i ], LSD_Output, LSP_Right ) );
	}
}

void CQuestRandomBlock::AddOutput()
{
	m_randomOutputs.PushBack( CName( String::Printf( TXT( "Random %d" ), m_randomOutputs.Size() + 1 ) ) );
	OnRebuildSockets();
}

void CQuestRandomBlock::RemoveOutput()
{
	m_randomOutputs.PopBack();
	OnRebuildSockets();
}

Bool CQuestRandomBlock::CanAddOutput()
{
	return true;
}

Bool CQuestRandomBlock::CanRemoveOutput()
{
	return m_randomOutputs.Size() > 1;
}

#endif