#include "build.h"

#include "cookerTextEncoder.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef RED_PLATFORM_WIN64
# define LIB_HARFBUZZ_DIR "../../../external/harfbuzz-0.9.33/lib/x64/"
#else
# define LIB_HARFBUZZ_DIR "../../../external/harfbuzz-0.9.33/lib/x86/"
#endif

#ifdef _DEBUG
# pragma comment( lib, LIB_HARFBUZZ_DIR "Debug/harfbuzz.lib" )
#else
# pragma comment( lib, LIB_HARFBUZZ_DIR "Release/harfbuzz.lib" )
#endif

#include "../../common/core/stringConversion.h"

#include "../../../external/freetype/include/freetype/ftmodapi.h"

#include <hb.h>
#include <hb-ot-font.h>
#include <hb-ft.h>

#pragma comment( lib, "usp10.lib")
#pragma comment( lib, "Rpcrt4.lib")

static void* FreeType_Alloc( FT_Memory memory, long size )
{
	return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Engine, static_cast< size_t>( size ), 16 );
}

static void FreeType_Free( FT_Memory memory, void* block )
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_Engine, block );
}

static void* FreeType_Realloc( FT_Memory  memory, long cur_size, long new_size, void* block )
{
	return RED_MEMORY_REALLOCATE_ALIGNED( MemoryPool_Default, block, MC_Engine, new_size, 16 );
}

CCookerTextEncoder::SFontEntry::SFontEntry( const String& fontName, hb_font_t* font, FT_Face face )
	: m_fontName( fontName )
	, m_font( font )
	, m_face( face )
{}

CCookerTextEncoder::CCookerTextEncoder()
	: m_currentFontEntry( nullptr )
	, m_currentLanguageScript( SCRIPT_Invalid )
{
	m_freeTypeContext.m_memory = (FT_Memory)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Engine, sizeof(*m_freeTypeContext.m_memory) );
	Red::System::MemoryZero( m_freeTypeContext.m_memory, sizeof(*m_freeTypeContext.m_memory) );
	m_freeTypeContext.m_memory->alloc = FreeType_Alloc;
	m_freeTypeContext.m_memory->realloc = FreeType_Realloc;
	m_freeTypeContext.m_memory->free = FreeType_Free;
	m_freeTypeContext.m_memory->user = &m_freeTypeContext;

	if ( FT_New_Library( m_freeTypeContext.m_memory, &m_freeTypeContext.m_library ) == 0 )
	{
// 		m_freeTypeContext.m_library->version_major = FREETYPE_MAJOR;
// 		m_freeTypeContext.m_library->version_minor = FREETYPE_MINOR;
// 		m_freeTypeContext.m_library->version_patch = FREETYPE_PATCH;

		FT_Add_Default_Modules( m_freeTypeContext.m_library );
	}

	m_harfBuzzBuffer = hb_buffer_create();
}

CCookerTextEncoder::~CCookerTextEncoder()
{
	Cleanup();

	if ( m_freeTypeContext.m_library )
	{
		FT_Done_Library( m_freeTypeContext.m_library );
		m_freeTypeContext.m_library = nullptr;
	}
	RED_MEMORY_FREE( MemoryPool_Default, MC_Engine, m_freeTypeContext.m_memory );
	m_freeTypeContext.m_memory = nullptr;
}

void CCookerTextEncoder::Cleanup()
{
	for ( auto it = m_fontMap.Begin(); it != m_fontMap.End(); ++it )
	{
		SFontEntry* fontEntry = it->m_second;
		if ( fontEntry->m_font )
		{
			hb_font_destroy( fontEntry->m_font );
		}
		if ( fontEntry->m_face )
		{
			FT_Done_Face( fontEntry->m_face );
		}
		delete fontEntry;
	}

	if ( m_harfBuzzBuffer )
	{
		hb_buffer_destroy( m_harfBuzzBuffer );
		m_harfBuzzBuffer = nullptr;
	}
}

