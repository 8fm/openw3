#pragma once

struct hb_font_t;
struct hb_buffer_t;
struct FT_FaceRec_;
typedef FT_FaceRec_*  FT_Face;
struct FT_LibraryRec_;
typedef FT_LibraryRec_ * FT_Library;
struct FT_MemoryRec_;
typedef FT_MemoryRec_*  FT_Memory;

class CCookerTextEncoder
{
public:
	enum ELanguageScript
	{
		SCRIPT_Invalid,
		SCRIPT_Latin,
		SCRIPT_Arabic,
	};

private:
	struct SFontEntry
	{
		DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Engine );

		String								m_fontName; // For debugging
		hb_font_t*							m_font;
		FT_Face								m_face; // maybe can pass FT_Done_Face into hb_ft_create_font?
		THashMap< Uint32 , Uint32 >			m_glyphIndexCharMap;

		SFontEntry( const String& fontName, hb_font_t* font, FT_Face face );
	
	};

private:
	THashMap< String, SFontEntry* >	m_fontMap;

private:
	struct SFreeTypeContext
	{
		FT_Library						m_library;
		FT_Memory						m_memory;

		SFreeTypeContext()
			: m_library( nullptr )
			, m_memory( nullptr )
		{}
	};

private:
	SFreeTypeContext					m_freeTypeContext;
	hb_buffer_t*						m_harfBuzzBuffer;

private:
	SFontEntry*							m_currentFontEntry;
	ELanguageScript						m_currentLanguageScript;

public:
	CCookerTextEncoder();
	~CCookerTextEncoder();

public:
	Bool LoadFont( const String& fontAbsolutePath );

public:
	Bool SelectFont( const String& fontName );
	void SelectLanguageScript( ELanguageScript script ) { m_currentLanguageScript = script; }

public:
	Bool ShapeText( const String& line, String& outText );
	void ReverseTextInPlace( String& inoutText );

private:
	void Cleanup();

private:
	void SetupDefaultBuffer();
	void SetupArabicBuffer();
};

