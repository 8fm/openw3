/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../core/hashmap.h"
#include "renderObject.h"
#include "inputKeys.h"

class CFlashMovie;
class IViewport;
class IRenderVideo;

//////////////////////////////////////////////////////////////////////////
// SLoadingScreenFenceInitParams
//////////////////////////////////////////////////////////////////////////
struct SLoadingScreenFenceInitParams
{
	CFlashMovie*	m_flashMovie;
	IViewport*		m_viewport;
	String			m_initString;
	String			m_caption;

	SLoadingScreenFenceInitParams( CFlashMovie* flashMovie, IViewport* viewport, const String& initString, const String& caption )
		: m_flashMovie( flashMovie )
		, m_viewport( viewport )
		, m_initString( initString )
		, m_caption( caption )
	{}
};

//////////////////////////////////////////////////////////////////////////
// ILoadingScreenFence
//////////////////////////////////////////////////////////////////////////
class ILoadingScreenFence : public IRenderObject
{
public:
	virtual Bool IsActive() const=0;
};

//////////////////////////////////////////////////////////////////////////
// SLoadingScreenParam
//////////////////////////////////////////////////////////////////////////
struct SLoadingScreenParam
{
	CName	m_contextName;			//!< Loading screen context 
	String	m_initString;			//!< Init string to pass to the local loading screen (e.g., fast travel - not world loading screen)
	String	m_videoToPlay;			//!< Video to play during the local loading screen (e.g., fast travel - not world loading screen)
	Bool	m_supressGameAudio;		//!< Supress the game audio. E.g., exploration music. Might want to set to false if the loading screen won't have its own music.

	SLoadingScreenParam()
		: m_supressGameAudio( true )
	{}

	explicit SLoadingScreenParam( CName contextName, const String& initString = String::EMPTY, const String& videoToPlay = String::EMPTY, Bool supressGameAudio = true )
		: m_contextName( contextName )
		, m_initString( initString )
		, m_videoToPlay( videoToPlay )
		, m_supressGameAudio( supressGameAudio )
	{}

	explicit SLoadingScreenParam( CName contextName, Bool supressGameAudio )
		: m_contextName( contextName )
		, m_supressGameAudio( supressGameAudio )
	{}

	void Clear()
	{
		m_contextName = CName::NONE;
		m_videoToPlay.Clear();
		m_initString.Clear();
		m_supressGameAudio = true;
	}

	static const SLoadingScreenParam DEFAULT;
	static const SLoadingScreenParam BLACKSCREEN;
};

//////////////////////////////////////////////////////////////////////////
// CLoadingScreen
//////////////////////////////////////////////////////////////////////////
class CLoadingScreen
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	CLoadingScreen();
	~CLoadingScreen();
	Bool Init();

public:
	Bool							Show( const SLoadingScreenParam& param );
	Bool							StartFadeOut();
	Bool							IsShown() const;
	Bool							IsPlayingVideo() const;
	Bool							ToggleVideoSkip( Bool enabled );
	Bool							IsVideoSkipKey( EInputKey key ) const;
	Bool							SetPCInput( Bool enabled );
	Bool							SetExpansionsAvailable( Bool ep1, Bool ep2 );

public:
	static const String&			GetIgnoredVideoName();

private:
	Bool							ShowWithSwf( const String& swfPath, const String& name, const SLoadingScreenParam& param );

private:
	Bool							InitLoadingScreenMap();

private:
	Bool							PlayVideo( const String& videoFile );

private:
	ILoadingScreenFence*			CreateLoadingScreenFence( const String& initString, const String& swfPath, const String& name );
	void							FadeInBlackscreenIfNeeded();

private:
	Bool							ParseLoadingScreenSettings();

private:
	THashMap< CName, String >		m_loadingScreenMap;
	mutable ILoadingScreenFence*	m_fence;
	mutable	IRenderVideo*			m_renderVideo;

private:
	TBitSet< IK_Count >				m_videoSkipKeys;

private:
	CGameSaveLock					m_saveLock;

private:
	static const String				IGNORED_VIDEO_NAME;

private:
	Bool							AllowLoadingScreenStart() const;
	void							PrepareForLoadingScreenStart();
};