/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#ifdef USE_SCALEFORM

#include "guiGlobals.h"

#include "renderer.h"
#include "renderScaleform.h"

#include "swfResource.h"
#include "scaleformExternalInterface.h"
#include "scaleformFile.h"
#include "scaleformTaskManager.h"
#include "scaleformTranslator.h"
#include "scaleformUserEvent.h"
#include "scaleformLog.h"
#include "renderScaleformCommands.h"

#include "scaleformLoader.h"

#if defined( RED_PLATFORM_ORBIS ) && !defined( RED_FINAL_BUILD )
# define TRACE_FUNC __attribute__((optnone))
#else
# define TRACE_FUNC
#endif

// The docs encourage you to make a local copy of AS3_Global.h and remove the classes you don't want to support,
// but then you need to be careful that they aren't ever actually used in ActionScript.
#if WITH_SCALEFORM_AS3
# include <GFx/AS3/AS3_Global.h>
#endif

#include <Render/ImageFiles/DDS_ImageFile.h>
#include <Render/ImageFiles/PNG_ImageFile.h>

#if WITH_SCALEFORM_VIDEO
# if WITH_SCALEFORM_WWISE
#  include <GFx_Sound_WWise.h>
#  include <Video/Video_VideoSoundSystemWWise.h>
// SHITHACK to avoid recompiling libgfxvideo
#  include "..\..\..\external\gfx4\Src\Video\Video_VideoSoundSystemWwise.cpp"
# endif
# if WITH_SCALEFORM_XA2
//#  include <Video/Video_VideoSoundSystemXA2.h>
#include "videoSoundSystemXA2.h"
#include "videoSoundSystemDX8.h"
# endif
# if defined( RED_PLATFORM_WINPC )
#  include <Video/Video_VideoPC.h>
# elif defined( RED_PLATFORM_DURANGO )
#  include <Video/Video_VideoXboxOne.h>
# elif defined( RED_PLATFORM_ORBIS )
#  include <Video/Video_VideoPS4.h>
//#  include <Video/Video_VideoSoundSystemPS4.h>
# include "videoSoundSystemPS4.h"
# else
#  error "Undefined platform for video!"
# endif
#endif // WITH_SCALEFORM_VIDEO


namespace Config
{
	static const SF::Render::GlyphCacheParams DefaultGlyphCacheParams;

	TConfigVar< Int32 > cv_gc_ntx1	("Scaleform/GlyphCache/1", "NumTextures", DefaultGlyphCacheParams.NumTextures );
	TConfigVar< Int32 > cv_gc_nth1	("Scaleform/GlyphCache/1", "TextureHeight", DefaultGlyphCacheParams.TextureHeight );
	TConfigVar< Int32 > cv_gc_ntw1	("Scaleform/GlyphCache/1", "TextureWidth", DefaultGlyphCacheParams.TextureWidth );
	TConfigVar< Int32 > cv_gc_uh1	("Scaleform/GlyphCache/1", "TexUpdHeight", DefaultGlyphCacheParams.TexUpdHeight );
	TConfigVar< Int32 > cv_gc_uw1	("Scaleform/GlyphCache/1", "TexUpdWidth", DefaultGlyphCacheParams.TexUpdWidth );
	TConfigVar< Int32 > cv_gc_msh1	("Scaleform/GlyphCache/1", "MaxSlotHeight", DefaultGlyphCacheParams.MaxSlotHeight );
	TConfigVar< Int32 > cv_gc_mv1	("Scaleform/GlyphCache/1", "MaxVectorCacheSize", DefaultGlyphCacheParams.MaxVectorCacheSize );
	TConfigVar< Float > cv_gc_mrs1	("Scaleform/GlyphCache/1", "MaxRasterScale", DefaultGlyphCacheParams.MaxRasterScale );

	TConfigVar< Int32 > cv_gc_ntx2	("Scaleform/GlyphCache/2", "NumTextures", DefaultGlyphCacheParams.NumTextures );
	TConfigVar< Int32 >	cv_gc_nth2	("Scaleform/GlyphCache/2", "TextureHeight", DefaultGlyphCacheParams.TextureHeight );
	TConfigVar< Int32 >	cv_gc_ntw2	("Scaleform/GlyphCache/2", "TextureWidth", DefaultGlyphCacheParams.TextureWidth );
	TConfigVar< Int32 >	cv_gc_uh2	("Scaleform/GlyphCache/2", "TexUpdHeight", DefaultGlyphCacheParams.TexUpdHeight );
	TConfigVar< Int32 >	cv_gc_uw2	("Scaleform/GlyphCache/2", "TexUpdWidth", DefaultGlyphCacheParams.TexUpdWidth );
	TConfigVar< Int32 > cv_gc_msh2	("Scaleform/GlyphCache/2", "MaxSlotHeight", DefaultGlyphCacheParams.MaxSlotHeight );
	TConfigVar< Int32 > cv_gc_mv2	("Scaleform/GlyphCache/2", "MaxVectorCacheSize", DefaultGlyphCacheParams.MaxVectorCacheSize );
	TConfigVar< Float > cv_gc_mrs2	("Scaleform/GlyphCache/2", "MaxRasterScale", DefaultGlyphCacheParams.MaxRasterScale );

	TConfigVar< Int32 >	cv_gc_ntx3	("Scaleform/GlyphCache/3", "NumTextures", DefaultGlyphCacheParams.NumTextures );
	TConfigVar< Int32 >	cv_gc_nth3	("Scaleform/GlyphCache/3", "TextureHeight", DefaultGlyphCacheParams.TextureHeight );
	TConfigVar< Int32 >	cv_gc_ntw3	("Scaleform/GlyphCache/3", "TextureWidth", DefaultGlyphCacheParams.TextureWidth );
	TConfigVar< Int32 >	cv_gc_uh3	("Scaleform/GlyphCache/3", "TexUpdHeight", DefaultGlyphCacheParams.TexUpdHeight );
	TConfigVar< Int32 >	cv_gc_uw3	("Scaleform/GlyphCache/3", "TexUpdWidth", DefaultGlyphCacheParams.TexUpdWidth );
	TConfigVar< Int32 > cv_gc_msh3	("Scaleform/GlyphCache/3", "MaxSlotHeight", DefaultGlyphCacheParams.MaxSlotHeight );
	TConfigVar< Int32 > cv_gc_mv3	("Scaleform/GlyphCache/3", "MaxVectorCacheSize", DefaultGlyphCacheParams.MaxVectorCacheSize );
	TConfigVar< Float > cv_gc_mrs3	("Scaleform/GlyphCache/3", "MaxRasterScale", DefaultGlyphCacheParams.MaxRasterScale );

