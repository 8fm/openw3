/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../core/gatheredResource.h"
#include "../core/xmlReader.h"
#include "../core/scopedPtr.h"
#include "../core/depot.h"
#include "guiGlobals.h"
#include "flashFontConfig.h"

CGatheredResource resFlashFontConfig( TXT("gameplay\\gui_new\\fonts.xml"), 0 );

//////////////////////////////////////////////////////////////////////////
// CFlashFontConfig
//////////////////////////////////////////////////////////////////////////
const SFlashFontLib CFlashFontConfig::INVALID_FONTLIB;
const Char* const CFlashFontConfig::DEFAULT_FONTLIB = TXT("gfxfontlib");

CFlashFontConfig::CFlashFontConfig()
	: m_defaultFontlib( DEFAULT_FONTLIB )
{
}

CFlashFontConfig::~CFlashFontConfig()
{
}

Bool CFlashFontConfig::LoadFromFile()
{
	m_fontLibMap.Clear();

	// load font config
	Red::TScopedPtr< CXMLReader > xml( GDepot->LoadXML( resFlashFontConfig.GetPath().ToString() ) ); 
	if ( !xml )
	{
		GUI_ERROR(TXT("InitFontConfig: Unable to load font config"));
		return false;
	}

	// process the fonts
	if ( xml->BeginNode( TXT("fonts") ) )
	{
		if ( !xml->Attribute( TXT("fontlib"), m_defaultFontlib ) || m_defaultFontlib.Empty() )
		{
			m_defaultFontlib = DEFAULT_FONTLIB;
			GUI_WARN(TXT("No default fontlib found. Using %ls"), m_defaultFontlib.AsChar() );
		}

		// process data for each language
		while ( xml->BeginNode( TXT("lang") ) )
		{
			// get language name
			String name;
			if ( !xml->Attribute( TXT("name"), name ) )
			{
				GUI_ERROR(TXT("CFlashFontConfig::LoadFromFile(): Missing language name"));
				return false;
			}

			// get reference data
			String refName;
			if ( xml->Attribute( TXT("ref"), refName ) )
			{
				const SFlashFontLib* existingFontLib = m_fontLibMap.FindPtr( refName );
				if ( !existingFontLib )
				{
					GUI_ERROR(TXT("CFlashFontConfig::LoadFromFile(): Missing reference fontlib '%ls' for language '%ls'"), refName.AsChar(), name.AsChar() );
					return false;
				}

				// copy data
				SFlashFontLib lib = *existingFontLib;
				lib.m_id = name;
				m_fontLibMap[ name ] = lib;

				// end the "lang" node
				xml->EndNode();
				continue;
			}

			// create mapping
			SFlashFontLib& fontLib = m_fontLibMap[ name ];
			fontLib.m_id = name;

			// get font lib
			if ( !xml->Attribute( TXT("fontlib"), fontLib.m_fontLibPath ) )
			{
				GUI_ERROR(TXT("CFlashFontConfig::LoadFromFile(): Missing fontlib attribute on language '%ls'"), name.AsChar() );
				return false;
			}

			String tmp;
			Int32 glyphCache = 0;
			if ( !xml->Attribute( TXT("glyphcache"), tmp) || !FromString<Int32>(tmp,glyphCache) )
			//if ( !xml->AttributeTT( TXT("glyphcache"), fontLib.m_glyphCache ) )
			{
				GUI_LOG(TXT("Using default glyphCache %d for fontLibPath '%ls'"), fontLib.m_glyphCache, fontLib.m_fontLibPath.AsChar());
			}
			else
			{
				fontLib.m_glyphCache = glyphCache;
				GUI_LOG(TXT("Using glyphCache %d for fontLibPath '%ls'"), fontLib.m_glyphCache, fontLib.m_fontLibPath.AsChar());
			}

			// parse font names
			while ( xml->BeginNextNode() )
			{
				SFlashFontMap map;

				// get font definition name
				if ( !xml->GetNodeName(map.m_alias) )
				{
					GUI_ERROR(TXT("CFlashFontConfig::LoadFromFile(): Missing font definition name in language '%ls'"), name.AsChar() );
					return false;
				}

				// parse name
				if ( !xml->Attribute( TXT("font"), map.m_fontName ) )
				{
					GUI_ERROR(TXT("CFlashFontConfig::LoadFromFile(): Missing font name in font definition '%ls' in language '%ls'"), map.m_alias.AsChar(), name.AsChar() );
					return false;
				}

				// parse additional attributes
				map.m_alias = TXT("$") + map.m_alias; // scaleform...
				map.m_bold = xml->AttributeTT( TXT("bold"), false );
				map.m_italic = xml->AttributeTT( TXT("italic"), false );
				map.m_scale = xml->AttributeTT( TXT("scale"), 1.0f );
				map.m_fauxBold = xml->AttributeTT( TXT("fauxbold"), false );
				map.m_fauxItalic = xml->AttributeTT( TXT("fauxitalic"), false );
				map.m_noAutoFit = xml->AttributeTT( TXT("noautofit"), false );
				fontLib.m_fontMaps.PushBack( map );

				// end the font definition name
				xml->EndNode();
			}

			// end the "lang" node
			xml->EndNode();
		}
	}

	return true;
}

const SFlashFontLib& CFlashFontConfig::GetFontLib( const String& textLocale ) const
{
	const SFlashFontLib* pFontLib = m_fontLibMap.FindPtr( textLocale );
	if ( pFontLib )
	{
		return *pFontLib;
	}

	return INVALID_FONTLIB;
}

