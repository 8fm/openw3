/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#ifdef USE_SCALEFORM

#include "renderer.h"
#include "renderCommands.h"
#include "scaleformSystem.h"
#include "scaleformMemory.h"
#include "scaleformPlayer.h"
#include "scaleformLibs.h"
#include "scaleformTranslator.h"
#include "localizationManager.h"
#include "scaleformLoader.h"
#include "../core/depot.h"

//////////////////////////////////////////////////////////////////////////
// SCreateScaleformSystem
//////////////////////////////////////////////////////////////////////////
CScaleformSystem* CScaleformSystem::s_instance = nullptr;
CScaleformSystem* CScaleformSystem::StaticInstance()
{
	if ( !CScaleformSystem::s_instance )
	{
		// Moved the fatal assert inside here now, since we need to get it from other threads to shutdown rendering too now
		RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only!");
		GFx::System::Init( CreateScaleformSysAlloc() );

		CScaleformSystem::s_instance = new CScaleformSystem;
	}

	return s_instance;
}

CScaleformSystem::CScaleformSystem()
	: m_systemLoader( nullptr )
	, m_isInitialized( false )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only!");
}

CScaleformSystem::~CScaleformSystem()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only!");
}

Bool CScaleformSystem::Init( IViewport* viewport )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only!");

	if ( m_isInitialized )
	{
		return true;
	}

	// In case the game world is started to soon - before the renderer has rendered a normal frame, lazily initializing Scaleform
	// since the backbuffer needs to be set when it's initialized since it's somewhat deep within Scaleform to assume it is.
	{
		CRenderFrameInfo frameInfo( viewport );
		frameInfo.SetShowFlag( SHOW_GUI, true ); // Make sure for this frame
		CRenderFrame* emptyFrame = GRender->CreateFrame( nullptr, frameInfo );
		RED_ASSERT( emptyFrame );
		if ( emptyFrame )
		{
			( new CRenderCommand_RenderScene( emptyFrame, nullptr) )->Commit();
			GRender->Flush();
			emptyFrame->Release();
		}
	}

	ASSERT( GRender && GRender->GetRenderScaleform() );
	if ( ! GRender || ! GRender->GetRenderScaleform() )
	{
		return false;
	}

	// Creating a shared loader because just a shared resourcelib isn't enough to share fontlib (even if the fonts are shared)
	// and would need to duplicate updates. Easier to just add the extra states to the moviedef as needed
	m_systemLoader = *SF_NEW CScaleformLoader;

	class CDeferedInit_ScaleformFont : public IDeferredInitDelegate
	{
	public:
		CScaleformSystem* mSys;
		CDeferedInit_ScaleformFont(CScaleformSystem* scaleformSystem) : mSys(scaleformSystem) {}
		void OnBaseEngineInit() override
		{
			if ( ! mSys->m_fontConfig.LoadFromFile() )
			{
				WARN_ENGINE(TXT("Failed to load Fonts.ini"));
			}

			//String defaultFontLib = TXT("gfxfontlib.swf"); // What Scaleform checks for exactly by default if no default font name
			//const SFlashFontLib& fontLib = m_fontConfig.GetDefaultFontlib();
			String defaultFontLib = mSys->m_fontConfig.GetDefaultFontlib();
			CFilePath filePath( defaultFontLib );
			String fileName = filePath.GetFileName(); // without extension because we need to tell it .swf, not .redswf or .gfx

			// Possible to override with development font name instead of using gfxfontlib.swf.
			// E.g., swf/fonts_en.redswf -> fonts_en.swf, as referenced in the SWF files
			const String defaultFontLibName = fileName + TXT(".swf");
			LOG_ENGINE(TXT("Scaleform defaultFontLib set to '%ls'"), defaultFontLibName.AsChar() );

			mSys->m_systemLoader->SetDefaultFontLibName( defaultFontLibName );

			// Make sure language set to something initially
			mSys->OnLanguageChange();
		}
	};

	// Do not add video here!
	const Uint32 baseLoaderInitFlags = 
		CScaleformLoader::SYS_Images
		| CScaleformLoader::SYS_AS2
		| CScaleformLoader::SYS_AS3
		;

	if ( ! m_systemLoader->Init( baseLoaderInitFlags ) )
	{
		ERR_ENGINE(TXT("CScaleformSystem: failed to initialize loader!"));
		return false;
	}

	if (GDeferredInit)
	{
		GDeferredInit->AddDelegate( new CDeferedInit_ScaleformFont(this) );
	}
	else
	{
		CDeferedInit_ScaleformFont init(this);
		init.OnBaseEngineInit();
	}

	for ( IScaleformPlayer* player : m_players )
	{
		player->OnScaleformInit();
	}

	m_isInitialized = true;

	return true;
}

void CScaleformSystem::Shutdown()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only!");

	// Clear and force rendering shutdown in the movie dtor instead of waiting for render thread to process a frame
	GRender->Flush();

	for ( IScaleformPlayer* player : m_players )
	{
		player->OnScaleformShutdown();
	}

	( new CRenderCommand_SetVideoFlash( nullptr ) )->Commit();
	GRender->Flush(); // Making sure video shutdown before clearing the loader

	if ( m_systemLoader )
	{
		m_systemLoader->Shutdown();
	}

	m_systemLoader.Clear();

	DestroyScaleformSysAlloc();
}

void CScaleformSystem::Destroy()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only!");

	if ( CScaleformSystem::s_instance )
	{
		delete CScaleformSystem::s_instance;
		CScaleformSystem::s_instance = nullptr;
		GFx::System::Destroy();
	}
}

Bool CScaleformSystem::RegisterScaleformPlayer( IScaleformPlayer* player )
{
	RED_FATAL_ASSERT( player, "No player" );
	if ( m_players.PushBackUnique( player ) )
	{
		if ( m_isInitialized )
		{
			player->OnScaleformInit();
		}
		return true;
	}

	return false;
}

Bool CScaleformSystem::UnregisterScaleformPlayer( IScaleformPlayer* player )
{
	RED_FATAL_ASSERT( player, "No player" );
	return m_players.Remove( player );
}

void CScaleformSystem::OnLanguageChange()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Not on main thread" );

	const String& textLocale = SLocalizationManager::GetInstance().GetTextLocale();
	const SFlashFontLib& fontLib = m_fontConfig.GetFontLib( textLocale );
	if ( &fontLib == &CFlashFontConfig::INVALID_FONTLIB )
	{
		ERR_ENGINE(TXT("No fontlib for language '%ls'"), textLocale.AsChar() );
		return;
	}

	m_systemLoader->SetFontLib( fontLib );
}

#endif // USE_SCALEFORM