	TConfigVar< Int32 >	cv_gc_ntx4	("Scaleform/GlyphCache/4", "NumTextures", DefaultGlyphCacheParams.NumTextures );
	TConfigVar< Int32 >	cv_gc_nth4	("Scaleform/GlyphCache/4", "TextureHeight", DefaultGlyphCacheParams.TextureHeight );
	TConfigVar< Int32 >	cv_gc_ntw4	("Scaleform/GlyphCache/4", "TextureWidth", DefaultGlyphCacheParams.TextureWidth );
	TConfigVar< Int32 >	cv_gc_uh4	("Scaleform/GlyphCache/4", "TexUpdHeight", DefaultGlyphCacheParams.TexUpdHeight );
	TConfigVar< Int32 >	cv_gc_uw4	("Scaleform/GlyphCache/4", "TexUpdWidth", DefaultGlyphCacheParams.TexUpdWidth );
	TConfigVar< Int32 > cv_gc_msh4	("Scaleform/GlyphCache/4", "MaxSlotHeight", DefaultGlyphCacheParams.MaxSlotHeight );
	TConfigVar< Int32 > cv_gc_mv4	("Scaleform/GlyphCache/4", "MaxVectorCacheSize", DefaultGlyphCacheParams.MaxVectorCacheSize );
	TConfigVar< Float > cv_gc_mrs4	("Scaleform/GlyphCache/4", "MaxRasterScale", DefaultGlyphCacheParams.MaxRasterScale );

	TConfigVar< Int32 >	cv_gc_ntx5	("Scaleform/GlyphCache/5", "NumTextures", DefaultGlyphCacheParams.NumTextures );
	TConfigVar< Int32 >	cv_gc_nth5	("Scaleform/GlyphCache/5", "TextureHeight", DefaultGlyphCacheParams.TextureHeight );
	TConfigVar< Int32 >	cv_gc_ntw5	("Scaleform/GlyphCache/5", "TextureWidth", DefaultGlyphCacheParams.TextureWidth );
	TConfigVar< Int32 >	cv_gc_uh5	("Scaleform/GlyphCache/5", "TexUpdHeight", DefaultGlyphCacheParams.TexUpdHeight );
	TConfigVar< Int32 >	cv_gc_uw5	("Scaleform/GlyphCache/5", "TexUpdWidth", DefaultGlyphCacheParams.TexUpdWidth );
	TConfigVar< Int32 > cv_gc_msh5	("Scaleform/GlyphCache/5", "MaxSlotHeight", DefaultGlyphCacheParams.MaxSlotHeight );
	TConfigVar< Int32 > cv_gc_mv5	("Scaleform/GlyphCache/5", "MaxVectorCacheSize", DefaultGlyphCacheParams.MaxVectorCacheSize );
	TConfigVar< Float > cv_gc_mrs5	("Scaleform/GlyphCache/5", "MaxRasterScale", DefaultGlyphCacheParams.MaxRasterScale );

	struct SGlyphParam
	{
		const TConfigVar<Int32>* m_pNumTextures;
		const TConfigVar<Int32>* m_pTextureHeight;
		const TConfigVar<Int32>* m_pTextureWidth;
		const TConfigVar<Int32>* m_pTexUpdHeight;
		const TConfigVar<Int32>* m_pTexUpdWidth;
		const TConfigVar<Int32>* m_pMaxSlotHeight;
		const TConfigVar<Int32>* m_pMaxVectorCacheSize;
		const TConfigVar<Float>* m_pMaxRasterScale;
	};

	static const SGlyphParam GlyphCaches[] = {
		{ &cv_gc_ntx1, &cv_gc_nth1, &cv_gc_ntw1, &cv_gc_uh1, &cv_gc_uw1, &cv_gc_msh1, &cv_gc_mv1, &cv_gc_mrs1 },
		{ &cv_gc_ntx2, &cv_gc_nth2, &cv_gc_ntw2, &cv_gc_uh2, &cv_gc_uw2, &cv_gc_msh2, &cv_gc_mv2, &cv_gc_mrs2 },
		{ &cv_gc_ntx3, &cv_gc_nth3, &cv_gc_ntw3, &cv_gc_uh3, &cv_gc_uw3, &cv_gc_msh3, &cv_gc_mv3, &cv_gc_mrs3 },
		{ &cv_gc_ntx4, &cv_gc_nth4, &cv_gc_ntw4, &cv_gc_uh4, &cv_gc_uw4, &cv_gc_msh4, &cv_gc_mv4, &cv_gc_mrs4 },
		{ &cv_gc_ntx5, &cv_gc_nth5, &cv_gc_ntw5, &cv_gc_uh5, &cv_gc_uw5, &cv_gc_msh5, &cv_gc_mv5, &cv_gc_mrs5 },
	};
} // namespace Config

namespace ScaleformFontHelpers
{
#ifdef RED_LOGGING_ENABLED
	String GetGlyphCacheStringForLog( const SF::Render::GlyphCacheParams& params )
	{
 		return String::Printf(TXT("[GlyphCacheParams\n")
 			TXT("\tTextureWidth=%u, TextureHeight=%u, NumTextures=%u, MaxSlotHeight=%u, SlotPadding=%u\n")
 			TXT("\tTexUpdWidth=%u, TexUpdHeight=%u, MaxRasterScale=%.2f, MaxVectorCacheSize=%u\n")
 			TXT("\tFauxItalicAngle=%.2f, FauxBoldRatio=%.2f, OutlineRatio=%.2f, ShadowQuality=%.2f\n")
 			TXT("\tUseAutoFit=%d, UseVectorOnFullCache=%d]"),
 			params.TextureWidth, params.TextureHeight, params.NumTextures, params.MaxSlotHeight, params.SlotPadding,
 			params.TexUpdWidth, params.TexUpdHeight, params.MaxRasterScale, params.MaxVectorCacheSize,
 			params.FauxItalicAngle, params.FauxBoldRatio, params.OutlineRatio, params.ShadowQuality,
 			params.UseAutoFit, params.UseVectorOnFullCache);			
	}
#endif

