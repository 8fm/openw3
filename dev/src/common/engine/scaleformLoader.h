/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

#include <Kernel/SF_RefCount.h>

#include "scaleformConfig.h"
#include "flashFontConfig.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class IScaleformExternalInterfaceHandler;
class IScaleformUserEventHandler;

#if WITH_SCALEFORM_VIDEO
namespace Scaleform { namespace GFx { namespace Video {
	class Video;
} } }
#endif

#ifdef RED_PLATFORM_WINPC
class CScaleformSideLoaderThread;
#endif

//////////////////////////////////////////////////////////////////////////
// CScaleformLoader
//////////////////////////////////////////////////////////////////////////
class CScaleformLoader : public SF::RefCountBase<CScaleformLoader, SF::Stat_Default_Mem>
{
public:
	enum ESystemFlags
	{
		SYS_Images =			FLAG(0),
		SYS_AS2 =				FLAG(2),
		SYS_AS3 =				FLAG(3),
	};

private:
	String						m_currentTextLocale;
	String						m_currentFontConfigId;
	Int32						m_currentGlyphCache;

private:
	GFx::Loader					m_loader;
	SF::Ptr< GFx::MovieDef >	m_fontsMovieDef;
	Uint32						m_systemFlags;
	Bool						m_isInitialized;

private:
#ifdef RED_PLATFORM_WINPC
	CScaleformSideLoaderThread*	m_sideLoaderThread;
#endif

public:
	CScaleformLoader();
	Bool Init( Uint32 systemFlags );
	void Shutdown();

public:
	struct SCreateMovieContext
	{
		IScaleformExternalInterfaceHandler*		m_externalInterfaceHandler;
		IScaleformUserEventHandler*				m_userEventHandler;

		SCreateMovieContext()
			: m_externalInterfaceHandler( nullptr )
			, m_userEventHandler( nullptr )
		{}
	};

	void AddAdditionalContentDirectory( const String& contentDir );
	void RemoveAdditionalContentDirectory(  const String& contentDir );

public:
	GFx::Movie* CreateMovie( const TSoftHandle< CSwfResource >& swfResource, const SCreateMovieContext& context, Bool waitForLoadFinish = false );
	GFx::Movie* CreateMovie( const THandle< CSwfResource >& swfResource, const SCreateMovieContext& context, Bool waitForLoadFinish = false );

public:
	void SetDefaultFontLibName( const String& fontlibName );
	Bool SetFontLib( const SFlashFontLib& fontLib );
	Bool SetExternalInterfaceHandler( IScaleformExternalInterfaceHandler* handler );
	Bool SetUserEventHandler( IScaleformUserEventHandler* handler );

private:
	Bool LoadFonts( const SFlashFontLib& fontLib );
	Bool UpdateTranslator( const SFlashFontLib& fontLib );

private:
	GFx::Movie* CreateMovie( const String& swfDepotPath, const SCreateMovieContext& context, Bool waitForLoadFinish );

private:
	Bool InitImages();
	Bool InitActionScript();
	Bool InitVideo();
};

#endif // USE_SCALEFORM