Bool CCookerTextEncoder::LoadFont( const String& fontAbsolutePath )
{
	CFilePath filePath( fontAbsolutePath );

	const String fontNameKey = filePath.GetFileNameWithExt().ToLower();
	SFontEntry* fontEntry = nullptr;
	m_fontMap.Find( fontNameKey, fontEntry );
	if ( fontEntry )
	{
		ERR_WCC(TXT("Font '%ls' already added. Can't add again from '%ls'"), fontNameKey.AsChar(), fontAbsolutePath.AsChar() );
		return false;
	}

	FT_Face face;
	FT_Error error = FT_New_Face( m_freeTypeContext.m_library, UNICODE_TO_ANSI(fontAbsolutePath.AsChar()), 0, &face );
	if ( error )
	{
		ERR_WCC(TXT("Unable to load font from '%ls'"), fontAbsolutePath.AsChar() );
		return false;
	}

	// Select explicitly just in case, although unicode should be selected by default if the font supports the charmap
	error = FT_Select_Charmap( face, FT_ENCODING_UNICODE );
	if ( error )
	{
		FT_Done_Face( face );
		ERR_WCC(TXT("Unable to select unicode charmap on font from '%ls'"), fontAbsolutePath.AsChar() );
		return false;
	}

	hb_font_t* font = hb_ft_font_create( face, nullptr );
	if ( !font )
	{
		FT_Done_Face( face );
		ERR_WCC(TXT("Unable to load font from '%ls'"), fontAbsolutePath.AsChar() );
		return false;
	}

	fontEntry = new SFontEntry( fontNameKey, font, face );
	m_fontMap.Insert( fontNameKey, fontEntry );

	FT_ULong charCode = 0;
	FT_UInt glyphIndex = 0;
	charCode = FT_Get_First_Char( face, &glyphIndex );
	while ( glyphIndex != 0 )
	{
		if ( charCode <= 0xFFFFFFFF )
		{
			fontEntry->m_glyphIndexCharMap.Insert( glyphIndex, (Uint32)charCode );
		}
		else
		{
			WARN_WCC(TXT("Skipping charCode '%llu' in font '%ls'"), (Uint64)charCode, fontNameKey.AsChar() );
		}
		charCode = FT_Get_Next_Char( face, charCode, &glyphIndex );
	}

	return true;
}

Bool CCookerTextEncoder::ShapeText( const String& line, String& outText )
{
	SFontEntry* fontEntry = m_currentFontEntry;
	if ( ! fontEntry )
	{
		ERR_WCC(TXT("Not font selected. Select with SelectFont()"));
		return false;
	}

	if ( ! fontEntry->m_font )
	{
		ERR_WCC(TXT("Font entry '%ls' has no associated font!"), fontEntry->m_fontName.AsChar() );
		return false;
	}

	switch ( m_currentLanguageScript )
	{
	case SCRIPT_Latin:
		SetupDefaultBuffer();
		break;
	case SCRIPT_Arabic:
		SetupArabicBuffer();
		break;
	case SCRIPT_Invalid: /* fall through */
	default:
		ERR_WCC(TXT("Invalid current language script. Set with SelectLanguageScript()"));
		return false;
	}

	if ( line == String::EMPTY )
	{
		return true;
	}

	static_assert( sizeof(Char) == sizeof(uint16_t), "Incompatible types sizes" );
	const Uint32 strLen = line.GetLength();
	hb_buffer_add_utf16( m_harfBuzzBuffer, (const uint16_t*)line.AsChar(), strLen, 0, strLen );

	const AnsiChar* shapers[] = { "ot", nullptr }; // {"uniscribe", "ot", nullptr }
	hb_shape_full( fontEntry->m_font, m_harfBuzzBuffer, nullptr, 0, shapers );

	Uint32 numGlyphs = 0;
	hb_glyph_info_t* glyphInfos = hb_buffer_get_glyph_infos( m_harfBuzzBuffer, &numGlyphs );

	outText.ResizeFast( numGlyphs + 1 );

#ifdef _DEBUG
	AnsiChar glyphName[128];
#endif
	for ( Uint32 i = 0; i < numGlyphs; ++i )
	{
#ifdef _DEBUG
		hb_font_glyph_to_string( fontEntry->m_font, glyphInfos[i].codepoint, glyphName, sizeof(glyphName) );
#endif
		Uint32 encoding = 0xFFFFFFFF;
		const Uint32 glyphIndex = glyphInfos[i].codepoint; // "codepoint" is really glyphIndex
		fontEntry->m_glyphIndexCharMap.Find( glyphIndex, encoding );
		if ( encoding >= 0xFFFF )
		{
#ifdef _DEBUG
 			ERR_WCC(TXT("Glyph '%ls' with encoding '0x%08X' out of range in string '%ls'"), ANSI_TO_UNICODE(glyphName), encoding, line.AsChar() );
#endif
			encoding = 0xFFFF; // TBD, kind of a big hack anyway
		}
		outText[ i ] = static_cast< Char >( encoding );
	}
	outText[ numGlyphs ] = TXT('\0');

	return true;
}