	void GetGlyphCacheParams( Int32 cache, SF::Render::GlyphCacheParams& outGlyphCacheParams )
	{
		const Int32 index = cache - 1;

		if ( index < 0 || index >= ARRAY_COUNT(Config::GlyphCaches) )
		{
			outGlyphCacheParams = Config::DefaultGlyphCacheParams;
		}
		else
		{
			const Config::SGlyphParam& param = Config::GlyphCaches[index];
			outGlyphCacheParams.NumTextures = param.m_pNumTextures->Get();
			outGlyphCacheParams.TextureHeight = param.m_pTextureHeight->Get();
			outGlyphCacheParams.TextureWidth = param.m_pTextureWidth->Get();
			outGlyphCacheParams.TexUpdHeight = param.m_pTexUpdHeight->Get();
			outGlyphCacheParams.TexUpdWidth = param.m_pTexUpdWidth->Get();
			outGlyphCacheParams.MaxSlotHeight = param.m_pMaxSlotHeight->Get();
			outGlyphCacheParams.MaxVectorCacheSize = param.m_pMaxVectorCacheSize->Get();
			outGlyphCacheParams.MaxRasterScale = param.m_pMaxRasterScale->Get();
		}
	}
}

#ifdef RED_PLATFORM_WINPC
class CScaleformSideLoaderThread : public Red::Threads::CThread, private Red::System::NonCopyable
{
public:
	CScaleformSideLoaderThread( GFx::Loader* loader )
#ifdef RED_PLATFORM_WINPC
		// HACK - the stack size needs to be increased because the logging crashes on this one thread and this makes it impossible for people to debug certain issues
		: CThread( "ScaleformSideLoader", 1024*1024 )
#else
		: CThread( "ScaleformSideLoader", 64*1024 )
#endif
		, m_pLoader( loader )
		, m_exitFlag( false )
		, m_workSem( 0, 1 )
	{
		RED_FATAL_ASSERT( loader, "No loader" );
	}

	virtual ~CScaleformSideLoaderThread()
	{
		RED_FATAL_ASSERT( !m_outMovieDef, "Movie left in sideloader!" );
	}

public:
	struct SWorkItem : Red::System::NonCopyable
	{
		const StringAnsi*	m_pgfxPath;
		Uint32				m_loadFlags;

		SWorkItem()
			: m_pgfxPath( nullptr )
			, m_loadFlags( 0 )
		{}

		void Clear()
		{
			m_pgfxPath = nullptr;
			m_loadFlags = 0;
		}
	};

public:
	virtual void ThreadFunc() override
	{
		for(;;)
		{
			m_workSem.Acquire();
			if ( m_exitFlag )
				break;

			m_outMovieDef = CreateMovieDef( *m_workItem.m_pgfxPath, m_workItem.m_loadFlags );
			m_loadedFlag = true;
		}
	}

public:
	void BeginCreateMovieDef( const StringAnsi* pgfxPath, Uint32 loadFlags )
	{
		RED_FATAL_ASSERT( pgfxPath, "pgfxPath nullptr" );
		m_loadedFlag = false;
		m_workItem.m_pgfxPath = pgfxPath;
		m_workItem.m_loadFlags = loadFlags;
		m_workSem.Release();
	}

	Bool TryFinishCreateMovieDef( GFx::MovieDef** outMovieDef )
	{
		RED_FATAL_ASSERT( outMovieDef, "outMovie nullptr" );

		if ( !m_loadedFlag )
		{
			return false;
		}

		*outMovieDef = m_outMovieDef;
		m_outMovieDef = nullptr;
		m_workItem.Clear();
		m_loadedFlag = false;
		return true;
	}

	void RequestExit()
	{
		m_exitFlag = true;
		m_workSem.Release();
	}

private:
	GFx::MovieDef* CreateMovieDef( const StringAnsi& gfxPath, Uint32 loadFlags )
	{
		GFx::MovieDef* movieDef = m_pLoader->CreateMovie( gfxPath.AsChar(), loadFlags );
		return movieDef;
	}

private:
	GFx::Loader*				m_pLoader;
	volatile Bool				m_exitFlag;
	volatile Bool				m_loadedFlag;
	Red::Threads::CSemaphore	m_workSem;
	SWorkItem					m_workItem;
	GFx::MovieDef*				m_outMovieDef;
};

#endif // RED_PLATFORM_WINPC

#if WITH_SCALEFORM_VIDEO
class VideoReadCallback : public SF::GFx::Video::VideoBase::ReadCallback
{
	// Notify then a video read operation is required. Upon receiving this notification
	// the application should stop all its disk IO operations (and wait until they are really
	// finished). After returning form this method the video system will start file read 
	// operation immediately.
	virtual void OnReadRequested()
	{
		SJobManager::GetInstance().Lock();
		
		// Flushing doesn't seem to help or hinder
		//SJobManager::GetInstance().FlushProcessingJobs();
#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileSignalVideoReadRequested();
#endif
	}

	// Notify then the video read operation has finished. Upon receiving this notification 
	// the application can resume its disk IO operations until it receives OnReadRequested 
	// notification.
	virtual void OnReadCompleted()
	{
#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileSignalVideoReadRequested();
#endif
		SJobManager::GetInstance().Unlock();
	}
};
#endif // WITH_SCALEFORM_VIDEO

//////////////////////////////////////////////////////////////////////////
// CScaleformLoader
//////////////////////////////////////////////////////////////////////////
static const String SWF_EXTENSION( TXT(".swf") );
static const String GFX_EXTENSION( TXT(".gfx") );
static const String REDSWF_EXTENSION( TXT(".redswf") );
static const StringAnsi REDSWF_EXTENSION_ANSI( ".redswf" );
static const StringAnsi GFX_EXTENSION_ANSI( ".gfx" );

CScaleformLoader::CScaleformLoader()
	: m_currentGlyphCache(-1)
	, m_systemFlags( 0 )
	, m_isInitialized( false )
#ifdef RED_PLATFORM_WINPC
	, m_sideLoaderThread( nullptr )
#endif
{
}

