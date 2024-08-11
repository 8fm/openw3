#include "build.h"
#include "questPhaseBlock.h"
#include "questThread.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/contentManager.h"
#include "../engine/soundFileLoader.h"
#include "../engine/worldStreaming.h"

#ifndef NO_DEBUG_WINDOWS
#include "../engine/soundSystem.h"
#endif

IMPLEMENT_ENGINE_CLASS( CQuestPhaseBlock );
IMPLEMENT_RTTI_ENUM( EQuestPhaseSaveMode );

CQuestPhaseBlock::CQuestPhaseBlock()
	: m_isBlackscreenPhase( false )
	, m_saveMode( QPSM_SavesAllowed )
	, m_blackscreenFadeDuration( 1.0f )
	, m_purgeSavedData( false )
{
	m_name = TXT( "Phase" );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

Color CQuestPhaseBlock::GetTitleColor() const
{
	if ( m_isBlackscreenPhase )
	{
		return Color( 0, 0, 0 );
	}
	return CQuestScopeBlock::GetTitleColor();
}

Color CQuestPhaseBlock::GetClientColor() const
{
	if ( m_isBlackscreenPhase )
	{
		return Color( 0, 0, 0 );
	}
	return Color( 163, 163, 163 );
}

#endif

void CQuestPhaseBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_loadingFence;
	compiler << i_saveLock;
	compiler << i_layersLoaded;
	compiler << i_soundBanksRequested;
}

void CQuestPhaseBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );
	instanceData[ i_loadingFence ] = nullptr;
	instanceData[ i_layersLoaded ] = false;
	instanceData[ i_saveLock ] = CGameSessionManager::GAMESAVELOCK_INVALID;
	instanceData[ i_soundBanksRequested ] = false;
}

void CQuestPhaseBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	// Hack for disabling fade out while loading a blackscreen phase
	if ( m_isBlackscreenPhase == true && inputName != CName::NONE )
	{
		GGame->StartFade( false, TXT( "CQuestPhaseBlock_OnActivate" ), m_blackscreenFadeDuration );
		GGame->SetFadeLock( TXT( "CQuestPhaseBlock_OnActivate" ) );
	}

	// create a save lock, if required ( providing one hasn't already been created )
	if ( m_saveMode == QPSM_SaveBlocker && data[ i_saveLock ] < 0 )
	{
#ifdef NO_EDITOR_GRAPH_SUPPORT
		String lockReason = TXT("Phase");
#else
		String lockReason = String::Printf( TXT("Phase '%ls'"), GetCaption().AsChar() );
#endif
		SGameSessionManager::GetInstance().CreateNoSaveLock( lockReason, data[ i_saveLock ] );
	}

	TBaseClass::OnActivate( data, inputName, parentThread );
}