void CCookerTextEncoder::SetupDefaultBuffer()
{
	hb_buffer_clear_contents( m_harfBuzzBuffer );
	hb_buffer_set_unicode_funcs( m_harfBuzzBuffer, nullptr ); // resets to default unicode funcs. Shouldn't have to call it...
	//hb_buffer_set_replacement_codepoint(m_harfBuzzBuffer, (hb_codepoint_t) -1);
	hb_buffer_set_direction( m_harfBuzzBuffer, HB_DIRECTION_LTR );
	hb_buffer_set_script( m_harfBuzzBuffer, HB_SCRIPT_LATIN );
	hb_buffer_set_language( m_harfBuzzBuffer, hb_language_from_string("en",-1));
	//hb_buffer_set_flags( m_harfBuzzBuffer, flags );
}

void CCookerTextEncoder::SetupArabicBuffer()
{
	hb_buffer_clear_contents( m_harfBuzzBuffer );
	hb_buffer_set_unicode_funcs( m_harfBuzzBuffer, nullptr ); // resets to default unicode funcs. Shouldn't have to call it...
	//hb_buffer_set_replacement_codepoint(m_harfBuzzBuffer, (hb_codepoint_t) -1);
	hb_buffer_set_direction( m_harfBuzzBuffer, HB_DIRECTION_RTL );
	hb_buffer_set_script( m_harfBuzzBuffer, HB_SCRIPT_ARABIC );
	hb_buffer_set_language( m_harfBuzzBuffer, hb_language_from_string("ar",-1));
	//hb_buffer_set_flags( m_harfBuzzBuffer, flags );
}

Bool CCookerTextEncoder::SelectFont( const String& fontName )
{
	if ( m_fontMap.Find( fontName.ToLower(), m_currentFontEntry ) )
	{
		return true;
	}

	ERR_WCC(TXT("Failed to find loaded font '%ls'"), fontName.AsChar() );
	return false;
}

static Char ReverseChar( Char ch )
{
	Char rch = ch;
	switch (ch)
	{
	case TXT('('):
		rch = TXT(')');
		break;
	case TXT(')'):
		rch = TXT('(');
		break;
	case TXT('['):
		rch = TXT(']');
		break;
	case TXT(']'):
		rch = TXT('[');
		break;
	case TXT('<'):
		rch = TXT('>');
		break;
	case TXT('>'):
		rch = TXT('<');
		break;
	case TXT('{'):
		rch = TXT('}');
		break;
	case TXT('}'):
		rch = TXT('{');
		break;
	default:
		break;
	}

	return rch;
}

void CCookerTextEncoder::ReverseTextInPlace( String& inoutText )
{
	// Reverse the RTL text, so it can be reversed again by Scaleform's bidi (and handle HTML)... yeah
	// Undo symbols that Scaleform's bidi will swap. Also for fixing HTML.
	// Could make a custom OnBidirectionalText but w/e
	
	const Int32 strLen = (Int32)inoutText.GetLength();
	const Int32 halfStrLen = strLen / 2;

	if ( strLen < 1 )
	{
		return;
	}

	for ( Int32 i = 0; i < halfStrLen; ++i )
	{
		const Char tmp = inoutText[ strLen - 1 - i ];
		inoutText[ strLen - 1 - i ] = ReverseChar( inoutText[ i ] );
		inoutText[ i ] = ReverseChar( tmp );
	}

	// The middle char still needs to be reversed
	const Bool isOddLength = (strLen & 1 ) != 0;
	if ( isOddLength )
	{
		inoutText[ halfStrLen ] = ReverseChar( inoutText[ halfStrLen ] );
	}
}

