/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../core/resource.h"
#include "bitmapTexture.h"

class CRenderFrame;

struct SFontGlyph
{
	Float				m_UVs[2][2];		//!< rectangle occupied by glyph on texture
	Uint32				m_textureIndex;		//!< texture index
	Uint32				m_width;			//!< glyph width
	Uint32				m_height;			//!< glyph height	
	Int32					m_advanceX;			//!< value used to advance pen position
	Int32					m_bearingX;			//!< horizontal position of the glyph relative to current pen pos
	Int32					m_bearingY;			//!< vertical position of the glyph relative to baseline

	RED_INLINE SFontGlyph() 
		: m_textureIndex( 0 )
		, m_width( 0 )
		, m_height( 0 )
		, m_advanceX( 0 )
		, m_bearingX( 0 )
		, m_bearingY( 0 )
	{
		m_UVs[0][0] = 0.0f;
		m_UVs[0][1] = 0.0f;
		m_UVs[1][0] = 0.0f;
		m_UVs[1][1] = 0.0f;
	}

	friend IFile& operator<<( IFile& file, SFontGlyph& glyph )
	{
		file << glyph.m_UVs[0][0];
		file << glyph.m_UVs[0][1];
		file << glyph.m_UVs[1][0];
		file << glyph.m_UVs[1][1];
		file << glyph.m_textureIndex;	
		file << glyph.m_width;		
		file << glyph.m_height;		
		file << glyph.m_advanceX;		
		file << glyph.m_bearingX;		
		file << glyph.m_bearingY;

		return file;
	}
};

struct SFontInfo
{	
	static const Uint16			GLYPH_DOESNT_EXIST = 0xFFFF;	
	TDynArray<Uint16>			m_unicodeMapping;				//!< mapping of unicode codes to glyph indices

	Uint32						m_lineDist;						//!< distance between baselines
	Uint32						m_maxGlyphHeight;				//!< maximum glyph height
	Bool						m_kerning;						//!< does font contain kerning info

	TDynArray< SFontGlyph >		m_glyphs;						//!< array of glyphs

	RED_INLINE SFontInfo()
		: m_unicodeMapping( 65536 )
		, m_lineDist( 0 )
		, m_maxGlyphHeight( 0 )
		, m_kerning( false )
	{		
		for( Uint32 i=0; i<m_unicodeMapping.Size(); ++i )
			m_unicodeMapping[ i ] = GLYPH_DOESNT_EXIST;
	}

	friend IFile& operator<<( IFile& file, SFontInfo& fontInfo )
	{
		file << fontInfo.m_unicodeMapping;
		file << fontInfo.m_lineDist;
		file << fontInfo.m_maxGlyphHeight;
		file << fontInfo.m_kerning;
		file << fontInfo.m_glyphs;

		return file;
	}
};

class CFont : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CFont, CResource, "w2fnt", "Font" );	

	SFontInfo								m_fontInfo;
	TDynArray< THandle< CBitmapTexture > >	m_textures;

private:
	CFont();

public:
	void OnSerialize( IFile &file );



	virtual ResourceReloadPriority GetReloadPriority() { return 0; }

public:
	RED_INLINE Uint32 GetNumTextures() const { return m_textures.Size(); }

	RED_INLINE const CBitmapTexture* GetTexture( Uint32 index ) const { return m_textures[index].Get(); }

	RED_INLINE CBitmapTexture* GetTexture( Uint32 index ) { return m_textures[index].Get(); }

	const SFontGlyph* GetGlyph( Char character ) const; 	

	Uint32 GetLineDist() const;

	void Print( CRenderFrame *frame, Int32 x, Int32 y, Float z, const Char* text, const Color &color = Color( 255, 255, 255 ) );

	void GetTextRectangle( const String& text, Int32 &x, Int32 &y, Uint32 &width, Uint32 &height ) const;
	void GetTextRectangle( const Char* text, Uint32 length, Int32 &x, Int32 &y, Uint32 &width, Uint32 &height ) const;

public:
	// font object creation 
	struct FactoryInfo : public CResource::FactoryInfo< CFont >
	{
		SFontInfo								m_fontInfo;		//!< font info structure
		TDynArray< CBitmapTexture::MipMap >		m_textures;		//!< raw font textures
	};

	void Init( const FactoryInfo& data );
};

BEGIN_CLASS_RTTI( CFont );
	PARENT_CLASS( CResource );
	PROPERTY_INLINED_RO( m_textures, TXT("Font textures") );
END_CLASS_RTTI();

RED_INLINE void CFont::GetTextRectangle( const String& text, Int32 &x, Int32 &y, Uint32 &width, Uint32 &height ) const
{
	GetTextRectangle(text.AsChar(), text.GetLength(), x, y, width, height);
}