Bool CScaleformLoader::Init( Uint32 systemFlags )
{
	ASSERT( ::SIsMainThread() );

	if ( m_isInitialized )
	{
		return true;
	}

	m_systemFlags = systemFlags;

	if ( ! GRender )
	{
		return false;
	}

	SF::Ptr<SF::Render::TextureManager> textureManager = GRender->GetRenderScaleform() ? GRender->GetRenderScaleform()->GetTextureManager() : nullptr;
	RED_WARNING( textureManager, "texture manager should not be null" );
	if ( ! textureManager )
	{
		return false;
	}

	InitActionScript();

	// Set an image creator regardless of SYS_Images. Needed for video etc.
	SF::Ptr<GFx::ImageCreator> pImageCreator = *SF_NEW CScaleformImageCreator( textureManager );
	m_loader.SetImageCreator( pImageCreator );

	if ( (m_systemFlags & SYS_Images) != 0 )
	{
		if ( ! InitImages() )
		{
			GUI_ERROR( TXT("Failed to initialize Scaleform images manager!") );
			return false;
		}
	}

	//m_loader.SetTaskManager( SF::Ptr< SF::GFx::TaskManager >( *SF_NEW CScaleformTaskManager ) );
	m_loader.SetLog( SF::Ptr<GFx::Log>( *SF_NEW CScaleformLog ) );
	m_loader.SetURLBuilder( SF::Ptr<GFx::URLBuilder>( *SF_NEW CScaleformUrlBuilder ) );
	m_loader.SetFileOpener( SF::Ptr<GFx::FileOpener>( *SF_NEW CScaleformFileOpener ) );
	const Uint32 parseFlags = GFx::ParseControl::VerboseParseNone;
	m_loader.SetParseControl( SF::Ptr<GFx::ParseControl>( *SF_NEW GFx::ParseControl( parseFlags ) ) );

#ifndef RED_FINAL_BUILD
	const Uint32 actionFlags = /*GFx::ActionControl::Action_Verbose | */ GFx::ActionControl::Action_LogAllFilenames |
							 GFx::ActionControl::Action_LongFilenames;
#else
	const Uint32 actionFlags = 0;
#endif
	m_loader.SetActionControl( SF::Ptr<GFx::ActionControl>( *SF_NEW GFx::ActionControl( actionFlags ) ) );

#if WITH_SCALEFORM_VIDEO
	if ( ! InitVideo() )
	{
		return false;
	}
#endif

#ifdef RED_PLATFORM_WINPC
	m_sideLoaderThread = new CScaleformSideLoaderThread( &m_loader );
	m_sideLoaderThread->InitThread();
#endif

	m_isInitialized = true;

	return true;
}

void CScaleformLoader::Shutdown()
{
	ASSERT( ::SIsMainThread() );
	
	m_loader.CancelLoading();
	//WaitForLoadFinish

	if ( m_loader.GetExternalInterface() )
	{
		static_cast< CScaleformExternalInterface* >( m_loader.GetExternalInterface().GetPtr() )->SetHandler( nullptr );
	}

	if ( m_loader.GetUserEventHandler() )
	{
		static_cast< CScaleformUserEvent* >( m_loader.GetUserEventHandler().GetPtr() )->SetHandler( nullptr );
	}

	SF::Ptr< GFx::TaskManager > taskManager = m_loader.GetTaskManager();
	if ( taskManager )
	{
		taskManager->RequestShutdown();
	}

#ifdef RED_PLATFORM_WINPC
	m_sideLoaderThread->RequestExit();
	m_sideLoaderThread->JoinThread();
	delete m_sideLoaderThread;
	m_sideLoaderThread = nullptr;
#endif

	// Flush out all pending render commands before destroying the loader
	if ( GRender )
	{
		GRender->Flush();
	}
}

void CScaleformLoader::SetDefaultFontLibName( const String& fontlibName )
{
	m_loader.SetDefaultFontLibName( FLASH_TXT_TO_UTF8(fontlibName.AsChar()) );
}

GFx::Movie* CScaleformLoader::CreateMovie( const TSoftHandle< CSwfResource >& swfResource, const SCreateMovieContext& context, Bool waitForLoadFinish /*= false*/ )
{
	const String& depotPath = swfResource.GetPath();
	return CreateMovie( depotPath, context, waitForLoadFinish );
}

GFx::Movie* CScaleformLoader::CreateMovie( const THandle< CSwfResource >& swfResource, const SCreateMovieContext& context, Bool waitForLoadFinish /*= false*/ )
{
	if ( ! swfResource )
	{
		return nullptr;
	}
	
	const String& depotPath = swfResource->GetDepotPath();
	return CreateMovie( depotPath, context );
}

GFx::Movie* CScaleformLoader::CreateMovie( const String& swfDepotPath, const SCreateMovieContext& context, Bool waitForLoadFinish )
{
	ASSERT( ::SIsMainThread() );

	StringAnsi gfxPath = FLASH_TXT_TO_UTF8( swfDepotPath.AsChar() );
	if ( ! gfxPath.Replace( REDSWF_EXTENSION_ANSI, GFX_EXTENSION_ANSI, true ) )
	{
		return nullptr; // Make sure didn't already get passed a ".gfx" which won't get picked up by the cooker
	}

#ifdef RED_PLATFORM_CONSOLE
	const Uint32 extraFlags = GFx::Loader::LoadDisableSWF;
#else
	const Uint32 extraFlags = 0;
#endif

	const Uint32 loadFlags = GFx::Loader::LoadAll | extraFlags | ( waitForLoadFinish ? GFx::Loader::LoadWaitCompletion : 0 );

	// FIXME: don't like this solution much - problem: postCreateTexture blocks on an event waiting for the renderer to process textures. 
	// This can cause hangs on PC from needing to process DXGI related windows messages
	// Note that a threaded task manager wouldn't be enough, since the first movie loaded would still be on the main thread and block the message pump
#ifdef RED_PLATFORM_WINPC
	GFx::MovieDef* pMovieDef = nullptr;	
	m_sideLoaderThread->BeginCreateMovieDef( &gfxPath, loadFlags );
	while ( !m_sideLoaderThread->TryFinishCreateMovieDef( &pMovieDef ) )
	{
		GRender->Flush();
	}
#else
	GFx::MovieDef* pMovieDef = m_loader.CreateMovie( gfxPath.AsChar(), loadFlags );
#endif
	if ( !pMovieDef )
	{
		GUI_ERROR( TXT("CreateMovie: Failed to load Flash SWF '%ls'"), swfDepotPath.AsChar() );
		return nullptr;
	}

	SF::Ptr< GFx::MovieDef > movieDef = *pMovieDef;

	if ( context.m_externalInterfaceHandler )
	{
		SF::Ptr< CScaleformExternalInterface > externalInterface = *SF_NEW CScaleformExternalInterface;
		externalInterface->SetHandler( context.m_externalInterfaceHandler );
		movieDef->SetExternalInterface( externalInterface );
	}

	if ( context.m_userEventHandler )
	{
		SF::Ptr< CScaleformUserEvent > userEvent = *SF_NEW CScaleformUserEvent;
		userEvent->SetHandler( context.m_userEventHandler );
		movieDef->SetUserEventHandler( userEvent );
	}

	// "initFirstFrame" must be false because we must first set the user data to be able to handle the Flash file making an external interface call
	//SF::Ptr<GFx::Movie> movie = *movieDef->CreateInstance( false, 0, 0, GRender->GetGuiRenderSystemScaleform()->GetCommandQueue() );
	GFx::Movie* movie = movieDef->CreateInstance( false, 0 /* For GFx 4.1: , GRender->GetGuiRenderCommandQueue() */ );
	ASSERT( movie );
	if ( ! movie )
	{
		GUI_ERROR( TXT("CreateMovie: Failed to create movie instance for Flash SWF '%ls'"), swfDepotPath.AsChar() );
		return nullptr;
	}

	movie->SetBackgroundAlpha( 0.f );
	// 	movie->SetBackgroundAlpha( 1.f );
	// 	movie->SetBackgroundColor( SF::Render::Color::Cyan );

	return movie;
}

