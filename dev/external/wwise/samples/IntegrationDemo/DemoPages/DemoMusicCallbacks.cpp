// DemoMusicCallbacks.cpp
// Copyright (C) 2010 Audiokinetic Inc
/// \file 
/// Defines the methods declared in DemoMarkers.h.

#include "stdafx.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>    // Sound engine

#include "../WwiseProject/GeneratedSoundBanks/Wwise_IDs.h"		// IDs generated by Wwise
#include "Menu.h"
#include "DemoMusicCallbacks.h"


/////////////////////////////////////////////////////////////////////
// DemoMarkers Public Methods
/////////////////////////////////////////////////////////////////////

DemoMusicCallbacks::DemoMusicCallbacks( Menu& in_ParentMenu ):Page( in_ParentMenu, "Music Callbacks Demo" ),
m_bIsPlaying( false ),
m_iPlayingID( 0 ),
m_uiBeatCount( 0 ),
m_uiBarCount( 0 ),
m_uiPlaylistItem( 0 )
{
	m_szHelp  = "This demo shows how to use music callbacks.\n\n"
				"Music Sync Callbacks: Beat and Bar notifications are generated from bpm and "
				"time signature info.\n\n"
				"Music Playlist Callback: Used in this example to force a random playlist to "
				"select its next item sequentially.  The playlist item may be stopped via the "
				"callback as well.\n\n";
}

bool DemoMusicCallbacks::Init()
{
	// Load the sound bank
	AkBankID bankID; // Not used
	if ( AK::SoundEngine::LoadBank( "MusicCallbacks.bnk", AK_DEFAULT_POOL_ID, bankID ) != AK_Success )
	{
		SetLoadFileErrorMessage( "MusicCallbacks.bnk" );
		return false;
	}

	// Register the "Markers" game object
	AK::SoundEngine::RegisterGameObj( GAME_OBJECT_MUSIC, "Music" );

	// Initialize the marker playback variables
	m_bIsPlaying = false;
	
	// Initialize the page
	return Page::Init();
}

void DemoMusicCallbacks::Release()
{
	// Make sure we do not get called back after page destruction
	AK::SoundEngine::CancelEventCallback( m_iPlayingID );

	// Stop the sound playing
	AK::SoundEngine::StopPlayingID( m_iPlayingID );

	// Unregister the "Music" game object
	AK::SoundEngine::UnregisterGameObj( GAME_OBJECT_MUSIC );

	// Unload the sound bank
	AK::SoundEngine::UnloadBank( "MusicCallbacks.bnk", NULL );

	// Releases the page
	Page::Release();
}

void DemoMusicCallbacks::Draw()
{
	Page::Draw();

	if ( m_bIsPlaying )
	{
		char strBuf[50];
		int iPosX = m_pParentMenu->GetWidth() / 4;
		int iPosY = m_pParentMenu->GetHeight() / 3;

		if ( m_ePlayType == PlayType_MusicSync )
		{
			AkTimeMs uPosition;

			// Get the current play position and store it in a string buffer
			AK::SoundEngine::GetSourcePlayPosition( m_iPlayingID, &uPosition );
			snprintf( strBuf, 50, "Bar: %d\nBeat: %d", (int)m_uiBarCount, (int)m_uiBeatCount );
		}
		else
		{
			snprintf( strBuf, 50, "Random playlist forced to sequential" );
		}

		// Draw the play position and subtitles
		DrawTextOnScreen( strBuf, iPosX, iPosY, DrawStyle_Text );
	}
}


/////////////////////////////////////////////////////////////////////
// DemoMusicCallbacks Private Methods
/////////////////////////////////////////////////////////////////////

void DemoMusicCallbacks::InitControls()
{
	ButtonControl* newBtn;

	newBtn = new ButtonControl( *this );
	newBtn->SetLabel( "Music Sync Callback Demo" );
	newBtn->SetDelegate( (PageMFP)&DemoMusicCallbacks::MusicSyncCallbackButton_Pressed );
	m_Controls.push_back( newBtn );

	newBtn = new ButtonControl( *this );
	newBtn->SetLabel( "Music Playlist Callback Demo" );
	newBtn->SetDelegate( (PageMFP)&DemoMusicCallbacks::MusicPlaylistCallbackButton_Pressed );
	m_Controls.push_back( newBtn );
}