Bool CQuestPhaseBlock::OnProcessActivation( InstanceBuffer& data ) const
{
	if ( m_playGoChunk )
	{
		if ( !GContentManager->IsContentActivated( m_playGoChunk ) )
		{
			return false;
		}
	}

	// Ask parent class
	if ( !TBaseClass::OnProcessActivation( data ) )
	{
		return false;
	}

	// load sound banks
	if( !data[ i_soundBanksRequested ] )
	{
		Uint32 banksCount = m_soundBanksDependency.Size();
		for( Uint32 i = 0; i != banksCount; ++i )
		{
			CSoundBank* soundBank = CSoundBank::FindSoundBank( m_soundBanksDependency[ i ] );
			RED_ASSERT( m_soundBanksDependency[ i ] != CName::NONE, 
				TXT( "CQuestPhaseBlock::OnProcessActivation - The quest block has NONE as bank dependency! - bankName [%ls] - guid [%ls]" ), 
				m_soundBanksDependency[ i ].AsChar(), String::Printf( RED_GUID_STRING_FORMAT, GetGUID().parts.A, GetGUID().parts.B, GetGUID().parts.C, GetGUID().parts.D ).AsChar() );
			RED_ASSERT( soundBank != nullptr || m_soundBanksDependency[ i ] == CName::NONE, 
				TXT( "CQuestPhaseBlock::OnProcessActivation - The bank is not in the banks array! - bankName [%ls] - guid [%ls]" ), 
				m_soundBanksDependency[ i ].AsChar(), String::Printf( RED_GUID_STRING_FORMAT, GetGUID().parts.A, GetGUID().parts.B, GetGUID().parts.C, GetGUID().parts.D ).AsChar() );
			if( !soundBank ) continue;
			soundBank->QueueLoading();
#ifndef NO_DEBUG_WINDOWS
			String guidString = String::EMPTY;
			CGUID guid = GetGUID();
			guidString = guidString.Printf( RED_GUID_STRING_FORMAT, guid.parts.A, guid.parts.B, guid.parts.C, guid.parts.D );
			GSoundSystem->AddQuestBank( soundBank->GetFileName().AsString(), guidString );
#endif
		}
		data[ i_soundBanksRequested ] = true;
	}
	if( data[i_soundBanksRequested] )
	{
		Uint32 banksCount = m_soundBanksDependency.Size();
		for( Uint32 i = 0; i != banksCount; ++i )
		{
			CSoundBank* soundBank = CSoundBank::FindSoundBank( m_soundBanksDependency[ i ] );
			if( !soundBank ) continue;
			if( !soundBank->IsLoadingFinished() )
			{
				return false;
			}
			RED_ASSERT( soundBank->IsLoaded(), TXT("CQuestPhaseBlock::OnProcessActivation - sound bank didn't load properly - bank: [%ls] - result [%ls] - guid [%ls]"), 
				soundBank->GetFileName().AsChar(), soundBank->GetLoadingResultString().AsChar(), 
				String::Printf( RED_GUID_STRING_FORMAT, GetGUID().parts.A, GetGUID().parts.B, GetGUID().parts.C, GetGUID().parts.D ).AsChar() );
		}
	}

	// All shit is attached
	return true;
}

void CQuestPhaseBlock::OnDeactivate( InstanceBuffer& data ) const
{
	// Hide all specified layers
	// WARNING! This is about to be removed. Please don't use.
	if ( !m_layersToLoad.Empty() )
	{
		TDynArray<String> emptyArray;
		GGame->ScheduleLayersVisibilityChange( String::EMPTY, m_layersToLoad, emptyArray, false );
	}

	if( data[ i_soundBanksRequested ] )
	{
		// Unload related sound banks
		Uint32 banksCount = m_soundBanksDependency.Size();
		for( Uint32 i = 0; i != banksCount; ++i )
		{
			CSoundBank* soundBank = CSoundBank::FindSoundBank( m_soundBanksDependency[ i ] );
			if( !soundBank ) continue;
#ifndef NO_DEBUG_WINDOWS
			String guidString = String::EMPTY;
			CGUID guid = GetGUID();
			guidString = guidString.Printf( RED_GUID_STRING_FORMAT, guid.parts.A, guid.parts.B, guid.parts.C, guid.parts.D );
			GSoundSystem->RemoveQuestBank( soundBank->GetFileName().AsString(), guidString );
#endif
			soundBank->Unload();
		}
		data[ i_soundBanksRequested ] = false;
	}

	// Release save lock
	if ( data[ i_saveLock ] >= 0 )
	{
		SGameSessionManager::GetInstance().ReleaseNoSaveLock( data[ i_saveLock ] );
		data[ i_saveLock ] = CGameSessionManager::GAMESAVELOCK_INVALID;
	}

	// Destroy the loading fence if still around
	if ( data[ i_loadingFence ] )
	{
		CWorldLayerStreamingFence* loadingFence = (CWorldLayerStreamingFence*) data[ i_loadingFence ];
		loadingFence->Release();
		data[ i_loadingFence ] = nullptr;
	}

	data[ i_layersLoaded ] = false;

	TBaseClass::OnDeactivate( data );

	if ( m_isBlackscreenPhase == true )
	{
		GGame->ResetFadeLock( TXT( "CQuestPhaseBlock_OnDeactivate" ) );
		GGame->StartFade( true, TXT( "CQuestPhaseBlock_OnDeactivate" ), m_blackscreenFadeDuration );
	}	
}