Bool CScaleformLoader::SetFontLib( const SFlashFontLib& fontLib )
{
	// Check against the ID, since don't change fontlib if just reloading the same fonts!
	if ( m_currentFontConfigId != fontLib.m_id )
	{
		m_currentFontConfigId = fontLib.m_id;

		if ( ! LoadFonts( fontLib ) )
		{
			return false;
		}
	}

	// Update the translator to force retranslation of the language, even if the same font config
	return UpdateTranslator( fontLib );
}

Bool CScaleformLoader::LoadFonts( const SFlashFontLib& fontLib )
{
	
	// The FontLib never unpins the resource, but we should keep it around unless we want to potentially
	// reload the fonts all the time. So we do our own "manual" pinning by keeping the fontsMovieDef around.
	m_fontsMovieDef.Clear();

	SF::Ptr<GFx::FontLib> pFontLib = *new GFx::FontLib;
	SF::Ptr<GFx::FontMap> pFontMap = *new GFx::FontMap;
	m_loader.SetFontLib(pFontLib);
	m_loader.SetFontMap(pFontMap);

	String fontsPath = fontLib.m_fontLibPath;
	fontsPath.Replace( TXT(".redswf"), TXT(".gfx"), true );
	m_fontsMovieDef = *m_loader.CreateMovie( FLASH_TXT_TO_UTF8(fontsPath.AsChar()) );
	if ( ! m_fontsMovieDef )
	{
		GUI_ERROR( TXT("LoadFonts: Failed to load fonts for '%ls'"), fontsPath.AsChar() );

		m_loader.SetFontLib( nullptr );
		m_loader.SetFontMap( nullptr );
		m_loader.SetTranslator( nullptr );
		return false;
	}
	pFontLib->AddFontsFrom(m_fontsMovieDef, false );

	// TBD: Check for incompat flags
	for ( const SFlashFontMap& fontMap : fontLib.m_fontMaps )
	{
		const String& fontName = fontMap.m_fontName;
		Uint32 mapFlags = GFx::FontMap::MFF_Normal;

		if ( fontMap.m_bold )
		{
			mapFlags |= GFx::FontMap::MFF_Bold;
		}
		if ( fontMap.m_italic )
		{
			mapFlags |= GFx::FontMap::MFF_Italic;
		}
		if ( fontMap.m_fauxBold )
		{
			mapFlags |= GFx::FontMap::MFF_FauxBold;
		}
		if ( fontMap.m_fauxItalic )
		{
			mapFlags |= GFx::FontMap::MFF_FauxItalic;
		}
		if ( fontMap.m_noAutoFit )
		{
			mapFlags |= GFx::FontMap::MFF_NoAutoFit;
		}

		if ( ! pFontMap->MapFont( fontMap.m_alias.AsChar(), fontName.AsChar(), static_cast< GFx::FontMap::MapFontFlags >( mapFlags ), fontMap.m_scale ) )
		{
			GUI_WARN(TXT("Failed to map font '%ls' to '%ls'"), fontMap.m_alias.AsChar(), fontName.AsChar() );
		}
	}

	if ( m_currentGlyphCache != fontLib.m_glyphCache )
	{
		m_currentGlyphCache = fontLib.m_glyphCache;

		SF::Render::GlyphCacheParams params;
		ScaleformFontHelpers::GetGlyphCacheParams( fontLib.m_glyphCache, params );
#ifdef RED_LOGGING_ENABLED
		LOG_ENGINE(TXT("Using glyph param for fontsPath '%ls', %ls"), fontsPath.AsChar(), ScaleformFontHelpers::GetGlyphCacheStringForLog( params ).AsChar());
#endif

		Bool glyphCacheResult = false;
		CGuiRenderCommand<TSetGlyphCacheParams>::Send( params, &glyphCacheResult );
		GRender->Flush(); // wait for glyph cache to be updated before creating any movies that rely on it

		if ( !glyphCacheResult )
		{
			WARN_ENGINE(TXT("Failed to set glyph cache parameters for fontsPath '%ls'"), fontsPath.AsChar() );

			const Uint32 DEFAULT_GLYPH_CACHE = 0;
			ScaleformFontHelpers::GetGlyphCacheParams( DEFAULT_GLYPH_CACHE, params );
#ifdef RED_LOGGING_ENABLED
			LOG_ENGINE(TXT("Trying default glyph param for fontsPath '%ls', %ls"), fontsPath.AsChar(), ScaleformFontHelpers::GetGlyphCacheStringForLog( params ).AsChar());
#endif
			CGuiRenderCommand<TSetGlyphCacheParams>::Send( params, &glyphCacheResult );
			GRender->Flush(); // wait for glyph cache to be updated before creating any movies that rely on it
		}

		if ( !glyphCacheResult )
		{
			WARN_ENGINE(TXT("Failed to set ANY glyph cache parameters for fontsPath '%ls'"), fontsPath.AsChar() );
		}
	}

	return true;
}

Bool CScaleformLoader::UpdateTranslator( const SFlashFontLib& fontLib )
{
	// TBD: GlyphCache param values per language!
	SFUInt wwMode = GFx::Translator::WWT_Default;
	SFUInt extraCaps = 0;
	if ( fontLib.m_id == TXT("AR") )
	{
		// TBD: word wrapping and anything else not properly implemented by default?
		extraCaps = GFx::Translator::Cap_BidirectionalText;
	}
	else if ( fontLib.m_id == TXT("ZH") || fontLib.m_id == TXT("CN") )
	{
		wwMode = GFx::Translator::WWT_Chinese;
	}
	else if ( fontLib.m_id == TXT("KR") )
	{
		wwMode = GFx::Translator::WWT_Korean;
	}

	m_loader.SetTranslator( SF::Ptr<GFx::Translator>( *SF_NEW CScaleformTranslator( wwMode, extraCaps ) ) );

	return true;
}

