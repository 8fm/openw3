#include "build.h"
#include "questChangeWorldBlock.h"
#include "../../common/game/questGraphSocket.h"
#include "commonMapManager.h"
#include "../../common/core/depot.h"
#include "../../common/core/instanceDataLayoutCompiler.h"
#include "../../common/core/gameSave.h"
#include "../../common/engine/graphConnectionRebuilder.h"


IMPLEMENT_ENGINE_CLASS( CQuestChangeWorldBlock )

RED_DEFINE_STATIC_NAME( GetWorldPathFromAreaType )

CQuestChangeWorldBlock::CQuestChangeWorldBlock()
{
	m_name = TXT("Change world");
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestChangeWorldBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestChangeWorldBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	// Pass to base class
	TBaseClass::OnActivate( data, inputName, parentThread );
}

void CQuestChangeWorldBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	String worldFilePath;
	
	CCommonMapManager* manager = GCommonGame->GetSystem< CCommonMapManager >();
	if ( !manager )
	{
		ActivateOutput( data, CNAME( Out ) );
		return;
	}
	CallFunctionRet< String >( manager, CNAME( GetWorldPathFromAreaType ), m_newWorld, worldFilePath );
	if ( worldFilePath.Empty() )
	{
		ActivateOutput( data, CNAME( Out ) );
		return;
	}
	
	// Change
	if ( data[ i_wasChangeScheduled ] == false )
	{
		LOG_R4( TXT("Changing the world to '%ls'..."), worldFilePath.AsChar() );

		SChangeWorldInfo changeWorldInfo;
		changeWorldInfo.SetWorldName( worldFilePath );
		changeWorldInfo.SetVideoToPlay( m_loadingMovieName );
		changeWorldInfo.m_teleport.SetTarget( STeleportInfo::TargetType_Node, m_targetTag );
		GCommonGame->ScheduleWorldChange( changeWorldInfo );
		GCommonGame->HACK_P_1_1_SetWorldChangeFromQuest( true );

		data[ i_wasChangeScheduled ] = true;
	}
	else
	{
		if ( GGame->GetActiveWorld().IsValid() && GGame->GetActiveWorld()->DepotPath() == worldFilePath ) // hmmmm
		{
			ActivateOutput( data, CNAME( Out ) );
		}
	}
	
}

void CQuestChangeWorldBlock::OnDeactivate( InstanceBuffer& data ) const
{
	data[ i_wasChangeScheduled ] = false;
}

void CQuestChangeWorldBlock::SaveGame( InstanceBuffer& data, IGameSaver* saver ) const
{
	TBaseClass::SaveGame( data, saver );

	saver->WriteValue< Bool >( CNAME( WorldChangeScheduled ), data[ i_wasChangeScheduled ] );
}


void CQuestChangeWorldBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader ) const
{
	TBaseClass::LoadGame( data, loader );

	loader->ReadValue< Bool >( CNAME( WorldChangeScheduled ), data[ i_wasChangeScheduled ] );

	if ( data[ i_wasChangeScheduled ] == true )
	{
		ActivateOutput( data, CNAME( Out ) );
	}
}

void CQuestChangeWorldBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_wasChangeScheduled;
}

void CQuestChangeWorldBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );

	instanceData[ i_wasChangeScheduled ] = false;
}
