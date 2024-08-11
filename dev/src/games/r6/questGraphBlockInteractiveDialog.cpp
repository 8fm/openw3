#include "build.h"
#include "idResource.h"
#include "idInstance.h"
#include "questGraphBlockInteractiveDialog.h"

#include "../../common/game/questGraphSocket.h"
#include "../../common/game/questThread.h"
#include "../../common/core/instanceDataLayoutCompiler.h"
#include "../../common/engine/graphConnectionRebuilder.h"


IMPLEMENT_ENGINE_CLASS( CQuestGraphBlockInteractiveDialog );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestGraphBlockInteractiveDialog::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create mandatory sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Start ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Finished ), LSD_Output, LSP_Right ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Interrupted ), LSD_Output, LSP_Right ) );

	// Create scene interface sockets (if a dialog is set)
	RebuildSceneInterfaceSockets();
}

void CQuestGraphBlockInteractiveDialog::RebuildSceneInterfaceSockets()
{
	// TODO: add inputs and outputs from the dialog file
}

void CQuestGraphBlockInteractiveDialog::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == CNAME( dialog ) )
	{
		OnRebuildSockets();
	}
}

#endif

void CQuestGraphBlockInteractiveDialog::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_request;
}

void CQuestGraphBlockInteractiveDialog::OnInitInstance( InstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );

	data[ i_request ] = nullptr;
}

void CQuestGraphBlockInteractiveDialog::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	SDialogStartRequest* info = CQuestInteractiveDialogHelper::PlayDialogForPlayer( m_dialog );
	data[ i_request ] = reinterpret_cast< TGenericPtr > ( info );
}

void CQuestGraphBlockInteractiveDialog::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	SDialogStartRequest *info = reinterpret_cast< SDialogStartRequest* > ( data[ i_request ] );

	EIDPlayState state = CQuestInteractiveDialogHelper::IsDialogPlaying( info );
	if ( state == DIALOG_Finished || state == DIALOG_Error )
	{
		ActivateOutput( data, CNAME( Finished ) );
	}
	else if ( state == DIALOG_Interrupted )
	{
		ActivateOutput( data, CNAME( Interrupted ) );
	}
}

void CQuestGraphBlockInteractiveDialog::OnDeactivate( InstanceBuffer& data ) const
{
	SDialogStartRequest *info = reinterpret_cast< SDialogStartRequest* > ( data[ i_request ] );
	if ( info )
	{
		delete info;
		data[ i_request ] = nullptr;
	}

	TBaseClass::OnDeactivate( data );
}

void CQuestGraphBlockInteractiveDialog::SaveGame( InstanceBuffer& data, IGameSaver* saver ) const 
{
	TBaseClass::SaveGame( data, saver );

	// TODO
}

void CQuestGraphBlockInteractiveDialog::LoadGame( InstanceBuffer& data, IGameLoader* loader ) const
{
	TBaseClass::LoadGame( data, loader );

	// TODO
}


SDialogStartRequest* CQuestInteractiveDialogHelper::PlayDialogForPlayer( const TSoftHandle<CInteractiveDialog>& dialog )
{
	if ( dialog.IsEmpty() )
	{
		return nullptr;
	}

	CIDInterlocutorComponent* player = Cast < CR6Player > ( GCommonGame->GetPlayer() )->GetInterlocutorComponent();
	SDialogStartRequest *info = new SDialogStartRequest( player, player, dialog );
	GCommonGame->GetSystem < CInteractiveDialogSystem > ()->RequestDialog( *info );
	if ( info->m_state == DIALOG_Error )
	{
		delete info;
		return nullptr;
	}

	return info;
}

EIDPlayState CQuestInteractiveDialogHelper::IsDialogPlaying( SDialogStartRequest *info )
{
	if ( nullptr == info )
	{
		return DIALOG_Error;
	}

	GCommonGame->GetSystem < CInteractiveDialogSystem > ()->RequestDialog( *info );
	if ( info->m_state == DIALOG_Loading )
	{
		return DIALOG_Loading; // ...just wait for it
	}

	const CInteractiveDialogInstance* instance = GCommonGame->GetSystem < CInteractiveDialogSystem > ()->GetDialogInstance(	info->m_startedInstanceID ); 
	if ( !instance )
	{
		return DIALOG_Finished;
	}
	return instance->GetPlayState();
}