Bool CScaleformLoader::SetExternalInterfaceHandler( IScaleformExternalInterfaceHandler* handler )
{
	SF::Ptr< GFx::ExternalInterface > externalInterface = m_loader.GetExternalInterface();
	if ( externalInterface )
	{
		static_cast< CScaleformExternalInterface* >( externalInterface.GetPtr() )->SetHandler( handler );
		return true;
	}
	return false;
}

Bool CScaleformLoader::SetUserEventHandler( IScaleformUserEventHandler* handler )
{
	SF::Ptr< GFx::UserEventHandler > userEvent = m_loader.GetUserEventHandler();
	if ( userEvent )
	{
		static_cast< CScaleformUserEvent* >( userEvent.GetPtr() )->SetHandler( handler );
		return true;
	}

	return false;
}

namespace Config
{
	TConfigVar< Bool > cvVideoVolumeLinearScale( "Video/Volume", "Linear", true ); // :(
}

Float GHackVideoSFXVolume = 1.f;
Float GHackVideoVoiceVolume = 1.f;

#ifdef RED_PLATFORM_ORBIS
# if WITH_SCALEFORM_VIDEO

namespace Config
{
	// Have fun generalizing these config vars to other platforms. For now just PS4.

	const Uint32 MAIN_THREAD_AFFINITY_MASK = 1<<3;
	const Uint32 RENDER_THREAD_AFFINITY_MASK = 1<<2;
	const Uint32 TASK_THREAD1_AFFINITY_MASK = 1<<0;
	const Uint32 TASK_THREAD2_AFFINITY_MASK = 1<<1;
	const Uint32 WEAK_CLUSTER_THREAD1_AFFINITY_MASK = 1<<4;
	const Uint32 WEAK_CLUSTER_THREAD2_AFFINITY_MASK = 1<<5;
	const Uint32 STRONG_CLUSTER_ALL_AFFINITY_MASK = MAIN_THREAD_AFFINITY_MASK|RENDER_THREAD_AFFINITY_MASK|TASK_THREAD1_AFFINITY_MASK|TASK_THREAD2_AFFINITY_MASK;

	const Int32 DEFAULT_DECODER_PRIORITY = SCE_KERNEL_PRIO_FIFO_HIGHEST + 10;
	const Uint32 DEFAULT_DECODER_AFFINITY_MASK = RENDER_THREAD_AFFINITY_MASK|TASK_THREAD1_AFFINITY_MASK|TASK_THREAD2_AFFINITY_MASK;

	// Keep the reader and sound hard affinitized and separate or else
	// you'll get stutters in heavier USMs (like intro.usm) when context switching to fetch I/O
	// Render thread, default pri to play nice. Not much game rendering to do while playing videos - thread is pretty often idle.
	// And this way we don't take up task threads which can block the renderer and main threads because of they themselves block on tasks
	const Uint32 VideoDecoderAffinityMask = DEFAULT_DECODER_AFFINITY_MASK;
	const Int32 VideoDecoderPriority = DEFAULT_DECODER_PRIORITY;
	
	const Uint32 VideoSoundAffinityMask = WEAK_CLUSTER_THREAD1_AFFINITY_MASK;
	const Int32 VideoSoundPriority = SCE_KERNEL_PRIO_FIFO_HIGHEST + 10;

	const Uint32 VideoReaderAffinityMask = WEAK_CLUSTER_THREAD2_AFFINITY_MASK;
	const Int32 VideoReaderPriority = SCE_KERNEL_PRIO_FIFO_HIGHEST + 10;

	const Uint32 DEFAULT_CRI_DELEGATE_AFFINITY_MASK = DEFAULT_DECODER_AFFINITY_MASK;
	const Uint32 CRIDelegateAffinityMask1 = DEFAULT_CRI_DELEGATE_AFFINITY_MASK;
	const Uint32 CRIDelegateAffinityMask2 = DEFAULT_CRI_DELEGATE_AFFINITY_MASK;
	const Uint32 CRIDelegateAffinityMask3 = DEFAULT_CRI_DELEGATE_AFFINITY_MASK;
	const Uint32 CRIDelegateAffinityMask4 = DEFAULT_CRI_DELEGATE_AFFINITY_MASK;
	const Uint32 CRIDelegateAffinityMask5 = DEFAULT_CRI_DELEGATE_AFFINITY_MASK;

	const Int32 DEFAULT_CRI_DELEGATE_PRIORITY = DEFAULT_DECODER_PRIORITY;
	const Int32 CRIDelegatePriority1 = DEFAULT_CRI_DELEGATE_PRIORITY;
	const Int32 CRIDelegatePriority2 = DEFAULT_CRI_DELEGATE_PRIORITY;
	const Int32 CRIDelegatePriority3 = DEFAULT_CRI_DELEGATE_PRIORITY;
	const Int32 CRIDelegatePriority4 = DEFAULT_CRI_DELEGATE_PRIORITY;
	const Int32 CRIDelegatePriority5 = DEFAULT_CRI_DELEGATE_PRIORITY;

	const Int32 CRIDelegateConcurrency = 2;

	typedef Validation::IntRange< SCE_KERNEL_PRIO_FIFO_HIGHEST, SCE_KERNEL_PRIO_FIFO_LOWEST > Priority;
	typedef Validation::IntRange< 0, 0x3F > Cpumask;

	TConfigVar< Int32, Cpumask > cvVideoDecoderAffinityMask( "Video/PS4/Threads/VideoDecoder", "AffinityMask", VideoDecoderAffinityMask );
	TConfigVar< Int32, Priority > cvVideoDecoderPriority( "Video/PS4/Threads/VideoDecoder", "Priority", VideoDecoderPriority );
	
	TConfigVar< Int32, Cpumask > cvVideoReaderAffinityMask( "Video/PS4/Threads/VideoReader", "AffinityMask", VideoReaderAffinityMask );
	TConfigVar< Int32, Priority > cvVideoReaderPriority( "Video/PS4/Threads/VideoReader", "Priority", VideoReaderPriority );
	
	TConfigVar< Int32, Cpumask > cvVideoSoundAffinityMask( "Video/PS4/Threads/VideoSound", "AffinityMask", VideoSoundAffinityMask );
	TConfigVar< Int32, Priority > cvVideoSoundPriority( "Video/PS4/Threads/VideoSound", "Priority", VideoSoundPriority );
	
