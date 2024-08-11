#include "build.h"
#include "questGraphSocket.h"
#include "questCheckpointBlock.h"
#include "gameSaver.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestCheckpointBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestCheckpointBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestCheckpointBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_savePhase;
}

void CQuestCheckpointBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );

	instanceData[ i_savePhase ] = SP_None;
}

void CQuestCheckpointBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	data[ i_savePhase ] = m_enableSaving ? SP_MakeSave : SP_Exit;
}

void CQuestCheckpointBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	const ESaveGameType typeToCheck = /* m_ignoreSaveLocks ? SGT_ForcedCheckPoint : */ SGT_CheckPoint; // m_ignoreSaveLocks to be removed soon(ish)
	ESavePhase phase = ( ESavePhase )( data[ i_savePhase ] );

	if ( GCommonGame->CERT_HACK_IsInStartGame() )
	{
		// saving during StartGame() isn't a good idea, bad things happen
		// AFAIK we have only 1 situation in the whole game where this can happen
		// but we can't remove the checkpoint from there.
		// This hack is to delay the checkpoint at least until we get out of StartGame() scope. 

		RED_LOG( Save, TXT("skipping checkpoint block execution - CERT_HACK_IsInStartGame") );
		return; 
	}

	if ( GCommonGame->IsSavingGame() )
	{
		return;
	}

	/*if ( GCommonGame->IsSavedRecently( true ) )
	{
		SGameSessionManager::GetInstance().AddFailedSaveDebugMessage( typeToCheck, TXT("Autosaved no more than a minute ago."), GetDebugName().AsChar() );
		RED_LOG( Save, TXT("Checkpoint %ls failed, because the game was autosaved no more than a minute ago."), GetDebugName().AsChar() );
		phase = SP_Exit;
	}*/

	if ( phase == SP_MakeSave && SGameSessionManager::GetInstance().AreGameSavesLocked( typeToCheck ) )
	{
		SGameSessionManager::GetInstance().AddFailedSaveDebugMessage( typeToCheck, TXT("Saves were locked."), GetDebugName().AsChar() );
		RED_LOG( Save, TXT("Checkpoint %ls failed, because of a save lock."), GetDebugName().AsChar() );
		phase = SP_Exit;
	}

	switch( phase )
	{
	case SP_MakeSave:
		{
			SGameSessionManager::GetInstance().ShowBlackscreenAfterRestore( GGame->IsBlackscreen() );
			GCommonGame->RequestGameSave( typeToCheck, -1, GetDebugName() );
			data[ i_savePhase ] = SP_WaitForSaveEnd;
			// fallthrough
		}

	case SP_WaitForSaveEnd:
		{
			if ( GCommonGame->IsSavingGame() == false )
			{
				data[ i_savePhase ] = SP_Exit;
			}
			else
			{
				break;
			}
		}

	case SP_Exit:
	default:
		{
			ActivateOutput( data, CNAME( Out ) );
			break;
		}
	}
}

void CQuestCheckpointBlock::OnDeactivate( InstanceBuffer& data ) const
{
	TBaseClass::OnDeactivate( data );

	data[ i_savePhase ] = SP_None;

	SGameSessionManager::GetInstance().ShowBlackscreenAfterRestore( false );
}

void CQuestCheckpointBlock::SaveGame( InstanceBuffer& data, IGameSaver* saver ) const
{
	TBaseClass::SaveGame( data, saver );
}

void CQuestCheckpointBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader ) const
{
	TBaseClass::LoadGame( data, loader );
	ActivateOutput( data, CNAME( Out ) );
}
