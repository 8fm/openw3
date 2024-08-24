// DemoMarkers.h
// Copyright (C) 2010 Audiokinetic Inc 
/// \file
/// Contains the declaration for the DemoMusicCallbacks class.

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkCallback.h>
#include "Page.h"

/// Class representing the Music Callbacks Demo page. This page demonstrates how to use the music
/// callabacks in the interactive music system.
class DemoMusicCallbacks : public Page
{
public:

	/// DemoMarkers class constructor
	DemoMusicCallbacks( Menu& in_ParentMenu );

	/// Initializes the demo.
	/// \return True if successful and False otherwise.
	virtual bool Init();

	/// Releases resources used by the demo.
	virtual void Release();

	/// Override of the Page::Draw() method.
	virtual void Draw();

private:

	/// Initializes the controls on the page.
	virtual void InitControls();

	/// Delegate functions for buttons.
	void MusicSyncCallbackButton_Pressed( void* in_pSender, ControlEvent* in_pEvent );
	void MusicPlaylistCallbackButton_Pressed( void* in_pSender, ControlEvent* in_pEvent );

	/// Callback method for the events raised by Wwise while playing the markers demo audio.
	static void MusicCallback( 
		AkCallbackType in_eType,			///< - The type of the callback
		AkCallbackInfo* in_pCallbackInfo	///< - Structure containing info about the callback
		);

	/// Game Object ID for the "Narrator".
	static const AkGameObjectID GAME_OBJECT_MUSIC = 100;

	enum PlayType
	{
		PlayType_MusicSync = 0,
		PlayType_Playlist
	};

	/// Whether the demo is playing or not.
	bool m_bIsPlaying;
	bool m_bStopPlaylist;
	PlayType m_ePlayType;

	/// Holds the playing ID of the launched PLAY_MARKERS event.
	AkPlayingID m_iPlayingID;

	AkUInt32 m_uiBeatCount;
	AkUInt32 m_uiBarCount;

	AkUInt32 m_uiPlaylistItem;
};
