#include "build.h"

#include "../core/gameSave.h"

#include "questGraphSocket.h"
#include "questCutControlBlock.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestCutControlBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestCutControlBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Cut ), LSD_Input, LSP_Center ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Thunder ), LSD_Output, LSP_Center, ClassID<CQuestCutControlGraphSocket>() ) );
}

#endif


void CQuestCutControlBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	TDynArray< SBlockDesc > blocks;
	GetConnectedBlocks( CNAME( Thunder ), blocks );

	for( TDynArray< SBlockDesc >::const_iterator it = blocks.Begin(); it != blocks.End(); ++it )
	{
		it->block->Disable( data, !m_permanent );
	}

	if ( m_permanent )
	{
		// do not activate this kind of cut control twice
		if ( false == data[ i_wasOutputActivated ] )
		{
			ActivateOutputWithoutExiting( data, CNAME( Out ) );
		}
	}
	else
	{
		ActivateOutput( data, CNAME( Out ) );
	}
}

void CQuestCutControlBlock::OnDeactivate( InstanceBuffer& data ) const
{
	TBaseClass::OnDeactivate( data );

	if ( m_permanent )
	{
		TDynArray< SBlockDesc > blocks;
		GetConnectedBlocks( CNAME( Thunder ), blocks );

		for( TDynArray< SBlockDesc >::const_iterator it = blocks.Begin(); it != blocks.End(); ++it )
		{
			it->block->SetCanActivate( data, true );
		}
	}
}

void CQuestCutControlBlock::SaveGame( InstanceBuffer& data, IGameSaver* saver ) const
{
	CGameSaverBlock block( saver, CNAME( CutControlBlock ) );

	TBaseClass::SaveGame( data, saver );

	if ( m_permanent )
	{
		saver->WriteValue( CNAME( wasOutputActivated ), data[ i_wasOutputActivated ] );
	}
}

void CQuestCutControlBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader ) const
{
	CGameSaverBlock block( loader, CNAME( CutControlBlock ) );

	TBaseClass::LoadGame( data, loader );

	if ( m_permanent )
	{
		loader->ReadValue< Bool > ( CNAME( wasOutputActivated ), data[ i_wasOutputActivated ] );
	}
}
