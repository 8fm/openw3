/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderFragment.h"
#include "fonts.h"

IMPLEMENT_ENGINE_CLASS( CFont );

CFont::CFont()
{
}

void CFont::OnSerialize( IFile &file )
{
	TBaseClass::OnSerialize( file );

	if( !file.IsGarbageCollector() )
	{
		file << m_fontInfo;
	}
}

void CFont::Init( const FactoryInfo& data )
{

	m_fontInfo = data.m_fontInfo;

	m_textures.Clear();

	for( Uint32 i=0; i<data.m_textures.Size(); ++i )
	{
		// Setup texture import stuff
		CBitmapTexture* texture = CreateObject< CBitmapTexture >( this );

		Bool result = texture->InitFromMip( data.m_textures[i], CNAME( Font ), TRF_AlphaGrayscale );

		ASSERT( result );

		if ( result )
		{
			m_textures.PushBack( texture );
		}
	}
}

const SFontGlyph* CFont::GetGlyph( Char character ) const
{
	Uint16 glyphIndex = SFontInfo::GLYPH_DOESNT_EXIST;
	if ( (Uint32)character < m_fontInfo.m_unicodeMapping.Size())
	{
		glyphIndex = m_fontInfo.m_unicodeMapping[ character ];
	}
	else
	{
	//	WARN_ENGINE(TXT("Character '%lc' (0x%04X) outside unicode mapping for font '%ls'"), character, (Uint32)character, GetDepotPath().AsChar() );
	}

	if ( glyphIndex == SFontInfo::GLYPH_DOESNT_EXIST )
	{
		return NULL;
	}
	else
	{
		return &(m_fontInfo.m_glyphs[ glyphIndex ]);
	}
}

Uint32 CFont::GetLineDist() const
{
	return m_fontInfo.m_lineDist;
}

void CFont::Print( CRenderFrame *frame, Int32 x, Int32 y, Float z, const Char* text, const Color &color )
{
	Matrix textMatrix;
	textMatrix.SetIdentity();
	textMatrix.SetTranslation( (Float)x, (Float)y, z );

	new (frame) CRenderFragmentText( frame, textMatrix, *this, text, color );
}

void CFont::GetTextRectangle( const Char* text, Uint32 length, Int32 &x, Int32 &y, Uint32 &width, Uint32 &height ) const
{
	Int32 maxHeight = 0;
	Uint32 maxWidth = 0;
	Uint32 penPos = 0;
	Int32 newLine = 0;
	for( Uint32 i = 0; i < length; ++i )
	{
		const SFontGlyph* glyph = GetGlyph( text[i] );
		if ( !glyph )
		{
			if ( text[i] == '\n' )
			{
				newLine++;
				penPos = 0;
			}
			continue;
		}

		Int32 xPos = penPos;
		Int32 yPos = 0;

		// add bearing
		xPos += glyph->m_bearingX;
		yPos -= glyph->m_bearingY;

		if ( yPos < maxHeight )
			maxHeight = yPos;

		// move pen to next position
		Uint32 newPenPos = penPos + glyph->m_advanceX;
		penPos = Max( newPenPos, penPos );

		if ( penPos > maxWidth )
			maxWidth = penPos;
	}

	x		= 0;
	width	= maxWidth;
	y		= 0;
	height	= newLine ? GetLineDist() * (newLine+1) : -maxHeight;
}

//////////////////////////////////////////////////////////////////////////