void CQuestPhaseBlock::OnExecute( InstanceBuffer& data ) const
{
	// make sure PlayGo content is activated before proceeding
	//TBD: activte vs installed on map and dialog choices? But could be guarding this execution, so not now.
	if ( m_playGoChunk )
	{
		if ( !GContentManager->IsContentActivated( m_playGoChunk ) )
			return;
	}

	// Start loading the layers
	if ( !m_layersToLoad.Empty() )
	{	
		// start loading layers
		if ( !data[ i_layersLoaded ] && !data[ i_layersLoaded ] )
		{
			// create fence
			const String fenceName = GetDebugName();
			CWorldLayerStreamingFence* loadingFence = new CWorldLayerStreamingFence( fenceName );

			// start loading the layers
			TDynArray< String > emptyLayers;
			GGame->ScheduleLayersVisibilityChange( String::EMPTY, emptyLayers, m_layersToLoad, m_purgeSavedData, loadingFence );

			// remember the fence
			data[ i_loadingFence ] = (TGenericPtr)loadingFence;
			data[ i_layersLoaded ] = true;
		}

		// wait for the layers
		if ( data[ i_loadingFence ] && data[ i_layersLoaded ] )
		{
			// Check if the loading of the referenced layers has completed
			CWorldLayerStreamingFence* loadingFence = (CWorldLayerStreamingFence*) data[ i_loadingFence ];
			if ( !loadingFence->CheckIfCompleted() )
				return;

			// We indeed have completed the loading
			data[ i_loadingFence ] = nullptr;
			loadingFence->Release();
		}
	}

	// check black screen
	if ( m_isBlackscreenPhase == true && GGame->IsFadeInProgress() == true )
	{
		return;
	}

	if( data[i_soundBanksRequested] )
	{
		Uint32 banksCount = m_soundBanksDependency.Size();
		for( Uint32 i = 0; i != banksCount; ++i )
		{
			CSoundBank* soundBank = CSoundBank::FindSoundBank( m_soundBanksDependency[ i ] );
			if( !soundBank ) 
			{
				if( m_soundBanksDependency[ i ] != CName::NONE )
				{
					RED_LOG("SoundBanksMissing", TXT( "CQuestPhaseBlock::OnExecute - The bank is not in the banks array! - bankname [%ls] - guid [%ls]" ), 
						m_soundBanksDependency[ i ].AsChar(), String::Printf( RED_GUID_STRING_FORMAT, GetGUID().parts.A, GetGUID().parts.B, GetGUID().parts.C, GetGUID().parts.D ).AsChar() );
				}
				continue;
			}
			RED_ASSERT( soundBank->IsLoaded(), TXT( "The bank is not loaded even though there is quest phase that requested it! - bankname [%ls]- guid [%ls]" ), 
				m_soundBanksDependency[ i ].AsChar(), String::Printf( RED_GUID_STRING_FORMAT, GetGUID().parts.A, GetGUID().parts.B, GetGUID().parts.C, GetGUID().parts.D ).AsChar() );
#ifndef NO_DEBUG_WINDOWS
			String guidString = String::EMPTY;
			CGUID guid = GetGUID();
			guidString = guidString.Printf( RED_GUID_STRING_FORMAT, guid.parts.A, guid.parts.B, guid.parts.C, guid.parts.D );
			GSoundSystem->CheckQuestBank( soundBank->GetFileName().AsString(), guidString );
#endif
		}
	}


	TBaseClass::OnExecute( data );
}