void DemoMusicCallbacks::MusicSyncCallbackButton_Pressed( void* in_pSender, ControlEvent* )
{
	ButtonControl* pSyncBtn = static_cast<ButtonControl*>( m_Controls[0] );
	ButtonControl* pPlaylistBtn = static_cast<ButtonControl*>( m_Controls[1] );
	AKASSERT( (ButtonControl*)in_pSender == pSyncBtn );

	if ( !m_bIsPlaying )
	{
		m_bIsPlaying = true;
		m_ePlayType = PlayType_MusicSync;

		pSyncBtn->SetLabel( "Stop Play" );
		pPlaylistBtn->SetLabel( "" );

		// Post the PLAYMUSICDEMO1 event and ask for marker event notifications
		m_iPlayingID = AK::SoundEngine::PostEvent( 
			AK::EVENTS::PLAYMUSICDEMO1,
			GAME_OBJECT_MUSIC,
			AK_MusicSyncBeat | AK_MusicSyncBar | AK_MusicSyncEntry | AK_MusicSyncExit | AK_EndOfEvent,
			&DemoMusicCallbacks::MusicCallback,
			this
			);
	}
	else
	{
		AK::SoundEngine::StopPlayingID( m_iPlayingID );
	}
}

void DemoMusicCallbacks::MusicPlaylistCallbackButton_Pressed( void* in_pSender, ControlEvent* )
{
	ButtonControl* pSyncBtn = static_cast<ButtonControl*>( m_Controls[0] );
	ButtonControl* pPlaylistBtn = static_cast<ButtonControl*>( m_Controls[1] );
	AKASSERT( (ButtonControl*)in_pSender == pPlaylistBtn );

	if ( !m_bIsPlaying )
	{
		m_bIsPlaying = true;
		m_bStopPlaylist = false;
		m_ePlayType = PlayType_Playlist;

		pSyncBtn->SetLabel( "Stop Play" );
		pPlaylistBtn->SetLabel( "End Playlist" );

		// Post the PLAYMUSICDEMO2 event and ask for playlist selection callback
		m_iPlayingID = AK::SoundEngine::PostEvent( 
			AK::EVENTS::PLAYMUSICDEMO2,
			GAME_OBJECT_MUSIC,
			AK_MusicPlaylistSelect | AK_EndOfEvent,
			&DemoMusicCallbacks::MusicCallback,
			this
			);
	}
	else
	{
		if ( m_ePlayType == PlayType_Playlist )
		{
			m_bStopPlaylist = true;
		}
	}
}

void DemoMusicCallbacks::MusicCallback( AkCallbackType in_eType, AkCallbackInfo* in_pCallbackInfo )
{
	DemoMusicCallbacks* pPage = (DemoMusicCallbacks*)in_pCallbackInfo->pCookie;
	
	if ( in_eType == AK_MusicSyncBar )
	{
		pPage->m_uiBeatCount = 0;
		pPage->m_uiBarCount++;
	}
	else if ( in_eType == AK_MusicSyncBeat )
	{
		pPage->m_uiBeatCount++;
	}
	/*
	else if ( in_eType == AK_MusicSyncEntry )
	{

	}
	else if ( in_eType == AK_MusicSyncExit )
	{

	}*/
	else if ( in_eType == AK_MusicPlaylistSelect )
	{
		AkMusicPlaylistCallbackInfo* pPlaylistInfo = static_cast<AkMusicPlaylistCallbackInfo*>( in_pCallbackInfo );
		pPlaylistInfo->uPlaylistItemDone = pPage->m_bStopPlaylist;
		pPlaylistInfo->uPlaylistSelection = pPage->m_uiPlaylistItem++;
		if ( pPage->m_uiPlaylistItem == pPlaylistInfo->uNumPlaylistItems )
			pPage->m_uiPlaylistItem = 0;
	}
	else if ( in_eType == AK_EndOfEvent )
	{
		pPage->m_Controls[0]->SetLabel( "Music Sync Callback Demo" );
		pPage->m_Controls[1]->SetLabel( "Music Playlist Callback Demo" );

		pPage->m_bIsPlaying = false;
		pPage->m_uiBeatCount = 0;
		pPage->m_uiBarCount = 0;
		pPage->m_uiPlaylistItem = 0;
	}
}