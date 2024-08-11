/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/dynarray.h"
#include "../core/softHandle.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CSwfResource;

struct SFlashFontMap
{
	String	m_alias;
	String	m_fontName;
	Float	m_scale;
	Bool	m_italic;
	Bool	m_bold;
	Bool	m_fauxItalic;
	Bool	m_fauxBold;
	Bool	m_noAutoFit;

	SFlashFontMap()
		: m_scale(1.f)
		, m_italic( false )
		, m_bold( false )
		, m_fauxItalic( false )
		, m_fauxBold( false )
		, m_noAutoFit( false )
	{}
};

struct SFlashFontLib
{
	static const Int32							DEFAULT_GLYPH_CACHE = 0;

	String										m_id; // Id of actual textLocale, a ref will copy this instead of having its own
	String										m_fontLibPath;
	TDynArray< SFlashFontMap >					m_fontMaps;
	Int32										m_glyphCache;

	SFlashFontLib() 
		 : m_glyphCache(DEFAULT_GLYPH_CACHE)
	{}
};

//////////////////////////////////////////////////////////////////////////
// CFlashFontConfig
//////////////////////////////////////////////////////////////////////////
class CFlashFontConfig
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	static const Char* const					DEFAULT_FONTLIB;

public:
	static const SFlashFontLib					INVALID_FONTLIB;

private:
	THashMap< String, SFlashFontLib >			m_fontLibMap;	
	String										m_defaultFontlib;

public:
	CFlashFontConfig();
	~CFlashFontConfig();

public:
	const SFlashFontLib& GetFontLib( const String& textLocale ) const;
	const String& GetDefaultFontlib() const { return m_defaultFontlib; }

public:
	Bool LoadFromFile();
};