	TConfigVar< Int32, Cpumask > cvVideoCRIDelegateAffinityMask1( "Video/PS4/Threads/CRIDelegate/1", "AffinityMask", CRIDelegateAffinityMask1 );
	TConfigVar< Int32, Cpumask > cvVideoCRIDelegateAffinityMask2( "Video/PS4/Threads/CRIDelegate/2", "AffinityMask", CRIDelegateAffinityMask2 );
	TConfigVar< Int32, Cpumask > cvVideoCRIDelegateAffinityMask3( "Video/PS4/Threads/CRIDelegate/3", "AffinityMask", CRIDelegateAffinityMask3 );
	TConfigVar< Int32, Cpumask > cvVideoCRIDelegateAffinityMask4( "Video/PS4/Threads/CRIDelegate/4", "AffinityMask", CRIDelegateAffinityMask4 );
	TConfigVar< Int32, Cpumask > cvVideoCRIDelegateAffinityMask5( "Video/PS4/Threads/CRIDelegate/5", "AffinityMask", CRIDelegateAffinityMask5 );

	TConfigVar< Int32, Priority > cvVideoCRIDelegatePriority1( "Video/PS4/Threads/CRIDelegate/1", "Priority", CRIDelegatePriority1 );
	TConfigVar< Int32, Priority > cvVideoCRIDelegatePriority2( "Video/PS4/Threads/CRIDelegate/2", "Priority", CRIDelegatePriority2 );
	TConfigVar< Int32, Priority > cvVideoCRIDelegatePriority3( "Video/PS4/Threads/CRIDelegate/3", "Priority", CRIDelegatePriority3 );
	TConfigVar< Int32, Priority > cvVideoCRIDelegatePriority4( "Video/PS4/Threads/CRIDelegate/4", "Priority", CRIDelegatePriority4 );
	TConfigVar< Int32, Priority > cvVideoCRIDelegatePriority5( "Video/PS4/Threads/CRIDelegate/5", "Priority", CRIDelegatePriority5 );

	TConfigVar< Int32, Validation::IntRange< 0, 5 > > cvVideoCRIDelegateConcurrency( "Video/PS4/Threads/CRIDelegate", "Concurrency", CRIDelegateConcurrency );
}

TRACE_FUNC static void FInitVideoDecoderThrd( ScePthread pid, int requestedPriority )
{
	RED_UNUSED( requestedPriority );
	SceKernelSchedParam schedParam;
	schedParam.sched_priority = Config::cvVideoDecoderPriority.Get();
	{
		ScePthreadAttr attr;
		scePthreadAttrInit( &attr );
		scePthreadAttrGet( pid, &attr );
		scePthreadAttrSetinheritsched( &attr, SCE_PTHREAD_EXPLICIT_SCHED );
		scePthreadSetschedparam( pid, SCE_KERNEL_SCHED_FIFO, &schedParam ); // CriDelegate uses FIFO scheduling too
		scePthreadAttrDestroy( &attr );
	}

	const SceKernelCpumask mask = (SceKernelCpumask)Config::cvVideoDecoderAffinityMask.Get();
	scePthreadSetaffinity( pid, mask );
}

TRACE_FUNC static void FInitVideoReaderThrd( ScePthread pid, int requestedPriority )
{
	RED_UNUSED( requestedPriority );

	SceKernelSchedParam schedParam;
	schedParam.sched_priority = Config::cvVideoReaderPriority.Get();
	{
		ScePthreadAttr attr;
		scePthreadAttrInit( &attr );
		scePthreadAttrGet( pid, &attr );
		scePthreadAttrSetinheritsched( &attr, SCE_PTHREAD_EXPLICIT_SCHED );
		scePthreadSetschedparam( pid, SCE_KERNEL_SCHED_FIFO, &schedParam );
		scePthreadAttrDestroy( &attr );
	}

	const SceKernelCpumask mask = (SceKernelCpumask)Config::cvVideoReaderAffinityMask.Get();
	scePthreadSetaffinity( pid, mask );
}

TRACE_FUNC static void FInitVideoSoundThrd( ScePthread pid, int requestedPriority )
{
	RED_UNUSED( requestedPriority );

	// Scheduling attr already correctly set, just set these directly
	const Int32 priority = Config::cvVideoSoundPriority.Get();
	scePthreadSetprio( pid, priority );

	const SceKernelCpumask mask = Config::cvVideoSoundAffinityMask.Get();
	scePthreadSetaffinity( pid, mask );
}

namespace Scaleform { namespace GFx { namespace Video {
	typedef void (*FUNC_INIT_VIDEO_DECODER_THRD)(ScePthread,int);
	typedef void (*FUNC_INIT_VIDEO_READER_THRD)(ScePthread,int);
	typedef void (*FUNC_INIT_VIDEO_SOUND_THRD)(ScePthread pid,int);
	
	extern FUNC_INIT_VIDEO_DECODER_THRD InitVideoDecoderThrd;
	extern FUNC_INIT_VIDEO_READER_THRD	InitVideoReaderThrd;
	extern FUNC_INIT_VIDEO_SOUND_THRD	InitVideoSoundThrd;
} } }

# endif // WITH_SCALEFORM_VIDEO
#endif // RED_PLATFORM_ORBIS

TRACE_FUNC Bool CScaleformLoader::InitVideo()
{
#if WITH_SCALEFORM_VIDEO
	struct VideoVMSupportNone : public GFx::Video::VideoVMSupport
	{
		VideoVMSupportNone()
			: GFx::Video::VideoVMSupport( nullptr, nullptr )
		{}
	};

	GFx::Video::VideoVMSupport videoVMSupport = VideoVMSupportNone();
	SF::Ptr< GFx::Video::Video > video;

# if WITH_SCALEFORM_AS2
	videoVMSupport.pAS2VSupport = GFx::Video::AS2VideoSupport::CreateInstance();
# endif
# if WITH_SCALEFORM_AS3
	videoVMSupport.pAS3VSupport = GFx::Video::AS3VideoSupport::CreateInstance();
# endif

	if ( !videoVMSupport.pAS2VSupport && !videoVMSupport.pAS3VSupport )
	{
		GUI_ERROR(TXT("Video has no VM support defined!"));
		return false;
	}

# if defined( RED_PLATFORM_WINPC )
	SF::UInt32 affinityMasks[] = {
		DEFAULT_VIDEO_DECODING_AFFINITY_MASK,
		DEFAULT_VIDEO_DECODING_AFFINITY_MASK,
		DEFAULT_VIDEO_DECODING_AFFINITY_MASK
	};
	video = *SF_NEW GFx::Video::VideoPC( videoVMSupport, SF::Thread::HighestPriority, MAX_VIDEO_DECODING_THREADS, affinityMasks );
# elif defined( RED_PLATFORM_DURANGO )
	DWORD_PTR affinityMasks[] = {
		GFx::Video::AllCPUs,
		GFx::Video::AllCPUs,
		GFx::Video::AllCPUs,
		GFx::Video::AllCPUs,
		GFx::Video::AllCPUs
	};
	video = *SF_NEW GFx::Video::VideoXboxOne( videoVMSupport, SF::Thread::HighestPriority, GFx::Video::XboxOneMaxVideoDecodingThreads, affinityMasks );
# elif defined( RED_PLATFORM_ORBIS )

	SF::GFx::Video::InitVideoDecoderThrd = &FInitVideoDecoderThrd;
	SF::GFx::Video::InitVideoReaderThrd = &FInitVideoReaderThrd;
	SF::GFx::Video::InitVideoSoundThrd = &FInitVideoSoundThrd;

	// CRI creates 5 threads anyway, but the rest are just suspended
	const Int32 numThreads = Config::cvVideoCRIDelegateConcurrency.Get();
	const SceKernelCpumask affinityMasks[] = { (SceKernelCpumask)Config::cvVideoCRIDelegateAffinityMask1.Get(),
											   (SceKernelCpumask)Config::cvVideoCRIDelegateAffinityMask2.Get(), 
											   (SceKernelCpumask)Config::cvVideoCRIDelegateAffinityMask3.Get(),
											   (SceKernelCpumask)Config::cvVideoCRIDelegateAffinityMask4.Get(),
											   (SceKernelCpumask)Config::cvVideoCRIDelegateAffinityMask5.Get() };

	const int priorities[] = { Config::cvVideoCRIDelegatePriority1.Get(), Config::cvVideoCRIDelegatePriority2.Get(),
							   Config::cvVideoCRIDelegatePriority3.Get(), Config::cvVideoCRIDelegatePriority4.Get(),
							   Config::cvVideoCRIDelegatePriority5.Get() };

	video = *SF_NEW GFx::Video::VideoPS4( videoVMSupport, numThreads, affinityMasks, priorities );
# else
#  error "Video playback not initialized for current platform"
# endif
	//TODO: Recompile SF for WWise support, and maybe use a wwise soundrenderer instead of initializing directly here
	// Also, need to call wwise update pre-advance apparently (not documented but as per the app sample)
# if WITH_SCALEFORM_WWISE
	video->SetSoundSystem( SF::Ptr< GFx::Video::VideoSoundSystem >( *SF_NEW GFx::Video::VideoSoundSystemWwise ) );
# elif WITH_SCALEFORM_XA2

	SF::Ptr< GFx::Video::VideoSoundSystem > soundSystem;
	GFx::Video::VideoSoundSystemXA2Tmp* pSoundXA2 = SF_NEW GFx::Video::VideoSoundSystemXA2Tmp( 0, 0 );
	if ( pSoundXA2->IsValid() )
	{
		soundSystem = *pSoundXA2;
		pSoundXA2 = nullptr;
	}
#ifdef RED_PLATFORM_WINPC
	else
	{
		LOG_ENGINE(TXT("Failed to create XAudio2 audio engine. Falling back on DirectSound!"));
		pSoundXA2->Release();
		pSoundXA2 = nullptr;
		soundSystem = *SF_NEW GFx::Video::VideoSoundSystemDX8Tmp( nullptr );
	}
#endif

	video->SetSoundSystem( soundSystem );

# elif WITH_SCALEFORM_AUDIOOUT
	video->SetSoundSystem( SF::Ptr< GFx::Video::VideoSoundSystem >( *SF_NEW GFx::Video::VideoSoundSystemPS4Tmp ) );
# endif

	// Can't set on the moviedef statebag before creating the movie instance, and can't remove it by setting nullptr after.
	// So we'll stop rogue videos from the URL builder.
	/*
	[2014.09.19 12:12:37][Error][Gui] Scaleform Error: 'Sprite::AddDisplayObject(): unknown cid = 1'
	[2014.09.19 12:12:37][Error][Gui] Scaleform Error: 'TypeError: Error #1009: Cannot access a property or method of a null object reference.
	at VideoPlayer instance constructor()'
	*/
	video->SetReadCallback( SF::Ptr< SF::GFx::Video::VideoBase::ReadCallback >( *SF_NEW VideoReadCallback ) );
	m_loader.SetVideo( video );

	return true;
#else // !WITH_SCALEFORM_VIDEO
	return false;
#endif // WITH_SCALEFORM_VIDEO
}

Bool CScaleformLoader::InitActionScript()
{
#if WITH_SCALEFORM_AS2
	if ( (m_systemFlags & SYS_AS2 ) != 0 )
	{
		m_loader.SetAS2Support( SF::Ptr<GFx::AS2Support>( *SF_NEW GFx::AS2Support ) );
	}
#endif

#if WITH_SCALEFORM_AS3
	if ( (m_systemFlags & SYS_AS3) != 0 )
	{
		m_loader.SetAS3Support( SF::Ptr<GFx::AS3Support>( *SF_NEW GFx::AS3Support ) );
	}
#endif

#if !WITH_SCALEFORM_AS2 && !WITH_SCALEFORM_AS3
	return false;
#else
	return true;
#endif
}

Bool CScaleformLoader::InitImages()
{
	// Image handlers
	{
		SF::Ptr<GFx::ImageFileHandlerRegistry> pImageFileHandlerRegistry = *SF_NEW GFx::ImageFileHandlerRegistry;
		pImageFileHandlerRegistry->AddHandler( &SF::Render::DDS::FileReader::Instance );
		pImageFileHandlerRegistry->AddHandler( &SF::Render::JPEG::FileReader::Instance );
		pImageFileHandlerRegistry->AddHandler( &SF::Render::PNG::FileReader::Instance );
		m_loader.SetImageFileHandlerRegistry( pImageFileHandlerRegistry );

		return true;
	}
	
	return false;
}

void CScaleformLoader::AddAdditionalContentDirectory( const String& contentDir )
{
	GFx::ImageCreator* imageCreator = m_loader.GetImageCreator().GetPtr();
	if( imageCreator )
	{
		CScaleformImageCreator* scaleFormImageCreator = static_cast<CScaleformImageCreator*>( imageCreator );
		scaleFormImageCreator->AddAdditionalContentDirectory( contentDir );
	}
}

void CScaleformLoader::RemoveAdditionalContentDirectory(  const String& contentDir )
{
	GFx::ImageCreator* imageCreator = m_loader.GetImageCreator().GetPtr();
	if( imageCreator )
	{
		CScaleformImageCreator* scaleFormImageCreator = static_cast<CScaleformImageCreator*>( imageCreator );
		scaleFormImageCreator->RemoveAdditionalContentDirectory( contentDir );
	}
}

#endif // USE_SCALEFORM
